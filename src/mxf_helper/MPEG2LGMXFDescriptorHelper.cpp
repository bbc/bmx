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

#include <im/mxf_helper/MPEG2LGMXFDescriptorHelper.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    mxfUL pc_label;
    MXFDescriptorHelper::EssenceType essence_type;
    mxfRational sample_rate;
    int32_t avid_resolution_id;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),    MXFDescriptorHelper::MPEG2LG_422P_HL,   {25, 1},        0x0fea},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),    MXFDescriptorHelper::MPEG2LG_422P_HL,   {30000, 1001},  0x00},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),    MXFDescriptorHelper::MPEG2LG_422P_HL,   {50, 1},        0x00},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),    MXFDescriptorHelper::MPEG2LG_422P_HL,   {60000, 1001},  0x00},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),      MXFDescriptorHelper::MPEG2LG_MP_HL,     {25, 1},        0x00},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),      MXFDescriptorHelper::MPEG2LG_MP_HL,     {30000, 1001},  0x00},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),      MXFDescriptorHelper::MPEG2LG_MP_HL,     {50, 1},        0x00},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),      MXFDescriptorHelper::MPEG2LG_MP_HL,     {60000, 1001},  0x00},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),     MXFDescriptorHelper::MPEG2LG_MP_H14,    {25, 1},        0x00},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),     MXFDescriptorHelper::MPEG2LG_MP_H14,    {30000, 1001},  0x00},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),     MXFDescriptorHelper::MPEG2LG_MP_H14,    {50, 1},        0x00},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),     MXFDescriptorHelper::MPEG2LG_MP_H14,    {60000, 1001},  0x00},
};

#define SUPPORTED_ESSENCE_SIZE  (sizeof(SUPPORTED_ESSENCE) / sizeof(SupportedEssence))



MXFDescriptorHelper::EssenceType MPEG2LGMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor,
                                                                         mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(MPEGES0FrameWrapped)) &&
        !mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(MPEGES0ClipWrapped)) &&
        !(mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
            mxf_equals_ul_mod_regver(&alternative_ec_label, &MXF_EC_L(AvidMPEGClipWrapped))))
    {
        return MXFDescriptorHelper::UNKNOWN_ESSENCE;
    }

    mxfRational sample_rate = file_descriptor->getSampleRate();

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return MXFDescriptorHelper::UNKNOWN_ESSENCE;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            return SUPPORTED_ESSENCE[i].essence_type;
        }
    }

    return MXFDescriptorHelper::UNKNOWN_ESSENCE;
}

bool MPEG2LGMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

MPEG2LGMXFDescriptorHelper::MPEG2LGMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mSignalStandard = MXF_SIGNAL_STANDARD_SMPTE274M;
    mFrameLayout = MXF_SEPARATE_FIELDS;
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
}

MPEG2LGMXFDescriptorHelper::~MPEG2LGMXFDescriptorHelper()
{
}

void MPEG2LGMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    IM_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mFrameWrapped = mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(MPEGES0FrameWrapped));

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate)
        {
            mEssenceIndex = i;
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }
}

void MPEG2LGMXFDescriptorHelper::SetSignalStandard(uint8_t signal_standard)
{
    IM_CHECK(signal_standard == MXF_SIGNAL_STANDARD_SMPTE274M ||
             signal_standard == MXF_SIGNAL_STANDARD_SMPTE296M);

    mSignalStandard = signal_standard;
}

void MPEG2LGMXFDescriptorHelper::SetFrameLayout(uint8_t frame_layout)
{
    IM_CHECK(frame_layout == MXF_FULL_FRAME ||
             frame_layout == MXF_SEPARATE_FIELDS);

    mFrameLayout = frame_layout;
}

void MPEG2LGMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    IM_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void MPEG2LGMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    IM_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

void MPEG2LGMXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    IM_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFrameWrapped(frame_wrapped);

    UpdateEssenceIndex();
}

