/*
 * Parse metadata from an Avid MXF file
 *
 * Copyright (C) 2008, British Broadcasting Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_uu_metadata.h>
#include <mxf/mxf_macros.h>

#include "avid_mxf_info.h"


#define DEBUG_PRINT_ERROR(cmd) \
    if (printDebugError) \
    { \
        fprintf(stderr, "'%s' failed in %s, line %d\n", #cmd, __FILENAME__, __LINE__); \
    } \

#define CHECK(cmd, ecode) \
    if (!(cmd)) \
    { \
        DEBUG_PRINT_ERROR(cmd); \
        errorCode = ecode; \
        goto fail; \
    }

#define DCHECK(cmd) \
    if (!(cmd)) \
    { \
        DEBUG_PRINT_ERROR(cmd); \
        errorCode = -1; \
        goto fail; \
    }

#define FCHECK(cmd) \
    if (!(cmd)) \
    { \
        DEBUG_PRINT_ERROR(cmd); \
        goto fail; \
    }


typedef struct
{
    uint32_t first;
    uint32_t last;
} TrackNumberRange;


/* keep this in sync with AvidEssenceType in the header file */
static const char* ESSENCE_TYPE_STRINGS[] =
{
    "not recognized",
    "MPEG 30",
    "MPEG 40",
    "MPEG 50",
    "DV 25 411",
    "DV 25 420",
    "DV 50",
    "DV 100",
    "20:1",
    "2:1s",
    "4:1s",
    "15:1s",
    "10:1",
    "10:1m",
    "4:1m",
    "3:1",
    "2:1",
    "1:1",
    "1:1 10b",
    "35:1p",
    "28:1p",
    "14:1p",
    "3:1p",
    "2:1p",
    "3:1m",
    "8:1m",
    "DNxHD 1235",
    "DNxHD 1237",
    "DNxHD 1238",
    "DNxHD 1241",
    "DNxHD 1242",
    "DNxHD 1243",
    "DNxHD 1250",
    "DNxHD 1251",
    "DNxHD 1252",
    "DNxHD 1253",
    "MPEG-4",
    "XDCAM HD",
    "AVC-Intra 100",
    "AVC-Intra 50",
    "PCM"
};


typedef struct
{
    AvidEssenceType essenceType;
    mxfRational editRate;
    const char *name;
} DNxHDTypeString;

