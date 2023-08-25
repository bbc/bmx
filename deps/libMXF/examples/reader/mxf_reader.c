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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "mxf_reader_int.h"
#include <mxf/mxf_uu_metadata.h>
#include "mxf_opatom_reader.h"
#include "mxf_op1a_reader.h"
#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>


typedef struct
{
    uint32_t first;
    uint32_t last;
} TrackNumberRange;

typedef struct
{
    uint32_t framesPerMin;
    uint32_t nonDropFramesPerMin;
    uint32_t framesPer10Min;
    uint32_t framesPerHour;
    int64_t maxOffset;
    int16_t dropCount;
} TimecodeInfo;



static void insert_track_number(TrackNumberRange *trackNumbers, uint32_t trackNumber, int *numTrackNumberRanges)
{
    int i;
    int j;

    for (i = 0; i < *numTrackNumberRanges; i++)
    {
        if (trackNumber < trackNumbers[i].first - 1)
        {
            /* insert new track range */
            for (j = *numTrackNumberRanges - 1; j >= i; j--)
            {
                trackNumbers[j + 1] = trackNumbers[j];
            }
            trackNumbers[i].first = trackNumber;
            trackNumbers[i].last = trackNumber;

            (*numTrackNumberRanges)++;
            return;
        }
        else if (trackNumber == trackNumbers[i].first - 1)
        {
            /* extend range back one */
            trackNumbers[i].first = trackNumber;
            return;
        }
        else if (trackNumber == trackNumbers[i].last + 1)
        {
            if (i + 1 < *numTrackNumberRanges &&
                trackNumber == trackNumbers[i + 1].first - 1)
            {
                /* merge range forwards */
                trackNumbers[i + 1].first = trackNumbers[i].first;
                for (j = i; j < *numTrackNumberRanges - 1; j++)
                {
                    trackNumbers[j] = trackNumbers[j + 1];
                }
                (*numTrackNumberRanges)--;
            }
            else
            {
                /* extend range forward one */
                trackNumbers[i].last = trackNumber;
            }

            return;
        }
        else if (trackNumber == trackNumbers[i].first ||
            trackNumber == trackNumbers[i].last)
        {
            /* duplicate */
            return;
        }
    }


    /* append new track range */
    trackNumbers[i].first = trackNumber;
    trackNumbers[i].last = trackNumber;

    (*numTrackNumberRanges)++;
    return;
}

static size_t get_range_string(char *buffer, size_t maxSize, int isFirst, int isVideo, const TrackNumberRange *range)
{
    size_t strLen = 0;

    if (isFirst)
    {
        mxf_snprintf(buffer, maxSize, "%s", (isVideo) ? "V" : "A");
        strLen = strlen(buffer);
        buffer += strLen;
        maxSize -= strLen;
    }
    else
    {
        mxf_snprintf(buffer, maxSize, ",");
        strLen = strlen(buffer);
        buffer += strLen;
        maxSize -= strLen;
    }


    if (range->first == range->last)
    {
        mxf_snprintf(buffer, maxSize, "%d", range->first);
        strLen += strlen(buffer);
    }
    else
    {
        mxf_snprintf(buffer, maxSize, "%d-%d", range->first, range->last);
        strLen += strlen(buffer);
    }

    return strLen;
}

static int create_tracks_string(MXFReader *reader)
{
    MXFTrack *track;
    TrackNumberRange videoTrackNumberRanges[64];
    int numVideoTrackNumberRanges = 0;
    TrackNumberRange audioTrackNumberRanges[64];
    int numAudioTrackNumberRanges = 0;
    char tracksString[256] = {0};
    int i;
    uint32_t c;
    size_t remSize;
    char *tracksStringPtr;
    size_t strLen;
    int videoTrackNumber, audioTrackNumber;

    /* create track number ranges for audio and video */
    /* if the material track number is <= 0 or the audio is not 1 channel then this track number range method
       is not used and a simple count is used instead */
    track = reader->clip.tracks;
    while (track != NULL)
    {
        if (track->materialTrackNumber <= 0 ||
            (!track->isVideo && track->audio.channelCount != 1))
        {
            break;
        }

        if (track->isVideo)
        {
            insert_track_number(videoTrackNumberRanges, track->materialTrackNumber, &numVideoTrackNumberRanges);
        }
        else
        {
            insert_track_number(audioTrackNumberRanges, track->materialTrackNumber, &numAudioTrackNumberRanges);
        }
        track = track->next;
    }
    if (track != NULL)
    {
        /* ignore material track numbers and use a simple count instead */
        memset(&videoTrackNumberRanges, 0, sizeof(videoTrackNumberRanges));
        numVideoTrackNumberRanges = 0;
        memset(&audioTrackNumberRanges, 0, sizeof(audioTrackNumberRanges));
        numAudioTrackNumberRanges = 0;
        videoTrackNumber = 1;
        audioTrackNumber = 1;
        track = reader->clip.tracks;
        while (track != NULL)
        {
            if (track->isVideo)
            {
                insert_track_number(videoTrackNumberRanges, videoTrackNumber++, &numVideoTrackNumberRanges);
            }
            else
            {
                for (c = 0; c < track->audio.channelCount; c++)
                {
                    insert_track_number(audioTrackNumberRanges, audioTrackNumber++, &numAudioTrackNumberRanges);
                }
            }
            track = track->next;
        }
    }

    /* create string representing each track range */
    remSize = sizeof(tracksString);
    tracksStringPtr = tracksString;
    for (i = 0; i < numVideoTrackNumberRanges; i++)
    {
        if (remSize < 4)
        {
            break;
        }

        strLen = get_range_string(tracksStringPtr, remSize, i == 0, 1, &videoTrackNumberRanges[i]);

        tracksStringPtr += strLen;
        remSize -= strLen;
    }
    if (numVideoTrackNumberRanges > 0 && numAudioTrackNumberRanges > 0)
    {
        mxf_snprintf(tracksStringPtr, remSize, " ");
        strLen = strlen(tracksStringPtr);
        tracksStringPtr += strLen;
        remSize -= strLen;
    }
    for (i = 0; i < numAudioTrackNumberRanges; i++)
    {
        if (remSize < 4)
        {
            break;
        }
        strLen = get_range_string(tracksStringPtr, remSize, i == 0, 0, &audioTrackNumberRanges[i]);

        tracksStringPtr += strLen;
        remSize -= strLen;
    }

    CHK_ORET((reader->clip.tracksString = strdup(tracksString)) != NULL);

    return 1;
}

static void get_timecode_info(int dropFrame, uint16_t roundedTCBase, TimecodeInfo *info)
{
    if (dropFrame && (roundedTCBase == 30 || roundedTCBase == 60)) {
        // first 2 frame numbers shall be omitted at the start of each minute,
        // except minutes 0, 10, 20, 30, 40 and 50
        info->dropCount = 2;
        if (roundedTCBase == 60)
            info->dropCount *= 2;
        info->framesPerMin = roundedTCBase * 60 - info->dropCount;
        info->framesPer10Min = info->framesPerMin * 10 + info->dropCount;
    } else {
        info->dropCount = 0;
        info->framesPerMin = roundedTCBase * 60;
        info->framesPer10Min = info->framesPerMin * 10;
    }
    info->framesPerHour = info->framesPer10Min * 6;
    info->nonDropFramesPerMin = roundedTCBase * 60;

    info->maxOffset = 24 * info->framesPerHour;
}

