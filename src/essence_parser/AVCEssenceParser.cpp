/*
 * Copyright (C) 2013, British Broadcasting Corporation
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
#define __STDC_LIMIT_MACROS

#include <cstring>
#include <cmath>

#include <bmx/essence_parser/AVCEssenceParser.h>
#include <bmx/mxf_helper/AVCIMXFDescriptorHelper.h>
#include <bmx/BitBuffer.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define CEIL_DIVISION(a, b)     (((a) + (b) - 1) / (b))


typedef enum
{
    CODED_SLICE_NON_IDR_PICT        = 1,
    CODED_SLICE_DATA_PART_A         = 2,
    CODED_SLICE_DATA_PART_B         = 3,
    CODED_SLICE_DATA_PART_C         = 4,
    CODED_SLICE_IDR_PICT            = 5,
    SEI                             = 6,
    SEQUENCE_PARAMETER_SET          = 7,
    PICTURE_PARAMETER_SET           = 8,
    ACCESS_UNIT_DELIMITER           = 9,
    END_OF_SEQUENCE                 = 10,
    END_OF_STREAM                   = 11,
    FILLER                          = 12,
    SEQUENCE_PARAMETER_SET_EXT      = 13,
    PREFIX_NAL_UNIT                 = 14,
    SUBSET_SEQUENCE_PARAMETER_SET   = 15,
    CODED_SLICE_AUX_NO_PART         = 19,
    CODED_SLICE_EXT                 = 20,
} NALUnitType;



AVCGetBitBuffer::AVCGetBitBuffer(const unsigned char *data, uint32_t data_size)
: GetBitBuffer(data, data_size)
{
}

void AVCGetBitBuffer::GetF(uint8_t num_bits, uint64_t *value)
{
    GetRBSPBits(num_bits, value);
}

void AVCGetBitBuffer::GetU(uint8_t num_bits, uint64_t *value)
{
    GetRBSPBits(num_bits, value);
}

void AVCGetBitBuffer::GetU(uint8_t num_bits, uint8_t *value)
{
    if (num_bits > 8)
        throw false;

    uint64_t u64value;
    GetRBSPBits(num_bits, &u64value);
    *value = (uint8_t)u64value;
}

void AVCGetBitBuffer::GetII(uint8_t num_bits, int64_t *value)
{
    uint64_t uvalue;
    GetRBSPBits(num_bits, &uvalue);

    if (uvalue & (1ULL << (num_bits - 1)))
        *value = (UINT64_MAX << num_bits) | uvalue;
    else
        *value = uvalue;
}

void AVCGetBitBuffer::GetUE(uint64_t *value)
{
    uint64_t start_bit_pos = mBitPos;
    try
    {
        int8_t leading_zero_bits = -1;
        uint64_t b;
        uint64_t code_num;
        uint64_t temp;

        for (b = 0; !b && leading_zero_bits < 63; leading_zero_bits++)
            GetRBSPBits(1, &b);
        if (!b) {
            log_debug("Exp-Golumb size >= %d not supported\n", leading_zero_bits);
            throw false;
        }

        if (leading_zero_bits == 0) {
            *value = 0;
        } else {
            GetRBSPBits(leading_zero_bits, &temp);
            code_num = (1ULL << leading_zero_bits) - 1 + temp;
            *value = code_num;
        }
    }
    catch (...)
    {
        SetBitPos(start_bit_pos);
        throw;
    }
}

void AVCGetBitBuffer::GetSE(int64_t *value)
{
    uint64_t uvalue;
    GetUE(&uvalue);

    *value = ((uvalue & 1) ? 1 : -1) * CEIL_DIVISION(uvalue, 2);
}

bool AVCGetBitBuffer::MoreRBSPData()
{
    uint64_t bit_pos = mBitSize - 1;
    uint8_t b = 0;

    while (!b && bit_pos >= mBitPos) {
        b = mData[bit_pos / 8] & (0x80 >> (bit_pos % 8));
        bit_pos--;
    }

    return (b && bit_pos >= mBitPos);
}

bool AVCGetBitBuffer::NextBits(uint8_t num_bits, uint64_t *next_value)
{
    uint32_t orig_pos = mPos;
    uint64_t orig_bit_pos = mBitPos;
    bool result = true;

    try
    {
        GetRBSPBits(num_bits, next_value);
    }
    catch (...)
    {
        result = false;
    }

    mPos = orig_pos;
    mBitPos = orig_bit_pos;

    return result;
}

void AVCGetBitBuffer::ResetPos(uint32_t pos)
{
    mPos = pos;
    mBitPos = (uint64_t)pos << 3;
}

void AVCGetBitBuffer::SkipRBSPBytes(uint32_t count)
{
    uint32_t rbsp_count = 0;
    BMX_ASSERT((mBitPos & 7) == 0);
    while (rbsp_count < count && mPos < mSize) {
        if (mData[mPos] != 0x03 || mPos < 2 || mData[mPos - 1] != 0x00 || mData[mPos - 2] != 0x00)
            rbsp_count++;
        mPos++;
    }
    mBitPos = (uint64_t)mPos << 3;

    if (rbsp_count < count) {
        log_debug("Failed to skip %u RBSP bytes. Short by %u\n", rbsp_count, count - rbsp_count);
        throw false;
    }
}

void AVCGetBitBuffer::GetRBSPBits(uint8_t num_bits, uint64_t *value)
{
    if (num_bits > GetRemBitSize())
        throw false;

    // remove emulation prevention bytes (0x03 in a 0x000003 sequence) from NAL unit bitstream

    uint8_t check_num_bytes = (uint8_t)(((uint64_t)num_bits + (mBitPos & 7) + 7) >> 3);
    const unsigned char *bytes = &mData[mPos];

    uint8_t prev_bytes = 0;
    if (mPos >= 2 && (mBitPos & 7) == 0)
        prev_bytes = 2; // start check where current byte could be 0x03
    else if (mPos >= 1)
        prev_bytes = 1; // start check where the next byte could be 0x03
    bytes -= prev_bytes;
    check_num_bytes += prev_bytes;

    uint32_t state = 0xffffff;
    uint8_t i;
    for (i = 0; i < check_num_bytes; i++) {
        state = (state << 8) | bytes[i];
        if ((state & 0xffffff) == 0x000003)
            break;
    }

    uint64_t start_bit_pos = mBitPos;
    try
    {
        if (i < check_num_bytes) {
            // discard emulation prevention byte
            uint8_t before_bits, after_bits;
            uint64_t before_value, after_value;

            before_bits = ((i - prev_bytes) << 3) - (uint8_t)(mBitPos & 7);
            after_bits = num_bits - before_bits;

            GetBits(before_bits, &before_value);
            SetBitPos(mBitPos + 8);
            GetRBSPBits(after_bits, &after_value);

            *value = (before_value << after_bits) | after_value;
        } else if (!GetBits(num_bits, value)) {
            throw false;
        }
    }
    catch (...)
    {
        SetBitPos(start_bit_pos);
        throw;
    }
}



AVCEssenceParser::AVCEssenceParser()
{
    ResetFrameSize();
    ResetFrameInfo();
}

AVCEssenceParser::~AVCEssenceParser()
{
}

uint32_t AVCEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    // the access unit shall start with a zero_byte followed by start_code_prefix_one_3byte

    uint32_t offset = NextStartCodePrefix(data, data_size);
    if (offset == ESSENCE_PARSER_NULL_OFFSET)
        return ESSENCE_PARSER_NULL_OFFSET;

    if (offset == 0 || data[offset - 1] != 0x00) {
        log_warn("Missing zero_byte before start_code_prefix_one_3byte at access unit start\n");
        return ESSENCE_PARSER_NULL_OFFSET;
    }

    return offset - 1;
}

uint32_t AVCEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size < ESSENCE_PARSER_NULL_OFFSET);
    BMX_CHECK(mOffset < data_size);

    // The access unit shall contain a single primary coded picture.
    // Search for the first primary coded picture NAL unit.
    // The access unit ends when the next NAL is a AUD, SPS, PPS, SEI,
    //   has a NAL unit type >= 15 and <= 18 or a different primary coded picture VCL NAL
    // The frame ends at the byte before zero_byte + start_code_prefix_one_3byte of the next
    //   acess unit's NAL unit

    uint32_t frame_size;
    uint8_t nal_unit_byte;
    uint8_t nal_unit_type;
    uint32_t offset;
    SliceHeader slice_header;
    const SPS *sps;
    const PPS *pps;
    bool unknown_param_sets;
    while (mOffset < data_size) {
        offset = NextStartCodePrefix(&data[mOffset], data_size - mOffset);
        if (offset == ESSENCE_PARSER_NULL_OFFSET)
            return ESSENCE_PARSER_NULL_OFFSET;
        offset = mOffset + offset;
        if (offset + 3 >= data_size)
            return ESSENCE_PARSER_NULL_OFFSET;

        nal_unit_byte = data[offset + 3];
        nal_unit_type = nal_unit_byte & 0x1f;

        if (nal_unit_type == CODED_SLICE_NON_IDR_PICT ||
            nal_unit_type == CODED_SLICE_DATA_PART_A ||
            nal_unit_type == CODED_SLICE_IDR_PICT)
        {
            if (offset + 4 >= data_size)
                return ESSENCE_PARSER_NULL_OFFSET;

            if (!ParseSliceHeader(&data[offset + 4], data_size - (offset + 4), nal_unit_byte,
                                  &slice_header, &unknown_param_sets))
            {
                if (unknown_param_sets) {
                    ResetFrameSize();
                    return ESSENCE_PARSER_NULL_FRAME_SIZE;
                }
                return ESSENCE_PARSER_NULL_OFFSET;
            }
            GetParameterSets(slice_header.pic_parameter_set_id, &sps, &pps);

            if (mHavePrimPicSliceHeader) {
                if (slice_header.redundant_pic_cnt == 0 &&
                        (slice_header.pic_parameter_set_id != mPrimPicSliceHeader.pic_parameter_set_id ||
                         slice_header.frame_num != mPrimPicSliceHeader.frame_num ||
                         slice_header.field_pic_flag != mPrimPicSliceHeader.field_pic_flag ||
                         (slice_header.field_pic_flag &&
                             slice_header.bottom_field_flag != mPrimPicSliceHeader.bottom_field_flag) ||
                         (slice_header.nal_ref_idc != mPrimPicSliceHeader.nal_ref_idc &&
                             (slice_header.nal_ref_idc != 0 || mPrimPicSliceHeader.nal_ref_idc != 0)) ||
                         slice_header.idr_pic_flag != mPrimPicSliceHeader.idr_pic_flag ||
                         (slice_header.idr_pic_flag == 1 &&
                             slice_header.idr_pic_id != mPrimPicSliceHeader.idr_pic_id) ||
                         (sps->pic_order_cnt_type == 0 &&
                             (slice_header.pic_order_cnt_lsb != mPrimPicSliceHeader.pic_order_cnt_lsb ||
                              slice_header.delta_pic_order_cnt_bottom != mPrimPicSliceHeader.delta_pic_order_cnt_bottom)) ||
                         (sps->pic_order_cnt_type == 1 &&
                             (slice_header.delta_pic_order_cnt_0 != mPrimPicSliceHeader.delta_pic_order_cnt_0 ||
                              slice_header.delta_pic_order_cnt_1 != mPrimPicSliceHeader.delta_pic_order_cnt_1))))
                {
                    if (offset == 0 || data[offset - 1] != 0) {
                        log_warn("Missing zero_byte before start_code_prefix_one_3byte at access unit start\n");
                        ResetFrameSize();
                        return ESSENCE_PARSER_NULL_FRAME_SIZE;
                    }
                    frame_size = offset - 1;
                    ResetFrameSize();
                    return frame_size;
                }
            } else {
                if (slice_header.redundant_pic_cnt != 0) {
                    log_warn("First VCL NAL unit is not for the primary picture (redundant_pic_cnt != 0)\n");
                    ResetFrameSize();
                    return ESSENCE_PARSER_NULL_FRAME_SIZE;
                }
                mHavePrimPicSliceHeader = true;
                mPrimPicSliceHeader = slice_header;
            }
        }
        else if (mHavePrimPicSliceHeader &&
                    (nal_unit_type == ACCESS_UNIT_DELIMITER ||
                     nal_unit_type == SEQUENCE_PARAMETER_SET ||
                     nal_unit_type == PICTURE_PARAMETER_SET ||
                     nal_unit_type == SEI ||
                     (nal_unit_type >= 15 && nal_unit_type <= 18)))
        {
            if (offset == 0 || data[offset - 1] != 0) {
                log_warn("Missing zero_byte before start_code_prefix_one_3byte at access unit start\n");
                ResetFrameSize();
                return ESSENCE_PARSER_NULL_FRAME_SIZE;
            }
            frame_size = offset - 1;
            ResetFrameSize();
            return frame_size;
        }
        else if (nal_unit_type == SEQUENCE_PARAMETER_SET)
        {
            if (offset + 4 >= data_size)
                return ESSENCE_PARSER_NULL_OFFSET;

            SPS sps;
            if (!ParseSPS(&data[offset + 4], data_size - (offset + 4), &sps))
                return ESSENCE_PARSER_NULL_OFFSET;
            mSPS[sps.seq_parameter_set_id] = sps;
        }
        else if (nal_unit_type == PICTURE_PARAMETER_SET)
        {
            if (offset + 4 >= data_size)
                return ESSENCE_PARSER_NULL_OFFSET;

            PPS pps;
            if (!ParsePPS(&data[offset + 4], data_size - (offset + 4), &pps))
                return ESSENCE_PARSER_NULL_OFFSET;
            mPPS[pps.pic_parameter_set_id] = pps;
        }

        mOffset = offset + 3;
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

void AVCEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    ResetFrameInfo();

    bool field_pic_flag = false;
    uint8_t nal_unit_byte;
    uint8_t nal_unit_type;
    uint32_t offset = 0;
    uint32_t next_offset = 0;
    bool have_first_slice = false;
    SliceHeader slice_header;
    const SPS *sps = 0;
    const PPS *pps = 0;
    while (mOffset < data_size) {
        next_offset = NextStartCodePrefix(&data[offset], data_size - offset);
        if (next_offset == ESSENCE_PARSER_NULL_OFFSET)
            break;
        offset = offset + next_offset;
        if (offset + 3 >= data_size)
            break;

        nal_unit_byte = data[offset + 3];
        nal_unit_type = nal_unit_byte & 0x1f;

        if (nal_unit_type == CODED_SLICE_NON_IDR_PICT ||
            nal_unit_type == CODED_SLICE_DATA_PART_A ||
            nal_unit_type == CODED_SLICE_IDR_PICT)
        {
            if (offset + 4 >= data_size)
                BMX_EXCEPTION(("Insufficient VCL NAL unit data"));

            if (!ParseSliceHeader(&data[offset + 4], data_size - (offset + 4), nal_unit_byte, &slice_header))
                BMX_EXCEPTION(("Failed to parse VCL slice header"));
            if (!have_first_slice) {
                if (slice_header.redundant_pic_cnt != 0)
                    BMX_EXCEPTION(("Missing primary picture VCL NAL unit (first VCL redundant_pic_cnt != 0)"));
                if (!GetParameterSets(slice_header.pic_parameter_set_id, &sps, &pps))
                    BMX_EXCEPTION(("Failed to get SPS+PPS for slice"));
                field_pic_flag = slice_header.field_pic_flag;
                have_first_slice = true;
            }

            if (slice_header.redundant_pic_cnt == 0) {
                MPEGFrameType slice_frame_type = UNKNOWN_FRAME_TYPE;
                if (slice_header.slice_type > 9)
                    BMX_EXCEPTION(("Unknown slice_type %"PRIu64, slice_header.slice_type));
                if (slice_header.slice_type == 2 || slice_header.slice_type == 4 ||
                    slice_header.slice_type == 7 || slice_header.slice_type == 9)
                {
                    slice_frame_type = I_FRAME;
                }
                else if (slice_header.slice_type == 0 || slice_header.slice_type == 3 ||
                         slice_header.slice_type == 5 || slice_header.slice_type == 8)
                {
                    slice_frame_type = P_FRAME;
                }
                else
                {
                    slice_frame_type = B_FRAME;
                }

                if (mFrameType == UNKNOWN_FRAME_TYPE) {
                    mFrameType  = slice_frame_type;
                    mIsIDRFrame = slice_header.idr_pic_flag;
                } else if (mFrameType == I_FRAME) {
                    if (slice_frame_type != I_FRAME)
                        mFrameType = slice_frame_type;
                } else if (mFrameType == P_FRAME) {
                    if (slice_frame_type == B_FRAME)
                        mFrameType = slice_frame_type;
                }

                if (mFrameType != I_FRAME || !slice_header.idr_pic_flag)
                    mIsIDRFrame = false;

                if (mFrameType == B_FRAME ||      // no more changes to frame type will occur
                    slice_header.slice_type >= 5) // section 7.4.3 requires all slices to have the same
                                                  // slice_type (possibly slice_type - 5) when value is 5..9
                {
                    break;
                }
            } else {
                // reached end of primary picture slices
                break;
            }
        }
        else if (nal_unit_type == SEQUENCE_PARAMETER_SET)
        {
            if (offset + 4 >= data_size)
                BMX_EXCEPTION(("Insufficient SPS NAL unit data"));

            SPS sps;
            if (!ParseSPS(&data[offset + 4], data_size - (offset + 4), &sps))
                BMX_EXCEPTION(("Failed to parse SPS"));
            mSPS[sps.seq_parameter_set_id] = sps;

            mFrameHasSPS = true;
        }
        else if (nal_unit_type == PICTURE_PARAMETER_SET)
        {
            if (offset + 4 >= data_size)
                BMX_EXCEPTION(("Insufficient PPS NAL unit data"));

            PPS pps;
            if (!ParsePPS(&data[offset + 4], data_size - (offset + 4), &pps))
                BMX_EXCEPTION(("Failed to parse PPS"));
            mPPS[pps.pic_parameter_set_id] = pps;

            mFrameHasPPS = true;
        }

        offset += 3;
    }
    if (!sps || !pps)
        BMX_EXCEPTION(("Failed to parse AVC frame information"));


    mProfile           = sps->profile_idc;
    mProfileConstraint = sps->constraint_flags;
    mLevel             = sps->level_idc;

    if (sps->have_bit_rate_value_minus1)
        mMaxBitRate = (uint32_t)((sps->bit_rate_value_minus1_schedselidx0 + 1) * pow(2.0, 6.0 + sps->bit_rate_scale) + 0.5);
    // else could be inferred according to section E.2.2, but is simply set to 0 here

    if (sps->timing_info_present_flag) {
        if (sps->time_scale == 0 || sps->num_units_in_tick == 0) {
            BMX_EXCEPTION(("Invalid timing info: time_scale=%"PRIu64", num_units_in_tick=%"PRIu64,
                           sps->time_scale, sps->num_units_in_tick));
        } else {
            // TODO: implement D.2.2 of H.264 spec. and explanation surrounding Table E-6
            uint64_t delta_tfi_divisor = 2;
            if (field_pic_flag)
                delta_tfi_divisor = 1;

            mFrameRate.numerator   = (int32_t)sps->time_scale;
            mFrameRate.denominator = (int32_t)(sps->num_units_in_tick * delta_tfi_divisor);
            mFrameRate = normalize_rate(mFrameRate);
        }
        mFixedFrameRate = (sps->fixed_frame_rate_flag != 0);
    }

    if (sps->max_num_ref_frames > 0xff)
        mMaxNumRefFrames = 0xff;
    else
        mMaxNumRefFrames = (uint8_t)sps->max_num_ref_frames;

    mStoredWidth  = (uint32_t)((sps->pic_width_in_mbs_minus1 + 1) * 16);
    mStoredHeight = (uint32_t)((sps->pic_height_in_map_units_minus1 + 1) * 16 * (2 - sps->frame_mbs_only_flag));

    uint64_t crop_unit_x, crop_unit_y;
    if (sps->chroma_array_type == 0) {
        crop_unit_x = 1;
        crop_unit_y = 2 - sps->frame_mbs_only_flag;
    } else {
        uint64_t sub_width_c, sub_height_c;
        if (sps->chroma_array_type == 1) {
            sub_width_c  = 2;
            sub_height_c = 2;
        } else if (sps->chroma_array_type == 2) {
            sub_width_c  = 2;
            sub_height_c = 1;
        } else { // sps->chroma_array_type == 3
            sub_width_c  = 1;
            sub_height_c = 1;
        }
        crop_unit_x = sub_width_c;
        crop_unit_y = sub_height_c * (2 - sps->frame_mbs_only_flag);
    }
    mDisplayXOffset = (uint32_t)(crop_unit_x * sps->frame_crop_left_offset);
    mDisplayYOffset = (uint32_t)(crop_unit_y * sps->frame_crop_top_offset);
    mDisplayWidth   = (uint32_t)(mStoredWidth  - crop_unit_x * sps->frame_crop_right_offset  - mDisplayXOffset);
    mDisplayHeight  = (uint32_t)(mStoredHeight - crop_unit_y * sps->frame_crop_bottom_offset - mDisplayYOffset);

    SetSampleAspectRatio(sps);

    mFrameMBSOnlyFlag = sps->frame_mbs_only_flag;
}

EssenceType AVCEssenceParser::GetAVCIEssenceType(uint32_t data_size, bool is_interlaced, bool is_progressive) const
{
    if (!mIsIDRFrame || !mFixedFrameRate)
        return UNKNOWN_ESSENCE_TYPE;

    EssenceType essence_type = UNKNOWN_ESSENCE_TYPE;

    if (mProfile == 110 && (mProfileConstraint & 0x10)) {
        if (mStoredWidth == 1440 && mStoredHeight == 1088) {
            if (mLevel == 42 &&
                (mFrameRate == FRAME_RATE_5994 ||
                 mFrameRate == FRAME_RATE_50))
            {
                essence_type = AVCI50_1080P;
            }
            else if (mLevel == 40)
            {
                if (mFrameRate == FRAME_RATE_23976) {
                    essence_type = AVCI50_1080P;
                } else if (mFrameRate == FRAME_RATE_2997 || mFrameRate == FRAME_RATE_25) {
                    if (is_progressive)
                        essence_type = AVCI50_1080P;
                    else if (is_interlaced)
                        essence_type = AVCI50_1080I;
                    // guessing interlaced/progressive using RP2027 recommended frame_mbs_only_flag value
                    else if (mFrameMBSOnlyFlag)
                        essence_type = AVCI50_1080P;
                    else
                        essence_type = AVCI50_1080I;
                }
            }
        } else if (mStoredWidth == 960 && mStoredHeight == 720) {
            if (mLevel == 32 &&
                (mFrameRate == FRAME_RATE_5994 ||
                 mFrameRate == FRAME_RATE_50 ||
                 mFrameRate == FRAME_RATE_2997 ||
                 mFrameRate == FRAME_RATE_25 ||
                 mFrameRate == FRAME_RATE_23976))
            {
                essence_type = AVCI50_720P;
            }
        }
    } else if (mProfile == 122 && (mProfileConstraint & 0x10)) {
        if (mStoredWidth == 1920 && mStoredHeight == 1088) {
            if (mLevel == 50 &&
                (mFrameRate == FRAME_RATE_5994 ||
                 mFrameRate == FRAME_RATE_50))
            {
                essence_type = AVCI200_1080P;
            }
            else if (mLevel == 42 &&
                (mFrameRate == FRAME_RATE_5994 ||
                 mFrameRate == FRAME_RATE_50))
            {
                essence_type = AVCI100_1080P;
            }
            else if (mLevel == 41)
            {
                // level 4.1 is used by class 100 and class 200, lets says if
                // data size is much too big for class 100 its class 200
                bool class200 = false;
                if (data_size > 700000)
                    class200 = true;

                if (mFrameRate == FRAME_RATE_23976) {
                    essence_type = class200 ? AVCI200_1080P : AVCI100_1080P;
                } else if (mFrameRate == FRAME_RATE_2997 || mFrameRate == FRAME_RATE_25) {
                    if (is_progressive)
                        essence_type = class200 ? AVCI200_1080P : AVCI100_1080P;
                    else if (is_interlaced)
                        essence_type = class200 ? AVCI200_1080I : AVCI100_1080I;
                    // guessing interlaced/progressive using RP2027 recommended frame_mbs_only_flag value
                    else if (mFrameMBSOnlyFlag)
                        essence_type = class200 ? AVCI200_1080P : AVCI100_1080P;
                    else
                        essence_type = class200 ? AVCI200_1080I : AVCI100_1080I;
                }
            }
        } else if (mStoredWidth == 1280 && mStoredHeight == 720) {
            if (mLevel == 41 &&
                (mFrameRate == FRAME_RATE_5994 ||
                 mFrameRate == FRAME_RATE_50 ||
                 mFrameRate == FRAME_RATE_2997 ||
                 mFrameRate == FRAME_RATE_25 ||
                 mFrameRate == FRAME_RATE_23976))
            {

                // level 4.1 is used by class 100 and class 200, lets says if
                // data size is much too big for class 100 its class 200
                if (data_size > 375000)
                    essence_type = AVCI200_720P;
                else
                    essence_type = AVCI100_720P;
            }
        }
    }

    if (essence_type != UNKNOWN_ESSENCE_TYPE) {
        uint32_t frame_size = AVCIMXFDescriptorHelper::GetSampleSize(essence_type, mFrameRate);
        if (data_size != frame_size && data_size + 512 != frame_size)
            essence_type = UNKNOWN_ESSENCE_TYPE;
    }

    return essence_type;
}

uint32_t AVCEssenceParser::NextStartCodePrefix(const unsigned char *data, uint32_t size)
{
    uint32_t offset;
    uint32_t state = 0xffffffff;
    for (offset = 0; offset < size - 1; offset++) {
        state <<= 8;
        state |= data[offset];
        if ((state & 0x00ffffff) == 0x000001)
            return offset - 2;
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

bool AVCEssenceParser::ParseSPS(const unsigned char *data, uint32_t data_size, SPS *sps)
{
    try
    {
        AVCGetBitBuffer buffer(data, data_size);
        uint64_t temp;
        int64_t stemp;
        uint8_t nal_hrd_parameters_present_flag, vcl_hrd_parameters_present_flag;

        memset(sps, 0, sizeof(*sps));
        sps->chroma_format_idc = 1;
        sps->chroma_array_type = 1;

        buffer.GetU(8, &sps->profile_idc);
        buffer.GetU(8, &sps->constraint_flags); // constraint_set0...set5_flag + reserved_zero_2bits
        buffer.GetU(8, &sps->level_idc);
        buffer.GetUE(&sps->seq_parameter_set_id);

        if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
            sps->profile_idc == 122 || sps->profile_idc == 244 || sps->profile_idc ==  44 ||
            sps->profile_idc ==  83 || sps->profile_idc ==  86 || sps->profile_idc == 118 ||
            sps->profile_idc == 128)
        {
            buffer.GetUE(&sps->chroma_format_idc);
            if (sps->chroma_format_idc == 3)
                buffer.GetU(1, &sps->separate_colour_plane_flag);
            if (sps->separate_colour_plane_flag == 0)
                sps->chroma_array_type = (uint8_t)sps->chroma_format_idc;
            else
                sps->chroma_array_type = 0;
            buffer.GetUE(&sps->bit_depth_luma_minus8);
            buffer.GetUE(&sps->bit_depth_chroma_minus8);
            buffer.GetU(1, &temp); // qpprime_y_zero_transform_bypass_flag
            buffer.GetU(1, &temp); // seq_scaling_matrix_present_flag
            if (temp) {
                uint8_t num_scaling_lists = ((sps->chroma_format_idc != 3) ? 8 : 12);
                uint8_t i;
                for (i = 0; i < num_scaling_lists; i++) {
                    buffer.GetU(1, &temp); // seq_scaling_list_present_flag
                    if (temp) {
                        uint8_t size = (i < 6 ? 4 * 4 : 8 * 8);
                        uint8_t last_scale = 8, next_scale = 8;
                        uint8_t j;
                        for (j = 0; j < size; j++) {
                            if (next_scale != 0) {
                                buffer.GetSE(&stemp);
                                next_scale = (uint8_t)((last_scale + stemp + 256) % 256);
                            }
                            last_scale = (next_scale == 0 ? last_scale : next_scale);
                        }
                    }
                }
            }
        }
        buffer.GetUE(&sps->log2_max_frame_num_minus4);
        buffer.GetUE(&sps->pic_order_cnt_type);
        if (sps->pic_order_cnt_type == 0) {
            buffer.GetUE(&sps->log2_max_pic_order_cnt_lsb_minus4);
        } else if (sps->pic_order_cnt_type == 1) {
            uint64_t num_ref_frames_in_pic_order_cnt_cycle;
            uint64_t i;

            buffer.GetU(1, &sps->delta_pic_order_always_zero_flag);
            buffer.GetSE(&stemp); // offset_for_non_ref_pic
            buffer.GetSE(&stemp); // offset_for_top_to_bottom_field
            buffer.GetUE(&num_ref_frames_in_pic_order_cnt_cycle);
            for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
                buffer.GetSE(&stemp); // offset_for_ref_frame
        }
        buffer.GetUE(&sps->max_num_ref_frames);
        buffer.GetU(1, &temp);  // gaps_in_frame_num_value_allowed_flag
        buffer.GetUE(&sps->pic_width_in_mbs_minus1);
        buffer.GetUE(&sps->pic_height_in_map_units_minus1);
        buffer.GetU(1, &sps->frame_mbs_only_flag);
        if (!sps->frame_mbs_only_flag)
            buffer.GetU(1, &sps->mb_adaptive_frame_field_flag);
        buffer.GetU(1, &temp); // direct_8x8_inference_flag
        buffer.GetU(1, &temp); // frame_cropping_flag
        if (temp) {
            buffer.GetUE(&sps->frame_crop_left_offset);
            buffer.GetUE(&sps->frame_crop_right_offset);
            buffer.GetUE(&sps->frame_crop_top_offset);
            buffer.GetUE(&sps->frame_crop_bottom_offset);
        }
        buffer.GetU(1, &temp); // vui_parameters_present_flag
        if (temp) {
            buffer.GetU(1, &temp); // aspect_ratio_present_flag
            if (temp) {
                buffer.GetU(8, &sps->aspect_ratio_idc);
                if (sps->aspect_ratio_idc == 255) { // 255 == extended SAR
                    buffer.GetU(16, &sps->sar_width);
                    buffer.GetU(16, &sps->sar_height);
                }
            }
            buffer.GetU(1, &temp); // overscan_info_present_flag
            if (temp)
                buffer.GetU(1, &temp); // overscan_appropriate_flag
            buffer.GetU(1, &temp); // video_signal_type_present_flag
            if (temp) {
                buffer.GetU(3, &temp); // video_format
                buffer.GetU(1, &temp); // video_full_range_flag
                buffer.GetU(1, &temp); // colour_description_present_flag
                if (temp) {
                    buffer.GetU(8, &temp); // colour_primaries
                    buffer.GetU(8, &temp); // transfer_characteristics
                    buffer.GetU(8, &temp); // matrix_coefficients
                }
            }
            buffer.GetU(1, &temp); // chroma_loc_info_present_flag
            if (temp) {
                buffer.GetUE(&temp); // chroma_sample_loc_type_top_field
                buffer.GetUE(&temp); // chroma_sample_loc_type_bottom_field
            }
            buffer.GetU(1, &sps->timing_info_present_flag);
            if (sps->timing_info_present_flag) {
                buffer.GetU(32, &sps->num_units_in_tick);
                buffer.GetU(32, &sps->time_scale);
                buffer.GetU(1, &sps->fixed_frame_rate_flag);
            }
            buffer.GetU(1, &nal_hrd_parameters_present_flag);
            if (nal_hrd_parameters_present_flag)
                ParseHRDParameters(buffer, sps);
            buffer.GetU(1, &vcl_hrd_parameters_present_flag);
            if (vcl_hrd_parameters_present_flag)
                ParseHRDParameters(buffer, sps);
            if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
                buffer.GetU(1, &temp); // low_delay_hrd_flag
            buffer.GetU(1, &sps->pic_struct_present_flag);
            // ignoring the rest
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool AVCEssenceParser::ParsePPS(const unsigned char *data, uint32_t data_size, PPS *pps)
{
    try
    {
        AVCGetBitBuffer buffer(data, data_size);
        uint64_t temp;
        int64_t stemp;
        uint64_t num_slice_groups_minus1;

        memset(pps, 0, sizeof(*pps));

        buffer.GetUE(&pps->pic_parameter_set_id);
        buffer.GetUE(&pps->seq_parameter_set_id);
        buffer.GetU(1, &temp); // entropy_coding_mode_flag
        buffer.GetU(1, &pps->bottom_field_pic_order_in_frame_present_flag);
        buffer.GetUE(&num_slice_groups_minus1);
        if (num_slice_groups_minus1 > 0) {
            uint64_t i;
            buffer.GetUE(&temp); // slice_group_map_type
            if (temp == 0) {
                for (i = 0; i <= num_slice_groups_minus1; i++)
                    buffer.GetUE(&temp); // run_length_minus1[]
            } else if (temp == 2) {
                for (i = 0; i < num_slice_groups_minus1; i++) {
                    buffer.GetUE(&temp); // top_left[]
                    buffer.GetUE(&temp); // bottom_right[]
                }
            } else if (temp == 3 || temp == 4 || temp == 5) {
                buffer.GetU(1, &temp);  // slice_group_change_direction_flag
                buffer.GetUE(&temp);    // slice_group_change_rate_minus1
            } else if (temp == 6) {
                uint64_t pic_size_in_map_units_minus1;
                buffer.GetUE(&pic_size_in_map_units_minus1);
                for (i = 0; i <= pic_size_in_map_units_minus1; i++) {
                    uint64_t value = pic_size_in_map_units_minus1 + 1;
                    uint8_t bits_required = 63;
                    while (bits_required && !(value & (1ULL << bits_required)))
                        bits_required--;
                    buffer.GetU(bits_required, &temp); // slice_group_id[]
                }
            }
        }
        buffer.GetUE(&temp);    // num_ref_idx_l0_default_active_minus1
        buffer.GetUE(&temp);    // num_ref_idx_l1_default_active_minus1
        buffer.GetU(1, &temp);  // weighted_pred_flag
        buffer.GetU(2, &temp);  // weighted_bipred_idc
        buffer.GetSE(&stemp);   // pic_init_qp_minus26
        buffer.GetSE(&stemp);   // pic_init_qs_minus26
        buffer.GetSE(&stemp);   // chroma_qp_index_offset
        buffer.GetU(1, &temp);  // deblocking_filter_control_present_flag
        buffer.GetU(1, &temp);  // constrained_intra_pred_flag
        buffer.GetU(1, &pps->redundant_pic_cnt_present_flag);
        // ignore the rest

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool AVCEssenceParser::ParseSEI(const unsigned char *data, uint32_t data_size)
{
    try
    {
        AVCGetBitBuffer buffer(data, data_size);

        do {
            uint64_t payload_type = 0;
            uint64_t payload_size = 0;
            uint64_t temp;

            while (buffer.NextBits(8, &temp) && temp == 0xff) {
                buffer.GetF(8, &temp);
                payload_type += 255;
            }
            buffer.GetU(8, &temp);
            payload_type += temp;

            while (buffer.NextBits(8, &temp) && temp == 0xff) {
                buffer.GetF(8, &temp);
                payload_size += 255;
            }
            buffer.GetU(8, &temp);
            payload_size += temp;


            BMX_ASSERT((buffer.GetBitPos() & 7) == 0);
            uint32_t start_pos = buffer.GetPos();

            // parse payloads here

            buffer.ResetPos(start_pos);
            buffer.SkipRBSPBytes((uint32_t)(payload_size * 8));
        } while (buffer.MoreRBSPData());

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool AVCEssenceParser::ParseSliceHeader(const unsigned char *data, uint32_t data_size, uint8_t nal_unit_byte,
                                        SliceHeader *slice_header, bool *unknown_param_sets)
{
    if (unknown_param_sets)
        *unknown_param_sets = false;

    try
    {
        AVCGetBitBuffer buffer(data, data_size);
        uint64_t temp;
        const SPS *sps;
        const PPS *pps;

        memset(slice_header, 0, sizeof(*slice_header));

        slice_header->nal_ref_idc = (nal_unit_byte >> 5) & 0x03;
        slice_header->idr_pic_flag = ((nal_unit_byte & 0x1f) == CODED_SLICE_IDR_PICT ? 1 : 0);

        buffer.GetUE(&temp); // first_mb_in_slice
        buffer.GetUE(&slice_header->slice_type);
        buffer.GetUE(&slice_header->pic_parameter_set_id);

        if (!GetParameterSets(slice_header->pic_parameter_set_id, &sps, &pps)) {
            if (unknown_param_sets)
                *unknown_param_sets = true;
            return false;
        }

        if (sps->separate_colour_plane_flag == 1)
            buffer.GetU(2, &temp); // colour_plane_id
        buffer.GetU((uint8_t)(sps->log2_max_frame_num_minus4 + 4), &slice_header->frame_num);
        if (!sps->frame_mbs_only_flag) {
            buffer.GetU(1, &slice_header->field_pic_flag);
            if (slice_header->field_pic_flag)
                buffer.GetU(1, &slice_header->bottom_field_flag);
        }
        if (slice_header->idr_pic_flag)
            buffer.GetUE(&slice_header->idr_pic_id);
        if (sps->pic_order_cnt_type == 0) {
            buffer.GetU((uint8_t)(sps->log2_max_pic_order_cnt_lsb_minus4 + 4), &slice_header->pic_order_cnt_lsb);
            if (pps->bottom_field_pic_order_in_frame_present_flag && !slice_header->field_pic_flag)
                buffer.GetSE(&slice_header->delta_pic_order_cnt_bottom);
        }
        if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
            buffer.GetSE(&slice_header->delta_pic_order_cnt_0);
            if (pps->bottom_field_pic_order_in_frame_present_flag && !slice_header->field_pic_flag)
                buffer.GetSE(&slice_header->delta_pic_order_cnt_1);
        }
        if (pps->redundant_pic_cnt_present_flag)
            buffer.GetUE(&slice_header->redundant_pic_cnt);
        // ignoring the rest of the slice header

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void AVCEssenceParser::ParseHRDParameters(AVCGetBitBuffer &buffer, SPS *sps)
{
    uint64_t temp;
    uint64_t cpb_cnt_minus1;
    uint64_t sched_sel_idx;

    buffer.GetUE(&cpb_cnt_minus1);
    buffer.GetU(4, &sps->bit_rate_scale);
    buffer.GetU(4, &temp); // cpb_size_scale
    for (sched_sel_idx = 0; sched_sel_idx <= cpb_cnt_minus1; sched_sel_idx++) {
        if (sched_sel_idx == 0) {
            buffer.GetUE(&sps->bit_rate_value_minus1_schedselidx0);
            sps->have_bit_rate_value_minus1 = true;
        } else {
            buffer.GetUE(&temp); // bit_rate_value_minus1
        }
        buffer.GetUE(&temp); // cpb_size_value_minus1
        buffer.GetU(1, &temp); // cbr_flag
    }
    buffer.GetU(5, &temp); // initial_cpb_removal_delay_length_minus1
    buffer.GetU(5, &temp); // cpb_removal_delay_length_minus1
    buffer.GetU(5, &temp); // dpb_removal_delay_length_minus1
    buffer.GetU(5, &temp); // time_offset_length
}

bool AVCEssenceParser::GetParameterSets(uint64_t pic_parameter_set_id, const SPS **sps, const PPS **pps)
{
    map<uint64_t, PPS>::const_iterator pps_iter = mPPS.find(pic_parameter_set_id);
    if (pps_iter == mPPS.end()) {
        log_warn("Missing PPS with id %"PRIu64" before VCL NAL unit\n", pic_parameter_set_id);
        return false;
    }
    map<uint64_t, SPS>::const_iterator sps_iter = mSPS.find(pps_iter->second.seq_parameter_set_id);
    if (sps_iter == mSPS.end()) {
        log_warn("Missing SPS with id %"PRIu64" reference from PPS with id %"PRIu64"\n",
                 pps_iter->second.seq_parameter_set_id, pic_parameter_set_id);
        return false;
    }

    *sps = &sps_iter->second;
    *pps = &pps_iter->second;

    return true;
}

void AVCEssenceParser::SetSampleAspectRatio(const SPS *sps)
{
    static const Rational aspect_ratios[] =
    {
        {  0,  1},
        {  1,  1},
        { 12, 11},
        { 10, 11},
        { 16, 11},
        { 40, 33},
        { 24, 11},
        { 20, 11},
        { 32, 11},
        { 80, 33},
        { 18, 11},
        { 15, 11},
        { 64, 33},
        {160, 99},
        {  4,  3},
        {  3,  2},
        {  2,  1},
    };

    if (sps->aspect_ratio_idc == 255) { // extended SAR
        mSampleAspectRatio.numerator   = (int32_t)sps->sar_width;
        mSampleAspectRatio.denominator = (int32_t)sps->sar_height;
    } else if (sps->aspect_ratio_idc >= BMX_ARRAY_SIZE(aspect_ratios)) {
        log_warn("Invalid aspect_ratio_idc %u\n", sps->aspect_ratio_idc);
        mSampleAspectRatio = ZERO_RATIONAL;
    } else {
        mSampleAspectRatio = aspect_ratios[sps->aspect_ratio_idc];
    }
}

void AVCEssenceParser::ResetFrameSize()
{
    mHavePrimPicSliceHeader = false;
    memset(&mPrimPicSliceHeader, 0, sizeof(mPrimPicSliceHeader));
    mOffset = 0;
}

void AVCEssenceParser::ResetFrameInfo()
{
    mFrameHasSPS = false;
    mFrameHasPPS = false;
    mProfile = 0;
    mProfileConstraint = 0;
    mLevel = 0;
    mMaxBitRate = 0;
    mMaxNumRefFrames = 0;
    mFixedFrameRate = true;
    mFrameRate = ZERO_RATIONAL;
    mStoredWidth = 0;
    mStoredHeight = 0;
    mDisplayWidth = 0;
    mDisplayHeight = 0;
    mDisplayXOffset = 0;
    mDisplayYOffset = 0;
    mSampleAspectRatio = ZERO_RATIONAL;
    mIsIDRFrame = false;
    mFrameType = UNKNOWN_FRAME_TYPE;
    mFrameMBSOnlyFlag = true;
}

