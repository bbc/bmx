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

#include <cstring>

#include <bmx/d10_mxf/D10ContentPackage.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


// some number > max content packages to buffer
#define MAX_CONTENT_PACKAGES    250


static const uint32_t SYSTEM_ITEM_METADATA_PACK_SIZE = 7 + 16 + 17 + 17;

static const mxfKey MXF_EE_K(EmptyPackageMetadataSet) = MXF_SDTI_CP_PACKAGE_METADATA_KEY(0x00);

static const mxfKey PICTURE_ELEMENT_KEY = MXF_D10_PICTURE_EE_K(0x00);
static const mxfKey SOUND_ELEMENT_KEY   = MXF_D10_SOUND_EE_K(0x00);

static const uint8_t LLEN = 4;

static const uint32_t KAG_SIZE = 0x200;



D10ContentPackageInfo::D10ContentPackageInfo()
{
    is_25hz = true;
    memset(&essence_container_ul, 0, sizeof(essence_container_ul));
    mute_sound_flags = 0;
    invalid_sound_flags = 0;
    have_input_user_timecode = false;
    picture_track_index = (uint32_t)(-1);
    picture_sample_size = 0;
    sound_sequence_offset_set = false;
    sound_sequence_offset = 0;
    max_sound_sample_count = 0;
    sound_sample_size = 0;
    system_item_size = 0;
    picture_item_size = 0;
    sound_item_size = 0;
}



D10ContentPackage::D10ContentPackage(D10ContentPackageInfo *info)
{
    mInfo = info;
    mUserTimecodeSet = false;

    mSoundSequenceIndex = 0;
    mSoundSampleCount = 0;

    mPosition = 0;
}

D10ContentPackage::~D10ContentPackage()
{
}

void D10ContentPackage::Reset(int64_t position)
{
    mUserTimecodeSet = false;
    mPictureData.SetSize(0);
    if (!mInfo->sound_sequence_offset_set) {
        mSoundSequenceIndex = 0;
        mSoundSampleCount = 0;
    } else {
        mSoundSequenceIndex = (size_t)((position + mInfo->sound_sequence_offset) % mInfo->sound_sample_sequence.size());
        mSoundSampleCount = mInfo->sound_sample_sequence[mSoundSequenceIndex];
    }
    mPosition = position;


    // initialise empty aes3 sound data and mSoundChannelSampleCount

    if (mSoundData.GetAllocatedSize() == 0) {
        mSoundData.Allocate(mInfo->max_sound_sample_count * 4 * 8 + 4);

        unsigned char *output = mSoundData.GetBytes();
        memset(output, 0, 4);
        output += 4;
        uint32_t s;
        uint8_t c;
        for (s = 0; s < mInfo->max_sound_sample_count; s++) {
            for (c = 0; c < 8; c++) {
                output[0] = c;
                output[1] = 0;
                output[2] = 0;
                output[3] = 0;

                output += 4;
            }
        }
    }

    unsigned char *output = mSoundData.GetBytes();
    output[3] = mInfo->mute_sound_flags; // byte 3 channel valid flags
    map<uint32_t, uint8_t>::const_iterator iter;
    for (iter = mInfo->sound_channels.begin(); iter != mInfo->sound_channels.end(); iter++) {
        mSoundChannelSampleCount[iter->first] = 0;
        output[3] |= (1 << iter->second);
    }
    output[3] &= ~mInfo->invalid_sound_flags;
}

void D10ContentPackage::SetUserTimecode(Timecode user_timecode)
{
    BMX_ASSERT(!mUserTimecodeSet);

    mUserTimecode = user_timecode;
    mUserTimecodeSet = true;
}

bool D10ContentPackage::IsComplete(uint32_t track_index)
{
    if (track_index == mInfo->picture_track_index)
        return mPictureData.GetSize() == mInfo->picture_sample_size;

    BMX_ASSERT(mSoundChannelSampleCount.find(track_index) != mSoundChannelSampleCount.end());
    return mSoundSampleCount > 0 && mSoundChannelSampleCount[track_index] == mSoundSampleCount;
}