static const DNxHDTypeString DNXHD_TYPE_STRINGS[] =
{
    /* 23.976 fps */
    {DNXHD_1235_ESSENCE_TYPE,   {24000, 1001},  "DNxHD 175x (1235)"},
    {DNXHD_1237_ESSENCE_TYPE,   {24000, 1001},  "DNxHD 115 (1237)"},
    {DNXHD_1238_ESSENCE_TYPE,   {24000, 1001},  "DNxHD 175 (1238)"},
    {DNXHD_1250_ESSENCE_TYPE,   {24000, 1001},  "DNxHD 90x (1250)"},
    {DNXHD_1251_ESSENCE_TYPE,   {24000, 1001},  "DNxHD 90 (1251)"},
    {DNXHD_1252_ESSENCE_TYPE,   {24000, 1001},  "DNxHD 60 (1252)"},
    {DNXHD_1253_ESSENCE_TYPE,   {24000, 1001},  "DNxHD 36 (1253)"},

    /* 24 fps */
    {DNXHD_1235_ESSENCE_TYPE,   {24, 1},    "DNxHD 175x (1235)"},
    {DNXHD_1237_ESSENCE_TYPE,   {24, 1},    "DNxHD 115 (1237)"},
    {DNXHD_1238_ESSENCE_TYPE,   {24, 1},    "DNxHD 175 (1238)"},
    {DNXHD_1253_ESSENCE_TYPE,   {24, 1},    "DNxHD 36 (1253)"},

    /* 25 fps */
    {DNXHD_1235_ESSENCE_TYPE,   {25, 1},    "DNxHD 185x (1235)"},
    {DNXHD_1237_ESSENCE_TYPE,   {25, 1},    "DNxHD 120 (1237)"},
    {DNXHD_1238_ESSENCE_TYPE,   {25, 1},    "DNxHD 185 (1238)"},
    {DNXHD_1241_ESSENCE_TYPE,   {25, 1},    "DNxHD 185x (1241)"},
    {DNXHD_1242_ESSENCE_TYPE,   {25, 1},    "DNxHD 120 (1242)"},
    {DNXHD_1243_ESSENCE_TYPE,   {25, 1},    "DNxHD 185 (1243)"},
    {DNXHD_1250_ESSENCE_TYPE,   {25, 1},    "DNxHD 90x (1250)"},
    {DNXHD_1251_ESSENCE_TYPE,   {25, 1},    "DNxHD 90 (1251)"},
    {DNXHD_1252_ESSENCE_TYPE,   {25, 1},    "DNxHD 60 (1252)"},
    {DNXHD_1253_ESSENCE_TYPE,   {25, 1},    "DNxHD 36 (1253)"},

    /* 29.97 fps */
    {DNXHD_1235_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 220x (1235)"},
    {DNXHD_1237_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 145 (1237)"},
    {DNXHD_1238_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 220 (1238)"},
    {DNXHD_1241_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 220x (1241)"},
    {DNXHD_1242_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 145 (1242)"},
    {DNXHD_1243_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 220 (1243)"},
    {DNXHD_1250_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 110x (1250)"},
    {DNXHD_1251_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 110 (1251)"},
    {DNXHD_1252_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 75 (1252)"},
    {DNXHD_1253_ESSENCE_TYPE,   {30000, 1001},    "DNxHD 45 (1253)"},

    /* 50 fps */
    {DNXHD_1250_ESSENCE_TYPE,   {50, 1},    "DNxHD 175x (1250)"},
    {DNXHD_1251_ESSENCE_TYPE,   {50, 1},    "DNxHD 175 (1251)"},
    {DNXHD_1252_ESSENCE_TYPE,   {50, 1},    "DNxHD 115 (1252)"},

    /* 59.94 fps */
    {DNXHD_1250_ESSENCE_TYPE,   {60000, 1001},    "DNxHD 220x (1250)"},
    {DNXHD_1251_ESSENCE_TYPE,   {60000, 1001},    "DNxHD 220 (1251)"},
    {DNXHD_1252_ESSENCE_TYPE,   {60000, 1001},    "DNxHD 145 (1252)"},
};


typedef struct
{
    AvidEssenceType essenceType;
    int checkPictureCodingLabel;
    const mxfUL *essenceContainerLabel;
    const mxfUL *pictureCodingLabel;
    int32_t avidResolutionID;
} EssenceTypeMap;

static const EssenceTypeMap ESSENCE_TYPE_MAP[] =
{
    {MPEG_30_ESSENCE_TYPE,  0,  &MXF_EC_L(D10_30_625_50_picture_only),   &MXF_CMDEF_L(D10_30_625_50), 0},
    {MPEG_30_ESSENCE_TYPE,  0,  &MXF_EC_L(D10_30_525_60_picture_only),   &MXF_CMDEF_L(D10_30_525_60), 0},
    {MPEG_40_ESSENCE_TYPE,  0,  &MXF_EC_L(D10_40_625_50_picture_only),   &MXF_CMDEF_L(D10_40_625_50), 0},
    {MPEG_40_ESSENCE_TYPE,  0,  &MXF_EC_L(D10_40_525_60_picture_only),   &MXF_CMDEF_L(D10_40_525_60), 0},
    {MPEG_50_ESSENCE_TYPE,  0,  &MXF_EC_L(D10_50_625_50_picture_only),   &MXF_CMDEF_L(D10_50_625_50), 0},
    {MPEG_50_ESSENCE_TYPE,  0,  &MXF_EC_L(D10_50_525_60_picture_only),   &MXF_CMDEF_L(D10_50_525_60), 0},

    {DV_25_411_ESSENCE_TYPE,  0,  &MXF_EC_L(IECDV_25_525_60_ClipWrapped),           &MXF_CMDEF_L(IECDV_25_525_60),         0},
    {DV_25_420_ESSENCE_TYPE,  0,  &MXF_EC_L(IECDV_25_625_50_ClipWrapped),           &MXF_CMDEF_L(IECDV_25_625_50),         0},
    {DV_25_411_ESSENCE_TYPE,  0,  &MXF_EC_L(DVBased_25_525_60_ClipWrapped),         &MXF_CMDEF_L(DVBased_25_525_60),       0},
    {DV_25_411_ESSENCE_TYPE,  0,  &MXF_EC_L(DVBased_25_625_50_ClipWrapped),         &MXF_CMDEF_L(DVBased_25_625_50),       0},
    {DV_50_ESSENCE_TYPE,      0,  &MXF_EC_L(DVBased_50_525_60_ClipWrapped),         &MXF_CMDEF_L(DVBased_50_525_60),       0},
    {DV_50_ESSENCE_TYPE,      0,  &MXF_EC_L(DVBased_50_625_50_ClipWrapped),         &MXF_CMDEF_L(DVBased_50_625_50),       0},
    {DV_100_ESSENCE_TYPE,     0,  &MXF_EC_L(DVBased_100_720_50_P_ClipWrapped),      &MXF_CMDEF_L(DVBased_100_720_50_P),    0},
    {DV_100_ESSENCE_TYPE,     0,  &MXF_EC_L(DVBased_100_720_60_P_ClipWrapped),      &MXF_CMDEF_L(DVBased_100_720_60_P),    0},
    {DV_100_ESSENCE_TYPE,     0,  &MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped),     &MXF_CMDEF_L(DVBased_100_1080_50_I),   0},
    {DV_100_ESSENCE_TYPE,     0,  &MXF_EC_L(DVBased_100_1080_60_I_ClipWrapped),     &MXF_CMDEF_L(DVBased_100_1080_60_I),   0},

    {MJPEG_10_1_ESSENCE_TYPE,     1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG101_PAL),    0x4b},
    {MJPEG_10_1_ESSENCE_TYPE,     1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG101_NTSC),   0x4b},
    {MJPEG_2_1_ESSENCE_TYPE,      1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG21_PAL),     0x4c},
    {MJPEG_2_1_ESSENCE_TYPE,      1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG21_NTSC),    0x4c},
    {MJPEG_3_1_ESSENCE_TYPE,      1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG31_PAL),     0x4d},
    {MJPEG_3_1_ESSENCE_TYPE,      1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG31_NTSC),    0x4d},
    {MJPEG_15_1_S_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG151s_PAL),   0x4e},
    {MJPEG_15_1_S_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG151s_NTSC),  0x4e},
    {MJPEG_4_1_S_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG41s_PAL),    0x4f},
    {MJPEG_4_1_S_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG41s_NTSC),   0x4f},
    {MJPEG_2_1_S_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG21s_PAL),    0x50},
    {MJPEG_2_1_S_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG21s_NTSC),   0x50},
    {MJPEG_20_1_ESSENCE_TYPE,     1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG201_PAL),    0x52},
    {MJPEG_20_1_ESSENCE_TYPE,     1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG201_NTSC),   0x52},
    {MJPEG_3_1_P_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG31p_PAL),    0x61},
    {MJPEG_3_1_P_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG31p_NTSC),   0x61},
    {MJPEG_2_1_P_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG21p_PAL),    0x62},
    {MJPEG_2_1_P_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG21p_NTSC),   0x62},
    {MJPEG_35_1_P_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG351p_PAL),   0x66},
    {MJPEG_35_1_P_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG351p_NTSC),  0x66},
    {MJPEG_14_1_P_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG141p_PAL),   0x67},
    {MJPEG_14_1_P_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG141p_NTSC),  0x67},
    {MJPEG_28_1_P_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG281p_PAL),   0x68},
    {MJPEG_28_1_P_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG281p_NTSC),  0x68},
    {MJPEG_10_1_M_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG101m_PAL),   0x6e},
    {MJPEG_10_1_M_ESSENCE_TYPE,   1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG101m_NTSC),  0x6e},
    {MJPEG_4_1_M_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  0,                                 0x6f},
    {MJPEG_8_1_M_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG81m_PAL),    0x70},
    {MJPEG_8_1_M_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG81m_NTSC),   0x70},
    {MJPEG_3_1_M_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG31m_PAL),    0x71},
    {MJPEG_3_1_M_ESSENCE_TYPE,    1,  &MXF_EC_L(AvidMJPEGClipWrapped),  &MXF_CMDEF_L(AvidMJPEG31m_NTSC),   0x71},

    {UNC_1_1_ESSENCE_TYPE,  0,  &MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped),      0, 0xaa},
    {UNC_1_1_ESSENCE_TYPE,  0,  &MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped),    0, 0xaa},
    {UNC_1_1_ESSENCE_TYPE,  0,  &MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped),         0, 0},
    {UNC_1_1_ESSENCE_TYPE,  0,  &MXF_EC_L(HD_Unc_1080_5994i_422_ClipWrapped),       0, 0},
    {UNC_1_1_ESSENCE_TYPE,  0,  &MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped),          0, 0},
    {UNC_1_1_ESSENCE_TYPE,  0,  &MXF_EC_L(HD_Unc_720_5994p_422_ClipWrapped),        0, 0},
    /* Avid Media Composer 5.03 wrongly used this label for 720_50p */
    {UNC_1_1_ESSENCE_TYPE,  0,  &MXF_EC_L(HD_Unc_720_60p_422_ClipWrapped),          0, 0},

    {UNC_1_1_10B_ESSENCE_TYPE,  0,  &MXF_EC_L(AvidUnc10Bit625ClipWrapped),    &MXF_CMDEF_L(AvidUncSD10Bit),  0x07e6},
    {UNC_1_1_10B_ESSENCE_TYPE,  0,  &MXF_EC_L(AvidUnc10Bit525ClipWrapped),    &MXF_CMDEF_L(AvidUncSD10Bit),  0x07e5},
    {UNC_1_1_10B_ESSENCE_TYPE,  0,  &MXF_EC_L(AvidUnc10Bit1080iClipWrapped),  &MXF_CMDEF_L(AvidUncHD10Bit),  0x07d0},
    {UNC_1_1_10B_ESSENCE_TYPE,  0,  &MXF_EC_L(AvidUnc10Bit720pClipWrapped),   &MXF_CMDEF_L(AvidUncHD10Bit),  0x07d4},

    {DNXHD_1235_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD1080p1235ClipWrapped),  &MXF_CMDEF_L(VC3_1080P_1235), 1235},
    {DNXHD_1237_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD1080p1237ClipWrapped),  &MXF_CMDEF_L(VC3_1080P_1237), 1237},
    {DNXHD_1238_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD1080p1238ClipWrapped),  &MXF_CMDEF_L(VC3_1080P_1238), 1238},
    {DNXHD_1241_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD1080i1241ClipWrapped),  &MXF_CMDEF_L(VC3_1080I_1241), 1241},
    {DNXHD_1242_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD1080i1242ClipWrapped),  &MXF_CMDEF_L(VC3_1080I_1242), 1242},
    {DNXHD_1243_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD1080i1243ClipWrapped),  &MXF_CMDEF_L(VC3_1080I_1243), 1243},
    {DNXHD_1250_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD720p1250ClipWrapped),   &MXF_CMDEF_L(VC3_720P_1250),  1250},
    {DNXHD_1251_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD720p1251ClipWrapped),   &MXF_CMDEF_L(VC3_720P_1251),  1251},
    {DNXHD_1252_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD720p1252ClipWrapped),   &MXF_CMDEF_L(VC3_720P_1252),  1252},
    {DNXHD_1253_ESSENCE_TYPE,  0,  &MXF_EC_L(DNxHD1080p1253ClipWrapped),  &MXF_CMDEF_L(VC3_1080P_1253), 1253},

    {MPEG4_ESSENCE_TYPE,  1,  &MXF_EC_L(AvidMPEGClipWrapped),  &MXF_CMDEF_L(MP4AdvancedRealTimeSimpleL4), 0},

    {XDCAM_HD_ESSENCE_TYPE,  1,  &MXF_EC_L(AvidMPEGClipWrapped),  &MXF_CMDEF_L(MPEG2_422P_HL_LONGGOP),  0},
    {XDCAM_HD_ESSENCE_TYPE,  1,  &MXF_EC_L(AvidMPEGClipWrapped),  &MXF_CMDEF_L(MPEG2_MP_HL_LONGGOP),    0},
    {XDCAM_HD_ESSENCE_TYPE,  1,  &MXF_EC_L(AvidMPEGClipWrapped),  &MXF_CMDEF_L(MPEG2_MP_H14_LONGGOP),   0},

    {AVCINTRA_100_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_100_1080_60_I),  0},
    {AVCINTRA_100_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_100_1080_50_I),  0},
    {AVCINTRA_100_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_100_1080_30_P),  0},
    {AVCINTRA_100_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_100_1080_25_P),  0},
    {AVCINTRA_100_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_100_720_60_P),   0},
    {AVCINTRA_100_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_100_720_50_P),   0},

    {AVCINTRA_50_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_50_1080_60_I),  0},
    {AVCINTRA_50_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_50_1080_50_I),  0},
    {AVCINTRA_50_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_50_1080_30_P),  0},
    {AVCINTRA_50_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_50_1080_25_P),  0},
    {AVCINTRA_50_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_50_720_60_P),   0},
    {AVCINTRA_50_ESSENCE_TYPE,  1,  &MXF_EC_L(AVCClipWrapped),  &MXF_CMDEF_L(AVCI_50_720_50_P),   0},

    {PCM_ESSENCE_TYPE,  0,  &MXF_EC_L(BWFClipWrapped),  0, 0},
    {PCM_ESSENCE_TYPE,  0,  &MXF_EC_L(AES3ClipWrapped), 0, 0},
};


