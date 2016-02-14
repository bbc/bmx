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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_LIMIT_MACROS

#include <stdio.h>
#include <cstring>
#include <map>
#include <sstream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

#include <bmx/writer_helper/MPEG2LGWriterHelper.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define NULL_TEMPORAL_OFFSET        127

#include <bmx/writer_helper/MPEG2LGWriterAS10Shims.h>

// trim from start
static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}

MPEG2LGWriterHelper::MPEG2LGWriterHelper()
{
    mFlavour = DEFAULT_FLAVOUR;
    mPosition = 0;
    mPrevKeyFramePosition = -1;
    mKeyFramePosition = -1;
    mKeyFrameTemporalReference = 0;
    memset(mGOPTemporalOffsets, NULL_TEMPORAL_OFFSET, sizeof(mGOPTemporalOffsets));
    mGOPStartPosition = 0;
    mFirstGOP = true;
    mTemporalReference = 0;
    mHaveTemporalOffset = false;
    mTemporalOffset = NULL_TEMPORAL_OFFSET;
    mHavePrevTemporalOffset = false;
    mPrevTemporalOffset = NULL_TEMPORAL_OFFSET;
    mKeyFrameOffset = 0;
    mFlags = 0;
    mHaveGOPHeader = false;
    mSingleSequence = true;
    mBPictureCount = 0;
    mConstantBFrames = true;
    mLowDelay = true;
    mClosedGOP = true;
    mCurrentGOPClosed = false;
    mIdenticalGOP = true;
    mMaxGOP = 0;
    mUnlimitedGOPSize = false;
    mMaxBPictureCount = 0;
    mBitRate = 0;

	mEssenceType = UNKNOWN_ESSENCE_TYPE;
	mSequenceHeaderChk = mSequenceExtentionChk = mDisplayExtentionChk = 
	mPicCodingExtentionChk = mAllHeadersChecks = mPrintParsingReport = mLooseChecks = false;
	mHDRefaults = as10_high_hd_2014_mpeg_values; 
	mKnownShims = D_KNOWN_AS10_SHIMS; 
	AssignMapVals();

	for (int i = 0; (AS10_MXF_Descriptors_Arr[i]).name != ""; i++ )
	{
		mMXFDescriptors.push_back(AS10_MXF_Descriptors_Arr[i]);
	}
}

MPEG2LGWriterHelper::~MPEG2LGWriterHelper()
{
}

void MPEG2LGWriterHelper::shim(const char *shimname)
{
	if(!strcmp(shimname, D_AS10_HIGH_HD_2014))
	    mHDRefaults = as10_high_hd_2014_mpeg_values;
	else if (!strcmp(shimname, D_AS10_CNN_HD_2012))
		mHDRefaults = as10_cnn_hd_2012_mpeg_values;
	else if (!strcmp(shimname, D_AS10_NRK_HD_2012))
		mHDRefaults = as10_nrk_hd_2012_mpeg_values;
	else if (!strcmp(shimname, D_AS10_JVC_HD_35_VBR))
		mHDRefaults = as10_jvc_hd_35_vbr_2012_mpeg_values;
	else if (!strcmp(shimname, D_AS10_JVC_HD_25_CBR))
		mHDRefaults = as10_jvc_hd_25_cbr_2012_mpeg_values;
	else
	{
	   fprintf(stderr, "unknown shim name '%s', known shims: %s...\n", shimname, D_KNOWN_AS10_SHIMS);
	   BMX_EXCEPTION(("input error"));
	}
	
	AssignMapVals();
}

void MPEG2LGWriterHelper::AssignMapVals()
{
	DescriptorValue dummy;

	mDfaultsMap[D_SHIMNAME] = &dummy;
	mDfaultsMap[D_SINGLESEQUENCE] = &mHDRefaults.mSingleSequence;
	mDfaultsMap[D_ASPECTRATIO] = &dummy;
	mDfaultsMap[D_SAMPLERATE] = &dummy;
	mDfaultsMap[D_BITRATE] = &mHDRefaults.mBitRate;
	mDfaultsMap[D_BITRATE_DELTA] = &mHDRefaults.mBitRateDelta;
	mDfaultsMap[D_ISCONSTBITRATE] = &mHDRefaults.mConstBitRate;
	mDfaultsMap[D_ISPROGRESSIVE] = &mHDRefaults.mIsProgressive;
	mDfaultsMap[D_UNIQUERESOLUTION] = &mHDRefaults.mUniqShimHVSize;
	mDfaultsMap[D_HSIZE] = &mHDRefaults.mHorizontalSize;
	mDfaultsMap[D_VSIZE] = &mHDRefaults.mVerticalSize;
	mDfaultsMap[D_CROMA] = &mHDRefaults.mChromaFormat;
	mDfaultsMap[D_COLORPRIMARIES] = &mHDRefaults.mColorPrimaries;
	mDfaultsMap[D_ISTFF] = &mHDRefaults.mTFF;
	mDfaultsMap[D_ISLOWDELAY] = &mHDRefaults.mLowDelay;
	mDfaultsMap[D_ISCLOSEDGOP] = &mHDRefaults.mClosedGOP;
	mDfaultsMap[D_CONSTGOP] = &mHDRefaults.mIdenticalGOP;
	mDfaultsMap[D_MAXGOP] = &mHDRefaults.mMaxGOP;
	mDfaultsMap[D_MAXBFRAMES] = &mHDRefaults.mBPictureCount;
	mDfaultsMap[D_CONSTBFRAMES] = &mHDRefaults.mConstantBFrames;
}

