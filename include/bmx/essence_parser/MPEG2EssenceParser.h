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
    virtual void ParseFrameAllInfo(const unsigned char *data, uint32_t data_size);

public:
    // bitstream properties
    bool HaveSequenceHeader() const             { return mHaveSequenceHeader; }
    bool HaveExtension() const                  { return mHaveSequenceExtension ||
                                                         mHaveDisplayExtension ||
                                                         mHavePicCodingExtension; }
    bool HaveSequenceExtension() const          { return mHaveSequenceExtension; }
    bool HaveDisplayExtension() const           { return mHaveDisplayExtension; }
    bool HavePicCodingExtension() const         { return mHavePicCodingExtension; }
    bool HaveColorDescription() const           { return mHaveColorDescription; }
    bool HaveGOPHeader() const                  { return mHaveGOPHeader; }

    // sequence properties
    uint32_t GetHorizontalSize() const          { return mHorizontalSize; }
    uint32_t GetVerticalSize() const            { return mVerticalSize; }
    bool HaveKnownAspectRatio() const           { return mHaveKnownAspectRatio; }
    Rational GetAspectRatio() const             { return mAspectRatio; }
    bool HaveKnownFrameRate() const             { return mHaveKnownFrameRate; }
    Rational GetFrameRate() const               { return mFrameRate; }
    uint32_t GetBitRate() const                 { return mBitRate; } // in 400bps units
    bool IsLowDelay() const                     { return mLowDelay; }
    uint8_t GetProfileAndLevel() const          { return mProfileAndLevel; }
    bool IsProgressive() const                  { return mIsProgressive; }
    uint32_t GetChromaFormat() const            { return mChromaFormat;  }

    // sequence display properties
    uint8_t GetVideoFormat() const              { return mVideoFormat; }
    uint8_t GetColorPrimaries() const           { return mColorPrimaries; }
    uint8_t GetTransferCharacteristics() const  { return mTransferCharacteristics; }
    uint8_t GetMatrixCoeffs() const             { return mMatrixCoeffs; }
    uint32_t GetDHorizontalSize() const         { return mDHorizontalSize; }
    uint32_t GetDVerticalSize() const           { return mDVerticalSize; }

    // gop properties
    bool IsClosedGOP() const                    { return mClosedGOP; }

    // picture properties
    MPEGFrameType GetFrameType() const          { return mFrameType; }
    uint32_t GetTemporalReference() const       { return mTemporalReference; }
    uint32_t GetVBVDelay() const                { return mVBVDelay; }
    bool IsTFF() const                          { return mIsTFF; }

private:
    void ResetFrameSize();
    void ResetFrameInfo();

private:
    // parse frame
    uint32_t mOffset;
    uint32_t mState;
    bool mSequenceHeader;
    bool mGroupHeader;
    bool mPictureStart;

    // bitstream
    bool mHaveSequenceHeader;
    bool mHaveSequenceExtension;
    bool mHaveDisplayExtension;
    bool mHavePicCodingExtension;
    bool mHaveColorDescription;
    bool mHaveGOPHeader;

    // sequence properties
    uint32_t mHorizontalSize;
    uint32_t mVerticalSize;
    bool mHaveKnownAspectRatio;
    Rational mAspectRatio;
    bool mHaveKnownFrameRate;
    Rational mFrameRate;
    uint32_t mBitRate;
    bool mLowDelay;
    uint8_t mProfileAndLevel;
    bool mIsProgressive;
    uint32_t mChromaFormat;

    // sequence display properties
    uint8_t mVideoFormat;
    uint8_t mColorPrimaries;
    uint8_t mTransferCharacteristics;
    uint8_t mMatrixCoeffs;
    uint32_t mDHorizontalSize;
    uint32_t mDVerticalSize;

    // gop properties
    bool mClosedGOP;

    // picture properties
    MPEGFrameType mFrameType;
    uint32_t mTemporalReference;
    uint32_t mVBVDelay;
    bool mIsTFF;
};


};



#endif

