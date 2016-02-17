/*
 * Copyright (C) 2016, British Broadcasting Corporation
 * All Rights Reserved.
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

#ifndef BMX_AS10_MPEG2_VALIDATOR_H_
#define BMX_AS10_MPEG2_VALIDATOR_H_

#include <string.h>
#include <map>

#include <bmx/EssenceType.h>
#include <bmx/mxf_helper/MPEG2Validator.h>
#include <bmx/as10/AS10ShimNames.h>


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
{   AS10Shim shim;
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


class AS10MPEG2Validator : public MPEG2Validator
{
public:
    AS10MPEG2Validator(AS10Shim as10_shim, const char *fname, int max_same_warn_messages,
                       bool print_defaults, bool loose_checks);
    virtual ~AS10MPEG2Validator();

    virtual void ProcessFrame(const unsigned char *data, uint32_t size);
    virtual void CompleteWrite();

private:
    void ParseDescriptorRefValues(const char *filename);
    void shim(AS10Shim as10_shim);
    void DoAllHeadersChecks(bool set_it_on, uint32_t maxLoggedViolations, bool verbose, bool loose_checks);
    void ReportCheckedHeaders();

private:
    AS10Shim mShim;
    bool mFirstFrame;
    MPEG2EssenceParser mEssenceParser;
    EssenceType mEssenceType;
    bool mSequenceHeaderChk;
    bool mSequenceExtensionChk;
    bool mDisplayExtensionChk;
    bool mPicCodingExtensionChk;

    bool mPrintParsingReport;

    MpegDefaults mHDRefaults;
    std::map<std::string, DescriptorValue*> mDfaultsMap;

    uint32_t mMaxLoggedViolations;

    bool mAllHeadersChecks;
    bool mLooseChecks;
    std::vector<descriptor> mMXFDescriptors;
    void AllHeadersChecks();
    void AssignMapVals();
    void log_warn_descr_value(const DescriptorValue *dv);
    void modify_descriptors(std::vector<descriptor> &mxf_descriptors, const char *name, const int &modified, const char *value);
};



};


#endif
