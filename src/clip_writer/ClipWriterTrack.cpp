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

#include <im/clip_writer/ClipWriterTrack.h>
#include <im/as02/AS02PictureTrack.h>
#include <im/as02/AS02DVTrack.h>
#include <im/as02/AS02UncTrack.h>
#include <im/as02/AS02D10Track.h>
#include <im/as02/AS02MPEG2LGTrack.h>
#include <im/as02/AS02AVCITrack.h>
#include <im/as02/AS02PCMTrack.h>
#include <im/as11/AS11Track.h>
#include <im/mxf_op1a/OP1APictureTrack.h>
#include <im/mxf_op1a/OP1ADVTrack.h>
#include <im/mxf_op1a/OP1AUncTrack.h>
#include <im/mxf_op1a/OP1AD10Track.h>
#include <im/mxf_op1a/OP1AMPEG2LGTrack.h>
#include <im/mxf_op1a/OP1AAVCITrack.h>
#include <im/mxf_op1a/OP1APCMTrack.h>
#include <im/avid_mxf/AvidPictureTrack.h>
#include <im/avid_mxf/AvidDVTrack.h>
#include <im/avid_mxf/AvidD10Track.h>
#include <im/avid_mxf/AvidMPEG2LGTrack.h>
#include <im/avid_mxf/AvidMJPEGTrack.h>
#include <im/avid_mxf/AvidVC3Track.h>
#include <im/avid_mxf/AvidAVCITrack.h>
#include <im/avid_mxf/AvidUncTrack.h>
#include <im/avid_mxf/AvidPCMTrack.h>
#include <im/d10_mxf/D10MPEGTrack.h>
#include <im/d10_mxf/D10PCMTrack.h>
#include <im/MXFUtils.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;


typedef struct
{
    ClipWriterEssenceType type;
    AS02EssenceType as02_type;
    AS11EssenceType as11_type;
    OP1AEssenceType op1a_type;
    AvidEssenceType avid_type;
    D10EssenceType d10_type;
    MXFDescriptorHelper::EssenceType dh_type;
} EssenceTypeMap;

