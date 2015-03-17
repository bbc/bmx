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

#include <bmx/EssenceType.h>
#include <bmx/Utils.h>
#include <bmx/Logging.h>
#include <bmx/BMXException.h>

using namespace bmx;


typedef struct
{
    EssenceType essence_type;
    EssenceType generic_essence_type;
    const char *str;
    const char *enum_str;
} EssenceTypeInfo;

static const EssenceTypeInfo ESSENCE_TYPE_INFO[] =
{
    {UNKNOWN_ESSENCE_TYPE,      UNKNOWN_ESSENCE_TYPE,   "unknown essence type",                 "Unknown"},
    {PICTURE_ESSENCE,           PICTURE_ESSENCE,        "picture essence",                      "Picture"},
    {SOUND_ESSENCE,             SOUND_ESSENCE,          "sound essence",                        "Sound"},
    {DATA_ESSENCE,              DATA_ESSENCE,           "data essence",                         "Data"},
    {D10_30,                    PICTURE_ESSENCE,        "D10 30",                               "D10_30"},
    {D10_40,                    PICTURE_ESSENCE,        "D10 40",                               "D10_40"},
    {D10_50,                    PICTURE_ESSENCE,        "D10 50",                               "D10_50"},
    {IEC_DV25,                  PICTURE_ESSENCE,        "IEC DV25",                             "IEC_DV_25"},
    {DVBASED_DV25,              PICTURE_ESSENCE,        "DV-Based DV25",                        "DV_Based_DV_25"},
    {DV50,                      PICTURE_ESSENCE,        "DV50",                                 "DV_50"},
    {DV100_1080I,               PICTURE_ESSENCE,        "DV100 1080i",                          "DV_100_1080i"},
    {DV100_720P,                PICTURE_ESSENCE,        "DV100 720p",                           "DV_100_720p"},
    {AVCI200_1080I,             PICTURE_ESSENCE,        "AVCI 200 1080i",                       "AVCI_200_1080i"},
    {AVCI200_1080P,             PICTURE_ESSENCE,        "AVCI 200 1080p",                       "AVCI_200_1080p"},
    {AVCI200_720P,              PICTURE_ESSENCE,        "AVCI 200 720p",                        "AVCI_200_720p"},
    {AVCI100_1080I,             PICTURE_ESSENCE,        "AVCI 100 1080i",                       "AVCI_100_1080i"},
    {AVCI100_1080P,             PICTURE_ESSENCE,        "AVCI 100 1080p",                       "AVCI_100_1080p"},
    {AVCI100_720P,              PICTURE_ESSENCE,        "AVCI 100 720p",                        "AVCI_100_720p"},
    {AVCI50_1080I,              PICTURE_ESSENCE,        "AVCI 50 1080i",                        "AVCI_50_1080i"},
    {AVCI50_1080P,              PICTURE_ESSENCE,        "AVCI 50 1080p",                        "AVCI_50_1080p"},
    {AVCI50_720P,               PICTURE_ESSENCE,        "AVCI 50 720p",                         "AVCI_50_720p"},
    {AVC_HIGH_10_INTRA_UNCS,    PICTURE_ESSENCE,        "AVC High 10 Intra Unconstrained",      "AVC_High_10_Intra_Uncs"},
    {AVC_HIGH_422_INTRA_UNCS,   PICTURE_ESSENCE,        "AVC High 4:2:2 Intra Unconstrained",   "AVC_High_422_Intra_Uncs"},
    {UNC_SD,                    PICTURE_ESSENCE,        "uncompressed SD",                      "Unc_SD"},
    {UNC_HD_1080I,              PICTURE_ESSENCE,        "uncompressed HD 1080i",                "Unc_HD_1080i"},
    {UNC_HD_1080P,              PICTURE_ESSENCE,        "uncompressed HD 1080p",                "Unc_HD_1080p"},
    {UNC_HD_720P,               PICTURE_ESSENCE,        "uncompressed HD 720p",                 "Unc_HD_720p"},
    {UNC_UHD_3840,              PICTURE_ESSENCE,        "uncompressed UHD 3840x2160",           "Unc_UHD_3840"},
    {AVID_10BIT_UNC_SD,         PICTURE_ESSENCE,        "Avid 10-bit uncompressed SD",          "Avid_10b_Unc_SD"},
    {AVID_10BIT_UNC_HD_1080I,   PICTURE_ESSENCE,        "Avid 10-bit uncompressed HD 1080i",    "Avid_10b_Unc_HD_1080i"},
    {AVID_10BIT_UNC_HD_1080P,   PICTURE_ESSENCE,        "Avid 10-bit uncompressed HD 1080p",    "Avid_10b_Unc_HD_1080p"},
    {AVID_10BIT_UNC_HD_720P,    PICTURE_ESSENCE,        "Avid 10-bit uncompressed HD 720p",     "Avid_10b_Unc_HD_720p"},
    {AVID_ALPHA_SD,             PICTURE_ESSENCE,        "Avid uncompressed Alpha SD",           "Avid_Unc_Alpha_SD"},
    {AVID_ALPHA_HD_1080I,       PICTURE_ESSENCE,        "Avid uncompressed Alpha HD 1080i",     "Avid_Unc_Alpha_HD_1080i"},
    {AVID_ALPHA_HD_1080P,       PICTURE_ESSENCE,        "Avid uncompressed Alpha HD 1080p",     "Avid_Unc_Alpha_HD_1080p"},
    {AVID_ALPHA_HD_720P,        PICTURE_ESSENCE,        "Avid uncompressed Alpha HD 720p",      "Avid_Unc_Alpha_HD_720p"},
    {MPEG2LG_422P_HL_1080I,     PICTURE_ESSENCE,        "MPEG-2 Long GOP 422P@HL 1080i",        "MPEG_2_Long_GOP_422P_HL_1080i"},
    {MPEG2LG_422P_HL_1080P,     PICTURE_ESSENCE,        "MPEG-2 Long GOP 422P@HL 1080p",        "MPEG_2_Long_GOP_422P_HL_1080p"},
    {MPEG2LG_422P_HL_720P,      PICTURE_ESSENCE,        "MPEG-2 Long GOP 422P@HL 720p",         "MPEG_2_Long_GOP_422P_HL_720p"},
    {MPEG2LG_MP_HL_1920_1080I,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1920x1080i",     "MPEG_2_Long_GOP_MP_HL_1920_1080i"},
    {MPEG2LG_MP_HL_1920_1080P,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1920x1080p",     "MPEG_2_Long_GOP_MP_HL_1920_1080p"},
    {MPEG2LG_MP_HL_1440_1080I,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1440x1080i",     "MPEG_2_Long_GOP_MP_HL_1440_1080i"},
    {MPEG2LG_MP_HL_1440_1080P,  PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 1440x1080p",     "MPEG_2_Long_GOP_MP_HL_1440_1080p"},
    {MPEG2LG_MP_HL_720P,        PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@HL 720p",           "MPEG_2_Long_GOP_MP_HL_720p"},
    {MPEG2LG_MP_H14_1080I,      PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@H14 1080i",         "MPEG_2_Long_GOP_MP_H14_1080i"},
    {MPEG2LG_MP_H14_1080P,      PICTURE_ESSENCE,        "MPEG-2 Long GOP MP@H14 1080p",         "MPEG_2_Long_GOP_MP_H14_1080p"},
    {VC3_1080P_1235,            PICTURE_ESSENCE,        "VC3 1080p 1235",                       "VC3_1080p_1235"},
    {VC3_1080P_1237,            PICTURE_ESSENCE,        "VC3 1080p 1237",                       "VC3_1080p_1237"},
    {VC3_1080P_1238,            PICTURE_ESSENCE,        "VC3 1080p 1238",                       "VC3_1080p_1238"},
    {VC3_1080I_1241,            PICTURE_ESSENCE,        "VC3 1080i 1241",                       "VC3_1080i_1241"},
    {VC3_1080I_1242,            PICTURE_ESSENCE,        "VC3 1080i 1242",                       "VC3_1080i_1242"},
    {VC3_1080I_1243,            PICTURE_ESSENCE,        "VC3 1080i 1243",                       "VC3_1080i_1243"},
    {VC3_1080I_1244,            PICTURE_ESSENCE,        "VC3 1080i 1244",                       "VC3_1080i_1244"},
    {VC3_720P_1250,             PICTURE_ESSENCE,        "VC3 720p 1250",                        "VC3_720p_1250"},
    {VC3_720P_1251,             PICTURE_ESSENCE,        "VC3 720p 1251",                        "VC3_720p_1251"},
    {VC3_720P_1252,             PICTURE_ESSENCE,        "VC3 720p 1252",                        "VC3_720p_1252"},
    {VC3_1080P_1253,            PICTURE_ESSENCE,        "VC3 1080p 1253",                       "VC3_1080p_1253"},
    {VC3_720P_1258,             PICTURE_ESSENCE,        "VC3 720p 1258",                        "VC3_720p_1258"},
    {VC3_1080P_1259,            PICTURE_ESSENCE,        "VC3 1080p 1259",                       "VC3_1080p_1259"},
    {VC3_1080I_1260,            PICTURE_ESSENCE,        "VC3 1080i 1260",                       "VC3_1080i_1260"},
    {MJPEG_2_1,                 PICTURE_ESSENCE,        "MJPEG 2:1",                            "MJPEG_2_1"},
    {MJPEG_3_1,                 PICTURE_ESSENCE,        "MJPEG 3:1",                            "MJPEG_3_1"},
    {MJPEG_10_1,                PICTURE_ESSENCE,        "MJPEG 10:1",                           "MJPEG_10_1"},
    {MJPEG_20_1,                PICTURE_ESSENCE,        "MJPEG 20:1",                           "MJPEG_20_1"},
    {MJPEG_4_1M,                PICTURE_ESSENCE,        "MJPEG 4:1m",                           "MJPEG_4_1_M"},
    {MJPEG_10_1M,               PICTURE_ESSENCE,        "MJPEG 10:1m",                          "MJPEG_10_1_M"},
    {MJPEG_15_1S,               PICTURE_ESSENCE,        "MJPEG 15:1s",                          "MJPEG_15_1_S"},
    {WAVE_PCM,                  SOUND_ESSENCE,          "WAVE PCM",                             "WAVE_PCM"},
    {D10_AES3_PCM,              SOUND_ESSENCE,          "D10 AES3 PCM",                         "D10_AES3_PCM"},
    {ANC_DATA,                  DATA_ESSENCE,           "ANC data",                             "ANC_Data"},
    {VBI_DATA,                  DATA_ESSENCE,           "VBI data",                             "VBI_Data"},
};



const char* bmx::essence_type_to_string(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < BMX_ARRAY_SIZE(ESSENCE_TYPE_INFO));
    BMX_ASSERT(ESSENCE_TYPE_INFO[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_INFO[essence_type].str;
}

const char* bmx::essence_type_to_enum_string(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < BMX_ARRAY_SIZE(ESSENCE_TYPE_INFO));
    BMX_ASSERT(ESSENCE_TYPE_INFO[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_INFO[essence_type].enum_str;
}

EssenceType bmx::get_generic_essence_type(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < BMX_ARRAY_SIZE(ESSENCE_TYPE_INFO));
    BMX_ASSERT(ESSENCE_TYPE_INFO[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_INFO[essence_type].generic_essence_type;
}