void MPEG2LGWriterHelper::ParseDescriptorRefValues(const char *filename)
{	
	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "failed to open file '%s'\n", filename);
		BMX_EXCEPTION(("input error"));
		return;
	}
		
	bool fatalError = false;

	int line_num = 0;
	int c = '1';
	while (c != EOF) 
	{
		// move file pointer past the newline characters
		while ((c = fgetc(file)) != EOF && (c == '\r' || c == '\n')) 
		{
			if (c == '\n')
				line_num++;
		}

		string name, value;
		bool parse_name = true;
		while (c != EOF && (c != '\r' && c != '\n')) 
		{
			if (c == ':' && parse_name) 
			{
				parse_name = false;
			}
			else
			{
				if (parse_name)
					name += tolower(c);
				else
					value += c;
			}
			c = fgetc(file);
		}

		if (!name.empty()) 
		{
			if (parse_name)
{
				fprintf(stderr, "failed to parse line %d\n", line_num);
				fclose(file);
				BMX_EXCEPTION(("input error"));
				return;
			}
		    
			name = trim(name);
			value = trim(value);

			if (mDfaultsMap.find(name) == mDfaultsMap.end())
			{
				fprintf(stderr, "unknown key %s on line %d\n", name.c_str(), line_num);
				fatalError = true;
			}
			else
			{
				if (! strcmp(name.c_str(), D_SHIMNAME))
				{
					if(! strcmp(value.c_str(), as10_high_hd_2014_mpeg_values.shimName))
						mHDRefaults = as10_high_hd_2014_mpeg_values;
					else
						if (!strcmp(value.c_str(), as10_cnn_hd_2012_mpeg_values.shimName))
							mHDRefaults = as10_cnn_hd_2012_mpeg_values;
					else
					   if (!strcmp(value.c_str(), as10_nrk_hd_2012_mpeg_values.shimName))
						   mHDRefaults = as10_nrk_hd_2012_mpeg_values;
				    else
				       if (!strcmp(value.c_str(), as10_jvc_hd_35_vbr_2012_mpeg_values.shimName))
						   mHDRefaults = as10_jvc_hd_35_vbr_2012_mpeg_values;
					else 
					   if (!strcmp(value.c_str(), as10_jvc_hd_25_cbr_2012_mpeg_values.shimName))
						   mHDRefaults = as10_jvc_hd_25_cbr_2012_mpeg_values;
					else
					   {
						   fprintf(stderr, "unknown shim name '%s', select one of these: %s...\n", value.c_str(), D_KNOWN_AS10_SHIMS);
						   fatalError = true;
					   }

					if (!fatalError)
						log_info("using shim %s\n", mHDRefaults.shimName);
				}
				else if (!strcmp(name.c_str(), D_ASPECTRATIO))
				{
					int num, den;
					if (sscanf(value.c_str(), "%d/%d", &num, &den) == 2)
					{
						mHDRefaults.mAspectRatio.value.numerator = num;
						mHDRefaults.mAspectRatio.value.denominator = den;
						if (mHDRefaults.mAspectRatio.value == ASPECT_RATIO_16_9 || mHDRefaults.mAspectRatio.value == ASPECT_RATIO_4_3)
							mHDRefaults.mAspectRatio.issetbyuser = true;
						else
						{
							fprintf(stderr, "invalid aspect ratio %u/%u, should be 16/9 or 4/3...\n", num, den);
							fatalError = true;
						}
					}
					else
					{
						fprintf(stderr, "aspect ratio %s should be defined as x/y, N.B. only 16/9 or 4/3 are possible\n", value.c_str());
						fatalError = true;
					}
				}
				else if (!strcmp(name.c_str(), D_SAMPLERATE))
				{
					int num, den;
					if (sscanf(value.c_str(), "%d:%d", &num, &den) == 2)
					{
						mHDRefaults.mSampleRate.value.numerator = num;
						mHDRefaults.mSampleRate.value.denominator = den;
						if (mHDRefaults.mSampleRate.value == FRAME_RATE_23976 ||
							mHDRefaults.mSampleRate.value == FRAME_RATE_2997  ||
							mHDRefaults.mSampleRate.value ==  FRAME_RATE_5994)
							mHDRefaults.mSampleRate.issetbyuser = true;
						else
						{
							fprintf(stderr, "invalid aspect ratio %u/%u, only 24000:1001, 30000:1001, 60000:1001 are allowed...\n", num, den);
							fatalError = true;
						}
					}
					else if (sscanf(value.c_str(), "%d", &num) == 1)
					{
						mHDRefaults.mSampleRate.value.numerator = num;
						mHDRefaults.mSampleRate.value.denominator = 1;
						if (mHDRefaults.mSampleRate.value == FRAME_RATE_24 ||
							mHDRefaults.mSampleRate.value == FRAME_RATE_25 ||
							mHDRefaults.mSampleRate.value == FRAME_RATE_30 ||
							mHDRefaults.mSampleRate.value == FRAME_RATE_50 ||
							mHDRefaults.mSampleRate.value == FRAME_RATE_50 )
							mHDRefaults.mSampleRate.issetbyuser = true;
						else
						{
							fprintf(stderr, "invalid aspect ratio %u/%u, only 24, 25, 30, 50, 60 are allowed...\n", num, den);
							fatalError = true;
						}
					}
					else
					{
						fprintf(stderr, "aspect ratio %s should be defined as n or n:m\n", value.c_str());
						fatalError = true;
					}
				}
				else 
				{
					mDfaultsMap.at(name)->value = atoi(value.c_str());
					mDfaultsMap.at(name)->issetbyuser = true;
				}				
			}

		}

		if (c == '\n')
			line_num++;
	}

	fclose(file);

	if (fatalError)
	{
		fprintf(stderr, "\ndefine values one per line\nkey: value (value will be trimmed)\
            \nall values except aspect ratio & sample rate are integers\nfor booleans named 'is...' use 0 or 1 or 2, 2=any\
            \nknown keys:\n\
            \n%s, %s, %s, %s, %s\n%s, %s, %s, %s, %s\n%s, %s, %s, %s, %s\n%s, %s, %s, %s, %s\n\n%s\n%s\n",
		    D_SHIMNAME ,       D_SINGLESEQUENCE , D_ASPECTRATIO ,   D_SAMPLERATE,       D_BITRATE,  \
			D_BITRATE_DELTA,   D_ISCONSTBITRATE , D_ISPROGRESSIVE,  D_UNIQUERESOLUTION, D_HSIZE,   \
			D_VSIZE,           D_CROMA ,          D_COLORPRIMARIES, D_ISTFF,            D_ISLOWDELAY,  \
			D_ISCLOSEDGOP,     D_CONSTGOP,        D_MAXGOP,         D_MAXBFRAMES,       D_CONSTBFRAMES,  \
			"known shims: ", mKnownShims) ;

		BMX_EXCEPTION(("input error"));
	}
}

