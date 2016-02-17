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

#include <bmx/essence_parser/MPEG2EssenceParser.h>
#include "EssenceParserUtils.h"
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define PICTURE_START_CODE      0x00000100
#define SEQUENCE_HEADER_CODE    0x000001b3
#define EXTENSION_START_CODE    0x000001b5
#define GROUP_HEADER_CODE       0x000001b8
#define MIN_SLICE_START_CODE    0x00000101
#define MAX_SLICE_START_CODE    0x000001af


typedef struct
{
    uint8_t info;
    Rational aspect_ratio;
} AspectRatioMap;

static const AspectRatioMap ASPECT_RATIO_MAP[] =
{
    {0x02,  {  4,   3}},
    {0x03,  { 16,   9}},
    {0x04,  {221, 100}},
};


typedef struct
{
    uint8_t code;
    Rational frame_rate;
} FrameRateMap;

static const FrameRateMap FRAME_RATE_MAP[] =
{
    {0x01,  {24000, 1001}},
    {0x02,  {24,    1}},
    {0x03,  {25,    1}},
    {0x04,  {30000, 1001}},
    {0x05,  {30,    1}},
    {0x06,  {50,    1}},
    {0x07,  {60000, 1001}},
    {0x08,  {60,    1}},
};



MPEG2EssenceParser::MPEG2EssenceParser()
{
    ResetFrameSize();
    ResetFrameInfo();

    mHorizontalSize = 0;
    mVerticalSize = 0;
    mHaveKnownAspectRatio = false;
    mAspectRatio = ZERO_RATIONAL;
    mHaveKnownFrameRate = false;
    mFrameRate = ZERO_RATIONAL;
    mBitRate = 0;
    mLowDelay = false;
    mProfileAndLevel = 0;
    mIsProgressive = false;
    mChromaFormat = 0;

    mVideoFormat = 0;
    mColorPrimaries = 0;
    mTransferCharacteristics = 0;
    mMatrixCoeffs = 0;
    mDHorizontalSize = 0;
    mDVerticalSize = 0;

    mClosedGOP = false;
}

MPEG2EssenceParser::~MPEG2EssenceParser()
{
}

