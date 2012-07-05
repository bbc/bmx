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

#include <bmx/avid_mxf/AvidPCMTrack.h>
#include <bmx/avid_mxf/AvidClip.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const mxfKey AUDIO_ELEMENT_KEY = MXF_AES3BWF_EE_K(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x01);



AvidPCMTrack::AvidPCMTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidTrack(clip, track_index, essence_type, file)
{
    mWaveDescriptorHelper = dynamic_cast<WaveMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mWaveDescriptorHelper);

    mWaveDescriptorHelper->SetFrameWrapped(false);
    mWaveDescriptorHelper->SetSamplingRate(SAMPLING_RATE_48K);
    mWaveDescriptorHelper->SetSampleRate(mWaveDescriptorHelper->GetSamplingRate());
    mWaveDescriptorHelper->SetQuantizationBits(16);
    mWaveDescriptorHelper->SetChannelCount(1);

    mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x01);
    mEssenceElementKey = AUDIO_ELEMENT_KEY;

    SetSampleSequence();
}

AvidPCMTrack::~AvidPCMTrack()
{
}

void AvidPCMTrack::SetSamplingRate(mxfRational sampling_rate)
{
    BMX_CHECK(sampling_rate == SAMPLING_RATE_48K);

    mWaveDescriptorHelper->SetSamplingRate(sampling_rate);
    mWaveDescriptorHelper->SetSampleRate(mWaveDescriptorHelper->GetSamplingRate());

    SetSampleSequence();
}

void AvidPCMTrack::SetQuantizationBits(uint32_t bits)
{
    BMX_CHECK(bits > 0 && bits <= 32);

    mWaveDescriptorHelper->SetQuantizationBits(bits);
}

void AvidPCMTrack::SetChannelCount(uint32_t count)
{
    mWaveDescriptorHelper->SetChannelCount(count);
}

void AvidPCMTrack::SetLocked(bool locked)
{
    mWaveDescriptorHelper->SetLocked(locked);
}

void AvidPCMTrack::SetAudioRefLevel(int8_t level)
{
    mWaveDescriptorHelper->SetAudioRefLevel(level);
}

void AvidPCMTrack::SetDialNorm(int8_t dial_norm)
{
    mWaveDescriptorHelper->SetDialNorm(dial_norm);
}

void AvidPCMTrack::SetSequenceOffset(uint8_t offset)
{
    mWaveDescriptorHelper->SetSequenceOffset(offset);

    SetSampleSequence();
}

vector<uint32_t> AvidPCMTrack::GetShiftedSampleSequence() const
{
    vector<uint32_t> shifted_sample_sequence = mSampleSequence;
    offset_sample_sequence(shifted_sample_sequence, mWaveDescriptorHelper->GetSequenceOffset());

    return shifted_sample_sequence;
}

int64_t AvidPCMTrack::GetOutputDuration(bool clip_frame_rate) const
{
    if (clip_frame_rate)
        return convert_duration_lower(mContainerDuration, mSampleSequence);

    return mContainerDuration;
}

void AvidPCMTrack::SetSampleSequence()
{
    mSampleSequence.clear();
    BMX_CHECK(get_sample_sequence(mClip->GetFrameRate(), mWaveDescriptorHelper->GetSamplingRate(), &mSampleSequence));
}

