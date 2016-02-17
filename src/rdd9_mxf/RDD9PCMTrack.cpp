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

#include <bmx/rdd9_mxf/RDD9PCMTrack.h>
#include <bmx/rdd9_mxf/RDD9File.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey AUDIO_ELEMENT_KEY = MXF_AES3BWF_EE_K(0x01, MXF_AES3_FRAME_WRAPPED_EE_TYPE, 0x00);



RDD9PCMTrack::RDD9PCMTrack(RDD9File *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                           Rational frame_rate, EssenceType essence_type)
: RDD9Track(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    BMX_ASSERT(essence_type == WAVE_PCM);

    mWaveDescriptorHelper = dynamic_cast<WaveMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mWaveDescriptorHelper);

    if (!(file->mFlavour & RDD9_SMPTE_377_2004_FLAVOUR))
        mWaveDescriptorHelper->SetSampleRate(SAMPLING_RATE_48K);
    mWaveDescriptorHelper->SetSamplingRate(SAMPLING_RATE_48K);
    mWaveDescriptorHelper->SetQuantizationBits(16);
    mWaveDescriptorHelper->SetChannelCount(1);
    mWaveDescriptorHelper->SetLocked(true);
    if (!(file->mFlavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        mWaveDescriptorHelper->SetAudioRefLevel(0);

    mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_AES3_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = AUDIO_ELEMENT_KEY;

    SetSampleSequence();
}

RDD9PCMTrack::~RDD9PCMTrack()
{
}

void RDD9PCMTrack::SetSamplingRate(Rational sampling_rate)
{
    BMX_CHECK(sampling_rate == SAMPLING_RATE_48K);

    mWaveDescriptorHelper->SetSamplingRate(sampling_rate);

    SetSampleSequence();
}

void RDD9PCMTrack::SetQuantizationBits(uint32_t bits)
{
    BMX_CHECK(bits > 0 && bits <= 32);

    if (bits != 16 && bits != 24)
        log_warn("Audio quantization bits is set to %u; RDD9 requires audio quantization bits 16 or 24\n", bits);
    mWaveDescriptorHelper->SetQuantizationBits(bits);
}

void RDD9PCMTrack::SetChannelCount(uint32_t count)
{
    BMX_CHECK_M(count == 1, ("Invalid channel count %u; RDD9 requires 1 channel per audio element / track", count));

    mWaveDescriptorHelper->SetChannelCount(count);
}

void RDD9PCMTrack::SetLocked(bool locked)
{
    if (!locked)
        log_warn("Audio locked is set to false; RDD9 requires audio locked true\n");
    mWaveDescriptorHelper->SetLocked(locked);
}

void RDD9PCMTrack::SetAudioRefLevel(int8_t level)
{
    if (level != 0 && !(mRDD9File->GetFlavour() & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        log_warn("Audio reference level is set to %d; RDD9 requires audio reference level 0\n", level);
    mWaveDescriptorHelper->SetAudioRefLevel(level);
}

void RDD9PCMTrack::SetDialNorm(int8_t dial_norm)
{
    mWaveDescriptorHelper->SetDialNorm(dial_norm);
}

void RDD9PCMTrack::SetSequenceOffset(uint8_t offset)
{
    mWaveDescriptorHelper->SetSequenceOffset(offset);
    mCPManager->SetSoundSequenceOffset(offset);
}

vector<uint32_t> RDD9PCMTrack::GetShiftedSampleSequence() const
{
    vector<uint32_t> shifted_sample_sequence = mSampleSequence;
    offset_sample_sequence(shifted_sample_sequence, mWaveDescriptorHelper->GetSequenceOffset());

    return shifted_sample_sequence;
}

void RDD9PCMTrack::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterSoundTrackElement(mTrackIndex, mEssenceElementKey,
                                          mSampleSequence, mWaveDescriptorHelper->GetSampleSize());

    mIndexTable->RegisterSoundTrackElement(mTrackIndex);
}

void RDD9PCMTrack::CompleteWrite()
{
    mWaveDescriptorHelper->SetSequenceOffset(mCPManager->GetSoundSequenceOffset());
}

void RDD9PCMTrack::SetSampleSequence()
{
    mSampleSequence.clear();
    BMX_CHECK(get_sample_sequence(mFrameRate, mWaveDescriptorHelper->GetSamplingRate(), &mSampleSequence));
}

