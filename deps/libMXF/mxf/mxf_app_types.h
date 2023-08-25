/*
 * Copyright (C) 2006, British Broadcasting Corporation
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

#ifndef MXF_APP_TYPES_H_
#define MXF_APP_TYPES_H_


#include <mxf/mxf_types.h>


#ifdef __cplusplus
extern "C"
{
#endif


#define INVALID_TIMECODE_HOUR           0xff


#define FORMAT_SIZE                     7
#define PROGTITLE_SIZE                  73
#define EPTITLE_SIZE                    145
#define MAGPREFIX_SIZE                  2
#define PROGNO_SIZE                     9
#define PRODCODE_SIZE                   3
#define SPOOLSTATUS_SIZE                2
#define SPOOLDESC_SIZE                  30
#define MEMO_SIZE                       121
#define SPOOLNO_SIZE                    15
#define ACCNO_SIZE                      15
#define CATDETAIL_SIZE                  11

/* External size = "the string sizes above" * 2 (utf16) +
                   2 * "timestamp size" +
                   "duration size" +
                   "item no size" +
                   16 * ("local tag" + "local length")
                 = 433 * 2 +
                   2 * 8 +
                   8 +
                   4 +
                   16 * (2 + 2)
                 = 958
*/
#define COMPLETE_INFAX_EXTERNAL_SIZE    958

/* use for TimecodeBreak::timecodeType */
#define TIMECODE_BREAK_VITC             0x0001
#define TIMECODE_BREAK_LTC              0x0002



typedef struct
{
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
    int dropFrame;
} ArchiveTimecode;

typedef struct
{
    int64_t position;
    ArchiveTimecode vitcTimecode;
    ArchiveTimecode ltcTimecode;

    int16_t redFlash;
    int16_t spatialPattern;
    int16_t luminanceFlash;
    mxfBoolean extendedFailure;
} PSEFailure;

typedef struct
{
    ArchiveTimecode vitcTimecode;
    ArchiveTimecode ltcTimecode;
    uint8_t errorCode;
} VTRError;

typedef struct
{
    int64_t position;
    uint8_t errorCode;
} VTRErrorAtPos;

typedef struct
{
    int64_t position;
    int32_t strength;
} DigiBetaDropout;

typedef struct
{
    int64_t position;
    uint16_t timecodeType;
} TimecodeBreak;

typedef struct
{
    char format[FORMAT_SIZE];
    char progTitle[PROGTITLE_SIZE];
    char epTitle[EPTITLE_SIZE];
    mxfTimestamp txDate;                /* only date part is relevant */
    char magPrefix[MAGPREFIX_SIZE];
    char progNo[PROGNO_SIZE];
    char prodCode[PRODCODE_SIZE];
    char spoolStatus[SPOOLSTATUS_SIZE];
    mxfTimestamp stockDate;             /* only date part is relevant */
    char spoolDesc[SPOOLDESC_SIZE];
    char memo[MEMO_SIZE];
    int64_t duration;                   /* number of seconds */
    char spoolNo[SPOOLNO_SIZE];         /* max 4 character prefix followed by integer (max 10 digits) */
                                        /* used as the tape SourcePackage name and part of the MaterialPackage name */
    char accNo[ACCNO_SIZE];             /* max 4 character prefix followed by integer (max 10 digits) */
    char catDetail[CATDETAIL_SIZE];
    uint32_t itemNo;
} InfaxData;



#ifdef __cplusplus
}
#endif


#endif

