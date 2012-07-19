/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_MXF_FRAME_METADATA_H_
#define BMX_MXF_FRAME_METADATA_H_


#include <bmx/frame/Frame.h>



namespace bmx
{


extern const char *SYSTEM_SCHEME_1_FMETA_ID;
extern const char *SDTI_CP_SYSTEM_METADATA_FMETA_ID;
extern const char *SDTI_CP_PACKAGE_METADATA_FMETA_ID;


class SystemScheme1Metadata : public FrameMetadata
{
public:
    typedef enum
    {
        TIMECODE_ARRAY,
        APP_CHECKSUM,
    } Type;

public:
    SystemScheme1Metadata(Type type);
    virtual ~SystemScheme1Metadata();

    Type GetType() const { return mType; }

private:
    Type mType;
};


class SS1TimecodeArray : public SystemScheme1Metadata
{
public:
    SS1TimecodeArray(Rational frame_rate, bool is_bbc_preservation_file);
    virtual ~SS1TimecodeArray();

    bool IsBBCAPPTimecodeArray() const;
    Timecode GetVITC() const;
    Timecode GetLTC() const;

public:
    std::vector<SMPTE12MTimecode> mS12MTimecodes;

private:
    Rational mFrameRate;
    bool mIsBBCPreservationFile;
};


class SS1APPChecksum : public SystemScheme1Metadata
{
public:
    SS1APPChecksum(uint32_t crc32);
    virtual ~SS1APPChecksum();

public:
    uint32_t mCRC32;
};



class SDTICPSystemMetadata : public FrameMetadata
{
public:
    SDTICPSystemMetadata();
    virtual ~SDTICPSystemMetadata();

public:
    Rational mCPRate;
    bool mHaveUserTimecode;
    SMPTE12MTimecode mUserTimecode;
};


class SDTICPPackageMetadata : public FrameMetadata
{
public:
    SDTICPPackageMetadata();
    virtual ~SDTICPPackageMetadata();

public:
    bool mHaveUMID;
    ExtendedUMID mUMID;
    std::string mEssenceMark;
};


};



#endif