void MPEG2LGWriterHelper::log_warn_descr_value(const DescriptorValue *dv)
{
	log_info(((dv->issetbyuser) ? "[user set] %s = %d\n" : "[shim set] %s = %d\n"), dv->name, dv->value);
}

void MPEG2LGWriterHelper::DoAllHeadersChecks(bool set_it_on, uint32_t maxLoggedViolations, bool verbose, bool looseChecks)
{
	mAllHeadersChecks = set_it_on;

	if (!mAllHeadersChecks)
		return;

	mLooseChecks = looseChecks;

	mMaxLoggedViolations = maxLoggedViolations;
	mPrintParsingReport  = verbose;

	if (verbose)
		log_info("descriptors control values\n");

	typedef std::map<std::string, DescriptorValue*>::iterator myIt; 
	for (myIt it=mDfaultsMap.begin(); it != mDfaultsMap.end(); it++)
	{
		if (it->first == D_SHIMNAME) continue;

		if(it->first == D_ASPECTRATIO)
		{ 
			if (! mHDRefaults.mAspectRatio.issetbyuser)
				mHDRefaults.mAspectRatio.value = mHDRefaults.mAspectRatio.defaultvalue;
			if (verbose)
			{
				if (mHDRefaults.mAspectRatio.value == ZERO_RATIONAL)
					log_info(((mHDRefaults.mAspectRatio.issetbyuser) ? "[user set] %s = %s\n" : "[shim set] %s = %s\n"), "any value");
				else
				   log_info(((mHDRefaults.mAspectRatio.issetbyuser) ? "[user set] %s = %d/%d\n" : "[shim set] %s = %d/%d\n"),
					 mHDRefaults.mAspectRatio.name, mHDRefaults.mAspectRatio.value.numerator, mHDRefaults.mAspectRatio.value.denominator);
			}
		}
		else if (it->first == D_SAMPLERATE)
		{
			if (!mHDRefaults.mSampleRate.issetbyuser)
				mHDRefaults.mSampleRate.value = mHDRefaults.mSampleRate.defaultvalue;
			if (verbose)
			{
				if (mHDRefaults.mSampleRate.value == ZERO_RATIONAL)
					log_info(((mHDRefaults.mSampleRate.issetbyuser) ? "[user set] %s = %s\n" : "[shim set] %s = %s\n"), mHDRefaults.mSampleRate.name, "any value");
				else
				   log_info(((mHDRefaults.mSampleRate.issetbyuser) ? "[user set] %s = %d/%d\n" : "[shim set] %s = %d/%d\n"),
					   mHDRefaults.mSampleRate.name, mHDRefaults.mSampleRate.value.numerator, mHDRefaults.mSampleRate.value.denominator);
			}
		}
		else
		{
			DescriptorValue *value = it->second;
			if (!value->issetbyuser)
				value->value = value->defaultvalue;
			if (verbose)
			  log_warn_descr_value(value);
		}		
	}   
}

