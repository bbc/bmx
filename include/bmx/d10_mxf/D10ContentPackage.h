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

#ifndef BMX_D10_CONTENT_PACKAGE_H_
#define BMX_D10_CONTENT_PACKAGE_H_

#include <vector>
#include <deque>
#include <map>

#include <libMXF++/MXF.h>

#include <bmx/BMXTypes.h>
#include <bmx/ByteArray.h>
#include <bmx/frame/DataBufferArray.h>



namespace bmx
{


class D10ContentPackageInfo
{
public:
    D10ContentPackageInfo();

public:
    bool is_25hz;
    mxfUL essence_container_ul;
    uint8_t mute_sound_flags;
    uint8_t invalid_sound_flags;
    bool have_input_user_timecode;
    uint32_t picture_track_index;
    uint32_t picture_sample_size;
    std::map<uint32_t, uint8_t> sound_channels;
    std::vector<uint32_t> sound_sample_sequence;
    bool sound_sequence_offset_set;
    uint8_t sound_sequence_offset;
    uint32_t max_sound_sample_count;
    uint32_t sound_sample_size;
    uint32_t system_item_size;
    uint32_t picture_item_size;
    uint32_t sound_item_size;
    Timecode start_timecode;
};


class D10ContentPackage
{
public:
    D10ContentPackage(D10ContentPackageInfo *info);
    ~D10ContentPackage();

    void Reset(int64_t position);

    bool HaveUserTimecode() { return mUserTimecodeSet; }
    void SetUserTimecode(Timecode user_timecode);

    uint32_t GetSoundSampleCount() const { return mSoundSampleCount; }

    bool IsComplete(uint32_t track_index);
    uint32_t WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size);

    bool IsComplete();
    void Write(mxfpp::File *mxf_file);

private:
    void CopySoundSamples(const unsigned char *data, uint32_t num_samples, uint8_t channel,
                          uint32_t output_start_sample);

    uint32_t WriteSystemItem(mxfpp::File *mxf_file);

private:
    D10ContentPackageInfo *mInfo;
    Timecode mUserTimecode;
    bool mUserTimecodeSet;
    ByteArray mPictureData;
    ByteArray mSoundData;
    std::map<uint32_t, uint32_t> mSoundChannelSampleCount;
    size_t mSoundSequenceIndex;
    uint32_t mSoundSampleCount;
    int64_t mPosition;
};


class D10ContentPackageManager
{
public:
    D10ContentPackageManager(mxfRational frame_rate);
    ~D10ContentPackageManager();

    void SetEssenceContainerUL(mxfUL essence_container_ul);
    void SetMuteSoundFlags(uint8_t flags);
    void SetInvalidSoundFlags(uint8_t flags);
    void SetHaveInputUserTimecode(bool enable);
    void SetStartTimecode(Timecode start_timecode);
    void SetSoundSequenceOffset(uint8_t offset);

    void RegisterMPEGTrackElement(uint32_t track_index, uint32_t sample_size);
    void RegisterPCMTrackElement(uint32_t track_index, uint8_t output_channel_index,
                                 std::vector<uint32_t> sample_sequence, uint32_t sample_size);

    void PrepareWrite();

public:
    void WriteUserTimecode(Timecode user_timecode);
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void WriteSample(uint32_t track_index, const CDataBuffer *data_array, uint32_t array_size);

public:
    uint8_t GetSoundChannelCount() const;

    bool HaveSoundSequenceOffset() const   { return mInfo.sound_sequence_offset_set; }
    uint8_t GetSoundSequenceOffset() const { return mInfo.sound_sequence_offset; }

    int64_t GetDuration() const;

    const std::vector<uint32_t>& GetExtDeltaEntryArray() const { return mExtDeltaEntryArray; }
    uint32_t GetContentPackageSize() const { return mContentPackageSize; }

    bool HaveContentPackage() const;
    void WriteNextContentPackage(mxfpp::File *mxf_file);
    void FinalWrite(mxfpp::File *mxf_file);

private:
    void CreateContentPackage();

    void CalcSoundSequenceOffset(bool final_write);

private:
    D10ContentPackageInfo mInfo;
    std::vector<uint32_t> mExtDeltaEntryArray;
    uint32_t mContentPackageSize;

    std::deque<D10ContentPackage*> mContentPackages;
    std::vector<D10ContentPackage*> mFreeContentPackages;

    int64_t mPosition;
};


};



#endif

