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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <map>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include <bmx/as10/AS10MPEG2Validator.h>
#include "as10_shims.h"
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace mxfpp;
using namespace bmx;


AS10MPEG2Validator::AS10MPEG2Validator(AS10Shim as10_shim, const char *fname, int max_same_warn_messages,
                                       bool print_defaults, bool loose_checks)
: MPEG2Validator()
{
    mShim = as10_shim;
    mFirstFrame = true;
    mEssenceType = UNKNOWN_ESSENCE_TYPE;
    mSequenceHeaderChk = mSequenceExtensionChk = mDisplayExtensionChk =
    mPicCodingExtensionChk = mAllHeadersChecks = mPrintParsingReport = mLooseChecks = false;
    mHDRefaults = as10_high_hd_2014_mpeg_values;
    AssignMapVals();

    for (int i = 0; (AS10_MXF_Descriptors_Arr[i]).name != ""; i++ )
    {
        mMXFDescriptors.push_back(AS10_MXF_Descriptors_Arr[i]);
    }


    if (as10_shim != AS10_UNKNOWN_SHIM)
        shim(as10_shim);

    if (fname != NULL)
        ParseDescriptorRefValues(fname);

    DoAllHeadersChecks(true, max_same_warn_messages, print_defaults, loose_checks);

}

AS10MPEG2Validator::~AS10MPEG2Validator()
{
}

void AS10MPEG2Validator::ProcessFrame(const unsigned char *data, uint32_t size)
{
    if (mFirstFrame) {
        if (mShim == AS10_HIGH_HD_2014) {
            EssenceType essence_type = mDescHelper->GetEssenceType();
            if (essence_type != MPEG2LG_422P_HL_1080I &&
                essence_type != MPEG2LG_422P_HL_1080P)
            {
                log_error("as10 high hd shim requires MPEG-2 4:2:2 HL coding\n");
                BMX_EXCEPTION(("shim spec violation(s) in mxf descriptor(s), incorrect essence"));
            }
        }

        mFirstFrame = false;
    }

    if (!mAllHeadersChecks)
        return;

    mEssenceParser.ParseFrameAllInfo(data, size);
    if (mEssenceParser.HaveSequenceHeader() || mEssenceParser.HaveExtension())
        AllHeadersChecks();
}

void AS10MPEG2Validator::CompleteWrite()
{
    ReportCheckedHeaders();
}

void AS10MPEG2Validator::shim(AS10Shim as10_shim)
{
    switch (as10_shim)
    {
        case AS10_HIGH_HD_2014:
            mHDRefaults = as10_high_hd_2014_mpeg_values;
            break;
        case AS10_CNN_HD_2012:
            mHDRefaults = as10_cnn_hd_2012_mpeg_values;
            break;
        case AS10_NRK_HD_2012:
            mHDRefaults = as10_nrk_hd_2012_mpeg_values;
            break;
        case AS10_JVC_HD_35_VBR_2012:
            mHDRefaults = as10_jvc_hd_35_vbr_2012_mpeg_values;
            break;
        case AS10_JVC_HD_25_CBR_2012:
            mHDRefaults = as10_jvc_hd_25_cbr_2012_mpeg_values;
            break;
        case AS10_UNKNOWN_SHIM:
            fprintf(stderr, "unknown shim, known shims: %s...\n", get_as10_shim_names().c_str());
            BMX_EXCEPTION(("input error"));
    }

    AssignMapVals();
}

void AS10MPEG2Validator::AssignMapVals()
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

