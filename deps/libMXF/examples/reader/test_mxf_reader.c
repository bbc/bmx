/*
 * Test the MXF Reader
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
#include <assert.h>

#include "mxf_reader.h"
#include <mxf/mxf_macros.h>


#define DO_TEST1 1
//#define DO_TEST2 1
//#define DO_TEST3 1

#define INVALID_TIMECODE_HOUR       99


struct MXFReaderListenerData
{
    FILE *outFile;

    MXFReader *input;

    uint8_t *buffer;
    uint32_t bufferSize;
};

static void print_timecode(MXFTimecode *timecode)
{
    char *sep;

    if (timecode->isDropFrame)
    {
        sep = ";";
    }
    else
    {
        sep = ":";
    }

    printf("%02u%s%02u%s%02u%s%02u", timecode->hour, sep, timecode->min, sep, timecode->sec, sep, timecode->frame);
}

static int accept_frame(MXFReaderListener *listener, int trackIndex)
{
    (void)listener;
    (void)trackIndex;
    return 1;
}

static int allocate_buffer(MXFReaderListener *listener, int trackIndex, uint8_t **buffer, uint32_t bufferSize)
{
    MXFTrack *track;

    if (listener->data->bufferSize >= bufferSize)
    {
        *buffer = listener->data->buffer;
        return 1;
    }

    track = get_mxf_track(listener->data->input, trackIndex);
    if (track == NULL)
    {
        fprintf(stderr, "Received allocate buffer from unknown track %d\n", trackIndex);
        return 0;
    }

    if (listener->data->buffer != NULL)
    {
        free(listener->data->buffer);
    }
    listener->data->buffer = (uint8_t*)malloc(bufferSize);
    if (buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate buffer\n");
        return 0;
    }
    listener->data->bufferSize = bufferSize;

    *buffer = listener->data->buffer;
    return 1;
}

static void deallocate_buffer(MXFReaderListener *listener, int trackIndex, uint8_t **buffer)
{
    MXFTrack *track;

    assert(*buffer == listener->data->buffer);

    track = get_mxf_track(listener->data->input, trackIndex);
    if (track == NULL)
    {
        fprintf(stderr, "Received deallocate buffer from unknown track %d\n", trackIndex);
    }

    if (listener->data->buffer != NULL)
    {
        free(listener->data->buffer);
        listener->data->buffer = NULL;
        *buffer = NULL;
    }
}

static int receive_frame(MXFReaderListener *listener, int trackIndex, uint8_t *buffer, uint32_t bufferSize)
{
    MXFTrack *track;

    assert(trackIndex != 9999); /* avoid compiler warning */

    track = get_mxf_track(listener->data->input, trackIndex);
    if (track == NULL)
    {
        fprintf(stderr, "Received frame from unknown track %d\n", trackIndex);
        return 0;
    }
    printf("received frame from track index %d with size %d\n", trackIndex, bufferSize);

    if (track->isVideo)
    {
        if (fwrite(buffer, 1, bufferSize, listener->data->outFile) != bufferSize)
        {
            fprintf(stderr, "Failed to write frame data\n");
            return 0;
        }
    }


    return 1;
}

#if defined(DO_TEST1)

