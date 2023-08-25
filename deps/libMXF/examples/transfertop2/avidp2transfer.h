/*
 * Reads an Avid AAF composition file and transfers referenced MXF files to P2
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Stuart Cunningham
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

#ifndef AVIDP2TRANSFER_H_
#define AVIDP2TRANSFER_H_


#include <vector>
#include <string>

#include <avid_mxf_to_p2.h>



#if defined(_MSC_VER) && defined(_WIN32)

typedef unsigned char avtUInt8;
typedef unsigned short int avtUInt16;
typedef unsigned long int avtUInt32;

typedef unsigned __int64 avtUInt64;

typedef signed char avtInt8;
typedef signed short int avtInt16;
typedef signed long int avtInt32;
typedef __int64 avtInt64;

#else

#include <inttypes.h>

typedef uint8_t	avtUInt8;
typedef uint16_t avtUInt16;
typedef uint32_t avtUInt32;
typedef uint64_t avtUInt64;

typedef int8_t avtInt8;
typedef int16_t avtInt16;
typedef int32_t avtInt32;
typedef int64_t avtInt64;

#endif


class APTException
{
public:
    APTException(const char *format, ...);
    ~APTException();

    const char* getMessage();

private:
    std::string _message;
};

typedef struct APTRational
{
    avtInt32 numerator;
    avtInt32 denominator;
} APTRational;


class APTTrackInfo
{
public:
    bool isPicture;
    APTRational compositionEditRate; // the track edit rate in the CompositionMob
    avtInt64 compositionTrackLength; // the track length in the CompositionMob
    APTRational sourceEditRate; // the track edit rate in the SourceMob
    avtInt64 sourceTrackLength; // the track length in the SourceMob
    std::string mxfFilename;
    std::string name;
};


typedef void (*progress_callback) (float percentCompleted);


class AvidP2Transfer
{
public:
    AvidP2Transfer(const char *aafFilename, progress_callback progress,
        insert_timecode_callback insertTimecode,
        const char *filepathPrefix, bool omitDriveColon);
    ~AvidP2Transfer();

    bool transferToP2(const char *p2path);

    std::string clipName;
    avtUInt64 totalStorageSizeEstimate;  /* not an exact size */

    std::vector<APTTrackInfo> trackInfo;

private:
    void processAvidComposition(const char *filename);
    std::string rewriteFilepath(std::string filepath);

    AvidMXFToP2Transfer* _transfer;
    bool _readyToTransfer;

    std::string _filepathPrefix;
    bool _omitDriveColon;

    avtInt64 _timecodeStart;
    bool _timecodeDrop;
    avtUInt16 _timecodeFPS;
};




#endif

