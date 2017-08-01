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

#ifndef BMX_RDD36_ESSENCE_PARSER_H_
#define BMX_RDD36_ESSENCE_PARSER_H_

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/BitBuffer.h>


namespace bmx
{


typedef enum
{
  RDD36_NO_ALPHA      = 0,
  RDD36_ALPHA_8BIT    = 1,
  RDD36_ALPHA_16BIT   = 2,
} RDD36AlphaChannelType;

typedef enum
{
  RDD36_PROGRESSIVE_FRAME   = 0,
  RDD36_INTERLACED_TFF      = 1,
  RDD36_INTERLACED_BFF      = 2,
} RDD36InterlaceMode;

typedef enum
{
  RDD36_CHROMA_422   = 2,
  RDD36_CHROMA_444   = 3,
} RDD36ChromaFormat;

typedef enum
{
  RDD36_ASPECT_RATIO_UNKNOWN  = 0,
  RDD36_SQUARE_PIXELS         = 1,
  RDD36_ASPECT_RATIO_4X3      = 2,
  RDD36_ASPECT_RATIO_16X9     = 3,
} RDD36AspectRatio;


class RDD36GetBitBuffer : public GetBitBuffer
{
public:
    RDD36GetBitBuffer(const unsigned char *data, uint32_t data_size);

    uint64_t GetF(uint8_t num_bits);
    uint32_t GetF32(uint8_t num_bits);

    uint64_t GetU(uint8_t num_bits);
    uint8_t GetU8(uint8_t num_bits);
    uint16_t GetU16(uint8_t num_bits);
    uint32_t GetU32(uint8_t num_bits);
};


class RDD36EssenceParser : public EssenceParser
{
public:
    RDD36EssenceParser();
    virtual ~RDD36EssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

public:
    bool HaveFrameRate() const                  { return mFrameRate.numerator > 0 && mFrameRate.denominator > 0; }
    Rational GetFrameRate() const               { return mFrameRate; }
    uint16_t GetWidth() const                   { return mWidth; }
    uint16_t GetHeight() const                  { return mHeight; }
    uint8_t GetChromaFormat() const             { return mChromaFormat; }
    uint8_t GetInterlaceMode() const            { return mInterlaceMode; }
    uint8_t GetAspectRatioCode() const          { return mAspectRatioCode; }
    uint8_t GetColorPrimaries() const           { return mColorPrimaries; }
    uint8_t GetTransferCharacteristic() const   { return mTransferCharacteristic; }
    uint8_t GetMatrixCoefficients() const       { return mMatrixCoefficients; }
    uint8_t GetAlphaChannelType() const         { return mAlphaChannelType; }

private:
    void ResetFrameInfo();

private:
    Rational mFrameRate;
    uint16_t mWidth;
    uint16_t mHeight;
    uint8_t mChromaFormat;
    uint8_t mInterlaceMode;
    uint8_t mAspectRatioCode;
    uint8_t mColorPrimaries;
    uint8_t mTransferCharacteristic;
    uint8_t mMatrixCoefficients;
    uint8_t mAlphaChannelType;
};


};



#endif
