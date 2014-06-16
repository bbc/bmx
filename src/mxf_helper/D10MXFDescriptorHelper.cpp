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

#include <bmx/mxf_helper/D10MXFDescriptorHelper.h>
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
    uint32_t stored_width;
    uint32_t stored_height;
    int32_t display_y_offset;
    int32_t video_line_map[2];
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(D10_30_625_50),    MXF_EC_L(D10_30_625_50_picture_only),      D10_30,   {25, 1},         true,    0x00,   150000, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_30_625_50),    MXF_EC_L(D10_30_625_50_defined_template),  D10_30,   {25, 1},         true,    0x00,   150000, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_30_625_50),    MXF_EC_L(AvidIMX30_625_50),                D10_30,   {25, 1},         false,   0xa2,   151552, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_30_525_60),    MXF_EC_L(D10_30_525_60_picture_only),      D10_30,   {30000, 1001},   true,    0x00,   125125, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_30_525_60),    MXF_EC_L(D10_30_525_60_defined_template),  D10_30,   {30000, 1001},   true,    0x00,   125125, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_30_525_60),    MXF_EC_L(AvidIMX30_525_60),                D10_30,   {30000, 1001},   false,   0xa2,   126976, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_40_625_50),    MXF_EC_L(D10_40_625_50_picture_only),      D10_40,   {25, 1},         true,    0x00,   200000, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_40_625_50),    MXF_EC_L(D10_40_625_50_defined_template),  D10_40,   {25, 1},         true,    0x00,   200000, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_40_625_50),    MXF_EC_L(AvidIMX40_625_50),                D10_40,   {25, 1},         false,   0xa1,   200704, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_40_525_60),    MXF_EC_L(D10_40_525_60_picture_only),      D10_40,   {30000, 1001},   true,    0x00,   166833, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_40_525_60),    MXF_EC_L(D10_40_525_60_defined_template),  D10_40,   {30000, 1001},   true,    0x00,   166833, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_40_525_60),    MXF_EC_L(AvidIMX40_525_60),                D10_40,   {30000, 1001},   false,   0xa1,   167936, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_50_625_50),    MXF_EC_L(D10_50_625_50_picture_only),      D10_50,   {25, 1},         true,    0x00,   250000, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_50_625_50),    MXF_EC_L(D10_50_625_50_defined_template),  D10_50,   {25, 1},         true,    0x00,   250000, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_50_625_50),    MXF_EC_L(AvidIMX50_625_50),                D10_50,   {25, 1},         false,   0xa0,   253952, 720,    304,    16, {7, 320}},
    {MXF_CMDEF_L(D10_50_525_60),    MXF_EC_L(D10_50_525_60_picture_only),      D10_50,   {30000, 1001},   true,    0x00,   208541, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_50_525_60),    MXF_EC_L(D10_50_525_60_defined_template),  D10_50,   {30000, 1001},   true,    0x00,   208541, 720,    256,    13, {7, 270}},
    {MXF_CMDEF_L(D10_50_525_60),    MXF_EC_L(AvidIMX50_525_60),                D10_50,   {30000, 1001},   false,   0xa0,   208896, 720,    256,    13, {7, 270}},
};



EssenceType D10MXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label) ||
                IsNullAvidECUL(ec_label, alternative_ec_label)) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            return SUPPORTED_ESSENCE[i].essence_type;
        }
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool D10MXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

D10MXFDescriptorHelper::D10MXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;;
    mSampleSize = 0;
}

D10MXFDescriptorHelper::~D10MXFDescriptorHelper()
{
}

void D10MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
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
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label) &&
            (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label) ||
                IsNullAvidECUL(ec_label, alternative_ec_label)) &&
            SUPPORTED_ESSENCE[i].sample_rate == sample_rate)
        {
            mEssenceIndex = i;
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mFrameWrapped = SUPPORTED_ESSENCE[i].frame_wrapped;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }

    mSampleSize = 0;
}

void D10MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void D10MXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

void D10MXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFrameWrapped(frame_wrapped);

    UpdateEssenceIndex();
}

void D10MXFDescriptorHelper::SetFlavour(int flavour)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFlavour(flavour);

    UpdateEssenceIndex();
}

void D10MXFDescriptorHelper::SetSampleSize(uint32_t size)
{
    BMX_ASSERT(!mFileDescriptor);

    mSampleSize = size;
}

FileDescriptor* D10MXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void D10MXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_ITU601);
    cdci_descriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
    cdci_descriptor->setStoredWidth(SUPPORTED_ESSENCE[mEssenceIndex].stored_width);
    cdci_descriptor->setStoredHeight(SUPPORTED_ESSENCE[mEssenceIndex].stored_height);
    cdci_descriptor->setSampledWidth(cdci_descriptor->getStoredWidth());
    cdci_descriptor->setSampledHeight(cdci_descriptor->getStoredHeight());
    cdci_descriptor->setDisplayWidth(cdci_descriptor->getStoredWidth());
    cdci_descriptor->setDisplayHeight(cdci_descriptor->getStoredHeight() - SUPPORTED_ESSENCE[mEssenceIndex].display_y_offset);
    cdci_descriptor->setDisplayXOffset(0);
    cdci_descriptor->setDisplayYOffset(SUPPORTED_ESSENCE[mEssenceIndex].display_y_offset);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[0]);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[1]);
    cdci_descriptor->setStoredF2Offset(0);
    cdci_descriptor->setSampledXOffset(0);
    cdci_descriptor->setSampledYOffset(0);
    cdci_descriptor->setDisplayF2Offset(0);
    cdci_descriptor->setAlphaTransparency(false);
    cdci_descriptor->setCaptureGamma(ITUR_BT470_TRANSFER_CH);
    cdci_descriptor->setImageAlignmentOffset(0);
    cdci_descriptor->setFieldDominance(1); // field 1
    cdci_descriptor->setImageStartOffset(0);
    cdci_descriptor->setImageEndOffset(0);
    cdci_descriptor->setComponentDepth(8);
    cdci_descriptor->setHorizontalSubsampling(2);
    cdci_descriptor->setVerticalSubsampling(1);
    SetColorSiting(MXF_COLOR_SITING_REC601);
    cdci_descriptor->setReversedByteOrder(false);
    cdci_descriptor->setPaddingBits(0);
    cdci_descriptor->setBlackRefLevel(16);
    cdci_descriptor->setWhiteReflevel(235);
    cdci_descriptor->setColorRange(225);
    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
}

uint32_t D10MXFDescriptorHelper::GetSampleSize()
{
    if (mSampleSize != 0)
        return mSampleSize;

    return SUPPORTED_ESSENCE[mEssenceIndex].sample_size;
}

uint32_t D10MXFDescriptorHelper::GetMaxSampleSize()
{
    return SUPPORTED_ESSENCE[mEssenceIndex].sample_size;
}

mxfUL D10MXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return SUPPORTED_ESSENCE[mEssenceIndex].ec_label;
}

void D10MXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate &&
            SUPPORTED_ESSENCE[i].frame_wrapped == mFrameWrapped &&
            (SUPPORTED_ESSENCE[i].avid_resolution_id != 0 || !(mFlavour & MXFDESC_AVID_FLAVOUR)))
        {
            mEssenceIndex = i;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
}

