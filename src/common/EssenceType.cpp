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
    const char *str;
} EssenceTypeStringMap;

static const EssenceTypeStringMap ESSENCE_TYPE_STRING_MAP[] =
{
    {UNKNOWN_ESSENCE_TYPE,     "unknown essence type"},
    {PICTURE_ESSENCE,          "picture essence"},
    {SOUND_ESSENCE,            "sound essence"},
    {D10_30,                   "D10 30Mbps"},
    {D10_40,                   "D10 40Mbps"},
    {D10_50,                   "D10 50Mbps"},
    {IEC_DV25,                 "IEC DV25"},
    {DVBASED_DV25,             "DV-Based DV25"},
    {DV50,                     "DV50"},
    {DV100_1080I,              "DV100 1080i"},
    {DV100_720P,               "DV100 720p"},
    {AVCI100_1080I,            "AVCI 100Mbps 1080i"},
    {AVCI100_1080P,            "AVCI 100Mbps 1080p"},
    {AVCI100_720P,             "AVCI 100Mbps 720p"},
    {AVCI50_1080I,             "AVCI 50Mbps 1080i"},
    {AVCI50_1080P,             "AVCI 50Mbps 1080p"},
    {AVCI50_720P,              "AVCI 50Mbps 720p"},
    {UNC_SD,                   "uncompressed SD"},
    {UNC_HD_1080I,             "uncompressed HD 1080i"},
    {UNC_HD_1080P,             "uncompressed HD 1080p"},
    {UNC_HD_720P,              "uncompressed HD 720p"},
    {AVID_10BIT_UNC_SD,        "Avid 10-bit uncompressed SD"},
    {AVID_10BIT_UNC_HD_1080I,  "Avid 10-bit uncompressed HD 1080i"},
    {AVID_10BIT_UNC_HD_1080P,  "Avid 10-bit uncompressed HD 1080p"},
    {AVID_10BIT_UNC_HD_720P,   "Avid 10-bit uncompressed HD 720p"},
    {AVID_ALPHA_SD,            "Avid uncompressed Alpha SD"},
    {AVID_ALPHA_HD_1080I,      "Avid uncompressed Alpha HD 1080i"},
    {AVID_ALPHA_HD_1080P,      "Avid uncompressed Alpha HD 1080p"},
    {AVID_ALPHA_HD_720P,       "Avid uncompressed Alpha HD 720p"},
    {MPEG2LG_422P_HL_1080I,    "MPEG-2 Long GOP 422P@HL 1080i"},
    {MPEG2LG_422P_HL_1080P,    "MPEG-2 Long GOP 422P@HL 1080p"},
    {MPEG2LG_422P_HL_720P,     "MPEG-2 Long GOP 422P@HL 720p"},
    {MPEG2LG_MP_HL_1080I,      "MPEG-2 Long GOP MP@HL 1080i"},
    {MPEG2LG_MP_HL_1080P,      "MPEG-2 Long GOP MP@HL 1080p"},
    {MPEG2LG_MP_HL_720P,       "MPEG-2 Long GOP MP@HL 720p"},
    {MPEG2LG_MP_H14_1080I,     "MPEG-2 Long GOP MP@H14 1080i"},
    {MPEG2LG_MP_H14_1080P,     "MPEG-2 Long GOP MP@H14 1080p"},
    {VC3_1080P_1235,           "VC3 1080p 1235"},
    {VC3_1080P_1237,           "VC3 1080p 1237"},
    {VC3_1080P_1238,           "VC3 1080p 1238"},
    {VC3_1080I_1241,           "VC3 1080i 1241"},
    {VC3_1080I_1242,           "VC3 1080i 1242"},
    {VC3_1080I_1243,           "VC3 1080i 1243"},
    {VC3_720P_1250,            "VC3 720p 1250"},
    {VC3_720P_1251,            "VC3 720p 1251"},
    {VC3_720P_1252,            "VC3 720p 1252"},
    {VC3_1080P_1253,           "VC3 1080p 1253"},
    {MJPEG_2_1,                "MJPEG 2:1"},
    {MJPEG_3_1,                "MJPEG 3:1"},
    {MJPEG_10_1,               "MJPEG 10:1"},
    {MJPEG_20_1,               "MJPEG 20:1"},
    {MJPEG_4_1M,               "MJPEG 4:1m"},
    {MJPEG_10_1M,              "MJPEG 10:1m"},
    {MJPEG_15_1S,              "MJPEG 15:1s"},
    {WAVE_PCM,                 "WAVE PCM"},
    {D10_AES3_PCM,             "D10 AES3 PCM"},
};



const char* bmx::essence_type_to_string(EssenceType essence_type)
{
    BMX_ASSERT((size_t)essence_type < ARRAY_SIZE(ESSENCE_TYPE_STRING_MAP));
    BMX_ASSERT(ESSENCE_TYPE_STRING_MAP[essence_type].essence_type == essence_type);

    return ESSENCE_TYPE_STRING_MAP[essence_type].str;
}

