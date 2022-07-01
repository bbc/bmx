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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "OutputTrack.h"

#include "InputTrack.h"
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/MXFUtils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


OutputTrackSoundInfo::OutputTrackSoundInfo()
{
    sampling_rate = ZERO_RATIONAL;
    bits_per_sample = 0;
    sequence_offset = 0;
    BMX_OPT_PROP_DEFAULT(locked, false);
    BMX_OPT_PROP_DEFAULT(audio_ref_level, 0);
    BMX_OPT_PROP_DEFAULT(dial_norm, 0);
    BMX_OPT_PROP_DEFAULT(ref_image_edit_rate, g_Null_Rational);
    BMX_OPT_PROP_DEFAULT(ref_audio_align_level, 0);
}

void OutputTrackSoundInfo::Copy(const OutputTrackSoundInfo &from)
{
    sampling_rate   = from.sampling_rate;
    bits_per_sample = from.bits_per_sample;
    sequence_offset = from.sequence_offset;
    BMX_OPT_PROP_COPY(locked,                from.locked);
    BMX_OPT_PROP_COPY(audio_ref_level,       from.audio_ref_level);
    BMX_OPT_PROP_COPY(dial_norm,             from.dial_norm);
    BMX_OPT_PROP_COPY(ref_image_edit_rate,   from.ref_image_edit_rate);
    BMX_OPT_PROP_COPY(ref_audio_align_level, from.ref_audio_align_level);
}


OutputTrack::OutputTrack(ClipWriterTrack *clip_writer_track)
{
    mClipWriterTrack = clip_writer_track;
    mChannelCount = 0;
    mPhysSrcTrackIndex = 0;
    mRemSkipPrecharge = 0;
    mFilter = 0;
    mNumSamples = 0;
    mAvailableChannelCount = 0;
}

OutputTrack::~OutputTrack()
{
    delete mFilter;
}

void OutputTrack::AddInput(InputTrack *input_track, uint32_t input_channel_index, uint32_t output_channel_index)
{
    BMX_ASSERT(mInputMaps.count(output_channel_index) == 0);

    InputMap input_map;
    input_map.input_track         = input_track;
    input_map.input_channel_index = input_channel_index;
    input_map.have_sample_data    = false;

    mInputMaps[output_channel_index] = input_map;

    if (output_channel_index + 1 > mChannelCount)
        mChannelCount = output_channel_index + 1;

    BMX_ASSERT(mChannelCount == 1 || input_track->GetDataDef() == MXF_SOUND_DDEF);
}

void OutputTrack::AddSilenceChannel(uint32_t output_channel_index)
{
    BMX_ASSERT(mInputMaps.count(output_channel_index) == 0);
    if (output_channel_index + 1 > mChannelCount)
        mChannelCount = output_channel_index + 1;
}

void OutputTrack::SetPhysSrcTrackIndex(uint32_t index)
{
    mPhysSrcTrackIndex = index;
}

void OutputTrack::SetSkipPrecharge(int64_t precharge)
{
    mRemSkipPrecharge = precharge;
}

void OutputTrack::SetFilter(EssenceFilter *filter)
{
    delete mFilter;
    mFilter = filter;
}

bool OutputTrack::IsSilenceTrack()
{
    return mInputMaps.empty() && GetSoundInfo();
}

OutputTrackSoundInfo* OutputTrack::GetSoundInfo()
{
    if (convert_essence_type_to_data_def(mClipWriterTrack->GetEssenceType()) == MXF_SOUND_DDEF)
        return &mSoundInfo;
    else
        return 0;
}

InputTrack* OutputTrack::GetFirstInputTrack()
{
    BMX_ASSERT(!mInputMaps.empty());
    return mInputMaps.begin()->second.input_track;
}