void MPEG2LGWriterHelper::ProcessFrame(const unsigned char *data, uint32_t size)
{
	int  extType = 0 ;

	if (mAllHeadersChecks)
	   extType = mEssenceParser.ParseFrameAllInfo(data, size);
	else
    mEssenceParser.ParseFrameInfo(data, size);

    MPEGFrameType frame_type = mEssenceParser.GetFrameType();
    BMX_CHECK(frame_type != UNKNOWN_FRAME_TYPE);
    BMX_CHECK(mPosition > 0 || frame_type == I_FRAME); // require first frame to be an I-frame

    mHaveGOPHeader = mEssenceParser.HaveGOPHeader();

    if (mSingleSequence && mPosition > 0 && mEssenceParser.HaveSequenceHeader())
        mSingleSequence = false;

    if (mHaveGOPHeader) {
        if (!mEssenceParser.IsClosedGOP()) // descriptor closed GOP == true if all sequences are closed GOP
            mClosedGOP = false;
        mCurrentGOPClosed = mEssenceParser.IsClosedGOP();
    }

    if (frame_type == B_FRAME) {
        mBPictureCount++;
    } else if (mBPictureCount > 0) {
        if (mMaxBPictureCount > 0 && mBPictureCount != mMaxBPictureCount)
            mConstantBFrames = false;
        if (mBPictureCount > mMaxBPictureCount)
            mMaxBPictureCount = mBPictureCount;
        mBPictureCount = 0;
    }

    if (mHaveGOPHeader && !mUnlimitedGOPSize) {
        if (mPosition - mGOPStartPosition > UINT16_MAX) {
            mUnlimitedGOPSize = true;
            mMaxGOP = 0;
        } else {
            uint16_t gop_size = (uint16_t)(mPosition - mGOPStartPosition);
            if (gop_size > mMaxGOP)
                mMaxGOP = gop_size;
        }
    }

    if (mEssenceParser.HaveSequenceHeader()) {
        if (mLowDelay && !mEssenceParser.IsLowDelay())
            mLowDelay = false;
        mBitRate = mEssenceParser.GetBitRate() * 400; // MPEG-2 bit rate is in 400 bits/second units
    }

    if (mIdenticalGOP) {
        if (mFirstGOP && mHaveGOPHeader && mPosition > 0)
            mFirstGOP = false;

        if (mFirstGOP) {
            mGOPStructure.push_back(frame_type);
            if (mGOPStructure.size() >= 256) { // eg. max gop size for xdcam is 15
                log_warn("unexpected GOP size >= %" PRIszt "\n", mGOPStructure.size());
                mIdenticalGOP = false;
            }
        } else {
            size_t pos_in_gop = (mHaveGOPHeader ? 0 : (size_t)(mPosition - mGOPStartPosition));
            if (pos_in_gop >= mGOPStructure.size() || mGOPStructure[pos_in_gop] != frame_type)
                mIdenticalGOP = false;
        }
    }

    mKeyFrameOffset = 0;
    if (frame_type != I_FRAME) {
        if (!mCurrentGOPClosed && mKeyFramePosition + mKeyFrameTemporalReference >= mPosition) {
            BMX_CHECK(mPrevKeyFramePosition - mPosition >= -128);
            mKeyFrameOffset = (int8_t)(mPrevKeyFramePosition - mPosition);
        } else {
            BMX_CHECK(mKeyFramePosition - mPosition >= -128);
            mKeyFrameOffset = (int8_t)(mKeyFramePosition - mPosition);
        }
    }

    if (mHaveGOPHeader) {
        if (!CheckTemporalOffsetsComplete(0))
            log_warn("incomplete MPEG-2 temporal offset data in index table\n");

        mGOPStartPosition = mPosition;
        memset(mGOPTemporalOffsets, NULL_TEMPORAL_OFFSET, sizeof(mGOPTemporalOffsets));
    }
    BMX_CHECK(mPosition - mGOPStartPosition <= UINT8_MAX);
    uint8_t gop_start_offset = (uint8_t)(mPosition - mGOPStartPosition);

    // temporal reference = display position for current frame
    mTemporalReference = mEssenceParser.GetTemporalReference();

    // temporal offset = offset to frame data required for displaying at the current position
    BMX_CHECK(mTemporalReference < sizeof(mGOPTemporalOffsets));
    BMX_CHECK(gop_start_offset - (int64_t)mTemporalReference <= 127 &&
              gop_start_offset - (int64_t)mTemporalReference >= -128);
    mGOPTemporalOffsets[mTemporalReference] = (int8_t)(gop_start_offset - mTemporalReference);

    mHaveTemporalOffset = (mGOPTemporalOffsets[gop_start_offset] != NULL_TEMPORAL_OFFSET);
    mTemporalOffset = mGOPTemporalOffsets[gop_start_offset];

    mHavePrevTemporalOffset = false;
    if (mTemporalReference < gop_start_offset) {
        // a temporal offset value for a previous position is now available
        mHavePrevTemporalOffset = true;
        mPrevTemporalOffset = mGOPTemporalOffsets[mTemporalReference];
    }

    mFlags = 0x00;
    if (mEssenceParser.HaveSequenceHeader())
        mFlags |= 0x40; // sequence header bit
    if (frame_type == I_FRAME) {
        // according to SMPTE ST-381 bit 7 shall not be set if the GOP is Open. However, in Avid OP-Atom files
        // this bit must be set because otherwise it assumes the precharge is right back to the first frame
        // (which has a Closed GOP), but doesn't get the index table correct resulting in this error:
        //      Exception: MXF_DIDMapper::ReadRange - End Sample Index exceeds on-disk Index Entry Count.
        if (mEssenceParser.HaveSequenceHeader() &&
            (mFlavour == AVID_FLAVOUR || (mHaveGOPHeader && mEssenceParser.IsClosedGOP())))
        {
            mFlags |= 0x80; // reference frame bit
        }
    } else if (frame_type == P_FRAME) {
        mFlags |= 0x22; // naive setting - assume forward prediction
    } else {
        if (mCurrentGOPClosed && mTemporalReference + 1 == mBPictureCount && mFlavour != AVID_FLAVOUR)
            mFlags |= 0x13; // B frames commence closed GOP - assume backward prediction
        else
            mFlags |= 0x33; // naive setting - assume forward and backward prediction
    }
    if (mKeyFrameOffset + mPosition < 0 || mTemporalOffset + mPosition < 0)
        mFlags |= 0x0b; // offsets out of range


    if (frame_type == I_FRAME) {
        mPrevKeyFramePosition = mKeyFramePosition;
        mKeyFramePosition = mPosition;
        mKeyFrameTemporalReference = mTemporalReference;
    }

	if (mAllHeadersChecks && extType != 0)
		AllHeadersChecks();

    mPosition++;
}

