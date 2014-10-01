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
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey BWF_ELEMENT_FW_KEY    = MXF_AES3BWF_EE_K(0x01, MXF_BWF_FRAME_WRAPPED_EE_TYPE, 0x00);
static const mxfKey BWF_ELEMENT_CW_KEY    = MXF_AES3BWF_EE_K(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x00);
static const mxfKey AES3_ELEMENT_FW_KEY   = MXF_AES3BWF_EE_K(0x01, MXF_AES3_FRAME_WRAPPED_EE_TYPE, 0x00);
static const mxfKey AES3_ELEMENT_CW_KEY   = MXF_AES3BWF_EE_K(0x01, MXF_AES3_CLIP_WRAPPED_EE_TYPE, 0x00);
static const uint8_t AUDIO_ELEMENT_CW_LLEN  = 8;



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

    if (mOP1AFile->IsFrameWrapped()) {
        if ((file->GetFlavour() & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR))
            mWaveDescriptorHelper->SetSampleRate(SAMPLING_RATE_48K);
    } else {
        mEditRate = mWaveDescriptorHelper->GetSamplingRate();
        mWaveDescriptorHelper->SetSampleRate(mEditRate);
    }

    if ((file->GetFlavour() & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        SetAES3Mapping(true);
    else
        SetAES3Mapping(false);

    SetSampleSequence();
}

OP1APCMTrack::~OP1APCMTrack()
{
}

void OP1APCMTrack::SetAES3Mapping(bool enable)
{
    if (enable) {
        mWaveDescriptorHelper->SetUseAES3AudioDescriptor(true);
        if (mOP1AFile->IsFrameWrapped()) {
            mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_AES3_FRAME_WRAPPED_EE_TYPE, 0x00);
            mEssenceElementKey = AES3_ELEMENT_FW_KEY;
        } else {
            mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_AES3_CLIP_WRAPPED_EE_TYPE, 0x00);
            mEssenceElementKey = AES3_ELEMENT_CW_KEY;
        }
    } else {
        mWaveDescriptorHelper->SetUseAES3AudioDescriptor(false);
        if (mOP1AFile->IsFrameWrapped()) {
            mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_FRAME_WRAPPED_EE_TYPE, 0x00);
            mEssenceElementKey = BWF_ELEMENT_FW_KEY;
        } else {
            mTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x00);
            mEssenceElementKey = BWF_ELEMENT_CW_KEY;
        }
    }
}

void OP1APCMTrack::SetSamplingRate(mxfRational sampling_rate)
{
    BMX_CHECK(sampling_rate == SAMPLING_RATE_48K);

    mWaveDescriptorHelper->SetSamplingRate(sampling_rate);

    if (!mOP1AFile->IsFrameWrapped()) {
        mEditRate = sampling_rate;
        mDescriptorHelper->SetSampleRate(sampling_rate);
    }

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

void OP1APCMTrack::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    if (mOP1AFile->IsFrameWrapped()) {
        mCPManager->RegisterSoundTrackElement(mTrackIndex, mEssenceElementKey,
                                              GetShiftedSampleSequence(), mWaveDescriptorHelper->GetSampleSize());
    } else {
        mCPManager->RegisterSoundTrackElement(mTrackIndex, mEssenceElementKey, AUDIO_ELEMENT_CW_LLEN);
    }

    mIndexTable->RegisterSoundTrackElement(mTrackIndex);
}

void OP1APCMTrack::SetSampleSequence()
{
    mSampleSequence.clear();
    BMX_CHECK(get_sample_sequence(mFrameRate, mWaveDescriptorHelper->GetSamplingRate(), &mSampleSequence));
}

