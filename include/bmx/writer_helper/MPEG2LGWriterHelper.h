/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#ifndef BMX_MPEG2LG_WRITER_HELPER_H_
#define BMX_MPEG2LG_WRITER_HELPER_H_

#include <vector>

#include <bmx/essence_parser/MPEG2EssenceParser.h>



namespace bmx
{


class MPEG2LGWriterHelper
{
public:
    typedef enum
    {
        DEFAULT_FLAVOUR,
        AVID_FLAVOUR,
    } Flavour;

public:
    MPEG2LGWriterHelper();
    ~MPEG2LGWriterHelper();

    void SetFlavour(Flavour flavour);

public:
    void ProcessFrame(const unsigned char *data, uint32_t size);

    bool CheckTemporalOffsetsComplete(int64_t end_offset);

public:
    int64_t GetFramePosition() const        { return mPosition - 1; }

    uint32_t GetTemporalReference() const   { return mTemporalReference; }

    bool HaveTemporalOffset() const         { return mHaveTemporalOffset; }
    int8_t GetTemporalOffset() const        { return mHaveTemporalOffset ? mTemporalOffset : 0; }
    bool HavePrevTemporalOffset() const     { return mHavePrevTemporalOffset; }
    int8_t GetPrevTemporalOffset() const    { return mHavePrevTemporalOffset ? mPrevTemporalOffset : 0; }
    int8_t GetKeyFrameOffset() const        { return mKeyFrameOffset; }
    uint8_t GetFlags() const                { return mFlags; }

    bool HaveGOPHeader() const              { return mHaveGOPHeader; }
    bool GetSingleSequence() const          { return mSingleSequence; }
    int16_t GetBPictureCount() const        { return mBPictureCount; }
    bool GetConstantBFrames() const         { return mConstantBFrames; }
    bool GetLowDelay() const                { return mLowDelay; }
    bool GetClosedGOP() const               { return mClosedGOP; }
    bool GetIdenticalGOP() const            { return mIdenticalGOP; }
    uint16_t GetMaxGOP() const              { return mMaxGOP; }
    uint16_t GetMaxBPictureCount() const    { return mMaxBPictureCount; }
    uint32_t GetBitRate() const             { return mBitRate; }

private:
    Flavour mFlavour;

    MPEG2EssenceParser mEssenceParser;

    int64_t mPosition;

    int64_t mPrevKeyFramePosition;
    int64_t mKeyFramePosition;
    int8_t mKeyFrameTemporalReference;
    int8_t mGOPTemporalOffsets[256];

    int64_t mGOPStartPosition;
    bool mFirstGOP;
    std::vector<int> mGOPStructure;

    uint32_t mTemporalReference;

    bool mHaveTemporalOffset;
    int8_t mTemporalOffset;
    bool mHavePrevTemporalOffset;
    int8_t mPrevTemporalOffset;
    int8_t mKeyFrameOffset;
    uint8_t mFlags;

    bool mHaveGOPHeader;
    bool mSingleSequence;
    uint16_t mBPictureCount;
    bool mConstantBFrames;
    bool mLowDelay;
    bool mClosedGOP;
    bool mCurrentGOPClosed;
    bool mIdenticalGOP;
    uint16_t mMaxGOP;
    bool mUnlimitedGOPSize;
    uint16_t mMaxBPictureCount;
    uint32_t mBitRate;
};



};


#endif