static const EssenceTypeMap ESSENCE_TYPE_MAP[] =
{
    {CW_UNKNOWN_ESSENCE,          AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_UNKNOWN_ESSENCE,     D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::UNKNOWN_ESSENCE},
    {CW_IEC_DV25,                 AS02_IEC_DV25,          AS11_IEC_DV25,          OP1A_IEC_DV25,             AVID_IEC_DV25,            D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::IEC_DV25},
    {CW_DVBASED_DV25,             AS02_DVBASED_DV25,      AS11_DVBASED_DV25,      OP1A_DVBASED_DV25,         AVID_DVBASED_DV25,        D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::DVBASED_DV25},
    {CW_DV50,                     AS02_DV50,              AS11_DV50,              OP1A_DV50,                 AVID_DV50,                D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::DV50},
    {CW_DV100_1080I,              AS02_DV100_1080I,       AS11_DV100_1080I,       OP1A_DV100_1080I,          AVID_DV100_1080I,         D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::DV100_1080I},
    {CW_DV100_1080P,              AS02_DV100_1080I,       AS11_DV100_1080I,       OP1A_DV100_1080I,          AVID_DV100_1080I,         D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::DV100_1080I},
    {CW_DV100_720P,               AS02_DV100_720P,        AS11_DV100_720P,        OP1A_DV100_720P,           AVID_DV100_720P,          D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::DV100_720P},
    {CW_D10_30,                   AS02_D10_30,            AS11_D10_30,            OP1A_D10_30,               AVID_D10_30,              D10_MPEG_30,           MXFDescriptorHelper::D10_30},
    {CW_D10_40,                   AS02_D10_40,            AS11_D10_40,            OP1A_D10_40,               AVID_D10_40,              D10_MPEG_40,           MXFDescriptorHelper::D10_40},
    {CW_D10_50,                   AS02_D10_50,            AS11_D10_50,            OP1A_D10_50,               AVID_D10_50,              D10_MPEG_50,           MXFDescriptorHelper::D10_50},
    {CW_AVCI100_1080I,            AS02_AVCI100_1080I,     AS11_AVCI100_1080I,     OP1A_AVCI100_1080I,        AVID_AVCI100_1080I,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVCI100_1080I},
    {CW_AVCI100_1080P,            AS02_AVCI100_1080P,     AS11_AVCI100_1080P,     OP1A_AVCI100_1080P,        AVID_AVCI100_1080P,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVCI100_1080P},
    {CW_AVCI100_720P,             AS02_AVCI100_720P,      AS11_AVCI100_720P,      OP1A_AVCI100_720P,         AVID_AVCI100_720P,        D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVCI100_720P},
    {CW_AVCI50_1080I,             AS02_AVCI50_1080I,      AS11_AVCI50_1080I,      OP1A_AVCI50_1080I,         AVID_AVCI50_1080I,        D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVCI50_1080I},
    {CW_AVCI50_1080P,             AS02_AVCI50_1080P,      AS11_AVCI50_1080P,      OP1A_AVCI50_1080P,         AVID_AVCI50_1080P,        D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVCI50_1080P},
    {CW_AVCI50_720P,              AS02_AVCI50_720P,       AS11_AVCI50_720P,       OP1A_AVCI50_720P,          AVID_AVCI50_720P,         D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVCI50_720P},
    {CW_UNC_SD,                   AS02_UNC_SD,            AS11_UNC_SD,            OP1A_UNC_SD,               AVID_UNC_SD,              D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::UNC_SD},
    {CW_UNC_HD_1080I,             AS02_UNC_HD_1080I,      AS11_UNC_HD_1080I,      OP1A_UNC_HD_1080I,         AVID_UNC_HD_1080I,        D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::UNC_HD_1080I},
    {CW_UNC_HD_1080P,             AS02_UNC_HD_1080P,      AS11_UNC_HD_1080P,      OP1A_UNC_HD_1080P,         AVID_UNC_HD_1080P,        D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::UNC_HD_1080P},
    {CW_UNC_HD_720P,              AS02_UNC_HD_720P,       AS11_UNC_HD_720P,       OP1A_UNC_HD_720P,          AVID_UNC_HD_720P,         D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::UNC_HD_720P},
    {CW_AVID_10BIT_UNC_SD,        AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_10BIT_UNC_SD,        D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVID_10BIT_UNC_SD},
    {CW_AVID_10BIT_UNC_HD_1080I,  AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_10BIT_UNC_HD_1080I,  D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080I},
    {CW_AVID_10BIT_UNC_HD_1080P,  AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_10BIT_UNC_HD_1080P,  D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080P},
    {CW_AVID_10BIT_UNC_HD_720P,   AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_10BIT_UNC_HD_720P,   D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::AVID_10BIT_UNC_HD_720P},
    {CW_MPEG2LG_422P_HL_1080I,    AS02_MPEG2LG_422P_HL,   AS11_MPEG2LG_422P_HL,   OP1A_MPEG2LG_422P_HL,      AVID_MPEG2LG_422P_HL,     D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_422P_HL},
    {CW_MPEG2LG_422P_HL_1080P,    AS02_MPEG2LG_422P_HL,   AS11_MPEG2LG_422P_HL,   OP1A_MPEG2LG_422P_HL,      AVID_MPEG2LG_422P_HL,     D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_422P_HL},
    {CW_MPEG2LG_422P_HL_720P,     AS02_MPEG2LG_422P_HL,   AS11_MPEG2LG_422P_HL,   OP1A_MPEG2LG_422P_HL,      AVID_MPEG2LG_422P_HL,     D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_422P_HL},
    {CW_MPEG2LG_MP_HL_1080I,      AS02_MPEG2LG_MP_HL,     AS11_MPEG2LG_MP_HL,     OP1A_MPEG2LG_MP_HL,        AVID_MPEG2LG_MP_HL,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_MP_HL},
    {CW_MPEG2LG_MP_HL_1080P,      AS02_MPEG2LG_MP_HL,     AS11_MPEG2LG_MP_HL,     OP1A_MPEG2LG_MP_HL,        AVID_MPEG2LG_MP_HL,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_MP_HL},
    {CW_MPEG2LG_MP_HL_720P,       AS02_MPEG2LG_MP_HL,     AS11_MPEG2LG_MP_HL,     OP1A_MPEG2LG_MP_HL,        AVID_MPEG2LG_MP_HL,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_MP_HL},
    {CW_MPEG2LG_MP_H14_1080I,     AS02_MPEG2LG_MP_H14,    AS11_MPEG2LG_MP_H14,    OP1A_MPEG2LG_MP_H14,       AVID_MPEG2LG_MP_H14,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_MP_H14},
    {CW_MPEG2LG_MP_H14_1080P,     AS02_MPEG2LG_MP_H14,    AS11_MPEG2LG_MP_H14,    OP1A_MPEG2LG_MP_H14,       AVID_MPEG2LG_MP_H14,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MPEG2LG_MP_H14},
    {CW_MJPEG_2_1,                AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_MJPEG_2_1,           D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MJPEG_2_1},
    {CW_MJPEG_3_1,                AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_MJPEG_3_1,           D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MJPEG_3_1},
    {CW_MJPEG_10_1,               AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_MJPEG_10_1,          D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MJPEG_10_1},
    {CW_MJPEG_20_1,               AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_MJPEG_20_1,          D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MJPEG_20_1},
    {CW_MJPEG_4_1M,               AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_MJPEG_4_1M,          D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MJPEG_4_1M},
    {CW_MJPEG_10_1M,              AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_MJPEG_10_1M,         D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MJPEG_10_1M},
    {CW_MJPEG_15_1S,              AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_UNKNOWN_ESSENCE,      AVID_MJPEG_15_1S,         D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::MJPEG_15_1S},
    {CW_VC3_1080P_1235,           AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_1080P_1235,       AVID_VC3_1080P_1235,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_1080P_1235},
    {CW_VC3_1080P_1237,           AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_1080P_1237,       AVID_VC3_1080P_1237,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_1080P_1237},
    {CW_VC3_1080P_1238,           AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_1080P_1238,       AVID_VC3_1080P_1238,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_1080P_1238},
    {CW_VC3_1080I_1241,           AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_1080I_1241,       AVID_VC3_1080I_1241,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_1080I_1241},
    {CW_VC3_1080I_1242,           AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_1080I_1242,       AVID_VC3_1080I_1242,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_1080I_1242},
    {CW_VC3_1080I_1243,           AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_1080I_1243,       AVID_VC3_1080I_1243,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_1080I_1243},
    {CW_VC3_720P_1250,            AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_720P_1250,        AVID_VC3_720P_1250,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_720P_1250},
    {CW_VC3_720P_1251,            AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_720P_1251,        AVID_VC3_720P_1251,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_720P_1251},
    {CW_VC3_720P_1252,            AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_720P_1252,        AVID_VC3_720P_1252,       D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_720P_1252},
    {CW_VC3_1080P_1253,           AS02_UNKNOWN_ESSENCE,   AS11_UNKNOWN_ESSENCE,   OP1A_VC3_1080P_1253,       AVID_VC3_1080P_1253,      D10_UNKNOWN_ESSENCE,   MXFDescriptorHelper::VC3_1080P_1253},
    {CW_PCM,                      AS02_PCM,               AS11_PCM,               OP1A_PCM,                  AVID_PCM,                 D10_PCM,               MXFDescriptorHelper::WAVE_PCM},
};


