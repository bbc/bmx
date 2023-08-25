/*
 * MXF types
 *
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

#ifndef MXF_TYPES_H_
#define MXF_TYPES_H_

#include <wchar.h>
#include <inttypes.h>


#ifdef __cplusplus
extern "C"
{
#endif


#define MXF_PREFACE_VER(major, minor)   (((major) << 8) | (minor))



typedef enum
{
    MXF_FULL_FRAME       = 0x00,
    MXF_SEPARATE_FIELDS  = 0x01,
    MXF_SINGLE_FIELD     = 0x02,
    MXF_MIXED_FIELDS     = 0x03,
    MXF_SEGMENTED_FRAME  = 0x04,
} MXFFrameLayoutEnum;

typedef uint8_t MXFFrameLayout;

typedef enum
{
    MXF_SIGNAL_STANDARD_NONE        = 0x00,
    MXF_SIGNAL_STANDARD_ITU601      = 0x01,
    MXF_SIGNAL_STANDARD_ITU1358     = 0x02,
    MXF_SIGNAL_STANDARD_SMPTE347M   = 0x03,
    MXF_SIGNAL_STANDARD_SMPTE274M   = 0x04,
    MXF_SIGNAL_STANDARD_SMPTE296M   = 0x05,
    MXF_SIGNAL_STANDARD_SMPTE349M   = 0x06,
    MXF_SIGNAL_STANDARD_SMPTE428_1  = 0x07, /* added SMPTE 377-1 */
} MXFSignalStandardEnum;

typedef uint8_t MXFSignalStandard;

typedef enum
{
    MXF_COLOR_SITING_COSITING        = 0x00,
    MXF_COLOR_SITING_HORIZ_MIDPOINT  = 0x01,
    MXF_COLOR_SITING_THREE_TAP       = 0x02,
    MXF_COLOR_SITING_QUINCUNX        = 0x03,
    MXF_COLOR_SITING_REC601          = 0x04, /* deprecated in SMPTE 377-1 - use 0x00 instead */
    MXF_COLOR_SITING_LINE_ALTERN     = 0x05, /* added SMPTE 377-1 */
    MXF_COLOR_SITING_VERT_MIDPOINT   = 0x06, /* added SMPTE 377-1 */
    MXF_COLOR_SITING_UNKNOWN         = 0xff, /* added SMPTE 377-1 */
} MXFColorSitingEnum;

typedef uint8_t MXFColorSiting;

typedef enum
{
    MXF_SCANNING_DIRECTION_LR_TB    = 0x00,
    MXF_SCANNING_DIRECTION_RL_TB    = 0x01,
    MXF_SCANNING_DIRECTION_LR_BT    = 0x02,
    MXF_SCANNING_DIRECTION_RL_BT    = 0x03,
    MXF_SCANNING_DIRECTION_TB_LR    = 0x04,
    MXF_SCANNING_DIRECTION_TB_RL    = 0x05,
    MXF_SCANNING_DIRECTION_BT_LR    = 0x06,
    MXF_SCANNING_DIRECTION_BT_RL    = 0x07,
} MXFScanningDirectionEnum;

typedef uint8_t MXFScanningDirection;

typedef enum
{
    MXF_AVC_UNKNOWN_CODED_CONTENT_TYPE          = 0x00,
    MXF_AVC_PROGRESSIVE_FRAME_PICTURE           = 0x01,
    MXF_AVC_INTERLACED_FIELD_PICTURE            = 0x02,
    MXF_AVC_INTERLACED_FRAME_PICTURE            = 0x03,
    MXF_AVC_INTERLACED_FRAME_AND_FIELD_PICTURE  = 0x04,
} MXFAVCCodedContentTypeEnum;

typedef uint8_t MXFAVCCodedContentType;

typedef enum
{
    MXF_VC2_WAVELET_DESLAURIERS_DUBUC_9_7       = 0x00,
    MXF_VC2_WAVELET_LEGALL_5_3                  = 0x01,
    MXF_VC2_WAVELET_DESLAURIERS_DUBUC_13_7      = 0x02,
    MXF_VC2_WAVELET_HAAR_NO_SHIFT               = 0x03,
    MXF_VC2_WAVELET_HAAR_SINGLE_SHIFT_PER_LEVEL = 0x04,
    MXF_VC2_WAVELET_FIDELITY_FILTER             = 0x05,
    MXF_VC2_WAVELET_DAUBECHIES_9_7_INT_APPROX   = 0x06,
} MXFVC2WaveletFilterEnum;

typedef uint8_t MXFVC2WaveletFilterType;

/* software-only enumerations corresponding to data definition SMPTE labels */
typedef enum
{
    MXF_UNKNOWN_DDEF,
    MXF_PICTURE_DDEF,
    MXF_SOUND_DDEF,
    MXF_TIMECODE_DDEF,
    MXF_DATA_DDEF,
    MXF_DM_DDEF,
} MXFDataDefEnum;

typedef enum
{
    MXF_UNKNOWN_WRAPPING_TYPE,
    MXF_FRAME_WRAPPED,
    MXF_CLIP_WRAPPED,
} MXFEssenceWrappingType;

