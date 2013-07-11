/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#include <algorithm>

#include <bmx/mxf_op1a/OP1AContentPackage.h>
#include <bmx/MXFUtils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define MAX_CONTENT_PACKAGES    250

#define FW_ESS_ELEMENT_LLEN     4



static bool compare_element(const OP1AContentPackageElement *left, const OP1AContentPackageElement *right)
{
    // positioning data before picture and sound
    if (left->data_def == MXF_DATA_DDEF || right->data_def == MXF_DATA_DDEF)
        return left->data_def == MXF_DATA_DDEF && right->data_def != MXF_DATA_DDEF;
    else
        return left->data_def < right->data_def;
}



OP1AContentPackageElement::OP1AContentPackageElement(uint32_t track_index_, MXFDataDefEnum data_def_,
                                                     mxfKey element_key_, uint32_t kag_size_, uint8_t min_llen_)
{
    track_index = track_index_;
    element_key = element_key_;
    kag_size = kag_size_;
    min_llen = min_llen_;
    essence_llen = FW_ESS_ELEMENT_LLEN;
    data_def = data_def_;
    is_cbe = true;
    is_frame_wrapped = true;
    sample_size = 0;
    fixed_element_size = 0;
    first_sample_size = 0;
    nonfirst_sample_size = 0;
}

void OP1AContentPackageElement::SetSampleSequence(const vector<uint32_t> &sample_sequence_, uint32_t sample_size_)
{
    sample_sequence = sample_sequence_;
    sample_size = sample_size_;

    uint32_t max_sample_sequence = 0;
    bool variable_sequence = false;
    size_t i;
    for (i = 0; i < sample_sequence.size(); i++) {
        if (max_sample_sequence != 0 && sample_sequence[i] != max_sample_sequence)
            variable_sequence = true;
        if (sample_sequence[i] > max_sample_sequence)
            max_sample_sequence = sample_sequence[i];
    }

    if (variable_sequence)
        SetMaxEssenceLen(max_sample_sequence * sample_size);
    else
        SetConstantEssenceLen(max_sample_sequence * sample_size);
}

void OP1AContentPackageElement::SetConstantEssenceLen(uint32_t constant_essence_len)
{
    fixed_element_size = GetKAGAlignedSize(mxfKey_extlen + essence_llen + constant_essence_len);
}

void OP1AContentPackageElement::SetMaxEssenceLen(uint32_t max_essence_len)
{
    fixed_element_size = GetKAGAlignedSize(mxfKey_extlen + essence_llen + max_essence_len);

    if (fixed_element_size == mxfKey_extlen + essence_llen + max_essence_len) {
        // allow space to include a KLV fill
        fixed_element_size = GetKAGAlignedSize(mxfKey_extlen + essence_llen + max_essence_len +
                                               mxfKey_extlen + min_llen);
    }
}

uint32_t OP1AContentPackageElement::GetNumSamples(int64_t position) const
{
    if (!is_frame_wrapped)
        return 0;
    else if (sample_sequence.empty())
        return 1;
    else
        return sample_sequence[(size_t)(position % sample_sequence.size())];
}

uint32_t OP1AContentPackageElement::GetKAGAlignedSize(uint32_t klv_size)
{
    return klv_size + GetKAGFillSize(klv_size);
}

uint32_t OP1AContentPackageElement::GetKAGFillSize(int64_t klv_size)
{
    // assuming the partition pack is aligned to the kag working from the first byte of the file

    uint32_t fill_size = 0;
    uint32_t klv_in_kag_size = (uint32_t)(klv_size % kag_size);
    if (klv_in_kag_size > 0) {
        fill_size = kag_size - klv_in_kag_size;
        while (fill_size < (uint32_t)min_llen + mxfKey_extlen)
            fill_size += kag_size;
    }

    return fill_size;
}

void OP1AContentPackageElement::WriteKL(File *mxf_file, int64_t essence_len)
{
    mxf_file->writeFixedKL(&element_key, essence_llen, essence_len);
}

void OP1AContentPackageElement::WriteFill(File *mxf_file, int64_t essence_len)
{
    uint32_t fill_size = GetKAGFillSize(mxfKey_extlen + essence_llen + essence_len);
    if (fill_size > 0)
        mxf_file->writeFill(fill_size);
}



