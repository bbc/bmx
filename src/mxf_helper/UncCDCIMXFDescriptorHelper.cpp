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

#include <bmx/mxf_helper/UncCDCIMXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define AVID_IMAGE_ALIGNMENT    8192


typedef struct
{
    mxfUL ec_label;
    EssenceType essence_type;
    bool is_avid_10bit;
    mxfRational sample_rate;
    bool frame_wrapped;
    uint32_t display_width;
    uint32_t display_height;
    int32_t avid_display_y_offset;
    int32_t video_line_map[2];
    uint8_t frame_layout;
    uint8_t signal_standard;
    mxfUL coding_eq;
    int32_t avid_resolution_id;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped),     UNC_SD,       false,  {25, 1},         true,   720,    576,    16, {23, 336},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ,  0xaa},
    {MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped),      UNC_SD,       false,  {25, 1},         false,  720,    576,    16, {23, 336},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ,  0xaa},
    {MXF_EC_L(SD_Unc_525_5994i_422_135_FrameWrapped),   UNC_SD,       false,  {30000, 1001},   true,   720,    486,    10, {21, 283},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ,  0xaa},
    {MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped),    UNC_SD,       false,  {30000, 1001},   false,  720,    486,    10, {21, 283},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ,  0xaa},
    {MXF_EC_L(HD_Unc_1080_50i_422_FrameWrapped),        UNC_HD_1080I, false,  {25, 1},         true,   1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped),         UNC_HD_1080I, false,  {25, 1},         false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_5994i_422_FrameWrapped),      UNC_HD_1080I, false,  {30000, 1001},   true,   1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_5994i_422_ClipWrapped),       UNC_HD_1080I, false,  {30000, 1001},   false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_60i_422_FrameWrapped),        UNC_HD_1080I, false,  {30, 1},         true,   1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_60i_422_ClipWrapped),         UNC_HD_1080I, false,  {30, 1},         false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_25p_422_FrameWrapped),        UNC_HD_1080P, false,  {25, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_25p_422_ClipWrapped),         UNC_HD_1080P, false,  {25, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_2997p_422_FrameWrapped),      UNC_HD_1080P, false,  {30000, 1001},   true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_2997p_422_ClipWrapped),       UNC_HD_1080P, false,  {30000, 1001},   false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_30p_422_FrameWrapped),        UNC_HD_1080P, false,  {30, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_30p_422_ClipWrapped),         UNC_HD_1080P, false,  {30, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_50p_422_FrameWrapped),        UNC_HD_1080P, false,  {50, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_50p_422_ClipWrapped),         UNC_HD_1080P, false,  {50, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_5994p_422_FrameWrapped),      UNC_HD_1080P, false,  {60000, 1001},   true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_5994p_422_ClipWrapped),       UNC_HD_1080P, false,  {60000, 1001},   false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_60p_422_FrameWrapped),        UNC_HD_1080P, false,  {60, 1},         true,   1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_1080_60p_422_ClipWrapped),         UNC_HD_1080P, false,  {60, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_25p_422_FrameWrapped),         UNC_HD_720P,  false,  {25, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_25p_422_ClipWrapped),          UNC_HD_720P,  false,  {25, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_2997p_422_FrameWrapped),       UNC_HD_720P,  false,  {30000, 1001},   true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_2997p_422_ClipWrapped),        UNC_HD_720P,  false,  {30000, 1001},   false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_30p_422_FrameWrapped),         UNC_HD_720P,  false,  {30, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_30p_422_ClipWrapped),          UNC_HD_720P,  false,  {30, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_50p_422_FrameWrapped),         UNC_HD_720P,  false,  {50, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped),          UNC_HD_720P,  false,  {50, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_5994p_422_FrameWrapped),       UNC_HD_720P,  false,  {60000, 1001},   true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_5994p_422_ClipWrapped),        UNC_HD_720P,  false,  {60000, 1001},   false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_60p_422_FrameWrapped),         UNC_HD_720P,  false,  {60, 1},         true,   1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},
    {MXF_EC_L(HD_Unc_720_60p_422_ClipWrapped),          UNC_HD_720P,  false,  {60, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,  0x00},

    {MXF_EC_L(Unc_FrameWrapped),                        UNC_UHD_3840, false,  {-1, -1},        true,   3840,   2160,   0,  {1, 0},     MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_NONE,        g_Null_UL,             0x00},
    {MXF_EC_L(Unc_ClipWrapped),                         UNC_UHD_3840, false,  {-1, -1},        false,  3840,   2160,   0,  {1, 0},     MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_NONE,        g_Null_UL,             0x00},

    {MXF_EC_L(AvidUnc10Bit625ClipWrapped),              AVID_10BIT_UNC_SD,       true,  {25, 1},         false,  720,    576,    16, {23, 336},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ,   0x07e6},
    {MXF_EC_L(AvidUnc10Bit525ClipWrapped),              AVID_10BIT_UNC_SD,       true,  {30000, 1001},   false,  720,    486,    10, {21, 283},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ,   0x07e5},
    {MXF_EC_L(AvidUnc10Bit1080iClipWrapped),            AVID_10BIT_UNC_HD_1080I, true,  {25, 1},         false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,   0x07d0},
    {MXF_EC_L(AvidUnc10Bit1080iClipWrapped),            AVID_10BIT_UNC_HD_1080I, true,  {30000, 1001},   false,  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,   0x07d0},
    {MXF_EC_L(AvidUnc10Bit1080pClipWrapped),            AVID_10BIT_UNC_HD_1080P, true,  {25, 1},         false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,   0x07d3},
    {MXF_EC_L(AvidUnc10Bit1080pClipWrapped),            AVID_10BIT_UNC_HD_1080P, true,  {30000, 1001},   false,  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ,   0x07d3},
    {MXF_EC_L(AvidUnc10Bit720pClipWrapped),             AVID_10BIT_UNC_HD_720P,  true,  {25, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,   0x07d4},
    {MXF_EC_L(AvidUnc10Bit720pClipWrapped),             AVID_10BIT_UNC_HD_720P,  true,  {30000, 1001},   false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,   0x07d4},
    {MXF_EC_L(AvidUnc10Bit720pClipWrapped),             AVID_10BIT_UNC_HD_720P,  true,  {50, 1},         false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,   0x07d4},
    {MXF_EC_L(AvidUnc10Bit720pClipWrapped),             AVID_10BIT_UNC_HD_720P,  true,  {60000, 1001},   false,  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,     MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ,   0x07d4},
};



EssenceType UncCDCIMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    size_t essence_index = GetEssenceIndex(file_descriptor, alternative_ec_label);
    if (essence_index == (size_t)(-1))
        return UNKNOWN_ESSENCE_TYPE;

    return SUPPORTED_ESSENCE[essence_index].essence_type;
}