static const char *FRAME_LAYOUT_STRINGS[] =
{
    "unknown layout",
    "full frame",
    "separate fields",
    "single field",
    "mixed field",
    "segmented frame",
};



static AvidEssenceType get_essence_type(mxfUL *essenceContainerLabel, mxfUL *pictureCodingLabel,
                                        int32_t avidResolutionID)
{
    /* Note: using mxf_equals_ul_mod_regver function below because some Avid labels (e.g. D10)
       use a different registry version byte */

    int match_ec, match_pc, match_resid;
    int null_in_ec, null_in_pc;
    int null_resid, null_ec, null_pc;
    size_t i;

    null_in_ec = mxf_equals_ul(essenceContainerLabel, &g_Null_UL);
    null_in_pc = mxf_equals_ul(pictureCodingLabel, &g_Null_UL);

    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++)
    {
        null_ec = null_in_ec || !ESSENCE_TYPE_MAP[i].essenceContainerLabel;
        null_pc = null_in_pc || !ESSENCE_TYPE_MAP[i].pictureCodingLabel;
        null_resid = avidResolutionID == 0 || ESSENCE_TYPE_MAP[i].avidResolutionID == 0;

        match_ec = !null_ec &&
                    mxf_equals_ul_mod_regver(essenceContainerLabel, ESSENCE_TYPE_MAP[i].essenceContainerLabel);
        match_pc = !null_pc &&
                   mxf_equals_ul_mod_regver(pictureCodingLabel, ESSENCE_TYPE_MAP[i].pictureCodingLabel);
        match_resid = !null_resid &&
                      avidResolutionID == ESSENCE_TYPE_MAP[i].avidResolutionID;

        if (match_ec &&
            (!ESSENCE_TYPE_MAP[i].checkPictureCodingLabel || match_pc) &&
            (null_resid || match_resid))
        {
            return ESSENCE_TYPE_MAP[i].essenceType;
        }
        else if (match_ec &&
                 ESSENCE_TYPE_MAP[i].checkPictureCodingLabel &&
                 !ESSENCE_TYPE_MAP[i].pictureCodingLabel &&
                 match_resid)
        {
            return ESSENCE_TYPE_MAP[i].essenceType;
        }
        else if (null_ec && match_pc && (null_resid || match_resid))
        {
            return ESSENCE_TYPE_MAP[i].essenceType;
        }
    }

    return UNKNOWN_ESSENCE_TYPE;
}

static const char* get_dnxhd_type_string(AvidEssenceType essenceType, mxfRational editRate)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(DNXHD_TYPE_STRINGS); i++) {
        if (essenceType == DNXHD_TYPE_STRINGS[i].essenceType &&
            memcmp(&editRate, &DNXHD_TYPE_STRINGS[i].editRate, sizeof(editRate)) == 0)
        {
            return DNXHD_TYPE_STRINGS[i].name;
        }
    }

    return ESSENCE_TYPE_STRINGS[essenceType];
}

static const char* get_essence_type_string(AvidEssenceType essenceType, mxfRational editRate)
{
    assert(essenceType < ARRAY_SIZE(ESSENCE_TYPE_STRINGS));
    assert(PCM_ESSENCE_TYPE + 1 == ARRAY_SIZE(ESSENCE_TYPE_STRINGS));

    if (essenceType == DNXHD_1235_ESSENCE_TYPE ||
        essenceType == DNXHD_1237_ESSENCE_TYPE ||
        essenceType == DNXHD_1238_ESSENCE_TYPE ||
        essenceType == DNXHD_1241_ESSENCE_TYPE ||
        essenceType == DNXHD_1242_ESSENCE_TYPE ||
        essenceType == DNXHD_1243_ESSENCE_TYPE ||
        essenceType == DNXHD_1250_ESSENCE_TYPE ||
        essenceType == DNXHD_1251_ESSENCE_TYPE ||
        essenceType == DNXHD_1252_ESSENCE_TYPE ||
        essenceType == DNXHD_1253_ESSENCE_TYPE)
    {
        return get_dnxhd_type_string(essenceType, editRate);
    }

    return ESSENCE_TYPE_STRINGS[essenceType];
}

static const char* frame_layout_string(uint8_t frameLayout)
{
    if ((size_t)frameLayout + 1 < ARRAY_SIZE(FRAME_LAYOUT_STRINGS))
    {
        return FRAME_LAYOUT_STRINGS[frameLayout + 1];
    }

    return FRAME_LAYOUT_STRINGS[0];
}

static void print_umid(const mxfUMID *umid)
{
    printf("%02x%02x%02x%02x%02x%02x%02x%02x"
           "%02x%02x%02x%02x%02x%02x%02x%02x"
           "%02x%02x%02x%02x%02x%02x%02x%02x"
           "%02x%02x%02x%02x%02x%02x%02x%02x",
           umid->octet0, umid->octet1, umid->octet2, umid->octet3,
           umid->octet4, umid->octet5, umid->octet6, umid->octet7,
           umid->octet8, umid->octet9, umid->octet10, umid->octet11,
           umid->octet12, umid->octet13, umid->octet14, umid->octet15,
           umid->octet16, umid->octet17, umid->octet18, umid->octet19,
           umid->octet20, umid->octet21, umid->octet22, umid->octet23,
           umid->octet24, umid->octet25, umid->octet26, umid->octet27,
           umid->octet28, umid->octet29, umid->octet30, umid->octet31);
}

