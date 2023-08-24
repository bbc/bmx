/*
 * Defines MXF package data structures and functions to create them
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

#ifndef PACKAGE_DEFINITIONS_H_
#define PACKAGE_DEFINITIONS_H_


#ifdef __cplusplus
extern "C"
{
#endif



#include <mxf/mxf_types.h>
#include <mxf/mxf_list.h>
#include <mxf/mxf_utils.h>


#define MAX_LOCATORS        128


typedef enum
{
    AvidMJPEG,
    IECDV25,
    DVBased25,
    DVBased50,
    DV1080i,
    DV720p,
    IMX30,
    IMX40,
    IMX50,
    DNxHD1080p1235,
    DNxHD1080p1237,
    DNxHD1080p1238,
    DNxHD1080i1241,
    DNxHD1080i1242,
    DNxHD1080i1243,
    DNxHD720p1250,
    DNxHD720p1251,
    DNxHD720p1252,
    DNxHD1080p1253,
    UncUYVY,
    Unc1080iUYVY,
    Unc1080pUYVY,
    Unc720pUYVY,
    PCM
} EssenceType;

typedef enum
{
    Res21,
    Res31,
    Res101,
    Res41m,
    Res101m,
    Res151s,
    Res201
} AvidMJPEGResolution;

typedef enum
{
    AVID_WHITE = 0,
    AVID_RED,
    AVID_YELLOW,
    AVID_GREEN,
    AVID_CYAN,
    AVID_BLUE,
    AVID_MAGENTA,
    AVID_BLACK
} AvidRGBColor;



typedef struct
{
    mxfRational imageAspectRatio;
    AvidMJPEGResolution mjpegResolution;
    int imxFrameSize;

    int pcmBitsPerSample;
    int locked;         /* not set if value != 0 and value != 1 */
    int audioRefLevel;  /* not set if value < -128 or value > 127 */
    int dialNorm;       /* not set if value < -128 or value > 127 */
    int sequenceOffset; /* not set if value < 0 or value >= 5 */

    /* inputHeight is used for UncUYVY to determine how many lines of VBI the input image contains */
    uint32_t inputHeight;
} EssenceInfo;


typedef struct
{
    char *name;
    char *value;
} UserComment;


typedef struct
{
    mxfUMID uid;
    char *name;
    mxfTimestamp creationDate;
    MXFList tracks;
    char *filename;
    EssenceType essenceType;
    EssenceInfo essenceInfo;
} Package;


typedef struct
{
    uint32_t id;
    uint32_t number;
    char *name;
    int isPicture;
    mxfRational editRate;
    int64_t origin;
    mxfUMID sourcePackageUID;
    uint32_t sourceTrackID;
    int64_t startPosition;
    int64_t length;
} Track;


typedef struct
{
    int64_t position;
    int64_t duration;
    char *comment;
    AvidRGBColor color;
} Locator;


typedef struct PackageDefinitions
{
    Package *materialPackage;
    MXFList fileSourcePackages;
    Package *tapeSourcePackage;
    /* user comments are attached to the material package */
    MXFList userComments;
    /* locators are added to an event track in the material package */
    mxfRational locatorEditRate;
    MXFList locators;
} PackageDefinitions;


int create_package_definitions(PackageDefinitions **definitions, const mxfRational *locatorEditRate);
void free_package_definitions(PackageDefinitions **definitions);

void init_essence_info(EssenceInfo *essenceInfo);

int create_material_package(PackageDefinitions *definitions, const mxfUMID *uid, const char *name,
                            const mxfTimestamp *creationDate);
int create_file_source_package(PackageDefinitions *definitions, const mxfUMID *uid, const char *name,
                               const mxfTimestamp *creationDate, const char *filename, EssenceType essenceType,
                               const EssenceInfo *essenceInfo, Package **filePackage);
int create_tape_source_package(PackageDefinitions *definitions, const mxfUMID *uid, const char *name,
                               const mxfTimestamp *creationDate);

int set_user_comment(PackageDefinitions *definitions, const char *name, const char *value);
void clear_user_comments(PackageDefinitions *definitions);

int add_locator(PackageDefinitions *definitions, int64_t position, const char *comment, AvidRGBColor color);
void clear_locators(PackageDefinitions *definitions);

/* note: number is ignored for file source packages */
int create_track(Package *package, uint32_t id, uint32_t number, const char *name, int isPicture,
                 const mxfRational *editRate, const mxfUMID *sourcePackageUID, uint32_t sourceTrackID,
                 int64_t startPosition, int64_t length, int64_t origin, Track **track);


void get_image_aspect_ratio(PackageDefinitions *definitions, const mxfRational *defaultImageAspectRatio,
                            mxfRational *imageAspectRatio);


#ifdef __cplusplus
}
#endif


#endif

