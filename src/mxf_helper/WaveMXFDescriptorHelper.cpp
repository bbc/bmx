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

#include <bmx/mxf_helper/WaveMXFDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    mxfUL ec_label;
    EssenceType essence_type;
    bool frame_wrapped;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_EC_L(BWFFrameWrapped),     WAVE_PCM,     true},
    {MXF_EC_L(BWFClipWrapped),      WAVE_PCM,     false},
    {MXF_EC_L(AES3FrameWrapped),    WAVE_PCM,     true},
    {MXF_EC_L(AES3ClipWrapped),     WAVE_PCM,     false},
};



EssenceType WaveMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool WaveMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
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
    mUseAES3AudioDescriptor = false;
}

WaveMXFDescriptorHelper::~WaveMXFDescriptorHelper()
{
}

void WaveMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                         mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    SoundMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label)) {
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mFrameWrapped = SUPPORTED_ESSENCE[i].frame_wrapped;
            break;
        }
    }
    BMX_ASSERT(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
}

void WaveMXFDescriptorHelper::SetSequenceOffset(uint8_t offset)
{
    mSequenceOffset = offset;

    if (mFileDescriptor) {
        WaveAudioDescriptor *wav_descriptor = dynamic_cast<WaveAudioDescriptor*>(mFileDescriptor);
        BMX_ASSERT(wav_descriptor);
        if ((!wav_descriptor->haveSequenceOffset() && mSequenceOffset != 0) ||
            ( wav_descriptor->haveSequenceOffset() && mSequenceOffset != wav_descriptor->getSequenceOffset()))
        {
            wav_descriptor->setSequenceOffset(mSequenceOffset);
        }
    }
}

void WaveMXFDescriptorHelper::SetUseAES3AudioDescriptor(bool enable)
{
    BMX_ASSERT(!mFileDescriptor);
    mUseAES3AudioDescriptor = enable;
}

FileDescriptor* WaveMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    if ((mFlavour & MXFDESC_RDD9_FLAVOUR) ||
        (mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR) ||
        mUseAES3AudioDescriptor)
    {
        mFileDescriptor = new AES3AudioDescriptor(header_metadata);
    }
    else
    {
        mFileDescriptor = new WaveAudioDescriptor(header_metadata);
    }
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void WaveMXFDescriptorHelper::UpdateFileDescriptor()
{
    SoundMXFDescriptorHelper::UpdateFileDescriptor();

    WaveAudioDescriptor *wav_descriptor = dynamic_cast<WaveAudioDescriptor*>(mFileDescriptor);
    BMX_ASSERT(wav_descriptor);

    uint32_t sample_size = GetSampleSize();
    if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        wav_descriptor->setSoundEssenceCompression(MXF_CMDEF_L(UNDEFINED_SOUND));
    wav_descriptor->setBlockAlign(sample_size);
    wav_descriptor->setAvgBps(sample_size * mSamplingRate.numerator / mSamplingRate.denominator);
    if (mSequenceOffset > 0)
        wav_descriptor->setSequenceOffset(mSequenceOffset);
    if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR)) { // Note: this trumps RDD9 flavour
        // Professional use, linear PCM, no emphasis, 48KHz sampling, CRCC value 60
        static const mxfAES3FixedData fixed_channel_status_data =
        {
            {
                0x85, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60
            }
        };

        AES3AudioDescriptor *aes3_descriptor = dynamic_cast<AES3AudioDescriptor*>(mFileDescriptor);
        BMX_ASSERT(aes3_descriptor);

        aes3_descriptor->appendChannelStatusMode(2); // STANDARD mode
        aes3_descriptor->appendFixedChannelStatusData(fixed_channel_status_data);
    } else if ((mFlavour & MXFDESC_RDD9_FLAVOUR)) {
        // Professional use, linear PCM, no emphasis, 48KHz sampling
        static const mxfAES3FixedData fixed_channel_status_data =
        {
            {
                0x85, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            }
        };

        AES3AudioDescriptor *aes3_descriptor = dynamic_cast<AES3AudioDescriptor*>(mFileDescriptor);
        BMX_ASSERT(aes3_descriptor);

        aes3_descriptor->appendChannelStatusMode(1); // MINIMUM mode
        aes3_descriptor->appendFixedChannelStatusData(fixed_channel_status_data);
    }
}

uint32_t WaveMXFDescriptorHelper::GetSampleSize()
{
    return mChannelCount * ((mQuantizationBits + 7) / 8);
}

mxfUL WaveMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if ((mFlavour & MXFDESC_RDD9_FLAVOUR) ||
        (mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR) ||
        mUseAES3AudioDescriptor)
    {
        if (mFrameWrapped)
            return MXF_EC_L(AES3FrameWrapped);
        else
            return MXF_EC_L(AES3ClipWrapped);
    }
    else
    {
        if (mFrameWrapped)
            return MXF_EC_L(BWFFrameWrapped);
        else
            return MXF_EC_L(BWFClipWrapped);
    }
}

