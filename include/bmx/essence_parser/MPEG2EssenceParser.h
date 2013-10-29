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

#ifndef BMX_MPEG2_ESSENCE_PARSER_H_
#define BMX_MPEG2_ESSENCE_PARSER_H_


#include <bmx/essence_parser/EssenceParser.h>



namespace bmx
{


class MPEG2EssenceParser : public EssenceParser
{
public:
    MPEG2EssenceParser();
    virtual ~MPEG2EssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

public:
    bool HaveSequenceHeader() const             { return mHaveSequenceHeader; }
    uint32_t GetHorizontalSize() const          { return mHorizontalSize; }
    uint32_t GetVerticalSize() const            { return mVerticalSize; }
    bool HaveKnownAspectRatio() const           { return mHaveKnownAspectRatio; }
    Rational GetAspectRatio() const             { return mAspectRatio; }
    Rational GetFrameRate() const               { return mFrameRate; }
    uint32_t GetBitRate() const                 { return mBitRate; } // in 400bps units
    bool IsLowDelay() const                     { return mLowDelay; }
    uint8_t GetProfileAndLevel() const          { return mProfileAndLevel; }
    bool IsProgressive() const                  { return mIsProgressive; }

    bool HaveGOPHeader() const                  { return mHaveGOPHeader; }
    bool IsClosedGOP() const                    { return mClosedGOP; }

    MPEGFrameType GetFrameType() const          { return mFrameType; }
    uint32_t GetTemporalReference() const       { return mTemporalReference; }

private:
    void Reset();

private:
    uint32_t mOffset;
    uint32_t mState;
    bool mSequenceHeader;
    bool mGroupHeader;
    bool mPictureStart;

    uint32_t mHorizontalSize;
    uint32_t mVerticalSize;
    bool mHaveKnownAspectRatio;
    Rational mAspectRatio;
    Rational mFrameRate;
    uint32_t mBitRate;
    bool mLowDelay;
    uint8_t mProfileAndLevel;
    bool mIsProgressive;

    bool mClosedGOP;

    bool mHaveSequenceHeader;
    bool mHaveGOPHeader;
    MPEGFrameType mFrameType;
    uint32_t mTemporalReference;
};


};



#endif

