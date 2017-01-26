/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#include <bmx/writer_helper/VC2WriterHelper.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define WORKSPACE_ALLOC_SIZE  4096


VC2WriterHelper::VC2WriterHelper()
{
    mDescriptorHelper = 0;
    mModeFlags = VC2_PICTURE_ONLY | VC2_COMPLETE_SEQUENCES;
    mProcessCount = 0;
    mSeqHeaderData = 0;
    mSeqHeaderDataAllocSize = 0;
    mSeqHeaderDataSize = 0;
    mFirstFrame = true;
    mLastPictureNumber = 0;
    mWarnedPictureNumberUpdate = false;
    memset(&mParseInfo, 0, sizeof(mParseInfo));
    memset(&mFirstSequenceHeader, 0, sizeof(mFirstSequenceHeader));
    memset(&mCurrentSequenceHeader, 0, sizeof(mCurrentSequenceHeader));
    mIdenticalSequence = true;
    mCompleteSequence = true;
    mWorkspace = new unsigned char[WORKSPACE_ALLOC_SIZE];
    mWorkspaceSize = 0;
}

VC2WriterHelper::~VC2WriterHelper()
{
    delete [] mSeqHeaderData;
    delete [] mWorkspace;
}

void VC2WriterHelper::SetDescriptorHelper(VC2MXFDescriptorHelper *descriptor_helper)
{
    mDescriptorHelper = descriptor_helper;
}

void VC2WriterHelper::SetModeFlags(int flags)
{
    BMX_ASSERT(mFirstFrame);
    mModeFlags = flags;
}

void VC2WriterHelper::SetSequenceHeader(const unsigned char *data, uint32_t size)
{
    BMX_ASSERT(mFirstFrame);

    uint32_t actual_size = mEssenceParser.ParseSequenceHeader(data, size, &mCurrentSequenceHeader);
    if (actual_size == 0 || actual_size == ESSENCE_PARSER_NULL_OFFSET)
        BMX_EXCEPTION(("Failed to parse provided VC-2 Sequence Header data"));
    if (actual_size < size)
        log_warn("Provided VC-2 Sequence Header data size %u is larger than necessary %u\n", size, actual_size);

    if (actual_size > mSeqHeaderDataAllocSize) {
        unsigned char *new_header_data = new unsigned char[actual_size];
        delete [] mSeqHeaderData;
        mSeqHeaderData = new_header_data;
        mSeqHeaderDataAllocSize = actual_size;
    }
    memcpy(mSeqHeaderData, data, actual_size);
    mSeqHeaderDataSize = actual_size;

    mEssenceParser.SetSequenceHeader(&mCurrentSequenceHeader);
}

