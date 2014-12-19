/*
 * Copyright (C) 2012, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the British Broadcasting Corporation nor the names
 *       of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <cstring>

#include <algorithm>

#include <bmx/rdd9_mxf/RDD9ContentPackage.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define MAX_CONTENT_PACKAGES        250
#define SYSTEM_ITEM_TRACK_INDEX     (uint32_t)(-1)

#define SYS_META_PICTURE_ITEM_FLAG  0x08
#define SYS_META_SOUND_ITEM_FLAG    0x04
#define SYS_META_DATA_ITEM_FLAG     0x02

static const mxfKey MXF_EE_K(EmptyPackageMetadataSet) = MXF_SDTI_CP_PACKAGE_METADATA_KEY(0x00);

static const uint32_t KAG_SIZE = 0x200;
static const uint8_t LLEN      = 4;



static bool compare_element(const RDD9ContentPackageElement *left, const RDD9ContentPackageElement *right)
{
    return left->GetElementType() < right->GetElementType();
}



RDD9ContentPackageElement::RDD9ContentPackageElement(uint32_t track_index, mxfKey element_key)
{
    mTrackIndex = track_index;
    mElementKey = element_key;
    mElementType = PICTURE_ELEMENT;
    mSampleSequence.push_back(1);
    mSampleSize = 0;
    mFixedElementSize = 0;
    mSoundSequenceOffsetSet = false;
    mSoundSequenceOffset = 0;
}

RDD9ContentPackageElement::RDD9ContentPackageElement(uint32_t track_index, mxfKey element_key,
                                                     vector<uint32_t> sample_sequence, uint32_t sample_size)
{
    mTrackIndex = track_index;
    mElementKey = element_key;
    mElementType = SOUND_ELEMENT;
    mSampleSequence = sample_sequence;
    mSampleSize = sample_size;
    mSoundSequenceOffsetSet = false;
    mSoundSequenceOffset = 0;

    // calculate the fixed element size

    uint32_t max_num_samples = 0;
    bool variable_sequence = false;
    size_t i;
    for (i = 0; i < mSampleSequence.size(); i++) {
        if (max_num_samples != 0 && mSampleSequence[i] != max_num_samples)
            variable_sequence = true;
        if (mSampleSequence[i] > max_num_samples)
            max_num_samples = mSampleSequence[i];
    }

    mFixedElementSize = GetKAGAlignedSize(mxfKey_extlen + LLEN + max_num_samples * mSampleSize);

    if (variable_sequence && mFixedElementSize == mxfKey_extlen + LLEN + max_num_samples * mSampleSize) {
        // allow space to include a KLV fill
        mFixedElementSize = GetKAGAlignedSize(mxfKey_extlen + LLEN +
                                              max_num_samples * sample_size +
                                              mxfKey_extlen + LLEN);
    }
}

RDD9ContentPackageElement::RDD9ContentPackageElement(uint32_t track_index, mxfKey element_key,
                                                     uint32_t constant_essence_len, uint32_t max_essence_len)
{
    mTrackIndex = track_index;
    mElementKey = element_key;
    mElementType = DATA_ELEMENT;
    mSampleSequence.push_back(1);
    mSampleSize = 0;
    mFixedElementSize = 0;
    mSoundSequenceOffsetSet = false;
    mSoundSequenceOffset = 0;

    if (constant_essence_len) {
        mFixedElementSize = GetKAGAlignedSize(mxfKey_extlen + LLEN + constant_essence_len);
    }
    else if (max_essence_len) {
        mFixedElementSize = GetKAGAlignedSize(mxfKey_extlen + LLEN + max_essence_len);
        if (mFixedElementSize == mxfKey_extlen + LLEN + max_essence_len) {
            // allow space to include a KLV fill
            mFixedElementSize = GetKAGAlignedSize(mxfKey_extlen + LLEN + max_essence_len +
                                                  mxfKey_extlen + LLEN);
        }
    }
}

void RDD9ContentPackageElement::SetSoundSequenceOffset(uint8_t offset)
{
    mSoundSequenceOffset    = offset;
    mSoundSequenceOffsetSet = true;
}

bool RDD9ContentPackageElement::CheckValidSampleCount(uint32_t num_samples) const
{
    size_t i;
    for (i = 0; i < mSampleSequence.size(); i++) {
        if (mSampleSequence[i] == num_samples)
            return true;
    }

    return false;
}

uint32_t RDD9ContentPackageElement::GetNumSamples(int64_t position) const
{
    if (mElementType == PICTURE_ELEMENT) {
        return 1;
    } else if (mElementType == SOUND_ELEMENT) {
        if (mSoundSequenceOffsetSet)
            return mSampleSequence[(size_t)((position + mSoundSequenceOffset) % mSampleSequence.size())];
        else
            return 0; // unknown
    }
    else
        return 0; // unknown
}

uint32_t RDD9ContentPackageElement::GetElementSize(uint32_t data_size) const
{
    if (mFixedElementSize)
        return mFixedElementSize;
    else
        return GetKAGAlignedSize(mxfKey_extlen + LLEN + data_size);
}

void RDD9ContentPackageElement::Write(File *mxf_file, unsigned char *data, uint32_t size)
{
    uint32_t element_size = GetElementSize(size);

    mxf_file->writeFixedKL(&mElementKey, LLEN, size);
    BMX_CHECK(mxf_file->write(data, size) == size);
    if (element_size > mxfKey_extlen + LLEN + size)
        mxf_file->writeFill(element_size - (mxfKey_extlen + LLEN + size));
}

uint32_t RDD9ContentPackageElement::GetKAGAlignedSize(uint32_t klv_size) const
{
    return (uint32_t)get_kag_aligned_size(klv_size, KAG_SIZE, LLEN);
}

uint32_t RDD9ContentPackageElement::GetKAGFillSize(int64_t klv_size) const
{
    return get_kag_fill_size(klv_size, KAG_SIZE, LLEN);
}



RDD9ContentPackageElementData::RDD9ContentPackageElementData(File *mxf_file, RDD9IndexTable *index_table,
                                                             RDD9ContentPackageElement *element,
                                                             int64_t position)
{
    mMXFFile = mxf_file;
    mIndexTable = index_table;
    mElement = element;
    mPosition = position;
    mNumSamplesWritten = 0;
    mNumSamples = element->GetNumSamples(position);
}

uint32_t RDD9ContentPackageElementData::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(size % num_samples == 0);

    if (mNumSamples == 0) {
        // assume sample count equals input num_samples
        if (!mElement->CheckValidSampleCount(num_samples))
            BMX_EXCEPTION(("Invalid sound sample count %u for sample sequence", num_samples));
        mNumSamples = num_samples;
    }

    BMX_ASSERT(mNumSamples > mNumSamplesWritten);
    uint32_t write_num_samples = mNumSamples - mNumSamplesWritten;
    if (write_num_samples > num_samples)
        write_num_samples = num_samples;
    uint32_t write_size = (size / num_samples) * write_num_samples;

    mData.Append(data, write_size);
    mNumSamplesWritten += write_num_samples;

    return write_num_samples;
}

bool RDD9ContentPackageElementData::IsComplete() const
{
    return mNumSamplesWritten > 0 && mNumSamplesWritten >= mNumSamples;
}

uint32_t RDD9ContentPackageElementData::GetElementSize() const
{
    return mElement->GetElementSize(mData.GetSize());
}

void RDD9ContentPackageElementData::Write()
{
    mElement->Write(mMXFFile, mData.GetBytes(), mData.GetSize());
}

void RDD9ContentPackageElementData::Reset(int64_t new_position)
{
    mData.SetSize(0);
    mPosition = new_position;
    mNumSamplesWritten = 0;
    mNumSamples = mElement->GetNumSamples(new_position);
}



RDD9ContentPackage::RDD9ContentPackage(File *mxf_file, RDD9IndexTable *index_table, bool have_user_timecode,
                                       Rational frame_rate, uint8_t sys_meta_item_flags, vector<RDD9ContentPackageElement*> elements,
                                       int64_t position, Timecode start_timecode)
{
    mMXFFile = mxf_file;
    mIndexTable = index_table;
    mHaveInputUserTimecode = have_user_timecode;
    mStartTimecode = start_timecode;
    mFrameRate = frame_rate;
    mPosition = position;
    mHaveUpdatedIndexTable = false;
    mUserTimecodeSet = false;
    mSysMetaItemFlags = sys_meta_item_flags;


    size_t i;
    for (i = 0; i < elements.size(); i++) {
        mElementData.push_back(new RDD9ContentPackageElementData(mxf_file, index_table, elements[i], position));
        mElementTrackIndexMap[elements[i]->GetTrackIndex()] = mElementData.back();
    }
}

RDD9ContentPackage::~RDD9ContentPackage()
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        delete mElementData[i];
}

void RDD9ContentPackage::Reset(int64_t new_position)
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        mElementData[i]->Reset(new_position);
    mPosition = new_position;
    mHaveUpdatedIndexTable = false;
    mUserTimecodeSet = false;
}

bool RDD9ContentPackage::IsComplete(uint32_t track_index)
{
    if (track_index == SYSTEM_ITEM_TRACK_INDEX)
        return mUserTimecodeSet || !mHaveInputUserTimecode;

    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());
    return mElementTrackIndexMap[track_index]->IsComplete();
}

void RDD9ContentPackage::WriteUserTimecode(Timecode user_timecode)
{
    mUserTimecode    = user_timecode;
    mUserTimecodeSet = true;
}

uint32_t RDD9ContentPackage::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                          uint32_t num_samples)
{
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    return mElementTrackIndexMap[track_index]->WriteSamples(data, size, num_samples);
}

uint32_t RDD9ContentPackage::GetSoundSampleCount() const
{
    uint32_t min_sample_count = 0;
    size_t i;
    for (i = 0; i < mElementData.size(); i++) {
        if ((mElementData[i]->GetElementType() == RDD9ContentPackageElement::SOUND_ELEMENT) &&
            (min_sample_count == 0 || mElementData[i]->GetNumSamplesWritten() < min_sample_count))
        {
            min_sample_count = mElementData[i]->GetNumSamplesWritten();
            if (min_sample_count == 0)
                break;
        }
    }

    return min_sample_count;
}

bool RDD9ContentPackage::IsComplete()
{
    if (mHaveInputUserTimecode && !mUserTimecodeSet)
        return false;

    size_t i;
    for (i = 0; i < mElementData.size(); i++) {
        if (!mElementData[i]->IsComplete())
            return false;
    }

    return true;
}

void RDD9ContentPackage::UpdateIndexTable()
{
    if (mHaveUpdatedIndexTable)
        return;

    uint32_t size = 0;
    vector<uint32_t> element_sizes;

    // system item
    element_sizes.push_back(KAG_SIZE);
    size += KAG_SIZE;

    // picture and sound elements
    size_t i;
    for (i = 0; i < mElementData.size(); i++) {
        element_sizes.push_back(mElementData[i]->GetElementSize());
        size += element_sizes.back();
    }

    mIndexTable->UpdateIndex(size, element_sizes);
    mHaveUpdatedIndexTable = true;
}

void RDD9ContentPackage::Write()
{
    BMX_ASSERT(mHaveUpdatedIndexTable);

    WriteSystemItem();

    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        mElementData[i]->Write();
}

void RDD9ContentPackage::WriteSystemItem()
{
    static const uint32_t SYSTEM_ITEM_METADATA_PACK_SIZE = 7 + 16 + 17 + 17;

    mMXFFile->writeFixedKL(&MXF_EE_K(SDTI_CP_System_Pack), LLEN, SYSTEM_ITEM_METADATA_PACK_SIZE);

    // system metadata bitmap = 0x50
    // b7 = 0 (FEC not used)
    // b6 = 1 (SMPTE Universal label)
    // b5 = 0 (creation date/time stamp)
    // b4 = 1 (user date/time stamp)
    // b3 = 0/1 (picture item)
    // b2 = 0/1 (sound item)
    // b1 = 0/1 (data item)
    // b0 = 0 (control element)

    // core fields
    mMXFFile->writeUInt8(0x50 | mSysMetaItemFlags);                     // system metadata bitmap
    mMXFFile->writeUInt8(get_system_item_cp_rate(mFrameRate));          // content package rate
    mMXFFile->writeUInt8(0x00);                                         // content package type (default)
    mMXFFile->writeUInt16(0x0000);                                      // channel handle (default)
    mMXFFile->writeUInt16((uint16_t)(mPosition & 0xffff));              // continuity count

    // SMPTE Universal Label
    mMXFFile->writeUL(&MXF_EC_L(MultipleWrappings));

    // (null) Package creation date / time stamp
    unsigned char ts_bytes[17];
    memset(ts_bytes, 0, sizeof(ts_bytes));
    BMX_CHECK(mMXFFile->write(ts_bytes, sizeof(ts_bytes)) == sizeof(ts_bytes));

    // User date / time stamp
    Timecode user_timecode;
    ts_bytes[0] = 0x81; // SMPTE 12-M timecode
    if (mHaveInputUserTimecode) {
        user_timecode = mUserTimecode;
    } else if (!mStartTimecode.IsInvalid()) {
        user_timecode = mStartTimecode;
        user_timecode.AddOffset(mPosition);
    } else {
        user_timecode.Init(get_rounded_tc_base(mFrameRate), false, mPosition);
    }
    encode_smpte_timecode(user_timecode, false, &ts_bytes[1], sizeof(ts_bytes) - 1);
    BMX_CHECK(mMXFFile->write(ts_bytes, sizeof(ts_bytes)) == sizeof(ts_bytes));


    // empty Package Metadata Set
    mMXFFile->writeFixedKL(&MXF_EE_K(EmptyPackageMetadataSet), LLEN, 0);


    // align to KAG
    mMXFFile->writeFill(KAG_SIZE - (mxfKey_extlen + LLEN + SYSTEM_ITEM_METADATA_PACK_SIZE +
                                    mxfKey_extlen + LLEN));
}



RDD9ContentPackageManager::RDD9ContentPackageManager(File *mxf_file, RDD9IndexTable *index_table,
                                                     Rational frame_rate)
{
    mMXFFile = mxf_file;
    mIndexTable = index_table;
    mFrameRate = frame_rate;
    mHaveInputUserTimecode = false;
    mSoundSequenceOffsetSet = false;
    mSoundSequenceOffset = 0;
    mPosition = 0;
    mSysMetaItemFlags = 0;
}

RDD9ContentPackageManager::~RDD9ContentPackageManager()
{
    size_t i;
    for (i = 0; i < mElements.size(); i++)
        delete mElements[i];

    for (i = 0; i < mContentPackages.size(); i++)
        delete mContentPackages[i];
    for (i = 0; i < mFreeContentPackages.size(); i++)
        delete mFreeContentPackages[i];
}

void RDD9ContentPackageManager::SetHaveInputUserTimecode(bool enable)
{
    mHaveInputUserTimecode = enable;
}

void RDD9ContentPackageManager::SetStartTimecode(Timecode start_timecode)
{
    if (!start_timecode.IsInvalid() && start_timecode.GetRoundedTCBase() == get_rounded_tc_base(mFrameRate))
        mStartTimecode = start_timecode;
    else
        mStartTimecode.SetInvalid();
}

void RDD9ContentPackageManager::SetSoundSequenceOffset(uint8_t offset)
{
    BMX_CHECK(!mSoundSequenceOffsetSet || offset == mSoundSequenceOffset);

    mSoundSequenceOffset    = offset;
    mSoundSequenceOffsetSet = true;
}

void RDD9ContentPackageManager::RegisterPictureTrackElement(uint32_t track_index, mxfKey element_key)
{
    mElements.push_back(new RDD9ContentPackageElement(track_index, element_key));
    mElementTrackIndexMap[track_index] = mElements.back();
    mSysMetaItemFlags |= SYS_META_PICTURE_ITEM_FLAG;
}

void RDD9ContentPackageManager::RegisterSoundTrackElement(uint32_t track_index, mxfKey element_key,
                                                          vector<uint32_t> sample_sequence, uint32_t sample_size)
{
    mElements.push_back(new RDD9ContentPackageElement(track_index, element_key, sample_sequence, sample_size));
    mElementTrackIndexMap[track_index] = mElements.back();
    mSysMetaItemFlags |= SYS_META_SOUND_ITEM_FLAG;
}

void RDD9ContentPackageManager::RegisterDataTrackElement(uint32_t track_index, mxfKey element_key,
                                                         uint32_t constant_essence_len, uint32_t max_essence_len)
{
    mElements.push_back(new RDD9ContentPackageElement(track_index, element_key, constant_essence_len, max_essence_len));
    mElementTrackIndexMap[track_index] = mElements.back();
    mSysMetaItemFlags |= SYS_META_DATA_ITEM_FLAG;
}

void RDD9ContentPackageManager::PrepareWrite()
{
    // order elements: picture - sound - data elements
    stable_sort(mElements.begin(), mElements.end(), compare_element);

    // check sound elements have identical sequences
    bool valid_sequences = true;
    size_t i;

    // find the first audio track
    for (i = 0; i < mElements.size(); i++) {
        if (mElements[i]->GetElementType() == RDD9ContentPackageElement::SOUND_ELEMENT)
            break;
    }

    if (i + 1 < mElements.size()) {
        mSoundSequence = mElements[i]->GetSampleSequence();
        for (i = i + 1; i < mElements.size() && valid_sequences; i++) {
            // finished the audio tracks?
            if (mElements[i]->GetElementType() != RDD9ContentPackageElement::SOUND_ELEMENT)
                break;

            if (mSoundSequence.size() != mElements[i]->GetSampleSequence().size()) {
                valid_sequences = false;
                break;
            }

            size_t j;
            for (j = 0; j < mSoundSequence.size(); j++) {
                if (mSoundSequence[j] != mElements[i]->GetSampleSequence()[j]) {
                    valid_sequences = false;
                    break;
                }
            }
        }
    }
    BMX_CHECK_M(valid_sequences, ("Sound tracks have different number of samples per frame"));

    // clean mSoundSequenceOffset
    if (mSoundSequence.size() == 1) {
        mSoundSequenceOffset    = 0;
        mSoundSequenceOffsetSet = true;
    } else if (mSoundSequenceOffsetSet) {
        mSoundSequenceOffset   %= mSoundSequence.size();
    }

    if (mSoundSequenceOffsetSet) {
        for (i = 0; i < mElements.size(); i++)
            mElements[i]->SetSoundSequenceOffset(mSoundSequenceOffset);
    }

    mIndexTable->RegisterSystemItem();
}

void RDD9ContentPackageManager::WriteUserTimecode(Timecode user_timecode)
{
    if (mHaveInputUserTimecode)
        mContentPackages[GetCurrentContentPackage(SYSTEM_ITEM_TRACK_INDEX)]->WriteUserTimecode(user_timecode);
}

void RDD9ContentPackageManager::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                             uint32_t num_samples)
{
    BMX_ASSERT(data && size && num_samples);

    size_t cp_index = GetCurrentContentPackage(track_index);

    uint32_t sample_size = mElementTrackIndexMap[track_index]->GetSampleSize();
    if (sample_size == 0)
        sample_size = size / num_samples;
    BMX_CHECK(size >= sample_size * num_samples);

    const unsigned char *data_ptr = data;
    uint32_t rem_size = sample_size * num_samples;
    uint32_t rem_num_samples = num_samples;
    while (rem_num_samples > 0) {
        if (cp_index >= mContentPackages.size())
            cp_index = CreateContentPackage();

        uint32_t num_written = mContentPackages[cp_index]->WriteSamples(track_index, data_ptr, rem_size, rem_num_samples);
        rem_num_samples -= num_written;
        rem_size -= num_written * sample_size;
        data_ptr += num_written * sample_size;

        cp_index++;
    }
}

bool RDD9ContentPackageManager::HaveContentPackage(bool final_write)
{
    if (final_write && !mSoundSequenceOffsetSet)
        CalcSoundSequenceOffset(true);

    return !mContentPackages.empty() && mContentPackages.front()->IsComplete() && mSoundSequenceOffsetSet;
}

void RDD9ContentPackageManager::WriteNextContentPackage()
{
    BMX_ASSERT(HaveContentPackage(false));

    mContentPackages.front()->UpdateIndexTable();
    mContentPackages.front()->Write();

    mFreeContentPackages.push_back(mContentPackages.front());
    mContentPackages.pop_front();

    mPosition++;
}

size_t RDD9ContentPackageManager::GetCurrentContentPackage(uint32_t track_index)
{
    size_t cp_index = 0;
    while (cp_index < mContentPackages.size() &&
           mContentPackages[cp_index]->IsComplete(track_index))
    {
        cp_index++;
    }

    return cp_index;
}

size_t RDD9ContentPackageManager::CreateContentPackage()
{
    if (!mSoundSequenceOffsetSet)
        CalcSoundSequenceOffset(false);

    size_t cp_index = mContentPackages.size();

    if (mFreeContentPackages.empty()) {
        BMX_CHECK(mContentPackages.size() < MAX_CONTENT_PACKAGES);
        mContentPackages.push_back(new RDD9ContentPackage(mMXFFile, mIndexTable, mHaveInputUserTimecode, mFrameRate,
                                                          mSysMetaItemFlags, mElements, mPosition + cp_index,
                                                          mStartTimecode));
    } else {
        mContentPackages.push_back(mFreeContentPackages.back());
        mFreeContentPackages.pop_back();
        mContentPackages.back()->Reset(mPosition + cp_index);
    }

    return cp_index;
}

void RDD9ContentPackageManager::CalcSoundSequenceOffset(bool final_write)
{
    vector<uint32_t> input_sequence;
    size_t i;
    for (i = 0; i < mContentPackages.size(); i++) {
        uint32_t sample_count = mContentPackages[i]->GetSoundSampleCount();
        if (sample_count == 0)
            break; // no more sound data
        input_sequence.push_back(sample_count);
    }

    uint8_t offset = 0;
    while (offset < mSoundSequence.size()) {
        for (i = 0; i < input_sequence.size(); i++) {
            if (input_sequence[i] != mSoundSequence[(offset + i) % mSoundSequence.size()])
                break;
        }
        if (i >= input_sequence.size())
            break;

        offset++;
    }
    BMX_CHECK_M(offset < mSoundSequence.size(), ("Invalid sound sample sequence"));

    if (final_write || input_sequence.size() >= mSoundSequence.size()) {
        mSoundSequenceOffset    = offset;
        mSoundSequenceOffsetSet = true;
        for (i = 0; i < mElements.size(); i++)
            mElements[i]->SetSoundSequenceOffset(mSoundSequenceOffset);
    } else {
        mSoundSequenceOffset    = 0;
        mSoundSequenceOffsetSet = false;
    }
}