OP1AContentPackageElementData::OP1AContentPackageElementData(File *mxf_file, OP1AIndexTable *index_table,
                                                             OP1AContentPackageElement *element,
                                                             int64_t position)
{
    mMXFFile = mxf_file;
    mIndexTable = index_table;
    mElement = element;
    mNumSamplesWritten = 0;
    mNumSamples = element->GetNumSamples(position);
    mTotalWriteSize = 0;
    mElementStartPos = 0;
}

uint32_t OP1AContentPackageElementData::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(size % num_samples == 0);

    uint32_t write_num_samples;
    uint32_t write_size;
    if (mElement->is_frame_wrapped) {
        write_num_samples = mNumSamples - mNumSamplesWritten;
        if (write_num_samples > num_samples)
            write_num_samples = num_samples;
        write_size = (size / num_samples) * write_num_samples;
    } else {
        write_num_samples = num_samples;
        write_size = size;
    }

    if (mElement->is_frame_wrapped || mTotalWriteSize == 0) {
        mData.Append(data, write_size);
    } else {
        BMX_CHECK(mMXFFile->write(data, write_size) == write_size);
        mTotalWriteSize += write_size;
        mIndexTable->UpdateIndex(write_size, write_num_samples);
    }
    mNumSamplesWritten += write_num_samples;

    return write_num_samples;
}

void OP1AContentPackageElementData::WriteSample(const CDataBuffer *data_array, uint32_t array_size)
{
    if (mElement->is_frame_wrapped || mTotalWriteSize == 0) {
        uint32_t size = dba_get_total_size(data_array, array_size);
        mData.Grow(size);
        dba_copy_data(mData.GetBytesAvailable(), mData.GetSizeAvailable(), data_array, array_size);
        mData.IncrementSize(size);
    } else {
        uint32_t write_size = 0;
        uint32_t i;
        for (i = 0; i < array_size; i++) {
            BMX_CHECK(mMXFFile->write(data_array[i].data, data_array[i].size) == data_array[i].size);
            write_size += data_array[i].size;
            mTotalWriteSize += data_array[i].size;
        }
        mIndexTable->UpdateIndex(write_size, 1);
    }

    mNumSamplesWritten++;
}

bool OP1AContentPackageElementData::IsReady() const
{
    return ( mElement->is_frame_wrapped && mNumSamplesWritten >= mNumSamples) ||
           (!mElement->is_frame_wrapped && mData.GetSize() > 0);
}

uint32_t OP1AContentPackageElementData::GetWriteSize() const
{
    if (!mElement->is_frame_wrapped) {
        return mData.GetSize();
    } else if (mElement->fixed_element_size) {
        uint32_t essence_write_size = mxfKey_extlen + mElement->essence_llen + mData.GetSize();
        if (essence_write_size != mElement->fixed_element_size) {
            if (essence_write_size > mElement->fixed_element_size) {
                BMX_EXCEPTION(("Essence KLV element size %u exceeds fixed size %u",
                               essence_write_size, mElement->fixed_element_size));
            } else if (essence_write_size + mxfKey_extlen + mElement->min_llen > mElement->fixed_element_size) {
                BMX_EXCEPTION(("Essence KLV element size %u does not allow insertion of KLV fill (min %u) for fixed size %u",
                               essence_write_size, mxfKey_extlen + mElement->min_llen, mElement->fixed_element_size));
            }
        }
        return mElement->fixed_element_size;
    } else {
        return mElement->GetKAGAlignedSize(mxfKey_extlen + mElement->essence_llen + mData.GetSize());
    }
}

