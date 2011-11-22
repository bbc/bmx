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
    uint32_t display_width;
    uint32_t display_height;
    int32_t display_y_offset;
    int32_t video_line_map[2];
    uint8_t frame_layout;
    uint8_t signal_standard;
    mxfUL coding_eq;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped),     MXFDescriptorHelper::UNC_SD,       {25, 1},         true,   720,    576,    0,  {23, 336},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped),      MXFDescriptorHelper::UNC_SD,       {25, 1},         false,  720,    576,    0,  {23, 336},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(SD_Unc_525_5994i_422_135_FrameWrapped),   MXFDescriptorHelper::UNC_SD,       {30000, 1001},   true,   720,    486,    0,  {21, 283},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped),    MXFDescriptorHelper::UNC_SD,       {30000, 1001},   false,  720,    486,    0,  {21, 283},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50i_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080I, {25, 1},         true,   1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080I, {25, 1},         false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994i_422_FrameWrapped),      MXFDescriptorHelper::UNC_HD_1080I, {30000, 1001},   true,   1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994i_422_ClipWrapped),       MXFDescriptorHelper::UNC_HD_1080I, {30000, 1001},   false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60i_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080I, {30, 1},         true,   1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60i_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080I, {30, 1},         false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_25p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {25, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_25p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {25, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_2997p_422_FrameWrapped),      MXFDescriptorHelper::UNC_HD_1080P, {30000, 1001},   true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_2997p_422_ClipWrapped),       MXFDescriptorHelper::UNC_HD_1080P, {30000, 1001},   false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_30p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {30, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_30p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {30, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {50, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_50p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {50, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994p_422_FrameWrapped),      MXFDescriptorHelper::UNC_HD_1080P, {60000, 1001},   true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_5994p_422_ClipWrapped),       MXFDescriptorHelper::UNC_HD_1080P, {60000, 1001},   false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60p_422_FrameWrapped),        MXFDescriptorHelper::UNC_HD_1080P, {60, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_1080_60p_422_ClipWrapped),         MXFDescriptorHelper::UNC_HD_1080P, {60, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_25p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {25, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_25p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {25, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_2997p_422_FrameWrapped),       MXFDescriptorHelper::UNC_HD_720P,  {30000, 1001},   true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_2997p_422_ClipWrapped),        MXFDescriptorHelper::UNC_HD_720P,  {30000, 1001},   false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_30p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {30, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_30p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {30, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_50p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {50, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {50, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_5994p_422_FrameWrapped),       MXFDescriptorHelper::UNC_HD_720P,  {60000, 1001},   true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_5994p_422_ClipWrapped),        MXFDescriptorHelper::UNC_HD_720P,  {60000, 1001},   false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_60p_422_FrameWrapped),         MXFDescriptorHelper::UNC_HD_720P,  {60, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(HD_Unc_720_60p_422_ClipWrapped),          MXFDescriptorHelper::UNC_HD_720P,  {60, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},

    // TODO: support writing Avid 8 and 10 bit uncompressed formats
    {MXF_EC_L(AvidUnc10Bit625ClipWrapped),              MXFDescriptorHelper::AVID_10BIT_UNC_SD,       {25, 1},         false,  720,    576,    16, {15, 328},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(AvidUnc10Bit525ClipWrapped),              MXFDescriptorHelper::AVID_10BIT_UNC_SD,       {30000, 1001},   false,  720,    486,    10, {16, 278},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_EC_L(AvidUnc10Bit1080iClipWrapped),            MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080I, {25, 1},         false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(AvidUnc10Bit1080iClipWrapped),            MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080I, {30000, 1001},   false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(AvidUnc10Bit720pClipWrapped),             MXFDescriptorHelper::AVID_10BIT_UNC_HD_720P,  {50, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_EC_L(AvidUnc10Bit720pClipWrapped),             MXFDescriptorHelper::AVID_10BIT_UNC_HD_720P,  {60000, 1001},   false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
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
    mStoredDimensionsSet = false;
    mDisplayDimensionsSet = false;
    mSampledDimensionsSet = false;

    SetDefaultDimensions();
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

    if (cdci_descriptor->haveComponentDepth()) {
        mComponentDepth = cdci_descriptor->getComponentDepth();
    } else {
        mComponentDepth = 8;
        log_warn("Uncompressed picture descriptor is missing ComponentDepth; assuming 8 bits\n");
    }

    if (cdci_descriptor->haveStoredWidth()) {
        mStoredWidth = cdci_descriptor->getStoredWidth();
    } else {
        mStoredWidth = 0;
        log_warn("Uncompressed picture descriptor is missing StoredWidth\n");
    }
    if (cdci_descriptor->haveStoredHeight()) {
        mStoredHeight = cdci_descriptor->getStoredHeight();
    } else {
        mStoredHeight = 0;
        log_warn("Uncompressed picture descriptor is missing StoredHeight\n");
    }
    mStoredDimensionsSet = true;

    if (cdci_descriptor->haveDisplayWidth())
        mDisplayWidth = cdci_descriptor->getDisplayWidth();
    else
        mDisplayWidth = mStoredWidth;
    if (cdci_descriptor->haveDisplayHeight())
        mDisplayHeight = cdci_descriptor->getDisplayHeight();
    else
        mDisplayHeight = mStoredHeight;
    if (cdci_descriptor->haveDisplayXOffset())
        mDisplayXOffset = cdci_descriptor->getDisplayXOffset();
    else
        mDisplayXOffset = 0;
    if (cdci_descriptor->haveDisplayYOffset())
        mDisplayYOffset = cdci_descriptor->getDisplayYOffset();
    else
        mDisplayYOffset = 0;
    mDisplayDimensionsSet = true;

    if (cdci_descriptor->haveSampledWidth())
        mSampledWidth = cdci_descriptor->getSampledWidth();
    else
        mSampledWidth = mStoredWidth;
    if (cdci_descriptor->haveSampledHeight())
        mSampledHeight = cdci_descriptor->getSampledHeight();
    else
        mSampledHeight = mStoredHeight;
    if (cdci_descriptor->haveSampledXOffset())
        mSampledXOffset = cdci_descriptor->getSampledXOffset();
    else
        mSampledXOffset = 0;
    if (cdci_descriptor->haveSampledYOffset())
        mSampledYOffset = cdci_descriptor->getSampledYOffset();
    else
        mSampledYOffset = 0;
    mSampledDimensionsSet = true;
}

void UncMXFDescriptorHelper::SetComponentDepth(uint32_t depth)
{
    IM_ASSERT(!mFileDescriptor);
    IM_CHECK(depth == 8 || depth == 10);

    mComponentDepth = depth;
}

void UncMXFDescriptorHelper::SetStoredDimensions(uint32_t width, uint32_t height)
{
    mStoredWidth         = width;
    mStoredHeight        = height;
    mStoredDimensionsSet = true;
    SetDefaultDimensions();
}

void UncMXFDescriptorHelper::SetDisplayDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset)
{
    mDisplayWidth         = width;
    mDisplayHeight        = height;
    mDisplayXOffset       = x_offset;
    mDisplayYOffset       = y_offset;
    mDisplayDimensionsSet = true;
    SetDefaultDimensions();
}

void UncMXFDescriptorHelper::SetSampledDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset)
{
    mSampledWidth         = width;
    mSampledHeight        = height;
    mSampledXOffset       = x_offset;
    mSampledYOffset       = y_offset;
    mSampledDimensionsSet = true;
    SetDefaultDimensions();
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
    cdci_descriptor->setStoredWidth(mStoredWidth);
    cdci_descriptor->setStoredHeight(mStoredHeight);
    cdci_descriptor->setDisplayWidth(mDisplayWidth);
    cdci_descriptor->setDisplayHeight(mDisplayHeight);
    if (mDisplayXOffset != 0)
        cdci_descriptor->setDisplayXOffset(mDisplayXOffset);
    if (mDisplayYOffset != 0)
        cdci_descriptor->setDisplayYOffset(mDisplayYOffset);
    cdci_descriptor->setSampledWidth(mSampledWidth);
    cdci_descriptor->setSampledHeight(mSampledHeight);
    if (mSampledXOffset != 0)
        cdci_descriptor->setSampledXOffset(mSampledXOffset);
    if (mSampledYOffset != 0)
        cdci_descriptor->setSampledYOffset(mSampledYOffset);
}

uint32_t UncMXFDescriptorHelper::GetSampleSize()
{
    if (mComponentDepth == 8)
        return mStoredWidth * mStoredHeight * 2;
    else
        return mStoredWidth / 48 * 128 * mStoredHeight;
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
            SetDefaultDimensions();
            break;
        }
    }
    IM_CHECK(i < SUPPORTED_ESSENCE_SIZE);
}

void UncMXFDescriptorHelper::SetDefaultDimensions()
{
    if (!mStoredDimensionsSet) {
        if (mComponentDepth == 8)
            mStoredWidth = SUPPORTED_ESSENCE[mEssenceIndex].display_width;
        else
            mStoredWidth = (SUPPORTED_ESSENCE[mEssenceIndex].display_width + 47) / 48 * 48;
        mStoredHeight = SUPPORTED_ESSENCE[mEssenceIndex].display_height +
                            SUPPORTED_ESSENCE[mEssenceIndex].display_y_offset;
    }
    if (!mDisplayDimensionsSet) {
        if (mStoredDimensionsSet) {
            mDisplayWidth   = mStoredWidth;
            mDisplayHeight  = mStoredHeight;
            mDisplayXOffset = 0;
            mDisplayYOffset = 0;
        } else {
            mDisplayWidth   = SUPPORTED_ESSENCE[mEssenceIndex].display_width;
            mDisplayHeight  = SUPPORTED_ESSENCE[mEssenceIndex].display_height;
            mDisplayXOffset = 0;
            mDisplayYOffset = SUPPORTED_ESSENCE[mEssenceIndex].display_y_offset;
        }
    }
    if (!mSampledDimensionsSet) {
        if (mStoredDimensionsSet) {
            mSampledWidth   = mStoredWidth;
            mSampledHeight  = mStoredHeight;
            mSampledXOffset = 0;
            mSampledYOffset = 0;
        } else {
            mSampledWidth  = SUPPORTED_ESSENCE[mEssenceIndex].display_width;
            mSampledHeight = SUPPORTED_ESSENCE[mEssenceIndex].display_height +
                                SUPPORTED_ESSENCE[mEssenceIndex].display_y_offset;
            mSampledXOffset = 0;
            mSampledYOffset = 0;
        }
    }
}

