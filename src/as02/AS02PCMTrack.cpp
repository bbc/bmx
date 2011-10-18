/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <im/as02/AS02PCMTrack.h>
#include <im/as02/AS02Clip.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



static const mxfKey AUDIO_ELEMENT_KEY = MXF_AES3BWF_EE_K(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x00);
static const uint8_t AUDIO_ELEMENT_LLEN = 8;



AS02PCMTrack::AS02PCMTrack(AS02Clip *clip, uint32_t track_index, mxfpp::File *file, string rel_uri)
: AS02Track(clip, track_index, AS02_PCM, file, rel_uri)
{
    mWaveDescriptorHelper = dynamic_cast<WaveMXFDescriptorHelper*>(mDescriptorHelper);
    IM_ASSERT(mWaveDescriptorHelper);
    mEssenceDataStartPos = 0;

    mWaveDescriptorHelper->SetFrameWrapped(false);
    mWaveDescriptorHelper->SetSamplingRate(SAMPLING_RATE_48K);
    mWaveDescriptorHelper->SetSampleRate(mWaveDescriptorHelper->GetSamplingRate());
    mWaveDescriptorHelper->SetQuantizationBits(16);
    mWaveDescriptorHelper->SetChannelCount(1);

    mIsPicture = false;
    mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = AUDIO_ELEMENT_KEY;

    SetSampleSequence();
}

AS02PCMTrack::~AS02PCMTrack()
{
}

void AS02PCMTrack::SetSamplingRate(mxfRational sampling_rate)
{
    IM_CHECK(sampling_rate.numerator == 48000 && sampling_rate.denominator == 1);

    mWaveDescriptorHelper->SetSamplingRate(sampling_rate);
    mWaveDescriptorHelper->SetSampleRate(mWaveDescriptorHelper->GetSamplingRate());

    SetSampleSequence();
}

void AS02PCMTrack::SetQuantizationBits(uint32_t bits)
{
    IM_CHECK(bits > 0 && bits <= 32);

    mWaveDescriptorHelper->SetQuantizationBits(bits);
}

void AS02PCMTrack::SetChannelCount(uint32_t count)
{
    mWaveDescriptorHelper->SetChannelCount(count);
}

void AS02PCMTrack::SetLocked(bool locked)
{
    mWaveDescriptorHelper->SetLocked(locked);
}

void AS02PCMTrack::SetAudioRefLevel(int8_t level)
{
    mWaveDescriptorHelper->SetAudioRefLevel(level);
}

void AS02PCMTrack::SetDialNorm(int8_t dial_norm)
{
    mWaveDescriptorHelper->SetDialNorm(dial_norm);
}

void AS02PCMTrack::SetSequenceOffset(uint8_t offset)
{
    mWaveDescriptorHelper->SetSequenceOffset(offset);

    SetSampleSequence();
}

vector<uint32_t> AS02PCMTrack::GetShiftedSampleSequence() const
{
    vector<uint32_t> shifted_sample_sequence = mSampleSequence;
    offset_sound_sample_sequence(shifted_sample_sequence, mWaveDescriptorHelper->GetSequenceOffset());

    return shifted_sample_sequence;
}

void AS02PCMTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    IM_ASSERT(mMXFFile);
    IM_CHECK(mSampleSize > 0);
    IM_CHECK(size > 0 && num_samples > 0);
    IM_CHECK(size == num_samples * mSampleSize);

    IM_CHECK(mMXFFile->write(data, size) == size);

    UpdateEssenceOnlyChecksum(data, size);

    mContainerDuration += num_samples;
    mContainerSize += size;
}

void AS02PCMTrack::PreSampleWriting()
{
    mEssenceDataStartPos = mMXFFile->tell(); // need this position when we re-write the key
    mMXFFile->writeFixedKL(&AUDIO_ELEMENT_KEY, AUDIO_ELEMENT_LLEN, 0);
}

void AS02PCMTrack::PostSampleWriting(Partition *partition)
{
    int64_t pos = mMXFFile->tell();

    mMXFFile->seek(mEssenceDataStartPos, SEEK_SET);
    mMXFFile->writeFixedKL(&AUDIO_ELEMENT_KEY, AUDIO_ELEMENT_LLEN,
                           mWaveDescriptorHelper->GetSampleSize() * mContainerDuration);

    mMXFFile->seek(pos, SEEK_SET);

    partition->fillToKag(mMXFFile);
}

void AS02PCMTrack::SetSampleSequence()
{
    mSampleSequence.clear();
    IM_CHECK(get_sound_sample_sequence(GetVideoFrameRate(), mWaveDescriptorHelper->GetSamplingRate(),
                                       &mSampleSequence));
}

