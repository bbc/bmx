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

#include <bmx/mxf_helper/DVMXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    mxfUL pc_label;
    mxfUL ec_label;
    EssenceType essence_type;
    mxfRational sample_rate;
    bool frame_wrapped;
    int32_t avid_resolution_id;
    uint32_t sample_size;
    uint32_t horiz_subs;
    uint32_t vert_subs;
    uint32_t stored_width;
    uint32_t stored_height;
    uint32_t avid_stored_width;
    int32_t video_line_map[2];
    uint8_t color_siting;
    uint8_t frame_layout;
    uint8_t signal_standard;
    mxfUL coding_eq;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(IECDV_25_625_50),          MXF_EC_L(IECDV_25_625_50_FrameWrapped),        IEC_DV25,         {25, 1},         true,    0x00,  144000,  2,  2,  720,    288,    720,    {23, 335},  MXF_COLOR_SITING_LINE_ALTERN,   MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(IECDV_25_625_50),          MXF_EC_L(IECDV_25_625_50_ClipWrapped),         IEC_DV25,         {25, 1},         false,   0x8d,  144000,  2,  2,  720,    288,    720,    {23, 335},  MXF_COLOR_SITING_LINE_ALTERN,   MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(IECDV_25_525_60),          MXF_EC_L(IECDV_25_525_60_FrameWrapped),        IEC_DV25,         {30000, 1001},   true,    0x00,  120000,  4,  1,  720,    240,    720,    {23, 285},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(IECDV_25_525_60),          MXF_EC_L(IECDV_25_525_60_ClipWrapped),         IEC_DV25,         {30000, 1001},   false,   0x8c,  120000,  4,  1,  720,    240,    720,    {23, 285},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_25_625_50),        MXF_EC_L(DVBased_25_625_50_FrameWrapped),      DVBASED_DV25,     {25, 1},         true,    0x00,  144000,  4,  1,  720,    288,    720,    {23, 335},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_25_625_50),        MXF_EC_L(DVBased_25_625_50_ClipWrapped),       DVBASED_DV25,     {25, 1},         false,   0x8c,  144000,  4,  1,  720,    288,    720,    {23, 335},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_25_525_60),        MXF_EC_L(DVBased_25_525_60_FrameWrapped),      DVBASED_DV25,     {30000, 1001},   true,    0x00,  120000,  4,  1,  720,    240,    720,    {23, 285},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_25_525_60),        MXF_EC_L(DVBased_25_525_60_ClipWrapped),       DVBASED_DV25,     {30000, 1001},   false,   0x8c,  120000,  4,  1,  720,    240,    720,    {23, 285},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_50_625_50),        MXF_EC_L(DVBased_50_625_50_FrameWrapped),      DV50,             {25, 1},         true,    0x00,  288000,  2,  1,  720,    288,    720,    {23, 335},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_50_625_50),        MXF_EC_L(DVBased_50_625_50_ClipWrapped),       DV50,             {25, 1},         false,   0x8e,  288000,  2,  1,  720,    288,    720,    {23, 335},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_50_525_60),        MXF_EC_L(DVBased_50_525_60_FrameWrapped),      DV50,             {30000, 1001},   true,    0x00,  240000,  2,  1,  720,    240,    720,    {23, 285},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_50_525_60),        MXF_EC_L(DVBased_50_525_60_ClipWrapped),       DV50,             {30000, 1001},   false,   0x8e,  240000,  2,  1,  720,    240,    720,    {23, 285},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_ITU601,      ITUR_BT601_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_1080_50_I),    MXF_EC_L(DVBased_100_1080_50_I_FrameWrapped),  DV100_1080I,      {25, 1},         true,    0x00,  576000,  2,  1,  1920,   540,    1440,   {21, 584},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_1080_50_I),    MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped),   DV100_1080I,      {25, 1},         false,   0x00,  576000,  2,  1,  1920,   540,    1440,   {21, 584},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_1080_60_I),    MXF_EC_L(DVBased_100_1080_60_I_FrameWrapped),  DV100_1080I,      {30000, 1001},   true,    0x00,  480000,  2,  1,  1920,   540,    1280,   {21, 584},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_1080_60_I),    MXF_EC_L(DVBased_100_1080_60_I_ClipWrapped),   DV100_1080I,      {30000, 1001},   false,   0x00,  480000,  2,  1,  1920,   540,    1280,   {21, 584},  MXF_COLOR_SITING_REC601,        MXF_SEPARATE_FIELDS,   MXF_SIGNAL_STANDARD_SMPTE274M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_50_P),     MXF_EC_L(DVBased_100_720_50_P_FrameWrapped),   DV100_720P,       {50, 1},         true,    0x00,  288000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_50_P),     MXF_EC_L(DVBased_100_720_50_P_ClipWrapped),    DV100_720P,       {50, 1},         false,   0x00,  288000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_50_P),     MXF_EC_L(DVBased_100_720_50_P_FrameWrapped),   DV100_720P,       {25, 1},         true,    0x00,  288000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_50_P),     MXF_EC_L(DVBased_100_720_50_P_ClipWrapped),    DV100_720P,       {25, 1},         false,   0x00,  288000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_60_P),     MXF_EC_L(DVBased_100_720_60_P_FrameWrapped),   DV100_720P,       {60000, 1001},   true,    0x00,  240000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_60_P),     MXF_EC_L(DVBased_100_720_60_P_ClipWrapped),    DV100_720P,       {60000, 1001},   false,   0x00,  240000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_60_P),     MXF_EC_L(DVBased_100_720_60_P_FrameWrapped),   DV100_720P,       {30000, 1001},   true,    0x00,  240000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_60_P),     MXF_EC_L(DVBased_100_720_60_P_ClipWrapped),    DV100_720P,       {30000, 1001},   false,   0x00,  240000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_60_P),     MXF_EC_L(DVBased_100_720_60_P_FrameWrapped),   DV100_720P,       {24000, 1001},   true,    0x00,  240000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
    {MXF_CMDEF_L(DVBased_100_720_60_P),     MXF_EC_L(DVBased_100_720_60_P_ClipWrapped),    DV100_720P,       {24000, 1001},   false,   0x00,  240000,  2,  1,  1280,   720,    960,    {26, 0},    MXF_COLOR_SITING_REC601,        MXF_FULL_FRAME,        MXF_SIGNAL_STANDARD_SMPTE296M,   ITUR_BT709_CODING_EQ},
};