uint32_t MPEG2EssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    uint32_t state = 0xffffffff;
    uint32_t offset = 0;
    while (offset < data_size) {
        state = (state << 8) | data[offset];
        if (state == SEQUENCE_HEADER_CODE ||
            state == GROUP_HEADER_CODE ||
            state == PICTURE_START_CODE)
        {
            return offset - 3;
        }

        offset++;
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

uint32_t MPEG2EssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);
    BMX_CHECK(mOffset <= data_size);

    if (data_size < 4)
        return ESSENCE_PARSER_NULL_OFFSET;

    while (mOffset < data_size) {
        mState = (mState << 8) | data[mOffset];
        if (mState == SEQUENCE_HEADER_CODE ||
            mState == GROUP_HEADER_CODE ||
            mState == PICTURE_START_CODE)
        {
            if ((mState == SEQUENCE_HEADER_CODE && (mSequenceHeader || mGroupHeader || mPictureStart)) ||
                (mState == GROUP_HEADER_CODE && (mGroupHeader || mPictureStart)) ||
                (mState == PICTURE_START_CODE && mPictureStart))
            {
                uint32_t frame_size = mOffset - 3;
                ResetFrameSize();
                return frame_size;
            }

            mSequenceHeader = mSequenceHeader || (mState == SEQUENCE_HEADER_CODE);
            mGroupHeader = mGroupHeader || (mState == GROUP_HEADER_CODE);
            mPictureStart = mPictureStart || (mState == PICTURE_START_CODE);
        }
        else if (mOffset == 3)
        {
            // not a valid frame start
            ResetFrameSize();
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
        }

        mOffset++;
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

void MPEG2EssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    ResetFrameInfo();

    size_t i;
    uint32_t state = 0xffffffff;
    uint32_t offset = 0;
    while (offset < data_size) {
        state = (state << 8) | data[offset];
        if (state == SEQUENCE_HEADER_CODE) {
            mHaveSequenceHeader = true;
            mHorizontalSize = get_bits(data, data_size, (offset - 3) * 8 + 32, 12);
            mVerticalSize = get_bits(data, data_size, (offset - 3) * 8 + 44, 12);

            // note that MPEG-2 aspect ratio isn't quite the same as MXF's aspect ratio
            uint8_t aspect_ratio_info = get_bits(data, data_size, (offset - 3) * 8 + 56, 4);
            mHaveKnownAspectRatio = false;
            mAspectRatio = ZERO_RATIONAL;
            for (i = 0; i < BMX_ARRAY_SIZE(ASPECT_RATIO_MAP); i++) {
                if (aspect_ratio_info == ASPECT_RATIO_MAP[i].info) {
                    mHaveKnownAspectRatio = true;
                    mAspectRatio = ASPECT_RATIO_MAP[i].aspect_ratio;
                    break;
                }
            }

            uint8_t frame_rate_code = get_bits(data, data_size, (offset - 3) * 8 + 60, 4);
            mFrameRate = ZERO_RATIONAL;
            mHaveKnownFrameRate = false;
            for (i = 0; i < BMX_ARRAY_SIZE(FRAME_RATE_MAP); i++) {
                if (frame_rate_code == FRAME_RATE_MAP[i].code) {
                    mFrameRate = FRAME_RATE_MAP[i].frame_rate;
                    mHaveKnownFrameRate = true;
                    break;
                }
            }

            mBitRate = get_bits(data, data_size, (offset - 3) * 8 + 64, 18);
        } else if (state == EXTENSION_START_CODE) {
            uint8_t ext_id = get_bits(data, data_size, (offset - 3) * 8 + 32, 4);
            if (ext_id == 0x01) { // sequence extension id
                mHorizontalSize |= get_bits(data, data_size, (offset - 3) * 8 + 47, 2) << 12;
                mVerticalSize |= get_bits(data, data_size, (offset - 3) * 8 + 49, 2) << 12;
                mBitRate |= get_bits(data, data_size, (offset - 3) * 8 + 51, 12) << 18;
                mLowDelay = get_bits(data, data_size, (offset - 3) * 8 + 72, 1);

                if (mFrameRate.numerator > 0) {
                    mFrameRate.numerator   *= get_bits(data, data_size, (offset - 3) * 8 + 73, 2) + 1;
                    mFrameRate.denominator *= get_bits(data, data_size, (offset - 3) * 8 + 75, 5) + 1;
                }

                mProfileAndLevel = get_bits(data, data_size, (offset - 3) * 8 + 36, 8);
                mIsProgressive = (get_bits(data, data_size, (offset - 3) * 8 + 44, 1) == 1);
                mChromaFormat = get_bits(data, data_size, (offset - 3) * 8 + 45, 2);
                switch (mChromaFormat)
                {
                    case 2 : mChromaFormat = 422; break;
                    case 1 : mChromaFormat = 420; break;
                    case 3 : mChromaFormat = 444; break;
                    default: mChromaFormat = 0;   break;
                }
            }
        } else if (state == GROUP_HEADER_CODE) {
            mHaveGOPHeader = true;
            mClosedGOP = get_bits(data, data_size, (offset - 3) * 8 + 57, 1);
        } else if (state == PICTURE_START_CODE) {
            mTemporalReference = get_bits(data, data_size, (offset - 3) * 8 + 32, 10);
            switch (get_bits(data, data_size, (offset - 3) * 8 + 42, 3))
            {
                case 0x01:
                    mFrameType = I_FRAME;
                    break;
                case 0x02:
                    mFrameType = P_FRAME;
                    break;
                case 0x03:
                    mFrameType = B_FRAME;
                    break;
                default:
                    mFrameType = UNKNOWN_FRAME_TYPE;
                    break;
            }
            mVBVDelay = get_bits(data, data_size, (offset - 3) * 8 + 45, 16);

            break;
        }

        offset++;
    }
}