static void print_timestamp(const mxfTimestamp *timestamp)
{
    printf("%d-%02u-%02u %02u:%02u:%02u.%03u",
           timestamp->year, timestamp->month, timestamp->day,
           timestamp->hour, timestamp->min, timestamp->sec, timestamp->qmsec * 4);
}

static void print_label(const mxfUL *label)
{
    printf("%02x%02x%02x%02x%02x%02x%02x%02x"
           "%02x%02x%02x%02x%02x%02x%02x%02x",
           label->octet0, label->octet1, label->octet2, label->octet3,
           label->octet4, label->octet5, label->octet6, label->octet7,
           label->octet8, label->octet9, label->octet10, label->octet11,
           label->octet12, label->octet13, label->octet14, label->octet15);
}

static void print_timecode(int64_t timecode, const mxfRational *sampleRate)
{
    int hour, min, sec, frame;
    int roundedTimecodeBase = (int)(sampleRate->numerator / (double)sampleRate->denominator + 0.5);

    hour  = (int)(  timecode / (60 * 60 * roundedTimecodeBase));
    min   = (int)(( timecode % (60 * 60 * roundedTimecodeBase)) / (60 * roundedTimecodeBase));
    sec   = (int)(((timecode % (60 * 60 * roundedTimecodeBase)) % (60 * roundedTimecodeBase)) / roundedTimecodeBase);
    frame = (int)(((timecode % (60 * 60 * roundedTimecodeBase)) % (60 * roundedTimecodeBase)) % roundedTimecodeBase);

    printf("%02d:%02d:%02d:%02d", hour, min, sec, frame);
}

static int convert_string(const mxfUTF16Char *utf16Str, char **str, int printDebugError)
{
    size_t utf8Len;

    utf8Len = mxf_utf16_to_utf8(0, utf16Str, 0);
    FCHECK(utf8Len != (size_t)(-1));
    FCHECK((*str = malloc(utf8Len + 1)) != NULL);
    mxf_utf16_to_utf8(*str, utf16Str, utf8Len + 1);

    return 1;

fail:
    return 0;
}

static void free_avid_tagged_values(AvidTaggedValue *value, int numValues)
{
    int i, j;

    if (value == NULL)
    {
        return;
    }

    for (i = 0; i < numValues; i++)
    {
        SAFE_FREE(value[i].name);
        SAFE_FREE(value[i].value);
        for (j = 0; j < value[i].numAttributes; j++)
        {
            SAFE_FREE(value[i].attributes[j].name);
            SAFE_FREE(value[i].attributes[j].value);
        }
        SAFE_FREE(value[i].attributes);
    }

    free(value);
}

static int get_string_value(MXFMetadataSet *set, const mxfKey *itemKey, char **str, int printDebugError)
{
    uint16_t utf16Size;
    mxfUTF16Char *utf16Str = NULL;

    FCHECK(mxf_get_utf16string_item_size(set, itemKey, &utf16Size));
    FCHECK((utf16Str = malloc(utf16Size * sizeof(mxfUTF16Char))) != NULL);
    FCHECK(mxf_get_utf16string_item(set, itemKey, utf16Str));

    FCHECK(convert_string(utf16Str, str, printDebugError));

    SAFE_FREE(utf16Str);
    return 1;

fail:
    SAFE_FREE(utf16Str);
    return 0;
}

static int get_package_tagged_values(MXFMetadataSet *packageSet, const mxfKey *itemKey, int *numValues,
                                     AvidTaggedValue **values, int printDebugError)
{
    MXFMetadataSet *taggedValueSet;
    uint32_t count = 0;
    uint32_t i;
    uint8_t *element;
    mxfUTF16Char *taggedValueName = NULL;
    mxfUTF16Char *taggedValueValue = NULL;
    AvidTaggedValue *newValues = NULL;
    int index;
    MXFListIterator namesIter;
    MXFListIterator valuesIter;
    MXFList *taggedValueNames = NULL;
    MXFList *taggedValueValues = NULL;

    FCHECK(mxf_have_item(packageSet, itemKey));

    CHK_OFAIL(mxf_get_array_item_count(packageSet, itemKey, &count));

    FCHECK((newValues = (AvidTaggedValue*)malloc(count * sizeof(AvidTaggedValue))) != NULL);
    memset(newValues, 0, count * sizeof(AvidTaggedValue));

    for (i = 0; i < count; i++)
    {
        CHK_OFAIL(mxf_get_array_item_element(packageSet, itemKey, i, &element));
        CHK_OFAIL(mxf_get_strongref(packageSet->headerMetadata, element, &taggedValueSet));

        CHK_OFAIL(mxf_avid_read_string_tagged_value(taggedValueSet, &taggedValueName, &taggedValueValue));

        FCHECK(convert_string(taggedValueName, &newValues[i].name, printDebugError));
        FCHECK(convert_string(taggedValueValue, &newValues[i].value, printDebugError));
        SAFE_FREE(taggedValueName);
        SAFE_FREE(taggedValueValue);

        /* Check for any attributes */
        if (mxf_have_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, TaggedValueAttributeList)))
        {
           CHK_OFAIL(mxf_avid_read_string_tagged_values(taggedValueSet, &MXF_ITEM_K(TaggedValue, TaggedValueAttributeList),
                                                        &taggedValueNames, &taggedValueValues));
           newValues[i].numAttributes = (int)mxf_get_list_length(taggedValueNames);

           FCHECK((newValues[i].attributes = (AvidNameValuePair*)malloc(newValues[i].numAttributes * sizeof(AvidNameValuePair))) != NULL);
           memset(newValues[i].attributes, 0, newValues[i].numAttributes * sizeof(AvidNameValuePair));

           index = 0;
           mxf_initialise_list_iter(&namesIter, taggedValueNames);
           mxf_initialise_list_iter(&valuesIter, taggedValueValues);
           while (mxf_next_list_iter_element(&namesIter) && mxf_next_list_iter_element(&valuesIter))
           {
               FCHECK(convert_string(mxf_get_iter_element(&namesIter), &newValues[i].attributes[index].name, printDebugError));
               FCHECK(convert_string(mxf_get_iter_element(&valuesIter), &newValues[i].attributes[index].value, printDebugError));

               index++;
           }

           mxf_free_list(&taggedValueNames);
           mxf_free_list(&taggedValueValues);
        }
    }

    *values = newValues;
    *numValues = count;

    return 1;

fail:
    free_avid_tagged_values(newValues, count);
    SAFE_FREE(taggedValueName);
    SAFE_FREE(taggedValueValue);
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);
    return 0;
}

static int get_single_track_component(MXFMetadataSet *trackSet, const mxfKey *componentSetKey,
                                      MXFMetadataSet **componentSet, int printDebugError)
{
    MXFMetadataSet *sequenceSet = NULL;
    MXFMetadataSet *cSet = NULL;
    uint32_t componentCount;
    uint8_t *arrayElementValue;

    /* get the single component in the sequence which is a subclass of componentSetKey */
    FCHECK(mxf_get_strongref_item(trackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
    if (mxf_set_is_subclass_of(sequenceSet, &MXF_SET_K(Sequence)))
    {
        /* is a sequence, so we get the first component */
        FCHECK(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &componentCount));
        if (componentCount != 1)
        {
            /* empty sequence or > 1 are not what we expect */
            return 0;
        }

        /* get first component */
        FCHECK(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElementValue));
        if (!mxf_get_strongref(trackSet->headerMetadata, arrayElementValue, &cSet))
        {
            /* reference to a dark set and we assume it isn't something we're interested in */
            return 0;
        }
    }
    else
    {
        /* something other than a sequence */
        cSet = sequenceSet;
    }

    if (!mxf_set_is_subclass_of(cSet, componentSetKey))
    {
        /* not a componentSetKey component */
        return 0;
    }

    *componentSet = cSet;
    return 1;

fail:
    return 0;
}

