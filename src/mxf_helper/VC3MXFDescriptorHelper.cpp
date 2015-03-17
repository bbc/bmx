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

#include <cstring>

#include <bmx/mxf_helper/VC3MXFDescriptorHelper.h>
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
    int32_t resolution_id;
    uint32_t component_depth;
    uint32_t horiz_subsampling;
    uint32_t frame_size;
    uint32_t stored_width;
    uint32_t stored_height;
    uint32_t display_width;
    uint32_t display_height;
    int32_t video_line_map[2];
    uint8_t frame_layout;
    uint8_t signal_standard;
    mxfUL avid_pc_label;
    mxfUL avid_ec_label;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(VC3_1080P_1235),  VC3_1080P_1235, 1235,   10,  2,  917504,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1235ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1237),  VC3_1080P_1237, 1237,   8,   2,  606208,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1237ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1238),  VC3_1080P_1238, 1238,   8,   2,  917504,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1238ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1241),  VC3_1080I_1241, 1241,   10,  2,  917504,  1920,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1241ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1242),  VC3_1080I_1242, 1242,   8,   2,  606208,  1920,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1242ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1243),  VC3_1080I_1243, 1243,   8,   2,  917504,  1920,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1243ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1244),  VC3_1080I_1244, 1244,   8,   2,  606208,  1440,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1244ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1250),   VC3_720P_1250,  1250,   10,  2,  458752,  1280,   720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1250ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1251),   VC3_720P_1251,  1251,   8,   2,  458752,  1280,   720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1251ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1252),   VC3_720P_1252,  1252,   8,   2,  303104,  1280,   720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1252ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1253),  VC3_1080P_1253, 1253,   8,   2,  188416,  1920,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1253ClipWrapped)},
    {MXF_CMDEF_L(VC3_720P_1258),   VC3_720P_1258,  1258,   8,   2,  212992,  960,    720,    1280,   720,    {26, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE296M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD720p1258ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080P_1259),  VC3_1080P_1259, 1259,   8,   2,  417792,  1440,   1080,   1920,   1080,   {42, 0},    MXF_FULL_FRAME,       MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080p1259ClipWrapped)},
    {MXF_CMDEF_L(VC3_1080I_1260),  VC3_1080I_1260, 1260,   8,   2,  417792,  1440,   540,    1920,   540,    {21, 584},  MXF_SEPARATE_FIELDS,  MXF_SIGNAL_STANDARD_SMPTE274M,    MXF_CMDEF_L(DNxHD), MXF_EC_L(DNxHD1080i1260ClipWrapped)},
};



EssenceType VC3MXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(VC3FrameWrapped)) &&
        !CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(VC3ClipWrapped)))
    {
        size_t index;
        if (IsAvidDNxHD(file_descriptor, alternative_ec_label, &index))
            return SUPPORTED_ESSENCE[index].essence_type;

        return UNKNOWN_ESSENCE_TYPE;
    }

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool VC3MXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

bool VC3MXFDescriptorHelper::IsAvidDNxHD(FileDescriptor *file_descriptor, mxfUL alternative_ec_label, size_t *index)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&alternative_ec_label, &SUPPORTED_ESSENCE[i].avid_ec_label))
            break;
    }
    if (i >= BMX_ARRAY_SIZE(SUPPORTED_ESSENCE))
        return false;

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return false;
    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();

    bool result = mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].avid_pc_label);
    if (result)
        *index = i;

    return result;
}

VC3MXFDescriptorHelper::VC3MXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
}

VC3MXFDescriptorHelper::~VC3MXFDescriptorHelper()
{
}

void VC3MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                        mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    if (IsAvidDNxHD(file_descriptor, alternative_ec_label, &mEssenceIndex)) {
        mFrameWrapped = false;
        mEssenceType = SUPPORTED_ESSENCE[mEssenceIndex].essence_type;
        mAvidResolutionId = SUPPORTED_ESSENCE[mEssenceIndex].resolution_id;
    } else {
        mxfUL ec_label = file_descriptor->getEssenceContainer();
        mFrameWrapped = CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(VC3FrameWrapped));

        GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
        mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
        size_t i;
        for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
            if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label)) {
                mEssenceIndex = i;
                mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
                mAvidResolutionId = SUPPORTED_ESSENCE[i].resolution_id;
                break;
            }
        }
    }
}

void VC3MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == essence_type) {
            mEssenceIndex = i;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].resolution_id;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);
}

FileDescriptor* VC3MXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void VC3MXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    if ((mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].avid_pc_label);
    else
        cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    cdci_descriptor->setSignalStandard(SUPPORTED_ESSENCE[mEssenceIndex].signal_standard);
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    SetColorSiting(MXF_COLOR_SITING_REC601);
    cdci_descriptor->setComponentDepth(SUPPORTED_ESSENCE[mEssenceIndex].component_depth);
    if (SUPPORTED_ESSENCE[mEssenceIndex].component_depth == 10) {
        cdci_descriptor->setBlackRefLevel(64);
        cdci_descriptor->setWhiteReflevel(940);
        cdci_descriptor->setColorRange(897);
    } else {
        cdci_descriptor->setBlackRefLevel(16);
        cdci_descriptor->setWhiteReflevel(235);
        cdci_descriptor->setColorRange(225);
    }
    SetCodingEquations(ITUR_BT709_CODING_EQ);
    cdci_descriptor->setStoredWidth(SUPPORTED_ESSENCE[mEssenceIndex].stored_width);
    cdci_descriptor->setStoredHeight(SUPPORTED_ESSENCE[mEssenceIndex].stored_height);
    cdci_descriptor->setDisplayWidth(SUPPORTED_ESSENCE[mEssenceIndex].display_width);
    cdci_descriptor->setDisplayHeight(SUPPORTED_ESSENCE[mEssenceIndex].display_height);
    cdci_descriptor->setSampledWidth(cdci_descriptor->getDisplayWidth());
    cdci_descriptor->setSampledHeight(cdci_descriptor->getDisplayHeight());
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[0]);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[1]);
    cdci_descriptor->setHorizontalSubsampling(SUPPORTED_ESSENCE[mEssenceIndex].horiz_subsampling);
    cdci_descriptor->setVerticalSubsampling(1);
    if ((mFlavour & MXFDESC_AVID_FLAVOUR))
        cdci_descriptor->setImageAlignmentOffset(8192);
}

uint32_t VC3MXFDescriptorHelper::GetSampleSize()
{
    return SUPPORTED_ESSENCE[mEssenceIndex].frame_size;
}

mxfUL VC3MXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        BMX_ASSERT(!mFrameWrapped);
        return SUPPORTED_ESSENCE[mEssenceIndex].avid_ec_label;
    } else {
        if (mFrameWrapped)
            return MXF_EC_L(VC3FrameWrapped);
        else
            return MXF_EC_L(VC3ClipWrapped);
    }
}