typedef struct
{
    uint8_t octet0;
    uint8_t octet1;
    uint8_t octet2;
    uint8_t octet3;
    uint8_t octet4;
    uint8_t octet5;
    uint8_t octet6;
    uint8_t octet7;
    uint8_t octet8;
    uint8_t octet9;
    uint8_t octet10;
    uint8_t octet11;
    uint8_t octet12;
    uint8_t octet13;
    uint8_t octet14;
    uint8_t octet15;
} mxfUL;

typedef mxfUL mxfKey;

typedef struct
{
    uint8_t octet0;
    uint8_t octet1;
    uint8_t octet2;
    uint8_t octet3;
    uint8_t octet4;
    uint8_t octet5;
    uint8_t octet6;
    uint8_t octet7;
    uint8_t octet8;
    uint8_t octet9;
    uint8_t octet10;
    uint8_t octet11;
    uint8_t octet12;
    uint8_t octet13;
    uint8_t octet14;
    uint8_t octet15;
} mxfUUID;

typedef mxfUL mxfUID;

typedef mxfUID mxfAUID;

typedef uint16_t mxfLocalTag;

typedef uint16_t mxfVersionType;

typedef struct
{
    int16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t qmsec;
} mxfTimestamp;

typedef wchar_t mxfUTF16Char;

typedef struct
{
    uint8_t octet0;
    uint8_t octet1;
    uint8_t octet2;
    uint8_t octet3;
    uint8_t octet4;
    uint8_t octet5;
    uint8_t octet6;
    uint8_t octet7;
    uint8_t octet8;
    uint8_t octet9;
    uint8_t octet10;
    uint8_t octet11;
    uint8_t octet12;
    uint8_t octet13;
    uint8_t octet14;
    uint8_t octet15;
    uint8_t octet16;
    uint8_t octet17;
    uint8_t octet18;
    uint8_t octet19;
    uint8_t octet20;
    uint8_t octet21;
    uint8_t octet22;
    uint8_t octet23;
    uint8_t octet24;
    uint8_t octet25;
    uint8_t octet26;
    uint8_t octet27;
    uint8_t octet28;
    uint8_t octet29;
    uint8_t octet30;
    uint8_t octet31;
} mxfUMID;

typedef struct
{
    unsigned char bytes[64];
} mxfExtendedUMID;

typedef struct
{
    int32_t numerator;
    int32_t denominator;
} mxfRational;

typedef int64_t mxfPosition;

typedef int64_t mxfLength;

typedef uint8_t mxfBoolean;

typedef struct
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t build;
    uint16_t release;
} mxfProductVersion;

typedef struct
{
    struct
    {
        uint8_t code;
        uint8_t depth;
    } components[8];
} mxfRGBALayout;

typedef struct
{
    unsigned char bytes[24];
} mxfAES3FixedData;

typedef struct
{
    uint8_t s_siz;
    uint8_t xr_siz;
    uint8_t yr_siz;
} mxfJ2KComponentSizing;

typedef struct
{
    uint32_t p_cap;
    uint16_t c_capi[32];    /* count is the number of bits set in p_cap */
} mxfJ2KExtendedCapabilities;

typedef struct
{
    uint16_t x;
    uint16_t y;
} mxfColorPrimary;

typedef struct
{
    mxfColorPrimary primaries[3];
} mxfThreeColorPrimaries;

typedef struct
{
    int32_t first;   /* first line number of first field or FULL_FRAME progressive frame */
    int32_t second;  /* first line number of second field */
} mxfVideoLineMap;


/* external MXF data lengths */
#define mxfLocalTag_extlen              2
#define mxfVersionType_extlen           2
#define mxfUUID_extlen                  16
#define mxfKey_extlen                   16
#define mxfUID_extlen                   16
#define mxfUL_extlen                    16
#define mxfAUID_extlen                  16
#define mxfTimestamp_extlen             8
#define mxfUTF16Char_extlen             2
#define mxfUMID_extlen                  32
#define mxfRational_extlen              8
#define mxfPosition_extlen              8
#define mxfLength_extlen                8
#define mxfBoolean_extlen               1
#define mxfProductVersion_extlen        10
#define mxfRGBALayout_extlen            16
#define mxfAES3FixedData_extlen         24
#define mxfJ2KComponentSizing_extlen    3
#define mxfColorPrimary_extlen          4
#define mxfThreeColorPrimaries_extlen   12


static const mxfUUID g_Null_UUID =
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const mxfKey g_Null_Key =
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const mxfUL g_Null_UL =
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const mxfLocalTag g_Null_LocalTag = 0x0000;

static const mxfUMID g_Null_UMID =
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const mxfExtendedUMID g_Null_Extended_UMID = {{0}};

static const mxfRational g_Null_Rational = {0, 0};

static const mxfThreeColorPrimaries g_Null_Three_Color_Primaries = {{{0, 0}}};
static const mxfColorPrimary g_Null_Color_Primary = {0, 0};

static const mxfVideoLineMap g_Null_Video_Line_Map = {0, 0};


#ifdef __cplusplus
}
#endif


#endif 
