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

#include <bmx/mxf_helper/AVCIMXFDescriptorHelper.h>
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
    uint32_t frame_size; // includes size of sequence and picture parameter sets (512 bytes)
    int32_t avid_resolution_id;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(AVCI_100_1080_50_I),   AVCI100_1080I,   {25, 1},        568832,  0x0d58},
    {MXF_CMDEF_L(AVCI_100_1080_60_I),   AVCI100_1080I,   {30000, 1001},  472576,  0x0d57},
    {MXF_CMDEF_L(AVCI_100_1080_25_P),   AVCI100_1080P,   {50, 1},        568832,       0},
    {MXF_CMDEF_L(AVCI_100_1080_25_P),   AVCI100_1080P,   {25, 1},        568832,  0x0d59},
    {MXF_CMDEF_L(AVCI_100_1080_30_P),   AVCI100_1080P,   {60000, 1001},  472576,       0},
    {MXF_CMDEF_L(AVCI_100_1080_30_P),   AVCI100_1080P,   {30000, 1001},  472576,  0x0d5a},
    {MXF_CMDEF_L(AVCI_100_1080_30_P),   AVCI100_1080P,   {24000, 1001},  472576,  0x0d5b},
    {MXF_CMDEF_L(AVCI_100_720_50_P),    AVCI100_720P,    {50, 1},        284672,  0x0d52},
    {MXF_CMDEF_L(AVCI_100_720_50_P),    AVCI100_720P,    {25, 1},        284672,  0x0d54},
    {MXF_CMDEF_L(AVCI_100_720_60_P),    AVCI100_720P,    {60000, 1001},  236544,  0x0d56},
    {MXF_CMDEF_L(AVCI_100_720_60_P),    AVCI100_720P,    {30000, 1001},  236544,  0x0d53},
    {MXF_CMDEF_L(AVCI_100_720_60_P),    AVCI100_720P,    {24000, 1001},  236544,  0x0d55},
    {MXF_CMDEF_L(AVCI_50_1080_50_I),    AVCI50_1080I,    {25, 1},        281088,  0x0d4e},
    {MXF_CMDEF_L(AVCI_50_1080_60_I),    AVCI50_1080I,    {30000, 1001},  232960,  0x0d4d},
    {MXF_CMDEF_L(AVCI_50_1080_25_P),    AVCI50_1080P,    {50, 1},        281088,       0},
    {MXF_CMDEF_L(AVCI_50_1080_25_P),    AVCI50_1080P,    {25, 1},        281088,  0x0d4f},
    {MXF_CMDEF_L(AVCI_50_1080_30_P),    AVCI50_1080P,    {60000, 1001},  232960,       0},
    {MXF_CMDEF_L(AVCI_50_1080_30_P),    AVCI50_1080P,    {30000, 1001},  232960,  0x0d50},
    {MXF_CMDEF_L(AVCI_50_1080_30_P),    AVCI50_1080P,    {24000, 1001},  232960,  0x0d51},
    {MXF_CMDEF_L(AVCI_50_720_50_P),     AVCI50_720P,     {50, 1},        140800,  0x0d48},
    {MXF_CMDEF_L(AVCI_50_720_50_P),     AVCI50_720P,     {25, 1},        140800,  0x0d4a},
    {MXF_CMDEF_L(AVCI_50_720_60_P),     AVCI50_720P,     {60000, 1001},  116736,  0x0d4c},
    {MXF_CMDEF_L(AVCI_50_720_60_P),     AVCI50_720P,     {30000, 1001},  116736,  0x0d49},
    {MXF_CMDEF_L(AVCI_50_720_60_P),     AVCI50_720P,     {24000, 1001},  116736,  0x0d4b},
    {MXF_CMDEF_L(AVCI_200_1080_50_I),   AVCI200_1080I,   {25, 1},        1144320,      0},
    {MXF_CMDEF_L(AVCI_200_1080_60_I),   AVCI200_1080I,   {30000, 1001},  952832,       0},
    {MXF_CMDEF_L(AVCI_200_1080_25_P),   AVCI200_1080P,   {50, 1},        1144320,      0},
    {MXF_CMDEF_L(AVCI_200_1080_25_P),   AVCI200_1080P,   {25, 1},        1144320,      0},
    {MXF_CMDEF_L(AVCI_200_1080_30_P),   AVCI200_1080P,   {60000, 1001},  952832,       0},
    {MXF_CMDEF_L(AVCI_200_1080_30_P),   AVCI200_1080P,   {30000, 1001},  952832,       0},
    {MXF_CMDEF_L(AVCI_200_1080_30_P),   AVCI200_1080P,   {24000, 1001},  952832,       0},
    {MXF_CMDEF_L(AVCI_200_720_50_P),    AVCI200_720P,    {50, 1},        572416,       0},
    {MXF_CMDEF_L(AVCI_200_720_50_P),    AVCI200_720P,    {25, 1},        572416,       0},
    {MXF_CMDEF_L(AVCI_200_720_60_P),    AVCI200_720P,    {60000, 1001},  476672,       0},
    {MXF_CMDEF_L(AVCI_200_720_60_P),    AVCI200_720P,    {30000, 1001},  476672,       0},
    {MXF_CMDEF_L(AVCI_200_720_60_P),    AVCI200_720P,    {24000, 1001},  476672,       0},
};

