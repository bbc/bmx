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

#include <im/mxf_helper/MXFDescriptorHelper.h>
#include <im/mxf_helper/PictureMXFDescriptorHelper.h>
#include <im/mxf_helper/SoundMXFDescriptorHelper.h>
#include <im/Utils.h>
#include <im/IMTypes.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    MXFDescriptorHelper::EssenceType essence_type;
    const char *str;
} EssenceTypeStringMap;

static const EssenceTypeStringMap ESSENCE_TYPE_STRING_MAP[] =
{
    {MXFDescriptorHelper::UNKNOWN_ESSENCE,          "unknown essence"},
    {MXFDescriptorHelper::PICTURE_ESSENCE,          "picture essence"},
    {MXFDescriptorHelper::SOUND_ESSENCE,            "sound essence"},
    {MXFDescriptorHelper::D10_30,                   "D10 30Mbps"},
    {MXFDescriptorHelper::D10_40,                   "D10 40Mbps"},
    {MXFDescriptorHelper::D10_50,                   "D10 50Mbps"},
    {MXFDescriptorHelper::IEC_DV25,                 "IEC DV25"},
    {MXFDescriptorHelper::DVBASED_DV25,             "DV-Based DV25"},
    {MXFDescriptorHelper::DV50,                     "DV50"},
    {MXFDescriptorHelper::DV100_1080I,              "DV100 1080i"},
    {MXFDescriptorHelper::DV100_720P,               "DV100 720p"},
    {MXFDescriptorHelper::AVCI100_1080I,            "AVCI 100Mbps 1080i"},
    {MXFDescriptorHelper::AVCI100_1080P,            "AVCI 100Mbps 1080p"},
    {MXFDescriptorHelper::AVCI100_720P,             "AVCI 100Mbps 720p"},
    {MXFDescriptorHelper::AVCI50_1080I,             "AVCI 50Mbps 1080i"},
    {MXFDescriptorHelper::AVCI50_1080P,             "AVCI 50Mbps 1080p"},
    {MXFDescriptorHelper::AVCI50_720P,              "AVCI 50Mbps 720p"},
    {MXFDescriptorHelper::UNC_SD,                   "uncompressed SD"},
    {MXFDescriptorHelper::UNC_HD_1080I,             "uncompressed HD 1080i"},
    {MXFDescriptorHelper::UNC_HD_1080P,             "uncompressed HD 1080p"},
    {MXFDescriptorHelper::UNC_HD_720P,              "uncompressed HD 720p"},
    {MXFDescriptorHelper::AVID_10BIT_UNC_SD,        "Avid 10-bit uncompressed SD"},
    {MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080I,  "Avid 10-bit uncompressed HD 1080i"},
    {MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080P,  "Avid 10-bit uncompressed HD 1080p"},
    {MXFDescriptorHelper::AVID_10BIT_UNC_HD_720P,   "Avid 10-bit uncompressed HD 720p"},
    {MXFDescriptorHelper::MPEG2LG_422P_HL,          "MPEG-2 Long GOP 422P@HL"},
    {MXFDescriptorHelper::MPEG2LG_MP_HL,            "MPEG-2 Long GOP MP@HL"},
    {MXFDescriptorHelper::MPEG2LG_MP_H14,           "MPEG-2 Long GOP MP@H14"},
    {MXFDescriptorHelper::VC3_1080P_1235,           "VC3 1080p 1235"},
    {MXFDescriptorHelper::VC3_1080P_1237,           "VC3 1080p 1237"},
    {MXFDescriptorHelper::VC3_1080P_1238,           "VC3 1080p 1238"},
    {MXFDescriptorHelper::VC3_1080I_1241,           "VC3 1080i 1241"},
    {MXFDescriptorHelper::VC3_1080I_1242,           "VC3 1080i 1242"},
    {MXFDescriptorHelper::VC3_1080I_1243,           "VC3 1080i 1243"},
    {MXFDescriptorHelper::VC3_720P_1250,            "VC3 720p 1250"},
    {MXFDescriptorHelper::VC3_720P_1251,            "VC3 720p 1251"},
    {MXFDescriptorHelper::VC3_720P_1252,            "VC3 720p 1252"},
    {MXFDescriptorHelper::VC3_1080P_1253,           "VC3 1080p 1253"},
    {MXFDescriptorHelper::MJPEG_2_1,                "MJPEG 2:1"},
    {MXFDescriptorHelper::MJPEG_3_1,                "MJPEG 3:1"},
    {MXFDescriptorHelper::MJPEG_10_1,               "MJPEG 10:1"},
    {MXFDescriptorHelper::MJPEG_20_1,               "MJPEG 20:1"},
    {MXFDescriptorHelper::MJPEG_4_1M,               "MJPEG 4:1m"},
    {MXFDescriptorHelper::MJPEG_10_1M,              "MJPEG 10:1m"},
    {MXFDescriptorHelper::MJPEG_15_1S,              "MJPEG 15:1s"},
    {MXFDescriptorHelper::WAVE_PCM,                 "WAVE PCM"},
};