static int test1(const char *mxfFilename, MXFTimecode *startTimecode, int sourceTimecodeCount, const char *outFilename)
{
    MXFReader *input;
    MXFClip *clip;
    int64_t frameCount;
    int64_t frameNumber;
    MXFReaderListenerData data;
    MXFReaderListener listener;
    MXFFile *stdinMXFFile = NULL;
    MXFTimecode playoutTimecode;
    int i;
    MXFTimecode sourceTimecode;
    int type;
    int count;
    int result;
    uint32_t archiveCRC32;

    memset(&data, 0, sizeof(MXFReaderListenerData));
    listener.data = &data;
    listener.accept_frame = accept_frame;
    listener.allocate_buffer = allocate_buffer;
    listener.deallocate_buffer = deallocate_buffer;
    listener.receive_frame = receive_frame;

    if (strcmp("-", mxfFilename) != 0)
    {
        if (!open_mxf_reader(mxfFilename, &input))
        {
            fprintf(stderr, "Failed to open MXF reader\n");
            return 0;
        }
    }
    else
    {
        if (!mxf_stdin_wrap_read(&stdinMXFFile))
        {
            fprintf(stderr, "Failed to open standard input MXF file\n");
            return 0;
        }
        if (!init_mxf_reader(&stdinMXFFile, &input))
        {
            mxf_file_close(&stdinMXFFile);
            fprintf(stderr, "Failed to open MXF reader using standard input\n");
            return 0;
        }
    }
    listener.data->input = input;

    if ((data.outFile = fopen(outFilename, "wb")) == NULL)
    {
        fprintf(stderr, "Failed to open output data file\n");
        return 0;
    }

    clip = get_mxf_clip(input);
    printf("Clip tracks = %s\n", clip->tracksString);
    printf("Clip frame rate = %d/%d fps\n", clip->frameRate.numerator, clip->frameRate.denominator);
    printf("Clip duration = %" PRId64 " frames\n", clip->duration);

    if (startTimecode->hour != INVALID_TIMECODE_HOUR)
    {
        if (sourceTimecodeCount < 0)
        {
            if (!position_at_playout_timecode(input, startTimecode))
            {
                fprintf(stderr, "Failed to position files at start playout timecode\n");
                return 0;
            }
        }
        else
        {
            if (!position_at_source_timecode(input, startTimecode, SYSTEM_ITEM_TC_ARRAY_TIMECODE,
                sourceTimecodeCount))
            {
                fprintf(stderr, "Failed to position files at start source timecode\n");
                return 0;
            }
        }
    }

    frameCount = 0;
    while (read_next_frame(input, &listener) == 1)
    {
        frameNumber = get_frame_number(input);
        printf("frame =  %" PRId64 "\n", frameNumber);
        get_playout_timecode(input, &playoutTimecode);
        printf("playout timecode = ");
        print_timecode(&playoutTimecode);
        printf("\n");
        for (i = 0; i < get_num_source_timecodes(input); i++)
        {
            if ((result = get_source_timecode(input, i, &sourceTimecode, &type, &count)) == 1)
            {
                printf("source timecode (type = %d, count = %d) = ", type, count);
                print_timecode(&sourceTimecode);
                printf("\n");
            }
            else if (result == -1)
            {
                printf("source timecode (type = %d, count = %d) unavailable", type, count);
                printf("\n");
            }
        }
        for (i = 0; i < get_num_archive_crc32(input); i++)
        {
            if (get_archive_crc32(input, i, &archiveCRC32))
            {
                printf("crc32 %d = 0x%08x\n", i, archiveCRC32);
            }
            else
            {
                printf("Failed to get archive crc-32\n");
            }
        }
        frameCount++;
    }
    if (clip->duration != -1 && frameCount != clip->duration)
    {
        fprintf(stderr, "1) Frame count %" PRId64 " != duration %" PRId64 "\n", frameCount, clip->duration);
        return 0;
    }

    close_mxf_reader(&input);
    fclose(data.outFile);
    if (data.buffer != NULL)
    {
        free(data.buffer);
    }

    return 1;
}

#endif


#if defined(DO_TEST2)

static int test2(const char *mxfFilename, const char *outFilename)
{
    MXFReader *input;
    MXFClip *clip;
    int64_t frameCount;
    int64_t frameNumber;
    MXFReaderListenerData data;
    MXFReaderListener listener;
    MXFFile *stdinMXFFile = NULL;
    MXFTimecode playoutTimecode;
    int i;
    MXFTimecode sourceTimecode;
    int type;
    int count;
    int result;

    memset(&data, 0, sizeof(MXFReaderListenerData));
    listener.data = &data;
    listener.accept_frame = accept_frame;
    listener.allocate_buffer = allocate_buffer;
    listener.deallocate_buffer = deallocate_buffer;
    listener.receive_frame = receive_frame;

    if (strcmp("-", mxfFilename) != 0)
    {
        if (!open_mxf_reader(mxfFilename, &input))
        {
            fprintf(stderr, "Failed to open MXF reader\n");
            return 0;
        }
    }
    else
    {
        if (!mxf_stdin_wrap_read(&stdinMXFFile))
        {
            fprintf(stderr, "Failed to open standard input MXF file\n");
            return 0;
        }
        if (!init_mxf_reader(&stdinMXFFile, &input))
        {
            mxf_file_close(&stdinMXFFile);
            fprintf(stderr, "Failed to open MXF reader using standard input\n");
            return 0;
        }
    }
    listener.data->input = input;

    if ((data.outFile = fopen(outFilename, "wb")) == NULL)
    {
        fprintf(stderr, "Failed to open output data file\n");
        return 0;
    }

    clip = get_mxf_clip(input);

    frameCount = 0;
    while (read_next_frame(input, &listener) == 1)
    {
        frameNumber = get_frame_number(input);
        printf("frame =  %" PRId64 "\n", frameNumber);
        get_playout_timecode(input, &playoutTimecode);
        printf("playout timecode = ");
        print_timecode(&playoutTimecode);
        printf("\n");
        for (i = 0; i < get_num_source_timecodes(input); i++)
        {
            if ((result = get_source_timecode(input, i, &sourceTimecode, &type, &count)) == 1)
            {
                printf("source timecode (type = %d, count = %d) = ", type, count);
                print_timecode(&sourceTimecode);
                printf("\n");
            }
            else if (result == -1)
            {
                printf("source timecode (type = %d, count = %d) unavailable", type, count);
                printf("\n");
            }
        }
        frameCount++;
        if (frameCount == 10)
        {
            printf("Positioning at frame %" PRId64 " + 5\n", frameCount);
            if (!position_at_frame(input, frameCount + 5))
            {
                fprintf(stderr, "Failed to position file at %" PRId64 " + 5\n", frameCount);
                break;
            }
        }
    }
    if (clip->duration != -1 && frameCount != clip->duration - 5)
    {
        fprintf(stderr, "x) Frame count %" PRId64 " != duration %" PRId64 "\n", frameCount, clip->duration - 5);
        return 0;
    }

    close_mxf_reader(&input);
    fclose(data.outFile);
    if (data.buffer != NULL)
    {
        free(data.buffer);
    }

    return 1;
}

