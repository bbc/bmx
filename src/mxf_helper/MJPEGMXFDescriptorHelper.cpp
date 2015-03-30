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

#include <bmx/mxf_helper/MJPEGMXFDescriptorHelper.h>
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
    int32_t avid_resolution_id;
    int32_t video_line_map[2];
    uint32_t stored_width;
    uint32_t stored_height;
    int32_t display_y_offset;
    uint8_t frame_layout;
} SupportedEssence;


static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(AvidMJPEG21_PAL),      MJPEG_2_1,     {25, 1},        g_AvidMJPEG21_ResolutionID,     {15, 328},  720, 296, 8, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG31_PAL),      MJPEG_3_1,     {25, 1},        g_AvidMJPEG31_ResolutionID,     {15, 328},  720, 296, 8, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG101_PAL),     MJPEG_10_1,    {25, 1},        g_AvidMJPEG101_ResolutionID,    {15, 328},  720, 296, 8, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG201_PAL),     MJPEG_20_1,    {25, 1},        g_AvidMJPEG201_ResolutionID,    {15, 328},  720, 296, 8, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG41m_PAL),     MJPEG_4_1M,    {25, 1},        g_AvidMJPEG41m_ResolutionID,    {15, 0},    288, 296, 8, MXF_SINGLE_FIELD},
    {MXF_CMDEF_L(AvidMJPEG101m_PAL),    MJPEG_10_1M,   {25, 1},        g_AvidMJPEG101m_ResolutionID,   {15, 0},    288, 296, 8, MXF_SINGLE_FIELD},
    {MXF_CMDEF_L(AvidMJPEG151s_PAL),    MJPEG_15_1S,   {25, 1},        g_AvidMJPEG151s_ResolutionID,   {15, 0},    352, 296, 8, MXF_SINGLE_FIELD},

    {MXF_CMDEF_L(AvidMJPEG21_NTSC),     MJPEG_2_1,     {30000, 1001},  g_AvidMJPEG21_ResolutionID,     {16, 278},  720, 248, 5, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG31_NTSC),     MJPEG_3_1,     {30000, 1001},  g_AvidMJPEG31_ResolutionID,     {16, 278},  720, 248, 5, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG101_NTSC),    MJPEG_10_1,    {30000, 1001},  g_AvidMJPEG101_ResolutionID,    {16, 278},  720, 248, 5, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG201_NTSC),    MJPEG_20_1,    {30000, 1001},  g_AvidMJPEG201_ResolutionID,    {16, 278},  720, 248, 5, MXF_SEPARATE_FIELDS},
    {MXF_CMDEF_L(AvidMJPEG41m_NTSC),    MJPEG_4_1M,    {30000, 1001},  g_AvidMJPEG41m_ResolutionID,    {16, 0},    288, 248, 5, MXF_SINGLE_FIELD},
    {MXF_CMDEF_L(AvidMJPEG101m_NTSC),   MJPEG_10_1M,   {30000, 1001},  g_AvidMJPEG101m_ResolutionID,   {16, 0},    288, 248, 5, MXF_SINGLE_FIELD},
    {MXF_CMDEF_L(AvidMJPEG151s_NTSC),   MJPEG_15_1S,   {30000, 1001},  g_AvidMJPEG151s_ResolutionID,   {16, 0},    352, 248, 5, MXF_SINGLE_FIELD},
};



EssenceType MJPEGMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!CompareECULs(ec_label, alternative_ec_label, MXF_EC_L(AvidMJPEGClipWrapped)) &&
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
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label)) {
            BMX_CHECK(SUPPORTED_ESSENCE[i].sample_rate == sample_rate);
            return SUPPORTED_ESSENCE[i].essence_type;
        }
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool MJPEGMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

MJPEGMXFDescriptorHelper::MJPEGMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
}

MJPEGMXFDescriptorHelper::~MJPEGMXFDescriptorHelper()
{
}

void MJPEGMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                          mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mFrameWrapped = false; // only clip wrapped is supported

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label)) {
            mEssenceIndex = i;
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            mAvidResolutionId = SUPPORTED_ESSENCE[i].avid_resolution_id;
            break;
        }
    }
}

void MJPEGMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void MJPEGMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

void MJPEGMXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetFrameWrapped(frame_wrapped);

    UpdateEssenceIndex();
}

FileDescriptor* MJPEGMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void MJPEGMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
    cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_ITU601);
    cdci_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    SetColorSiting(MXF_COLOR_SITING_REC601);
    cdci_descriptor->setComponentDepth(8);
    cdci_descriptor->setBlackRefLevel(16);
    cdci_descriptor->setWhiteReflevel(235);
    cdci_descriptor->setColorRange(225);
    SetCodingEquations(ITUR_BT601_CODING_EQ);
    cdci_descriptor->setStoredWidth(SUPPORTED_ESSENCE[mEssenceIndex].stored_width);
    cdci_descriptor->setStoredHeight(SUPPORTED_ESSENCE[mEssenceIndex].stored_height);
    cdci_descriptor->setDisplayWidth(cdci_descriptor->getStoredWidth());
    cdci_descriptor->setDisplayHeight(cdci_descriptor->getStoredHeight() - SUPPORTED_ESSENCE[mEssenceIndex].display_y_offset);
    cdci_descriptor->setSampledWidth(cdci_descriptor->getStoredWidth());
    cdci_descriptor->setSampledHeight(cdci_descriptor->getStoredHeight());
    cdci_descriptor->setDisplayYOffset(SUPPORTED_ESSENCE[mEssenceIndex].display_y_offset);
    cdci_descriptor->setDisplayXOffset(0);
    cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[0]);
    if (SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[1])
        cdci_descriptor->appendVideoLineMap(SUPPORTED_ESSENCE[mEssenceIndex].video_line_map[1]);
    cdci_descriptor->setHorizontalSubsampling(2);
    cdci_descriptor->setVerticalSubsampling(1);
}

bool MJPEGMXFDescriptorHelper::IsSingleField() const
{
    return (SUPPORTED_ESSENCE[mEssenceIndex].frame_layout == MXF_SINGLE_FIELD);
}

mxfUL MJPEGMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    // Only have clip wrapped label
    BMX_ASSERT(!mFrameWrapped);

    return MXF_EC_L(AvidMJPEGClipWrapped);
}

void MJPEGMXFDescriptorHelper::UpdateEssenceIndex()
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