typedef struct
{
    ClipWriterEssenceType type;
    const char *str;
} EssenceTypeStringMap;

static const EssenceTypeStringMap ESSENCE_TYPE_STRINGS[] =
{
    {CW_UNKNOWN_ESSENCE,            "unknown"},
    {CW_IEC_DV25,                   "IEC DV25"},
    {CW_DVBASED_DV25,               "DVBased DV25"},
    {CW_DV50,                       "DV50"},
    {CW_DV100_1080I,                "DV100 1080i"},
    {CW_DV100_1080P,                "DV100 1080p"},
    {CW_DV100_720P,                 "DV100 720p"},
    {CW_D10_30,                     "D10 30Mbps"},
    {CW_D10_40,                     "D10 40Mbps"},
    {CW_D10_50,                     "D10 50Mbps"},
    {CW_AVCI100_1080I,              "AVCI 100Mbps 1080i"},
    {CW_AVCI100_1080P,              "AVCI 100Mbps 1080p"},
    {CW_AVCI100_720P,               "AVCI 100Mbps 720p"},
    {CW_AVCI50_1080I,               "AVCI 50Mbps 1080i"},
    {CW_AVCI50_1080P,               "AVCI 50Mbps 1080p"},
    {CW_AVCI50_720P,                "AVCI 50Mbps 720p"},
    {CW_UNC_SD,                     "uncompressed SD"},
    {CW_UNC_HD_1080I,               "uncompressed HD 1080i"},
    {CW_UNC_HD_1080P,               "uncompressed HD 1080p"},
    {CW_UNC_HD_720P,                "uncompressed HD 720p"},
    {CW_AVID_10BIT_UNC_SD,          "Avid Packed 10-bit uncompressed SD"},
    {CW_AVID_10BIT_UNC_HD_1080I,    "Avid Packed 10-bit uncompressed HD 1080i"},
    {CW_AVID_10BIT_UNC_HD_1080P,    "Avid Packed 10-bit uncompressed HD 1080p"},
    {CW_AVID_10BIT_UNC_HD_720P,     "Avid Packed 10-bit uncompressed HD 720p"},
    {CW_MPEG2LG_422P_HL_1080I,      "MPEG-2 Long GOP 422P@HL 1080i"},
    {CW_MPEG2LG_422P_HL_1080P,      "MPEG-2 Long GOP 422P@HL 1080p"},
    {CW_MPEG2LG_422P_HL_720P,       "MPEG-2 Long GOP 422P@HL 720p"},
    {CW_MPEG2LG_MP_HL_1080I,        "MPEG-2 Long GOP MP@HL 1080i"},
    {CW_MPEG2LG_MP_HL_1080P,        "MPEG-2 Long GOP MP@HL 1080p"},
    {CW_MPEG2LG_MP_HL_720P,         "MPEG-2 Long GOP MP@HL 720p"},
    {CW_MPEG2LG_MP_H14_1080I,       "MPEG-2 Long GOP MP@H14 1080i"},
    {CW_MPEG2LG_MP_H14_1080P,       "MPEG-2 Long GOP MP@H14 1080p"},
    {CW_MJPEG_2_1,                  "MJPEG 2:1"},
    {CW_MJPEG_3_1,                  "MJPEG 3:1"},
    {CW_MJPEG_10_1,                 "MJPEG 10:1"},
    {CW_MJPEG_20_1,                 "MJPEG 20:1"},
    {CW_MJPEG_4_1M,                 "MJPEG 4:1m"},
    {CW_MJPEG_10_1M,                "MJPEG 10:1m"},
    {CW_MJPEG_15_1S,                "MJPEG 15:1s"},
    {CW_VC3_1080P_1235,             "VC3/DNxHD 1080p 1235"},
    {CW_VC3_1080P_1237,             "VC3/DNxHD 1080p 1237"},
    {CW_VC3_1080P_1238,             "VC3/DNxHD 1080p 1238"},
    {CW_VC3_1080I_1241,             "VC3/DNxHD 1080i 1241"},
    {CW_VC3_1080I_1242,             "VC3/DNxHD 1080i 1242"},
    {CW_VC3_1080I_1243,             "VC3/DNxHD 1080i 1243"},
    {CW_VC3_720P_1250,              "VC3/DNxHD 720p 1250"},
    {CW_VC3_720P_1251,              "VC3/DNxHD 720p 1251"},
    {CW_VC3_720P_1252,              "VC3/DNxHD 720p 1252"},
    {CW_VC3_1080P_1253,             "VC3/DNxHD 1080p 1253"},
    {CW_PCM,                        "WAVE PCM"},
};



