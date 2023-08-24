/*
 * Copyright (C) 2012, British Broadcasting Corporation
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
#include <stdio.h>
#include <string.h>

#include <mxf/mxf.h>
#include <mxf/mxf_rw_intl_file.h>
#include <mxf/mxf_cache_file.h>
#include <mxf/mxf_macros.h>



struct MXFRWInterleaver
{
    uint32_t blockSize;
    uint32_t writerCacheSize;

    int64_t readCount;

    MXFCacheFile **writers;
    size_t numAllocWriters;
    size_t numWriters;
    size_t lastWriterIndexFlush;
};

struct MXFFileSysData
{
    MXFRWInterleaver *interleaver;
    MXFFile *target;
};



static int add_cached_writer(MXFRWInterleaver *interleaver, MXFFile *writer, MXFFile **cachedWriter)
{
    MXFCacheFile **newWriters;
    MXFCacheFile *newCacheFile;

    if (interleaver->numWriters >= interleaver->numAllocWriters) {
        newWriters = (MXFCacheFile**)realloc(interleaver->writers,
                                             (interleaver->numAllocWriters + 32) * sizeof(*interleaver->writers));
        if (!newWriters) {
            mxf_log_error("Failed to reallocate interleaver file writers array" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
        interleaver->writers          = newWriters;
        interleaver->numAllocWriters += 32;
    }

    CHK_ORET(mxf_cache_file_open(writer, 0, interleaver->writerCacheSize, &newCacheFile));

    interleaver->writers[interleaver->numWriters] = newCacheFile;
    interleaver->numWriters++;

    *cachedWriter = mxf_cache_file_get_file(newCacheFile);
    return 1;
}

static int have_writer_data(MXFRWInterleaver *interleaver)
{
    size_t i;
    for (i = 0; i < interleaver->numWriters; i++) {
        if (mxf_cache_file_get_dirty_count(interleaver->writers[i]) > 0)
            return 1;
    }

    return 0;
}

static int flush_writer_data(MXFRWInterleaver *interleaver)
{
    uint32_t flushRem = interleaver->blockSize;
    uint32_t numFlush;
    size_t writerIndex;
    size_t i;

    writerIndex = (interleaver->lastWriterIndexFlush + 1) % interleaver->numWriters;
    for (i = 0; i < interleaver->numWriters; i++) {
        if (mxf_cache_file_get_dirty_count(interleaver->writers[writerIndex]) == 0)
            break;

        numFlush = mxf_cache_file_flush(interleaver->writers[writerIndex], flushRem);
        if (numFlush == 0)
            return 0;

        interleaver->lastWriterIndexFlush = writerIndex;
        if (flushRem <= numFlush) /* note that more than flushRem could have been flushed */
            break;
        flushRem -= numFlush;

        writerIndex = (writerIndex + 1) % interleaver->numWriters;
    }

    return 1;
}


static void intl_file_close(MXFFileSysData *sysData)
{
    if (sysData->target)
        mxf_file_close(&sysData->target);
}

static uint32_t intl_file_read(MXFFileSysData *sysData, uint8_t *data, uint32_t count)
{
    uint32_t remCount = count;
    uint32_t numRead, numActualRead;
    int flushWriter;
    uint32_t flushCount = (uint32_t)(((sysData->interleaver->readCount % sysData->interleaver->blockSize) + count) /
                                            sysData->interleaver->blockSize);

    while (remCount > 0) {
        if (flushCount > 0) {
            if (have_writer_data(sysData->interleaver)) {
                if (remCount >= sysData->interleaver->blockSize)
                    numRead = sysData->interleaver->blockSize;
                else
                    numRead = remCount;
                flushWriter = 1;
            } else {
                numRead = remCount;
                flushWriter = 0;
                flushCount = 0;
            }
        } else {
            numRead = remCount;
            flushWriter = 0;
        }

        numActualRead = mxf_file_read(sysData->target, &data[count - remCount], numRead);
        remCount -= numActualRead;
        sysData->interleaver->readCount += numActualRead;
        if (sysData->interleaver->readCount < 0)
            sysData->interleaver->readCount = 0; /* reset after int64 overflow */

        if (flushWriter) {
            if (!flush_writer_data(sysData->interleaver)) {
                mxf_log_warn("R/W interleaver read failed because writer cache data flush failed\n");
                break;
            }
            flushCount--;
        }

        if (numActualRead != numRead)
            break;
    }

    return count - remCount;
}

