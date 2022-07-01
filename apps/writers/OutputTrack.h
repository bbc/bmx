/*
 * Copyright (C) 2016, British Broadcasting Corporation
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


#ifndef BMX_OUTPUT_TRACK_H_
#define BMX_OUTPUT_TRACK_H_

#include <map>

#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/essence_parser/EssenceFilter.h>
#include <bmx/mxf_reader/MXFTrackReader.h>
#include <bmx/ByteArray.h>


namespace bmx
{


class InputTrack;


class OutputTrackSoundInfo
{
public:
    OutputTrackSoundInfo();

    void Copy(const OutputTrackSoundInfo &from);

public:
    mxfRational sampling_rate;
    uint32_t bits_per_sample;
    uint8_t sequence_offset;
    BMX_OPT_PROP_DECL(bool, locked);
    BMX_OPT_PROP_DECL(int8_t, audio_ref_level);
    BMX_OPT_PROP_DECL(int8_t, dial_norm);
    BMX_OPT_PROP_DECL(mxfRational, ref_image_edit_rate);
    BMX_OPT_PROP_DECL(int8_t, ref_audio_align_level);
};


class OutputTrack
{
public:
    OutputTrack(ClipWriterTrack *clip_writer_track);
    ~OutputTrack();

    void AddInput(InputTrack *input_track, uint32_t input_channel_index, uint32_t output_channel_index);
    void AddSilenceChannel(uint32_t output_channel_index);
    void SetPhysSrcTrackIndex(uint32_t index);
    void SetSkipPrecharge(int64_t precharge);
    void SetFilter(EssenceFilter *filter);

public:
    void WriteSamples(uint32_t output_channel_index, unsigned char *data, uint32_t size, uint32_t num_samples);
    void WritePaddingSamples(uint32_t output_channel_index, uint32_t num_samples);

    void WriteSilenceSamples(uint32_t num_samples);

    void SkipPrecharge(int64_t num_read);

public:
    ClipWriterTrack* GetClipTrack() { return mClipWriterTrack; }
    uint32_t GetPhysSrcTrackIndex() { return mPhysSrcTrackIndex; }
    bool HaveInputTrack()           { return !mInputMaps.empty(); }
    uint32_t GetChannelCount()      { return mChannelCount; }
    bool HaveSkipPrecharge()        { return mRemSkipPrecharge > 0; }
    bool IsSilenceTrack();
    OutputTrackSoundInfo* GetSoundInfo();
    InputTrack* GetFirstInputTrack();

private:
    typedef struct
    {
        InputTrack *input_track;
        uint32_t input_channel_index;
        bool have_sample_data;
    } InputMap;

private:
    ClipWriterTrack *mClipWriterTrack;
    std::map<uint32_t, InputMap> mInputMaps;
    uint32_t mChannelCount;
    uint32_t mPhysSrcTrackIndex;
    int64_t mRemSkipPrecharge;
    EssenceFilter *mFilter;
    ByteArray mSampleBuffer;
    uint32_t mNumSamples;
    size_t mAvailableChannelCount;
    OutputTrackSoundInfo mSoundInfo;
};


};


#endif
