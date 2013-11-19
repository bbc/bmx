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

#ifndef BMX_AVC_ESSENCE_PARSER_H_
#define BMX_AVC_ESSENCE_PARSER_H_

#include <map>

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/BitBuffer.h>
#include <bmx/EssenceType.h>


namespace bmx
{


class AVCGetBitBuffer : public GetBitBuffer
{
public:
    AVCGetBitBuffer(const unsigned char *data, uint32_t data_size);

    void GetF(uint8_t num_bits, uint64_t *value);
    void GetU(uint8_t num_bits, uint8_t *value);
    void GetU(uint8_t num_bits, uint64_t *value);
    void GetII(uint8_t num_bits, int64_t *value);
    void GetUE(uint64_t *value);
    void GetSE(int64_t *value);

    bool MoreRBSPData();

    bool NextBits(uint8_t num_bits, uint64_t *next_value);

    void ResetPos(uint32_t pos);
    void SkipRBSPBytes(uint32_t count);

private:
    void GetRBSPBits(uint8_t num_bits, uint64_t *value);
};


class AVCEssenceParser : public EssenceParser
{
public:
    AVCEssenceParser();
    virtual ~AVCEssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

public:
    bool FrameHasSPS() const              { return mFrameHasSPS; }
    bool FrameHasPPS() const              { return mFrameHasPPS; }
    uint8_t GetProfile() const            { return mProfile; }
    uint8_t GetProfileConstraint() const  { return mProfileConstraint; }
    uint8_t GetLevel() const              { return mLevel; }
    uint32_t GetMaxBitRate() const        { return mMaxBitRate; }
    uint8_t GetMaxNumRefFrames() const    { return mMaxNumRefFrames; }
    bool IsFixedFrameRate() const         { return mFixedFrameRate; }
    Rational GetFrameRate() const         { return mFrameRate; }
    uint32_t GetStoredWidth() const       { return mStoredWidth; }
    uint32_t GetStoredHeight() const      { return mStoredHeight; }
    uint32_t GetDisplayWidth() const      { return mDisplayWidth; }
    uint32_t GetDisplayHeight() const     { return mDisplayHeight; }
    uint32_t GetDisplayXOffset() const    { return mDisplayXOffset; }
    uint32_t GetDisplayYOffset() const    { return mDisplayYOffset; }
    Rational GetSampleAspectRatio() const { return mSampleAspectRatio; }
    MPEGFrameType GetFrameType() const    { return mFrameType; }
    bool IsIDRFrame() const               { return mIsIDRFrame; }

    EssenceType GetAVCIEssenceType(uint32_t data_size, bool is_interlaced, bool is_progressive) const;

private:
    typedef struct
    {
        uint8_t profile_idc;
        uint8_t constraint_flags;
        uint8_t level_idc;
        uint64_t seq_parameter_set_id;
        uint64_t chroma_format_idc;
        uint8_t separate_colour_plane_flag;
        uint8_t chroma_array_type; // derived from chroma_format_idc and separate_colour_plane_flag
        uint64_t bit_depth_luma_minus8;
        uint64_t bit_depth_chroma_minus8;
        uint64_t log2_max_frame_num_minus4;
        uint64_t pic_order_cnt_type;
        uint64_t log2_max_pic_order_cnt_lsb_minus4;
        uint8_t delta_pic_order_always_zero_flag;
        uint64_t max_num_ref_frames;
        uint64_t pic_width_in_mbs_minus1;
        uint64_t pic_height_in_map_units_minus1;
        uint8_t frame_mbs_only_flag;
        uint8_t mb_adaptive_frame_field_flag;
        uint64_t frame_crop_left_offset;
        uint64_t frame_crop_right_offset;
        uint64_t frame_crop_top_offset;
        uint64_t frame_crop_bottom_offset;
        uint8_t aspect_ratio_idc;
        uint64_t sar_width;
        uint64_t sar_height;
        uint8_t timing_info_present_flag;
        uint64_t num_units_in_tick;
        uint64_t time_scale;
        uint8_t fixed_frame_rate_flag;
        uint8_t bit_rate_scale;
        bool have_bit_rate_value_minus1;
        uint64_t bit_rate_value_minus1_schedselidx0;
        uint8_t pic_struct_present_flag;
    } SPS;

    typedef struct
    {
        uint64_t pic_parameter_set_id;
        uint64_t seq_parameter_set_id;
        uint8_t bottom_field_pic_order_in_frame_present_flag;
        uint8_t redundant_pic_cnt_present_flag;
    } PPS;

    typedef struct
    {
        uint8_t nal_ref_idc;  // derived from nal_unit_byte
        uint8_t idr_pic_flag; // derived from nal_unit_byte

        uint64_t slice_type;
        uint64_t pic_parameter_set_id;
        uint64_t frame_num;
        uint8_t field_pic_flag;
        uint8_t bottom_field_flag;
        uint64_t idr_pic_id;
        uint64_t pic_order_cnt_lsb;
        int64_t delta_pic_order_cnt_bottom;
        int64_t delta_pic_order_cnt_0;
        int64_t delta_pic_order_cnt_1;
        uint64_t redundant_pic_cnt;
    } SliceHeader;

private:
    uint32_t NextStartCodePrefix(const unsigned char *data, uint32_t size);

    bool ParseSPS(const unsigned char *data, uint32_t data_size, SPS *sps);
    bool ParsePPS(const unsigned char *data, uint32_t data_size, PPS *pps);
    bool ParseSEI(const unsigned char *data, uint32_t data_size);
    bool ParseSliceHeader(const unsigned char *data, uint32_t data_size, uint8_t nal_unit_byte,
                          SliceHeader *slice_header, bool *unknown_param_sets = 0);
    void ParseHRDParameters(AVCGetBitBuffer &buffer, SPS *sps);

    bool GetParameterSets(uint64_t pic_parameter_set_id, const SPS **sps, const PPS **pps);

    void SetSampleAspectRatio(const SPS *sps);

    void ResetFrameSize();
    void ResetFrameInfo();

private:
    std::map<uint64_t, SPS> mSPS;
    std::map<uint64_t, PPS> mPPS;
    bool mHavePrimPicSliceHeader;
    SliceHeader mPrimPicSliceHeader;
    uint32_t mOffset;

    bool mFrameHasSPS;
    bool mFrameHasPPS;
    uint8_t mProfile;
    uint8_t mProfileConstraint;
    uint8_t mLevel;
    uint32_t mMaxBitRate;
    uint8_t mMaxNumRefFrames;
    bool mFixedFrameRate;
    Rational mFrameRate;
    uint32_t mStoredWidth;
    uint32_t mStoredHeight;
    uint32_t mDisplayWidth;
    uint32_t mDisplayHeight;
    uint32_t mDisplayXOffset;
    uint32_t mDisplayYOffset;
    Rational mSampleAspectRatio;
    bool mIsIDRFrame;
    MPEGFrameType mFrameType;
    bool mFrameMBSOnlyFlag;
};


};



#endif