uint32_t D10ContentPackage::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                         uint32_t num_samples)
{
    uint32_t write_num_samples;

    if (track_index == mInfo->picture_track_index) {
        write_num_samples = 1;

        BMX_CHECK(size >= mInfo->picture_sample_size);
        mPictureData.CopyBytes(data, write_num_samples * mInfo->picture_sample_size);
    } else {
        BMX_ASSERT(mSoundChannelSampleCount.find(track_index) != mSoundChannelSampleCount.end());

        if (!mInfo->sound_sequence_offset_set) {
            // this call's num_samples equals content package sound sample count
            if (mSoundSampleCount == 0)
                mSoundSampleCount = num_samples;
            else
                BMX_CHECK(num_samples == mSoundSampleCount);
        } else {
            BMX_ASSERT(mSoundSampleCount > 0);
        }

        uint32_t output_start_sample = mSoundChannelSampleCount[track_index];
        write_num_samples = mSoundSampleCount - output_start_sample;
        if (write_num_samples > num_samples)
            write_num_samples = num_samples;

        BMX_CHECK(size >= write_num_samples * mInfo->sound_sample_size);
        CopySoundSamples(data, write_num_samples, mInfo->sound_channels[track_index], output_start_sample);
        mSoundChannelSampleCount[track_index] += write_num_samples;
    }

    return write_num_samples;
}

void D10ContentPackage::WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size)
{
    // TODO: add sound support
    BMX_ASSERT(track_index == mInfo->picture_track_index);

    uint32_t size = dba_get_total_size(data_array, array_size);
    BMX_CHECK(size == mInfo->picture_sample_size);

    mPictureData.SetSize(0);
    mPictureData.Grow(size);
    dba_copy_data(mPictureData.GetBytesAvailable(), mPictureData.GetSizeAvailable(), data_array, array_size);
    mPictureData.SetSize(size);
}

bool D10ContentPackage::IsComplete()
{
    if (mInfo->have_input_user_timecode && !mUserTimecodeSet)
        return false;

    if (mPictureData.GetSize() != mInfo->picture_sample_size)
        return false;

    if (!mSoundChannelSampleCount.empty() && mSoundSampleCount == 0)
        return false;

    map<uint32_t, uint32_t>::const_iterator iter;
    for (iter = mSoundChannelSampleCount.begin(); iter != mSoundChannelSampleCount.end(); iter++) {
        if (iter->second != mSoundSampleCount)
            return false;
    }

    return true;
}

void D10ContentPackage::Write(File *mxf_file)
{
    BMX_ASSERT(IsComplete());

    // finalize data

    BMX_ASSERT(mInfo->sound_sequence_offset_set);

    mSoundSequenceIndex = (size_t)((mPosition + mInfo->sound_sequence_offset) % mInfo->sound_sample_sequence.size());
    BMX_ASSERT(mSoundSampleCount == mInfo->sound_sample_sequence[mSoundSequenceIndex]);

    mSoundData.SetSize(mSoundSampleCount * 4 * 8 + 4);

    unsigned char *output = mSoundData.GetBytes();
    output[0] = (unsigned char)(mSoundSequenceIndex & 0x07);      // FVUCP Valid Flag == 0 (false) and 5-sequence count
    output[1] = (unsigned char)( mSoundSampleCount       & 0xff); // samples per frame (LSB)
    output[2] = (unsigned char)((mSoundSampleCount >> 8) & 0xff); // samples per frame (MSB)


    // write

    uint32_t size = WriteSystemItem(mxf_file);
    mxf_file->writeFill(mInfo->system_item_size - size);

    mxf_file->writeFixedKL(&PICTURE_ELEMENT_KEY, LLEN, mPictureData.GetSize());
    BMX_CHECK(mxf_file->write(mPictureData.GetBytes(), mPictureData.GetSize()) == mPictureData.GetSize());
    mxf_file->writeFill(mInfo->picture_item_size - (mxfKey_extlen + LLEN + mPictureData.GetSize()));

    mxf_file->writeFixedKL(&SOUND_ELEMENT_KEY, LLEN, mSoundData.GetSize());
    BMX_CHECK(mxf_file->write(mSoundData.GetBytes(), mSoundData.GetSize()) == mSoundData.GetSize());
    mxf_file->writeFill(mInfo->sound_item_size - (mxfKey_extlen + LLEN + mSoundData.GetSize()));
}

