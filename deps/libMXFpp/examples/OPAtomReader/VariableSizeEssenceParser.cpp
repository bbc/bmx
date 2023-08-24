/*
 * Parse raw essence data with variable frame size
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

#include <cstdio>

#include <libMXF++/MXF.h>

#include <mxf/mxf_labels_and_keys.h>
#include <mxf/mxf_avid.h>

#include "VariableSizeEssenceParser.h"

using namespace std;
using namespace mxfpp;



MJPEGParseState::MJPEGParseState()
{
    resolution_id = 0;
    position = 0;
    prev_position = 0;
    end_of_field = false;
    field2 = false;
    skip_count = 0;
    have_len_byte1 = false;
    have_len_byte2 = false;
    marker_state = 0;
}

void MJPEGParseState::Reset()
{
    position = 0;
    prev_position = 0;
    end_of_field = false;
    field2 = false;
    skip_count = 0;
    have_len_byte1 = false;
    have_len_byte2 = false;
    marker_state = 0;
    buffer.setSize(0);
}



VariableSizeEssenceParser::VariableSizeEssenceParser(File *file, int64_t essence_length, mxfUL essence_label,
                                                     mxfRational edit_rate, const FileDescriptor *file_descriptor,
                                                     FrameOffsetIndexTableSegment *index_table)
: RawEssenceParser(file, essence_length, essence_label)
{
    (void)edit_rate;

    mFrameSizeEstimate = 0;

    if (index_table) {
        mIndexTable = index_table;
        mOwnIndexTable = false;
        mIndexTableIsComplete = true;
        mDuration = mIndexTable->getDuration();
    } else {
        // currently only support MJPEG
        MXFPP_ASSERT(mxf_equals_ul(&MXF_EC_L(AvidMJPEGClipWrapped), &essence_label));

        mIndexTable = new FrameOffsetIndexTableSegment();
        mOwnIndexTable = true;
        mIndexTable->appendFrameOffset(0);
        mIndexTableIsComplete = false;
        mDuration = -1;

        uint32_t unc_frame_size = DetermineUncFrameSize(file_descriptor);

        if (file_descriptor->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID)))
            mMJPEGParseState.resolution_id = file_descriptor->getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID));
        else
            mxf_log_warn("Missing Avid ResolutionID item in descriptor\n");

        switch (mMJPEGParseState.resolution_id)
        {
            case g_AvidMJPEG21_ResolutionID:
                mFrameSizeEstimate = (uint32_t)(unc_frame_size / (2 * 0.8));
                break;
            case g_AvidMJPEG31_ResolutionID:
                mFrameSizeEstimate = (uint32_t)(unc_frame_size / (3 * 0.8));
                break;
            case g_AvidMJPEG101_ResolutionID:
                mFrameSizeEstimate = (uint32_t)(unc_frame_size / (10 * 0.8));
                break;
            case g_AvidMJPEG41m_ResolutionID:
                mFrameSizeEstimate = (uint32_t)(unc_frame_size / (4 * 0.8));
                break;
            case g_AvidMJPEG101m_ResolutionID:
                mFrameSizeEstimate = (uint32_t)(unc_frame_size / (10 * 0.8));
                break;
            case g_AvidMJPEG151s_ResolutionID:
                mFrameSizeEstimate = (uint32_t)(unc_frame_size / (15 * 0.8));
                break;
            case g_AvidMJPEG201_ResolutionID:
                mFrameSizeEstimate = (uint32_t)(unc_frame_size / (20 * 0.8));
                break;
            default:
                mxf_log_warn("Unknown ResolutionID 0x%02x\n", mMJPEGParseState.resolution_id);
                mFrameSizeEstimate = 0;
                break;
        }
    }
}

VariableSizeEssenceParser::~VariableSizeEssenceParser()
{
    if (mOwnIndexTable)
        delete mIndexTable;
}

bool VariableSizeEssenceParser::Read(DynamicByteArray *data, uint32_t *num_samples)
{
    if (IsEOF())
        return false;

    if (!mIndexTable->haveFrameOffset(mPosition + 1)) {
        int64_t current_position = mPosition;

        UpdateIndexTable(data, mPosition + 1);

        // failed if position hasn't progressed by 1 frame which is the same as the index progressing by 1 frame
        if (!mIndexTable->haveFrameOffset(current_position + 1))
            return false;

        // UpdateIndexTable has read in the image data and updated mEssenceOffset and mPosition
    } else {
        uint32_t frame_size = (uint32_t)(mIndexTable->getFrameOffset(mPosition + 1) -
                                         mIndexTable->getFrameOffset(mPosition));
        data->allocate(frame_size);

        // copy data available in the parser buffer
        uint32_t available_size = 0;
        if (mMJPEGParseState.buffer.getSize() > 0) {
            available_size = mMJPEGParseState.buffer.getSize() - mMJPEGParseState.position;
            if (available_size > frame_size)
                available_size = frame_size;

            data->setSize(0);
            data->append(&mMJPEGParseState.buffer[mMJPEGParseState.position], available_size);

            mMJPEGParseState.position += available_size;
            mMJPEGParseState.prev_position = mMJPEGParseState.position;
            if (mMJPEGParseState.position == mMJPEGParseState.buffer.getSize())
                mMJPEGParseState.Reset();
        }

        // read from the file if more data is required
        if (frame_size - available_size > 0) {
            uint32_t count = mFile->read(data->getBytes() + available_size, frame_size - available_size);
            if (count != frame_size - available_size) {
                // this is unexpected
                mxf_log_warn("Failed to read frame indexed in the index table\n");
                Seek(mPosition);
                return false;
            }
        }

        data->setSize(frame_size);

        mEssenceOffset += data->getSize();
        mPosition++;
    }

    *num_samples = 1;

    return true;
}

bool VariableSizeEssenceParser::Seek(int64_t position)
{
    if (mDuration >= 0 && position >= mDuration)
        return false;

    int64_t frame_offset;
    if (!mIndexTable->haveFrameOffset(position)) {
        if (mIndexTableIsComplete || !UpdateIndexTable(&mSeekData, position))
            return false;

        frame_offset = mIndexTable->getFrameOffset(position);

        // no seek required because UpdateIndexTable has updated the file position and
        // mEssenceOffset and mPosition have also been updated
    } else {
        // discard data in the parser buffer
        mMJPEGParseState.Reset();

        frame_offset = mIndexTable->getFrameOffset(position);
        mFile->seek(mEssenceStartOffset + frame_offset, SEEK_SET);

        mEssenceOffset = frame_offset;
        mPosition = position;
    }

    return true;
}

int64_t VariableSizeEssenceParser::DetermineDuration()
{
    if (mDuration >= 0)
        return mDuration;

    int64_t current_position = mPosition;

    int64_t offset = 0;
    int64_t indexed_position = 0;
    mIndexTable->getLastIndexOffset(&offset, &indexed_position);

    while (!mIndexTableIsComplete) {
        indexed_position++;
        UpdateIndexTable(&mSeekData, indexed_position);
    }

    Seek(current_position);

    return mDuration;
}

uint32_t VariableSizeEssenceParser::DetermineUncFrameSize(const FileDescriptor *file_descriptor)
{
    const CDCIEssenceDescriptor *cdci_descriptor =
        dynamic_cast<const CDCIEssenceDescriptor*>(file_descriptor);
    MXFPP_CHECK(cdci_descriptor);

    uint32_t stored_width = 0, stored_height = 0;
    uint32_t h_subsamp = 0, v_subsamp = 0;
    int field_factor = 1;

    // only 8-bit is currently supported
    if (cdci_descriptor->haveComponentDepth() && cdci_descriptor->getComponentDepth() != 8)
        return 0;

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

    if (cdci_descriptor->haveVerticalSubsampling())
        v_subsamp = cdci_descriptor->getVerticalSubsampling();

    if (cdci_descriptor->haveStoredWidth())
        stored_width = cdci_descriptor->getStoredWidth();

    if (cdci_descriptor->haveStoredHeight())
        stored_height = cdci_descriptor->getStoredHeight();


    if (stored_width == 0 || stored_height == 0)
        return 0;

    if (h_subsamp == 0 || v_subsamp == 0) {
        h_subsamp = 1;
        v_subsamp = 1;
    }

    return (uint32_t)(field_factor * stored_width * stored_height * (1 + 2.0 / (h_subsamp * v_subsamp)) + 0.5);
}

bool VariableSizeEssenceParser::ParseMJPEGImage(DynamicByteArray *image_data)
{
    bool have_image = false;
    bool process_result;
    uint32_t num_read;

    mMJPEGParseState.buffer.minAllocate(4096);

    image_data->setSize(0);
    if (mFrameSizeEstimate > 0)
        image_data->minAllocate(mFrameSizeEstimate);

    while (true) {
        process_result = ProcessMJPEGImageData(image_data, &have_image);
        if (!process_result)
            return false;

        if (have_image)
            return true;

        mMJPEGParseState.buffer.setSize(0);
        num_read = mFile->read(mMJPEGParseState.buffer.getBytes(), mMJPEGParseState.buffer.getSizeAvailable());
        if (num_read == 0)
            return false; // EOF if nothing was read

        mMJPEGParseState.buffer.setSize(num_read);
        mMJPEGParseState.position = 0;
        mMJPEGParseState.prev_position = 0;
    }

    return false; // for the compiler
}

bool VariableSizeEssenceParser::ProcessMJPEGImageData(DynamicByteArray *image_data, bool *have_image)
{
    *have_image = false;

    if (mMJPEGParseState.buffer.getSize() == 0)
        return true;

    // locate start and end of image

    // states
    // 0 = search for 0xff
    // 1 = test for 0xd8 (start of image)
    // 2 = search for 0xff - start of marker
    // 3 = test for 0xd9 (end of image), else skip
    // 4 = skip marker segment data
    //
    // transitions
    // 0 -> 1 (data == 0xff)
    // 1 -> 0 (data != 0xd8 && data != 0xff)
    // 1 -> 2 (data == 0xd8)
    // 2 -> 3 (data == 0xff)
    // 3 -> 0 (data == 0xd9)
    // 3 -> 2 (data >= 0xd0 && data <= 0xd7 || data == 0x01 || data == 0x00)
    // 3 -> 4 (else and data != 0xff)

    while (!(*have_image) && mMJPEGParseState.position < mMJPEGParseState.buffer.getSize())
    {
        switch (mMJPEGParseState.marker_state)
        {
            case 0:
                if (mMJPEGParseState.buffer[mMJPEGParseState.position] == 0xff) {
                    mMJPEGParseState.marker_state = 1;
                } else {
                    // MJPEG image start is non-0xff byte - trailing data ignored
                    return false;
                }
                break;
            case 1:
                if (mMJPEGParseState.buffer[mMJPEGParseState.position] == 0xd8) // start of frame
                    mMJPEGParseState.marker_state = 2;
                else if (mMJPEGParseState.buffer[mMJPEGParseState.position] != 0xff) // 0xff is fill byte
                    mMJPEGParseState.marker_state = 0;
                break;
            case 2:
                if (mMJPEGParseState.buffer[mMJPEGParseState.position] == 0xff)
                    mMJPEGParseState.marker_state = 3;
                // else wait here
                break;
            case 3:
                if (mMJPEGParseState.buffer[mMJPEGParseState.position] == 0xD9) { // end of field
                    mMJPEGParseState.marker_state = 0;
                    mMJPEGParseState.end_of_field = true;
                }
                // 0xd0-0xd7 and 0x01 are empty markers and 0x00 is stuffed zero
                else if ((mMJPEGParseState.buffer[mMJPEGParseState.position] >= 0xd0 &&
                            mMJPEGParseState.buffer[mMJPEGParseState.position] <= 0xd7) ||
                         mMJPEGParseState.buffer[mMJPEGParseState.position] == 0x01 ||
                         mMJPEGParseState.buffer[mMJPEGParseState.position] == 0x00)
                {
                    mMJPEGParseState.marker_state = 2;
                }
                else if (mMJPEGParseState.buffer[mMJPEGParseState.position] != 0xff) // 0xff is fill byte
                {
                    mMJPEGParseState.marker_state = 4;
                    // initializations for mMJPEGParseState 4
                    mMJPEGParseState.have_len_byte1 = false;
                    mMJPEGParseState.have_len_byte2 = false;
                    mMJPEGParseState.skip_count = 0;
                }
                break;
            case 4:
                if (!mMJPEGParseState.have_len_byte1) {
                    mMJPEGParseState.have_len_byte1 = true;
                    mMJPEGParseState.skip_count = mMJPEGParseState.buffer[mMJPEGParseState.position] << 8;
                } else if (!mMJPEGParseState.have_len_byte2) {
                    mMJPEGParseState.have_len_byte2 = true;
                    mMJPEGParseState.skip_count += mMJPEGParseState.buffer[mMJPEGParseState.position];
                    mMJPEGParseState.skip_count -= 1; // length includes the 2 length bytes, one subtracted here and one below
                }

                if (mMJPEGParseState.have_len_byte1 && mMJPEGParseState.have_len_byte2) {
                    mMJPEGParseState.skip_count--;
                    if (mMJPEGParseState.skip_count == 0)
                        mMJPEGParseState.marker_state = 2;
                }
                break;
            default:
                MXFPP_ASSERT(false); // won't get here
        }
        mMJPEGParseState.position++;

        if (mMJPEGParseState.end_of_field) {
            // 15:1s, 10:1m and 4:1m are single field; other resolutions have 2 fields
            if (mMJPEGParseState.resolution_id == g_AvidMJPEG151s_ResolutionID ||
                mMJPEGParseState.resolution_id == g_AvidMJPEG101m_ResolutionID ||
                mMJPEGParseState.resolution_id == g_AvidMJPEG41m_ResolutionID ||
                mMJPEGParseState.field2)
            {
                *have_image = true;
            }
            mMJPEGParseState.end_of_field = false;
            mMJPEGParseState.field2 = !mMJPEGParseState.field2;
        }
    }

    image_data->append(&mMJPEGParseState.buffer[mMJPEGParseState.prev_position],
                       mMJPEGParseState.position - mMJPEGParseState.prev_position);

    mMJPEGParseState.prev_position = mMJPEGParseState.position;

    return true;
}

bool VariableSizeEssenceParser::UpdateIndexTable(DynamicByteArray *image_data, int64_t position)
{
    MXFPP_ASSERT(!mIndexTable->haveFrameOffset(position));

    if (mIndexTableIsComplete)
        return false;

    int64_t offset = 0;
    int64_t indexed_position = 0;
    mIndexTable->getLastIndexOffset(&offset, &indexed_position);

    if (indexed_position != mPosition)
        MXFPP_CHECK(Seek(indexed_position));

    while (indexed_position < position) {
        if (!ParseMJPEGImage(image_data))
            break;

        offset += image_data->getSize();
        mIndexTable->appendFrameOffset(offset);

        indexed_position++;
    }

    if (indexed_position < position) {
        // reached EOF
        mIndexTableIsComplete = true;
        mDuration = mIndexTable->getDuration();

        // seek back to mPosition if mPosition isn't at EOF
        if (mPosition != mDuration) {
            // last offset if the EOF
            mIndexTable->getLastIndexOffset(&offset, &indexed_position);
            mFile->seek(mEssenceOffset + offset, SEEK_SET);
        }

        return false;
    }

    mPosition = position;
    mEssenceOffset = offset;

    return true;
}