bool UncCDCIMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

size_t UncCDCIMXFDescriptorHelper::GetEssenceIndex(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());
    mxfUL ec_label = file_descriptor->getEssenceContainer();

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor)
        return (size_t)(-1);

    uint32_t display_width = 0;
    if (pic_descriptor->haveDisplayWidth())
        display_width = pic_descriptor->getDisplayWidth();
    else if (pic_descriptor->haveStoredWidth())
        display_width = pic_descriptor->getStoredWidth();
    if (display_width == 0)
        return (size_t)(-1);

    uint32_t display_height = 0;
    if (pic_descriptor->haveDisplayHeight())
        display_height = pic_descriptor->getDisplayHeight();
    else if (pic_descriptor->haveStoredHeight())
        display_height = pic_descriptor->getStoredHeight();
    if (display_height == 0)
        return (size_t)(-1);

    uint8_t frame_layout = MXF_SEPARATE_FIELDS;
    if (pic_descriptor->haveFrameLayout())
        frame_layout = pic_descriptor->getFrameLayout();

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].display_width  != display_width ||
            SUPPORTED_ESSENCE[i].display_height != display_height ||
            SUPPORTED_ESSENCE[i].frame_layout   != frame_layout ||
            (SUPPORTED_ESSENCE[i].sample_rate.numerator != -1 &&
               SUPPORTED_ESSENCE[i].sample_rate != sample_rate))
        {
            continue;
        }

        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label))
        {
            return i;
        }
        else if (mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
                 mxf_equals_ul_mod_regver(&alternative_ec_label, &MXF_EC_L(HD_Unc_720_60p_422_ClipWrapped)) &&
                 SUPPORTED_ESSENCE[i].essence_type == UNC_HD_720P &&
                 sample_rate == FRAME_RATE_50)
        {
            // Avid 8-bit 720p50 sample file had the wrong label
            return i;
        }
    }

    return (size_t)(-1);
}