static mxfPosition timecode_to_offset(const MXFTimecode *timecode, uint16_t roundedTCBase)
{
    TimecodeInfo info;
    int64_t offset;

    get_timecode_info(timecode->isDropFrame, roundedTCBase, &info);

    offset = timecode->hour * info.framesPerHour;
    if (info.dropCount > 0) {
        offset += (timecode->min / 10) * info.framesPer10Min;
        offset += (timecode->min % 10) * info.framesPerMin;
    } else {
        offset += timecode->min * info.framesPerMin;
    }
    offset += timecode->sec * roundedTCBase;
    if (info.dropCount > 0 && timecode->sec == 0 && (timecode->min % 10) && timecode->frame < info.dropCount)
        offset += info.dropCount;
    else
        offset += timecode->frame;

    return offset;
}

static void offset_to_timecode(mxfPosition offset_in, uint16_t roundedTCBase, int dropFrame, MXFTimecode* timecode)
{
    int64_t offset = offset_in;
    int frames_dropped = 0;
    TimecodeInfo info;

    get_timecode_info(dropFrame, roundedTCBase, &info);

    offset %= info.maxOffset;
    if (offset < 0)
        offset += info.maxOffset;

    timecode->isDropFrame = (info.dropCount > 0);
    timecode->hour = (uint8_t)(offset / info.framesPerHour);
    offset = offset % info.framesPerHour;
    timecode->min = (uint8_t)(offset / info.framesPer10Min * 10);
    offset = offset % info.framesPer10Min;
    if (offset >= info.nonDropFramesPerMin) {
        offset -= info.nonDropFramesPerMin;
        timecode->min += (uint8_t)((offset / info.framesPerMin) + 1);
        offset = offset % info.framesPerMin;
        frames_dropped = timecode->isDropFrame;
    }
    timecode->sec = (uint8_t)(offset / roundedTCBase);
    timecode->frame = (uint8_t)(offset % roundedTCBase);

    if (frames_dropped) {
        timecode->frame += info.dropCount;
        if (timecode->frame >= roundedTCBase) {
            timecode->frame -= roundedTCBase;
            timecode->sec++;
            if (timecode->sec >= 60) {
                timecode->sec = 0;
                timecode->min++;
                if (timecode->min >= 60) {
                    timecode->min = 0;
                    timecode->hour++;
                    if (timecode->hour >= 24)
                        timecode->hour = 0;
                }
            }
        }
    }
}

static int convert_timecode_to_position(TimecodeIndex *index, MXFTimecode *timecode, mxfPosition *position)
{
    TimecodeSegment *segment;
    MXFListIterator iter;
    mxfPosition segmentStartPosition;
    int64_t frameCount;

    /* calculate the frame count */
    frameCount = timecode_to_offset(timecode, index->roundedTimecodeBase);

    /* find the segment that contains the given timecode and set the position */
    segmentStartPosition = 0;
    mxf_initialise_list_iter(&iter, &index->segments);
    while (mxf_next_list_iter_element(&iter))
    {
        segment = (TimecodeSegment*)mxf_get_iter_element(&iter);

        if (frameCount >= segment->startTimecode &&
            (segment->duration == -1 /* segment with unknown duration */
                || frameCount < segment->startTimecode + segment->duration))
        {
            *position = segmentStartPosition + (frameCount - segment->startTimecode);
            return 1;
        }

        segmentStartPosition += segment->duration;
    }

    return 0;
}

static int convert_position_to_timecode(TimecodeIndex *index, mxfPosition position, MXFTimecode *timecode)
{
    MXFListIterator iter;
    TimecodeSegment *segment;
    mxfPosition segmentStartPosition;
    int64_t frameCount = 0;
    int foundTimecodeSegment;

    CHK_ORET(position >= 0);

    /* drop frame compensation only supported for 30/60Hz */
    CHK_ORET(!index->isDropFrame || index->roundedTimecodeBase == 30 || index->roundedTimecodeBase == 60);


    /* find the timecode segment that the position is within */
    foundTimecodeSegment = 0;
    segmentStartPosition = 0;
    mxf_initialise_list_iter(&iter, &index->segments);
    while (mxf_next_list_iter_element(&iter))
    {
        segment = (TimecodeSegment*)mxf_get_iter_element(&iter);
        if (segment->duration < 0 || position < segmentStartPosition + segment->duration)
        {
            frameCount = segment->startTimecode + (position - segmentStartPosition);
            foundTimecodeSegment = 1;
            break;
        }
        segmentStartPosition += segment->duration;
    }
    CHK_ORET(foundTimecodeSegment);


    offset_to_timecode(frameCount, index->roundedTimecodeBase, index->isDropFrame, timecode);

    return 1;
}

static int read_timecode_component(MXFMetadataSet *timecodeComponentSet, TimecodeIndex *timecodeIndex)
{
    TimecodeSegment *newSegment = NULL;
    mxfBoolean dropFrame;
    uint16_t roundedTimecodeBase;

    CHK_MALLOC_ORET(newSegment, TimecodeSegment);
    memset(newSegment, 0, sizeof(TimecodeSegment));

    /* Note: we assume that RoundedTimecodeBase and DropFrame are fixed */
    CHK_OFAIL(mxf_get_uint16_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), &roundedTimecodeBase));
    timecodeIndex->roundedTimecodeBase = roundedTimecodeBase;
    CHK_OFAIL(mxf_get_boolean_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, DropFrame), &dropFrame));
    timecodeIndex->isDropFrame = dropFrame;
    CHK_OFAIL(mxf_get_length_item(timecodeComponentSet, &MXF_ITEM_K(StructuralComponent, Duration), &newSegment->duration));
    CHK_OFAIL(mxf_get_position_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), &newSegment->startTimecode));

    CHK_OFAIL(mxf_append_list_element(&timecodeIndex->segments, newSegment));
    newSegment = NULL; /* list now owns it */

    return 1;

fail:
    SAFE_FREE(newSegment);
    return 0;
}

static int create_timecode_index(TimecodeIndex **index)
{
    TimecodeIndex *newIndex = NULL;

    CHK_MALLOC_ORET(newIndex, TimecodeIndex);
    memset(newIndex, 0, sizeof(TimecodeIndex));
    mxf_initialise_list(&newIndex->segments, free);

    *index = newIndex;
    return 1;
}

static void free_timecode_index_in_list(void *data)
{
    TimecodeIndex *timecodeIndex;

    if (data == NULL)
    {
        return;
    }

    timecodeIndex = (TimecodeIndex*)data;
    mxf_clear_list(&timecodeIndex->segments);

    free(data);
}



int format_is_supported(MXFFile *mxfFile)
{
    MXFPartition *headerPartition = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    if (!mxf_read_header_pp_kl_with_runin(mxfFile, &key, &llen, &len) ||
        !mxf_read_partition(mxfFile, &key, len, &headerPartition))
    {
        return 0;
    }

    if (!opa_is_supported(headerPartition) && !op1a_is_supported(headerPartition))
    {
        mxf_free_partition(&headerPartition);
        return 0;
    }

    mxf_free_partition(&headerPartition);
    return 1;
}

