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
#include <vector>

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/BitBuffer.h>
#include <bmx/EssenceType.h>

#define AVCI_HEADER_SIZE    512


namespace bmx
{


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

typedef struct
{
    uint8_t type;
    const unsigned char *data;
    uint32_t size;
} NALReference;


class AVCGetBitBuffer : public GetBitBuffer
{
public:
    AVCGetBitBuffer(const unsigned char *data, uint32_t data_size);

    void GetF(uint8_t num_bits, uint64_t *value);
    void GetU(uint8_t num_bits, uint8_t *value);
    void GetU(uint8_t num_bits, uint64_t *value);
    void GetII(uint8_t num_bits, int64_t *value);
    void GetUE(uint64_t *value);
    void GetUE(uint8_t *value, uint8_t max_value);
    void GetSE(int64_t *value);
    void GetSE(int32_t *value);

    bool MoreRBSPData();

    bool NextBits(uint8_t num_bits, uint64_t *next_value);

    void SkipRBSPBytes(uint32_t count);

private:
    void GetRBSPBits(uint8_t num_bits, uint64_t *value);
};


class ParamSetData
{
public:
    ParamSetData();
    ParamSetData(const unsigned char *data_, uint32_t size_);
    ~ParamSetData();

    void Update(const unsigned char *data_, uint32_t size_);

public:
    bool is_constant;
    unsigned char *data;
    uint32_t size;
};


class POCState
{
public:
    POCState();

    void Reset();

public:
    bool prev_have_mmco_5;
    bool prev_bottom_field_flag;
    int32_t prev_pic_order_cnt_msb;
    int32_t prev_pic_order_cnt_lsb;
    int32_t prev_top_field_order_cnt;
    int32_t prev_frame_num;
    int32_t prev_frame_num_offset;
};


class AVCEssenceParser : public EssenceParser
{
public:
    AVCEssenceParser();
    virtual ~AVCEssenceParser();

    void SetSPS(const unsigned char *data, uint32_t size);
    void SetPPS(const unsigned char *data, uint32_t size);

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);
    void DecodePOC(POCState *poc_state, int32_t *pic_order_cnt);

    void ParseNALUnits(const unsigned char *data, uint32_t size, std::vector<NALReference> *nals);

    bool CheckFrameHasAVCIHeader(const unsigned char *data, uint32_t data_size);

public:
    uint8_t GetProfile() const            { return mProfile; }
    uint8_t GetProfileConstraint() const  { return mProfileConstraint; }
    uint8_t GetLevel() const              { return mLevel; }
    uint32_t GetMaxBitRate() const        { return mMaxBitRate; }
    uint8_t GetMaxNumRefFrames() const    { return mMaxNumRefFrames; }
    bool HaveFrameRate() const            { return mFrameRate.numerator > 0; }
    bool IsFixedFrameRate() const         { return mFixedFrameRate; }
    Rational GetFrameRate() const         { return mFrameRate; }
    uint32_t GetStoredWidth() const       { return mStoredWidth; }
    uint32_t GetStoredHeight() const      { return mStoredHeight; }
    uint32_t GetDisplayWidth() const      { return mDisplayWidth; }
    uint32_t GetDisplayHeight() const     { return mDisplayHeight; }
    uint32_t GetDisplayXOffset() const    { return mDisplayXOffset; }
    uint32_t GetDisplayYOffset() const    { return mDisplayYOffset; }
    Rational GetSampleAspectRatio() const { return mSampleAspectRatio; }
    uint32_t GetComponentDepth() const    { return mComponentDepth; }
    uint8_t GetChromaFormat() const       { return mChromaFormat; }
    uint8_t GetChromaLocation() const     { return mChromaLocation; }
    uint8_t GetVideoFormat() const        { return mVideoFormat; }
    uint8_t GetColorPrimaries() const     { return mColorPrimaries; }
    uint8_t GetTransferCharacteristics() const { return mTransferCharacteristics; }
    uint8_t GetMatrixCoefficients() const { return mMatrixCoefficients; }
    MPEGFrameType GetFrameType() const    { return mFrameType; }
    bool IsIDRFrame() const               { return mIsIDRFrame; }
    bool IsFrameMBSOnly() const           { return mFrameMBSOnlyFlag; }
    bool IsMBAdaptiveFFEncoding() const   { return mMBAdaptiveFFEncoding; }
    bool IsFieldPicture() const           { return mFieldPicture; }
    bool IsBottomField() const            { return mBottomField; }
    uint64_t GetFrameNum() const          { return mFrameNum; }

    bool FrameHasActiveSPS() const        { return mFrameHasActiveSPS; }
    bool FrameHasActivePPS() const        { return mFrameHasActivePPS; }
    bool IsActiveSPSDataConstant() const;
    bool IsActivePPSDataConstant() const;