void D10ContentPackage::CopySoundSamples(const unsigned char *data, uint32_t num_samples, uint8_t channel,
                                         uint32_t output_start_sample)
{
    const unsigned char *input = data;
    unsigned char *output = mSoundData.GetBytes();
    uint8_t flag = (1 << channel);

    // don'c copy if channel was flagged as invalid or muted
    if (!(flag & output[3]) || (flag & mInfo->mute_sound_flags))
        return;

    output += 4 + output_start_sample * 4 * 8 + channel * 4;

    uint32_t s;
    if (mInfo->sound_sample_size == 3) { // 24-bit
        for (s = 0; s < num_samples; s++) {
            output[0] = channel                  | ((input[0] << 4) & 0xf0);
            output[1] = ((input[0] >> 4) & 0x0f) | ((input[1] << 4) & 0xf0);
            output[2] = ((input[1] >> 4) & 0x0f) | ((input[2] << 4) & 0xf0);
            output[3] = ((input[2] >> 4) & 0x0f);

            input += 3;
            output += 4 * 8;
        }
    } else { // 16-bit
        for (s = 0; s < num_samples; s++) {
            output[0] = channel;
            output[1] =                            ((input[0] << 4) & 0xf0);
            output[2] = ((input[0] >> 4) & 0x0f) | ((input[1] << 4) & 0xf0);
            output[3] = ((input[1] >> 4) & 0x0f);

            input += 2;
            output += 4 * 8;
        }
    }
}

uint32_t D10ContentPackage::WriteSystemItem(File *mxf_file)
{
    mxf_file->writeFixedKL(&MXF_EE_K(SDTI_CP_System_Pack), LLEN, SYSTEM_ITEM_METADATA_PACK_SIZE);

    // system metadata bitmap = 0x5c
    // b7 = 0 (FEC not used)
    // b6 = 1 (SMPTE Universal label)
    // b5 = 0 (creation date/time stamp)
    // b4 = 1 (user date/time stamp)
    // b3 = 1 (picture item)
    // b2 = 1 (sound item)
    // b1 = 0 (data item)
    // b0 = 0 (control element)

    // core fields
    mxf_file->writeUInt8(0x5c);                                         // system metadata bitmap
    mxf_file->writeUInt8(mInfo->is_25hz ? (2 << 1) : ((3 << 1) | 1));   // content package rate (25 or 30/1.001)
    mxf_file->writeUInt8(0x00);                                         // content package type (default)
    mxf_file->writeUInt16(0x0000);                                      // channel handle (default)
    mxf_file->writeUInt16((uint16_t)(mPosition % 65536));               // continuity count

    // SMPTE Universal Label
    mxf_file->writeUL(&mInfo->essence_container_ul);

    // (null) Package creation date / time stamp
    unsigned char ts_bytes[17];
    memset(ts_bytes, 0, sizeof(ts_bytes));
    BMX_CHECK(mxf_file->write(ts_bytes, sizeof(ts_bytes)) == sizeof(ts_bytes));

    // User date / time stamp
    Timecode user_timecode;
    ts_bytes[0] = 0x81; // SMPTE 12-M timecode
    if (mInfo->have_input_user_timecode) {
        user_timecode = mUserTimecode;
    } else if (!mInfo->start_timecode.IsInvalid()) {
        user_timecode = mInfo->start_timecode;
        user_timecode.AddOffset(mPosition);
    } else {
        user_timecode = Timecode((mInfo->is_25hz ? 25 : 30), false, mPosition);
    }
    encode_smpte_timecode(user_timecode, false, &ts_bytes[1], sizeof(ts_bytes) - 1);
    BMX_CHECK(mxf_file->write(ts_bytes, sizeof(ts_bytes)) == sizeof(ts_bytes));


    // (empty) Package Metadata Set
    mxf_file->writeFixedKL(&MXF_EE_K(EmptyPackageMetadataSet), LLEN, 0);


    return mxfKey_extlen + LLEN + SYSTEM_ITEM_METADATA_PACK_SIZE + mxfKey_extlen + LLEN;
}