UncCDCIMXFDescriptorHelper::UncCDCIMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;;
    mComponentDepth = 8;
    mStoredDimensionsSet = false;
    mDisplayDimensionsSet = false;
    mSampledDimensionsSet = false;
    mVideoLineMap[0] = SUPPORTED_ESSENCE[0].video_line_map[0];
    mVideoLineMap[1] = SUPPORTED_ESSENCE[0].video_line_map[1];
    mVideoLineMapSet = false;

    SetDefaultDimensions();
}

UncCDCIMXFDescriptorHelper::~UncCDCIMXFDescriptorHelper()
{
}

void UncCDCIMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                            mxfUL alternative_ec_label)
{
    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mEssenceIndex = GetEssenceIndex(file_descriptor, alternative_ec_label);
    BMX_ASSERT(mEssenceIndex != (size_t)(-1));
    mEssenceType = SUPPORTED_ESSENCE[mEssenceIndex].essence_type;
    mFrameWrapped = SUPPORTED_ESSENCE[mEssenceIndex].frame_wrapped;
    mAvidResolutionId = SUPPORTED_ESSENCE[mEssenceIndex].avid_resolution_id;

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor);
    BMX_ASSERT(cdci_descriptor);

    if (cdci_descriptor->haveComponentDepth()) {
        mComponentDepth = cdci_descriptor->getComponentDepth();
    } else {
        if (SUPPORTED_ESSENCE[mEssenceIndex].is_avid_10bit)
            mComponentDepth = 10;
        else
            mComponentDepth = 8;
        log_warn("Uncompressed picture descriptor is missing ComponentDepth\n");
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

    mVideoLineMap[0] = 0;
    mVideoLineMap[1] = 0;
    if (cdci_descriptor->haveVideoLineMap()) {
        vector<int32_t> video_line_map = cdci_descriptor->getVideoLineMap();
        if (video_line_map.size() > 0)
            mVideoLineMap[0] = video_line_map[0];
        if (video_line_map.size() > 1)
            mVideoLineMap[1] = video_line_map[1];
    }
}

void UncCDCIMXFDescriptorHelper::SetComponentDepth(uint32_t depth)
{
    BMX_ASSERT(!mFileDescriptor);
    BMX_CHECK(depth == 8 || depth == 10);

    mComponentDepth = depth;
}

void UncCDCIMXFDescriptorHelper::SetStoredDimensions(uint32_t width, uint32_t height)
{
    mStoredWidth         = width;
    mStoredHeight        = height;
    mStoredDimensionsSet = true;
    SetDefaultDimensions();
}

void UncCDCIMXFDescriptorHelper::SetDisplayDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset)
{
    mDisplayWidth         = width;
    mDisplayHeight        = height;
    mDisplayXOffset       = x_offset;
    mDisplayYOffset       = y_offset;
    mDisplayDimensionsSet = true;
    SetDefaultDimensions();
}

void UncCDCIMXFDescriptorHelper::SetSampledDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset)
{
    mSampledWidth         = width;
    mSampledHeight        = height;
    mSampledXOffset       = x_offset;
    mSampledYOffset       = y_offset;
    mSampledDimensionsSet = true;
    SetDefaultDimensions();
}

