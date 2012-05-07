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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <cstring>

#include <bmx/mxf_reader/MXFFrameMetadata.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



const char *bmx::SYSTEM_SCHEME_1_FMETA_ID           = "SYSTEM_SCHEME_1";
const char *bmx::SDTI_CP_SYSTEM_METADATA_FMETA_ID   = "SDTICP_SYS_META";
const char *bmx::SDTI_CP_PACKAGE_METADATA_FMETA_ID  = "SDTICP_PACK_META";



SystemScheme1Metadata::SystemScheme1Metadata(Type type)
: FrameMetadata(SYSTEM_SCHEME_1_FMETA_ID)
{
    mType = type;
}

SystemScheme1Metadata::~SystemScheme1Metadata()
{
}



SS1TimecodeArray::SS1TimecodeArray(Rational frame_rate, bool is_bbc_preservation_file)
: SystemScheme1Metadata(SystemScheme1Metadata::TIMECODE_ARRAY)
{
    mFrameRate = frame_rate;
    mIsBBCPreservationFile = is_bbc_preservation_file;
}

SS1TimecodeArray::~SS1TimecodeArray()
{
}

bool SS1TimecodeArray::IsBBCAPPTimecodeArray() const
{
    return mIsBBCPreservationFile && mS12MTimecodes.size() == 2;
}

Timecode SS1TimecodeArray::GetVITC() const
{
    BMX_CHECK(IsBBCAPPTimecodeArray());
    return decode_smpte_timecode(mFrameRate, mS12MTimecodes[0].bytes, sizeof(mS12MTimecodes[0].bytes));
}

Timecode SS1TimecodeArray::GetLTC() const
{
    BMX_CHECK(IsBBCAPPTimecodeArray());
    return decode_smpte_timecode(mFrameRate, mS12MTimecodes[1].bytes, sizeof(mS12MTimecodes[1].bytes));
}



SS1APPChecksum::SS1APPChecksum(uint32_t crc32)
: SystemScheme1Metadata(SystemScheme1Metadata::APP_CHECKSUM)
{
    mCRC32 = crc32;
}

SS1APPChecksum::~SS1APPChecksum()
{
}



SDTICPSystemMetadata::SDTICPSystemMetadata()
: FrameMetadata(SDTI_CP_SYSTEM_METADATA_FMETA_ID)
{
    mCPRate = ZERO_RATIONAL;
    mHaveUserTimecode = 0;
    memset(&mUserTimecode, 0, sizeof(mUserTimecode));
}

SDTICPSystemMetadata::~SDTICPSystemMetadata()
{
}



SDTICPPackageMetadata::SDTICPPackageMetadata()
: FrameMetadata(SDTI_CP_PACKAGE_METADATA_FMETA_ID)
{
    mHaveUMID = false;
    memset(mUMID.bytes, 0, sizeof(mUMID.bytes));
}

SDTICPPackageMetadata::~SDTICPPackageMetadata()
{
}

