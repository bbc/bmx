/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#include <bmx/mxf_helper/UncRGBAMXFDescriptorHelper.h>
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
    mxfRational sample_rate;
    uint32_t display_width;
    uint32_t display_height;
    int32_t avid_display_y_offset;
    int32_t video_line_map[2];
    uint8_t frame_layout;
    mxfRGBALayout pixel_layout;
} SupportedEssence;

static const mxfRGBALayout ALPHA_PIXEL_LAYOUT =
    {{{'A', 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}};

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_SD,        {25, 1},        720,    576,    16, {23, 336},  MXF_MIXED_FIELDS,  ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_SD,        {30000, 1001},  720,    486,    10, {21, 283},  MXF_MIXED_FIELDS,  ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080I,  {25, 1},        1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,  ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080I,  {30000, 1001},  1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,  ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080I,  {30, 1},        1920,   1080,   0,  {21, 584},  MXF_MIXED_FIELDS,  ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080P,  {25, 1},        1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080P,  {30000, 1001},  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080P,  {30, 1},        1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080P,  {50, 1},        1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080P,  {60000, 1001},  1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_1080P,  {60, 1},        1920,   1080,   0,  {42, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_720P,   {25, 1},        1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_720P,   {30000, 1001},  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_720P,   {30, 1},        1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_720P,   {50, 1},        1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_720P,   {60000, 1001},  1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
    {MXF_EC_L(AvidUncRGBA),  AVID_ALPHA_HD_720P,   {60, 1},        1280,   720,    0,  {26, 0},    MXF_FULL_FRAME,    ALPHA_PIXEL_LAYOUT},
};



EssenceType UncRGBAMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    size_t essence_index = GetEssenceIndex(file_descriptor, alternative_ec_label);
    if (essence_index == (size_t)(-1))
        return UNKNOWN_ESSENCE_TYPE;

    return SUPPORTED_ESSENCE[essence_index].essence_type;
}

bool UncRGBAMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

size_t UncRGBAMXFDescriptorHelper::GetEssenceIndex(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    RGBAEssenceDescriptor *rgba_descriptor = dynamic_cast<RGBAEssenceDescriptor*>(file_descriptor);
    if (!rgba_descriptor || !rgba_descriptor->havePixelLayout())
        return (size_t)(-1);

    mxfRational sample_rate = normalize_rate(file_descriptor->getSampleRate());
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mxfRGBALayout pixel_layout = rgba_descriptor->getPixelLayout();

    uint32_t stored_width = 0;
    uint32_t display_width;
    if (rgba_descriptor->haveStoredWidth())
        stored_width = rgba_descriptor->getStoredWidth();
    if (rgba_descriptor->haveDisplayWidth())
        display_width = rgba_descriptor->getDisplayWidth();
    else
        display_width = stored_width;

    uint8_t frame_layout = 255;
    if (rgba_descriptor->haveFrameLayout())
        frame_layout = rgba_descriptor->getFrameLayout();

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label) &&
            sample_rate == SUPPORTED_ESSENCE[i].sample_rate &&
            display_width == SUPPORTED_ESSENCE[i].display_width &&
            frame_layout == SUPPORTED_ESSENCE[i].frame_layout &&
            mxf_equals_rgba_layout(&pixel_layout, &SUPPORTED_ESSENCE[i].pixel_layout))
        {
            return i;
        }
    }

    return (size_t)(-1);
}

UncRGBAMXFDescriptorHelper::UncRGBAMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;;
    mStoredDimensionsSet = false;
    mDisplayDimensionsSet = false;
    mSampledDimensionsSet = false;
    mVideoLineMap[0] = SUPPORTED_ESSENCE[0].video_line_map[0];
    mVideoLineMap[1] = SUPPORTED_ESSENCE[0].video_line_map[1];
    mVideoLineMapSet = false;

    SetDefaultDimensions();
}

UncRGBAMXFDescriptorHelper::~UncRGBAMXFDescriptorHelper()
{
}

void UncRGBAMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                            mxfUL alternative_ec_label)
{
    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mEssenceIndex = GetEssenceIndex(file_descriptor, alternative_ec_label);
    BMX_ASSERT(mEssenceIndex != (size_t)(-1));
    mEssenceType = SUPPORTED_ESSENCE[mEssenceIndex].essence_type;
    mFrameWrapped = false;

    RGBAEssenceDescriptor *rgba_descriptor = dynamic_cast<RGBAEssenceDescriptor*>(file_descriptor);
    BMX_ASSERT(rgba_descriptor);

    if (rgba_descriptor->haveStoredWidth()) {
        mStoredWidth = rgba_descriptor->getStoredWidth();
    } else {
        mStoredWidth = 0;
        log_warn("Uncompressed picture descriptor is missing StoredWidth\n");
    }
    if (rgba_descriptor->haveStoredHeight()) {
        mStoredHeight = rgba_descriptor->getStoredHeight();
    } else {
        mStoredHeight = 0;
        log_warn("Uncompressed picture descriptor is missing StoredHeight\n");
    }
    mStoredDimensionsSet = true;

    if (rgba_descriptor->haveDisplayWidth())
        mDisplayWidth = rgba_descriptor->getDisplayWidth();
    else
        mDisplayWidth = mStoredWidth;
    if (rgba_descriptor->haveDisplayHeight())
        mDisplayHeight = rgba_descriptor->getDisplayHeight();
    else
        mDisplayHeight = mStoredHeight;
    if (rgba_descriptor->haveDisplayXOffset())
        mDisplayXOffset = rgba_descriptor->getDisplayXOffset();
    else
        mDisplayXOffset = 0;
    if (rgba_descriptor->haveDisplayYOffset())
        mDisplayYOffset = rgba_descriptor->getDisplayYOffset();
    else
        mDisplayYOffset = 0;
    mDisplayDimensionsSet = true;

    if (rgba_descriptor->haveSampledWidth())
        mSampledWidth = rgba_descriptor->getSampledWidth();
    else
        mSampledWidth = mStoredWidth;
    if (rgba_descriptor->haveSampledHeight())
        mSampledHeight = rgba_descriptor->getSampledHeight();
    else
        mSampledHeight = mStoredHeight;
    if (rgba_descriptor->haveSampledXOffset())
        mSampledXOffset = rgba_descriptor->getSampledXOffset();
    else
        mSampledXOffset = 0;
    if (rgba_descriptor->haveSampledYOffset())
        mSampledYOffset = rgba_descriptor->getSampledYOffset();
    else
        mSampledYOffset = 0;
    mSampledDimensionsSet = true;

    mVideoLineMap[0] = 0;
    mVideoLineMap[1] = 0;
    if (rgba_descriptor->haveVideoLineMap()) {
        vector<int32_t> video_line_map = rgba_descriptor->getVideoLineMap();
        if (video_line_map.size() > 0)
            mVideoLineMap[0] = video_line_map[0];
        if (video_line_map.size() > 1)
            mVideoLineMap[1] = video_line_map[1];
    }
}

void UncRGBAMXFDescriptorHelper::SetStoredDimensions(uint32_t width, uint32_t height)
{
    mStoredWidth         = width;
    mStoredHeight        = height;
    mStoredDimensionsSet = true;
    SetDefaultDimensions();
}

void UncRGBAMXFDescriptorHelper::SetDisplayDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset)
{
    mDisplayWidth         = width;
    mDisplayHeight        = height;
    mDisplayXOffset       = x_offset;
    mDisplayYOffset       = y_offset;
    mDisplayDimensionsSet = true;
    SetDefaultDimensions();
}

void UncRGBAMXFDescriptorHelper::SetSampledDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset)
{
    mSampledWidth         = width;
    mSampledHeight        = height;
    mSampledXOffset       = x_offset;
    mSampledYOffset       = y_offset;
    mSampledDimensionsSet = true;
    SetDefaultDimensions();
}

void UncRGBAMXFDescriptorHelper::SetVideoLineMap(int32_t field1, int32_t field2)
{
    mVideoLineMap[0] = field1;
    mVideoLineMap[1] = field2;
    mVideoLineMapSet = true;
}

void UncRGBAMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

void UncRGBAMXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetSampleRate(sample_rate);

    UpdateEssenceIndex();
}

FileDescriptor* UncRGBAMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    BMX_CHECK(UpdateEssenceIndex());

    mFileDescriptor = new RGBAEssenceDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

void UncRGBAMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    RGBAEssenceDescriptor *rgba_descriptor = dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(rgba_descriptor);

    rgba_descriptor->setFrameLayout(SUPPORTED_ESSENCE[mEssenceIndex].frame_layout);
    rgba_descriptor->setStoredWidth(mStoredWidth);
    rgba_descriptor->setStoredHeight(mStoredHeight);
    rgba_descriptor->setDisplayWidth(mDisplayWidth);
    rgba_descriptor->setDisplayHeight(mDisplayHeight);
    if (mDisplayXOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        rgba_descriptor->setDisplayXOffset(mDisplayXOffset);
    if (mDisplayYOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        rgba_descriptor->setDisplayYOffset(mDisplayYOffset);
    rgba_descriptor->setSampledWidth(mSampledWidth);
    rgba_descriptor->setSampledHeight(mSampledHeight);
    if (mSampledXOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        rgba_descriptor->setSampledXOffset(mSampledXOffset);
    if (mSampledYOffset != 0 || (mFlavour & MXFDESC_AVID_FLAVOUR))
        rgba_descriptor->setSampledYOffset(mSampledYOffset);
    rgba_descriptor->appendVideoLineMap(mVideoLineMap[0]);
    rgba_descriptor->appendVideoLineMap(mVideoLineMap[1]);
    rgba_descriptor->setPixelLayout(SUPPORTED_ESSENCE[mEssenceIndex].pixel_layout);
}

uint32_t UncRGBAMXFDescriptorHelper::GetImageAlignmentOffset()
{
    if (mImageAlignmentOffsetSet)
        return mImageAlignmentOffset;
    else if ((mFlavour & MXFDESC_AVID_FLAVOUR))
        return AVID_IMAGE_ALIGNMENT;
    else
        return 1;
}

uint32_t UncRGBAMXFDescriptorHelper::GetImageEndOffset()
{
    if (mImageEndOffsetSet)
        return mImageEndOffset;
    else if (!(mFlavour & MXFDESC_AVID_FLAVOUR))
        return 0;

    uint32_t image_alignment = GetImageAlignmentOffset();
    if (image_alignment <= 1)
        return 0;
    else
        return (image_alignment - (GetSampleSize(0) % image_alignment)) % image_alignment;
}

uint32_t UncRGBAMXFDescriptorHelper::GetSampleSize()
{
    return GetSampleSize(0);
}

uint32_t UncRGBAMXFDescriptorHelper::GetSampleSize(uint32_t input_height)
{
    uint32_t pixel_bits_size = 0;
    int i;
    for (i = 0; i < 8; i++) {
        if (SUPPORTED_ESSENCE[mEssenceIndex].pixel_layout.components[i].code == 0)
            break;
        pixel_bits_size += SUPPORTED_ESSENCE[mEssenceIndex].pixel_layout.components[i].depth;
    }
    BMX_CHECK(pixel_bits_size % 8 == 0);

    uint32_t height = input_height;
    if (height == 0)
        height = mStoredHeight;

    return mStoredWidth * height * (pixel_bits_size / 8);
}

mxfUL UncRGBAMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return SUPPORTED_ESSENCE[mEssenceIndex].ec_label;
}

bool UncRGBAMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType &&
            SUPPORTED_ESSENCE[i].sample_rate == mSampleRate)
        {
            mEssenceIndex = i;
            SetDefaultDimensions();
            return true;
        }
    }

    return false;
}

void UncRGBAMXFDescriptorHelper::SetDefaultDimensions()
{
    if (!mStoredDimensionsSet) {
        mStoredWidth  = SUPPORTED_ESSENCE[mEssenceIndex].display_width;
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