#endif


#if defined(DO_TEST3)

static int test3(const char *mxfFilename, const char *outFilename)
{
    MXFReader *input;
    int64_t frameCount;
    MXFClip *clip;
    MXFReaderListenerData data;
    MXFReaderListener listener;
    int64_t frameNumber;
    MXFFile *stdinMXFFile = NULL;
    MXFTimecode playoutTimecode;
    int i;
    MXFTimecode sourceTimecode;
    int type;
    int count;
    int result;

    memset(&data, 0, sizeof(MXFReaderListenerData));
    listener.data = &data;
    listener.accept_frame = accept_frame;
    listener.allocate_buffer = allocate_buffer;
    listener.deallocate_buffer = deallocate_buffer;
    listener.receive_frame = receive_frame;


    if (strcmp("-", mxfFilename) != 0)
    {
        if (!open_mxf_reader(mxfFilename, &input))
        {
            fprintf(stderr, "Failed to open MXF reader\n");
            return 0;
        }
    }
    else
    {
        if (!mxf_stdin_wrap_read(&stdinMXFFile))
        {
            fprintf(stderr, "Failed to open standard input MXF file\n");
            return 0;
        }
        if (!init_mxf_reader(&stdinMXFFile, &input))
        {
            mxf_file_close(&stdinMXFFile);
            fprintf(stderr, "Failed to open MXF reader using standard input\n");
            return 0;
        }
    }
    listener.data->input = input;

    if ((data.outFile = fopen(outFilename, "wb")) == NULL)
    {
        fprintf(stderr, "Failed to open output data file\n");
        return 0;
    }

    clip = get_mxf_clip(input);

    if (!position_at_frame(input, 10))
    {
        fprintf(stderr, "Failed to position at frame 10\n");
        return 0;
    }
    frameCount = 10;
    while (read_next_frame(input, &listener) == 1)
    {
        frameNumber = get_frame_number(input);
        printf("frame =  %" PRId64 "\n", frameNumber);
        get_playout_timecode(input, &playoutTimecode);
        printf("playout timecode = ");
        print_timecode(&playoutTimecode);
        printf("\n");
        for (i = 0; i < get_num_source_timecodes(input); i++)
        {
            if ((result = get_source_timecode(input, i, &sourceTimecode, &type, &count)) == 1)
            {
                printf("source timecode (type = %d, count = %d) = ", type, count);
                print_timecode(&sourceTimecode);
                printf("\n");
            }
            else if (result == -1)
            {
                printf("source timecode (type = %d, count = %d) unavailable", type, count);
                printf("\n");
            }
        }
        frameCount++;
    }
    if (clip->duration != -1 && frameCount != clip->duration)
    {
        fprintf(stderr, "2) Frame count %" PRId64 " != duration %" PRId64 "\n", frameCount, clip->duration);
        return 0;
    }

    if (!position_at_frame(input, 0))
    {
        fprintf(stderr, "Failed to position at frame 0\n");
        return 0;
    }
    frameCount = 0;
    while (read_next_frame(input, &listener) == 1)
    {
        frameNumber = get_frame_number(input);
        printf("frame =  %" PRId64 "\n", frameNumber);
        get_playout_timecode(input, &playoutTimecode);
        printf("playout timecode = ");
        print_timecode(&playoutTimecode);
        printf("\n");
        for (i = 0; i < get_num_source_timecodes(input); i++)
        {
            if ((result = get_source_timecode(input, i, &sourceTimecode, &type, &count)) == 1)
            {
                printf("source timecode (type = %d, count = %d) = ", type, count);
                print_timecode(&sourceTimecode);
                printf("\n");
            }
            else if (result == -1)
            {
                printf("source timecode (type = %d, count = %d) unavailable", type, count);
                printf("\n");
            }
        }
        frameCount++;
    }

    if (clip->duration != -1 && frameCount != clip->duration)
    {
        fprintf(stderr, "3) Frame count %" PRId64 " != duration %" PRId64 "\n", frameCount, clip->duration);
        return 0;
    }

    if (!position_at_frame(input, 5))
    {
        fprintf(stderr, "Failed to position at frame 5\n");
        return 0;
    }
    frameCount = 0;
    while (read_next_frame(input, &listener) == 1)
    {
        frameNumber = get_frame_number(input);
        printf("frame =  %" PRId64 "\n", frameNumber);
        get_playout_timecode(input, &playoutTimecode);
        printf("playout timecode = ");
        print_timecode(&playoutTimecode);
        printf("\n");
        for (i = 0; i < get_num_source_timecodes(input); i++)
        {
            if ((result = get_source_timecode(input, i, &sourceTimecode, &type, &count)) == 1)
            {
                printf("source timecode (type = %d, count = %d) = ", type, count);
                print_timecode(&sourceTimecode);
                printf("\n");
            }
            else if (result == -1)
            {
                printf("source timecode (type = %d, count = %d) unavailable", type, count);
                printf("\n");
            }
        }
        frameCount++;
    }

    if (clip->duration != -1 && frameCount + 5 != clip->duration)
    {
        fprintf(stderr, "4) Frame count %" PRId64 " != duration %" PRId64 "\n", frameCount + 5, clip->duration);
        return 0;
    }


    close_mxf_reader(&input);
    fclose(data.outFile);
    if (data.buffer != NULL)
    {
        free(data.buffer);
    }

    return 1;
}