FileDescriptor* MPEG2LGMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    // create CDCI descriptor for Avid files
    if (mFlavour == AVID_FLAVOUR)
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
    IM_ASSERT(cdci_descriptor);
    MPEGVideoDescriptor *mpeg_descriptor = dynamic_cast<MPEGVideoDescriptor*>(mFileDescriptor);
    IM_ASSERT(mFlavour == AVID_FLAVOUR || mpeg_descriptor);

    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    SetCodingEquations(ITUR_BT709_CODING_EQ);
    cdci_descriptor->setSignalStandard(mSignalStandard);
    cdci_descriptor->setFrameLayout(mFrameLayout);
    if (mEssenceType == MPEG2LG_422P_HL) {
        // 4:2:2
        SetColorSiting(MXF_COLOR_SITING_COSITING);
    } else {
        // 4:2:0
        SetColorSiting(MXF_COLOR_SITING_VERT_MIDPOINT);
    }
    if (mSignalStandard == MXF_SIGNAL_STANDARD_SMPTE274M) {
        if (mFrameLayout == MXF_FULL_FRAME) {
            cdci_descriptor->setSampledWidth(mEssenceType == MPEG2LG_MP_H14 ? 1440 : 1920);
            cdci_descriptor->setSampledHeight(1080);
            cdci_descriptor->setStoredWidth(mEssenceType == MPEG2LG_MP_H14 ? 1440 : 1920);
            cdci_descriptor->setStoredHeight(1088);
            cdci_descriptor->setDisplayWidth(mEssenceType == MPEG2LG_MP_H14 ? 1440 : 1920);
            cdci_descriptor->setDisplayHeight(1080);
        } else {
            cdci_descriptor->setSampledWidth(mEssenceType == MPEG2LG_MP_H14 ? 1440 : 1920);
            cdci_descriptor->setSampledHeight(540);
            cdci_descriptor->setStoredWidth(mEssenceType == MPEG2LG_MP_H14 ? 1440 : 1920);
            cdci_descriptor->setStoredHeight(544);
            cdci_descriptor->setDisplayWidth(mEssenceType == MPEG2LG_MP_H14 ? 1440 : 1920);
            cdci_descriptor->setDisplayHeight(540);
        }

        if (mFrameLayout == MXF_FULL_FRAME) {
            cdci_descriptor->appendVideoLineMap(42);
            cdci_descriptor->appendVideoLineMap(0);
        } else {
            cdci_descriptor->appendVideoLineMap(21);
            cdci_descriptor->appendVideoLineMap(584);
        }
    } else { // MXF_SIGNAL_STANDARD_SMPTE296M
        IM_CHECK(mFrameLayout == MXF_FULL_FRAME);
        IM_CHECK(mEssenceType != MPEG2LG_MP_H14);
        cdci_descriptor->setSampledWidth(1280);
        cdci_descriptor->setSampledHeight(720);
        cdci_descriptor->setStoredWidth(1280);
        cdci_descriptor->setStoredHeight(720);
        cdci_descriptor->setDisplayWidth(1280);
        cdci_descriptor->setDisplayHeight(720);
        cdci_descriptor->appendVideoLineMap(26);
        cdci_descriptor->appendVideoLineMap(0);
    }

    cdci_descriptor->setCaptureGamma(ITUR_BT709_TRANSFER_CH);
    cdci_descriptor->setComponentDepth(8);
    if (mEssenceType == MPEG2LG_422P_HL) {
        // 4:2:2
        cdci_descriptor->setHorizontalSubsampling(2);
        cdci_descriptor->setVerticalSubsampling(1);
    } else {
        // 4:2:0
        cdci_descriptor->setHorizontalSubsampling(2);
        cdci_descriptor->setVerticalSubsampling(2);
    }
    cdci_descriptor->setBlackRefLevel(16);
    cdci_descriptor->setWhiteReflevel(235);
    cdci_descriptor->setColorRange(225);

    if (mpeg_descriptor) {
        mpeg_descriptor->setSingleSequence(true);
        mpeg_descriptor->setCodedContentType(mFrameLayout == MXF_FULL_FRAME ? 1 : 2);
        mpeg_descriptor->setBPictureCount(2);
        switch (mEssenceType)
        {
            case MPEG2LG_422P_HL:
                mpeg_descriptor->setProfileAndLevel(0x82); // 4:2:2 Profile @ High level
                break;
            case MPEG2LG_MP_HL:
                mpeg_descriptor->setProfileAndLevel(0x44); // Main Profile @ High level
                break;
            case MPEG2LG_MP_H14:
                mpeg_descriptor->setProfileAndLevel(0x46); // Main Profile @ High 1440 level
                break;
            default:
                IM_ASSERT(false);
                break;
        }
    }
}

mxfUL MPEG2LGMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if (mFlavour == AVID_FLAVOUR) {
        IM_ASSERT(!mFrameWrapped);
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
    for (i = 0; i < SUPPORTED_ESSENCE_SIZE; i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate)
        {
            mEssenceIndex = i;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }
    IM_CHECK(i < SUPPORTED_ESSENCE_SIZE);
}