uint32_t VC2WriterHelper::ProcessFrame(const unsigned char *data, uint32_t size,
                                       const CDataBuffer **data_array, uint32_t *array_size)
{
    // extract information

    mEssenceParser.ResetFrameParse();
    mEssenceParser.ParseFrameInfo(data, size);

    const vector<VC2EssenceParser::ParseInfo> input_parse_infos = mEssenceParser.GetParseInfos();
    BMX_ASSERT(!input_parse_infos.empty());

    bool sequence_header_changed = false;
    uint32_t seq_header_offset = 0;
    uint32_t seq_header_size = 0;
    size_t i;
    for (i = 0; i < input_parse_infos.size(); i++) {
        if (VC2EssenceParser::IsSequenceHeader(input_parse_infos[i].parse_code)) {
            seq_header_offset = input_parse_infos[i].frame_offset + VC2_PARSE_INFO_SIZE;
            if (i + 1 < input_parse_infos.size())
                seq_header_size = input_parse_infos[i + 1].frame_offset - seq_header_offset;
            else
                seq_header_size = size - seq_header_offset;
        }
    }
    if (seq_header_size == 0)
    {
        if (!mSeqHeaderData)
            BMX_EXCEPTION(("VC-2 is missing a Sequence Header"));
    }
    else if (!mSeqHeaderData ||
             seq_header_size != mSeqHeaderDataSize ||
             memcmp(mSeqHeaderData, &data[seq_header_offset], seq_header_size) != 0)
    {
        if (!mFirstFrame && mSeqHeaderData) {
            sequence_header_changed = true;
            mIdenticalSequence = false;
        }
        if (seq_header_size > mSeqHeaderDataAllocSize) {
            unsigned char *new_header_data = new unsigned char[seq_header_size];
            delete [] mSeqHeaderData;
            mSeqHeaderData = new_header_data;
            mSeqHeaderDataAllocSize = seq_header_size;
        }
        memcpy(mSeqHeaderData, &data[seq_header_offset], seq_header_size);
        mSeqHeaderDataSize = seq_header_size;

        uint32_t actual_size = mEssenceParser.ParseSequenceHeader(mSeqHeaderData, mSeqHeaderDataSize,
                                                                  &mCurrentSequenceHeader);
        if (actual_size == 0 || actual_size == ESSENCE_PARSER_NULL_OFFSET)
            BMX_EXCEPTION(("Failed to parse VC-2 Sequence Header data"));
        if (actual_size < seq_header_size)
            log_warn("VC-2 Sequence Header data size %u is larger than necessary %u\n", seq_header_size, actual_size);
    }

    if (mFirstFrame)
        memcpy(&mFirstSequenceHeader, &mCurrentSequenceHeader, sizeof(mFirstSequenceHeader));

    if ((mCurrentSequenceHeader.picture_coding_mode == 0 && mEssenceParser.GetPictureCount() != 1) ||
        (mCurrentSequenceHeader.picture_coding_mode == 1 && mEssenceParser.GetPictureCount() != 2))
    {
        BMX_EXCEPTION(("VC-2 frame is missing Picture data unit(s)"));
    }
    mWaveletFilters.insert(mEssenceParser.GetPictureHeader1()->wavelet_index);
    if (mEssenceParser.GetPictureCount() == 2)
        mWaveletFilters.insert(mEssenceParser.GetPictureHeader2()->wavelet_index);


    // create output data array

    CDataBuffer c_data_buf;
    uint32_t output_frame_size = 0;
    mDataArray.clear();
    mWorkspaceSize = 0;

    mParseInfo.parse_code = 0x00; // Sequence Header
    mParseInfo.prev_parse_offset = mParseInfo.next_parse_offset;
    mParseInfo.next_parse_offset = VC2_PARSE_INFO_SIZE + mSeqHeaderDataSize;
    output_frame_size += WriteParseInfo();

    c_data_buf.data = mSeqHeaderData;
    c_data_buf.size = mSeqHeaderDataSize;
    mDataArray.push_back(c_data_buf);
    output_frame_size += c_data_buf.size;

    bool frame_is_complete_sequence = false;
    uint32_t picture_count = 0;
    for (i = 0; i < input_parse_infos.size(); i++) {
        if (VC2EssenceParser::IsSequenceHeader(input_parse_infos[i].parse_code))
            continue;
        if ((mModeFlags & VC2_PICTURE_ONLY) &&
            !VC2EssenceParser::IsPicture(input_parse_infos[i].parse_code) &&
            !VC2EssenceParser::IsEndOfSequence(input_parse_infos[i].parse_code))
        {
            continue;
        }

        uint32_t next_parse_offset;
        if (i + 1 < input_parse_infos.size())
            next_parse_offset = input_parse_infos[i + 1].frame_offset - input_parse_infos[i].frame_offset;
        else
            next_parse_offset = size - input_parse_infos[i].frame_offset;

        mParseInfo.parse_code = input_parse_infos[i].parse_code;
        mParseInfo.prev_parse_offset = mParseInfo.next_parse_offset;
        if (VC2EssenceParser::IsEndOfSequence(input_parse_infos[i].parse_code))
            mParseInfo.next_parse_offset = 0;
        else
            mParseInfo.next_parse_offset = next_parse_offset;
        output_frame_size += WriteParseInfo();

        if (VC2EssenceParser::IsPicture(input_parse_infos[i].parse_code)) {
            VC2EssenceParser::PictureHeader picture_header;
            uint32_t res = mEssenceParser.ParsePictureHeader(&data[input_parse_infos[i].frame_offset + VC2_PARSE_INFO_SIZE],
                                                             next_parse_offset - VC2_PARSE_INFO_SIZE,
                                                             &picture_header);
            if (res == 0 || res == ESSENCE_PARSER_NULL_OFFSET)
                BMX_EXCEPTION(("Failed to parse VC-2 Picture header data"));
            if (picture_count == 0 && mEssenceParser.GetPictureCount() == 2 && (picture_header.picture_number & 1))
                BMX_EXCEPTION(("VC-2 Picture number of first field is not even"));

            uint32_t picture_number;
            if (sequence_header_changed || (mFirstFrame && picture_count == 0))
                picture_number = picture_header.picture_number;
            else
                picture_number = mLastPictureNumber + 1;
            if (picture_count == 1 && picture_number != mLastPictureNumber + 1)
                BMX_EXCEPTION(("VC-2 Picture number of second field does not equal first field number + 1"));

            if (picture_number == picture_header.picture_number) {
                c_data_buf.data = (unsigned char*)&data[input_parse_infos[i].frame_offset + VC2_PARSE_INFO_SIZE];
                c_data_buf.size = next_parse_offset - VC2_PARSE_INFO_SIZE;
                mDataArray.push_back(c_data_buf);
                output_frame_size += c_data_buf.size;
            } else {
                uint32_t picture_header_size = WritePictureHeader(picture_number);
                output_frame_size += picture_header_size;

                c_data_buf.data = (unsigned char*)&data[input_parse_infos[i].frame_offset + VC2_PARSE_INFO_SIZE + picture_header_size];
                c_data_buf.size = next_parse_offset - VC2_PARSE_INFO_SIZE - picture_header_size;
                mDataArray.push_back(c_data_buf);
                output_frame_size += c_data_buf.size;

                if (!mWarnedPictureNumberUpdate) {
                    log_warn("VC-2 Picture number(s) updated\n");
                    mWarnedPictureNumberUpdate = true;
                }
            }

            picture_count++;
            mLastPictureNumber = picture_number;
        } else if (VC2EssenceParser::IsEndOfSequence(input_parse_infos[i].parse_code)) {
            if (i + 1 < input_parse_infos.size())
                BMX_EXCEPTION(("VC-2 frame data has Parse Info after an End Of Sequence"));
            frame_is_complete_sequence = true;
        } else if (next_parse_offset > VC2_PARSE_INFO_SIZE) {
            c_data_buf.data = (unsigned char*)&data[input_parse_infos[i].frame_offset + VC2_PARSE_INFO_SIZE];
            c_data_buf.size = next_parse_offset - VC2_PARSE_INFO_SIZE;
            mDataArray.push_back(c_data_buf);
            output_frame_size += c_data_buf.size;
        }
    }

    if ((mModeFlags & VC2_COMPLETE_SEQUENCES) && !frame_is_complete_sequence) {
        mParseInfo.parse_code = 0x10; // End of Sequence
        mParseInfo.prev_parse_offset = mParseInfo.next_parse_offset;
        mParseInfo.next_parse_offset = 0;
        output_frame_size += WriteParseInfo();

        mCompleteSequence = true;
    } else if (!frame_is_complete_sequence) {
        mCompleteSequence = false;
    }

    mProcessCount++;
    mFirstFrame = false;

    *data_array = &mDataArray[0];
    *array_size = (uint32_t)mDataArray.size();

    return output_frame_size;
}