D10ContentPackageManager::D10ContentPackageManager(mxfRational frame_rate)
{
    BMX_CHECK(frame_rate == FRAME_RATE_25 ||
              frame_rate == FRAME_RATE_2997);

    mContentPackageSize = 0;
    mPosition = 0;
    mInfo.is_25hz = (frame_rate.numerator == 25);

    // defaults used when no sound data present
    mInfo.sound_sample_size = 24;
    if (mInfo.is_25hz) {
        mInfo.sound_sample_sequence.push_back(1920);
        mInfo.max_sound_sample_count = 1920;
    } else {
        mInfo.sound_sample_sequence.push_back(1602);
        mInfo.sound_sample_sequence.push_back(1601);
        mInfo.sound_sample_sequence.push_back(1602);
        mInfo.sound_sample_sequence.push_back(1601);
        mInfo.sound_sample_sequence.push_back(1602);
        mInfo.max_sound_sample_count = 1602;
    }
}

D10ContentPackageManager::~D10ContentPackageManager()
{
    size_t i;
    for (i = 0; i < mContentPackages.size(); i++)
        delete mContentPackages[i];
    for (i = 0; i < mFreeContentPackages.size(); i++)
        delete mFreeContentPackages[i];
}

void D10ContentPackageManager::SetEssenceContainerUL(mxfUL essence_container_ul)
{
    mInfo.essence_container_ul = essence_container_ul;
}

void D10ContentPackageManager::SetMuteSoundFlags(uint8_t flags)
{
    mInfo.mute_sound_flags = flags;
}

void D10ContentPackageManager::SetInvalidSoundFlags(uint8_t flags)
{
    mInfo.invalid_sound_flags = flags;
}

void D10ContentPackageManager::SetHaveInputUserTimecode(bool enable)
{
    mInfo.have_input_user_timecode = enable;
}

void D10ContentPackageManager::SetSoundSequenceOffset(uint8_t offset)
{
    BMX_CHECK(!mInfo.sound_sequence_offset_set || offset == mInfo.sound_sequence_offset);

    mInfo.sound_sequence_offset = offset;
    mInfo.sound_sequence_offset_set = true;
}

void D10ContentPackageManager::SetStartTimecode(Timecode start_timecode)
{
    if (!start_timecode.IsInvalid() &&
        (( mInfo.is_25hz && start_timecode.GetRoundedTCBase() == 25) ||
         (!mInfo.is_25hz && start_timecode.GetRoundedTCBase() == 30)))
    {
        mInfo.start_timecode = start_timecode;
    }
    else
    {
        mInfo.start_timecode.SetInvalid();
    }
}

void D10ContentPackageManager::RegisterMPEGTrackElement(uint32_t track_index, uint32_t sample_size)
{
    mInfo.picture_track_index = track_index;
    mInfo.picture_sample_size = sample_size;
}

