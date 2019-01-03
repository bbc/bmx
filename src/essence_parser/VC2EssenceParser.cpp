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

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <string.h>
#include <limits.h>

#include <bmx/essence_parser/VC2EssenceParser.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define PARSE_INFO_PREFIX   0x42424344
#define MAX_SEARCH_COUNT    100000000   // 100MB


typedef struct
{
    uint32_t luma_offset;
    uint32_t luma_excursion;
    uint32_t color_diff_offset;
    uint32_t color_diff_excursion;
} SignalRange;

typedef struct
{
    uint8_t color_primaries;
    uint8_t color_matrix;
    uint8_t transfer_function;
} ColorSpec;


static const Rational PRESET_FRAME_RATE[] =
{
    {24000, 1001},
    {   24,    1},
    {   25,    1},
    {30000, 1001},
    {   30,    1},
    {   50,    1},
    {60000, 1001},
    {   60,    1},
    {15000, 1001},
    {   25,    2},
    {   48,    1},
};

static const Rational PRESET_PIXEL_ASPECT_RATIO[] =
{
    { 1,  1},
    {10, 11},
    {12, 11},
    {40, 33},
    {16, 11},
    { 4,  3},
};

static const SignalRange PRESET_SIGNAL_RANGE[] =
{
    {  0,  255,  128,  255},
    { 16,  219,  128,  224},
    { 64,  876,  512,  896},
    {256, 3504, 2048, 3584},
};

static const ColorSpec PRESET_COLOR_SPEC[] =
{
    {0, 0, 0},
    {1, 1, 0},
    {2, 1, 0},
    {0, 0, 0},
    {3, 2, 3},
};