EssenceType DVMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    (void)alternative_ec_label;

    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)))
        {
            // legacy Avid files use the wrong essence container label
            // eg. alternative_ec_label is DVBased_25_625_50_ClipWrapped for IEC DV25)
            // and so we check just the picture coding label
            if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
                !SUPPORTED_ESSENCE[i].frame_wrapped &&
                SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
            {
                return SUPPORTED_ESSENCE[i].essence_type;
            }
        }
        else if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
                 (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label) ||
                     IsNullAvidECUL(ec_label, alternative_ec_label)) &&
                 SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            return SUPPORTED_ESSENCE[i].essence_type;
        }
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool DVMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

DVMXFDescriptorHelper::DVMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;;
    mComponentDepth = 8;
}

DVMXFDescriptorHelper::~DVMXFDescriptorHelper()
{
}

void DVMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                       mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)))
        {
            // legacy Avid files use the wrong essence container label
            // eg. alternative_ec_label is DVBased_25_625_50_ClipWrapped for IEC DV25)
            // and so we check just the picture coding label
            if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
                !SUPPORTED_ESSENCE[i].frame_wrapped &&
                SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
            {
                break;
            }
        }
        else if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
                 (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label) ||
                     IsNullAvidECUL(ec_label, alternative_ec_label)) &&
                 SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            break;
        }
    }
    if (i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE)) {
        mEssenceIndex = i;
        mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
        mFrameWrapped = SUPPORTED_ESSENCE[i].frame_wrapped;
        mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
    }
}

void DVMXFDescriptorHelper::SetComponentDepth(uint32_t depth)
{
    BMX_ASSERT(!mFileDescriptor);
    BMX_CHECK(depth == 8 ||
             (depth == 10 && (mEssenceType == DV100_720P || mEssenceType == DV100_1080I)));

    mComponentDepth = depth;
}

void DVMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void DVMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

void DVMXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFrameWrapped(frame_wrapped);

    UpdateEssenceIndex();
}

FileDescriptor* DVMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void DVMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    SetColorSiting(SUPPORTED_ESSENCE[mEssenceIndex].color_siting);
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    cdci_descriptor->setImageAlignmentOffset(1);
    cdci_descriptor->setComponentDepth(mComponentDepth);
    cdci_descriptor->setBlackRefLevel(16);
    cdci_descriptor->setWhiteReflevel(235);
    cdci_descriptor->setColorRange(225);
    cdci_descriptor->setSignalStandard(SUPPORTED_ESSENCE[mEssenceIndex].signal_standard);
    SetCodingEquations(SUPPORTED_ESSENCE[mEssenceIndex].coding_eq);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[0]);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[1]);
    if ((mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setStoredWidth(SUPPORTED_ESSENCE[mEssenceIndex].avid_stored_width);
    else
        cdci_descriptor->setStoredWidth(SUPPORTED_ESSENCE[mEssenceIndex].stored_width);
    cdci_descriptor->setStoredHeight(SUPPORTED_ESSENCE[mEssenceIndex].stored_height);
    cdci_descriptor->setDisplayWidth(cdci_descriptor->getStoredWidth());
    cdci_descriptor->setDisplayHeight(cdci_descriptor->getStoredHeight());
    cdci_descriptor->setSampledWidth(cdci_descriptor->getStoredWidth());
    cdci_descriptor->setSampledHeight(cdci_descriptor->getStoredHeight());
    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    cdci_descriptor->setHorizontalSubsampling(SUPPORTED_ESSENCE[mEssenceIndex].horiz_subs);
    cdci_descriptor->setVerticalSubsampling(SUPPORTED_ESSENCE[mEssenceIndex].vert_subs);
}

uint32_t DVMXFDescriptorHelper::GetSampleSize()
{
    return SUPPORTED_ESSENCE[mEssenceIndex].sample_size;
}

mxfUL DVMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return SUPPORTED_ESSENCE[mEssenceIndex].ec_label;
}

void DVMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate &&
            SUPPORTED_ESSENCE[i].frame_wrapped == mFrameWrapped)
        {
            mEssenceIndex = i;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
}

