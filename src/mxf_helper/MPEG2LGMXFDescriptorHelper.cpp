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
    uint8_t signal_standard;
    uint8_t frame_layout;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080I,  MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_1080P,  MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  MPEG2LG_422P_HL_720P,   MXF_SIGNAL_STANDARD_SMPTE296M,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1080I,    MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_1080P,    MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    MPEG2LG_MP_HL_720P,     MXF_SIGNAL_STANDARD_SMPTE296M,  MXF_FULL_FRAME},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080I,   MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   MPEG2LG_MP_H14_1080P,   MXF_SIGNAL_STANDARD_SMPTE274M,  MXF_FULL_FRAME},
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
    for (i = 0; i < ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

size_t MPEG2LGMXFDescriptorHelper::GetEssenceIndex(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(MPEGES0FrameWrapped)) &&
        !mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(MPEGES0ClipWrapped)) &&
        !(mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
            mxf_equals_ul_mod_regver(&alternative_ec_label, &MXF_EC_L(AvidMPEGClipWrapped))))
    {
        return (size_t)(-1);
    }

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return (size_t)(-1);

    uint32_t stored_width = 0;
    if (pic_descriptor->haveStoredWidth())
        stored_width = pic_descriptor->getStoredWidth();

    uint8_t signal_standard = 255;
    uint8_t frame_layout = 255;
    if (pic_descriptor->haveSignalStandard()) {
        signal_standard = pic_descriptor->getSignalStandard();
    } else {
        if (stored_width == 1920 || stored_width == 1440)
            signal_standard = MXF_SIGNAL_STANDARD_SMPTE274M;
        else if (stored_width == 1280)
            signal_standard = MXF_SIGNAL_STANDARD_SMPTE296M;
    }
    if (pic_descriptor->haveFrameLayout())
        frame_layout = pic_descriptor->getFrameLayout();
    else
        frame_layout = MXF_SEPARATE_FIELDS;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            SUPPORTED_ESSENCE[i].signal_standard == signal_standard &&
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

void MPEG2LGMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    PictureMXFDescriptorHelper::Initialize(file_descriptor, alternative_ec_label);

    mxfRational sample_rate = file_descriptor->getSampleRate();

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mFrameWrapped = mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(MPEGES0FrameWrapped));

    size_t mEssenceIndex = GetEssenceIndex(file_descriptor, alternative_ec_label);
    BMX_ASSERT(mEssenceIndex != (size_t)(-1));
    mEssenceType = SUPPORTED_ESSENCE[mEssenceIndex].essence_type;

    // TODO: get other avid resolution ids
    if (mEssenceType == MPEG2LG_422P_HL_1080I) {
        if (sample_rate == FRAME_RATE_25)
            mAvidResolutionId = 0x0fea;
        else if (sample_rate == FRAME_RATE_2997)
            mAvidResolutionId = 0x0fe9;
    }
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
    BMX_ASSERT(cdci_descriptor);
    MPEGVideoDescriptor *mpeg_descriptor = dynamic_cast<MPEGVideoDescriptor*>(mFileDescriptor);
    BMX_ASSERT(mFlavour == AVID_FLAVOUR || mpeg_descriptor);

    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    SetCodingEquations(ITUR_BT709_CODING_EQ);
    cdci_descriptor->setSignalStandard(SUPPORTED_ESSENCE[mEssenceIndex].signal_standard);
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    if (mEssenceType == MPEG2LG_422P_HL_1080I ||
        mEssenceType == MPEG2LG_422P_HL_1080P ||
        mEssenceType == MPEG2LG_422P_HL_720P)
    {
        // 4:2:2
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
        case MPEG2LG_MP_HL_1080I:
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
        case MPEG2LG_MP_HL_1080P:
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
            case MPEG2LG_MP_HL_1080I:
            case MPEG2LG_MP_HL_1080P:
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
    }
}

mxfUL MPEG2LGMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if (mFlavour == AVID_FLAVOUR) {
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
    for (i = 0; i < ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType) {
            mEssenceIndex = i;
            break;
        }
    }
    BMX_CHECK(i < ARRAY_SIZE(SUPPORTED_ESSENCE));

    // TODO: get other avid resolution ids
    if (mEssenceType == MPEG2LG_422P_HL_1080I) {
        if (mSampleRate == FRAME_RATE_25)
            mAvidResolutionId = 0x0fea;
        else if (mSampleRate == FRAME_RATE_2997)
            mAvidResolutionId = 0x0fe9;
    }
}