static const VC2EssenceParser::VideoParameters DEFAULT_SOURCE_PARAMETERS[] =
{
    { 640,  480, 2, 0, 0, 24000, 1001,  1,  1,  640,  480, 0, 0,   0,  255,  128,  255, 0, 0, 0}, // Custom Format
    { 176,  120, 2, 0, 0, 15000, 1001, 10, 11,  176,  120, 0, 0,   0,  255,  128,  255, 1, 1, 0}, // QSIF525
    { 176,  144, 2, 0, 1,    25,    2, 12, 11,  176,  144, 0, 0,   0,  255,  128,  255, 2, 1, 0}, // QCIF
    { 352,  240, 2, 0, 0, 15000, 1001, 10, 11,  352,  240, 0, 0,   0,  255,  128,  255, 1, 1, 0}, // SIF525
    { 352,  288, 2, 0, 1,    25,    2, 12, 11,  352,  288, 0, 0,   0,  255,  128,  255, 2, 1, 0}, // CIF
    { 704,  480, 2, 0, 0, 15000, 1001, 10, 11,  704,  480, 0, 0,   0,  255,  128,  255, 1, 1, 0}, // 4SIF525
    { 704,  576, 2, 0, 1,    25,    2, 12, 11,  704,  576, 0, 0,   0,  255,  128,  255, 2, 1, 0}, // CIF
    { 720,  480, 1, 1, 0, 30000, 1001, 10, 11,  704,  480, 8, 0,  64,  876,  512,  896, 1, 1, 0}, // SD480I-60
    // note: the pixel aspect ratio in ST 2042-1,Table C1.a index is correct but the values, 10/11 are not
    { 720,  576, 1, 1, 1,    25,    1, 12, 11,  704,  576, 8, 0,  64,  876,  512,  896, 2, 1, 0}, // SD576I-50
    {1280,  720, 1, 0, 1, 60000, 1001,  1,  1, 1280,  720, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD720P-60
    {1280,  720, 1, 0, 1,    50,    1,  1,  1, 1280,  720, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD720P-50
    {1920, 1080, 1, 1, 1, 30000, 1001,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080I-60
    {1920, 1080, 1, 1, 1,    25,    1,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080I-50
    {1920, 1080, 1, 0, 1, 60000, 1001,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080P-60
    {1920, 1080, 1, 0, 1,    50,    1,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080P-50
    {2048, 1080, 0, 0, 1,    24,    1,  1,  1, 2048, 1080, 0, 0, 256, 3504, 2048, 3584, 3, 3, 3}, // DC2K
    {4096, 2160, 0, 0, 1,    24,    1,  1,  1, 4096, 2160, 0, 0, 256, 3504, 2048, 3584, 3, 3, 3}, // DC4K
    {3840, 2160, 1, 0, 1, 60000, 1001,  1,  1, 3840, 2160, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 4K-60
    {3840, 2160, 1, 0, 1,    50,    1,  1,  1, 3840, 2160, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 4K-50
    {7680, 4320, 1, 0, 1, 60000, 1001,  1,  1, 7680, 4320, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 8K-60
    {7680, 4320, 1, 0, 1,    50,    1,  1,  1, 7680, 4320, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 8K-50
    {1920, 1080, 1, 0, 1, 24000, 1001,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080P-24
    { 720,  486, 1, 1, 0, 30000, 1001, 10, 11,  720,  486, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // SD Pro486
};


static uint8_t intlog2(uint64_t n)
{
    uint8_t m = 63;
    uint64_t mask = UINT64_C(1) << 63;
    while (mask && !(n & mask)) {
        mask >>= 1;
        m--;
    }
    if (mask && (n & (mask - 1)))
        m++;
    return m;
}



VC2GetBitBuffer::VC2GetBitBuffer(const unsigned char *data, uint32_t data_size)
: GetBitBuffer(data, data_size)
{
}

void VC2GetBitBuffer::ByteAlign()
{
    SetBitPos(mBitPos + ((8 - (mBitPos & 7)) & 7));
}

uint8_t VC2GetBitBuffer::GetBit()
{
    uint8_t bit;
    if (!GetBits(1, &bit))
        throw false;
    return bit;
}

uint8_t VC2GetBitBuffer::GetByte()
{
    uint8_t value;
    if (!GetUInt8(&value))
        throw false;
    return value;
}

uint64_t VC2GetBitBuffer::GetUIntLit(uint32_t byte_count)
{
    uint64_t value;
    ByteAlign();
    if (!GetBits(byte_count * 8, &value))
        throw false;
    return value;
}

uint64_t VC2GetBitBuffer::GetUInt()
{
    uint64_t value = 1;
    while (GetBit() == 0) {
        value <<= 1;
        if (GetBit() == 1)
            value += 1;
    }
    return value - 1;
}


VC2EssenceParser::VC2EssenceParser()
{
    memset(&mSequenceHeader, 0, sizeof(mSequenceHeader));
    ResetFrameParse();
}

VC2EssenceParser::~VC2EssenceParser()
{
}

uint32_t VC2EssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    VC2GetBitBuffer buffer(data, data_size);

    ParseInfo parse_info;
    if (ParseParseInfo(&buffer, &parse_info) == 1)
        return 0;
    else
        return ESSENCE_PARSER_NULL_OFFSET;
}

uint32_t VC2EssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    if (mOffset == 0)
        ResetFrameSize();

    VC2GetBitBuffer buffer(data, data_size);
    buffer.SetPos(mOffset);

    uint32_t frame_size = 0;
    while (buffer.GetRemSize() > 0) {
        if (mParseState == PARSE_INFO_STATE) {
            uint32_t res = ParseParseInfo(&buffer, &mCurrentParseInfo);
            if (res == 0) {
                log_warn("Failed to parse Parse Info\n");
                return ESSENCE_PARSER_NULL_FRAME_SIZE;
            } else if (res == ESSENCE_PARSER_NULL_OFFSET) {
                break;
            }

            if (IsSequenceHeader(mCurrentParseInfo.parse_code)) {
                if ((mSequenceHeader.picture_coding_mode == 0 && mPictureCount == 1) ||
                    (mSequenceHeader.picture_coding_mode == 1 && mPictureCount == 2))
                {
                    frame_size = buffer.GetPos() - VC2_PARSE_INFO_SIZE;
                    break;
                }
                mParseInfos.push_back(mCurrentParseInfo);
                mOffset = buffer.GetPos();
                mParseState = SEQUENCE_HEADER_STATE;
            } else if (IsPicture(mCurrentParseInfo.parse_code)) {
                if ((mSequenceHeader.picture_coding_mode == 0 && mPictureCount == 1) ||
                    (mSequenceHeader.picture_coding_mode == 1 && mPictureCount == 2))
                {
                    frame_size = buffer.GetPos() - VC2_PARSE_INFO_SIZE;
                    break;
                }
                mParseInfos.push_back(mCurrentParseInfo);
                mOffset = buffer.GetPos();
                mParseState = PICTURE_STATE;
            } else if (IsEndOfSequence(mCurrentParseInfo.parse_code)) {
                mParseInfos.push_back(mCurrentParseInfo);
                frame_size = buffer.GetPos();
                break;
            } else {
                // decide whether Auxiliary/Padding units appear before (true) the picture or after (false)
                if (!mSecondaryParseInfoLocs.count(mCurrentParseInfo.parse_code))
                {
                    if (mPictureCount == 0 || (mPictureCount == 1 && mSequenceHeader.picture_coding_mode == 1))
                        mSecondaryParseInfoLocs[mCurrentParseInfo.parse_code] = true;
                    else
                        mSecondaryParseInfoLocs[mCurrentParseInfo.parse_code] = false;
                }
                else if (mSecondaryParseInfoLocs[mCurrentParseInfo.parse_code] &&
                          ((mSequenceHeader.picture_coding_mode == 0 && mPictureCount == 1) ||
                           (mSequenceHeader.picture_coding_mode == 1 && mPictureCount == 2)))
                {
                    frame_size = buffer.GetPos() - VC2_PARSE_INFO_SIZE;
                    break;
                }
                mParseInfos.push_back(mCurrentParseInfo);
                mOffset = buffer.GetPos();
                mParseState = NEXT_PARSE_INFO_STATE;
            }
        } else if (mParseState == SEQUENCE_HEADER_STATE) {
            SequenceHeader sequence_header;
            uint32_t res = ParseSequenceHeader(&buffer, &sequence_header);
            if (res == 0) {
                log_warn("Failed to parse Sequence Header\n");
                return ESSENCE_PARSER_NULL_FRAME_SIZE;
            } else if (res == ESSENCE_PARSER_NULL_OFFSET) {
                break;
            }

            memcpy(&mSequenceHeader, &sequence_header, sizeof(mSequenceHeader));
            mOffset = buffer.GetPos();
            mParseState = PARSE_INFO_STATE;
        } else if (mParseState == PICTURE_STATE) {
            mPictureCount++;
            mParseState = NEXT_PARSE_INFO_STATE;
        } else if (mParseState == NEXT_PARSE_INFO_STATE) {
            if (mCurrentParseInfo.next_parse_offset >= VC2_PARSE_INFO_SIZE) {
                if (buffer.GetPos() - VC2_PARSE_INFO_SIZE + mCurrentParseInfo.next_parse_offset >= buffer.GetSize())
                    break;
                buffer.SetPos(buffer.GetPos() - VC2_PARSE_INFO_SIZE + mCurrentParseInfo.next_parse_offset);
                mOffset = buffer.GetPos();
                mParseState = PARSE_INFO_STATE;
            } else {
                mSearchState = 0;
                mSearchCount = (uint32_t)(buffer.GetPos() - mOffset);
                mParseState = SEARCH_PARSE_INFO_STATE;
            }
        } else { // mParseState == SEARCH_PARSE_INFO_STATE
            buffer.SetPos(mOffset + mSearchCount);
            while (buffer.GetRemSize() > 0) {
                uint32_t next_data;
                uint32_t read_count;
                while (mSearchState != PARSE_INFO_PREFIX && mSearchCount < MAX_SEARCH_COUNT) {
                    if ((mSearchState & 0x00ffffff) == (PARSE_INFO_PREFIX >> 8))
                        read_count = 1;
                    else if ((mSearchState & 0x0000ffff) == (PARSE_INFO_PREFIX >> 16))
                        read_count = 2;
                    else if ((mSearchState & 0x000000ff) == (PARSE_INFO_PREFIX >> 24))
                        read_count = 3;
                    else
                        read_count = 4;
                    try {
                        next_data = (uint32_t)buffer.GetUIntLit(read_count);
                    } catch (...) {
                        // end of buffer
                        break;
                    }
                    if (read_count == 4)
                        mSearchState = next_data;
                    else
                        mSearchState = (mSearchState << (read_count * 8)) | next_data;
                    mSearchCount += read_count;
                }
                if (mSearchState != PARSE_INFO_PREFIX) {
                    if (mSearchCount >= MAX_SEARCH_COUNT) {
                        log_warn("Failed to find next parse info within maximum %u bytes\n", MAX_SEARCH_COUNT);
                        return ESSENCE_PARSER_NULL_FRAME_SIZE;
                    }
                    break;
                }

                ParseInfo parse_info;
                buffer.SetPos(buffer.GetPos() - 4);
                uint32_t res = ParseParseInfo(&buffer, &parse_info);
                if (res == 0) {
                    mSearchState = 0;
                } else {
                    if (res != ESSENCE_PARSER_NULL_OFFSET &&
                        parse_info.prev_parse_offset == VC2_PARSE_INFO_SIZE + mSearchCount - 4)
                    {
                        buffer.SetPos(buffer.GetPos() - VC2_PARSE_INFO_SIZE);
                        mOffset = buffer.GetPos();
                        mParseState = PARSE_INFO_STATE;
                    }
                    break;
                }
            }
            if (mParseState == SEARCH_PARSE_INFO_STATE)
                break; // reached end of buffer
        }
    }

    if (frame_size > 0) {
        if ((mSequenceHeader.picture_coding_mode == 0 && mPictureCount != 1) ||
            (mSequenceHeader.picture_coding_mode == 1 && mPictureCount != 2))
        {
            log_warn("Missing Picture data unit(s)\n");
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
        }
        mOffset = 0; // results in ResetFrameSize() in next call
        return frame_size;
    } else {
        return ESSENCE_PARSER_NULL_OFFSET;
    }
}

void VC2EssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    ResetFrameInfo();

    if (mParseInfos.empty()) {
        mOffset = 0; // results in reset
        uint32_t res = ParseFrameSize(data, data_size);
        if (res == ESSENCE_PARSER_NULL_FRAME_SIZE)
            BMX_EXCEPTION(("Invalid VC-2 frame"));
        if (mPictureCount == 0)
            BMX_EXCEPTION(("No Picture data units in VC-2 frame"));
        if (res != ESSENCE_PARSER_NULL_OFFSET && res < data_size)
            log_warn("VC-2 frame size %u is larger than size %u determined by parsing\n", data_size, res);
    }

    VC2GetBitBuffer buffer(data, data_size);

    mPictureCount = 0;

    size_t i;
    for (i = 0; i < mParseInfos.size(); i++) {
        if (IsSequenceHeader(mParseInfos[i].parse_code)) {
            mFrameHasSequenceHeader = true;
        } else if (IsPicture(mParseInfos[i].parse_code)) {
            buffer.SetPos(mParseInfos[i].frame_offset + VC2_PARSE_INFO_SIZE);
            uint32_t res;
            if (mPictureCount == 0)
                res = ParsePictureHeader(&buffer, &mPictureHeader1);
            else
                res = ParsePictureHeader(&buffer, &mPictureHeader2);
            if (res == ESSENCE_PARSER_NULL_FRAME_SIZE)
                BMX_EXCEPTION(("Invalid Picture Header data"));
            else if (res == ESSENCE_PARSER_NULL_OFFSET)
                BMX_EXCEPTION(("Insufficient data for Picture Header"));
            mPictureCount++;
        }
    }
}

uint32_t VC2EssenceParser::ParseSequenceHeader(const unsigned char *data, uint32_t data_size,
                                               SequenceHeader *sequence_header)
{
    VC2GetBitBuffer buffer(data, data_size);
    return ParseSequenceHeader(&buffer, sequence_header);
}

uint32_t VC2EssenceParser::ParsePictureHeader(const unsigned char *data, uint32_t data_size,
                                              PictureHeader *picture_header)
{
    VC2GetBitBuffer buffer(data, data_size);
    return ParsePictureHeader(&buffer, picture_header);
}

void VC2EssenceParser::ResetFrameParse()
{
    ResetFrameSize();
    ResetFrameInfo();
}

void VC2EssenceParser::SetSequenceHeader(const SequenceHeader *sequence_header)
{
    memcpy(&mSequenceHeader, sequence_header, sizeof(mSequenceHeader));
}

const VC2EssenceParser::PictureHeader* VC2EssenceParser::GetPictureHeader2() const
{
    if (mPictureCount > 1)
        return &mPictureHeader2;
    else
        return 0;
}

Rational VC2EssenceParser::GetFrameRate() const
{
    Rational frame_rate;
    frame_rate.numerator = mSequenceHeader.source_params.frame_rate_numer;
    frame_rate.denominator = mSequenceHeader.source_params.frame_rate_denom;
    return normalize_rate(frame_rate);
}

uint32_t VC2EssenceParser::ParseParseInfo(VC2GetBitBuffer *buffer, ParseInfo *parse_info)
{
    try
    {
        memset(parse_info, 0, sizeof(*parse_info));

        buffer->ByteAlign();
        parse_info->frame_offset = buffer->GetPos();

        uint32_t parse_info_prefix = (uint32_t)buffer->GetUIntLit(4);
        if (parse_info_prefix != PARSE_INFO_PREFIX) {
            log_warn("Invalid Parse Info prefix 0x%08x\n", parse_info_prefix);
            return 0;
        }
        parse_info->parse_code = buffer->GetByte();
        parse_info->next_parse_offset = (uint32_t)buffer->GetUIntLit(4);
        parse_info->prev_parse_offset = (uint32_t)buffer->GetUIntLit(4);

        return 1;
    }
    catch (...)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }
}

uint32_t VC2EssenceParser::ParseSequenceHeader(VC2GetBitBuffer *buffer, SequenceHeader *sequence_header)
{
    try
    {
        memset(sequence_header, 0, sizeof(*sequence_header));

        buffer->ByteAlign();
        uint32_t start_pos = buffer->GetPos();
        sequence_header->major_version = (uint8_t)buffer->GetUInt();
        sequence_header->minor_version = (uint8_t)buffer->GetUInt();
        sequence_header->profile = (uint8_t)buffer->GetUInt();
        sequence_header->level = (uint8_t)buffer->GetUInt();

        uint64_t base_video_format = buffer->GetUInt();
        if (base_video_format >= BMX_ARRAY_SIZE(DEFAULT_SOURCE_PARAMETERS)) {
            log_warn("Unknown base video format index %" PRIu64 "\n", base_video_format);
            return 0;
        }
        sequence_header->base_video_format = (uint8_t)base_video_format;
        if (!ParseSourceParameters(buffer, sequence_header))
            return 0;

        uint64_t picture_coding_mode = buffer->GetUInt();
        if (picture_coding_mode > 1) {
            log_warn("Unknown picture coding mode %" PRIu64 "\n", picture_coding_mode);
            return 0;
        }
        sequence_header->picture_coding_mode = (uint8_t)picture_coding_mode;
        SetCodingParameters(sequence_header);

        return buffer->GetPos() - start_pos + ((buffer->GetBitPos() & 7) ? 1 : 0);
    }
    catch (...)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }
}

uint32_t VC2EssenceParser::ParsePictureHeader(VC2GetBitBuffer *buffer, PictureHeader *picture_header)
{
    try
    {
        memset(picture_header, 0, sizeof(*picture_header));

        buffer->ByteAlign();
        picture_header->picture_number = (uint32_t)buffer->GetUIntLit(4);

        buffer->ByteAlign();
        uint64_t wavelet_index = buffer->GetUInt();
        if (wavelet_index > 6) {
            log_warn("Unknown wavelet filter index %" PRIu64 "\n", wavelet_index);
            return 0;
        }
        picture_header->wavelet_index = (uint8_t)wavelet_index;

        return 1;
    }
    catch (...)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }
}

bool VC2EssenceParser::ParseSourceParameters(VC2GetBitBuffer *buffer, SequenceHeader *sequence_header)
{
    BMX_ASSERT(sequence_header->base_video_format < BMX_ARRAY_SIZE(DEFAULT_SOURCE_PARAMETERS));
    memcpy(&sequence_header->source_params, &DEFAULT_SOURCE_PARAMETERS[sequence_header->base_video_format],
           sizeof(sequence_header->source_params));

    if (buffer->GetBool()) {
        sequence_header->source_params.frame_width = buffer->GetUInt();
        sequence_header->source_params.frame_height = buffer->GetUInt();
    }

    if (buffer->GetBool()) {
        uint64_t index = buffer->GetUInt();
        if (index > 2) {
            log_warn("Unknown color diff format index %" PRIu64 "\n", index);
            return false;
        }
        sequence_header->source_params.color_diff_format_index = (uint8_t)index;
    }

    if (buffer->GetBool()) {
        uint64_t index = buffer->GetUInt();
        if (index > 1) {
            log_warn("Unknown source sampling index %" PRIu64 "\n", index);
            return false;
        }
        sequence_header->source_params.source_sampling = (uint8_t)index;
    }

    if (buffer->GetBool()) {
        uint64_t index = buffer->GetUInt();
        if (index == 0) {
            uint64_t num = buffer->GetUInt();
            uint64_t den = buffer->GetUInt();
            if (num > INT32_MAX || den > INT32_MAX)
                BMX_EXCEPTION(("VC-2 frame rate %" PRIu64 "/%" PRIu64 " is too large for int32", num, den));
            sequence_header->source_params.frame_rate_numer = (int32_t)num;
            sequence_header->source_params.frame_rate_denom = (int32_t)den;
        } else {
            if (index - 1 >= BMX_ARRAY_SIZE(PRESET_FRAME_RATE)) {
                log_warn("Unknown frame rate index %" PRIu64 "\n", index);
                return false;
            }
            sequence_header->source_params.frame_rate_numer = PRESET_FRAME_RATE[index - 1].numerator;
            sequence_header->source_params.frame_rate_denom = PRESET_FRAME_RATE[index - 1].denominator;
        }
    }

    if (buffer->GetBool()) {
        uint64_t index = buffer->GetUInt();
        if (index == 0) {
            sequence_header->source_params.pixel_aspect_ratio_numer = buffer->GetUInt();
            sequence_header->source_params.pixel_aspect_ratio_denom = buffer->GetUInt();
        } else {
            if (index - 1 >= BMX_ARRAY_SIZE(PRESET_PIXEL_ASPECT_RATIO)) {
                log_warn("Unknown pixel aspect ratio index %" PRIu64 "\n", index);
                return false;
            }
            sequence_header->source_params.pixel_aspect_ratio_numer = PRESET_PIXEL_ASPECT_RATIO[index - 1].numerator;
            sequence_header->source_params.pixel_aspect_ratio_denom = PRESET_PIXEL_ASPECT_RATIO[index - 1].denominator;
        }
    }

    if (buffer->GetBool()) {
        sequence_header->source_params.clean_width = buffer->GetUInt();
        sequence_header->source_params.clean_height = buffer->GetUInt();
        sequence_header->source_params.left_offset = buffer->GetUInt();
        sequence_header->source_params.top_offset = buffer->GetUInt();
    }

    if (buffer->GetBool()) {
        uint64_t index = buffer->GetUInt();
        if (index == 0) {
            sequence_header->source_params.luma_offset = buffer->GetUInt();
            sequence_header->source_params.luma_excursion = buffer->GetUInt();
            sequence_header->source_params.color_diff_offset = buffer->GetUInt();
            sequence_header->source_params.color_diff_excursion = buffer->GetUInt();
        } else {
            if (index - 1 >= BMX_ARRAY_SIZE(PRESET_SIGNAL_RANGE)) {
                log_warn("Unknown signal range index %" PRIu64 "\n", index);
                return false;
            }
            sequence_header->source_params.luma_offset = PRESET_SIGNAL_RANGE[index - 1].luma_offset;
            sequence_header->source_params.luma_excursion = PRESET_SIGNAL_RANGE[index - 1].luma_excursion;
            sequence_header->source_params.color_diff_offset = PRESET_SIGNAL_RANGE[index - 1].color_diff_offset;
            sequence_header->source_params.color_diff_excursion = PRESET_SIGNAL_RANGE[index - 1].color_diff_excursion;
        }
    }

    if (buffer->GetBool()) {
        uint64_t index = buffer->GetUInt();
        if (index >= BMX_ARRAY_SIZE(PRESET_COLOR_SPEC)) {
            log_warn("Unknown color spec index %" PRIu64 "\n", index);
            return false;
        }
        sequence_header->source_params.color_primaries = PRESET_COLOR_SPEC[index].color_primaries;
        sequence_header->source_params.color_matrix = PRESET_COLOR_SPEC[index].color_matrix;
        sequence_header->source_params.transfer_function = PRESET_COLOR_SPEC[index].transfer_function;
        if (index == 0) {
            if (buffer->GetBool()) {
                index = buffer->GetUInt();
                if (index > 3) {
                    log_warn("Unknown color primaries index %" PRIu64 "\n", index);
                    return false;
                }
                sequence_header->source_params.color_primaries = (uint8_t)index;
            }
            if (buffer->GetBool()) {
                index = buffer->GetUInt();
                if (index > 3) {
                    log_warn("Unknown color matrix index %" PRIu64 "\n", index);
                    return false;
                }
                sequence_header->source_params.color_matrix = (uint8_t)index;
            }
            if (buffer->GetBool()) {
                index = buffer->GetUInt();
                if (index > 3) {
                    log_warn("Unknown transfer function index %" PRIu64 "\n", index);
                    return false;
                }
                sequence_header->source_params.transfer_function = (uint8_t)index;
            }
        }
    }

    return true;
}

void VC2EssenceParser::SetCodingParameters(SequenceHeader *sequence_header)
{
    sequence_header->coding_params.luma_width = sequence_header->source_params.frame_width;
    sequence_header->coding_params.luma_height = sequence_header->source_params.frame_height;
    sequence_header->coding_params.color_diff_width = sequence_header->coding_params.luma_width;
    sequence_header->coding_params.color_diff_height = sequence_header->coding_params.luma_height;
    if (sequence_header->source_params.color_diff_format_index == 1) {
        sequence_header->coding_params.color_diff_width /= 2;
    } else if (sequence_header->source_params.color_diff_format_index == 2) {
        sequence_header->coding_params.color_diff_width /= 2;
        sequence_header->coding_params.color_diff_height /= 2;
    }
    if (sequence_header->picture_coding_mode == 1) {
        sequence_header->coding_params.luma_height /= 2;
        sequence_header->coding_params.color_diff_height /= 2;
    }

    sequence_header->coding_params.luma_depth = intlog2(sequence_header->source_params.luma_excursion + 1);
    sequence_header->coding_params.color_diff_depth = intlog2(sequence_header->source_params.color_diff_excursion + 1);
}

void VC2EssenceParser::ResetFrameSize()
{
    mParseState = PARSE_INFO_STATE;
    mOffset = 0;
    memset(&mCurrentParseInfo, 0, sizeof(mCurrentParseInfo));
    mParseInfos.clear();
    mPictureCount = 0;
    mSearchState = 0;
    mSearchCount = 0;
}

void VC2EssenceParser::ResetFrameInfo()
{
    mPictureCount = 0;
    mFrameHasSequenceHeader = false;
    memset(&mPictureHeader1, 0, sizeof(mPictureHeader1));
    memset(&mPictureHeader2, 0, sizeof(mPictureHeader2));
}