    EssenceType GetEssenceType() const;
    EssenceType GetAVCIEssenceType(uint32_t data_size, bool is_interlaced, bool is_progressive) const;

private:
    typedef struct
    {
        uint8_t profile_idc;
        uint8_t constraint_flags;
        uint8_t level_idc;
        uint8_t seq_parameter_set_id;
        uint8_t chroma_format_idc;
        uint8_t separate_colour_plane_flag;
        uint8_t chroma_array_type; // derived from chroma_format_idc and separate_colour_plane_flag
        uint8_t bit_depth_luma_minus8;
        uint8_t bit_depth_chroma_minus8;
        uint64_t log2_max_frame_num_minus4;
        uint64_t pic_order_cnt_type;
        uint64_t log2_max_pic_order_cnt_lsb_minus4;
        uint8_t num_ref_frames_in_pic_order_cnt_cycle;
        // offset_for_ref_frame is held in SPSExtra
        int64_t expected_delta_per_pic_order_cnt_cycle; // derived from offset_for_ref_frame and num_ref_frames_in_pic_order_cnt_cycle
        uint8_t delta_pic_order_always_zero_flag;
        int64_t offset_for_non_ref_pic;
        int64_t offset_for_top_to_bottom_field;
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
        uint8_t video_signal_type_present_flag;
        uint8_t video_format;
        uint8_t video_full_range_flag;
        uint8_t colour_description_present_flag;
        uint8_t colour_primaries;
        uint8_t transfer_characteristics;
        uint8_t matrix_coefficients;
        uint8_t chroma_sample_loc_type_top_field;
        uint8_t chroma_sample_loc_type_bottom_field;
        uint8_t timing_info_present_flag;
        uint64_t num_units_in_tick;
        uint64_t time_scale;
        uint8_t fixed_frame_rate_flag;
        uint8_t bit_rate_scale;
        bool have_bit_rate_value_minus1;
        uint64_t bit_rate_value_minus1_schedselidx0;
        uint8_t pic_struct_present_flag;
    } SPS;

    class SPSExtra
    {
    public:
        SPSExtra();
        ~SPSExtra();

        uint8_t num_ref_frames_in_pic_order_cnt_cycle;
        int32_t *offset_for_ref_frame;
    };

    typedef struct
    {
        uint8_t pic_parameter_set_id;
        uint8_t seq_parameter_set_id;
        uint8_t bottom_field_pic_order_in_frame_present_flag;
        uint64_t num_ref_idx_l0_default_active_minus1;
        uint64_t num_ref_idx_l1_default_active_minus1;
        uint8_t weighted_pred_flag;
        uint8_t weighted_bipred_idc;
        uint8_t redundant_pic_cnt_present_flag;
    } PPS;

    typedef struct
    {
        uint8_t nal_ref_idc;    // derived from nal_unit_byte
        uint8_t idr_pic_flag;   // derived from nal_unit_byte
        uint8_t nal_unit_type;  // derived from nal_unit_byte

        uint64_t slice_type;
        uint8_t pic_parameter_set_id;
        uint64_t frame_num;
        uint8_t field_pic_flag;
        uint8_t bottom_field_flag;
        uint64_t idr_pic_id;
        uint64_t pic_order_cnt_lsb;
        int64_t delta_pic_order_cnt_bottom;
        int64_t delta_pic_order_cnt_0;
        int64_t delta_pic_order_cnt_1;
        uint64_t redundant_pic_cnt;

        // derived
        uint8_t have_mmco_5;
    } SliceHeader;

private:
    uint32_t NextStartCodePrefix(const unsigned char *data, uint32_t size);
    uint32_t CompletePSSize(const unsigned char *ps_start, const unsigned char *ps_max_end);

    bool ParseSPS(const unsigned char *data, uint32_t data_size, SPS *sps, SPSExtra **sps_extra);
    bool ParsePPS(const unsigned char *data, uint32_t data_size, PPS *pps);
    bool ParseSEI(const unsigned char *data, uint32_t data_size);
    bool ParseSliceHeader(const unsigned char *data, uint32_t data_size, uint8_t nal_unit_byte,
                          SliceHeader *slice_header, bool *unknown_param_sets = 0);
    void ParseHRDParameters(AVCGetBitBuffer &buffer, SPS *sps);

    void DecodePOCType0(POCState *poc_state, int32_t *top_field_order_cnt, int32_t *bottom_field_order_cnt);
    void DecodePOCType1(POCState *poc_state, int32_t *top_field_order_cnt, int32_t *bottom_field_order_cnt);
    void DecodePOCType2(POCState *poc_state, int32_t *top_field_order_cnt, int32_t *bottom_field_order_cnt);

    bool GetParameterSets(uint8_t pic_parameter_set_id, const SPS **sps, const PPS **pps);

    void SetSampleAspectRatio(const SPS *sps);

    void SetSPS(uint8_t id, SPS *sps, SPSExtra *sps_extra);
    void SetPPS(uint8_t id, PPS *pps);
    void SetSPSData(uint8_t id, const unsigned char *data, uint32_t size);
    void SetPPSData(uint8_t id, const unsigned char *data, uint32_t size);

    void ResetFrameSize();
    void ResetFrameInfo();

private:
    bool mHavePrimPicSliceHeader;
    SliceHeader mPrimPicSliceHeader;
    uint32_t mOffset;
    uint8_t mActiveSPSId;
    uint8_t mActivePPSId;
    bool mFrameHasActiveSPS;
    bool mFrameHasActivePPS;
    std::map<uint8_t, SPS> mSPS;
    std::map<uint8_t, SPSExtra*> mSPSExtra;
    std::map<uint8_t, PPS> mPPS;
    std::map<uint8_t, ParamSetData*> mSPSData;
    std::map<uint8_t, ParamSetData*> mPPSData;
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
    uint32_t mComponentDepth;
    uint8_t mChromaFormat;
    uint8_t mChromaLocation;
    uint8_t mVideoFormat;
    uint8_t mColorPrimaries;
    uint8_t mTransferCharacteristics;
    uint8_t mMatrixCoefficients;
    bool mIsIDRFrame;
    MPEGFrameType mFrameType;
    bool mFrameMBSOnlyFlag;
    bool mMBAdaptiveFFEncoding;
    bool mFieldPicture;
    bool mBottomField;
    bool mHaveMMCO5;
    uint64_t mFrameNum;
};


};



#endif