void D10ContentPackageManager::RegisterPCMTrackElement(uint32_t track_index, uint8_t output_channel_index,
                                                       vector<uint32_t> sample_sequence, uint32_t sample_size)
{
    BMX_CHECK(output_channel_index < 8);
    BMX_CHECK((mInfo.is_25hz && sample_sequence.size() == 1) || (!mInfo.is_25hz && sample_sequence.size() == 5));
    BMX_CHECK(sample_size == 2 || sample_size == 3);

    map<uint32_t, uint8_t>::const_iterator iter;
    for (iter = mInfo.sound_channels.begin(); iter != mInfo.sound_channels.end(); iter++) {
        BMX_CHECK_M(iter->second != output_channel_index,
                   ("Duplicate AES-3 channel indexes %d (track number %d)",
                     output_channel_index, output_channel_index + 1));
    }

    // set or check sample sequence and size
    if (mInfo.sound_channels.empty()) {
        mInfo.sound_sample_sequence = sample_sequence;
        mInfo.sound_sample_size = sample_size;
        size_t i;
        for (i = 0; i < sample_sequence.size(); i++) {
            if (sample_sequence[i] > mInfo.max_sound_sample_count)
                mInfo.max_sound_sample_count = sample_sequence[i];
        }
        BMX_CHECK((mInfo.is_25hz  && mInfo.max_sound_sample_count == 1920) ||
                 (!mInfo.is_25hz && mInfo.max_sound_sample_count == 1602));
    } else {
        BMX_CHECK(sample_size == mInfo.sound_sample_size);
        BMX_CHECK(sample_sequence.size() == mInfo.sound_sample_sequence.size());
        size_t i;
        for (i = 0; i < sample_sequence.size(); i++) {
            BMX_CHECK(mInfo.sound_sample_sequence[i] == sample_sequence[i]);
        }
    }

    mInfo.sound_channels[track_index] = output_channel_index;
}

void D10ContentPackageManager::PrepareWrite()
{
    BMX_CHECK_M(mInfo.picture_track_index != (uint32_t)(-1), ("Require video track for D10 MXF"));

    mInfo.system_item_size = (uint32_t)get_kag_aligned_size(mxfKey_extlen + LLEN + SYSTEM_ITEM_METADATA_PACK_SIZE +
                                                            mxfKey_extlen + LLEN,
                                                            KAG_SIZE, LLEN);
    mInfo.picture_item_size = (uint32_t)get_kag_aligned_size(mxfKey_extlen + LLEN + mInfo.picture_sample_size,
                                                             KAG_SIZE, LLEN);
    BMX_ASSERT(mxfKey_extlen + LLEN < 4 * 8); // can add fill for mInfo.max_sound_sample_count - 1
    mInfo.sound_item_size = (uint32_t)get_kag_aligned_size(mxfKey_extlen + LLEN + mInfo.max_sound_sample_count * 4 * 8 + 4,
                                                           KAG_SIZE, LLEN);


    // delta entry array
    mExtDeltaEntryArray.push_back(0);
    mExtDeltaEntryArray.push_back(mExtDeltaEntryArray[0] + mInfo.system_item_size);
    mExtDeltaEntryArray.push_back(mExtDeltaEntryArray[1] + mInfo.picture_item_size);
    mContentPackageSize = mExtDeltaEntryArray[2] + mInfo.sound_item_size;


    if (mInfo.sound_sample_sequence.size() == 1 || mInfo.sound_channels.empty()) {
        mInfo.sound_sequence_offset = 0;
        mInfo.sound_sequence_offset_set = true;
    } else if (mInfo.sound_sequence_offset_set) {
        mInfo.sound_sequence_offset %= mInfo.sound_sample_sequence.size();
    }
}

void D10ContentPackageManager::WriteUserTimecode(Timecode user_timecode)
{
    BMX_ASSERT(mInfo.have_input_user_timecode);

    size_t cp_index = 0;
    while (cp_index < mContentPackages.size() && mContentPackages[cp_index]->HaveUserTimecode())
        cp_index++;

    if (cp_index >= mContentPackages.size())
        CreateContentPackage();

    mContentPackages[cp_index]->SetUserTimecode(user_timecode);
}

void D10ContentPackageManager::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size,
                                            uint32_t num_samples)
{
    BMX_ASSERT(data && size && num_samples);

    size_t cp_index = 0;
    while (cp_index < mContentPackages.size() && mContentPackages[cp_index]->IsComplete(track_index))
        cp_index++;

    uint32_t sample_size = (track_index == mInfo.picture_track_index ? mInfo.picture_sample_size :
                                                                       mInfo.sound_sample_size);
    BMX_CHECK(size >= sample_size * num_samples);
    const unsigned char *data_ptr = data;
    uint32_t rem_size = sample_size * num_samples;
    uint32_t rem_num_samples = num_samples;
    while (rem_num_samples > 0) {
        if (cp_index >= mContentPackages.size())
            CreateContentPackage();

        uint32_t num_written = mContentPackages[cp_index]->WriteSamples(track_index, data_ptr, rem_size,
                                                                        rem_num_samples);
        rem_num_samples -= num_written;
        rem_size -= num_written * sample_size;
        data_ptr += num_written * sample_size;

        cp_index++;
    }
}

