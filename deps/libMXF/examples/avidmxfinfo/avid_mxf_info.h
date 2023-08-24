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

#ifndef AVID_MXF_INFO_H_
#define AVID_MXF_INFO_H_


#ifdef __cplusplus
extern "C"
{
#endif


#include <mxf/mxf_types.h>


/* Note: keep get_essence_type_string() in sync when changes are made here */
typedef enum
{
    UNKNOWN_ESSENCE_TYPE = 0,
    MPEG_30_ESSENCE_TYPE,
    MPEG_40_ESSENCE_TYPE,
    MPEG_50_ESSENCE_TYPE,
    DV_25_411_ESSENCE_TYPE,
    DV_25_420_ESSENCE_TYPE,
    DV_50_ESSENCE_TYPE,
    DV_100_ESSENCE_TYPE,
    MJPEG_20_1_ESSENCE_TYPE,
    MJPEG_2_1_S_ESSENCE_TYPE,
    MJPEG_4_1_S_ESSENCE_TYPE,
    MJPEG_15_1_S_ESSENCE_TYPE,
    MJPEG_10_1_ESSENCE_TYPE,
    MJPEG_10_1_M_ESSENCE_TYPE,
    MJPEG_4_1_M_ESSENCE_TYPE,
    MJPEG_3_1_ESSENCE_TYPE,
    MJPEG_2_1_ESSENCE_TYPE,
    UNC_1_1_ESSENCE_TYPE,
    UNC_1_1_10B_ESSENCE_TYPE,
    MJPEG_35_1_P_ESSENCE_TYPE,
    MJPEG_28_1_P_ESSENCE_TYPE,
    MJPEG_14_1_P_ESSENCE_TYPE,
    MJPEG_3_1_P_ESSENCE_TYPE,
    MJPEG_2_1_P_ESSENCE_TYPE,
    MJPEG_3_1_M_ESSENCE_TYPE,
    MJPEG_8_1_M_ESSENCE_TYPE,
    DNXHD_1235_ESSENCE_TYPE,
    DNXHD_1237_ESSENCE_TYPE,
    DNXHD_1238_ESSENCE_TYPE,
    DNXHD_1241_ESSENCE_TYPE,
    DNXHD_1242_ESSENCE_TYPE,
    DNXHD_1243_ESSENCE_TYPE,
    DNXHD_1250_ESSENCE_TYPE,
    DNXHD_1251_ESSENCE_TYPE,
    DNXHD_1252_ESSENCE_TYPE,
    DNXHD_1253_ESSENCE_TYPE,
    MPEG4_ESSENCE_TYPE,
    XDCAM_HD_ESSENCE_TYPE,
    AVCINTRA_100_ESSENCE_TYPE,
    AVCINTRA_50_ESSENCE_TYPE,
    PCM_ESSENCE_TYPE
} AvidEssenceType;

typedef enum
{
    UNKNOWN_PHYS_TYPE = 0,
    TAPE_PHYS_TYPE,
    IMPORT_PHYS_TYPE,
    RECORDING_PHYS_TYPE
} AvidPhysicalPackageType;

typedef enum
{
    FULL_FRAME_FRAME_LAYOUT = 0,
    SEPARATE_FIELDS_FRAME_LAYOUT,
    SINGLE_FIELD_FRAME_LAYOUT,
    MIXED_FIELD_FRAME_LAYOUT,
    SEGMENTED_FRAME_FRAME_LAYOUT,
} AvidFrameLayout;

typedef struct
{
    char *name;
    char *value;
} AvidNameValuePair;

typedef struct
{
    char *name;
    char *value;
    AvidNameValuePair *attributes;
    int numAttributes;
} AvidTaggedValue;

typedef struct
{
    /* clip info */
    char *clipName;
    char *projectName;
    mxfTimestamp clipCreated;
    mxfRational projectEditRate;
    int64_t clipDuration;
    mxfUMID materialPackageUID;
    AvidTaggedValue *userComments;
    int numUserComments;
    AvidTaggedValue *materialPackageAttributes;
    int numMaterialPackageAttributes;
    int numVideoTracks;
    int numAudioTracks;
    char *tracksString;

    /* track info */
    uint32_t trackNumber;
    int isVideo;
    mxfRational editRate;
    int64_t trackDuration;
    int64_t segmentDuration;
    int64_t segmentOffset;
    int64_t startTimecode;

    /* file essence info */
    AvidEssenceType essenceType;
    mxfUMID fileSourcePackageUID;
    mxfUL essenceContainerLabel;

    /* picture info */
    mxfUL pictureCodingLabel;
    uint8_t frameLayout;
    mxfRational aspectRatio;
    uint32_t storedWidth;
    uint32_t storedHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;

    /* sound info */
    mxfRational audioSamplingRate;
    uint32_t channelCount;
    uint32_t quantizationBits;

    /* physical source info */
    char *physicalPackageName;
    mxfUMID physicalSourcePackageUID;
    AvidPhysicalPackageType physicalPackageType;
    char *physicalPackageLocator;

} AvidMXFInfo;



int ami_read_info(const char *filename, AvidMXFInfo *info, int printDebugError);

void ami_free_info(AvidMXFInfo *info);

void ami_print_info(AvidMXFInfo *info);
/* TODO: add function to print machine readable info e.g. newline delimited fields */



#ifdef __cplusplus
}
#endif


#endif

