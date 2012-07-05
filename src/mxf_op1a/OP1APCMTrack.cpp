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

#include <bmx/mxf_op1a/OP1APCMTrack.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey AUDIO_ELEMENT_KEY = MXF_AES3BWF_EE_K(0x01, MXF_BWF_FRAME_WRAPPED_EE_TYPE, 0x00);



OP1APCMTrack::OP1APCMTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                           mxfRational frame_rate, EssenceType essence_type)
: OP1ATrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    BMX_ASSERT(essence_type == WAVE_PCM);

    mWaveDescriptorHelper = dynamic_cast<WaveMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mWaveDescriptorHelper);

    mWaveDescriptorHelper->SetSamplingRate(SAMPLING_RATE_48K);
    mWaveDescriptorHelper->SetQuantizationBits(16);
    mWaveDescriptorHelper->SetChannelCount(1);

    mIsPicture = false;
    mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = AUDIO_ELEMENT_KEY;

    SetSampleSequence();
}

OP1APCMTrack::~OP1APCMTrack()
{
}

void OP1APCMTrack::SetSamplingRate(mxfRational sampling_rate)
{
    BMX_CHECK(sampling_rate == SAMPLING_RATE_48K);

    mWaveDescriptorHelper->SetSamplingRate(sampling_rate);

    SetSampleSequence();
}

void OP1APCMTrack::SetQuantizationBits(uint32_t bits)
{
    BMX_CHECK(bits > 0 && bits <= 32);

    mWaveDescriptorHelper->SetQuantizationBits(bits);
}

void OP1APCMTrack::SetChannelCount(uint32_t count)
{
    mWaveDescriptorHelper->SetChannelCount(count);
}

void OP1APCMTrack::SetLocked(bool locked)
{
    mWaveDescriptorHelper->SetLocked(locked);
}

void OP1APCMTrack::SetAudioRefLevel(int8_t level)
{
    mWaveDescriptorHelper->SetAudioRefLevel(level);
}

void OP1APCMTrack::SetDialNorm(int8_t dial_norm)
{
    mWaveDescriptorHelper->SetDialNorm(dial_norm);
}

void OP1APCMTrack::SetSequenceOffset(uint8_t offset)
{
    mWaveDescriptorHelper->SetSequenceOffset(offset);

    SetSampleSequence();
}

vector<uint32_t> OP1APCMTrack::GetShiftedSampleSequence() const
{
    vector<uint32_t> shifted_sample_sequence = mSampleSequence;
    offset_sample_sequence(shifted_sample_sequence, mWaveDescriptorHelper->GetSequenceOffset());

    return shifted_sample_sequence;
}

void OP1APCMTrack::PrepareWrite(uint8_t picture_track_count, uint8_t sound_track_count)
{
    (void)picture_track_count;

    CompleteEssenceKeyAndTrackNum(sound_track_count);

    mCPManager->RegisterSoundTrackElement(mTrackIndex, mEssenceElementKey,
                                          GetShiftedSampleSequence(), mWaveDescriptorHelper->GetSampleSize());
    mIndexTable->RegisterSoundTrackElement(mTrackIndex);
}

void OP1APCMTrack::SetSampleSequence()
{
    mSampleSequence.clear();
    BMX_CHECK(get_sample_sequence(mFrameRate, mWaveDescriptorHelper->GetSamplingRate(), &mSampleSequence));
}