void MPEG2EssenceParser::ParseFrameAllInfo(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    ResetFrameInfo();

    size_t i;
    uint32_t state = 0xffffffff;
    uint32_t offset = 0;

    while (offset < data_size) {
        state = (state << 8) | data[offset];
        if (state == SEQUENCE_HEADER_CODE) {
            mHaveSequenceHeader = true;

            mHorizontalSize = get_bits(data, data_size, (offset - 3) * 8 + 32, 12);
            mVerticalSize = get_bits(data, data_size, (offset - 3) * 8 + 44, 12);

            // note that MPEG-2 aspect ratio isn't quite the same as MXF's aspect ratio
            uint8_t aspect_ratio_info = get_bits(data, data_size, (offset - 3) * 8 + 56, 4);
            mHaveKnownAspectRatio = false;
            mAspectRatio = ZERO_RATIONAL;
            for (i = 0; i < BMX_ARRAY_SIZE(ASPECT_RATIO_MAP); i++) {
                if (aspect_ratio_info == ASPECT_RATIO_MAP[i].info) {
                    mHaveKnownAspectRatio = true;
                    mAspectRatio = ASPECT_RATIO_MAP[i].aspect_ratio;
                    break;
                }
            }

            uint8_t frame_rate_code = get_bits(data, data_size, (offset - 3) * 8 + 60, 4);
            mFrameRate = ZERO_RATIONAL;
            mHaveKnownFrameRate = false;
            for (i = 0; i < BMX_ARRAY_SIZE(FRAME_RATE_MAP); i++) {
                if (frame_rate_code == FRAME_RATE_MAP[i].code) {
                    mFrameRate = FRAME_RATE_MAP[i].frame_rate;
                    mHaveKnownFrameRate = true;
                    break;
                }
            }

            mBitRate = get_bits(data, data_size, (offset - 3) * 8 + 64, 18);
        }
        else if (state == EXTENSION_START_CODE) {
            uint8_t ext_id = get_bits(data, data_size, (offset - 3) * 8 + 32, 4);
            if (ext_id == 0x01) { // sequence extension id
                mHaveSequenceExtension = true;

                mHorizontalSize |= get_bits(data, data_size, (offset - 3) * 8 + 47, 2) << 12;
                mVerticalSize |= get_bits(data, data_size, (offset - 3) * 8 + 49, 2) << 12;
                mBitRate |= get_bits(data, data_size, (offset - 3) * 8 + 51, 12) << 18;
                mLowDelay = get_bits(data, data_size, (offset - 3) * 8 + 72, 1);

                if (mFrameRate.numerator > 0) {
                    mFrameRate.numerator *= get_bits(data, data_size, (offset - 3) * 8 + 73, 2) + 1;
                    mFrameRate.denominator *= get_bits(data, data_size, (offset - 3) * 8 + 75, 5) + 1;
                }

                mProfileAndLevel = get_bits(data, data_size, (offset - 3) * 8 + 36, 8);
                mIsProgressive = (get_bits(data, data_size, (offset - 3) * 8 + 44, 1) == 1);
                mChromaFormat = get_bits(data, data_size, (offset - 3) * 8 + 45, 2);
                switch (mChromaFormat)
                {
                    case 2 : mChromaFormat = 422; break;
                    case 1 : mChromaFormat = 420; break;
                    case 3 : mChromaFormat = 444; break;
                    default: mChromaFormat = 0;   break;
                }
            }
            else if (ext_id == 0x02) // sequence display extension
            {
                mHaveDisplayExtension = true;

                // PAL 001 NTSC 010 SECAM 011 MAC 100
                mVideoFormat = get_bits(data, data_size, (offset - 3) * 8 + 36, 3);

                if (get_bits(data, data_size, (offset - 3) * 8 + 39, 1) == 1)
                {
                    mHaveColorDescription = true;
                    // mColorPrimaries == mTransferCharacteristics == mMatrixCoeffs = 1 => BT.709, SMPTE 274M
                    // -- " -- = 0000 0101 => BT.470 system B, G, I
                    //         = 0000 0110 => SMPTE 170M
                    //         = 0000 0111 => SMPTE 240M
                    mColorPrimaries          = get_bits(data, data_size, (offset - 3) * 8 + 40, 8);
                    mTransferCharacteristics = get_bits(data, data_size, (offset - 3) * 8 + 48, 8);
                    mMatrixCoeffs            = get_bits(data, data_size, (offset - 3) * 8 + 56, 8);
                }

                mDHorizontalSize = get_bits(data, data_size, (offset - 3) * 8 + 64, 14);
                mDVerticalSize   = get_bits(data, data_size, (offset - 3) * 8 + 79, 14);
            }
            else if (ext_id == 0x08) // picture coding extension
            {
                mHavePicCodingExtension = true;

                // if mIsProgressive == 0 => top or bottom
                mIsTFF = get_bits(data, data_size, (offset - 3) * 8 + 56, 1);
            }
        }
        else if (state == GROUP_HEADER_CODE) {
            mHaveGOPHeader = true;
            mClosedGOP = get_bits(data, data_size, (offset - 3) * 8 + 57, 1);
        }
        else if (state == PICTURE_START_CODE) {
            mTemporalReference = get_bits(data, data_size, (offset - 3) * 8 + 32, 10);
            switch (get_bits(data, data_size, (offset - 3) * 8 + 42, 3))
            {
            case 0x01:
                mFrameType = I_FRAME;
                break;
            case 0x02:
                mFrameType = P_FRAME;
                break;
            case 0x03:
                mFrameType = B_FRAME;
                break;
            default:
                mFrameType = UNKNOWN_FRAME_TYPE;
                break;
            }

            //!= 0xffff => const
            mVBVDelay = get_bits(data, data_size, (offset - 3) * 8 + 45, 16);
        }
        else if (state >= MIN_SLICE_START_CODE && state <= MAX_SLICE_START_CODE) {
            break;
        }

        offset++;
    }
}

void MPEG2EssenceParser::ResetFrameSize()
{
    mOffset = 0;
    mState = 0xffffffff;
    mSequenceHeader = false;
    mGroupHeader = false;
    mPictureStart = false;
}

void MPEG2EssenceParser::ResetFrameInfo()
{
    mHaveSequenceHeader = false;
    mHaveSequenceExtension = false;
    mHaveDisplayExtension = false;
    mHavePicCodingExtension = false;
    mHaveColorDescription = false;
    mHaveGOPHeader = false;

    mFrameType = UNKNOWN_FRAME_TYPE;
    mTemporalReference = 0;
    mVBVDelay = 0xffffffff;
    mIsTFF = false;
}