#define REFINE_MPEG2LG_TYPE(klass, track, ss_method, fl_method)     \
    switch (mEssenceType)                                           \
    {                                                               \
        case CW_MPEG2LG_422P_HL_1080I:                              \
        case CW_MPEG2LG_MP_HL_1080I:                                \
        case CW_MPEG2LG_MP_H14_1080I:                               \
        {                                                           \
            klass *mpeg_track = dynamic_cast<klass*>(track);        \
            IM_ASSERT(mpeg_track);                                  \
            mpeg_track->ss_method(MXF_SIGNAL_STANDARD_SMPTE274M);   \
            mpeg_track->fl_method(MXF_SEPARATE_FIELDS);             \
            break;                                                  \
        }                                                           \
        case CW_MPEG2LG_422P_HL_1080P:                              \
        case CW_MPEG2LG_MP_HL_1080P:                                \
        case CW_MPEG2LG_MP_H14_1080P:                               \
        {                                                           \
            klass *mpeg_track = dynamic_cast<klass*>(track);        \
            IM_ASSERT(mpeg_track);                                  \
            mpeg_track->ss_method(MXF_SIGNAL_STANDARD_SMPTE274M);   \
            mpeg_track->fl_method(MXF_FULL_FRAME);                  \
        break;                                                      \
        }                                                           \
        case CW_MPEG2LG_422P_HL_720P:                               \
        case CW_MPEG2LG_MP_HL_720P:                                 \
        {                                                           \
            klass *mpeg_track = dynamic_cast<klass*>(track);        \
            IM_ASSERT(mpeg_track);                                  \
            mpeg_track->ss_method(MXF_SIGNAL_STANDARD_SMPTE296M);   \
            mpeg_track->fl_method(MXF_FULL_FRAME);                  \
            break;                                                  \
        }                                                           \
        default:                                                    \
            break;                                                  \
    }



size_t get_essence_type_index(ClipWriterEssenceType type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (type == ESSENCE_TYPE_MAP[i].type)
            return i;
    }

    return 0;
}