typedef struct
{
    EssenceType essence_type;
    mxfRational sample_rate;
    uint8_t coded_content_kind;
    uint32_t bit_rate;
    uint8_t profile;
    uint8_t profile_constraint;
    uint8_t level;
} AVCDescriptorInfo;

static const AVCDescriptorInfo AVC_DESCRIPTOR_INFO[] =
{
    {AVCI100_1080I,   {25, 1},        MXF_AVC_INTERLACED_FRAME_PICTURE,   111820800,  122, 0x10, 41},
    {AVCI100_1080I,   {30000, 1001},  MXF_AVC_INTERLACED_FRAME_PICTURE,   110972544,  122, 0x10, 41},
    {AVCI100_1080P,   {50, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  223641600,  122, 0x10, 42},
    {AVCI100_1080P,   {25, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  111820800,  122, 0x10, 41},
    {AVCI100_1080P,   {60000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  221945088,  122, 0x10, 42},
    {AVCI100_1080P,   {30000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  110972544,  122, 0x10, 41},
    {AVCI100_1080P,   {24000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   88777984,  122, 0x10, 41},
    {AVCI100_720P,    {50, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  111616000,  122, 0x10, 41},
    {AVCI100_720P,    {25, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   55808000,  122, 0x10, 41},
    {AVCI100_720P,    {60000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  110726976,  122, 0x10, 41},
    {AVCI100_720P,    {30000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   55363456,  122, 0x10, 41},
    {AVCI100_720P,    {24000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   44290752,  122, 0x10, 41},
    {AVCI50_1080I,    {25, 1},        MXF_AVC_INTERLACED_FRAME_PICTURE,    54272000,  110, 0x10, 40},
    {AVCI50_1080I,    {30000, 1001},  MXF_AVC_INTERLACED_FRAME_PICTURE,    53522112,  110, 0x10, 40},
    {AVCI50_1080P,    {50, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  108544000,  110, 0x10, 42},
    {AVCI50_1080P,    {25, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   54272000,  110, 0x10, 40},
    {AVCI50_1080P,    {60000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  107044288,  110, 0x10, 42},
    {AVCI50_1080P,    {30000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   53522112,  110, 0x10, 40},
    {AVCI50_1080P,    {24000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   42817664,  110, 0x10, 40},
    {AVCI50_720P,     {50, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   54067200,  110, 0x10, 32},
    {AVCI50_720P,     {25, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   27033600,  110, 0x10, 32},
    {AVCI50_720P,     {60000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   53276608,  110, 0x10, 32},
    {AVCI50_720P,     {30000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   26638272,  110, 0x10, 32},
    {AVCI50_720P,     {24000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   21310656,  110, 0x10, 32},
    {AVCI200_1080I,   {25, 1},        MXF_AVC_INTERLACED_FRAME_PICTURE,   226918400,  122, 0x10, 41},
    {AVCI200_1080I,   {30000, 1001},  MXF_AVC_INTERLACED_FRAME_PICTURE,   226118784,  122, 0x10, 41},
    {AVCI200_1080P,   {50, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  453836800,  122, 0x10, 50},
    {AVCI200_1080P,   {25, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  226918400,  122, 0x10, 41},
    {AVCI200_1080P,   {60000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  452237632,  122, 0x10, 50},
    {AVCI200_1080P,   {30000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  226118784,  122, 0x10, 41},
    {AVCI200_1080P,   {24000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  180895040,  122, 0x10, 41},
    {AVCI200_720P,    {50, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  226713600,  122, 0x10, 41},
    {AVCI200_720P,    {25, 1},        MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  113356800,  122, 0x10, 41},
    {AVCI200_720P,    {60000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  225873280,  122, 0x10, 41},
    {AVCI200_720P,    {30000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,  112936640,  122, 0x10, 41},
    {AVCI200_720P,    {24000, 1001},  MXF_AVC_PROGRESSIVE_FRAME_PICTURE,   90349312,  122, 0x10, 41},
};


EssenceType AVCIMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_is_avc_ec(&ec_label, 0) &&
        !mxf_is_avc_ec(&ec_label, 1) &&
        !mxf_is_avc_ec(&alternative_ec_label, 0) &&
        !mxf_is_avc_ec(&alternative_ec_label, 1) &&
        !IsNullAvidECUL(ec_label, alternative_ec_label))
    {
        return UNKNOWN_ESSENCE_TYPE;
    }

    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            return SUPPORTED_ESSENCE[i].essence_type;
        }
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool AVCIMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

uint32_t AVCIMXFDescriptorHelper::GetSampleSize(EssenceType essence_type, mxfRational sample_rate)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type &&
            sample_rate  == SUPPORTED_ESSENCE[i].sample_rate)
        {
            return SUPPORTED_ESSENCE[i].frame_size;
        }
    }

    return 0;
}

AVCIMXFDescriptorHelper::AVCIMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
    mIncludeHeader = false;
    mIncludeHeaderSet = false;
    mAVCSubDescriptor = 0;
}

AVCIMXFDescriptorHelper::~AVCIMXFDescriptorHelper()
{
}

void AVCIMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                         mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mFrameWrapped = (mxf_is_avc_ec(&ec_label, 1) || mxf_is_avc_ec(&alternative_ec_label, 1));

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            mEssenceIndex = i;
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }

    if (file_descriptor->haveSubDescriptors()) {
        vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
        for (i = 0; i < sub_descriptors.size(); i++) {
            mAVCSubDescriptor = dynamic_cast<AVCSubDescriptor*>(sub_descriptors[i]);
            if (mAVCSubDescriptor)
                break;
        }
    }
}

void AVCIMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void AVCIMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

FileDescriptor* AVCIMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    if ((mFlavour & MXFDESC_AVID_FLAVOUR) || (mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR)) {
        mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
        if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR)) {
            mAVCSubDescriptor = new AVCSubDescriptor(header_metadata);
            mFileDescriptor->appendSubDescriptors(mAVCSubDescriptor);
        }
    } else {
        mFileDescriptor = new MPEGVideoDescriptor(header_metadata);
    }
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void AVCIMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    // override the default Avid frame sample size set in PictureMXFDescriptorHelper
    if ((mFlavour & MXFDESC_AVID_FLAVOUR) && mIncludeHeaderSet) {
        SetAvidFrameSampleSize(mIncludeHeader ? GetSampleSize() :
                                                GetSampleWithoutHeaderSize());
    }

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);
    MPEGVideoDescriptor *mpeg_descriptor = dynamic_cast<MPEGVideoDescriptor*>(mFileDescriptor);
    BMX_ASSERT((mFlavour & MXFDESC_AVID_FLAVOUR) || (mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR) || mpeg_descriptor);

    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    switch (mEssenceType)
    {
        case AVCI200_1080I:
        case AVCI200_1080P:
        case AVCI100_1080I:
        case AVCI100_1080P:
        case AVCI50_1080I:
        case AVCI50_1080P:
            cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_SMPTE274M);
            break;
        case AVCI200_720P:
        case AVCI100_720P:
        case AVCI50_720P:
            cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_SMPTE296M);
            break;
        default:
            BMX_ASSERT(false);
    }
    switch (mEssenceType)
    {
        case AVCI200_1080I:
        case AVCI100_1080I:
        case AVCI50_1080I:
            if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR)) {
                cdci_descriptor->setStoredF2Offset(0);
                cdci_descriptor->setDisplayF2Offset(0);
                cdci_descriptor->setFieldDominance(1);
            }
            cdci_descriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
            break;
        case AVCI200_1080P:
        case AVCI200_720P:
        case AVCI100_1080P:
        case AVCI100_720P:
        case AVCI50_1080P:
        case AVCI50_720P:
            cdci_descriptor->setFrameLayout(MXF_FULL_FRAME);
            break;
        default:
            BMX_ASSERT(false);
    }
    SetColorSiting(0x00); // coSiting
    cdci_descriptor->setComponentDepth(10);
    cdci_descriptor->setBlackRefLevel(64);
    cdci_descriptor->setWhiteReflevel(940);
    cdci_descriptor->setColorRange(897);
    if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        cdci_descriptor->setCaptureGamma(ITUR_BT709_TRANSFER_CH);
    else
        SetCodingEquations(ITUR_BT709_CODING_EQ);
    switch (mEssenceType)
    {
        case AVCI200_1080I:
        case AVCI100_1080I:
        case AVCI50_1080I:
            cdci_descriptor->setStoredWidth(1920);
            if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR))
                cdci_descriptor->setStoredHeight(544);
            else
                cdci_descriptor->setStoredHeight(540);
            cdci_descriptor->appendVideoLineMap(21);
            cdci_descriptor->appendVideoLineMap(584);
            break;
        case AVCI200_1080P:
        case AVCI100_1080P:
        case AVCI50_1080P:
            cdci_descriptor->setStoredWidth(1920);
            if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR))
                cdci_descriptor->setStoredHeight(1088);
            else
                cdci_descriptor->setStoredHeight(1080);
            cdci_descriptor->appendVideoLineMap(42);
            cdci_descriptor->appendVideoLineMap(0);
            break;
        case AVCI200_720P:
        case AVCI100_720P:
        case AVCI50_720P:
            cdci_descriptor->setStoredWidth(1280);
            cdci_descriptor->setStoredHeight(720);
            cdci_descriptor->appendVideoLineMap(26);
            cdci_descriptor->appendVideoLineMap(0);
            break;
        default:
            BMX_ASSERT(false);
    }
    switch (mEssenceType)
    {
        case AVCI200_1080I:
        case AVCI100_1080I:
        case AVCI50_1080I:
            cdci_descriptor->setDisplayWidth(cdci_descriptor->getStoredWidth());
            cdci_descriptor->setDisplayHeight(540);
            cdci_descriptor->setSampledWidth(cdci_descriptor->getStoredWidth());
            cdci_descriptor->setSampledHeight(540);
            break;
        case AVCI200_1080P:
        case AVCI100_1080P:
        case AVCI50_1080P:
            cdci_descriptor->setDisplayWidth(cdci_descriptor->getStoredWidth());
            cdci_descriptor->setDisplayHeight(1080);
            cdci_descriptor->setSampledWidth(cdci_descriptor->getStoredWidth());
            cdci_descriptor->setSampledHeight(1080);
            break;
        case AVCI200_720P:
        case AVCI100_720P:
        case AVCI50_720P:
            cdci_descriptor->setDisplayWidth(cdci_descriptor->getStoredWidth());
            cdci_descriptor->setDisplayHeight(cdci_descriptor->getStoredHeight());
            cdci_descriptor->setSampledWidth(cdci_descriptor->getStoredWidth());
            cdci_descriptor->setSampledHeight(cdci_descriptor->getStoredHeight());
            break;
        default:
            BMX_ASSERT(false);
    }
    if ((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR)) {
        cdci_descriptor->setSampledXOffset(0);
        cdci_descriptor->setSampledYOffset(0);
        cdci_descriptor->setDisplayXOffset(0);
        cdci_descriptor->setDisplayYOffset(0);
        cdci_descriptor->setImageStartOffset(0);
        cdci_descriptor->setPaddingBits(0);
    }
    switch (mEssenceType)
    {
        case AVCI200_1080I:
        case AVCI200_1080P:
        case AVCI200_720P:
        case AVCI100_1080I:
        case AVCI100_1080P:
        case AVCI100_720P:
            cdci_descriptor->setHorizontalSubsampling(2);
            cdci_descriptor->setVerticalSubsampling(1);
            break;
        case AVCI50_1080I:
        case AVCI50_1080P:
        case AVCI50_720P:
            cdci_descriptor->setHorizontalSubsampling(2);
            cdci_descriptor->setVerticalSubsampling(2);
            break;
        default:
            BMX_ASSERT(false);
    }

    if (mAVCSubDescriptor) {
        BMX_ASSERT(BMX_ARRAY_SIZE(SUPPORTED_ESSENCE) == BMX_ARRAY_SIZE(AVC_DESCRIPTOR_INFO));
        BMX_ASSERT(mEssenceIndex < BMX_ARRAY_SIZE(AVC_DESCRIPTOR_INFO));
        const AVCDescriptorInfo *avc_info = &AVC_DESCRIPTOR_INFO[mEssenceIndex];

        // TODO: should be extracting and checking this information from the bitstream
        mAVCSubDescriptor->setAVCDecodingDelay(0);
        mAVCSubDescriptor->setAVCCodedContentKind(avc_info->coded_content_kind);
        mAVCSubDescriptor->setAVCIdenticalGOPIndicator(true);
        mAVCSubDescriptor->setAVCMaximumGOPSize(1);
        mAVCSubDescriptor->setAVCMaximumBPictureCount(0);
        mAVCSubDescriptor->setAVCMaximumBitrate(avc_info->bit_rate);
        mAVCSubDescriptor->setAVCAverageBitrate(avc_info->bit_rate);
        mAVCSubDescriptor->setAVCProfile(avc_info->profile);
        mAVCSubDescriptor->setAVCProfileConstraint(avc_info->profile_constraint);
        mAVCSubDescriptor->setAVCLevel(avc_info->level);

        BMX_ASSERT((mFlavour & MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR));
        mAVCSubDescriptor->setAVCSequenceParameterSetFlag(0xa0);    // constant and present in every access unit
        mAVCSubDescriptor->setAVCPictureParameterSetFlag(0xa0);     // constant and present in every access unit
    }
}

uint32_t AVCIMXFDescriptorHelper::GetSampleSize()
{
    return SUPPORTED_ESSENCE[mEssenceIndex].frame_size;
}

uint32_t AVCIMXFDescriptorHelper::GetSampleWithoutHeaderSize()
{
    return SUPPORTED_ESSENCE[mEssenceIndex].frame_size - 512;
}

void AVCIMXFDescriptorHelper::SetIncludeHeader(bool include_header)
{
    mIncludeHeader = include_header;
    mIncludeHeaderSet = true;
}

mxfUL AVCIMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if (mFrameWrapped)
        return MXF_EC_L(AVCIFrameWrapped);
    else
        return MXF_EC_L(AVCIClipWrapped);
}

void AVCIMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate)
        {
            mEssenceIndex = i;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
}