MXFDescriptorHelper::EssenceType MXFDescriptorHelper::IsSupported(mxfpp::FileDescriptor *file_descriptor,
                                                                  mxfUL alternative_ec_label)
{
    EssenceType essence_type = PictureMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    else
        return SoundMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
}

MXFDescriptorHelper* MXFDescriptorHelper::Create(mxfpp::FileDescriptor *file_descriptor,
                                                 mxfUL alternative_ec_label)
{
    if (PictureMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        return PictureMXFDescriptorHelper::Create(file_descriptor, alternative_ec_label);
    else if (SoundMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        return SoundMXFDescriptorHelper::Create(file_descriptor, alternative_ec_label);

    IM_ASSERT(false);
    return 0;
}

MXFDescriptorHelper* MXFDescriptorHelper::Create(EssenceType essence_type)
{
    if (PictureMXFDescriptorHelper::IsSupported(essence_type))
        return PictureMXFDescriptorHelper::Create(essence_type);
    else if (SoundMXFDescriptorHelper::IsSupported(essence_type))
        return SoundMXFDescriptorHelper::Create(essence_type);

    IM_ASSERT(false);
    return 0;
}

string MXFDescriptorHelper::EssenceTypeToString(EssenceType essence_type)
{
    IM_ASSERT((size_t)essence_type < ARRAY_SIZE(ESSENCE_TYPE_STRING_MAP));
    IM_ASSERT(ESSENCE_TYPE_STRING_MAP[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_STRING_MAP[essence_type].str;
}

MXFDescriptorHelper::MXFDescriptorHelper()
{
    IM_ASSERT(ARRAY_SIZE(ESSENCE_TYPE_STRING_MAP) - 1 == WAVE_PCM);

    mSampleRate = FRAME_RATE_25;
    mFrameWrapped = true;
    mFileDescriptor = 0;
    mFlavour = SMPTE_377_2004_FLAVOUR;

    // mEssenceType is set by subclass
}

MXFDescriptorHelper::~MXFDescriptorHelper()
{
}

void MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    (void)alternative_ec_label;

    mSampleRate = file_descriptor->getSampleRate();
    // mEssenceType and mFrameWrapped are set by subclass

    mFileDescriptor = file_descriptor;
}

void MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    IM_ASSERT(!mFileDescriptor);

    mEssenceType = essence_type;
}

void MXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    IM_ASSERT(!mFileDescriptor);
    IM_CHECK((sample_rate.numerator == 25    && sample_rate.denominator == 1) ||
             (sample_rate.numerator == 30000 && sample_rate.denominator == 1001) ||
             (sample_rate.numerator == 50    && sample_rate.denominator == 1) ||
             (sample_rate.numerator == 60000 && sample_rate.denominator == 1001) ||
             (sample_rate.numerator == 60    && sample_rate.denominator == 1) ||
             (IsSound() &&
                 (sample_rate.numerator == 48000 && sample_rate.denominator == 1)));

    mSampleRate = sample_rate;
}

void MXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    IM_ASSERT(!mFileDescriptor);

    mFrameWrapped = frame_wrapped;
}

void MXFDescriptorHelper::SetFlavour(DescriptorFlavour flavour)
{
    IM_ASSERT(!mFileDescriptor);

    mFlavour = flavour;
    if (flavour == AVID_FLAVOUR)
        mFrameWrapped = false;
}

void MXFDescriptorHelper::UpdateFileDescriptor()
{
    mFileDescriptor->setEssenceContainer(ChooseEssenceContainerUL());
    mFileDescriptor->setSampleRate(mSampleRate);
}

mxfUL MXFDescriptorHelper::GetEssenceContainerUL() const
{
    if (mFileDescriptor)
        return mFileDescriptor->getEssenceContainer();
    else
        return ChooseEssenceContainerUL();
}

