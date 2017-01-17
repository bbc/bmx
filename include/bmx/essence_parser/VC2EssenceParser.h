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

#ifndef BMX_VC2_ESSENCE_PARSER_H_
#define BMX_VC2_ESSENCE_PARSER_H_

#include <vector>
#include <map>

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/BitBuffer.h>


#define VC2_PARSE_INFO_SIZE         13
#define VC2_PARSER_MIN_DATA_SIZE    VC2_PARSE_INFO_SIZE


namespace bmx
{


class VC2GetBitBuffer : public GetBitBuffer
{
public:
    VC2GetBitBuffer(const unsigned char *data, uint32_t data_size);

    void ByteAlign();

    uint8_t GetBit();
    bool GetBool() { return GetBit() == 1; }
    uint8_t GetByte();
    uint64_t GetUIntLit(uint32_t byte_count);
    uint64_t GetUInt();
};


class VC2EssenceParser : public EssenceParser
{
public:
    typedef struct
    {
      uint32_t frame_offset;
      uint8_t parse_code;
      uint32_t next_parse_offset;
      uint32_t prev_parse_offset;
    } ParseInfo;

    typedef struct
    {
        uint64_t frame_width;
        uint64_t frame_height;
        uint8_t color_diff_format_index;
        uint8_t source_sampling;
        bool top_field_first;
        int32_t frame_rate_numer;
        int32_t frame_rate_denom;
        uint64_t pixel_aspect_ratio_numer;
        uint64_t pixel_aspect_ratio_denom;
        uint64_t clean_width;
        uint64_t clean_height;
        uint64_t left_offset;
        uint64_t top_offset;
        uint64_t luma_offset;
        uint64_t luma_excursion;
        uint64_t color_diff_offset;
        uint64_t color_diff_excursion;
        uint8_t color_primaries;
        uint8_t color_matrix;
        uint8_t transfer_function;
    } VideoParameters;

    typedef struct {
        uint64_t luma_width;
        uint64_t luma_height;
        uint64_t color_diff_width;
        uint64_t color_diff_height;
        uint64_t luma_depth;
        uint64_t color_diff_depth;
    } CodingParameters;

    typedef struct
    {
        uint8_t major_version;
        uint8_t minor_version;
        uint8_t profile;
        uint8_t level;
        uint8_t base_video_format;
        VideoParameters source_params;
        uint8_t picture_coding_mode;
        CodingParameters coding_params;
    } SequenceHeader;

    typedef struct
    {
        uint32_t picture_number;
        uint8_t wavelet_index;
    } PictureHeader;

public:
    static bool IsSequenceHeader(uint8_t parse_code) { return parse_code == 0x00; }
    static bool IsPicture(uint8_t parse_code)        { return (parse_code & 0x08) == 0x08; }
    static bool IsAuxiliaryData(uint8_t parse_code)  { return (parse_code & 0xf8) == 0x20;}
    static bool IsPadding(uint8_t parse_code)        { return parse_code == 0x30; }
    static bool IsEndOfSequence(uint8_t parse_code)  { return parse_code == 0x10; }

public:
    VC2EssenceParser();
    virtual ~VC2EssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

    uint32_t ParseSequenceHeader(const unsigned char *data, uint32_t data_size, SequenceHeader *sequence_header);
    uint32_t ParsePictureHeader(const unsigned char *data, uint32_t data_size, PictureHeader *picture_header);

    void ResetFrameParse();

    void SetSequenceHeader(const SequenceHeader *sequence_header);

public:
    bool FrameHasSequenceHeader() const               { return mFrameHasSequenceHeader; }
    const SequenceHeader* GetSequenceHeader() const   { return &mSequenceHeader; }
    int GetPictureCount() const                       { return mPictureCount; }
    const PictureHeader* GetPictureHeader1() const    { return &mPictureHeader1; }
    const PictureHeader* GetPictureHeader2() const;

    Rational GetFrameRate() const;

    std::vector<ParseInfo> GetParseInfos() const      { return mParseInfos; }

private:
    typedef enum
    {
      PARSE_INFO_STATE,
      SEQUENCE_HEADER_STATE,
      PICTURE_STATE,
      NEXT_PARSE_INFO_STATE,
      SEARCH_PARSE_INFO_STATE,
    } ParseState;

private:
    uint32_t ParseParseInfo(VC2GetBitBuffer *buffer, ParseInfo *parse_info);
    uint32_t ParseSequenceHeader(VC2GetBitBuffer *buffer, SequenceHeader *sequence_header);
    uint32_t ParsePictureHeader(VC2GetBitBuffer *buffer, PictureHeader *picture_header);

    bool ParseSourceParameters(VC2GetBitBuffer *buffer, SequenceHeader *sequence_header);
    void SetCodingParameters(SequenceHeader *sequence_header);

    void ResetFrameSize();
    void ResetFrameInfo();

private:
    ParseState mParseState;
    uint32_t mOffset;
    ParseInfo mCurrentParseInfo;
    std::vector<ParseInfo> mParseInfos;
    int mPictureCount;
    uint32_t mSearchState;
    uint32_t mSearchCount;
    std::map<uint8_t, bool> mSecondaryParseInfoLocs;

    bool mFrameHasSequenceHeader;
    SequenceHeader mSequenceHeader;
    PictureHeader mPictureHeader1;
    PictureHeader mPictureHeader2;
};


};



#endif