bool ClipWriterTrack::IsSupported(ClipWriterType clip_type, ClipWriterEssenceType essence_type,
                                  bool is_mpeg2lg_720p, Rational sample_rate)
{
    size_t index = get_essence_type_index(essence_type);

    switch (clip_type)
    {
        case CW_AS02_CLIP_TYPE:
            return AS02Track::IsSupported(ESSENCE_TYPE_MAP[index].as02_type, is_mpeg2lg_720p, sample_rate);
        case CW_AS11_OP1A_CLIP_TYPE:
            return AS11Track::IsSupported(AS11_OP1A_CLIP_TYPE, ESSENCE_TYPE_MAP[index].as11_type,
                                          is_mpeg2lg_720p, sample_rate);
        case CW_AS11_D10_CLIP_TYPE:
            return AS11Track::IsSupported(AS11_D10_CLIP_TYPE, ESSENCE_TYPE_MAP[index].as11_type, false, sample_rate);
        case CW_OP1A_CLIP_TYPE:
            return OP1ATrack::IsSupported(ESSENCE_TYPE_MAP[index].op1a_type, is_mpeg2lg_720p, sample_rate);
        case CW_AVID_CLIP_TYPE:
            return AvidTrack::IsSupported(ESSENCE_TYPE_MAP[index].avid_type, is_mpeg2lg_720p, sample_rate);
        case CW_D10_CLIP_TYPE:
            return D10Track::IsSupported(ESSENCE_TYPE_MAP[index].d10_type, sample_rate);
        case CW_UNKNOWN_CLIP_TYPE:
            break;
    }

    return false;
}

int ClipWriterTrack::ConvertEssenceType(ClipWriterType clip_type, ClipWriterEssenceType essence_type)
{
    size_t index = get_essence_type_index(essence_type);

    switch (clip_type)
    {
        case CW_AS02_CLIP_TYPE:
            return ESSENCE_TYPE_MAP[index].as02_type;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            return ESSENCE_TYPE_MAP[index].as11_type;
        case CW_OP1A_CLIP_TYPE:
            return ESSENCE_TYPE_MAP[index].op1a_type;
        case CW_AVID_CLIP_TYPE:
            return ESSENCE_TYPE_MAP[index].avid_type;
        case CW_D10_CLIP_TYPE:
            return ESSENCE_TYPE_MAP[index].d10_type;
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

MXFDescriptorHelper::EssenceType ClipWriterTrack::ConvertEssenceType(ClipWriterEssenceType essence_type)
{
    size_t index = get_essence_type_index(essence_type);

    return ESSENCE_TYPE_MAP[index].dh_type;
}

ClipWriterEssenceType ClipWriterTrack::ConvertEssenceType(MXFDescriptorHelper::EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (essence_type == ESSENCE_TYPE_MAP[i].dh_type)
            return ESSENCE_TYPE_MAP[i].type;
    }

    return CW_UNKNOWN_ESSENCE;
}

string ClipWriterTrack::EssenceTypeToString(ClipWriterEssenceType essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_STRINGS); i++) {
        if (essence_type == ESSENCE_TYPE_STRINGS[i].type)
            return ESSENCE_TYPE_STRINGS[i].str;
    }

    IM_ASSERT(false);
    return "";
}

ClipWriterTrack::ClipWriterTrack(ClipWriterEssenceType essence_type, AS02Track *track)
{
    mClipType = CW_AS02_CLIP_TYPE;
    mEssenceType = essence_type;
    mAS02Track = track;
    mAS11Track = 0;
    mOP1ATrack = 0;
    mAvidTrack = 0;
    mD10Track = 0;

    REFINE_MPEG2LG_TYPE(AS02MPEG2LGTrack, mAS02Track, SetSignalStandard, SetFrameLayout)
}

ClipWriterTrack::ClipWriterTrack(ClipWriterEssenceType essence_type, AS11Track *track)
{
    if (track->GetClipType() == AS11_OP1A_CLIP_TYPE)
        mClipType = CW_AS11_OP1A_CLIP_TYPE;
    else
        mClipType = CW_AS11_D10_CLIP_TYPE;
    mEssenceType = essence_type;
    mAS02Track = 0;
    mAS11Track = track;
    mOP1ATrack = 0;
    mAvidTrack = 0;
    mD10Track = 0;

    REFINE_MPEG2LG_TYPE(AS11Track, mAS11Track, SetMPEG2LGSignalStandard, SetMPEG2LGFrameLayout)
}

ClipWriterTrack::ClipWriterTrack(ClipWriterEssenceType essence_type, OP1ATrack *track)
{
    mClipType = CW_OP1A_CLIP_TYPE;
    mEssenceType = essence_type;
    mAS02Track = 0;
    mAS11Track = 0;
    mOP1ATrack = track;
    mAvidTrack = 0;
    mD10Track = 0;

    REFINE_MPEG2LG_TYPE(OP1AMPEG2LGTrack, mOP1ATrack, SetSignalStandard, SetFrameLayout)
}

