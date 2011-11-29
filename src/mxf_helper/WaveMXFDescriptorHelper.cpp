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

#include <im/mxf_helper/WaveMXFDescriptorHelper.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    mxfUL ec_label;
    MXFDescriptorHelper::EssenceType essence_type;
    bool frame_wrapped;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_EC_L(BWFFrameWrapped),     MXFDescriptorHelper::WAVE_PCM,     true},
    {MXF_EC_L(BWFClipWrapped),      MXFDescriptorHelper::WAVE_PCM,     false},
    {MXF_EC_L(AES3FrameWrapped),    MXFDescriptorHelper::WAVE_PCM,     true},
    {MXF_EC_L(AES3ClipWrapped),     MXFDescriptorHelper::WAVE_PCM,     false},
};



MXFDescriptorHelper::EssenceType WaveMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor,
                                                                      mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&ec_label, &SUPPORTED_ESSENCE[i].ec_label) ||
            (mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
                mxf_equals_ul_mod_regver(&alternative_ec_label, &SUPPORTED_ESSENCE[i].ec_label)))
        {
            return SUPPORTED_ESSENCE[i].essence_type;
        }
    }

    return MXFDescriptorHelper::UNKNOWN_ESSENCE;
}

bool WaveMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

WaveMXFDescriptorHelper::WaveMXFDescriptorHelper()
: SoundMXFDescriptorHelper()
{
    mEssenceType = WAVE_PCM;
    mSequenceOffset = 0;
}

WaveMXFDescriptorHelper::~WaveMXFDescriptorHelper()
{
}

void WaveMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    IM_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    SoundMXFDescriptorHelper::Initialize(file_descriptor, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&ec_label, &SUPPORTED_ESSENCE[i].ec_label) ||
            (mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
                mxf_equals_ul_mod_regver(&alternative_ec_label, &SUPPORTED_ESSENCE[i].ec_label)))
        {
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mFrameWrapped = SUPPORTED_ESSENCE[i].frame_wrapped;
            break;
        }
    }
    IM_ASSERT(i < ARRAY_SIZE(SUPPORTED_ESSENCE));
}

void WaveMXFDescriptorHelper::SetSequenceOffset(uint8_t offset)
{
    IM_ASSERT(!mFileDescriptor);

    mSequenceOffset = offset;
}

FileDescriptor* WaveMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new WaveAudioDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void WaveMXFDescriptorHelper::UpdateFileDescriptor()
{
    SoundMXFDescriptorHelper::UpdateFileDescriptor();

    WaveAudioDescriptor *wav_descriptor = dynamic_cast<WaveAudioDescriptor*>(mFileDescriptor);
    IM_ASSERT(wav_descriptor);

    uint32_t sample_size = GetSampleSize();
    wav_descriptor->setBlockAlign(sample_size);
    wav_descriptor->setAvgBps(sample_size * mSamplingRate.numerator / mSamplingRate.denominator);
    if (mSequenceOffset > 0)
        wav_descriptor->setSequenceOffset(mSequenceOffset);
}

uint32_t WaveMXFDescriptorHelper::GetSampleSize()
{
    return mChannelCount * ((mQuantizationBits + 7) / 8);
}

mxfUL WaveMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if (mFrameWrapped)
        return MXF_EC_L(BWFFrameWrapped);
    else
        return MXF_EC_L(BWFClipWrapped);
}

