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

#ifndef WRITE_ARCHIVE_MXF_H_
#define WRITE_ARCHIVE_MXF_H_


#ifdef __cplusplus
extern "C"
{
#endif


#include <mxf/mxf_app_types.h>
#include <mxf/mxf_file.h>


#define MAX_ARCHIVE_AUDIO_TRACKS        16


typedef struct ArchiveMXFWriter ArchiveMXFWriter;


/* create a new Archive MXF file and prepare for writing the essence */
int prepare_archive_mxf_file(const char *filename, const mxfRational *frameRate, uint8_t signalStandard,
                             uint8_t frameLayout, uint32_t componentDepth, const mxfRational *aspectRatio,
                             int numAudioTracks, uint32_t audioQuantBits, int includeCRC32, int64_t startPosition,
                             ArchiveMXFWriter **output);

/* use the Archive MXF file (the filename is only used as metadata) and prepare for writing the essence */
/* note: if this function returns 0 then check whether *mxfFile is not NULL and needs to be closed */
int prepare_archive_mxf_file_2(MXFFile **mxfFile, const char *filename, const mxfRational *frameRate,
                               uint8_t signalStandard, uint8_t frameLayout, uint32_t componentDepth,
                               const mxfRational *aspectRatio, int numAudioTracks, uint32_t audioQuantBits,
                               int includeCRC32, int64_t startPosition, ArchiveMXFWriter **output);


/* write the essence, in order, starting with the system item, followed by video and then 0 or more audio */
int write_system_item(ArchiveMXFWriter *output, ArchiveTimecode vitc, ArchiveTimecode ltc, const uint32_t *crc32,
                      int numCRC32);
int write_video_frame(ArchiveMXFWriter *output, const uint8_t *data, uint32_t size);
int write_audio_frame(ArchiveMXFWriter *output, const uint8_t *data, uint32_t size);

/* close and delete the file and free output */
int abort_archive_mxf_file(ArchiveMXFWriter **output);

/* write the header metadata, do misc. fixups, close the file and free output */
int complete_archive_mxf_file(ArchiveMXFWriter **output, const InfaxData *sourceInfaxData,
                              const PSEFailure *pseFailures, long numPSEFailures,
                              const VTRError *vtrErrors, long numVTRErrors,
                              const DigiBetaDropout *digiBetaDropouts, long numDigiBetaDropouts,
                              const TimecodeBreak *timecodeBreaks, long numTimecodeBreaks);

int64_t get_archive_mxf_file_size(ArchiveMXFWriter *writer);

mxfUMID get_material_package_uid(ArchiveMXFWriter *writer);
mxfUMID get_file_package_uid(ArchiveMXFWriter *writer);
mxfUMID get_tape_package_uid(ArchiveMXFWriter *writer);


/* update the file source package in the header metadata with the infax data */
int update_archive_mxf_file(const char *filePath, const char *newFilename, const InfaxData *ltoInfaxData);

/* use the Archive MXF file, update the file source package in the header metadata with the infax data */
/* note: if this function returns 0 then check whether *mxfFile is not NULL and needs to be closed */
int update_archive_mxf_file_2(MXFFile **mxfFile, const char *newFilename, const InfaxData *ltoInfaxData);


/* return video and audio frame sizes */
uint32_t get_video_frame_size(const mxfRational *frameRate, uint8_t signalStandard, uint32_t componentDepth);
uint32_t get_audio_frame_size(uint32_t numSamples, uint32_t audioQuantBits);
int get_audio_sample_sequence(const mxfRational *frameRate, uint8_t maxSequenceSize, uint32_t *sequence,
                              uint8_t *sequenceSize);

/* returns the content package (system, video + x audio elements) size */
uint32_t get_archive_mxf_content_package_size(const mxfRational *frameRate, uint8_t signalStandard,
                                              uint32_t componentDepth, int numAudioTracks, uint32_t audioQuantBits,
                                              int includeCRC32);


int parse_infax_data(const char *infaxDataString, InfaxData *infaxData);

uint32_t calc_crc32(const uint8_t *data, uint32_t size);


#ifdef __cplusplus
}
#endif


#endif