ClipWriterTrack::ClipWriterTrack(ClipWriterEssenceType essence_type, AvidTrack *track)
{
    mClipType = CW_AVID_CLIP_TYPE;
    mEssenceType = essence_type;
    mAS02Track = 0;
    mAS11Track = 0;
    mOP1ATrack = 0;
    mAvidTrack = track;
    mD10Track = 0;

    REFINE_MPEG2LG_TYPE(AvidMPEG2LGTrack, mAvidTrack, SetSignalStandard, SetFrameLayout)
}

ClipWriterTrack::ClipWriterTrack(ClipWriterEssenceType essence_type, D10Track *track)
{
    mClipType = CW_D10_CLIP_TYPE;
    mEssenceType = essence_type;
    mAS02Track = 0;
    mAS11Track = 0;
    mOP1ATrack = 0;
    mAvidTrack = 0;
    mD10Track = track;
}

ClipWriterTrack::~ClipWriterTrack()
{
}

void ClipWriterTrack::SetOutputTrackNumber(uint32_t track_number)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Track->SetOutputTrackNumber(track_number);
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Track->SetOutputTrackNumber(track_number);
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1ATrack->SetOutputTrackNumber(track_number);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidTrack->SetOutputTrackNumber(track_number);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Track->SetOutputTrackNumber(track_number);
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetAspectRatio(Rational aspect_ratio)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PictureTrack *pict_track = dynamic_cast<AS02PictureTrack*>(mAS02Track);
            if (pict_track)
                pict_track->SetAspectRatio(aspect_ratio);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetAspectRatio(aspect_ratio);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APictureTrack *pict_track = dynamic_cast<OP1APictureTrack*>(mOP1ATrack);
            if (pict_track)
                pict_track->SetAspectRatio(aspect_ratio);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPictureTrack *pict_track = dynamic_cast<AvidPictureTrack*>(mAvidTrack);
            if (pict_track)
                pict_track->SetAspectRatio(aspect_ratio);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10MPEGTrack *mpeg_track = dynamic_cast<D10MPEGTrack*>(mD10Track);
            if (mpeg_track)
                mpeg_track->SetAspectRatio(aspect_ratio);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetComponentDepth(uint32_t depth)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02DVTrack *dv_track = dynamic_cast<AS02DVTrack*>(mAS02Track);
            AS02UncTrack *unc_track = dynamic_cast<AS02UncTrack*>(mAS02Track);
            if (dv_track)
                dv_track->SetComponentDepth(depth);
            else if (unc_track)
                unc_track->SetComponentDepth(depth);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetComponentDepth(depth);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1ADVTrack *dv_track = dynamic_cast<OP1ADVTrack*>(mOP1ATrack);
            OP1AUncTrack *unc_track = dynamic_cast<OP1AUncTrack*>(mOP1ATrack);
            if (dv_track)
                dv_track->SetComponentDepth(depth);
            else if (unc_track)
                unc_track->SetComponentDepth(depth);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidDVTrack *dv_track = dynamic_cast<AvidDVTrack*>(mAvidTrack);
            if (dv_track)
                dv_track->SetComponentDepth(depth);
            break;
        }
        case CW_D10_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetSampleSize(uint32_t size)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02D10Track *d10_track = dynamic_cast<AS02D10Track*>(mAS02Track);
            if (d10_track)
                d10_track->SetSampleSize(size);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetSampleSize(size);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1AD10Track *d10_track = dynamic_cast<OP1AD10Track*>(mOP1ATrack);
            if (d10_track)
                d10_track->SetSampleSize(size);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidD10Track *d10_track = dynamic_cast<AvidD10Track*>(mAvidTrack);
            if (d10_track)
                d10_track->SetSampleSize(size);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10MPEGTrack *mpeg_track = dynamic_cast<D10MPEGTrack*>(mD10Track);
            if (mpeg_track)
                mpeg_track->SetSampleSize(size);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetAVCIMode(AVCIMode mode)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02AVCITrack *avci_track = dynamic_cast<AS02AVCITrack*>(mAS02Track);
            if (avci_track) {
                switch (mode)
                {
                    case AVCI_PASS_MODE:
                    case AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE:
                        avci_track->SetMode(AS02_AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE);
                        break;
                    case AVCI_FIRST_FRAME_HEADER_MODE:
                        avci_track->SetMode(AS02_AVCI_FIRST_FRAME_HEADER_MODE);
                        break;
                    case AVCI_ALL_FRAME_HEADER_MODE:
                        avci_track->SetMode(AS02_AVCI_ALL_FRAME_HEADER_MODE);
                        break;
                    default:
                        log_warn("AVCI mode %d not supported\n", mode);
                        break;
                }
            }
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            switch (mode)
            {
                case AVCI_PASS_MODE:
                case AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE:
                    mAS11Track->SetAVCIMode(OP1A_AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE);
                    break;
                case AVCI_FIRST_FRAME_HEADER_MODE:
                    mAS11Track->SetAVCIMode(OP1A_AVCI_FIRST_FRAME_HEADER_MODE);
                    break;
                case AVCI_ALL_FRAME_HEADER_MODE:
                    mAS11Track->SetAVCIMode(OP1A_AVCI_ALL_FRAME_HEADER_MODE);
                    break;
                default:
                    log_warn("AVCI mode %d not supported\n", mode);
                    break;
            }
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1AAVCITrack *avci_track = dynamic_cast<OP1AAVCITrack*>(mOP1ATrack);
            if (avci_track) {
                switch (mode)
                {
                    case AVCI_PASS_MODE:
                    case AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE:
                        avci_track->SetMode(OP1A_AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE);
                        break;
                    case AVCI_FIRST_FRAME_HEADER_MODE:
                        avci_track->SetMode(OP1A_AVCI_FIRST_FRAME_HEADER_MODE);
                        break;
                    case AVCI_ALL_FRAME_HEADER_MODE:
                        avci_track->SetMode(OP1A_AVCI_ALL_FRAME_HEADER_MODE);
                        break;
                    default:
                        log_warn("AVCI mode %d not supported\n", mode);
                        break;
                }
            }
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidAVCITrack *avci_track = dynamic_cast<AvidAVCITrack*>(mAvidTrack);
            if (avci_track) {
                switch (mode)
                {
                    case AVCI_PASS_MODE:
                    case AVCI_NO_OR_ALL_FRAME_HEADER_MODE:
                        avci_track->SetMode(AVID_AVCI_NO_OR_ALL_FRAME_HEADER_MODE);
                        break;
                    case AVCI_NO_FRAME_HEADER_MODE:
                        avci_track->SetMode(AVID_AVCI_NO_FRAME_HEADER_MODE);
                        break;
                    case AVCI_ALL_FRAME_HEADER_MODE:
                        avci_track->SetMode(AVID_AVCI_ALL_FRAME_HEADER_MODE);
                        break;
                    default:
                        log_warn("AVCI mode %d not supported\n", mode);
                        break;
                }
            }
            break;
        }
        case CW_D10_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetAFD(uint8_t afd)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PictureTrack *picture_track = dynamic_cast<AS02PictureTrack*>(mAS02Track);
            if (picture_track)
                picture_track->SetAFD(afd);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetAFD(afd);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APictureTrack *picture_track = dynamic_cast<OP1APictureTrack*>(mOP1ATrack);
            if (picture_track)
                picture_track->SetAFD(afd);
            break;
        }
        case CW_AVID_CLIP_TYPE:
            break;
        case CW_D10_CLIP_TYPE:
        {
            D10MPEGTrack *mpeg_track = dynamic_cast<D10MPEGTrack*>(mD10Track);
            if (mpeg_track)
                mpeg_track->SetAFD(afd);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetInputHeight(uint32_t height)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        case CW_OP1A_CLIP_TYPE:
        case CW_D10_CLIP_TYPE:
            break;
        case CW_AVID_CLIP_TYPE:
        {
            AvidUncTrack *unc_track = dynamic_cast<AvidUncTrack*>(mAvidTrack);
            if (unc_track)
                unc_track->SetInputHeight(height);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetSamplingRate(Rational sampling_rate)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                pcm_track->SetSamplingRate(sampling_rate);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetSamplingRate(sampling_rate);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetSamplingRate(sampling_rate);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                pcm_track->SetSamplingRate(sampling_rate);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetSamplingRate(sampling_rate);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetQuantizationBits(uint32_t bits)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                pcm_track->SetQuantizationBits(bits);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetQuantizationBits(bits);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetQuantizationBits(bits);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                pcm_track->SetQuantizationBits(bits);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetQuantizationBits(bits);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetChannelCount(uint32_t count)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                pcm_track->SetChannelCount(count);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetChannelCount(count);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetChannelCount(count);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                pcm_track->SetChannelCount(count);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetChannelCount(count);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetLocked(bool locked)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                pcm_track->SetLocked(locked);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetLocked(locked);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetLocked(locked);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                pcm_track->SetLocked(locked);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetLocked(locked);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetAudioRefLevel(int8_t level)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                pcm_track->SetAudioRefLevel(level);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetAudioRefLevel(level);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetAudioRefLevel(level);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                pcm_track->SetAudioRefLevel(level);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetAudioRefLevel(level);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetDialNorm(int8_t dial_norm)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                pcm_track->SetDialNorm(dial_norm);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            mAS11Track->SetDialNorm(dial_norm);
            break;
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetDialNorm(dial_norm);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                pcm_track->SetDialNorm(dial_norm);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetDialNorm(dial_norm);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::SetSequenceOffset(uint8_t offset)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                pcm_track->SetSequenceOffset(offset);
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Track->SetSequenceOffset(offset);
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetSequenceOffset(offset);
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                pcm_track->SetSequenceOffset(offset);
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetSequenceOffset(offset);
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void ClipWriterTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Track->WriteSamples(data, size, num_samples);
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Track->WriteSamples(data, size, num_samples);
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1ATrack->WriteSamples(data, size, num_samples);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidTrack->WriteSamples(data, size, num_samples);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Track->WriteSamples(data, size, num_samples);
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

bool ClipWriterTrack::IsPicture() const
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
            return mAS02Track->IsPicture();
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            return mAS11Track->IsPicture();
        case CW_OP1A_CLIP_TYPE:
            return mOP1ATrack->IsPicture();
        case CW_AVID_CLIP_TYPE:
            return mAvidTrack->IsPicture();
        case CW_D10_CLIP_TYPE:
            return mD10Track->IsPicture();
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return false;
}

uint32_t ClipWriterTrack::GetSampleSize() const
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
            return mAS02Track->GetSampleSize();
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            return mAS11Track->GetSampleSize();
        case CW_OP1A_CLIP_TYPE:
            return mOP1ATrack->GetSampleSize();
        case CW_AVID_CLIP_TYPE:
            return mAvidTrack->GetSampleSize();
        case CW_D10_CLIP_TYPE:
            return mD10Track->GetSampleSize();
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

uint32_t ClipWriterTrack::GetInputSampleSize() const
{
    switch (mClipType)
    {
        case CW_AVID_CLIP_TYPE:
        {
            AvidUncTrack *unc_track = dynamic_cast<AvidUncTrack*>(mAvidTrack);
            if (unc_track)
                return unc_track->GetInputSampleSize();
            break;
        }
        default:
            break;
    }

    return GetSampleSize();
}

uint32_t ClipWriterTrack::GetAVCISampleWithoutHeaderSize() const
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02AVCITrack *avci_track = dynamic_cast<AS02AVCITrack*>(mAS02Track);
            if (avci_track)
                return avci_track->GetSampleWithoutHeaderSize();
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        {
            return mAS11Track->GetAVCISampleWithoutHeaderSize();
        }
        case CW_OP1A_CLIP_TYPE:
        {
            OP1AAVCITrack *avci_track = dynamic_cast<OP1AAVCITrack*>(mOP1ATrack);
            if (avci_track)
                return avci_track->GetSampleWithoutHeaderSize();
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidAVCITrack *avci_track = dynamic_cast<AvidAVCITrack*>(mAvidTrack);
            if (avci_track)
                return avci_track->GetSampleWithoutHeaderSize();
            break;
        }
        case CW_D10_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

bool ClipWriterTrack::IsSingleField() const
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
        case CW_OP1A_CLIP_TYPE:
        case CW_D10_CLIP_TYPE:
            break;
        case CW_AVID_CLIP_TYPE:
        {
            AvidMJPEGTrack *mjpeg_track = dynamic_cast<AvidMJPEGTrack*>(mAvidTrack);
            if (mjpeg_track)
                return mjpeg_track->IsSingleField();
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return false;
}

vector<uint32_t> ClipWriterTrack::GetShiftedSampleSequence() const
{
    switch (mClipType)
    {
        case CW_AS02_CLIP_TYPE:
        {
            AS02PCMTrack *pcm_track = dynamic_cast<AS02PCMTrack*>(mAS02Track);
            if (pcm_track)
                return pcm_track->GetShiftedSampleSequence();
            break;
        }
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            return mAS11Track->GetShiftedSampleSequence();
        case CW_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                return pcm_track->GetShiftedSampleSequence();
            break;
        }
        case CW_AVID_CLIP_TYPE:
        {
            AvidPCMTrack *pcm_track = dynamic_cast<AvidPCMTrack*>(mAvidTrack);
            if (pcm_track)
                return pcm_track->GetShiftedSampleSequence();
            break;
        }
        case CW_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                return pcm_track->GetShiftedSampleSequence();
            break;
        }
        case CW_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return vector<uint32_t>(1);
}