void VC2WriterHelper::CompleteProcess()
{
    BMX_ASSERT(mDescriptorHelper);

    if (mFirstFrame)
        return;

    vector<uint8_t> wavelet_filter_vector;
    wavelet_filter_vector.insert(wavelet_filter_vector.begin(), mWaveletFilters.begin(), mWaveletFilters.end());

    mDescriptorHelper->SetSequenceHeader(&mFirstSequenceHeader);
    mDescriptorHelper->SetWaveletFilters(wavelet_filter_vector);
    mDescriptorHelper->SetIdenticalSequence(mIdenticalSequence);
    mDescriptorHelper->SetCompleteSequence(mCompleteSequence);

    if (!mIdenticalSequence || !mCompleteSequence) {
        if (!mIdenticalSequence)
            log_warn("VC-2 sequences are not identical\n");
        if (!mCompleteSequence)
            log_warn("VC-2 frames are not complete sequences\n");
        log_warn("VC-2 is not using Operating Mode A\n");
    }
}

uint32_t VC2WriterHelper::WriteParseInfo()
{
    BMX_CHECK(mWorkspaceSize + VC2_PARSE_INFO_SIZE <= WORKSPACE_ALLOC_SIZE);

    unsigned char *data = &mWorkspace[mWorkspaceSize];
    data[0] = 0x42;
    data[1] = 0x42;
    data[2] = 0x43;
    data[3] = 0x44;
    data[4] = mParseInfo.parse_code;
    data[5] = (uint8_t)(mParseInfo.next_parse_offset >> 24);
    data[6] = (uint8_t)(mParseInfo.next_parse_offset >> 16);
    data[7] = (uint8_t)(mParseInfo.next_parse_offset >> 8);
    data[8] = (uint8_t)(mParseInfo.next_parse_offset);
    data[9] = (uint8_t)(mParseInfo.prev_parse_offset >> 24);
    data[10] = (uint8_t)(mParseInfo.prev_parse_offset >> 16);
    data[11] = (uint8_t)(mParseInfo.prev_parse_offset >> 8);
    data[12] = (uint8_t)(mParseInfo.prev_parse_offset);

    CDataBuffer c_data_buf;
    c_data_buf.data = data;
    c_data_buf.size = VC2_PARSE_INFO_SIZE;
    mDataArray.push_back(c_data_buf);

    mWorkspaceSize += VC2_PARSE_INFO_SIZE;

    return c_data_buf.size;
}

uint32_t VC2WriterHelper::WritePictureHeader(uint32_t picture_number)
{
    BMX_CHECK(mWorkspaceSize + 4 <= WORKSPACE_ALLOC_SIZE);

    unsigned char *data = &mWorkspace[mWorkspaceSize];
    data[0] = (uint8_t)(picture_number >> 24);
    data[1] = (uint8_t)(picture_number >> 16);
    data[2] = (uint8_t)(picture_number >> 8);
    data[3] = (uint8_t)(picture_number);

    CDataBuffer c_data_buf;
    c_data_buf.data = data;
    c_data_buf.size = 4;
    mDataArray.push_back(c_data_buf);

    mWorkspaceSize += 4;

    return c_data_buf.size;
}
