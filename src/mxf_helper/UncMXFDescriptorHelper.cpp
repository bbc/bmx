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

#include <im/mxf_helper/UncMXFDescriptorHelper.h>
#include <im/MXFUtils.h>
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
    mxfRational sample_rate;
    bool frame_wrapped;
    uint32_t width;
    uint32_t height;
    int32_t video_line_map[2];
    uint8_t frame_layout;
    uint8_t signal_standard;
    mxfUL coding_eq;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped),     MXFDescriptorHelper::UNC_SD,       {25, 1},         true,   720,    576,    {23, 336},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped),      MXFDescriptorHelper::UNC_SD,       {25, 1},         false,  720,    576,    {23, 336},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(SD_Unc_525_5994i_422_135_FrameWrapped),   MXFDescriptorHelper::UNC_SD,       {30000, 1001},   true,   720,    486,    {21, 283},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped),    MXFDescriptorHelper::UNC_SD,       {30000, 1001},   false,  720,    486,    {21, 283},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50i_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080I, {25, 1},         true,   1920,   1080,   {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080I, {25, 1},         false,  1920,   1080,   {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994i_422_FrameWrapped),      MXFDescriptorHelper::UNC_HD_1080I, {30000, 1001},   true,   1920,   1080,   {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994i_422_ClipWrapped),       MXFDescriptorHelper::UNC_HD_1080I, {30000, 1001},   false,  1920,   1080,   {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60i_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080I, {30, 1},         true,   1920,   1080,   {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60i_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080I, {30, 1},         false,  1920,   1080,   {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_25p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {25, 1},         true,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_25p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {25, 1},         false,  1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_2997p_422_FrameWrapped),      MXFDescriptorHelper::UNC_HD_1080P, {30000, 1001},   true,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_2997p_422_ClipWrapped),       MXFDescriptorHelper::UNC_HD_1080P, {30000, 1001},   false,  1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_30p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {30, 1},         true,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_30p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {30, 1},         false,  1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {50, 1},         true,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {50, 1},         false,  1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994p_422_FrameWrapped),      MXFDescriptorHelper::UNC_HD_1080P, {60000, 1001},   true,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994p_422_ClipWrapped),       MXFDescriptorHelper::UNC_HD_1080P, {60000, 1001},   false,  1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {60, 1},         true,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {60, 1},         false,  1920,   1080,   {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_25p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {25, 1},         true,   1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_25p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {25, 1},         false,  1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_2997p_422_FrameWrapped),       MXFDescriptorHelper::UNC_HD_720P,  {30000, 1001},   true,   1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_2997p_422_ClipWrapped),        MXFDescriptorHelper::UNC_HD_720P,  {30000, 1001},   false,  1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_30p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {30, 1},         true,   1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_30p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {30, 1},         false,  1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_50p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {50, 1},         true,   1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {50, 1},         false,  1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_5994p_422_FrameWrapped),       MXFDescriptorHelper::UNC_HD_720P,  {60000, 1001},   true,   1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_5994p_422_ClipWrapped),        MXFDescriptorHelper::UNC_HD_720P,  {60000, 1001},   false,  1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_60p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {60, 1},         true,   1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_60p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {60, 1},         false,  1280,   720,    {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
};

#define SUPPORTED_ESSENCE_SIZE  (sizeof(SUPPORTED_ESSENCE) / sizeof(SupportedEssence))



MXFDescriptorHelper::EssenceType UncMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor,
                                                                     mxfUL alternative_ec_label)
{
    mxfRational sample_rate = file_descriptor->getSampleRate();

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if ((mxf_equals_ul_mod_regver(&ec_label, &SUPPORTED_ESSENCE[i].ec_label) ||
                (mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
                    mxf_equals_ul_mod_regver(&alternative_ec_label, &SUPPORTED_ESSENCE[i].ec_label))) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            return SUPPORTED_ESSENCE[i].essence_type;
        }
    }

    return MXFDescriptorHelper::UNKNOWN_ESSENCE;
}

bool UncMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

UncMXFDescriptorHelper::UncMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;;
    mComponentDepth = 8;
}

UncMXFDescriptorHelper::~UncMXFDescriptorHelper()
{
}

void UncMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    IM_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, alternative_ec_label);

    mxfRational sample_rate = file_descriptor->getSampleRate();

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if ((mxf_equals_ul_mod_regver(&ec_label, &SUPPORTED_ESSENCE[i].ec_label) ||
                (mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
                    mxf_equals_ul_mod_regver(&alternative_ec_label, &SUPPORTED_ESSENCE[i].ec_label))) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            mEssenceIndex = i;
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mFrameWrapped = SUPPORTED_ESSENCE[i].frame_wrapped;
            break;
        }
    }
    IM_ASSERT(i < SUPPORTED_ESSENCE_SIZE);

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor);
    IM_ASSERT(cdci_descriptor);

    if (cdci_descriptor->haveComponentDepth())
        mComponentDepth = cdci_descriptor->getComponentDepth();
    else
        mComponentDepth = 8;
}

void UncMXFDescriptorHelper::SetComponentDepth(uint32_t depth)
{
    IM_ASSERT(!mFileDescriptor);
    IM_CHECK(depth == 8 || depth == 10);

    mComponentDepth = depth;
}

void UncMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    IM_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void UncMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    IM_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

void UncMXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    IM_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFrameWrapped(frame_wrapped);

    UpdateEssenceIndex();
}

FileDescriptor* UncMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void UncMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    IM_ASSERT(cdci_descriptor);

    cdci_descriptor->setComponentDepth(mComponentDepth);
    cdci_descriptor->setHorizontalSubsampling(2);
    cdci_descriptor->setVerticalSubsampling(1);
    SetColorSiting(MXF_COLOR_SITING_REC601);
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    if (SUPPORTED_ESSENCE[mEssenceIndex].frame_layout == MXF_MIXED_FIELDS)
        cdci_descriptor->setFieldDominance(1); // field 1

    if (mComponentDepth == 8) {
        cdci_descriptor->setPictureEssenceCoding(MXF_CMDEF_L(UNC_8B_422_INTERLEAVED));
        cdci_descriptor->setBlackRefLevel(16);
        cdci_descriptor->setWhiteReflevel(235);
        cdci_descriptor->setColorRange(225);
    } else {
        cdci_descriptor->setPictureEssenceCoding(MXF_CMDEF_L(UNC_10B_422_INTERLEAVED));
        cdci_descriptor->setBlackRefLevel(64);
        cdci_descriptor->setWhiteReflevel(940);
        cdci_descriptor->setColorRange(897);
    }

    SetCodingEquations(SUPPORTED_ESSENCE[mEssenceIndex].coding_eq);
    cdci_descriptor->setSignalStandard(SUPPORTED_ESSENCE[mEssenceIndex].signal_standard);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[0]);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[1]);
    if (mComponentDepth == 8)
        cdci_descriptor->setStoredWidth(SUPPORTED_ESSENCE[mEssenceIndex].width);
    else
        cdci_descriptor->setStoredWidth((SUPPORTED_ESSENCE[mEssenceIndex].width + 47) / 48 * 48);
    cdci_descriptor->setStoredHeight(SUPPORTED_ESSENCE[mEssenceIndex].height);
    cdci_descriptor->setDisplayWidth(SUPPORTED_ESSENCE[mEssenceIndex].width);
    cdci_descriptor->setDisplayHeight(SUPPORTED_ESSENCE[mEssenceIndex].height);
    cdci_descriptor->setSampledWidth(SUPPORTED_ESSENCE[mEssenceIndex].width);
    cdci_descriptor->setSampledHeight(SUPPORTED_ESSENCE[mEssenceIndex].height);
}

uint32_t UncMXFDescriptorHelper::GetSampleSize()
{
    uint32_t width = 0;
    uint32_t height = 0;

    switch (mEssenceType)
    {
        case UNC_SD:
            width = 720;
            if (mSampleRate.numerator == 25)
                height = 576;
            else
                height = 486;
            break;
        case UNC_HD_1080I:
        case UNC_HD_1080P:
            width = 1920;
            height = 1080;
            break;
        case UNC_HD_720P:
            width = 1280;
            height = 720;
            break;
        default:
            IM_ASSERT(false);
    }

    if (mComponentDepth == 8)
        return width * height * 2;
    else
        return (width + 47) / 48 * 128 * height;
}

mxfUL UncMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return SUPPORTED_ESSENCE[mEssenceIndex].ec_label;
}

void UncMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate &&
            SUPPORTED_ESSENCE[i].frame_wrapped == mFrameWrapped)
        {
            mEssenceIndex = i;
            break;
        }
    }
    IM_CHECK(i < SUPPORTED_ESSENCE_SIZE);
}