void MPEG2LGWriterHelper::AllHeadersChecks()
{
    //check MPEG2LGMXFDescriptorHelper.cpp for header settings

	bool fatalError = false;

	if (mHDRefaults.mConstBitRate.value == 1 && mEssenceParser.GetVBVDelay() == 0xffffffff)
	{
		if (mHDRefaults.mConstBitRate.nlogged++ <= mMaxLoggedViolations)
		  log_warn("constant bitrate of mpeg essence is required...\n");
	}

	if (mEssenceParser.HaveSequenceHeader())
	{
		mSequenceHeaderChk = true;

		if ((mHDRefaults.mSingleSequence.value != D_BOOL_IS_ANY) && ((uint32_t)mSingleSequence != mHDRefaults.mSingleSequence.value))
		{
			if (mHDRefaults.mSingleSequence.nlogged++ <= mMaxLoggedViolations)
			{
				log_warn("mpeg single seqence requirement is violated, value should be set to %s...\n", ((mHDRefaults.mSingleSequence.value == 1) ? "single" : "any"));
			}
			//else fatalError = true;
		}

		if (mEssenceParser.HaveKnownAspectRatio())
		{
			Rational mAspectRatio = mEssenceParser.GetAspectRatio();
			if (mHDRefaults.mAspectRatio.value != D_RATIONAL_IS_ANY && mAspectRatio != mHDRefaults.mAspectRatio.value)
			{
				log_error("found aspect ratio %u/%u, only %d/%d is allowed...\n", mAspectRatio.numerator, mAspectRatio.denominator,
					                                                              mHDRefaults.mAspectRatio.value.numerator, mHDRefaults.mAspectRatio.value.denominator);
				fatalError = true;
			}
		}

		if (mEssenceParser.HaveKnownFramRate())
		{
			Rational mSampleRate = mEssenceParser.GetFrameRate();
			if (mHDRefaults.mSampleRate.value != D_RATIONAL_IS_ANY && mSampleRate != mHDRefaults.mSampleRate.value)
			{
				log_error("found frame rate %u/%u, expected %d/%d...\n", mSampleRate.numerator, mSampleRate.denominator,
					mHDRefaults.mSampleRate.value.numerator, mHDRefaults.mSampleRate.value.denominator);
				fatalError = true;
			}
		}

		if (std::abs( ((long)( mBitRate - mHDRefaults.mBitRate.value) ) ) > (long)mHDRefaults.mBitRateDelta.value)
		{
			if (mHDRefaults.mBitRate.nlogged++ <= mMaxLoggedViolations)
			{
				log_warn("bitrate %u is not equal (whithin the margin of error) to AS10 shim requiret bitrate %u...\n", mBitRate, mHDRefaults.mBitRate.value);
			}
		}

		uint32_t mHorizontalSize = mEssenceParser.GetHorizontalSize();
		uint32_t mVerticalSize = mEssenceParser.GetVerticalSize();

		if (mHDRefaults.mUniqShimHVSize.value == 1 &&
			((mHorizontalSize != mHDRefaults.mHorizontalSize.value) || (mVerticalSize != mHDRefaults.mVerticalSize.value)))
		{
			log_error("resolution can only be %ux%u, but found %ux%u...\n", mHDRefaults.mHorizontalSize.value, mHDRefaults.mVerticalSize.value, mHorizontalSize, mVerticalSize);
			fatalError = true;
		}

		if ((mHDRefaults.mLowDelay.value != D_BOOL_IS_ANY) && (mEssenceParser.IsLowDelay() != (bool)mHDRefaults.mLowDelay.value))
		{
			if (mHDRefaults.mLowDelay.nlogged++ <= mMaxLoggedViolations)
			{
				log_warn("mpeg essence low delay value is required to be %u...\n", mHDRefaults.mLowDelay.value);
			}
			else
			{
				fatalError = true;
			}
		}
	}

	if (mEssenceParser.HaveSequenceExtention())
	{
		mSequenceExtentionChk = true;

		uint8_t  mProfileAndLevel = mEssenceParser.GetProfileAndLevel();
		uint32_t mHorizontalSize = mEssenceParser.GetHorizontalSize();
		uint32_t mVerticalSize = mEssenceParser.GetVerticalSize();
		uint32_t mChromaFormat = 420;

		if (mEssenceParser.IsProgressive())
		{
			if (mHDRefaults.mIsProgressive.value == 0)
			{
				log_error("expecting interlaced content only!\n");
				fatalError = true;
			}

			if (mVerticalSize != 1080 && mVerticalSize != 720)
			{
				log_error("unexpected MPEG-2 Long GOP vertical size %u; should be 1080 or 720\n", mVerticalSize);
				fatalError = true;
			}
			if (mHorizontalSize == 1920)
			{
				switch (mProfileAndLevel)
				{
				case 0x82:
					mEssenceType = MPEG2LG_422P_HL_1080P;
					mChromaFormat = 422;
					break;
				case 0x44:
					mEssenceType = MPEG2LG_MP_HL_1920_1080P;
					break;
				default:
					log_error("unexpected MPEG-2 Long GOP profile and level %u\n", mProfileAndLevel);
					fatalError = true;
				}
			}
			else if (mHorizontalSize == 1440)
			{
				switch (mProfileAndLevel)
				{
				case 0x44:
					mEssenceType = MPEG2LG_MP_HL_1440_1080P;
					break;
				case 0x46:
					mEssenceType = MPEG2LG_MP_H14_1080P;
					break;
				default:
					log_error("unexpected MPEG-2 Long GOP profile and level %u\n", mProfileAndLevel);
					fatalError = true;
				}
			}
			else if (mHorizontalSize == 1280)
			{
				switch (mProfileAndLevel)
				{
				case 0x82:
					mEssenceType = MPEG2LG_422P_HL_720P; 
					mChromaFormat = 422;
					break;
				case 0x44:
					mEssenceType = MPEG2LG_MP_HL_720P;
					break;
				default:
					log_error("unexpected MPEG-2 Long GOP profile and level %u\n", mProfileAndLevel);
					fatalError = true;
				}
			}
			else
			{
				log_error("unexpected MPEG-2 Long GOP horizontal size %u; should be 1920 or 1440 or 1280\n", mHorizontalSize);
				fatalError = true;
			}
		}
		else
		{
			if (mHDRefaults.mIsProgressive.value == 1)
			{
				log_error("expecting progressive content only!\n");
				fatalError = true;
			}

			if (mVerticalSize != 1080)
			{
				log_error("unexpected MPEG-2 Long GOP vertical size %u; should be 1080\n", mVerticalSize);
				fatalError = true;
			}
			if (mHorizontalSize == 1920)
			{
				switch (mProfileAndLevel)
				{
				case 0x82:
					mEssenceType = MPEG2LG_422P_HL_1080I;
					mChromaFormat = 422;
					break;
				case 0x44:
					mEssenceType = MPEG2LG_MP_HL_1920_1080I;
					break;
				default:
					log_error("unknown MPEG-2 Long GOP profile and level %u\n", mProfileAndLevel);
					fatalError = true;
				}
			}
			else if (mHorizontalSize == 1440)
			{
				switch (mProfileAndLevel)
				{
				case 0x44:
					mEssenceType = MPEG2LG_MP_HL_1440_1080I;
					break;
				case 0x46:
					mEssenceType = MPEG2LG_MP_H14_1080I;
					break;
				default:
					log_error("unknown MPEG-2 Long GOP profile and level %u\n", mProfileAndLevel);
					fatalError = true;
				}
			}
			else
			{
				log_error("unexpected MPEG-2 Long GOP horizontal size %u; should be 1920 or 1440\n", mHorizontalSize);
				fatalError = true;
			}
		}

		if (mHDRefaults.mUniqShimHVSize.value == 1 &&
			((mHorizontalSize != mHDRefaults.mHorizontalSize.value) || (mVerticalSize != mHDRefaults.mVerticalSize.value)))
		{
			log_error("resolution can only be %ux%u, but found %ux%u...\n", mHDRefaults.mHorizontalSize.value, mHDRefaults.mVerticalSize.value, mHorizontalSize, mVerticalSize);
			fatalError = true;
		}

		if (mChromaFormat != mHDRefaults.mChromaFormat.value)
		{
			log_error("croma format inferred form essence (profil & level) is %u, should be %u...\n", mChromaFormat, mHDRefaults.mChromaFormat.value);
			fatalError = true;
		}

		mChromaFormat = mEssenceParser.GetCromaFormat();

		if (mChromaFormat != 0 && mChromaFormat != mHDRefaults.mChromaFormat.value)
		{
			log_error("expecting croma format %u, found %u...\n", mHDRefaults.mChromaFormat.value, mChromaFormat);
			fatalError = true;
		}
	}

	if (mEssenceParser.HaveDisplayExtention())
	{
		mDisplayExtentionChk = true;

		//uint32_t mVideoFormat = mEssenceParser.GetVideoFormat();

		if (mEssenceParser.HaveColorDescription())
		{
			uint32_t mColorPrimaries = mEssenceParser.GetColorPrimaries();
			uint32_t mTransferCharacteristics = mEssenceParser.GetTransferCharacteristics();
			uint32_t mMatrixCoeffs = mEssenceParser.GetMatrixCoeffs();

			if (!(mColorPrimaries == mTransferCharacteristics == mMatrixCoeffs == mHDRefaults.mColorPrimaries.value))
			{
				if (mHDRefaults.mColorPrimaries.nlogged++ <= mMaxLoggedViolations)
				{
					if (mHDRefaults.mColorPrimaries.value == 1)
						log_warn("expecting color primaries BT.709, SMPTE 274M, found %s...\n", mColorPrimaries);
					else
						log_warn("expecting color primaries %s, found %s...\n", mHDRefaults.mColorPrimaries.value, mColorPrimaries);
				}
				else
				{
					fatalError = true;
				}
			}
		}

		uint32_t mDHorizontalSize = mEssenceParser.GetDHorizontalSize();
		uint32_t mDVerticalSize = mEssenceParser.GetDVerticalSize();

		if (mHDRefaults.mUniqShimHVSize.value == 1 &&
			((mDHorizontalSize != mHDRefaults.mHorizontalSize.value) || (mDVerticalSize != mHDRefaults.mVerticalSize.value)))
		{
			if (mHDRefaults.mUniqShimHVSize.nlogged++ <= mMaxLoggedViolations)
			{
				log_warn("display resolution  %ux%u differs from video resolution %ux%u...\n", mDHorizontalSize, mDVerticalSize,
					          mHDRefaults.mHorizontalSize.value, mHDRefaults.mVerticalSize.value);
			}
			else
			{
				fatalError = true;
			}
		}

	}
	  
	if (mEssenceParser.HavePicCodingExtention())
	{
		mPicCodingExtentionChk = true;

		if ((mHDRefaults.mTFF.value != D_BOOL_IS_ANY) && (mEssenceParser.HaveSequenceExtention() && !mEssenceParser.IsProgressive()))
		{
			if (mEssenceParser.GetTFF() != mHDRefaults.mTFF.value)
			{
				log_error("expecting interlaced content with %s frame first, which is not the case\n", (mHDRefaults.mTFF.value != 0) ? "top": "bottom");
				fatalError = true;
			}
		}
	}

    if (mHaveGOPHeader)
	{
		if (mMaxGOP > mHDRefaults.mMaxGOP.value)
		{
			log_error("max gop %u is more than max %u allowed\n", mMaxGOP, mHDRefaults.mMaxGOP.value);
			fatalError = true;
		}

		if ((mHDRefaults.mClosedGOP.value != D_BOOL_IS_ANY) && (mClosedGOP != (bool)mHDRefaults.mClosedGOP.value))
		{
			log_error("gop doesnt match reguiremet: %s\n", (mHDRefaults.mClosedGOP.value == 1) ? "closed" : "open");
			fatalError = true;
		}

		if ((mHDRefaults.mIdenticalGOP.value != D_BOOL_IS_ANY) && (!mIdenticalGOP && (mIdenticalGOP != (bool)mHDRefaults.mIdenticalGOP.value)))
		{
			log_error("gop is not constant\n");
			fatalError = true;
		}
	}

	if (fatalError)
	{
		if(!mLooseChecks)
		  BMX_EXCEPTION(("\n\nincorrect value(s) in frame descriptior(s), possible shim/norm violation\n"));
	}

	return;
}