#endif


static void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s [-sp startTimecode (-sc sourceTimecodeCount)] (<mxf filename> | -) <output filename>\n", cmd);
}


int main(int argc, const char *argv[])
{
    const char *mxfFilename;
    const char *outFilename;
    int cmdlIndex;
    MXFTimecode startTimecode;
    int sourceTimecodeCount = -1;

    startTimecode.hour = INVALID_TIMECODE_HOUR;

    cmdlIndex = 1;
    while (cmdlIndex < argc - 2)
    {
        if (!strcmp(argv[cmdlIndex], "-s"))
        {
            int value[4];
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -s argument\n");
                return 1;
            }
            if (sscanf(argv[cmdlIndex + 1], "%u:%u:%u:%u", &value[0], &value[1], &value[2], &value[3]) < 4)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid start timecode\n");
                return 1;
            }
            startTimecode.isDropFrame = 0;
            startTimecode.hour = value[0];
            startTimecode.min = value[1];
            startTimecode.sec = value[2];
            startTimecode.frame = value[3];
            cmdlIndex += 2;
        }
        else if (!strcmp(argv[cmdlIndex], "-sc"))
        {
            if (cmdlIndex >= argc-1)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing -sc argument\n");
                return 1;
            }
            if (sscanf(argv[cmdlIndex + 1], "%d", &sourceTimecodeCount) < 1)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid source timecode count\n");
                return 1;
            }
            cmdlIndex += 2;
        }
    }

    if (argc - cmdlIndex != 2)
    {
        usage(argv[0]);
        return 1;
    }
    mxfFilename = argv[cmdlIndex];
    cmdlIndex++;
    outFilename = argv[cmdlIndex];

#if defined(DO_TEST1)
    printf("TEST 1\n");
    if (!test1(mxfFilename, &startTimecode, sourceTimecodeCount, outFilename))
    {
        return 1;
    }
#endif

#if defined(DO_TEST2)
    printf("\nTEST 2\n");
    if (!test2(mxfFilename, outFilename))
    {
        return 1;
    }
#endif

#if defined(DO_TEST3)
    if (strcmp("-", mxfFilename) != 0)
    {
        printf("\nTEST 3\n");
        if (!test3(mxfFilename, outFilename))
        {
            return 1;
        }
    }
#endif

    return 0;
}

