/*
 * Copyright (C) 2017, British Broadcasting Corporation
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

#include <bmx/essence_parser/RDD36EssenceParser.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const uint32_t RDD36_FRAME_ID = 0x69637066; // 'icpf'


static Rational convert_frame_rate(uint8_t frame_rate_code)
{
    static const Rational FRAME_RATES[] = {
        {0, 1}, {24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {30, 1}, {50, 1},
        {60000, 1}, {60, 1}, {100, 1}, {120000, 1001}, {120, 1}
    };

    if (frame_rate_code < BMX_ARRAY_SIZE(FRAME_RATES))
        return FRAME_RATES[frame_rate_code];
    else
        return ZERO_RATIONAL;
}


RDD36GetBitBuffer::RDD36GetBitBuffer(const unsigned char *data, uint32_t data_size)
: GetBitBuffer(data, data_size)
{
}

uint64_t RDD36GetBitBuffer::GetF(uint8_t num_bits)
{
    uint64_t value;
    BMX_CHECK(GetBits(num_bits, &value));
    return value;
}

uint32_t RDD36GetBitBuffer::GetF32(uint8_t num_bits)
{
    BMX_ASSERT(num_bits <= 32);
    uint64_t value;
    BMX_CHECK(GetBits(num_bits, &value));
    return (uint32_t)value;
}

uint64_t RDD36GetBitBuffer::GetU(uint8_t num_bits)
{
    uint64_t value;
    BMX_CHECK(GetBits(num_bits, &value));
    return value;
}

uint8_t RDD36GetBitBuffer::GetU8(uint8_t num_bits)
{
    BMX_ASSERT(num_bits <= 8);
    uint64_t value;
    BMX_CHECK(GetBits(num_bits, &value));
    return (uint8_t)value;
}

uint16_t RDD36GetBitBuffer::GetU16(uint8_t num_bits)
{
    BMX_ASSERT(num_bits <= 16);
    uint64_t value;
    BMX_CHECK(GetBits(num_bits, &value));
    return (uint16_t)value;
}

uint32_t RDD36GetBitBuffer::GetU32(uint8_t num_bits)
{
    BMX_ASSERT(num_bits <= 32);
    uint64_t value;
    BMX_CHECK(GetBits(num_bits, &value));
    return (uint32_t)value;
}



RDD36EssenceParser::RDD36EssenceParser()
: EssenceParser()
{
    ResetFrameInfo();
}

RDD36EssenceParser::~RDD36EssenceParser()
{
}

uint32_t RDD36EssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    if (data_size < 8)
        return ESSENCE_PARSER_NULL_OFFSET;

    RDD36GetBitBuffer buffer(data, data_size);
    buffer.GetU(32); // frame size
    uint32_t frame_identifier = buffer.GetF32(32);
    if (frame_identifier != RDD36_FRAME_ID)
        return ESSENCE_PARSER_NULL_OFFSET;
    else
        return 0;
}

uint32_t RDD36EssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    if (data_size < 8)
        return ESSENCE_PARSER_NULL_OFFSET;

    RDD36GetBitBuffer buffer(data, data_size);
    uint32_t frame_size = buffer.GetU32(32);
    uint32_t frame_identifier = buffer.GetF32(32);
    if (frame_identifier != RDD36_FRAME_ID)
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    else if (data_size < frame_size)
        return ESSENCE_PARSER_NULL_OFFSET;
    else
        return frame_size;
}

void RDD36EssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    ResetFrameInfo();

    RDD36GetBitBuffer buffer(data, data_size);
    buffer.GetU(32); // frame_size
    uint32_t frame_identifier = buffer.GetF32(32);
    BMX_CHECK(frame_identifier == RDD36_FRAME_ID);

    buffer.GetU(16); // frame_header_size
    buffer.GetU(8); // reserved
    uint8_t bitstream_version = buffer.GetU8(8);
    if (bitstream_version > 1)
        BMX_EXCEPTION(("Unsupported RDD-36 bitstream version %u", bitstream_version));
    buffer.GetF(32); // encoder_identifier
    mWidth = buffer.GetU16(16);
    if (mWidth == 0)
        BMX_EXCEPTION(("RDD-36 horizontal size is 0"));
    mHeight = buffer.GetU16(16);
    if (mHeight == 0)
        BMX_EXCEPTION(("RDD-36 vertical size is 0"));
    mChromaFormat = buffer.GetU8(2);
    if (mChromaFormat != RDD36_CHROMA_422 && mChromaFormat != RDD36_CHROMA_444)
        BMX_EXCEPTION(("Unsupported RDD-36 chroma format %u", mChromaFormat));
    buffer.GetU(2); // reserved
    mInterlaceMode = buffer.GetU8(2);
    if (mInterlaceMode != RDD36_PROGRESSIVE_FRAME &&
        mInterlaceMode != RDD36_INTERLACED_TFF &&
        mInterlaceMode != RDD36_INTERLACED_BFF)
    {
        BMX_EXCEPTION(("Unsupported RDD-36 interlace mode %u", mInterlaceMode));
    }
    buffer.GetU(2); // reserved
    mAspectRatioCode = buffer.GetU8(4);
    mFrameRate = convert_frame_rate(buffer.GetU8(4));
    mColorPrimaries = buffer.GetU8(8);
    mTransferCharacteristic = buffer.GetU8(8);
    mMatrixCoefficients = buffer.GetU8(8);
    buffer.GetU(4); // reserved
    mAlphaChannelType = buffer.GetU8(4);
    if (mAlphaChannelType != RDD36_NO_ALPHA && mChromaFormat == RDD36_CHROMA_422)
        BMX_EXCEPTION(("Non-zero alpha channel type %u combined with 4:2:2 profile is not supported", mAlphaChannelType));
}

void RDD36EssenceParser::ResetFrameInfo()
{
    mFrameRate = ZERO_RATIONAL;
    mWidth = 0;
    mHeight = 0;
    mChromaFormat = 0;
    mInterlaceMode = 0;
    mAspectRatioCode = 0;
    mColorPrimaries = 0;
    mTransferCharacteristic = 0;
    mMatrixCoefficients = 0;
    mAlphaChannelType = 0;
}