void OutputTrack::WriteSamples(uint32_t output_channel_index,
                               unsigned char *input_data, uint32_t input_size, uint32_t num_samples)
{
    if (mInputMaps.empty()) {
        mClipWriterTrack->WriteSamples(input_data, input_size, num_samples);
        return;
    }

    BMX_CHECK(num_samples > 0);
    if (mNumSamples == 0)
        mNumSamples = num_samples;
    else
        BMX_CHECK(mNumSamples == num_samples);

    InputMap &input_map = mInputMaps.at(output_channel_index);
    BMX_ASSERT(!input_map.have_sample_data);

    uint32_t frame_size = mNumSamples * mClipWriterTrack->GetSampleSize();
    if (mChannelCount > 1) {
        if (mAvailableChannelCount == 0) {
            mSampleBuffer.Allocate(frame_size);
            mSampleBuffer.SetSize(frame_size);
            memset(mSampleBuffer.GetBytes(), 0, mSampleBuffer.GetSize()); // means silence channels already set
        }
        if (input_data) {
            uint32_t bits_per_sample = mClipWriterTrack->GetSampleSize() / mChannelCount * 8;
            interleave_audio(input_data, input_size,
                             bits_per_sample, mChannelCount, output_channel_index,
                             mSampleBuffer.GetBytes(), mSampleBuffer.GetSize());
        }
    } else if (!input_data) {
        if (mSampleBuffer.GetAllocatedSize() < frame_size)
            mSampleBuffer.Allocate(frame_size); // will clear data
        mSampleBuffer.SetSize(frame_size);
    }
    input_map.have_sample_data = true;

    mAvailableChannelCount++;
    if (mAvailableChannelCount < mInputMaps.size())
        return;


    unsigned char *output_data;
    uint32_t output_size;
    if (!input_data || mChannelCount > 1) {
        output_data = mSampleBuffer.GetBytes();
        output_size = mSampleBuffer.GetSize();
    } else {
        output_data = input_data;
        output_size = input_size;
    }

    if (mFilter) {
        if (mFilter->SupportsInPlaceFilter()) {
            mFilter->Filter(output_data, output_size);
            mClipWriterTrack->WriteSamples(output_data, output_size, mNumSamples);
        } else {
            unsigned char *f_data = 0;
            try
            {
                uint32_t f_size;
                mFilter->Filter(output_data, output_size, &f_data, &f_size);
                mClipWriterTrack->WriteSamples(f_data, f_size, mNumSamples);
                delete [] f_data;
            }
            catch (...)
            {
                delete [] f_data;
                throw;
            }
        }
    } else {
        mClipWriterTrack->WriteSamples(output_data, output_size, mNumSamples);
    }

    mNumSamples = 0;
    mAvailableChannelCount = 0;
    map<uint32_t, InputMap>::iterator iter;
    for (iter = mInputMaps.begin(); iter != mInputMaps.end(); iter++)
        iter->second.have_sample_data = false;
}

void OutputTrack::WritePaddingSamples(uint32_t output_channel_index, uint32_t num_samples)
{
    WriteSamples(output_channel_index, 0, 0, num_samples);
}

void OutputTrack::WriteSilenceSamples(uint32_t num_samples)
{
    BMX_ASSERT(mInputMaps.empty());

    uint32_t frame_size = num_samples * mClipWriterTrack->GetSampleSize();
    if (mSampleBuffer.GetAllocatedSize() < frame_size)
        mSampleBuffer.Allocate(frame_size); // will clear data
    mSampleBuffer.SetSize(frame_size);

    if (mFilter) {
        if (mFilter->SupportsInPlaceFilter()) {
            mFilter->Filter(mSampleBuffer.GetBytes(), mSampleBuffer.GetSize());
            mClipWriterTrack->WriteSamples(mSampleBuffer.GetBytes(), mSampleBuffer.GetSize(), num_samples);
        } else {
            unsigned char *f_data = 0;
            try
            {
                uint32_t f_size;
                mFilter->Filter(mSampleBuffer.GetBytes(), mSampleBuffer.GetSize(), &f_data, &f_size);
                mClipWriterTrack->WriteSamples(f_data, f_size, num_samples);
                delete [] f_data;
            }
            catch (...)
            {
                delete [] f_data;
                throw;
            }
        }
    } else {
        mClipWriterTrack->WriteSamples(mSampleBuffer.GetBytes(), mSampleBuffer.GetSize(), num_samples);
    }
}

void OutputTrack::SkipPrecharge(int64_t num_read)
{
    BMX_ASSERT(mRemSkipPrecharge >= num_read);
    mRemSkipPrecharge -= num_read;
}
