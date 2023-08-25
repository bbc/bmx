/*
 * Internal functions for reading MXF files
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

#ifndef MXF_READER_INT_H_
#define MXF_READER_INT_H_


#include "mxf_reader.h"


typedef struct EssenceReaderData EssenceReaderData;

typedef struct EssenceTrack
{
    struct EssenceTrack *next;

    int isVideo;
    uint32_t trackNumber;

    int64_t frameSize;  /* -1 indicates variable frame size, 0 indicates sequence */
    uint32_t frameSizeSeq[15];
    int frameSizeSeqSize;
    int64_t frameSeqSize;

    mxfRational frameRate; /* required playout frame rate */
    int64_t playoutDuration;

    mxfRational sampleRate; /* sample rate of essence container */
    int64_t containerDuration;

    uint32_t imageStartOffset; /* used for Avid unc frames which are aligned to 8k boundaries */

    int32_t avidFirstFrameOffset; /* Avid extension: offset to first frame in clip-wrapped container */

    uint32_t bodySID;
    uint32_t indexSID;
} EssenceTrack;

typedef struct
{
    EssenceTrack *essenceTracks;

    void (*close) (MXFReader *reader);
    int (*position_at_frame) (MXFReader *reader, int64_t frameNumber);
    int (*skip_next_frame) (MXFReader *reader);
    int (*read_next_frame) (MXFReader *reader, MXFReaderListener *listener);
    int64_t (*get_next_frame_number) (MXFReader *reader);
    int64_t (*get_last_written_frame_number) (MXFReader *reader);
    MXFHeaderMetadata* (*get_header_metadata) (MXFReader *reader);
    int (*have_footer_metadata)(MXFReader *reader);
    int (*set_frame_rate)(MXFReader *reader, const mxfRational *frameRate);

    EssenceReaderData *data;
} EssenceReader;

typedef struct
{
    mxfPosition startTimecode;
    mxfLength duration;
} TimecodeSegment;

typedef struct
{
    int type;
    int count;

    int isDropFrame;
    uint16_t roundedTimecodeBase;

    /* playout and source timeodes originating from the header metadata */
    MXFList segments;

    /* source timecodes originating from the system or video item in the essence container */
    mxfPosition position;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
} TimecodeIndex;

struct MXFReader
{
    MXFFile *mxfFile;
    MXFClip clip;

    int isMetadataOnly;

    int haveReadAFrame; /* is true if a frame has been read and therefore the number of source timecodes is up to date */
    TimecodeIndex playoutTimecodeIndex;
    MXFList sourceTimecodeIndexes;

    uint32_t *archiveCRC32;
    uint32_t numArchiveCRC32Alloc;
    uint32_t numArchiveCRC32;

    EssenceReader *essenceReader;

    MXFDataModel *dataModel;
    int ownDataModel;  /* the reader will free it when closed */

    /* buffer for internal use */
    uint8_t *buffer;
    uint32_t bufferSize;
};


static const mxfRational g_palFrameRate = {25, 1};
static const mxfRational g_ntscFrameRate = {30000, 1001};
static const mxfRational g_profAudioSamplingRate = {48000, 1};



int add_track(MXFReader *reader, MXFTrack **track);

int add_essence_track(EssenceReader *essenceReader, EssenceTrack **essenceTrack);
int get_num_essence_tracks(EssenceReader *essenceReader);
EssenceTrack* get_essence_track(EssenceReader *essenceReader, int trackIndex);
int get_essence_track_with_tracknumber(EssenceReader *essenceReader, uint32_t trackNumber,
    EssenceTrack**, int *trackIndex);

void clean_rate(mxfRational *rate);

int initialise_playout_timecode(MXFReader *reader, MXFMetadataSet *materialPackageSet);
int initialise_default_playout_timecode(MXFReader *reader);

int initialise_source_timecodes(MXFReader *reader, MXFMetadataSet *sourcePackageSet);
int set_essence_container_timecode(MXFReader *reader, mxfPosition position,
    int type, int count, int isDropFrame, uint8_t hour, uint8_t min, uint8_t sec, uint8_t frame);

int allocate_archive_crc32(MXFReader *reader, uint32_t num);
int set_archive_crc32(MXFReader *reader, uint32_t index, uint32_t crc32);

int64_t mxfr_convert_length(const mxfRational *frameRateIn, int64_t lengthIn, const mxfRational *frameRateOut);

int get_clip_duration(MXFHeaderMetadata *headerMetadata, MXFClip *clip, int isOPAtom);

int mxfr_is_pal_frame_rate(const mxfRational *frameRate);
int mxfr_is_ntsc_frame_rate(const mxfRational *frameRate);
int mxfr_is_prof_sampling_rate(const mxfRational *samplingRate);


#endif

