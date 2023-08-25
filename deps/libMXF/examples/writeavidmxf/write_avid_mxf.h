/*
 * Write video and audio to MXF files supported by Avid editing software
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

#ifndef WRITE_AVID_MXF_H_
#define WRITE_AVID_MXF_H_


#ifdef __cplusplus
extern "C"
{
#endif


#include "package_definitions.h"


typedef struct AvidClipWriter AvidClipWriter;

typedef enum
{
    PAL_25i,
    NTSC_30i
} ProjectFormat;


/* create the writer */
int create_clip_writer(const char *projectName, ProjectFormat projectFormat, mxfRational projectEditRate,
                       int dropFrameFlag, int useLegacy, PackageDefinitions *packageDefinitions,
                       AvidClipWriter **clipWriter);


/* write essence samples
    the number of samples is a multiple of the file package track edit rate
    eg. if the edit rate is 48000/1 then the number of sample in a PAL video frame
        is 1920. If the edit rate is 25/1 in the audio track then the number of
        samples is 1

    Note: numSamples must equal 1 for variable size samples (eg. MJPEG) to allow indexing to work
    Note: numSamples must equal 1 for uncompressed video
*/
int write_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID, uint32_t numSamples,
                  const uint8_t *data, uint32_t size);

/* same as write_samples, but data is written in multiple calls */
int start_write_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID);
int write_sample_data(AvidClipWriter *clipWriter, uint32_t materialTrackID, const uint8_t *data, uint32_t size);
int end_write_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID, uint32_t numSamples);


/* returns num_samples written as a multiple of the file package track edit rate */
int get_num_samples(AvidClipWriter *clipWriter, uint32_t materialTrackID, int64_t *num_samples);

/* returns the stored dimensions for the given picture track */
int get_stored_dimensions(AvidClipWriter *clipWriter, uint32_t materialTrackID, uint32_t *width, uint32_t *height);

/* delete the output files and free the writer */
void abort_writing(AvidClipWriter **clipWriter, int deleteFile);

/* complete and save the output files and free the writer */
int complete_writing(AvidClipWriter **clipWriter);

/* update the metadata, complete and save the output files and free the writer */
int update_and_complete_writing(AvidClipWriter **clipWriter, PackageDefinitions *packageDefinitions,
                                const char *projectName);



#ifdef __cplusplus
}
#endif


#endif

