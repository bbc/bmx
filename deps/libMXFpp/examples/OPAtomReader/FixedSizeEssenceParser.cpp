/*
 * Parse raw essence data with fixed frame size
 *
 * Copyright (C) 2009, British Broadcasting Corporation
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
#include <cstdio>
#include <inttypes.h>

#include <libMXF++/MXF.h>

#include <mxf/mxf_labels_and_keys.h>
#include <mxf/mxf_avid.h>

#include "FixedSizeEssenceParser.h"

using namespace std;
using namespace mxfpp;



FixedSizeEssenceParser::FixedSizeEssenceParser(File *file, int64_t essence_length, mxfUL essence_label,
                                               const FileDescriptor *file_descriptor, uint32_t frame_size)
: RawEssenceParser(file, essence_length, essence_label)
{
    mFrameSize = frame_size;

    if (frame_size == 0 &&
        (mxf_equals_ul(&essence_label, &MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped)) ||
            mxf_equals_ul(&essence_label, &MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped)) ||
            mxf_equals_ul(&essence_label, &MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped))))
    {
        DetermineUncFrameSize(file_descriptor);
    }
    else
    {
        MXFPP_ASSERT(frame_size != 0);
    }

    DetermineDuration();
}

FixedSizeEssenceParser::~FixedSizeEssenceParser()
{
}

bool FixedSizeEssenceParser::Read(DynamicByteArray *data, uint32_t *num_samples)
{
    if (mFrameSize <= 0 || IsEOF())
        return false;

    data->allocate(mFrameSize);
    uint32_t count = mFile->read(data->getBytes(), mFrameSize);
    if (count != mFrameSize) {
        mFile->seek(-(int64_t)count, SEEK_CUR);
        mDuration = mPosition;
        return false;
    }

    data->setSize(mFrameSize);
    *num_samples = 1;

    mEssenceOffset += mFrameSize;
    mPosition++;

    return true;
}

bool FixedSizeEssenceParser::Seek(int64_t position)
{
    if (mFrameSize <= 0 || position >= mDuration)
        return false;

    mFile->seek(mEssenceStartOffset + mFrameSize * position, SEEK_SET);

    mEssenceOffset = mFrameSize * position;
    mPosition = position;

    return true;
}

int64_t FixedSizeEssenceParser::DetermineDuration()
{
    if (mDuration >= 0)
        return mDuration;

    if (mFrameSize <= 0) {
        if (!DetermineFrameSize())
            return -1;
    }

    int64_t file_size = mFile->size();

    int64_t essence_length = file_size - mEssenceStartOffset;
    if (mEssenceLength > 0 && essence_length > mEssenceLength)
        essence_length = mEssenceLength;

    mDuration = essence_length / mFrameSize;

    return mDuration;
}

bool FixedSizeEssenceParser::DetermineFrameSize()
{
    if (memcmp(&mEssenceLabel, &MXF_EC_L(AvidIMX30_625_50), sizeof(mEssenceLabel)) == 0 ||
        memcmp(&mEssenceLabel, &MXF_EC_L(AvidIMX40_625_50), sizeof(mEssenceLabel)) == 0 ||
        memcmp(&mEssenceLabel, &MXF_EC_L(AvidIMX50_625_50), sizeof(mEssenceLabel)) == 0 ||
        memcmp(&mEssenceLabel, &MXF_EC_L(AvidIMX30_525_60), sizeof(mEssenceLabel)) == 0 ||
        memcmp(&mEssenceLabel, &MXF_EC_L(AvidIMX40_525_60), sizeof(mEssenceLabel)) == 0 ||
        memcmp(&mEssenceLabel, &MXF_EC_L(AvidIMX50_525_60), sizeof(mEssenceLabel)) == 0)
    {
        return DetermineMPEGFrameSize();
    }

    return false;
}

bool FixedSizeEssenceParser::DetermineMPEGFrameSize()
{
    if (mFrameSize > 0)
        return true;

    int64_t first_frame_start = -1;
    int64_t second_frame_start = -1;
    int64_t offset = 0;
    unsigned char buffer[4096];
    size_t num_read;
    size_t i;
    int num_zeros = 0;
    bool have_start_code = 0;

    // MPEG elementary stream
    // start code prefix is 0000 0000 0000 0000 0000 0001 (2 0x00 bytes followed by 0x01)
    // sequence start code is 0xb3

    // try find the start of the first and second frame, where the difference in offsets gives the frame size

    while (second_frame_start < 0) {
        num_read = mFile->read(buffer, sizeof(buffer));

        i = 0;
        while (i < num_read) {
            if (have_start_code) {
                if (buffer[i] == 0xb3) {
                    if (first_frame_start < 0) {
                        first_frame_start = offset - 3;
                    } else {
                        second_frame_start = offset - 3;
                        break;
                    }
                }
                have_start_code = 0;
            } else if (buffer[i] == 0x00) {
                num_zeros++;
            } else {
                if (buffer[i] == 0x01 && num_zeros >= 2)
                    have_start_code = 1;
                num_zeros = 0;
            }

            i++;
            offset++;
        }

        if (num_read != sizeof(buffer))
            break;
    }


    if (second_frame_start > 0) {
        mEssenceStartOffset += first_frame_start;
        if (mEssenceLength > 0)
            mEssenceLength -= first_frame_start;
        MXFPP_CHECK(second_frame_start - first_frame_start <= UINT32_MAX);
        mFrameSize = (uint32_t)(second_frame_start - first_frame_start);
    } else if (first_frame_start > 0) {
        // assume a single frame which ends at the end of the essence data or file
        mEssenceStartOffset += first_frame_start;
        if (mEssenceLength > 0) {
            mEssenceLength -= first_frame_start;
            MXFPP_CHECK(mEssenceLength <= UINT32_MAX);
            mFrameSize = (uint32_t)mEssenceLength;
        } else {
            MXFPP_CHECK(offset - first_frame_start <= UINT32_MAX);
            mFrameSize = (uint32_t)(offset - first_frame_start);
        }
    }

    mFile->seek(mEssenceStartOffset, SEEK_SET);

    return mFrameSize > 0;
}

void FixedSizeEssenceParser::DetermineUncFrameSize(const FileDescriptor *file_descriptor)
{
    const CDCIEssenceDescriptor *cdci_descriptor =
        dynamic_cast<const CDCIEssenceDescriptor*>(file_descriptor);
    MXFPP_CHECK(cdci_descriptor);

    // Use the Avid FrameSampleSize extension item if available and non-zero
    if (cdci_descriptor->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize)))
    {
        int32_t frame_sample_size = cdci_descriptor->getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize));
        if (frame_sample_size > 0)
            mFrameSize = (uint32_t)frame_sample_size;
    }

    if (mFrameSize == 0)
    {
        uint32_t stored_width = 0, stored_height = 0;
        uint32_t h_subsamp = 0, v_subsamp = 0;
        int field_factor = 1;
        uint32_t image_start_offset = 0, image_end_offset = 0;

        // only 8-bit is currently supported
        MXFPP_CHECK(!cdci_descriptor->haveComponentDepth() || cdci_descriptor->getComponentDepth() == 8);

        if (cdci_descriptor->haveFrameLayout()) {
            uint8_t frame_layout = cdci_descriptor->getFrameLayout();
            switch (frame_layout)
            {
                case 1: // separate fields
                case 4: // segmented frame
                    // stored_height is the height of one of the 2 fields
                    field_factor = 2;
                    break;
                case 0: // full frame
                case 2: // single_field
                case 3: // mixed fields
                case 255: // distinguished value
                default:
                    field_factor = 1;
                    break;
            }
        }

        if (cdci_descriptor->haveHorizontalSubsampling())
            h_subsamp = cdci_descriptor->getHorizontalSubsampling();
        MXFPP_CHECK(h_subsamp != 0);

        if (cdci_descriptor->haveVerticalSubsampling())
            v_subsamp = cdci_descriptor->getVerticalSubsampling();
        MXFPP_CHECK(v_subsamp != 0);

        if (cdci_descriptor->haveStoredWidth())
            stored_width = cdci_descriptor->getStoredWidth();
        MXFPP_CHECK(stored_width != 0);

        if (cdci_descriptor->haveStoredHeight())
            stored_height = cdci_descriptor->getStoredHeight();
        MXFPP_CHECK(stored_height != 0);

        if (cdci_descriptor->haveImageStartOffset())
            image_start_offset = cdci_descriptor->getImageStartOffset();

        if (cdci_descriptor->haveImageEndOffset())
            image_end_offset = cdci_descriptor->getImageEndOffset();


        mFrameSize = (uint32_t)(image_start_offset + image_end_offset +
                                field_factor * stored_width * stored_height *
                                    (1 + 2.0 / (h_subsamp * v_subsamp)) + 0.5);
    }
}