uint32_t OP1AContentPackageElementData::Write()
{
    uint32_t write_size = GetWriteSize();

    if (mElement->is_frame_wrapped) {
        mElement->WriteKL(mMXFFile, mData.GetSize());
        BMX_CHECK(mMXFFile->write(mData.GetBytes(), mData.GetSize()) == mData.GetSize());
        if (write_size > mxfKey_extlen + mElement->essence_llen + mData.GetSize())
            mMXFFile->writeFill(write_size - (mxfKey_extlen + mElement->essence_llen + mData.GetSize()));
        else
            BMX_ASSERT(write_size == mxfKey_extlen + mElement->essence_llen + mData.GetSize());
    } else {
        BMX_ASSERT(mTotalWriteSize == 0);
        mElementStartPos = mMXFFile->tell();
        mElement->WriteKL(mMXFFile, 0);
        BMX_CHECK(mMXFFile->write(mData.GetBytes(), mData.GetSize()) == mData.GetSize());
        mData.SetSize(0);
    }

    mTotalWriteSize += write_size;

    return write_size;
}

void OP1AContentPackageElementData::CompleteWrite()
{
    if (!mElement->is_frame_wrapped && mTotalWriteSize > 0) {
        int64_t pos = mMXFFile->tell();

        mMXFFile->seek(mElementStartPos, SEEK_SET);
        mElement->WriteKL(mMXFFile, mTotalWriteSize);
        mElement->WriteFill(mMXFFile, mTotalWriteSize);

        mMXFFile->seek(pos, SEEK_SET);
    }
}

void OP1AContentPackageElementData::Reset(int64_t new_position)
{
    mData.SetSize(0);
    mNumSamplesWritten = 0;
    mNumSamples = mElement->GetNumSamples(new_position);
    mTotalWriteSize = 0;
    mElementStartPos = 0;
}



OP1AContentPackage::OP1AContentPackage(File *mxf_file, OP1AIndexTable *index_table,
                                       vector<OP1AContentPackageElement*> elements, int64_t position)
{
    mIndexTable = index_table;
    mFrameWrapped = true;
    mHaveUpdatedIndexTable = false;

    if (!elements.empty())
        mFrameWrapped = elements[0]->is_frame_wrapped;

    size_t i;
    for (i = 0; i < elements.size(); i++) {
        mElementData.push_back(new OP1AContentPackageElementData(mxf_file, index_table, elements[i], position));
        mElementTrackIndexMap[elements[i]->track_index] = mElementData.back();
    }
}

OP1AContentPackage::~OP1AContentPackage()
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        delete mElementData[i];
}

void OP1AContentPackage::Reset(int64_t new_position)
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        mElementData[i]->Reset(new_position);
    mHaveUpdatedIndexTable = false;
}

bool OP1AContentPackage::IsReady(uint32_t track_index)
{
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    return mElementTrackIndexMap[track_index]->IsReady();
}

uint32_t OP1AContentPackage::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                          uint32_t num_samples)
{
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    return mElementTrackIndexMap[track_index]->WriteSamples(data, size, num_samples);
}

void OP1AContentPackage::WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size)
{
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    mElementTrackIndexMap[track_index]->WriteSample(data_array, array_size);
}

bool OP1AContentPackage::IsReady()
{
    size_t i;
    for (i = 0; i < mElementData.size(); i++) {
        if (!mElementData[i]->IsReady())
            return false;
    }

    return true;
}

void OP1AContentPackage::UpdateIndexTable()
{
    if (mHaveUpdatedIndexTable)
        return;

    if (mFrameWrapped) {
        uint32_t size = 0;
        vector<uint32_t> element_sizes;

        size_t i;
        for (i = 0; i < mElementData.size(); i++) {
            element_sizes.push_back(mElementData[i]->GetWriteSize());
            size += element_sizes.back();
        }

        mIndexTable->UpdateIndex(size, element_sizes);
    } else {
        mIndexTable->UpdateIndex(mElementData[0]->GetWriteSize(),
                                 mElementData[0]->GetNumSamplesWritten());
    }

    mHaveUpdatedIndexTable = true;
}

uint32_t OP1AContentPackage::Write()
{
    BMX_ASSERT(mHaveUpdatedIndexTable);

    uint32_t size = 0;
    size_t i;
    for (i = 0; i < mElementData.size(); i++)
        size += mElementData[i]->Write();

    return size;
}

void OP1AContentPackage::CompleteWrite()
{
    if (!mFrameWrapped)
        mElementData[0]->CompleteWrite();
}