static uint32_t intl_file_write(MXFFileSysData *sysData, const uint8_t *data, uint32_t count)
{
    return mxf_file_write(sysData->target, data, count);
}

static int intl_file_getchar(MXFFileSysData *sysData)
{
    uint8_t data;
    if (intl_file_read(sysData, &data, 1) != 1)
        return EOF;

    return data;
}

static int intl_file_putchar(MXFFileSysData *sysData, int c)
{
    return mxf_file_putc(sysData->target, c);
}

static int intl_file_eof(MXFFileSysData *sysData)
{
    return mxf_file_eof(sysData->target);
}

static int intl_file_seek(MXFFileSysData *sysData, int64_t offset, int whence)
{
    return mxf_file_seek(sysData->target, offset, whence);
}

static int64_t intl_file_tell(MXFFileSysData *sysData)
{
    return mxf_file_tell(sysData->target);
}

static int intl_file_is_seekable(MXFFileSysData *sysData)
{
    return mxf_file_is_seekable(sysData->target);
}

static int64_t intl_file_size(MXFFileSysData *sysData)
{
    return mxf_file_size(sysData->target);
}

static void free_intl_file(MXFFileSysData *sysData)
{
    free(sysData);
}



int mxf_create_rw_intl(uint32_t interleaveBlockSize, uint32_t writerCacheSize, MXFRWInterleaver **interleaver)
{
    MXFRWInterleaver *newInterleaver = NULL;

    CHK_MALLOC_ORET(newInterleaver, MXFRWInterleaver);
    memset(newInterleaver, 0, sizeof(*newInterleaver));

    newInterleaver->blockSize       = interleaveBlockSize;
    newInterleaver->writerCacheSize = writerCacheSize;

    *interleaver = newInterleaver;
    return 1;
}

void mxf_free_rw_intl(MXFRWInterleaver **interleaver)
{
    if (!(*interleaver))
        return;

    free((*interleaver)->writers);
    SAFE_FREE(*interleaver);
}

int mxf_rw_intl_open(MXFRWInterleaver *interleaver, MXFFile *target, int isWriter, MXFFile **mxfFile)
{
    MXFFile *newMXFFile = NULL;
    MXFFileSysData *newIntlFile = NULL;
    MXFFile *cachedTarget;

    CHK_MALLOC_ORET(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(MXFFile));
    CHK_MALLOC_OFAIL(newIntlFile, MXFFileSysData);
    memset(newIntlFile, 0, sizeof(MXFFileSysData));

    if (isWriter) {
        CHK_OFAIL(add_cached_writer(interleaver, target, &cachedTarget));
        newIntlFile->target  = cachedTarget;
    } else {
        newIntlFile->target  = target;
    }
    newIntlFile->interleaver = interleaver;

    newMXFFile->close         = intl_file_close;
    newMXFFile->read          = intl_file_read;
    newMXFFile->write         = intl_file_write;
    newMXFFile->get_char      = intl_file_getchar;
    newMXFFile->put_char      = intl_file_putchar;
    newMXFFile->eof           = intl_file_eof;
    newMXFFile->seek          = intl_file_seek;
    newMXFFile->tell          = intl_file_tell;
    newMXFFile->is_seekable   = intl_file_is_seekable;
    newMXFFile->size          = intl_file_size;
    newMXFFile->free_sys_data = free_intl_file;
    newMXFFile->sysData       = newIntlFile;
    newMXFFile->minLLen       = target->minLLen;
    newMXFFile->runinLen      = target->runinLen;

    *mxfFile = newMXFFile;
    return 1;

fail:
    SAFE_FREE(newMXFFile);
    SAFE_FREE(newIntlFile);
    return 0;
}