int open_mxf_reader(const char *filename, MXFReader **reader)
{
    MXFDataModel *dataModel = NULL;

    CHK_OFAIL(mxf_load_data_model(&dataModel));
    CHK_OFAIL(mxf_finalise_data_model(dataModel));

    CHK_OFAIL(open_mxf_reader_2(filename, dataModel, reader));
    (*reader)->ownDataModel = 1; /* the reader will free it when closed */
    dataModel = NULL;

    return 1;

fail:
    if (dataModel != NULL)
    {
        mxf_free_data_model(&dataModel);
    }
    return 0;
}

int init_mxf_reader(MXFFile **mxfFile, MXFReader **reader)
{
    MXFDataModel *dataModel = NULL;

    CHK_OFAIL(mxf_load_data_model(&dataModel));
    CHK_OFAIL(mxf_finalise_data_model(dataModel));

    CHK_OFAIL(init_mxf_reader_2(mxfFile, dataModel, reader));
    (*reader)->ownDataModel = 1; /* the reader will free it when closed */
    dataModel = NULL;

    return 1;

fail:
    if (dataModel != NULL)
    {
        mxf_free_data_model(&dataModel);
    }
    return 0;
}

int open_mxf_reader_2(const char *filename, MXFDataModel *dataModel, MXFReader **reader)
{
    MXFFile *newMXFFile = NULL;

    if (!mxf_disk_file_open_read(filename, &newMXFFile))
    {
        mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        goto fail;
    }

    CHK_OFAIL(init_mxf_reader_2(&newMXFFile, dataModel, reader));

    return 1;

fail:
    mxf_file_close(&newMXFFile);
    return 0;
}