void AS10MPEG2Validator::ParseDescriptorRefValues(const char *filename)
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

            name = trim_string(name);
            value = trim_string(value);

            if (mDfaultsMap.find(name) == mDfaultsMap.end())
            {
                fprintf(stderr, "unknown key %s on line %d\n", name.c_str(), line_num);
                fatalError = true;
            }
            else
            {
                if (! strcmp(name.c_str(), D_SHIMNAME))
                {
                    AS10Shim shim = get_as10_shim(value);
                    switch (shim)
                    {
                        case AS10_HIGH_HD_2014:
                            mHDRefaults = as10_high_hd_2014_mpeg_values;
                            break;
                        case AS10_CNN_HD_2012:
                            mHDRefaults = as10_cnn_hd_2012_mpeg_values;
                            break;
                        case AS10_NRK_HD_2012:
                            mHDRefaults = as10_nrk_hd_2012_mpeg_values;
                            break;
                        case AS10_JVC_HD_35_VBR_2012:
                            mHDRefaults = as10_jvc_hd_35_vbr_2012_mpeg_values;
                            break;
                        case AS10_JVC_HD_25_CBR_2012:
                            mHDRefaults = as10_jvc_hd_25_cbr_2012_mpeg_values;
                            break;
                        case AS10_UNKNOWN_SHIM:
                            fprintf(stderr, "unknown shim name '%s', select one of these: %s...\n", value.c_str(), get_as10_shim_names().c_str());
                            fatalError = true;
                    }

                    if (!fatalError)
                        log_info("using shim %s\n", get_as10_shim_name(mHDRefaults.shim));
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
                            fprintf(stderr, "invalid sample rate %u/%u, only 24000:1001, 30000:1001, 60000:1001 are allowed...\n", num, den);
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
                            fprintf(stderr, "invalid sample rate %u/%u, only 24, 25, 30, 50, 60 are allowed...\n", num, den);
                            fatalError = true;
                        }
                    }
                    else
                    {
                        fprintf(stderr, "sample rate %s should be defined as n or n:m\n", value.c_str());
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
            "known shims: ", get_as10_shim_names().c_str()) ;

        BMX_EXCEPTION(("input error"));
    }
}

void AS10MPEG2Validator::log_warn_descr_value(const DescriptorValue *dv)
{
    log_info(((dv->issetbyuser) ? "[user set] %s = %d\n" : "[shim set] %s = %d\n"), dv->name, dv->value);
}