void MPEG2LGWriterHelper::modify_descriptors(std::vector<descriptor> &mxf_descriptors, const char *name, const int &modified, const char *value)
{
	for (size_t i = 0; i < mxf_descriptors.size(); i++)
	{
		if (mxf_descriptors[i].name == name)
		{
			mxf_descriptors[i].modified = modified;
			mxf_descriptors[i].value = value;
		}
	}
}

//as-10 targeted 
void MPEG2LGWriterHelper::ReportCheckedHeaders()
{
	if (! (mAllHeadersChecks && mPrintParsingReport) ) return;

	stringstream ss;
	std::string report = "checked mpeg descriptors in headers:\npicture header 01000";	
	std::string tmp; //to make mac's clang++ happy

	modify_descriptors(mMXFDescriptors, "SingleSequence", 1, ((mSingleSequence) ? "single" : "multi"));
	modify_descriptors(mMXFDescriptors, "ConstantBframe", 1, ((mConstantBFrames) ? "const" : "variable"));
	modify_descriptors(mMXFDescriptors, "IdenticalGOP", 1, ((mIdenticalGOP) ? "const" : "variable"));
	modify_descriptors(mMXFDescriptors, "ClosedGOP", 1, ((mClosedGOP) ? "closed" : "open"));

	uint16_t mgop = mMaxGOP;
	if (mgop==0)
	{   switch (mEssenceType)
		{
			case MPEG2LG_422P_HL_720P:
			case MPEG2LG_MP_HL_720P:  mgop = 12; break;
			default: mgop = 16;
		}
	}
	ss.str(std::string());  
	ss << mgop; tmp = ss.str(); ss.str(std::string());
	modify_descriptors(mMXFDescriptors, "MaxGOP", 1, tmp.c_str()); 

	ss << mMaxBPictureCount; tmp = ss.str(); ss.str(std::string());
    modify_descriptors(mMXFDescriptors, "BPictureCount", 1, tmp.c_str()); 

	if (mSequenceHeaderChk)
	{
		report += "\nsequence header 01b3";
		Rational r = (mEssenceParser.HaveKnownFramRate() ? mEssenceParser.GetFrameRate() : ZERO_RATIONAL);
		ss <<  r.numerator << "/" << r.denominator; tmp = ss.str(); ss.str(std::string());
		modify_descriptors(mMXFDescriptors, "SampleRate", 1, tmp.c_str()); 
		modify_descriptors(mMXFDescriptors, "FrameLayout", 1, (mEssenceParser.IsProgressive() ? "progressive" : "interlaced"));
		ss << mEssenceParser.GetHorizontalSize(); tmp = ss.str(); ss.str(std::string());
	    modify_descriptors(mMXFDescriptors, "StoredWidth", 1, tmp.c_str()); 
		ss << mEssenceParser.GetVerticalSize(); tmp = ss.str(); ss.str(std::string());
		modify_descriptors(mMXFDescriptors, "StoredHeight", 1, tmp.c_str()); 
		r = (mEssenceParser.HaveKnownAspectRatio() ? mEssenceParser.GetAspectRatio() : ASPECT_RATIO_16_9);
		ss << r.numerator << "/" << r.denominator ; tmp = ss.str(); ss.str(std::string());
		modify_descriptors(mMXFDescriptors, "AspectRatio", 1, tmp.c_str()); 
		
		ss << mBitRate;  tmp = ss.str(); ss.str(std::string());
	    modify_descriptors(mMXFDescriptors, "BitRate", 1, tmp.c_str());
	}

	if (mSequenceExtentionChk)
	{
		report += "\nsequence extension 01b5/0001";
		modify_descriptors(mMXFDescriptors, "SignalStandard", 1, "SMPTE-274M");

		std::string mSampledWidth, mStoredWidth,     mDisplayWidth,
				    mSampledHeight, mDisplayHeight,  mStoredHeight, mVideoLineMap; 

		switch (mEssenceType)
		{
			case MPEG2LG_422P_HL_1080I:
			case MPEG2LG_MP_HL_1920_1080I:
			mSampledWidth  = mStoredWidth = mDisplayWidth = "1920" ;
			mSampledHeight = mDisplayHeight = "540";
			mStoredHeight  = "544";
			mVideoLineMap  = "(21,584)";
			break;
			case MPEG2LG_422P_HL_1080P:
			case MPEG2LG_MP_HL_1920_1080P:
			mSampledWidth  = mStoredWidth = mDisplayWidth = "1920";
			mStoredHeight  = "1088";
			mDisplayHeight = "1080";
			mVideoLineMap  = "(42,0)";
			break;
			case MPEG2LG_422P_HL_720P:
			case MPEG2LG_MP_HL_720P:
			mSampledWidth  = mStoredWidth  = mDisplayWidth = "1280";
			mSampledHeight = mStoredHeight = mDisplayHeight = "720";
			mVideoLineMap  = "(26,0)";
			modify_descriptors(mMXFDescriptors, "SignalStandard", 1, "SMPTE-296M");
			break;
			case MPEG2LG_MP_HL_1440_1080I:
			case MPEG2LG_MP_H14_1080I:
			mSampledWidth  = mStoredWidth = mDisplayWidth = "1440";
			mSampledHeight = mDisplayHeight = "540" ;
			mStoredHeight  = "544";
			mVideoLineMap  = "(21,584)";
			break;
			case MPEG2LG_MP_HL_1440_1080P:
			case MPEG2LG_MP_H14_1080P:
			mSampledWidth  = mStoredWidth = mDisplayWidth = "1440" ;
			mSampledHeight = mDisplayHeight = "1080" ;
			mStoredHeight  = "1088";
			mVideoLineMap  = "(42,0)";
			break;
			default:
				mSampledWidth  = mStoredWidth   = mDisplayWidth =
				mSampledHeight = mDisplayHeight = mStoredHeight = mVideoLineMap = "mxf descriptor copy";
			break;
		}
	    modify_descriptors(mMXFDescriptors, "SampledWidth", 1, mSampledWidth.c_str());
		modify_descriptors(mMXFDescriptors, "SampledHeight", 1, mSampledHeight.c_str());
		modify_descriptors(mMXFDescriptors, "StoredWidth", 1, mStoredWidth.c_str());
		modify_descriptors(mMXFDescriptors, "StoredHeight", 1, mStoredHeight.c_str());
		modify_descriptors(mMXFDescriptors, "VideoLineMap", 1, mVideoLineMap.c_str());
		modify_descriptors(mMXFDescriptors, "DisplayHeight", 1, mDisplayHeight.c_str());
		modify_descriptors(mMXFDescriptors, "DisplayWidth", 1, mDisplayWidth.c_str());
		ss.str(std::string()); ss << mEssenceParser.GetCromaFormat();  tmp = ss.str(); ss.str(std::string());
		modify_descriptors(mMXFDescriptors, "HorizontalSubsampling", 1, tmp.c_str());
		modify_descriptors(mMXFDescriptors, "VerticalSubsampling", 1, tmp.c_str());
		modify_descriptors(mMXFDescriptors, "ColorSiting", 1, tmp.c_str());
		modify_descriptors(mMXFDescriptors, "CodedContentType", 1, (mEssenceParser.IsProgressive() ? "progressive" : "interlaced"));
		modify_descriptors(mMXFDescriptors, "LowDelay", 1, (mEssenceParser.IsLowDelay() ? "true" : "false"));
		switch (mEssenceParser.GetProfileAndLevel())
		{
		  case 0x82: mSampledHeight = "422@HL_LGOP" ;  mSampledWidth = "422@HL(0x82)";  break;
		  case 0x46: mSampledHeight = "MP@H-14_LGOP";  mSampledWidth = "MP@H-14(0x46)"; break;
		  case 0x44: mSampledHeight = "MP@HL_LGOP"  ;  mSampledWidth = "MP@HL(0x44)";   break;
		  default:   mSampledHeight = "unknown";       mSampledWidth = "unknown";
		}
		modify_descriptors(mMXFDescriptors, "ProfileAndLevel", 1, mSampledWidth.c_str());
		modify_descriptors(mMXFDescriptors, "PictureEssenceCodinng", 1, mSampledHeight.c_str());
	}
		
	if (mDisplayExtentionChk)
	{
		report += "\ndisplay extension 01b5/0010";
		if (mEssenceParser.HaveColorDescription())
		{   ss.str(std::string());
			switch (mEssenceParser.GetColorPrimaries())
			{
				case 0x0001: ss << "BT.709/SMPTE-274M"; break;
				case 0x0101: ss << "BT.470B,G,I";       break;
				case 0x0110: ss << "SMPTE-170M";        break;
				case 0x0111: ss << "SMPTE-240M";        break;
				default: ss << mEssenceParser.GetColorPrimaries();
			}
			tmp = ss.str(); ss.str(std::string());
			modify_descriptors(mMXFDescriptors, "CaptureGamma", 1, tmp.c_str());
		}
	}
		
	if (mPicCodingExtentionChk)
		report += "\npicture conding extension 01b5/1000";
		
		
	report += "\n";

	log_info(report.c_str()) ;

	for (size_t i = 0; i < mMXFDescriptors.size(); i++)
	{   std::string s ; 
		switch (mMXFDescriptors[i].modified)
		{  case -1 :  s = "[std]"; break;
			case 0 :  s = "[mxf]"; break;
			case 1 :  s = "[mpg]"; break; 
		}
		s += " "; s += mMXFDescriptors[i].name;
		if (mMXFDescriptors[i].modified != 0)
		{
			s += "="; s += mMXFDescriptors[i].value ;
		}
		s += "\n";
		log_info(s.c_str());
	}
}

bool MPEG2LGWriterHelper::CheckTemporalOffsetsComplete(int64_t end_offset)
{
    // check all temporal offsets are known (end_offset is <= 0)
    int64_t i;
    for (i = 0; i < mPosition - mGOPStartPosition + end_offset; i++) {
        if (mGOPTemporalOffsets[i] == NULL_TEMPORAL_OFFSET)
            return false;
    }

    return true;
}