int init_mxf_reader_2(MXFFile **mxfFile, MXFDataModel *dataModel, MXFReader **reader)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFReader *newReader = NULL;
    MXFPartition *headerPartition = NULL;


    /* create the reader */

    CHK_MALLOC_ORET(newReader, MXFReader);
    memset(newReader, 0, sizeof(MXFReader));
    newReader->mxfFile = *mxfFile;
    memset(&newReader->clip, 0, sizeof(MXFClip));
    newReader->clip.duration = -1;
    newReader->clip.minDuration = -1;
    newReader->dataModel = dataModel;


    /* read header partition pack */

    if (!mxf_read_header_pp_kl(newReader->mxfFile, &key, &llen, &len))
    {
        mxf_log_error("Could not find header partition pack key" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        goto fail;
    }
    CHK_OFAIL(mxf_read_partition(newReader->mxfFile, &key, len, &headerPartition));


    /* create the essence reader */

    if (opa_is_supported(headerPartition))
    {
        CHK_MALLOC_OFAIL(newReader->essenceReader, EssenceReader);
        memset(newReader->essenceReader, 0, sizeof(EssenceReader));

        CHK_OFAIL(opa_initialise_reader(newReader, &headerPartition));
    }
    else if (op1a_is_supported(headerPartition))
    {
        CHK_MALLOC_OFAIL(newReader->essenceReader, EssenceReader);
        memset(newReader->essenceReader, 0, sizeof(EssenceReader));

        CHK_OFAIL(op1a_initialise_reader(newReader, &headerPartition));
    }
    else
    {
        /* if format_is_supported() succeeded then we shouldn't be here */
        mxf_log_error("MXF format not supported" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        goto fail;
    }


    CHK_OFAIL(create_tracks_string(newReader));


    *mxfFile = NULL; /* take ownership */
    *reader = newReader;
    return 1;

fail:
    mxf_free_partition(&headerPartition);
    if (newReader != NULL)
    {
        newReader->mxfFile = NULL; /* release ownership */
        close_mxf_reader(&newReader);
    }
    return 0;
}

void close_mxf_reader(MXFReader **reader)
{
    MXFTrack *track;
    MXFTrack *nextTrack;
    EssenceTrack *essenceTrack;
    EssenceTrack *nextEssenceTrack;

    if (*reader == NULL)
    {
        return;
    }

    /* close the MXF file */
    mxf_file_close(&(*reader)->mxfFile);

    /* free the essence reader */
    if ((*reader)->essenceReader != NULL)
    {
        (*reader)->essenceReader->close(*reader);
        essenceTrack = (*reader)->essenceReader->essenceTracks;
        while (essenceTrack != NULL)
        {
            nextEssenceTrack = essenceTrack->next;
            SAFE_FREE(essenceTrack);
            essenceTrack = nextEssenceTrack;
        }
        (*reader)->essenceReader->essenceTracks = NULL;
        SAFE_FREE((*reader)->essenceReader);
    }

    /* free the reader */
    track = (*reader)->clip.tracks;
    while (track != NULL)
    {
        nextTrack = track->next;
        SAFE_FREE(track);
        track = nextTrack;
    }
    (*reader)->clip.tracks = NULL;
    SAFE_FREE((*reader)->clip.tracksString);
    mxf_clear_list(&(*reader)->playoutTimecodeIndex.segments);
    mxf_clear_list(&(*reader)->sourceTimecodeIndexes);
    if ((*reader)->ownDataModel)
    {
        mxf_free_data_model(&(*reader)->dataModel);
    }
    SAFE_FREE((*reader)->buffer);
    SAFE_FREE((*reader)->archiveCRC32);

    SAFE_FREE(*reader);
}

int is_metadata_only(MXFReader *reader)
{
    return reader->isMetadataOnly;
}

MXFClip* get_mxf_clip(MXFReader *reader)
{
    return &reader->clip;
}

MXFTrack* get_mxf_track(MXFReader *reader, int trackIndex)
{
    MXFTrack *track = reader->clip.tracks;
    int count = 0;

    while (count < trackIndex && track != NULL)
    {
        track = track->next;
        count++;
    }

    return track;
}

void get_frame_rate(MXFReader *reader, mxfRational *frameRate)
{
    *frameRate = reader->clip.frameRate;
}

int64_t get_duration(MXFReader *reader)
{
    return reader->clip.duration;
}

int64_t get_min_duration(MXFReader *reader)
{
    return reader->clip.minDuration;
}

int get_num_tracks(MXFReader *reader)
{
    MXFTrack *track = reader->clip.tracks;
    int count = 0;

    while (track != NULL)
    {
        track = track->next;
        count++;
    }

    return count;
}

int clip_has_video(MXFReader *reader)
{
    return reader->clip.hasAssociatedVideo;
}

int set_frame_rate(MXFReader *reader, const mxfRational *frameRate)
{
    if (reader->isMetadataOnly)
    {
        return 0;
    }

    if (memcmp(frameRate, &reader->clip.frameRate, sizeof(*frameRate)) == 0)
    {
        return 1;
    }

    if (clip_has_video(reader))
    {
        /* we don't allow the frame rate to be changed if the clip has video */
        return 0;
    }

    /* note: this will also update the clip duration and frame rate */
    return reader->essenceReader->set_frame_rate(reader, frameRate);
}

const char* get_tracks_string(MXFReader *reader)
{
    return reader->clip.tracksString;
}

MXFHeaderMetadata* get_header_metadata(MXFReader *reader)
{
    return reader->essenceReader->get_header_metadata(reader);
}

int have_footer_metadata(MXFReader *reader)
{
    return reader->essenceReader->have_footer_metadata(reader);
}

int mxfr_is_seekable(MXFReader *reader)
{
    return mxf_file_is_seekable(reader->mxfFile);
}

int position_at_frame(MXFReader *reader, int64_t frameNumber)
{
    int result;
    int64_t i;
    int64_t skipFrameCount;

    if (reader->isMetadataOnly)
    {
        return 0;
    }

    if (frameNumber < 0 || (reader->clip.duration >= 0 && frameNumber > reader->clip.duration))
    {
        /* frame is outside essence boundaries */
        return 0;
    }

    if (frameNumber == (get_frame_number(reader) + 1))
    {
        return 1;
    }

    if (!mxf_file_is_seekable(reader->mxfFile))
    {
        if (frameNumber < (get_frame_number(reader) + 1))
        {
            /* can't skip backwards */
            return 0;
        }

        /* simulate seeking by skipping individual frames */
        result = 1;
        skipFrameCount = frameNumber - (get_frame_number(reader) + 1);
        for (i = 0; i < skipFrameCount; i++)
        {
            if ((result = skip_next_frame(reader)) != 1)
            {
                break;
            }
        }
    }
    else
    {
        if ((result = reader->essenceReader->position_at_frame(reader, frameNumber)))
        {
            /* update minDuration if frame is beyond the last known frame when the duration is unknown */
            if (reader->clip.duration < 0 && (get_frame_number(reader) + 1) > reader->clip.minDuration)
            {
                reader->clip.minDuration = (get_frame_number(reader) + 1);
            }
        }
    }

    return result;
}

int64_t get_last_written_frame_number(MXFReader *reader)
{
    if (reader->isMetadataOnly)
    {
        return -1;
    }

    if (reader->clip.duration >= 0 && get_frame_number(reader) >= reader->clip.duration)
    {
        /* we are at the last frame */
        return reader->clip.duration - 1;
    }

    if (!mxf_file_is_seekable(reader->mxfFile))
    {
        /* we'll always end up going past the end and not able to go back */
        return -1;
    }

    return reader->essenceReader->get_last_written_frame_number(reader);
}

int skip_next_frame(MXFReader *reader)
{
    int result;

    if (reader->isMetadataOnly)
    {
        return -1;
    }

    if (reader->clip.duration >= 0 && (get_frame_number(reader) + 1) >= reader->clip.duration)
    {
        /* end of essence reached */
        return -1;
    }

    if ((result = reader->essenceReader->skip_next_frame(reader)) == 1)
    {
        /* update minDuration if frame is beyond the last known frame when the duration is unknown */
        if (reader->clip.duration < 0 && (get_frame_number(reader) + 1) > reader->clip.minDuration)
        {
            reader->clip.minDuration = (get_frame_number(reader) + 1);
        }
    }

    return result;
}

int read_next_frame(MXFReader *reader, MXFReaderListener *listener)
{
    int result;

    if (reader->isMetadataOnly)
    {
        return -1;
    }

    if (reader->clip.duration >= 0 && (get_frame_number(reader) + 1) >= reader->clip.duration)
    {
        /* end of essence reached */
        return -1;
    }

    if ((result = reader->essenceReader->read_next_frame(reader, listener)) == 1)
    {
        /* update minDuration if frame is beyond the last known frame when the duration is unknown */
        if (reader->clip.duration < 0 && (get_frame_number(reader) + 1) > reader->clip.minDuration)
        {
            reader->clip.minDuration = (get_frame_number(reader) + 1);
        }
    }

    if (result == -1 && reader->clip.duration >= 0)
    {
        /* we know what the length should be so we assume that the file is still being written to
           and make the EOF a failure */
        result = 0;
    }

    if (result == 1)
    {
        /* flag that at least one frame has been read and therefore that source timecodes are present */
        reader->haveReadAFrame = 1;
    }
    return result;
}

int64_t get_frame_number(MXFReader *reader)
{
    if (reader->isMetadataOnly)
    {
        return -1;
    }

    return reader->essenceReader->get_next_frame_number(reader) - 1;
}

int get_playout_timecode(MXFReader *reader, MXFTimecode *timecode)
{
    CHK_ORET(convert_position_to_timecode(&reader->playoutTimecodeIndex, get_frame_number(reader),
        timecode));
    return 1;
}

int get_num_source_timecodes(MXFReader *reader)
{
    if (!reader->isMetadataOnly && !reader->haveReadAFrame)
    {
        if (mxf_file_is_seekable(reader->mxfFile))
        {
            /* read the first frame, which will extract the source timecodes as a side-effect */
            if (!read_next_frame(reader, NULL))
            {
                mxf_log_error("Failed to read first frame to update the number of source timecode" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            }
            if (!position_at_frame(reader, 0))
            {
                mxf_log_error("Failed to position reader back to frame 0" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            }
        }
        else
        {
            mxf_log_warn("Result of get_num_source_timecodes could be incorrect because "
                "MXF file is not seekable and first frame has not been read" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        }
    }
    return (int)mxf_get_list_length(&reader->sourceTimecodeIndexes);
}

int get_source_timecode_type(MXFReader *reader, int index)
{
    void *element = mxf_get_list_element(&reader->sourceTimecodeIndexes, (size_t)index);
    if (element == NULL)
    {
        return -1;
    }
    return ((TimecodeIndex*)element)->type;
}

int get_source_timecode(MXFReader *reader, int index, MXFTimecode *timecode, int *type, int *count)
{
    void *element;
    TimecodeIndex *timecodeIndex;
    int result;
    MXFClip *clip;
    mxfPosition playoutFrameNumber;
    mxfPosition sourceFrameNumber;

    CHK_ORET((element = mxf_get_list_element(&reader->sourceTimecodeIndexes, (size_t)index)) != NULL);
    timecodeIndex = (TimecodeIndex*)element;

    if (timecodeIndex->type == FILE_SOURCE_PACKAGE_TIMECODE ||
        timecodeIndex->type == AVID_FILE_SOURCE_PACKAGE_TIMECODE)
    {
        /* take into account frame rate differences between material package track and source package track */
        clip = get_mxf_clip(reader);
        playoutFrameNumber = get_frame_number(reader);
        sourceFrameNumber = (mxfPosition)(playoutFrameNumber * timecodeIndex->roundedTimecodeBase /
            (float)((uint16_t)(clip->frameRate.numerator / (float)clip->frameRate.denominator + 0.5)) + 0.5);

        CHK_ORET(convert_position_to_timecode(timecodeIndex, sourceFrameNumber, timecode));
        result = 1;
    }
    else
    {
        CHK_ORET(timecodeIndex->type == SYSTEM_ITEM_TC_ARRAY_TIMECODE ||
            timecodeIndex->type == SYSTEM_ITEM_SDTI_CREATION_TIMECODE ||
            timecodeIndex->type == SYSTEM_ITEM_SDTI_USER_TIMECODE);
        if (timecodeIndex->position == get_frame_number(reader))
        {
            timecode->isDropFrame = timecodeIndex->isDropFrame;
            timecode->hour = timecodeIndex->hour;
            timecode->min = timecodeIndex->min;
            timecode->sec = timecodeIndex->sec;
            timecode->frame = timecodeIndex->frame;
            result = 1;
        }
        else
        {
            timecode->isDropFrame = 0;
            timecode->hour = 0;
            timecode->min = 0;
            timecode->sec = 0;
            timecode->frame = 0;
            result = -1;
        }
    }

    *type = timecodeIndex->type;
    *count = timecodeIndex->count;

    return result;
}

int get_num_archive_crc32(MXFReader *reader)
{
    if (!reader->isMetadataOnly && !reader->haveReadAFrame)
    {
        if (mxf_file_is_seekable(reader->mxfFile))
        {
            /* read the first frame, which will extract the crc32's as a side-effect */
            if (!read_next_frame(reader, NULL))
            {
                mxf_log_error("Failed to read first frame to update the number of archive crc32" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            }
            if (!position_at_frame(reader, 0))
            {
                mxf_log_error("Failed to position reader back to frame 0" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            }
        }
        else
        {
            mxf_log_warn("Result of get_num_archive_crc32 could be incorrect because "
                "MXF file is not seekable and first frame has not been read" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        }
    }

    return reader->numArchiveCRC32;
}

int get_archive_crc32(MXFReader *reader, int index, uint32_t *crc32)
{
    CHK_ORET(index >= 0 && index < (int)reader->numArchiveCRC32);

    *crc32 = reader->archiveCRC32[index];
    return 1;
}

int position_at_playout_timecode(MXFReader *reader, MXFTimecode *timecode)
{
    int64_t position;

    CHK_ORET(convert_timecode_to_position(&reader->playoutTimecodeIndex, timecode, &position));

    CHK_ORET(position_at_frame(reader, position));

    return 1;
}

int position_at_source_timecode(MXFReader *reader, MXFTimecode *timecode, int type, int count)
{
    CHK_ORET(type == FILE_SOURCE_PACKAGE_TIMECODE ||
        type == AVID_FILE_SOURCE_PACKAGE_TIMECODE ||
        type == SYSTEM_ITEM_TC_ARRAY_TIMECODE ||
        type == SYSTEM_ITEM_SDTI_CREATION_TIMECODE ||
        type == SYSTEM_ITEM_SDTI_USER_TIMECODE);

    if (type == FILE_SOURCE_PACKAGE_TIMECODE || type == AVID_FILE_SOURCE_PACKAGE_TIMECODE)
    {
        int64_t position;
        TimecodeIndex *timecodeIndex = NULL;
        MXFClip *clip;
        mxfPosition playoutFrameNumber;
        MXFListIterator iter;

        mxf_initialise_list_iter(&iter, &reader->sourceTimecodeIndexes);
        while (mxf_next_list_iter_element(&iter))
        {
            timecodeIndex = (TimecodeIndex*)mxf_get_iter_element(&iter);

            if (timecodeIndex->type == type)
            {
                break;
            }
            timecodeIndex = NULL;
        }
        if (timecodeIndex == NULL)
        {
            mxf_log_error("MXF file does not have specified source timecode" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }

        if (timecodeIndex->isDropFrame != timecode->isDropFrame)
        {
            mxf_log_error("Timecode drop frame flag mismatch for specified source timecode" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }


        CHK_ORET(convert_timecode_to_position(timecodeIndex, timecode, &position));

        /* take into account frame rate differences between material package track and source package track */
        clip = get_mxf_clip(reader);
        playoutFrameNumber = (mxfPosition)(position *
            (float)((uint16_t)(clip->frameRate.numerator / (float)clip->frameRate.denominator + 0.5)) /
            timecodeIndex->roundedTimecodeBase + 0.5);

        CHK_ORET(position_at_frame(reader, playoutFrameNumber));

        return 1;
    }
    else
    {
        TimecodeIndex *timecodeIndex = NULL;
        MXFListIterator iter;
        int64_t originalFrameNumber;

        /* store original frame number for restoration later if failed to find frame with timecode */
        originalFrameNumber = get_frame_number(reader);

        /* read first frame to get list of available timecodes */
        CHK_ORET(position_at_frame(reader, 0));
        CHK_ORET(read_next_frame(reader, NULL) > 0);

        /* get the timecode index */
        mxf_initialise_list_iter(&iter, &reader->sourceTimecodeIndexes);
        while (mxf_next_list_iter_element(&iter))
        {
            timecodeIndex = (TimecodeIndex*)mxf_get_iter_element(&iter);

            if (timecodeIndex->type == type && timecodeIndex->count == count)
            {
                break;
            }
            timecodeIndex = NULL;
        }
        if (timecodeIndex == NULL)
        {
            mxf_log_error("MXF file does not have specified source timecode" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }

        if (timecodeIndex->isDropFrame != timecode->isDropFrame)
        {
            mxf_log_error("Timecode drop frame flag mismatch for specified source timecode" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }

        /* linear search starting from frame 0 */
        CHK_ORET(position_at_frame(reader, 0));
        while (read_next_frame(reader, NULL) > 0)
        {
            if (timecodeIndex->isDropFrame == timecode->isDropFrame &&
                timecodeIndex->hour == timecode->hour &&
                timecodeIndex->min == timecode->min &&
                timecodeIndex->sec == timecode->sec &&
                timecodeIndex->frame == timecode->frame)
            {
                /* move back to start of previous read frame which had the timecode */
                CHK_ORET(position_at_frame(reader, get_frame_number(reader)));
                return 1;
            }
        }

        mxf_log_error("Could not find frame with specified source timecode" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
        /* restore to original position */
        CHK_ORET(position_at_frame(reader, originalFrameNumber));
        return 0;
    }

    return 0;
}


int add_track(MXFReader *reader, MXFTrack **track)
{
    MXFTrack *newTrack = NULL;
    MXFTrack *lastTrack = reader->clip.tracks;

    CHK_MALLOC_ORET(newTrack, MXFTrack);
    memset(newTrack, 0, sizeof(MXFTrack));

    if (reader->clip.tracks == NULL)
    {
        reader->clip.tracks = newTrack;
    }
    else
    {
        while (lastTrack->next != NULL)
        {
            lastTrack = lastTrack->next;
        }
        lastTrack->next = newTrack;
    }

    *track = newTrack;
    return 1;
}

int add_essence_track(EssenceReader *essenceReader, EssenceTrack **essenceTrack)
{
    EssenceTrack *newTrack = NULL;
    EssenceTrack *lastTrack = essenceReader->essenceTracks;

    CHK_MALLOC_ORET(newTrack, EssenceTrack);
    memset(newTrack, 0, sizeof(EssenceTrack));

    if (essenceReader->essenceTracks == NULL)
    {
        essenceReader->essenceTracks = newTrack;
    }
    else
    {
        while (lastTrack->next != NULL)
        {
            lastTrack = lastTrack->next;
        }
        lastTrack->next = newTrack;
    }

    *essenceTrack = newTrack;
    return 1;
}

int get_num_essence_tracks(EssenceReader *essenceReader)
{
    EssenceTrack *essenceTrack = essenceReader->essenceTracks;
    int count = 0;

    while (essenceTrack != NULL)
    {
        essenceTrack = essenceTrack->next;
        count++;
    }

    return count;
}

EssenceTrack* get_essence_track(EssenceReader *essenceReader, int trackIndex)
{
    EssenceTrack *essenceTrack = essenceReader->essenceTracks;
    int count = 0;

    while (count < trackIndex && essenceTrack != NULL)
    {
        essenceTrack = essenceTrack->next;
        count++;
    }

    return essenceTrack;
}

int get_essence_track_with_tracknumber(EssenceReader *essenceReader, uint32_t trackNumber,
    EssenceTrack **essenceTrack, int *trackIndex)
{
    EssenceTrack *eTrack = essenceReader->essenceTracks;
    int tIndex = 0;

    while (eTrack != NULL && eTrack->trackNumber != trackNumber)
    {
        eTrack = eTrack->next;
        tIndex++;
    }

    if (eTrack == NULL)
    {
        return 0;
    }

    *essenceTrack = eTrack;
    *trackIndex = tIndex;
    return 1;
}

/* Fix inaccurate Avid NTSC rates */
void clean_rate(mxfRational *rate)
{
    if (rate->numerator == 2997 && rate->denominator == 100)
    {
        rate->numerator = 30000;
        rate->denominator = 1001;
    }
}

int initialise_playout_timecode(MXFReader *reader, MXFMetadataSet *materialPackageSet)
{
    TimecodeIndex *timecodeIndex = &reader->playoutTimecodeIndex;
    MXFMetadataSet *trackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *tcSet;
    MXFArrayItemIterator iter1;
    mxfUL dataDef;
    int haveTimecodeTrack;
    uint32_t sequenceComponentCount;
    MXFArrayItemIterator iter2;
    uint8_t *arrayElementValue;
    uint32_t arrayElementLength;

    mxf_initialise_list(&timecodeIndex->segments, free);
    timecodeIndex->isDropFrame = 0;
    timecodeIndex->roundedTimecodeBase = 0;


    /* get the TimecodeComponent in the timecode track
       There should only be 1 TimecodeComponent directly referenced by a timecode Track */
    haveTimecodeTrack = 0;
    CHK_OFAIL(mxf_uu_get_package_tracks(materialPackageSet, &iter1));
    while (mxf_uu_next_track(materialPackageSet->headerMetadata, &iter1, &trackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(trackSet, &dataDef));
        if (mxf_is_timecode(&dataDef))
        {
            if (haveTimecodeTrack)
            {
                mxf_log_warn("Multiple playout timecode tracks present in Material Package - use first encountered" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                break;
            }

            CHK_OFAIL(mxf_get_strongref_item(trackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(sequenceSet->headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(TimecodeComponent)))
            {
                /* extract the single TimecodeComponent */
                CHK_OFAIL(read_timecode_component(sequenceSet, timecodeIndex));
                haveTimecodeTrack = 1;
                /* don't break so that we check and warn if there are multiple timecode tracks */
            }
            else
            {
                /* MaterialPackage TimecodeComponent should not be contained in a Sequence, but we allow it, and
                   only a single TimecodeComponent should be present, but we will allow multiple */

                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &sequenceComponentCount));
                if (sequenceComponentCount != 1)
                {
                    mxf_log_warn("Material Package playout timecode has multiple components" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                }

                /* extract the sequence of TimecodeComponents */
                mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &iter2);
                while (mxf_next_array_item_element(&iter2, &arrayElementValue, &arrayElementLength))
                {
                    CHK_OFAIL(mxf_get_strongref(sequenceSet->headerMetadata, arrayElementValue, &tcSet));
                    CHK_OFAIL(read_timecode_component(tcSet, timecodeIndex));
                    haveTimecodeTrack = 1;
                }
                /* don't break so that we check and warn if there are multiple timecode tracks */
            }
        }
    }

    if (!haveTimecodeTrack)
    {
        return 0;
    }

    timecodeIndex->type = PLAYOUT_TIMECODE;

    return 1;

fail:
    mxf_clear_list(&timecodeIndex->segments);
    return 0;
}

int initialise_default_playout_timecode(MXFReader *reader)
{
    MXFClip *clip;
    TimecodeIndex *timecodeIndex = &reader->playoutTimecodeIndex;
    TimecodeSegment *newSegment = NULL;

    clip = get_mxf_clip(reader);

    mxf_initialise_list(&timecodeIndex->segments, free);
    timecodeIndex->isDropFrame = 0;
    timecodeIndex->roundedTimecodeBase = (uint16_t)(clip->frameRate.numerator / (float)clip->frameRate.denominator + 0.5);

    CHK_MALLOC_ORET(newSegment, TimecodeSegment);
    memset(newSegment, 0, sizeof(TimecodeSegment));
    newSegment->startTimecode = 0;
    newSegment->duration = clip->duration;

    CHK_OFAIL(mxf_append_list_element(&timecodeIndex->segments, newSegment));

    return 1;

fail:
    SAFE_FREE(newSegment);
    return 0;
}


int initialise_source_timecodes(MXFReader *reader, MXFMetadataSet *sourcePackageSet)
{
    MXFList *sourceTimecodeIndexes = &reader->sourceTimecodeIndexes;
    MXFMetadataSet *trackSet;
    MXFMetadataSet *sequenceSet;
    MXFMetadataSet *tcSet;
    MXFArrayItemIterator iter1;
    mxfUL dataDef;
    MXFArrayItemIterator iter2;
    uint8_t *arrayElementValue;
    uint32_t arrayElementLength;
    TimecodeIndex *timecodeIndex = NULL;
    TimecodeIndex *timecodeIndexRef = NULL;
    int count = 0;
    uint32_t componentCount;
    MXFMetadataSet *structuralComponentSet;
    mxfUMID sourcePackageID;
    MXFMetadataSet *sourceClipSet;
    MXFArrayItemIterator iter3;
    MXFMetadataSet *timecodeComponentSet;
    MXFMetadataSet *refSourcePackageSet;
    mxfPosition fromStartPosition;
    mxfPosition toStartPosition;
    mxfRational fromEditRate;
    mxfRational toEditRate;
    mxfPosition startTimecode;
    MXFTimecode timecode;
    uint16_t roundedTimecodeBase;
    TimecodeSegment *segment;
    int continueAvidTimecodeSearch;
    uint32_t i;

    mxf_initialise_list(sourceTimecodeIndexes, free_timecode_index_in_list);


    /* get the timecode tracks */
    CHK_OFAIL(mxf_uu_get_package_tracks(sourcePackageSet, &iter1));
    while (mxf_uu_next_track(sourcePackageSet->headerMetadata, &iter1, &trackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(trackSet, &dataDef));
        if (mxf_is_timecode(&dataDef))
        {
            CHK_OFAIL(mxf_get_strongref_item(trackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_is_subclass_of(sequenceSet->headerMetadata->dataModel, &sequenceSet->key, &MXF_SET_K(TimecodeComponent)))
            {
                /* extract the single TimecodeComponent */
                CHK_OFAIL(create_timecode_index(&timecodeIndex));
                CHK_OFAIL(mxf_append_list_element(sourceTimecodeIndexes, timecodeIndex));
                timecodeIndexRef = timecodeIndex;
                timecodeIndex = NULL; /* list now owns it */
                CHK_OFAIL(read_timecode_component(sequenceSet, timecodeIndexRef));
            }
            else
            {
                /* extract the sequence of TimecodeComponents */
                CHK_OFAIL(create_timecode_index(&timecodeIndex));
                CHK_OFAIL(mxf_append_list_element(sourceTimecodeIndexes, timecodeIndex));
                timecodeIndexRef = timecodeIndex;
                timecodeIndex = NULL; /* list now owns it */
                mxf_initialise_array_item_iterator(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &iter2);
                while (mxf_next_array_item_element(&iter2, &arrayElementValue, &arrayElementLength))
                {
                    CHK_OFAIL(mxf_get_strongref(sequenceSet->headerMetadata, arrayElementValue, &tcSet));
                    CHK_OFAIL(read_timecode_component(tcSet, timecodeIndexRef));
                }
            }
            timecodeIndexRef->type = FILE_SOURCE_PACKAGE_TIMECODE;
            timecodeIndexRef->count = count++;
        }
    }

    if (count > 0)
    {
        /* if we have timecode tracks in the file source package then we don't look
        for Avid style source timecodes */
        return 1;
    }

    /* get the Avid source timecode */
    /* the Avid source timecode is present when a SourceClip in the file SourcePackage references
       a tape SourcePackage. The SourceClip::StartPosition defines the start timecode
       in conjunction with the TimecodeComponent in the tape SourcePackage */
    continueAvidTimecodeSearch = 1;
    CHK_OFAIL(mxf_uu_get_package_tracks(sourcePackageSet, &iter1));
    while (continueAvidTimecodeSearch && mxf_uu_next_track(sourcePackageSet->headerMetadata, &iter1, &trackSet))
    {
        CHK_OFAIL(mxf_uu_get_track_datadef(trackSet, &dataDef));
        if (!mxf_is_picture(&dataDef) && !mxf_is_sound(&dataDef))
        {
            continue;
        }

        CHK_OFAIL(mxf_get_strongref_item(trackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));

        /* get the source clip */
        if (mxf_set_is_subclass_of(sequenceSet, &MXF_SET_K(Sequence)))
        {
            /* is a sequence, so we get the first component */

            CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &componentCount));
            if (componentCount == 0)
            {
                /* empty sequence */
                continue;
            }

            /* get first source clip component */
            for (i = 0; i < componentCount; i++)
            {
                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), i, &arrayElementValue));
                if (!mxf_get_strongref(sequenceSet->headerMetadata, arrayElementValue, &structuralComponentSet))
                {
                    /* probably a Filler if it hasn't been registered in the dictionary */
                    continue;
                }
                if (mxf_set_is_subclass_of(structuralComponentSet, &MXF_SET_K(SourceClip)))
                {
                    break;
                }
            }
        }
        else
        {
            /* something other than a sequence */
            structuralComponentSet = sequenceSet;
        }
        if (!mxf_set_is_subclass_of(structuralComponentSet, &MXF_SET_K(SourceClip)))
        {
            /* not a source clip, so we don't bother looking any further */
            continue;
        }
        sourceClipSet = structuralComponentSet;

        /* get the start position and edit rate for the 'from' source clip */
        CHK_OFAIL(mxf_get_rational_item(trackSet, &MXF_ITEM_K(Track, EditRate), &fromEditRate));
        CHK_OFAIL(mxf_get_position_item(sourceClipSet, &MXF_ITEM_K(SourceClip, StartPosition), &fromStartPosition));


        /* get the package referenced by the source clip */
        CHK_OFAIL(mxf_get_umid_item(sourceClipSet, &MXF_ITEM_K(SourceClip, SourcePackageID), &sourcePackageID));
        if (mxf_equals_umid(&g_Null_UMID, &sourcePackageID) ||
            !mxf_uu_get_referenced_package(sourceClipSet->headerMetadata, &sourcePackageID, &refSourcePackageSet))
        {
            /* either end of chain or don't have referenced package */
            continueAvidTimecodeSearch = 0;
            break;
        }

        /* find the timecode component */
        CHK_OFAIL(mxf_uu_get_package_tracks(refSourcePackageSet, &iter3));
        while (mxf_uu_next_track(refSourcePackageSet->headerMetadata, &iter3, &trackSet))
        {
            CHK_OFAIL(mxf_uu_get_track_datadef(trackSet, &dataDef));
            if (!mxf_is_timecode(&dataDef))
            {
                continue;
            }

            CHK_OFAIL(mxf_get_strongref_item(trackSet, &MXF_ITEM_K(GenericTrack, Sequence), &sequenceSet));
            if (mxf_set_is_subclass_of(sequenceSet, &MXF_SET_K(Sequence)))
            {
                /* is a sequence, so we get the first component */

                CHK_OFAIL(mxf_get_array_item_count(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), &componentCount));
                if (componentCount != 1)
                {
                    /* empty sequence or > 1 are not what we expect, so we don't bother looking further */
                    continueAvidTimecodeSearch = 0;
                    continue;
                }

                /* get first component */

                CHK_OFAIL(mxf_get_array_item_element(sequenceSet, &MXF_ITEM_K(Sequence, StructuralComponents), 0, &arrayElementValue));
                CHK_OFAIL(mxf_get_strongref(sequenceSet->headerMetadata, arrayElementValue, &structuralComponentSet));
            }
            else
            {
            /* something other than a sequence */
                structuralComponentSet = sequenceSet;
            }
            if (!mxf_set_is_subclass_of(structuralComponentSet, &MXF_SET_K(TimecodeComponent)))
            {
                /* not a timecode component, so we don't bother looking any further */
                continueAvidTimecodeSearch = 0;
                break;
            }
            timecodeComponentSet = structuralComponentSet;


            /* get the start timecode, rounded timecode base and edit rate for the 'to' timecode component */
            CHK_OFAIL(mxf_get_rational_item(trackSet, &MXF_ITEM_K(Track, EditRate), &toEditRate));
            CHK_OFAIL(mxf_get_position_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, StartTimecode), &startTimecode));
            CHK_OFAIL(mxf_get_uint16_item(timecodeComponentSet, &MXF_ITEM_K(TimecodeComponent, RoundedTimecodeBase), &roundedTimecodeBase));

            /* create an timecode index */
            CHK_OFAIL(create_timecode_index(&timecodeIndex));
            CHK_OFAIL(mxf_append_list_element(sourceTimecodeIndexes, timecodeIndex));
            timecodeIndexRef = timecodeIndex;
            timecodeIndex = NULL; /* list now owns it */
            CHK_OFAIL(read_timecode_component(timecodeComponentSet, timecodeIndexRef));
            timecodeIndexRef->type = AVID_FILE_SOURCE_PACKAGE_TIMECODE;
            timecodeIndexRef->count = 1;

            /* convert start position to referenced source start position */
            toStartPosition = (int64_t)(fromStartPosition * toEditRate.numerator * fromEditRate.denominator /
                (float)(toEditRate.denominator * fromEditRate.numerator) + 0.5);

            /* modify the timecode index with the file source package's start timecode */
            CHK_OFAIL(convert_position_to_timecode(timecodeIndexRef, toStartPosition, &timecode));
            segment = (TimecodeSegment*)mxf_get_first_list_element(&timecodeIndexRef->segments);
            segment->startTimecode = timecode_to_offset(&timecode, roundedTimecodeBase);

            continueAvidTimecodeSearch = 0;
            break;
        }
    }

    return 1;

fail:
    SAFE_FREE(timecodeIndex);
    mxf_clear_list(sourceTimecodeIndexes);
    return 0;
}

int set_essence_container_timecode(MXFReader *reader, mxfPosition position,
    int type, int count, int isDropFrame, uint8_t hour, uint8_t min, uint8_t sec, uint8_t frame)
{
    MXFList *sourceTimecodeIndexes = &reader->sourceTimecodeIndexes;
    MXFListIterator iter;
    TimecodeIndex *timecodeIndex = NULL;
    TimecodeIndex *timecodeIndexRef = NULL;
    int foundIt;

    foundIt = 0;
    mxf_initialise_list_iter(&iter, sourceTimecodeIndexes);
    while (mxf_next_list_iter_element(&iter))
    {
        timecodeIndexRef = (TimecodeIndex*)mxf_get_iter_element(&iter);
        if (timecodeIndexRef->type == type && timecodeIndexRef->count == count)
        {
            /* set existing timecode index to new value */
            timecodeIndexRef->isDropFrame = isDropFrame;
            timecodeIndexRef->position = position;
            timecodeIndexRef->hour = hour;
            timecodeIndexRef->min = min;
            timecodeIndexRef->sec = sec;
            timecodeIndexRef->frame = frame;
            foundIt = 1;
            break;
        }
    }

    if (!foundIt)
    {
        /* create new timecode index */
        CHK_ORET(create_timecode_index(&timecodeIndex));
        CHK_OFAIL(mxf_append_list_element(sourceTimecodeIndexes, timecodeIndex));
        timecodeIndexRef = timecodeIndex;
        timecodeIndex = NULL; /* list now owns it */

        timecodeIndexRef->isDropFrame = isDropFrame;
        timecodeIndexRef->position = position;
        timecodeIndexRef->type = type;
        timecodeIndexRef->count = count;
        timecodeIndexRef->hour = hour;
        timecodeIndexRef->min = min;
        timecodeIndexRef->sec = sec;
        timecodeIndexRef->frame = frame;
    }

    return 1;

fail:
    SAFE_FREE(timecodeIndex);
    return 0;
}

int allocate_archive_crc32(MXFReader *reader, uint32_t num)
{
    if (reader->numArchiveCRC32Alloc < num)
    {
        SAFE_FREE(reader->archiveCRC32);
        CHK_MALLOC_ARRAY_ORET(reader->archiveCRC32, uint32_t, num);
        reader->numArchiveCRC32Alloc = num;
    }

    reader->numArchiveCRC32 = num;
    memset(reader->archiveCRC32, 0, sizeof(uint32_t) * num);

    return 1;
}

int set_archive_crc32(MXFReader *reader, uint32_t index, uint32_t crc32)
{
    CHK_ORET(index < reader->numArchiveCRC32);

    reader->archiveCRC32[index] = crc32;

    return 1;
}

int64_t mxfr_convert_length(const mxfRational *frameRateIn, int64_t lengthIn, const mxfRational *frameRateOut)
{
    double factor;
    int64_t lengthOut;

    if (lengthIn < 0 || memcmp(frameRateIn, frameRateOut, sizeof(*frameRateIn)) == 0 ||
        frameRateIn->numerator < 1 || frameRateIn->denominator < 1 || frameRateOut->numerator < 1 || frameRateOut->denominator < 1)
    {
        return lengthIn;
    }

    factor = frameRateOut->numerator * frameRateIn->denominator / (double)(frameRateOut->denominator * frameRateIn->numerator);
    lengthOut = (int64_t)(lengthIn * factor + 0.5);

    if (lengthOut < 0)
    {
        /* e.g. if lengthIn was the max value and the new rate is faster */
        return lengthIn;
    }

    return lengthOut;
}

int get_clip_duration(MXFHeaderMetadata *headerMetadata, MXFClip *clip, int isOPAtom)
{
    MXFMetadataSet *materialPackageSet;
    MXFMetadataSet *materialPackageTrackSet;
    MXFArrayItemIterator arrayIter;
    mxfUL dataDefUL;
    int64_t duration;
    mxfRational editRate;
    int64_t videoDuration;
    int haveVideoTrack = 0;


    if (!mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(MaterialPackage), &materialPackageSet))
    {
        return 0;
    }

    CHK_ORET(mxf_uu_get_package_tracks(materialPackageSet, &arrayIter));
    while (mxf_uu_next_track(headerMetadata, &arrayIter, &materialPackageTrackSet))
    {
        /* CHK_OFAIL(mxf_uu_get_track_datadef(materialPackageTrackSet, &dataDefUL)); */
        /* NOTE: not failing because files from Omneon were found to have a missing DataDefinition item
           in the Sequence and DMSourceClip referenced by a static DM Track */
        if (!mxf_uu_get_track_datadef(materialPackageTrackSet, &dataDefUL))
        {
            continue;
        }

        if (isOPAtom &&
            !mxf_is_picture(&dataDefUL) && !mxf_is_sound(&dataDefUL) && !mxf_is_timecode(&dataDefUL))
        {
            /* some Avid OPAtom files have a weak reference to a DataDefinition instead of a UL */
            mxf_avid_get_data_def(headerMetadata, (mxfUUID*)&dataDefUL, &dataDefUL);
        }

        if (mxf_is_picture(&dataDefUL) || mxf_is_sound(&dataDefUL))
        {
            CHK_ORET(mxf_get_rational_item(materialPackageTrackSet, &MXF_ITEM_K(Track, EditRate), &editRate));
            CHK_ORET(mxf_uu_get_track_duration(materialPackageTrackSet, &duration));

            /* the clip duration is the duration of the shortest track in video track edit units */
            if (mxf_is_picture(&dataDefUL))
            {
                if (haveVideoTrack)
                {
                    videoDuration = mxfr_convert_length(&editRate, duration, &clip->frameRate);
                    if (videoDuration < clip->duration)
                    {
                        clip->duration = videoDuration;
                    }
                }
                else
                {
                    if (clip->duration > 0)
                    {
                        videoDuration = mxfr_convert_length(&clip->frameRate, clip->duration, &editRate);
                        if (duration < videoDuration)
                        {
                            clip->duration = duration;
                        }
                        else
                        {
                            clip->duration = videoDuration;
                        }
                        clip->frameRate = editRate;
                    }
                    else
                    {
                        clip->duration = duration;
                        clip->frameRate = editRate;
                    }
                }

                haveVideoTrack = 1;
            }
            else /* mxf_is_sound(&dataDefUL) */
            {
                if (haveVideoTrack)
                {
                    videoDuration = mxfr_convert_length(&editRate, duration, &clip->frameRate);
                    if (videoDuration < clip->duration)
                    {
                        clip->duration = videoDuration;
                    }
                }
                else
                {
                    /* assume PAL frame rate */
                    videoDuration = mxfr_convert_length(&editRate, duration, &g_palFrameRate);
                    if (clip->duration > 0)
                    {
                        if (videoDuration < clip->duration)
                        {
                            clip->frameRate = g_palFrameRate;
                            clip->duration = videoDuration;
                        }
                    }
                    else
                    {
                        clip->frameRate = g_palFrameRate;
                        clip->duration = videoDuration;
                    }
                }
            }
        }
    }

    clip->hasAssociatedVideo = haveVideoTrack;

    return 1;
}

int mxfr_is_pal_frame_rate(const mxfRational *frameRate)
{
    return memcmp(frameRate, &g_palFrameRate, sizeof(*frameRate)) == 0;
}

int mxfr_is_ntsc_frame_rate(const mxfRational *frameRate)
{
    return memcmp(frameRate, &g_ntscFrameRate, sizeof(*frameRate)) == 0;
}

int mxfr_is_prof_sampling_rate(const mxfRational *samplingRate)
{
    return memcmp(samplingRate, &g_profAudioSamplingRate, sizeof(*samplingRate)) == 0;
}

