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

#ifndef BMX_ESSENCE_TYPE_H_
#define BMX_ESSENCE_TYPE_H_



namespace bmx
{


typedef enum
{
    UNKNOWN_ESSENCE_TYPE,
    // generic
    PICTURE_ESSENCE,
    SOUND_ESSENCE,
    DATA_ESSENCE,
    // D-10 video
    D10_30,
    D10_40,
    D10_50,
    // DV
    IEC_DV25,
    DVBASED_DV25,
    DV50,
    DV100_1080I,
    DV100_720P,
    // AVC Intra RP2027
    AVCI200_1080I,
    AVCI200_1080P,
    AVCI200_720P,
    AVCI100_1080I,
    AVCI100_1080P,
    AVCI100_720P,
    AVCI50_1080I,
    AVCI50_1080P,
    AVCI50_720P,
    // AVC Intra Unconstrained
    AVC_HIGH_10_INTRA_UNCS,
    AVC_HIGH_422_INTRA_UNCS,
    // Uncompressed video
    UNC_SD,
    UNC_HD_1080I,
    UNC_HD_1080P,
    UNC_HD_720P,
    UNC_UHD_3840,
    AVID_10BIT_UNC_SD,
    AVID_10BIT_UNC_HD_1080I,
    AVID_10BIT_UNC_HD_1080P,
    AVID_10BIT_UNC_HD_720P,
    AVID_ALPHA_SD,
    AVID_ALPHA_HD_1080I,
    AVID_ALPHA_HD_1080P,
    AVID_ALPHA_HD_720P,
    // MPEG-2 Long GOP HD
    MPEG2LG_422P_HL_1080I,
    MPEG2LG_422P_HL_1080P,
    MPEG2LG_422P_HL_720P,
    MPEG2LG_MP_HL_1920_1080I,
    MPEG2LG_MP_HL_1920_1080P,
    MPEG2LG_MP_HL_1440_1080I,
    MPEG2LG_MP_HL_1440_1080P,
    MPEG2LG_MP_HL_720P,
    MPEG2LG_MP_H14_1080I,
    MPEG2LG_MP_H14_1080P,
    // VC-3
    VC3_1080P_1235,
    VC3_1080P_1237,
    VC3_1080P_1238,
    VC3_1080I_1241,
    VC3_1080I_1242,
    VC3_1080I_1243,
    VC3_1080I_1244,
    VC3_720P_1250,
    VC3_720P_1251,
    VC3_720P_1252,
    VC3_1080P_1253,
    VC3_720P_1258,
    VC3_1080P_1259,
    VC3_1080I_1260,
    // Avid MJPEG
    MJPEG_2_1,
    MJPEG_3_1,
    MJPEG_10_1,
    MJPEG_20_1,
    MJPEG_4_1M,
    MJPEG_10_1M,
    MJPEG_15_1S,
    // PCM
    WAVE_PCM,
    D10_AES3_PCM,
    // ST 436 data
    ANC_DATA,
    VBI_DATA,
} EssenceType;


const char* essence_type_to_string(EssenceType essence_type);
const char* essence_type_to_enum_string(EssenceType essence_type);

EssenceType get_generic_essence_type(EssenceType essence_type);


};


#endif

