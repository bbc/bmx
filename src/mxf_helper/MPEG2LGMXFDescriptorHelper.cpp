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

#include <bmx/mxf_helper/MPEG2LGMXFDescriptorHelper.h>
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
    EssenceType essence_type;
    mxfRational sample_rate;
    uint32_t stored_width;
    int32_t avid_resolution_id;
    uint8_t frame_layout;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080I,     {25, 1},        1920,  4074,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080I,     {30000, 1001},  1920,  4073,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080P,     {24000, 1001},  1920,  4077,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080P,     {24, 1},        1920,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080P,     {25, 1},        1920,  4076,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080P,     {30000, 1001},  1920,  4075,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_720P,      {24000, 1001},  1280,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_720P,      {24, 1},        1280,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_720P,      {25, 1},        1280,  4091,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_720P,      {30000, 1001},  1280,  4092,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_720P,      {50, 1},        1280,  4079,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_720P,      {60000, 1001},  1280,  4078,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1920_1080I,  {25, 1},        1920,  4081,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1920_1080I,  {30000, 1001},  1920,  4080,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1920_1080P,  {24000, 1001},  1920,  4084,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1920_1080P,  {24, 1},        1920,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1920_1080P,  {25, 1},        1920,  4083,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1920_1080P,  {30000, 1001},  1920,  4082,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1440_1080I,  {25, 1},        1440,  4043,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1440_1080I,  {30000, 1001},  1440,  4044,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1440_1080P,  {24000, 1001},  1440,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1440_1080P,  {24, 1},        1440,  4045,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1440_1080P,  {25, 1},        1440,  4072,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1440_1080P,  {30000, 1001},  1440,  4090,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_720P,        {24000, 1001},  1280,  4087,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_720P,        {24, 1},        1280,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_720P,        {25, 1},        1280,  4089,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_720P,        {30000, 1001},  1280,  4088,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_720P,        {50, 1},        1280,  4086,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_720P,        {60000, 1001},  1280,  4085,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080I,      {25, 1},        1440,  4003,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080I,      {30000, 1001},  1440,  4004,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080P,      {24000, 1001},  1440,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080P,      {24, 1},        1440,     0,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080P,      {25, 1},        1440,  4071,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080P,      {30000, 1001},  1440,     0,  MXF_FULL_FRAME},
};



EssenceType MPEG2LGMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    size_t essence_index = GetEssenceIndex(file_descriptor, alternative_ec_label);
    if (essence_index == (size_t)(-1))
        return UNKNOWN_ESSENCE_TYPE;

    return SUPPORTED_ESSENCE[essence_index].essence_type;
}

bool MPEG2LGMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

size_t MPEG2LGMXFDescriptorHelper::GetEssenceIndex(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_is_mpeg_video_ec(&ec_label, 0) &&
        !mxf_is_mpeg_video_ec(&ec_label, 1) &&
        !mxf_is_mpeg_video_ec(&alternative_ec_label, 0) &&
        !mxf_is_mpeg_video_ec(&alternative_ec_label, 1) &&
        !CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(AvidMPEGClipWrapped)) &&
        !IsNullAvidECUL(ec_label, alternative_ec_label))
    {
        return (size_t)(-1);
    }

    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return (size_t)(-1);

    uint32_t stored_width = 0;
    if (pic_descriptor->haveStoredWidth())
        stored_width = pic_descriptor->getStoredWidth();

    uint8_t frame_layout = MXF_SEPARATE_FIELDS;
    if (pic_descriptor->haveFrameLayout())
        frame_layout = pic_descriptor->getFrameLayout();

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate &&
            SUPPORTED_ESSENCE[i].stored_width == stored_width &&
            SUPPORTED_ESSENCE[i].frame_layout == frame_layout)
        {
            return i;
        }
    }

    return (size_t)(-1);
}

MPEG2LGMXFDescriptorHelper::MPEG2LGMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
}

MPEG2LGMXFDescriptorHelper::~MPEG2LGMXFDescriptorHelper()
{
}

void MPEG2LGMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                            mxfUL alternative_ec_label)
{
    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mFrameWrapped = (mxf_is_mpeg_video_ec(&ec_label, 1) || mxf_is_mpeg_video_ec(&alternative_ec_label, 1));

    size_t mEssenceIndex = GetEssenceIndex(file_descriptor, alternative_ec_label);
    BMX_ASSERT(mEssenceIndex != (size_t)(-1));
    mEssenceType = SUPPORTED_ESSENCE[mEssenceIndex].essence_type;
    mAvidResolutionId = SUPPORTED_ESSENCE[mEssenceIndex].avid_resolution_id;
}