void D10ContentPackageManager::WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size)
{
    BMX_ASSERT(data_array && array_size);

    size_t cp_index = 0;
    while (cp_index < mContentPackages.size() && mContentPackages[cp_index]->IsComplete(track_index))
        cp_index++;

    if (cp_index >= mContentPackages.size())
        CreateContentPackage();

    mContentPackages[cp_index]->WriteSample(track_index, data_array, array_size);
}

uint8_t D10ContentPackageManager::GetSoundChannelCount() const
{
    uint8_t flags = mInfo.mute_sound_flags;
    map<uint32_t, uint8_t>::const_iterator iter;
    for (iter = mInfo.sound_channels.begin(); iter != mInfo.sound_channels.end(); iter++)
        flags |= (1 << iter->second);
    flags &= ~mInfo.invalid_sound_flags;

    return (flags & 0xf0) ? 8 : 4;
}

int64_t D10ContentPackageManager::GetDuration() const
{
    // duration = written packages (mPosition) + complete but not yet written packages
    int64_t duration = mPosition;
    size_t i;
    for (i = 0; i < mContentPackages.size(); i++) {
        if (mContentPackages[i]->IsComplete())
            duration++;
        else
            break;
    }

    return duration;
}

bool D10ContentPackageManager::HaveContentPackage() const
{
    return !mContentPackages.empty() && mContentPackages.front()->IsComplete() &&
           (mInfo.sound_channels.empty() || mInfo.sound_sequence_offset_set);
}

void D10ContentPackageManager::WriteNextContentPackage(File *mxf_file)
{
    BMX_ASSERT(HaveContentPackage());

    mContentPackages.front()->Write(mxf_file);

    mFreeContentPackages.push_back(mContentPackages.front());
    mContentPackages.pop_front();

    mPosition++;
}

void D10ContentPackageManager::FinalWrite(mxfpp::File *mxf_file)
{
    if (!mInfo.sound_sequence_offset_set)
        CalcSoundSequenceOffset(true);

    while (HaveContentPackage())
        WriteNextContentPackage(mxf_file);
}

void D10ContentPackageManager::CreateContentPackage()
{
    if (!mInfo.sound_sequence_offset_set)
        CalcSoundSequenceOffset(false);

    if (mFreeContentPackages.empty()) {
        BMX_CHECK(mContentPackages.size() < MAX_CONTENT_PACKAGES);
        mContentPackages.push_back(new D10ContentPackage(&mInfo));
    } else {
        mContentPackages.push_back(mFreeContentPackages.back());
        mFreeContentPackages.pop_back();
    }

    mContentPackages.back()->Reset(mPosition + mContentPackages.size() - 1);
}

void D10ContentPackageManager::CalcSoundSequenceOffset(bool final_write)
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
    while (offset < mInfo.sound_sample_sequence.size()) {
        for (i = 0; i < input_sequence.size(); i++) {
            if (input_sequence[i] != mInfo.sound_sample_sequence[(offset + i) % mInfo.sound_sample_sequence.size()])
                break;
        }
        if (i >= input_sequence.size())
            break;

        offset++;
    }
    BMX_CHECK_M(offset < mInfo.sound_sample_sequence.size(), ("Invalid D10 sound sample sequence"));

    if (final_write || input_sequence.size() >= mInfo.sound_sample_sequence.size()) {
        mInfo.sound_sequence_offset = offset;
        mInfo.sound_sequence_offset_set = true;
    } else {
        mInfo.sound_sequence_offset = 0;
        mInfo.sound_sequence_offset_set = false;
    }
}

