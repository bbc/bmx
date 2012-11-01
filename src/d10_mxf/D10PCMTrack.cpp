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

#include <bmx/d10_mxf/D10PCMTrack.h>
#include <bmx/d10_mxf/D10File.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



D10PCMTrack::D10PCMTrack(D10File *file, uint32_t track_index, mxfRational frame_rate, EssenceType essence_type)
: D10Track(file, track_index, frame_rate, essence_type)
{
    BMX_ASSERT(essence_type == WAVE_PCM);

    mIsPicture = false;

    mSoundDescriptorHelper = dynamic_cast<SoundMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mSoundDescriptorHelper);

    mSoundDescriptorHelper->SetSamplingRate(SAMPLING_RATE_48K);
    mSoundDescriptorHelper->SetQuantizationBits(16);
    mSoundDescriptorHelper->SetChannelCount(1);

    mOutputTrackNumber = 1; // overrides init in D10Track

    SetSampleSequence();
}

D10PCMTrack::~D10PCMTrack()
{
}

void D10PCMTrack::SetOutputTrackNumber(uint32_t track_number)
{
    BMX_CHECK_M(track_number > 0,
               ("A zero D-10 AES-3 track number is not allowed. "
                "The AES-3 sound channel index is calculated as track number - 1"));
    BMX_CHECK(track_number <= 8);

    D10Track::SetOutputTrackNumber(track_number);
}

void D10PCMTrack::SetSamplingRate(mxfRational sampling_rate)
{
    BMX_CHECK(sampling_rate == SAMPLING_RATE_48K);

    mSoundDescriptorHelper->SetSamplingRate(sampling_rate);

    SetSampleSequence();
}

void D10PCMTrack::SetQuantizationBits(uint32_t bits)
{
    BMX_CHECK(bits == 16 || bits == 24);

    mSoundDescriptorHelper->SetQuantizationBits(bits);
}

void D10PCMTrack::SetChannelCount(uint32_t count)
{
    BMX_CHECK(count == 1); // currently support one channel per track only

    mSoundDescriptorHelper->SetChannelCount(count);
}

void D10PCMTrack::SetLocked(bool locked)
{
    mSoundDescriptorHelper->SetLocked(locked);
}

void D10PCMTrack::SetAudioRefLevel(int8_t level)
{
    mSoundDescriptorHelper->SetAudioRefLevel(level);
}

void D10PCMTrack::SetDialNorm(int8_t dial_norm)
{
    mSoundDescriptorHelper->SetDialNorm(dial_norm);
}

void D10PCMTrack::SetSequenceOffset(uint8_t offset)
{
    mD10File->SetSoundSequenceOffset(offset);
}

bool D10PCMTrack::HaveSequenceOffset() const
{
    return mCPManager->HaveSoundSequenceOffset();
}

uint8_t D10PCMTrack::GetSequenceOffset() const
{
    BMX_CHECK(mCPManager->HaveSoundSequenceOffset());
    return mCPManager->GetSoundSequenceOffset();
}

vector<uint32_t> D10PCMTrack::GetShiftedSampleSequence() const
{
    vector<uint32_t> shifted_sample_sequence = mSampleSequence;
    offset_sample_sequence(shifted_sample_sequence, GetSequenceOffset());

    return shifted_sample_sequence;
}

void D10PCMTrack::PrepareWrite()
{
    mCPManager->RegisterPCMTrackElement(mTrackIndex, mOutputTrackNumber - 1, mSampleSequence,
                                        mSoundDescriptorHelper->GetSampleSize());
}

void D10PCMTrack::SetSampleSequence()
{
    mSampleSequence.clear();
    BMX_CHECK(get_sample_sequence(mFrameRate, mSoundDescriptorHelper->GetSamplingRate(), &mSampleSequence));
}