static int64_t convert_length(const mxfRational *targetEditRate, const mxfRational *editRate, int64_t length)
{
    return (int64_t)(length *
                        targetEditRate->numerator * editRate->denominator /
                            (double)(targetEditRate->denominator * editRate->numerator) + 0.5);
}

static int64_t compare_length(const mxfRational *editRateA, int64_t lengthA, const mxfRational *editRateB, int64_t lengthB)
{
    return lengthA - convert_length(editRateA, editRateB, lengthB);
}

static void insert_track_number(TrackNumberRange *trackNumbers, uint32_t trackNumber, int *numTrackNumberRanges)
{
    int i;
    int j;

    for (i = 0; i < *numTrackNumberRanges; i++)
    {
        if (trackNumber < trackNumbers[i].first - 1)
        {
            /* insert new track range */
            for (j = *numTrackNumberRanges - 1; j >= i; j--)
            {
                trackNumbers[j + 1] = trackNumbers[j];
            }
            trackNumbers[i].first = trackNumber;
            trackNumbers[i].last = trackNumber;

            (*numTrackNumberRanges)++;
            return;
        }
        else if (trackNumber == trackNumbers[i].first - 1)
        {
            /* extend range back one */
            trackNumbers[i].first = trackNumber;
            return;
        }
        else if (trackNumber == trackNumbers[i].last + 1)
        {
            if (i + 1 < *numTrackNumberRanges &&
                trackNumber == trackNumbers[i + 1].first - 1)
            {
                /* merge range forwards */
                trackNumbers[i + 1].first = trackNumbers[i].first;
                for (j = i; j < *numTrackNumberRanges - 1; j++)
                {
                    trackNumbers[j] = trackNumbers[j + 1];
                }
                (*numTrackNumberRanges)--;
            }
            else
            {
                /* extend range forward one */
                trackNumbers[i].last = trackNumber;
            }

            return;
        }
        else if (trackNumber == trackNumbers[i].first ||
            trackNumber == trackNumbers[i].last)
        {
            /* duplicate */
            return;
        }
    }


    /* append new track range */
    trackNumbers[i].first = trackNumber;
    trackNumbers[i].last = trackNumber;

    (*numTrackNumberRanges)++;
    return;
}

static size_t get_range_string(char *buffer, size_t maxSize, int isFirst, int isVideo, const TrackNumberRange *range)
{
    size_t strLen = 0;

    if (isFirst)
    {
        mxf_snprintf(buffer, maxSize, "%s", (isVideo) ? "V" : "A");
        strLen = strlen(buffer);
        buffer += strLen;
        maxSize -= strLen;
    }
    else
    {
        mxf_snprintf(buffer, maxSize, ",");
        strLen = strlen(buffer);
        buffer += strLen;
        maxSize -= strLen;
    }


    if (range->first == range->last)
    {
        mxf_snprintf(buffer, maxSize, "%d", range->first);
        strLen += strlen(buffer);
    }
    else
    {
        mxf_snprintf(buffer, maxSize, "%d-%d", range->first, range->last);
        strLen += strlen(buffer);
    }

    return strLen;
}


