/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mxf/mxf.h>
#include <mxf/mxf_stream_file.h>
#include <mxf/mxf_macros.h>



struct MXFFileSysData
{
    MXFFile *target;
    MXFFile *mxfFile;
    int readOnly;
    int64_t headPosition;   /* current write/read position */
    int64_t position;       /* next write/read position */
};


static int read_skip(MXFFileSysData *sysData)
{
    int64_t newHeadPosition = sysData->position;

    sysData->position = sysData->headPosition;
    if (!mxf_skip(sysData->mxfFile, newHeadPosition - sysData->headPosition)) {
        sysData->position = newHeadPosition;
        return 0;
    }

    return 1;
}

static int write_skip(MXFFileSysData *sysData)
{
    int64_t newHeadPosition = sysData->position;

    sysData->position = sysData->headPosition;
    if (!mxf_write_zeros(sysData->mxfFile, newHeadPosition - sysData->headPosition)) {
        sysData->position = newHeadPosition;
        return 0;
    }

    return 1;
}


static void stream_file_close(MXFFileSysData *sysData)
{
    if (sysData->target)
        mxf_file_close(&sysData->target);
}

static uint32_t stream_file_read(MXFFileSysData *sysData, uint8_t *data, uint32_t count)
{
    uint32_t result;

    if (!sysData->readOnly)
        return 0;

    if (sysData->position > sysData->headPosition && !read_skip(sysData))
        return 0;

    result = mxf_file_read(sysData->target, data, count);
    sysData->headPosition += result;
    sysData->position = sysData->headPosition;

    return result;
}

static uint32_t stream_file_write(MXFFileSysData *sysData, const uint8_t *data, uint32_t count)
{
    uint32_t result;

    if (sysData->readOnly)
        return 0;

    if (sysData->position > sysData->headPosition && !write_skip(sysData))
        return 0;

    result = mxf_file_write(sysData->target, data, count);
    sysData->headPosition += result;
    sysData->position = sysData->headPosition;

    return result;
}

static int stream_file_getchar(MXFFileSysData *sysData)
{
    int result;

    if (!sysData->readOnly)
        return EOF;

    if (sysData->position > sysData->headPosition && !read_skip(sysData))
        return EOF;

    result = mxf_file_getc(sysData->target);
    if (result != EOF) {
        sysData->headPosition++;
        sysData->position = sysData->headPosition;
    }

    return result;
}

static int stream_file_putchar(MXFFileSysData *sysData, int c)
{
    int result;

    if (sysData->readOnly)
        return EOF;

    if (sysData->position > sysData->headPosition && !write_skip(sysData))
        return EOF;

    result = mxf_file_putc(sysData->target, c);
    if (result != EOF) {
        sysData->headPosition++;
        sysData->position = sysData->headPosition;
    }

    return result;
}

static int stream_file_eof(MXFFileSysData *sysData)
{
    return mxf_file_eof(sysData->target);
}

static int stream_file_seek(MXFFileSysData *sysData, int64_t offset, int whence)
{
    int64_t newPosition;

    if (whence == SEEK_CUR)
        newPosition = sysData->position + offset;
    else if (whence == SEEK_SET)
        newPosition = offset;
    else
        return 0;

    if (newPosition < sysData->headPosition)
        return 0;

    sysData->position = newPosition;

    return 1;
}

static int64_t stream_file_tell(MXFFileSysData *sysData)
{
    return sysData->position;
}

static int stream_file_is_seekable(MXFFileSysData *sysData)
{
    (void)sysData;
    return 0;
}

static int64_t stream_file_size(MXFFileSysData *sysData)
{
    (void)sysData;
    return 0;
}

static void free_stream_file(MXFFileSysData *sysData)
{
    free(sysData);
}


int mxf_stream_file_wrap(MXFFile *target, int readOnly, MXFFile **streamMXFFile)
{
    MXFFile *newMXFFile = NULL;
    MXFFileSysData *newStreamFile = NULL;

    CHK_MALLOC_ORET(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(MXFFile));
    CHK_MALLOC_OFAIL(newStreamFile, MXFFileSysData);
    memset(newStreamFile, 0, sizeof(MXFFileSysData));

    newStreamFile->target   = target;
    newStreamFile->mxfFile  = newMXFFile;
    newStreamFile->readOnly = readOnly;

    newMXFFile->close         = stream_file_close;
    newMXFFile->read          = stream_file_read;
    newMXFFile->write         = stream_file_write;
    newMXFFile->get_char      = stream_file_getchar;
    newMXFFile->put_char      = stream_file_putchar;
    newMXFFile->eof           = stream_file_eof;
    newMXFFile->seek          = stream_file_seek;
    newMXFFile->tell          = stream_file_tell;
    newMXFFile->is_seekable   = stream_file_is_seekable;
    newMXFFile->size          = stream_file_size;
    newMXFFile->free_sys_data = free_stream_file;
    newMXFFile->sysData       = newStreamFile;
    newMXFFile->minLLen       = target->minLLen;
    newMXFFile->runinLen      = target->runinLen;


    *streamMXFFile = newStreamFile->mxfFile;
    return 1;

fail:
    SAFE_FREE(newMXFFile);
    SAFE_FREE(newStreamFile);
    return 0;
}

