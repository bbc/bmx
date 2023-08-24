/*
 * Main functions for reading MXF files
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

#ifndef MXF_READER_H_
#define MXF_READER_H_


#ifdef __cplusplus
extern "C"
{
#endif


#include <mxf/mxf.h>

/* timecode types */
#define PLAYOUT_TIMECODE                        0x00
#define FILE_SOURCE_PACKAGE_TIMECODE            0x01
#define SYSTEM_ITEM_TC_ARRAY_TIMECODE           0x02
#define SYSTEM_ITEM_SDTI_CREATION_TIMECODE      0x03
#define SYSTEM_ITEM_SDTI_USER_TIMECODE          0x04
#define AVID_FILE_SOURCE_PACKAGE_TIMECODE       0x05



typedef struct MXFReader MXFReader;

typedef struct MXFReaderListenerData MXFReaderListenerData;

typedef struct MXFReaderListener
{
    /* returns true if listener will accept frame */
    int (*accept_frame)(struct MXFReaderListener *listener, int trackIndex);

    /* the listener must allocate a buffer for the data to be written to */
    int (*allocate_buffer)(struct MXFReaderListener *listener, int trackIndex, uint8_t **buffer, uint32_t bufferSize);

    /* this function is only called if the reader fails to read a frame _before_ calling receive_frame */
    void (*deallocate_buffer)(struct MXFReaderListener *listener, int trackIndex, uint8_t **buffer);

    /* passes the frame to the listener
    The buffer pointer equals the pointer for the data allocated in allocate_buffer()
    The listener is responsible for deleting the buffer data */
    int (*receive_frame)(struct MXFReaderListener *listener, int trackIndex, uint8_t *buffer, uint32_t bufferSize);

    MXFReaderListenerData *data;
} MXFReaderListener;

typedef struct
{
    mxfRational frameRate;
    uint32_t frameWidth;
    uint32_t frameHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
    uint32_t displayXOffset;
    uint32_t displayYOffset;
    uint32_t horizSubsampling;
    uint32_t vertSubsampling;
    uint32_t componentDepth;
    mxfRational aspectRatio;
    uint8_t signalStandard;
    uint8_t frameLayout;
} MXFVideoTrack;

typedef struct
{
    mxfRational samplingRate;
    uint32_t bitsPerSample;
    uint16_t blockAlign;
    uint32_t channelCount;
} MXFAudioTrack;

typedef struct MXFTrack
{
    struct MXFTrack *next;

    mxfUL essenceContainerLabel;
    mxfUL pictureEssenceCodingLabel;

    uint32_t materialTrackID;
    uint32_t materialTrackNumber;

    int isVideo;
    MXFVideoTrack video;
    MXFAudioTrack audio;
} MXFTrack;

typedef struct
{
    MXFTrack *tracks;
    char *tracksString;
    mxfRational frameRate;
    int64_t duration; /* -1 indicates unknown */
    int64_t minDuration; /* duration thus far */
    int hasAssociatedVideo; /* does the clip (material package) have a video track */
} MXFClip;

typedef struct
{
    int isDropFrame;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
} MXFTimecode;

int format_is_supported(MXFFile *mxfFile);

int open_mxf_reader(const char *filename, MXFReader **reader);
int open_mxf_reader_2(const char *filename, MXFDataModel *dataModel, MXFReader **reader);
int init_mxf_reader(MXFFile **mxfFile, MXFReader **reader);
int init_mxf_reader_2(MXFFile **mxfFile, MXFDataModel *dataModel, MXFReader **reader);
void close_mxf_reader(MXFReader **reader);

int is_metadata_only(MXFReader *reader);

MXFClip* get_mxf_clip(MXFReader *reader);
MXFTrack* get_mxf_track(MXFReader *reader, int trackIndex);
void get_frame_rate(MXFReader *reader, mxfRational *frameRate);
int64_t get_duration(MXFReader *reader);
int64_t get_min_duration(MXFReader *reader);
int get_num_tracks(MXFReader *reader);
int clip_has_video(MXFReader *reader);
int set_frame_rate(MXFReader *reader, const mxfRational *frameRate);
const char* get_tracks_string(MXFReader *reader);

MXFHeaderMetadata* get_header_metadata(MXFReader *reader);
int have_footer_metadata(MXFReader *reader);


int mxfr_is_seekable(MXFReader *reader);
int position_at_frame(MXFReader *reader, int64_t frameNumber);
int position_at_playout_timecode(MXFReader *reader, MXFTimecode *timecode);
int position_at_source_timecode(MXFReader *reader, MXFTimecode *timecode, int type, int count);
int skip_next_frame(MXFReader *reader);
/* returns 1 if successfull, -1 if EOF, 0 if failed */
int read_next_frame(MXFReader *reader, MXFReaderListener *listener);



/* functions to return info about the last frame read */

/* returns the number of the last frame read. equals -1 if no frame has been
read at the start. ranges from -1 to (duration - 1) */
int64_t get_frame_number(MXFReader *reader);

int get_playout_timecode(MXFReader *reader, MXFTimecode *timecode);
int get_num_source_timecodes(MXFReader *reader);
int get_source_timecode_type(MXFReader *reader, int index);
/* returns 1 if successfull, -1 if the timecode is unavailable, otherwise 0 indicating an error
   A timecode is unavailable if it is a timecode in the essence container that is not present
   or the frame needs to be read to extract it (eg. after a position or skip) */
int get_source_timecode(MXFReader *reader, int index, MXFTimecode *timecode, int *type, int *count);

int get_num_archive_crc32(MXFReader *reader);
int get_archive_crc32(MXFReader *reader, int index, uint32_t *crc32);


/* returns the last frame that can be read from the file */

int64_t get_last_written_frame_number(MXFReader *reader);


#ifdef __cplusplus
}
#endif

#endif