int ami_read_info(const char *filename, AvidMXFInfo *info, int printDebugError)
{
    int errorCode = -1;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint32_t sequenceComponentCount;
    uint32_t choicesCount;
    uint8_t *arrayElement;
    MXFList *list = NULL;
    MXFListIterator listIter;
    MXFArrayItemIterator arrayIter;
    MXFFile *mxfFile = NULL;
    MXFPartition *headerPartition = NULL;
    MXFDataModel *dataModel = NULL;
    MXFHeaderMetadata *headerMetadata = NULL;
    MXFMetadataSet *set = NULL;
    MXFMetadataSet *prefaceSet = NULL;
    MXFMetadataSet *fileSourcePackageSet = NULL;
    MXFMetadataSet *materialPackageSet = NULL;
    MXFMetadataSet *descriptorSet = NULL;
    MXFMetadataSet *locatorSet = NULL;
    MXFMetadataSet *physicalSourcePackageSet = NULL;
    MXFMetadataSet *materialPackageTrackSet = NULL;
    MXFMetadataSet *trackSet = NULL;
    MXFMetadataSet *sourceClipSet = NULL;
    MXFMetadataSet *sequenceSet = NULL;
    MXFMetadataSet *timecodeComponentSet = NULL;
    MXFMetadataSet *refSourcePackageSet = NULL;
    MXFMetadataSet *essenceGroupSet = NULL;
    mxfUMID packageUID;
    MXFList *taggedValueNames = NULL;
    MXFList *taggedValueValues = NULL;
    const mxfUTF16Char *taggedValue;
    mxfUL dataDef;
    mxfUMID sourcePackageID;
    MXFArrayItemIterator iter3;
    int64_t filePackageStartPosition;
    int64_t startPosition;
    mxfRational filePackageEditRate;
    int64_t startTimecode;
    uint16_t roundedTimecodeBase;
    int haveStartTimecode;
    mxfRational editRate;
    int64_t trackDuration;
    int64_t segmentDuration;
    int64_t segmentOffset;
    mxfRational maxEditRate = {25, 1};
    int64_t maxDuration = 0;
    int32_t avidResolutionID = 0x00;
    TrackNumberRange videoTrackNumberRanges[64];
    int numVideoTrackNumberRanges = 0;
    TrackNumberRange audioTrackNumberRanges[64];
    int numAudioTrackNumberRanges = 0;
    uint32_t trackNumber;
    char tracksString[256] = {0};
    int i, j;
    size_t remSize;
    char *tracksStringPtr;
    size_t strLen;
    uint32_t arrayElementLen;
    uint16_t fpRoundedTimecodeBase;

    memset(info, 0, sizeof(AvidMXFInfo));
    info->frameLayout = 0xff; /* unknown (0 is known) */


    /* open file */

    CHECK(mxf_disk_file_open_read(filename, &mxfFile), -2);


    /* read header partition pack */

    CHECK(mxf_read_header_pp_kl(mxfFile, &key, &llen, &len), -3);
    CHECK(mxf_read_partition(mxfFile, &key, len, &headerPartition), -3);


    /* check is OP-Atom */

    CHECK(mxf_is_op_atom(&headerPartition->operationalPattern), -4);


    /* read the header metadata (filter out meta-dictionary and dictionary except data defs) */

    DCHECK(mxf_load_data_model(&dataModel));
    DCHECK(mxf_avid_load_extensions(dataModel));

    DCHECK(mxf_finalise_data_model(dataModel));

    DCHECK(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    DCHECK(mxf_is_header_metadata(&key));
    DCHECK(mxf_create_header_metadata(&headerMetadata, dataModel));
    DCHECK(mxf_avid_read_filtered_header_metadata(mxfFile, 0, headerMetadata, headerPartition->headerByteCount, &key, llen, len));


    /* get the preface and info */

    DCHECK(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    if (mxf_have_item(prefaceSet, &MXF_ITEM_K(Preface, ProjectName)))
    {
        DCHECK(get_string_value(prefaceSet, &MXF_ITEM_K(Preface, ProjectName), &info->projectName, printDebugError));
    }
    if (mxf_have_item(prefaceSet, &MXF_ITEM_K(Preface, ProjectEditRate)))
    {
        DCHECK(mxf_get_rational_item(prefaceSet, &MXF_ITEM_K(Preface, ProjectEditRate), &info->projectEditRate));
    }


    /* get the material package and info */

    DCHECK(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet));
    DCHECK(mxf_get_umid_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &info->materialPackageUID));
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, Name)))
    {
        DCHECK(get_string_value(materialPackageSet, &MXF_ITEM_K(GenericPackage, Name), &info->clipName, printDebugError));
    }
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate)))
    {
        DCHECK(mxf_get_timestamp_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, PackageCreationDate), &info->clipCreated));
    }


    /* get the material package project name tagged value if not already set */

    if (info->projectName == NULL &&
        mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList)))
    {
        DCHECK(mxf_avid_read_string_mob_attributes(materialPackageSet, &taggedValueNames, &taggedValueValues));
        if (mxf_avid_get_mob_attribute(L"_PJ", taggedValueNames, taggedValueValues, &taggedValue))
        {
            DCHECK(convert_string(taggedValue, &info->projectName, printDebugError));
        }
    }
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);


    /* get the material package user comments */
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, UserComments)))
    {
        DCHECK(get_package_tagged_values(materialPackageSet, &MXF_ITEM_K(GenericPackage, UserComments), &info->numUserComments, &info->userComments, printDebugError));
    }

    /* get the material package attributes */
    if (mxf_have_item(materialPackageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList)))
    {
        DCHECK(get_package_tagged_values(materialPackageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList), &info->numMaterialPackageAttributes, &info->materialPackageAttributes, printDebugError));
    }

    /* get the top level file source package and info */

    DCHECK(mxf_uu_get_top_file_package(headerMetadata, &fileSourcePackageSet));
    DCHECK(mxf_get_umid_item(fileSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &info->fileSourcePackageUID));


    /* get the file source package essence descriptor info */

    DCHECK(mxf_get_strongref_item(fileSourcePackageSet, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet));
    if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(GenericPictureEssenceDescriptor)))
    {
        /* image aspect ratio */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio)))
        {
            DCHECK(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), &info->aspectRatio));
        }

        /* frame layout */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout)))
        {
            DCHECK(mxf_get_uint8_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), &info->frameLayout));
        }

        /* stored width and height */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), &info->storedWidth));
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), &info->storedHeight));
        }

        /* display width and height */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), &info->displayWidth));
        }
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), &info->displayHeight));
        }

        /* Avid resolution Id */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID)))
        {
            DCHECK(mxf_get_int32_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), &avidResolutionID));
        }

        /* picture essence coding label */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding)))
        {
            DCHECK(mxf_get_ul_item(descriptorSet, &MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), &info->pictureCodingLabel));
        }
    }
    else if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(GenericSoundEssenceDescriptor)))
    {
        /* audio sampling rate */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate)))
        {
            DCHECK(mxf_get_rational_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), &info->audioSamplingRate));
        }
        /* quantization bits */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), &info->quantizationBits));
        }
        /* channel count */
        if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount)))
        {
            DCHECK(mxf_get_uint32_item(descriptorSet, &MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), &info->channelCount));
        }
    }


    /* get the material track referencing the file source package and info */
    /* get the clip duration ( = duration of the longest track) */

    DCHECK(mxf_uu_get_package_tracks(materialPackageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &materialPackageTrackSet))
    {
        DCHECK(mxf_uu_get_track_datadef(materialPackageTrackSet, &dataDef));

        /* some Avid files have a weak reference to a DataDefinition instead of a UL */
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef) && !mxf_is_timecode(&dataDef))
        {
            if (!mxf_avid_get_data_def(headerMetadata, (mxfUUID*)&dataDef, &dataDef))
            {
                continue;
            }
        }

        /* skip non-video and audio tracks */
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef))
        {
            continue;
        }

        /* track counts */
        if (mxf_is_picture(&dataDef))
        {
            info->numVideoTracks++;
        }
        else
        {
            info->numAudioTracks++;
        }

        /* track number */
        if (mxf_have_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber)))
        {
            DCHECK(mxf_get_uint32_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, TrackNumber), &trackNumber));
            if (mxf_is_picture(&dataDef))
            {
                insert_track_number(videoTrackNumberRanges, trackNumber, &numVideoTrackNumberRanges);
            }
            else
            {
                insert_track_number(audioTrackNumberRanges, trackNumber, &numAudioTrackNumberRanges);
            }
        }
        else
        {
            trackNumber = 0;
        }

        /* edit rate */
        DCHECK(mxf_get_rational_item(materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &editRate));

        /* track duration */
        DCHECK(mxf_uu_get_track_duration(materialPackageTrackSet, &trackDuration));

        /* get max duration for the clip duration */
        if (compare_length(&maxEditRate, maxDuration, &editRate, trackDuration) <= 0)
        {
            maxEditRate = editRate;
            maxDuration = trackDuration;
        }

        /* assume the project edit rate equals the video edit rate if not set */
        if (memcmp(&info->projectEditRate, &g_Null_Rational, sizeof(g_Null_Rational)) == 0 &&
            mxf_is_picture(&dataDef))
        {
            info->projectEditRate = editRate;
        }

        /* get info from this track if it references the file source package through a source clip */
        packageUID = g_Null_UMID;
        segmentOffset = 0;
        CHK_ORET(mxf_get_strongref_item(materialPackageTrackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
        if (!mxf_is_subclass_of(sequenceSet->headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(SourceClip)))
        {
            /* iterate through sequence components */
            CHK_ORET(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &sequenceComponentCount));
            for (i = 0; i < (int)sequenceComponentCount; i++)
            {
                CHK_ORET(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), i, &arrayElement));
                if (!mxf_get_strongref(sequenceSet->headerMetadata, arrayElement, &sourceClipSet))
                {
                    /* dark set not registered in the dictionary */
                    continue;
                }

                CHK_ORET(mxf_get_length_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &segmentDuration));

                if (mxf_is_subclass_of(sourceClipSet->headerMetadata->dataModel, &sourceClipSet->key, &MXF_SET_K(EssenceGroup)))
                {
                    /* is an essence group - iterate through choices */
                    essenceGroupSet = sourceClipSet;
                    CHK_ORET(mxf_get_array_item_count(essenceGroupSet, &MXF_ITEM_K(EssenceGroup, Choices), &choicesCount));
                    for (j = 0; j < (int)choicesCount; j++)
                    {
                        CHK_ORET(mxf_get_array_item_element(essenceGroupSet, &MXF_ITEM_K(EssenceGroup, Choices), j, &arrayElement));
                        if (!mxf_get_strongref(essenceGroupSet->headerMetadata, arrayElement, &sourceClipSet))
                        {
                            /* dark set not registered in the dictionary */
                            continue;
                        }

                        if (mxf_is_subclass_of(sourceClipSet->headerMetadata->dataModel, &sourceClipSet->key, &MXF_SET_K(SourceClip)))
                        {
                            CHK_ORET(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &packageUID));
                            if (mxf_equals_umid(&info->fileSourcePackageUID, &packageUID))
                            {
                                /* found source clip referencing file source package */
                                break;
                            }
                        }
                    }
                    if (j < (int)choicesCount)
                    {
                        /* found source clip referencing file source package */
                        break;
                    }
                }
                else if (mxf_is_subclass_of(sourceClipSet->headerMetadata->dataModel, &sourceClipSet->key, &MXF_SET_K(SourceClip)))
                {
                    CHK_ORET(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &packageUID));
                    if (mxf_equals_umid(&info->fileSourcePackageUID, &packageUID))
                    {
                        /* found source clip referencing file source package */
                        break;
                    }
                }

                segmentOffset += segmentDuration;
            }
        }
        else
        {
            /* track sequence is a source clip */
            sourceClipSet = sequenceSet;
            CHK_ORET(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &packageUID));
            CHK_ORET(mxf_get_length_item(sourceClipSet, &MXF_ITEM_K(StructuralComponent, Duration), &segmentDuration));
            segmentOffset = 0;
        }

        if (mxf_equals_umid(&info->fileSourcePackageUID, &packageUID))
        {
            info->isVideo = mxf_is_picture(&dataDef);
            info->editRate = editRate;
            info->trackDuration = trackDuration;
            info->trackNumber = trackNumber;
            info->segmentDuration = segmentDuration;
            info->segmentOffset = segmentOffset;
        }
    }


    info->clipDuration = convert_length(&info->projectEditRate, &maxEditRate, maxDuration);



    /* construct the tracks string */

    remSize = sizeof(tracksString);
    tracksStringPtr = tracksString;
    for (i = 0; i < numVideoTrackNumberRanges; i++)
    {
        if (remSize < 4)
        {
            break;
        }

        strLen = get_range_string(tracksStringPtr, remSize, i == 0, 1, &videoTrackNumberRanges[i]);

        tracksStringPtr += strLen;
        remSize -= strLen;
    }
    if (numVideoTrackNumberRanges > 0 && numAudioTrackNumberRanges > 0)
    {
        mxf_snprintf(tracksStringPtr, remSize, " ");
        strLen = strlen(tracksStringPtr);
        tracksStringPtr += strLen;
        remSize -= strLen;
    }
    for (i = 0; i < numAudioTrackNumberRanges; i++)
    {
        if (remSize < 4)
        {
            break;
        }
        strLen = get_range_string(tracksStringPtr, remSize, i == 0, 0, &audioTrackNumberRanges[i]);

        tracksStringPtr += strLen;
        remSize -= strLen;
    }

    DCHECK((info->tracksString = strdup(tracksString)) != NULL);



    /* get the physical source package and info */

    DCHECK(mxf_find_set_by_key(headerMetadata, &MXF_SET_K(SourcePackage), &list));
    mxf_initialise_list_iter(&listIter, list);
    while (mxf_next_list_iter_element(&listIter))
    {
        set = (MXFMetadataSet*)mxf_get_iter_element(&listIter);

        /* the physical source package is the source package that references a physical descriptor */
        if (mxf_have_item(set, &MXF_ITEM_K(SourcePackage, Descriptor)))
        {
            if (mxf_get_strongref_item(set, &MXF_ITEM_K(SourcePackage, Descriptor), &descriptorSet))
            {
                /* Get first physical package network locator */
                if (mxf_have_item(descriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators)))
                {
                    DCHECK(mxf_initialise_array_item_iterator(descriptorSet, &MXF_ITEM_K(GenericDescriptor, Locators), &arrayIter));
                    while (mxf_next_array_item_element(&arrayIter, &arrayElement, &arrayElementLen))
                    {
                        DCHECK(mxf_get_strongref(headerMetadata, arrayElement, &locatorSet));
                        if (mxf_is_subclass_of(headerMetadata->dataModel, &locatorSet->key, &MXF_SET_K(NetworkLocator)))
                        {
                            DCHECK(get_string_value(locatorSet, &MXF_ITEM_K(NetworkLocator, URLString), &info->physicalPackageLocator, printDebugError));
                            break;
                        }
                    }
                }

                /* NOTE/TODO: some descriptors could be dark and so we don't assume we can dereference */
                if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(PhysicalDescriptor)))
                {
                    if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(TapeDescriptor)))
                    {
                        info->physicalPackageType = TAPE_PHYS_TYPE;
                    }
                    else if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(ImportDescriptor)))
                    {
                        info->physicalPackageType = IMPORT_PHYS_TYPE;
                    }
                    else if (mxf_is_subclass_of(dataModel, &descriptorSet->key, &MXF_SET_K(RecordingDescriptor)))
                    {
                        info->physicalPackageType = RECORDING_PHYS_TYPE;
                    }
                    else
                    {
                        info->physicalPackageType = UNKNOWN_PHYS_TYPE;
                    }
                    physicalSourcePackageSet = set;

                    DCHECK(mxf_get_umid_item(physicalSourcePackageSet, &MXF_ITEM_K(GenericPackage, PackageUID), &info->physicalSourcePackageUID));
                    if (mxf_have_item(physicalSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name)))
                    {
                        DCHECK(get_string_value(physicalSourcePackageSet, &MXF_ITEM_K(GenericPackage, Name), &info->physicalPackageName, printDebugError));
                    }

                    break;
                }
            }
        }
    }
    mxf_free_list(&list);


    /* get the start timecode */

    /* the source timecode is calculated using the SourceClip::StartPosition in the file source package
    in conjunction with the TimecodeComponent in the referenced physical source package */

    haveStartTimecode = 0;
    DCHECK(mxf_uu_get_package_tracks(fileSourcePackageSet, &arrayIter));
    while (!haveStartTimecode && mxf_uu_next_track(headerMetadata, &arrayIter, &trackSet))
    {
        /* skip tracks that are not picture or sound */
        DCHECK(mxf_uu_get_track_datadef(trackSet, &dataDef));

        /* some Avid files have a weak reference to a DataDefinition instead of a UL */
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef) && !mxf_is_timecode(&dataDef))
        {
            if (!mxf_avid_get_data_def(headerMetadata, (mxfUUID*)&dataDef, &dataDef))
            {
                continue;
            }
        }


        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef))
        {
            continue;
        }


        /* get the source clip */
        if (!get_single_track_component(trackSet, &MXF_SET_K(SourceClip), &sourceClipSet, printDebugError))
        {
            continue;
        }

        /* get the start position and edit rate for the file source package source clip */
        DCHECK(mxf_get_rational_item(trackSet, &MXF_ITEM_K(Track, EditRate), &filePackageEditRate));
        DCHECK(mxf_get_position_item(sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), &filePackageStartPosition));


        /* get the package referenced by the source clip */
        DCHECK(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &sourcePackageID));
        if (mxf_equals_umid(&g_Null_UMID, &sourcePackageID) ||
            !mxf_uu_get_referenced_package(sourceClipSet->headerMetadata, &sourcePackageID, &refSourcePackageSet))
        {
            /* either at the end of chain or don't have the referenced package */
            continue;
        }

        /* find the timecode component in the physical source package and calculate the start timecode */
        DCHECK(mxf_uu_get_package_tracks(refSourcePackageSet, &iter3));
        while (mxf_uu_next_track(headerMetadata, &iter3, &trackSet))
        {
            /* skip non-timecode tracks */
            DCHECK(mxf_uu_get_track_datadef(trackSet, &dataDef));

            /* some Avid files have a weak reference to a DataDefinition instead of a UL */
            if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef) && !mxf_is_timecode(&dataDef))
            {
                if (!mxf_avid_get_data_def(headerMetadata, (mxfUUID*)&dataDef, &dataDef))
                {
                    continue;
                }
            }

            if (!mxf_is_timecode(&dataDef))
            {
                continue;
            }

            /* get the timecode component */
            if (!get_single_track_component(trackSet, &MXF_SET_K(TimecodeComponent), &timecodeComponentSet, printDebugError))
            {
                continue;
            }

            /* get the start timecode and rounded timecode base for the timecode component */
            DCHECK(mxf_get_position_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), &startTimecode));
            DCHECK(mxf_get_uint16_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), &roundedTimecodeBase));

            /* convert the physical package start timecode to a start position in the file source package */
            startPosition = filePackageStartPosition;
            if (startTimecode > 0) {
                fpRoundedTimecodeBase = (uint16_t)(filePackageEditRate.numerator / (double)filePackageEditRate.denominator + 0.5);
                if (fpRoundedTimecodeBase == roundedTimecodeBase)
                {
                    startPosition += startTimecode;
                }
                else if (fpRoundedTimecodeBase == 2 * roundedTimecodeBase)
                {
                    startPosition += 2 * startTimecode;
                }
                else
                {
                    mxf_log_warn("Ignoring non-zero TimecodeComponent::StartTimecode because edit rate combination not supported\n");
                    /* TODO: complete support for different timecode and track edit rates */
                }
            }

            /* convert the start position to material package track edit rate units */
            info->startTimecode = (int64_t)(startPosition *
                info->editRate.numerator * filePackageEditRate.denominator /
                    (double)(info->editRate.denominator * filePackageEditRate.numerator) + 0.5);

            haveStartTimecode = 1;
            break;
        }
    }


    /* get the essence type */

    /* using the header partition's essence container label because the label in the FileDescriptor
     is sometimes a weak reference to a ContainerDefinition in Avid files and the ContainerDefinition
     is not of much use */
    if (mxf_get_list_length(&headerPartition->essenceContainers) > 0)
        info->essenceContainerLabel = *(mxfUL*)mxf_get_list_element(&headerPartition->essenceContainers, 0);

    info->essenceType = get_essence_type(&info->essenceContainerLabel, &info->pictureCodingLabel, avidResolutionID);



    /* clean up */

    mxf_file_close(&mxfFile);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    mxf_free_partition(&headerPartition);
    mxf_free_list(&list);
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);

    return 0;


