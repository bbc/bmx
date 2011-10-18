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

#include <im/mxf_helper/SoundMXFDescriptorHelper.h>
#include <im/mxf_helper/WaveMXFDescriptorHelper.h>
#include <im/IMTypes.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



MXFDescriptorHelper::EssenceType SoundMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor,
                                                                       mxfUL alternative_ec_label)
{
    GenericSoundEssenceDescriptor *sound_descriptor =
        dynamic_cast<GenericSoundEssenceDescriptor*>(file_descriptor);
    if (!sound_descriptor)
        return UNKNOWN_ESSENCE;

    MXFDescriptorHelper::EssenceType essence_type =
        WaveMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;

    return SOUND_ESSENCE;
}

SoundMXFDescriptorHelper* SoundMXFDescriptorHelper::Create(FileDescriptor *file_descriptor,
                                                           mxfUL alternative_ec_label)
{
    SoundMXFDescriptorHelper *helper;
    if (WaveMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new WaveMXFDescriptorHelper();
    else
        helper = new SoundMXFDescriptorHelper();

    helper->Initialize(file_descriptor, alternative_ec_label);

    return helper;
}

bool SoundMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return WaveMXFDescriptorHelper::IsSupported(essence_type);
}

MXFDescriptorHelper* SoundMXFDescriptorHelper::Create(EssenceType essence_type)
{
    IM_ASSERT(IsSupported(essence_type));

    SoundMXFDescriptorHelper *helper = new WaveMXFDescriptorHelper();
    helper->SetEssenceType(essence_type);

    return helper;
}

SoundMXFDescriptorHelper::SoundMXFDescriptorHelper()
: MXFDescriptorHelper()
{
    mSamplingRate = SAMPLING_RATE_48K;
    mQuantizationBits = 16;
    mSampleRate = mSamplingRate;
    mChannelCount = 1;
    mLocked = false;
    mLockedSet = false;
    mAudioRefLevel = 0;
    mAudioRefLevelSet = false;
    mDialNorm = 0;
    mDialNormSet = false;
}

SoundMXFDescriptorHelper::~SoundMXFDescriptorHelper()
{
}

void SoundMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    MXFDescriptorHelper::Initialize(file_descriptor, alternative_ec_label);

    GenericSoundEssenceDescriptor *sound_descriptor =
        dynamic_cast<GenericSoundEssenceDescriptor*>(file_descriptor);
    IM_ASSERT(sound_descriptor);

    if (sound_descriptor->haveAudioSamplingRate())
        mSamplingRate = sound_descriptor->getAudioSamplingRate();
    else
        mSamplingRate = SAMPLING_RATE_48K;

    if (sound_descriptor->haveQuantizationBits())
        mQuantizationBits = sound_descriptor->getQuantizationBits();
    else
        mQuantizationBits = 16;

    if (sound_descriptor->haveChannelCount())
        mChannelCount = sound_descriptor->getChannelCount();
    else
        mChannelCount = 1;

    if (sound_descriptor->haveLocked()) {
        mLocked = sound_descriptor->getLocked();
        mLockedSet = true;
    } else {
        mLockedSet = false;
    }

    if (sound_descriptor->haveAudioRefLevel()) {
        mAudioRefLevel = sound_descriptor->getAudioRefLevel();
        mAudioRefLevelSet = true;
    } else {
        mAudioRefLevelSet = false;
    }

    if (sound_descriptor->haveDialNorm()) {
        mDialNorm = sound_descriptor->getDialNorm();
        mDialNormSet = true;
    } else {
        mDialNormSet = false;
    }
}

void SoundMXFDescriptorHelper::SetSamplingRate(mxfRational sampling_rate)
{
    IM_ASSERT(!mFileDescriptor);

    mSamplingRate = sampling_rate;
}

void SoundMXFDescriptorHelper::SetQuantizationBits(uint32_t bits)
{
    IM_ASSERT(!mFileDescriptor);
    IM_CHECK(bits > 0 && bits <= 32);

    mQuantizationBits = bits;
}

void SoundMXFDescriptorHelper::SetChannelCount(uint32_t count)
{
    IM_ASSERT(!mFileDescriptor);

    mChannelCount = count;
}

void SoundMXFDescriptorHelper::SetLocked(bool locked)
{
    IM_ASSERT(!mFileDescriptor);

    mLocked = locked;
    mLockedSet = true;
}

void SoundMXFDescriptorHelper::SetAudioRefLevel(int8_t level)
{
    IM_ASSERT(!mFileDescriptor);

    mAudioRefLevel = level;
    mAudioRefLevelSet = true;
}

void SoundMXFDescriptorHelper::SetDialNorm(int8_t dial_norm)
{
    IM_ASSERT(!mFileDescriptor);

    mDialNorm = dial_norm;
    mDialNormSet = true;
}

FileDescriptor* SoundMXFDescriptorHelper::CreateFileDescriptor(HeaderMetadata *header_metadata)
{
    (void)header_metadata;

    // implemented by child classes only
    IM_ASSERT(false);
    return 0;
}

void SoundMXFDescriptorHelper::UpdateFileDescriptor()
{
    MXFDescriptorHelper::UpdateFileDescriptor();

    GenericSoundEssenceDescriptor *sound_descriptor =
        dynamic_cast<GenericSoundEssenceDescriptor*>(mFileDescriptor);
    IM_ASSERT(sound_descriptor);

    sound_descriptor->setAudioSamplingRate(mSamplingRate);
    if (mLockedSet)
        sound_descriptor->setLocked(mLocked);
    sound_descriptor->setChannelCount(mChannelCount);
    sound_descriptor->setQuantizationBits(mQuantizationBits);
    if (mAudioRefLevelSet)
        sound_descriptor->setAudioRefLevel(mAudioRefLevel);
    if (mDialNormSet)
        sound_descriptor->setDialNorm(mDialNorm);
}

uint32_t SoundMXFDescriptorHelper::GetSampleSize()
{
    // implemented by child classes only
    IM_ASSERT(false);
    return 0;
}

mxfUL SoundMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    // implemented by child classes only
    IM_ASSERT(false);
    return g_Null_UL;
}