void AS10MPEG2Validator::DoAllHeadersChecks(bool set_it_on, uint32_t maxLoggedViolations, bool verbose, bool loose_checks)
{
    mAllHeadersChecks = set_it_on;

    if (!mAllHeadersChecks)
        return;

    mLooseChecks = loose_checks;

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

void AS10MPEG2Validator::AllHeadersChecks()
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

        if ((mHDRefaults.mSingleSequence.value != D_BOOL_IS_ANY) && ((uint32_t)mWriterHelper->GetSingleSequence() != mHDRefaults.mSingleSequence.value))
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

        if (mEssenceParser.HaveKnownFrameRate())
        {
            Rational mSampleRate = mEssenceParser.GetFrameRate();
            if (mHDRefaults.mSampleRate.value != D_RATIONAL_IS_ANY && mSampleRate != mHDRefaults.mSampleRate.value)
            {
                log_error("found frame rate %u/%u, expected %d/%d...\n", mSampleRate.numerator, mSampleRate.denominator,
                    mHDRefaults.mSampleRate.value.numerator, mHDRefaults.mSampleRate.value.denominator);
                fatalError = true;
            }
        }

        if (std::abs( ((long)( mWriterHelper->GetBitRate() - mHDRefaults.mBitRate.value) ) ) > (long)mHDRefaults.mBitRateDelta.value)
        {
            if (mHDRefaults.mBitRate.nlogged++ <= mMaxLoggedViolations)
            {
                log_warn("bitrate %u is not equal (whithin the margin of error) to AS10 shim requiret bitrate %u...\n", mWriterHelper->GetBitRate(), mHDRefaults.mBitRate.value);
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

    if (mEssenceParser.HaveSequenceExtension())
    {
        mSequenceExtensionChk = true;

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

        mChromaFormat = mEssenceParser.GetChromaFormat();

        if (mChromaFormat != 0 && mChromaFormat != mHDRefaults.mChromaFormat.value)
        {
            log_error("expecting croma format %u, found %u...\n", mHDRefaults.mChromaFormat.value, mChromaFormat);
            fatalError = true;
        }
    }

    if (mEssenceParser.HaveDisplayExtension())
    {
        mDisplayExtensionChk = true;

        //uint32_t mVideoFormat = mEssenceParser.GetVideoFormat();

        if (mEssenceParser.HaveColorDescription())
        {
            uint32_t mColorPrimaries = mEssenceParser.GetColorPrimaries();
            //uint32_t mTransferCharacteristics = mEssenceParser.GetTransferCharacteristics();
            //uint32_t mMatrixCoeffs = mEssenceParser.GetMatrixCoeffs();

            if (mColorPrimaries != mHDRefaults.mColorPrimaries.value)
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

    if (mEssenceParser.HavePicCodingExtension())
    {
        mPicCodingExtensionChk = true;

        if ((mHDRefaults.mTFF.value != D_BOOL_IS_ANY) && (mEssenceParser.HaveSequenceExtension() && !mEssenceParser.IsProgressive()))
        {
            if (mEssenceParser.IsTFF() != (bool)mHDRefaults.mTFF.value)
            {
                log_error("expecting interlaced content with %s frame first, which is not the case\n", (mHDRefaults.mTFF.value != 0) ? "top": "bottom");
                fatalError = true;
            }
        }
    }

    if (mWriterHelper->HaveGOPHeader())
    {
        if (mWriterHelper->GetMaxGOP() > mHDRefaults.mMaxGOP.value)
        {
            log_error("max gop %u is more than max %u allowed\n", mWriterHelper->GetMaxGOP(), mHDRefaults.mMaxGOP.value);
            fatalError = true;
        }

        if ((mHDRefaults.mClosedGOP.value != D_BOOL_IS_ANY) && (mWriterHelper->GetClosedGOP() != (bool)mHDRefaults.mClosedGOP.value))
        {
            log_error("gop doesnt match reguiremet: %s\n", (mHDRefaults.mClosedGOP.value == 1) ? "closed" : "open");
            fatalError = true;
        }

        if ((mHDRefaults.mIdenticalGOP.value != D_BOOL_IS_ANY) && (!mWriterHelper->GetIdenticalGOP() && (mWriterHelper->GetIdenticalGOP() != (bool)mHDRefaults.mIdenticalGOP.value)))
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

void AS10MPEG2Validator::modify_descriptors(std::vector<descriptor> &mxf_descriptors, const char *name, const int &modified, const char *value)
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
void AS10MPEG2Validator::ReportCheckedHeaders()
{
    if (! (mAllHeadersChecks && mPrintParsingReport) ) return;

    stringstream ss;
    std::string report = "checked mpeg descriptors in headers:\npicture header 01000";
    std::string tmp; //to make mac's clang++ happy

    modify_descriptors(mMXFDescriptors, "SingleSequence", 1, ((mWriterHelper->GetSingleSequence()) ? "single" : "multi"));
    modify_descriptors(mMXFDescriptors, "ConstantBframe", 1, ((mWriterHelper->GetConstantBFrames()) ? "const" : "variable"));
    modify_descriptors(mMXFDescriptors, "IdenticalGOP", 1, ((mWriterHelper->GetIdenticalGOP()) ? "const" : "variable"));
    modify_descriptors(mMXFDescriptors, "ClosedGOP", 1, ((mWriterHelper->GetClosedGOP()) ? "closed" : "open"));

    uint16_t mgop = mWriterHelper->GetMaxGOP();
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

    ss << mWriterHelper->GetMaxBPictureCount(); tmp = ss.str(); ss.str(std::string());
    modify_descriptors(mMXFDescriptors, "BPictureCount", 1, tmp.c_str());

    if (mSequenceHeaderChk)
    {
        report += "\nsequence header 01b3";
        Rational r = (mEssenceParser.HaveKnownFrameRate() ? mEssenceParser.GetFrameRate() : ZERO_RATIONAL);
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

        ss << mWriterHelper->GetBitRate();  tmp = ss.str(); ss.str(std::string());
        modify_descriptors(mMXFDescriptors, "BitRate", 1, tmp.c_str());
    }

    if (mSequenceExtensionChk)
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
        ss.str(std::string()); ss << mEssenceParser.GetChromaFormat();  tmp = ss.str(); ss.str(std::string());
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

    if (mDisplayExtensionChk)
    {
        report += "\ndisplay extension 01b5/0010";
        if (mEssenceParser.HaveColorDescription())
        {   ss.str(std::string());
            switch (mEssenceParser.GetColorPrimaries())
            {
                case 1: ss << "BT.709/SMPTE-274M"; break;
                case 5: ss << "BT.470B,G,I";       break;
                case 6: ss << "SMPTE-170M";        break;
                case 7: ss << "SMPTE-240M";        break;
                default: ss << mEssenceParser.GetColorPrimaries();
            }
            tmp = ss.str(); ss.str(std::string());
            modify_descriptors(mMXFDescriptors, "CaptureGamma", 1, tmp.c_str());
        }
    }

    if (mPicCodingExtensionChk)
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