fail:

    mxf_file_close(&mxfFile);
    mxf_free_header_metadata(&headerMetadata);
    mxf_free_data_model(&dataModel);
    mxf_free_partition(&headerPartition);
    mxf_free_list(&list);
    mxf_free_list(&taggedValueNames);
    mxf_free_list(&taggedValueValues);

    ami_free_info(info);

    return errorCode;
}

void ami_free_info(AvidMXFInfo *info)
{
    SAFE_FREE(info->clipName);
    SAFE_FREE(info->projectName);
    SAFE_FREE(info->physicalPackageName);
    SAFE_FREE(info->tracksString);
    SAFE_FREE(info->physicalPackageLocator);

    if (info->userComments != NULL)
    {
        free_avid_tagged_values(info->userComments, info->numUserComments);
    }

    if (info->materialPackageAttributes != NULL)
    {
        free_avid_tagged_values(info->materialPackageAttributes, info->numMaterialPackageAttributes);
    }
}

void ami_print_info(AvidMXFInfo *info)
{
    int i, j;

    printf("Project name = %s\n", (info->projectName == NULL) ? "": info->projectName);
    printf("Project edit rate = %d/%d\n", info->projectEditRate.numerator, info->projectEditRate.denominator);
    printf("Clip name = %s\n", (info->clipName == NULL) ? "": info->clipName);
    printf("Clip created = ");
    print_timestamp(&info->clipCreated);
    printf("\n");
    printf("Clip edit rate = %d/%d\n", info->projectEditRate.numerator, info->projectEditRate.denominator);
    printf("Clip duration = %" PRId64 " samples (", info->clipDuration);
    print_timecode(info->clipDuration, &info->projectEditRate);
    printf(")\n");
    printf("Clip video tracks = %d\n", info->numVideoTracks);
    printf("Clip audio tracks = %d\n", info->numAudioTracks);
    printf("Clip track string = %s\n", (info->tracksString == NULL) ? "" : info->tracksString);
    printf("%s essence\n", info->isVideo ? "Video": "Audio");
    printf("Essence type = %s\n", get_essence_type_string(info->essenceType, info->projectEditRate));
    printf("Essence container label = ");
    print_label(&info->essenceContainerLabel);
    printf("\n");
    printf("Track number = %d\n", info->trackNumber);
    printf("Edit rate = %d/%d\n", info->editRate.numerator, info->editRate.denominator);
    printf("Track duration = %" PRId64 " samples (", info->trackDuration);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->trackDuration), &info->projectEditRate);
    printf(")\n");
    printf("Track segment duration = %" PRId64 " samples (", info->segmentDuration);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->segmentDuration), &info->projectEditRate);
    printf(")\n");
    printf("Track segment offset = %" PRId64 " samples (", info->segmentOffset);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->segmentOffset), &info->projectEditRate);
    printf(")\n");
    printf("Start timecode = %" PRId64 " samples (", info->startTimecode);
    print_timecode(convert_length(&info->projectEditRate, &info->editRate, info->startTimecode), &info->projectEditRate);
    printf(")\n");
    if (info->isVideo)
    {
        printf("Picture coding label = ");
        print_label(&info->pictureCodingLabel);
        printf("\n");
        printf("Image aspect ratio = %d/%d\n", info->aspectRatio.numerator, info->aspectRatio.denominator);
        printf("Stored WxH = %dx%d (%s)\n", info->storedWidth, info->storedHeight, frame_layout_string(info->frameLayout));
        printf("Display WxH = %dx%d (%s)\n", info->storedWidth, info->storedHeight, frame_layout_string(info->frameLayout));
    }
    else
    {
        printf("Audio sampling rate = %d/%d\n", info->audioSamplingRate.numerator, info->audioSamplingRate.denominator);
        printf("Channel count = %d\n", info->channelCount);
        printf("Quantization bits = %d\n", info->quantizationBits);
    }
    if (info->userComments != NULL)
    {
        printf("User comments:\n");
        for (i = 0; i < info->numUserComments; i++)
        {
            printf("  %s = %s\n", info->userComments[i].name, info->userComments[i].value);
            for (j = 0; j < info->userComments[i].numAttributes; j++)
            {
                printf("    %s = %s\n", info->userComments[i].attributes[j].name, info->userComments[i].attributes[j].value);
            }
        }
    }
    if (info->materialPackageAttributes != NULL)
    {
        printf("Material package attributes:\n");
        for (i = 0; i < info->numMaterialPackageAttributes; i++)
        {
            printf("  %s = %s\n", info->materialPackageAttributes[i].name, info->materialPackageAttributes[i].value);
            for (j = 0; j < info->materialPackageAttributes[i].numAttributes; j++)
            {
                printf("    %s = %s\n", info->materialPackageAttributes[i].attributes[j].name, info->materialPackageAttributes[i].attributes[j].value);
            }
        }
    }
    printf("Material package UID = ");
    print_umid(&info->materialPackageUID);
    printf("\n");
    printf("File package UID     = ");
    print_umid(&info->fileSourcePackageUID);
    printf("\n");
    printf("Physical package UID = ");
    print_umid(&info->physicalSourcePackageUID);
    printf("\n");
    printf("Physical package type = ");
    switch (info->physicalPackageType)
    {
        case TAPE_PHYS_TYPE:
            printf("Tape");
            break;
        case IMPORT_PHYS_TYPE:
            printf("Import");
            break;
        case RECORDING_PHYS_TYPE:
            printf("Recording");
            break;
        case UNKNOWN_PHYS_TYPE:
        default:
            break;
    }
    printf("\n");
    printf("Physical package name = %s\n", (info->physicalPackageName == NULL) ? "": info->physicalPackageName);
    printf("Physical package locator = %s\n", (info->physicalPackageLocator == NULL) ? "": info->physicalPackageLocator);
}