OP1AContentPackageManager::OP1AContentPackageManager(File *mxf_file, OP1AIndexTable *index_table, uint32_t kag_size,
                                                     uint8_t min_llen)
{
    // check assumption that filler will have llen == min_llen
    BMX_ASSERT(min_llen >= mxf_get_llen(0, kag_size + mxfKey_extlen + min_llen));

    mMXFFile = mxf_file;
    mIndexTable = index_table;
    mKAGSize = kag_size;
    mMinLLen = min_llen;
    mFrameWrapped = true;
    mPosition = 0;
}

OP1AContentPackageManager::~OP1AContentPackageManager()
{
    size_t i;
    for (i = 0; i < mElements.size(); i++)
        delete mElements[i];

    for (i = 0; i < mContentPackages.size(); i++)
        delete mContentPackages[i];
    for (i = 0; i < mFreeContentPackages.size(); i++)
        delete mFreeContentPackages[i];
}

void OP1AContentPackageManager::SetClipWrapped(bool enable)
{
    mFrameWrapped = !enable;
}

void OP1AContentPackageManager::RegisterPictureTrackElement(uint32_t track_index, mxfKey element_key, bool is_cbe)
{
    BMX_ASSERT(mFrameWrapped);

    OP1AContentPackageElement *element = new OP1AContentPackageElement(track_index, MXF_PICTURE_DDEF, element_key,
                                                                       mKAGSize, mMinLLen);
    element->is_cbe = is_cbe;

    mElements.push_back(element);
    mElementTrackIndexMap[track_index] = element;
}

void OP1AContentPackageManager::RegisterAVCITrackElement(uint32_t track_index, mxfKey element_key,
                                                         uint32_t first_sample_size, uint32_t nonfirst_sample_size)
{
    BMX_ASSERT(mFrameWrapped);

    OP1AContentPackageElement *element = new OP1AContentPackageElement(track_index, MXF_PICTURE_DDEF, element_key,
                                                                       mKAGSize, mMinLLen);
    element->first_sample_size = first_sample_size;
    element->nonfirst_sample_size = nonfirst_sample_size;

    mElements.push_back(element);
    mElementTrackIndexMap[track_index] = element;
}

void OP1AContentPackageManager::RegisterSoundTrackElement(uint32_t track_index, mxfKey element_key,
                                                          vector<uint32_t> sample_sequence, uint32_t sample_size)
{
    BMX_ASSERT(mFrameWrapped);

    OP1AContentPackageElement *element = new OP1AContentPackageElement(track_index, MXF_SOUND_DDEF, element_key,
                                                                       mKAGSize, mMinLLen);
    element->SetSampleSequence(sample_sequence, sample_size);

    mElements.push_back(element);
    mElementTrackIndexMap[track_index] = element;
}

void OP1AContentPackageManager::RegisterSoundTrackElement(uint32_t track_index, mxfKey element_key,
                                                          uint8_t element_llen)
{
    BMX_ASSERT(!mFrameWrapped);

    OP1AContentPackageElement *element = new OP1AContentPackageElement(track_index, MXF_SOUND_DDEF, element_key,
                                                                       mKAGSize, mMinLLen);
    element->is_frame_wrapped = false;
    element->essence_llen = element_llen;

    mElements.push_back(element);
    mElementTrackIndexMap[track_index] = element;
}

void OP1AContentPackageManager::RegisterDataTrackElement(uint32_t track_index, mxfKey element_key,
                                                         uint32_t constant_essence_len, uint32_t max_essence_len)
{
    BMX_ASSERT(mFrameWrapped);

    OP1AContentPackageElement *element = new OP1AContentPackageElement(track_index, MXF_DATA_DDEF, element_key,
                                                                       mKAGSize, mMinLLen);
    if (constant_essence_len)
        element->SetConstantEssenceLen(constant_essence_len);
    else if (max_essence_len)
        element->SetMaxEssenceLen(max_essence_len);
    else
        element->is_cbe = false;

    mElements.push_back(element);
    mElementTrackIndexMap[track_index] = element;
}