void UncCDCIMXFDescriptorHelper::SetVideoLineMap(int32_t field1, int32_t field2)
{
    mVideoLineMap[0] = field1;
    mVideoLineMap[1] = field2;
    mVideoLineMapSet = true;
}

void UncCDCIMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void UncCDCIMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

void UncCDCIMXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFrameWrapped(frame_wrapped);

    UpdateEssenceIndex();
}

FileDescriptor* UncCDCIMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    BMX_CHECK(UpdateEssenceIndex());

    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void UncCDCIMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    cdci_descriptor->setComponentDepth(mComponentDepth);
    cdci_descriptor->setHorizontalSubsampling(2);
    cdci_descriptor->setVerticalSubsampling(1);
    if ((mFlavour & MXFDESC_AVID_FLAVOUR) || (mFlavour & MXFDESC_SMPTE_377_2004_FLAVOUR))
        SetColorSiting(MXF_COLOR_SITING_REC601);
    else
        SetColorSiting(MXF_COLOR_SITING_COSITING);
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    if (!(mFlavour & MXFDESC_AVID_FLAVOUR) &&
        SUPPORTED_ESSENCE[mEssenceIndex].frame_layout == MXF_MIXED_FIELDS)
    {
        cdci_descriptor->setFieldDominance(1); // field 1
    }

    if (mComponentDepth == 8) {
        if (!(mFlavour & MXFDESC_AVID_FLAVOUR))
            cdci_descriptor->setPictureEssenceCoding(MXF_CMDEF_L(UNC_8B_422_INTERLEAVED));
        cdci_descriptor->setBlackRefLevel(16);
        cdci_descriptor->setWhiteReflevel(235);
        cdci_descriptor->setColorRange(225);
    } else {
        if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
            if (mEssenceType == AVID_10BIT_UNC_SD)
                cdci_descriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidUncSD10Bit));
            else
                cdci_descriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidUncHD10Bit));
        } else {
            cdci_descriptor->setPictureEssenceCoding(MXF_CMDEF_L(UNC_10B_422_INTERLEAVED));
        }
        cdci_descriptor->setBlackRefLevel(64);
        cdci_descriptor->setWhiteReflevel(940);
        cdci_descriptor->setColorRange(897);
    }

    if (!mxf_equals_ul(&SUPPORTED_ESSENCE[mEssenceIndex].coding_eq, &g_Null_UL) &&
        (!(mFlavour & MXFDESC_AVID_FLAVOUR) || (mEssenceType != UNC_SD && mEssenceType != AVID_10BIT_UNC_SD)))
    {
        SetCodingEquations(SUPPORTED_ESSENCE[mEssenceIndex].coding_eq);
    }
    if (!(mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setSignalStandard(SUPPORTED_ESSENCE[mEssenceIndex].signal_standard);
    cdci_descriptor->appendVideoLineMap(mVideoLineMap[0]);
    cdci_descriptor->appendVideoLineMap(mVideoLineMap[1]);
    cdci_descriptor->setStoredWidth(mStoredWidth);
    cdci_descriptor->setStoredHeight(mStoredHeight);
    cdci_descriptor->setDisplayWidth(mDisplayWidth);
    cdci_descriptor->setDisplayHeight(mDisplayHeight);
    if (mDisplayXOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setDisplayXOffset(mDisplayXOffset);
    if (mDisplayYOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setDisplayYOffset(mDisplayYOffset);
    cdci_descriptor->setSampledWidth(mSampledWidth);
    cdci_descriptor->setSampledHeight(mSampledHeight);
    if (mSampledXOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setSampledXOffset(mSampledXOffset);
    if (mSampledYOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setSampledYOffset(mSampledYOffset);
}

uint32_t UncCDCIMXFDescriptorHelper::GetImageAlignmentOffset()
{
    if (mImageAlignmentOffsetSet)
        return mImageAlignmentOffset;
    else if ((mFlavour & MXFDESC_AVID_FLAVOUR))
        return AVID_IMAGE_ALIGNMENT;
    else
        return 1;
}

uint32_t UncCDCIMXFDescriptorHelper::GetImageStartOffset()
{
    if (mImageStartOffsetSet)
        return mImageStartOffset;
    else if (!(mFlavour & MXFDESC_AVID_FLAVOUR))
        return 0;

    uint32_t image_alignment = GetImageAlignmentOffset();
    if (image_alignment <= 1)
        return 0;
    else
        return (image_alignment - (GetSampleSize(0) % image_alignment)) % image_alignment;
}

uint32_t UncCDCIMXFDescriptorHelper::GetSampleSize()
{
    return GetSampleSize(0);
}

uint32_t UncCDCIMXFDescriptorHelper::GetSampleSize(uint32_t input_height)
{
    uint32_t height = input_height;
    if (height == 0)
        height = mStoredHeight;

    if (mComponentDepth == 8)
        return mStoredWidth * height * 2;
    else if (SUPPORTED_ESSENCE[mEssenceIndex].is_avid_10bit)
        return mStoredWidth * height * 5 / 2;
    else
        return mStoredWidth / 48 * 128 * height;
}

mxfUL UncCDCIMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return SUPPORTED_ESSENCE[mEssenceIndex].ec_label;
}

bool UncCDCIMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            (SUPPORTED_ESSENCE[i].sample_rate.numerator == -1 ||
                SUPPORTED_ESSENCE[i].sample_rate == mSampleRate) &&
            SUPPORTED_ESSENCE[i].frame_wrapped == mFrameWrapped)
        {
            mEssenceIndex = i;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            if (SUPPORTED_ESSENCE[i].is_avid_10bit)
                mComponentDepth = 10;
            else if ((mFlavour & MXFDESC_AVID_FLAVOUR))
                mComponentDepth = 8;
            SetDefaultDimensions();
            return true;
        }
    }

    return false;
}

void UncCDCIMXFDescriptorHelper::SetDefaultDimensions()
{
    if (!mStoredDimensionsSet) {
        if (mComponentDepth == 8 || SUPPORTED_ESSENCE[mEssenceIndex].is_avid_10bit)
            mStoredWidth = SUPPORTED_ESSENCE[mEssenceIndex].display_width;
        else
            mStoredWidth = (SUPPORTED_ESSENCE[mEssenceIndex].display_width + 47) / 48 * 48;
        mStoredHeight = SUPPORTED_ESSENCE[mEssenceIndex].display_height;
        if ((mFlavour & MXFDESC_AVID_FLAVOUR))
            mStoredHeight += SUPPORTED_ESSENCE[mEssenceIndex].avid_display_y_offset;
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
            if ((mFlavour & MXFDESC_AVID_FLAVOUR))
                mDisplayYOffset = SUPPORTED_ESSENCE[mEssenceIndex].avid_display_y_offset;
            else
                mDisplayYOffset = 0;
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
            mSampledHeight = SUPPORTED_ESSENCE[mEssenceIndex].display_height;
            if ((mFlavour & MXFDESC_AVID_FLAVOUR))
                mSampledHeight += SUPPORTED_ESSENCE[mEssenceIndex].avid_display_y_offset;
            mSampledXOffset = 0;
            mSampledYOffset = 0;
        }
    }

    if (!mVideoLineMapSet) {
        mVideoLineMap[0] = SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[0];
        mVideoLineMap[1] = SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[1];
        if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
            if (SUPPORTED_ESSENCE[mEssenceIndex].frame_layout == MXF_MIXED_FIELDS) {
                mVideoLineMap[0] -= SUPPORTED_ESSENCE[mEssenceIndex].avid_display_y_offset / 2;
                mVideoLineMap[1] -= SUPPORTED_ESSENCE[mEssenceIndex].avid_display_y_offset / 2;
            } else {
                mVideoLineMap[0] -= SUPPORTED_ESSENCE[mEssenceIndex].avid_display_y_offset;
            }
        }
    }
}

