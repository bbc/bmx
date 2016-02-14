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
#include <cstring>
#include <map>

#include <bmx/EssenceType.h>
#include <bmx/essence_parser/MPEG2EssenceParser.h>

namespace bmx
{

//N.B nlogged = 1 used for very first check to ignore any user assignements
typedef struct
{   const char *name;
	uint32_t   defaultvalue;
	uint32_t   value;
	bool       issetbyuser;
	uint32_t   nlogged;
} DescriptorValue;

typedef struct
{   const char *name;
	Rational   defaultvalue;
	Rational   value;
	bool       issetbyuser;
	uint32_t   nlogged;
} DescriptorRationalValue;

typedef struct
{   const char* shimName;
	DescriptorValue  mSingleSequence;
	DescriptorRationalValue mAspectRatio;
	DescriptorRationalValue mSampleRate;
	DescriptorValue  mBitRate;
	DescriptorValue  mBitRateDelta;
	DescriptorValue  mConstBitRate;
	DescriptorValue  mIsProgressive; //0 int, 1 - prgr, 2 - *any*
	DescriptorValue  mUniqShimHVSize;
	DescriptorValue  mHorizontalSize;
	DescriptorValue  mVerticalSize;
	DescriptorValue  mChromaFormat;
	DescriptorValue  mColorPrimaries;
	DescriptorValue  mTFF;
	DescriptorValue  mLowDelay;
	DescriptorValue  mClosedGOP;
	DescriptorValue  mIdenticalGOP;
	DescriptorValue  mMaxGOP;
	DescriptorValue  mBPictureCount;
	DescriptorValue  mConstantBFrames;
} MpegDefaults;

typedef struct {
	std::string name;
	int modified;
	std::string value;
} descriptor;

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

	void SetFlavour(Flavour flavour) { mFlavour = flavour; } 
	void ParseDescriptorRefValues(const char *filename);
	void shim(const char *shimname);

public:
    void ProcessFrame(const unsigned char *data, uint32_t size);
    bool CheckTemporalOffsetsComplete(int64_t end_offset);
	void DoAllHeadersChecks(bool set_it_on, uint32_t maxLoggedViolations, bool verbose, bool looseChecks);
	void ReportCheckedHeaders();
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

	EssenceType mEssenceType;
	bool mSequenceHeaderChk;
	bool mSequenceExtentionChk;
	bool mDisplayExtentionChk;
	bool mPicCodingExtentionChk;

	bool mPrintParsingReport;

	MpegDefaults mHDRefaults;
	std::map<std::string, DescriptorValue*> mDfaultsMap;

	uint32_t mMaxLoggedViolations;

	bool mAllHeadersChecks;
	bool mLooseChecks; 
	const char *mKnownShims; 
	std::vector<descriptor> mMXFDescriptors;
	void AllHeadersChecks();
	void AssignMapVals();
	void log_warn_descr_value(const DescriptorValue *dv);
	void modify_descriptors(std::vector<descriptor> &mxf_descriptors, const char *name, const int &modified, const char *value);
};



};


#endif