void OP1AContentPackageManager::PrepareWrite()
{
    BMX_CHECK_M(mElements.size() <= 1 || mFrameWrapped,
                ("Multiple tracks using clip wrapping is not supported"));

    // order elements: data - picture - sound
    stable_sort(mElements.begin(), mElements.end(), compare_element);

    // check sound elements have identical sequences
    bool valid_sequences = true;
    size_t i;
    for (i = 0; i < mElements.size(); i++) {
        if (mElements[i]->data_def == MXF_SOUND_DDEF)
            break;
    }
    if (i + 1 < mElements.size()) {
        vector<uint32_t> first_sample_sequence = mElements[i]->sample_sequence;
        for (i = i + 1; i < mElements.size() && valid_sequences; i++) {
            if (first_sample_sequence.size() != mElements[i]->sample_sequence.size()) {
                valid_sequences = false;
                break;
            }

            size_t j;
            for (j = 0; j < first_sample_sequence.size(); j++) {
                if (first_sample_sequence[j] != mElements[i]->sample_sequence[j]) {
                    valid_sequences = false;
                    break;
                }
            }
        }
    }
    BMX_CHECK_M(valid_sequences, ("Sound tracks have different number of samples per frame"));
}

void OP1AContentPackageManager::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                             uint32_t num_samples)
{
    BMX_ASSERT(data && size && num_samples);
    BMX_ASSERT(mElementTrackIndexMap.find(track_index) != mElementTrackIndexMap.end());

    size_t cp_index = GetCurrentContentPackage(track_index);

    uint32_t sample_size = mElementTrackIndexMap[track_index]->sample_size;
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

        if (mFrameWrapped)
            cp_index++;
    }
}

void OP1AContentPackageManager::WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size)
{
    BMX_ASSERT(data_array && array_size);

    size_t cp_index = GetCurrentContentPackage(track_index);

    if (cp_index >= mContentPackages.size())
        cp_index = CreateContentPackage();

    mContentPackages[cp_index]->WriteSample(track_index, data_array, array_size);
}

bool OP1AContentPackageManager::HaveContentPackage() const
{
    return !mContentPackages.empty() && mContentPackages.front()->IsReady();
}

bool OP1AContentPackageManager::HaveContentPackages(size_t num) const
{
    size_t i;
    for (i = 0; i < mContentPackages.size() && i < num; i++) {
        if (!mContentPackages[i]->IsReady())
            break;
    }

    return i >= num;
}

void OP1AContentPackageManager::UpdateIndexTable(size_t num_content_packages)
{
    size_t i;
    for (i = 0; i < mContentPackages.size() && i < num_content_packages; i++) {
        BMX_ASSERT(mContentPackages[i]->IsReady());
        mContentPackages[i]->UpdateIndexTable();
    }
    BMX_ASSERT(i == num_content_packages);
}

void OP1AContentPackageManager::WriteNextContentPackage()
{
    BMX_ASSERT(HaveContentPackage());

    mContentPackages.front()->UpdateIndexTable();
    mContentPackages.front()->Write();

    if (mFrameWrapped) {
        mFreeContentPackages.push_back(mContentPackages.front());
        mContentPackages.pop_front();
    }

    mPosition++;
}

void OP1AContentPackageManager::CompleteWrite()
{
    if (!mFrameWrapped && !mContentPackages.empty())
        mContentPackages.front()->CompleteWrite();
}

size_t OP1AContentPackageManager::GetCurrentContentPackage(uint32_t track_index)
{
    size_t cp_index = 0;
    if (mFrameWrapped) {
        while (cp_index < mContentPackages.size() &&
               mContentPackages[cp_index]->IsReady(track_index))
        {
            cp_index++;
        }
    }

    return cp_index;
}

size_t OP1AContentPackageManager::CreateContentPackage()
{
    size_t cp_index = mContentPackages.size();

    if (mFreeContentPackages.empty()) {
        BMX_CHECK(mContentPackages.size() < MAX_CONTENT_PACKAGES);
        mContentPackages.push_back(new OP1AContentPackage(mMXFFile, mIndexTable, mElements, mPosition + cp_index));
    } else {
        mContentPackages.push_back(mFreeContentPackages.back());
        mFreeContentPackages.pop_back();
        mContentPackages.back()->Reset(mPosition + cp_index);
    }

    return cp_index;
}