void MPEG2LGMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void MPEG2LGMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

void MPEG2LGMXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFrameWrapped(frame_wrapped);

    UpdateEssenceIndex();
}

FileDescriptor* MPEG2LGMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    // create CDCI descriptor for Avid files
    if ((mFlavour & MXFDESC_AVID_FLAVOUR))
        mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    else
        mFileDescriptor = new MPEGVideoDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void MPEG2LGMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);
    MPEGVideoDescriptor *mpeg_descriptor = dynamic_cast<MPEGVideoDescriptor*>(mFileDescriptor);
    BMX_ASSERT((mFlavour & MXFDESC_AVID_FLAVOUR) || mpeg_descriptor);

    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    SetCodingEquations(ITUR_BT709_CODING_EQ);
    if (!(mFlavour & MXFDESC_AVID_FLAVOUR)) {
        if (mEssenceType == MPEG2LG_422P_HL_720P ||
            mEssenceType == MPEG2LG_MP_HL_720P)
        {
            cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_SMPTE296M);
        }
        else
        {
            cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_SMPTE274M);
        }
    }
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    if (mEssenceType == MPEG2LG_422P_HL_1080I ||
        mEssenceType == MPEG2LG_422P_HL_1080P ||
        mEssenceType == MPEG2LG_422P_HL_720P)
    {
        // 4:2:2
        if ((mFlavour & MXFDESC_AVID_FLAVOUR))
            SetColorSiting(MXF_COLOR_SITING_REC601);
        else
            SetColorSiting(MXF_COLOR_SITING_COSITING);
    }
    else
    {
        // 4:2:0
        SetColorSiting(MXF_COLOR_SITING_VERT_MIDPOINT);
    }
    switch (mEssenceType)
    {
        case MPEG2LG_422P_HL_1080I:
        case MPEG2LG_MP_HL_1920_1080I:
            cdci_descriptor->setSampledWidth(1920);
            cdci_descriptor->setSampledHeight(540);
            cdci_descriptor->setStoredWidth(1920);
            cdci_descriptor->setStoredHeight(544);
            cdci_descriptor->setDisplayWidth(1920);
            cdci_descriptor->setDisplayHeight(540);
            cdci_descriptor->appendVideoLineMap(21);
            cdci_descriptor->appendVideoLineMap(584);
            break;
        case MPEG2LG_422P_HL_1080P:
        case MPEG2LG_MP_HL_1920_1080P:
            cdci_descriptor->setSampledWidth(1920);
            cdci_descriptor->setSampledHeight(1080);
            cdci_descriptor->setStoredWidth(1920);
            cdci_descriptor->setStoredHeight(1088);
            cdci_descriptor->setDisplayWidth(1920);
            cdci_descriptor->setDisplayHeight(1080);
            cdci_descriptor->appendVideoLineMap(42);
            cdci_descriptor->appendVideoLineMap(0);
            break;
        case MPEG2LG_422P_HL_720P:
        case MPEG2LG_MP_HL_720P:
            cdci_descriptor->setSampledWidth(1280);
            cdci_descriptor->setSampledHeight(720);
            cdci_descriptor->setStoredWidth(1280);
            cdci_descriptor->setStoredHeight(720);
            cdci_descriptor->setDisplayWidth(1280);
            cdci_descriptor->setDisplayHeight(720);
            cdci_descriptor->appendVideoLineMap(26);
            cdci_descriptor->appendVideoLineMap(0);
            break;
        case MPEG2LG_MP_HL_1440_1080I:
        case MPEG2LG_MP_H14_1080I:
            cdci_descriptor->setSampledWidth(1440);
            cdci_descriptor->setSampledHeight(540);
            cdci_descriptor->setStoredWidth(1440);
            cdci_descriptor->setStoredHeight(544);
            cdci_descriptor->setDisplayWidth(1440);
            cdci_descriptor->setDisplayHeight(540);
            cdci_descriptor->appendVideoLineMap(21);
            cdci_descriptor->appendVideoLineMap(584);
            break;
        case MPEG2LG_MP_HL_1440_1080P:
        case MPEG2LG_MP_H14_1080P:
            cdci_descriptor->setSampledWidth(1440);
            cdci_descriptor->setSampledHeight(1080);
            cdci_descriptor->setStoredWidth(1440);
            cdci_descriptor->setStoredHeight(1088);
            cdci_descriptor->setDisplayWidth(1440);
            cdci_descriptor->setDisplayHeight(1080);
            cdci_descriptor->appendVideoLineMap(42);
            cdci_descriptor->appendVideoLineMap(0);
            break;
        default:
            BMX_ASSERT(false);
            break;
    }
    if (!(mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setCaptureGamma(ITUR_BT709_TRANSFER_CH);
    cdci_descriptor->setComponentDepth(8);
    if (mEssenceType == MPEG2LG_422P_HL_1080I ||
        mEssenceType == MPEG2LG_422P_HL_1080P ||
        mEssenceType == MPEG2LG_422P_HL_720P)
    {
        // 4:2:2
        cdci_descriptor->setHorizontalSubsampling(2);
        cdci_descriptor->setVerticalSubsampling(1);
    }
    else
    {
        // 4:2:0
        cdci_descriptor->setHorizontalSubsampling(2);
        cdci_descriptor->setVerticalSubsampling(2);
    }
    cdci_descriptor->setBlackRefLevel(16);
    cdci_descriptor->setWhiteReflevel(235);
    cdci_descriptor->setColorRange(225);

    if ((mFlavour & MXFDESC_RDD9_FLAVOUR)) {
        if (SUPPORTED_ESSENCE[mEssenceIndex].frame_layout == MXF_SEPARATE_FIELDS) {
            cdci_descriptor->setStoredF2Offset(0);
            cdci_descriptor->setDisplayF2Offset(0);
            cdci_descriptor->setFieldDominance(1);
        }
        cdci_descriptor->setSampledXOffset(0);
        cdci_descriptor->setSampledYOffset(0);
        cdci_descriptor->setDisplayXOffset(0);
        cdci_descriptor->setDisplayYOffset(0);
        cdci_descriptor->setImageStartOffset(0);
        cdci_descriptor->setImageEndOffset(0);
        cdci_descriptor->setPaddingBits(0);
    } else if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        cdci_descriptor->setSampledXOffset(0);
        cdci_descriptor->setSampledYOffset(0);
        cdci_descriptor->setDisplayXOffset(0);
        cdci_descriptor->setDisplayYOffset(0);
        cdci_descriptor->setImageAlignmentOffset(1);
    }

    if (mpeg_descriptor) {
        mpeg_descriptor->setSingleSequence(true);
        mpeg_descriptor->setCodedContentType(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout == MXF_FULL_FRAME ? 1 : 2);
        mpeg_descriptor->setBPictureCount(2);
        switch (mEssenceType)
        {
            case MPEG2LG_422P_HL_1080I:
            case MPEG2LG_422P_HL_1080P:
            case MPEG2LG_422P_HL_720P:
                mpeg_descriptor->setProfileAndLevel(0x82); // 4:2:2 Profile @ High level
                break;
            case MPEG2LG_MP_HL_1920_1080I:
            case MPEG2LG_MP_HL_1920_1080P:
            case MPEG2LG_MP_HL_1440_1080I:
            case MPEG2LG_MP_HL_1440_1080P:
            case MPEG2LG_MP_HL_720P:
                mpeg_descriptor->setProfileAndLevel(0x44); // Main Profile @ High level
                break;
            case MPEG2LG_MP_H14_1080I:
            case MPEG2LG_MP_H14_1080P:
                mpeg_descriptor->setProfileAndLevel(0x46); // Main Profile @ High 1440 level
                break;
            default:
                BMX_ASSERT(false);
                break;
        }

        if ((mFlavour & MXFDESC_RDD9_FLAVOUR)) {
            mpeg_descriptor->setLowDelay(false);
            switch (mEssenceType)
            {
                case MPEG2LG_422P_HL_720P:
                case MPEG2LG_MP_HL_720P:
                    mpeg_descriptor->setMaxGOP(12);
                    break;
                case MPEG2LG_422P_HL_1080I:
                case MPEG2LG_422P_HL_1080P:
                case MPEG2LG_MP_HL_1920_1080I:
                case MPEG2LG_MP_HL_1920_1080P:
                case MPEG2LG_MP_HL_1440_1080I:
                case MPEG2LG_MP_HL_1440_1080P:
                case MPEG2LG_MP_H14_1080I:
                case MPEG2LG_MP_H14_1080P:
                default:
                    mpeg_descriptor->setMaxGOP(15);
                    break;
            }
        }
    }
}

mxfUL MPEG2LGMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        BMX_ASSERT(!mFrameWrapped);
        return MXF_EC_L(AvidMPEGClipWrapped);
    } else {
        if (mFrameWrapped)
            return MXF_EC_L(MPEGES0FrameWrapped);
        else
            return MXF_EC_L(MPEGES0ClipWrapped);
    }
}

void MPEG2LGMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate) {
            mEssenceIndex = i;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
    mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
}

