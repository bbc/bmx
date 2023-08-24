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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mxf/mxf.h>
#include <mxf/mxf_cache_file.h>
#include <mxf/mxf_macros.h>



typedef struct
{
    int64_t position;
    uint32_t size;
    int isDirty;
} Page;

struct MXFCacheFile
{
    MXFFile *mxfFile;
};

struct MXFFileSysData
{
    MXFCacheFile cacheFile;
    MXFFile *target;
    int64_t position;
    unsigned char *allocCacheData;
    unsigned char *cacheData;
    uint32_t cacheSize;
    Page *pages;
    uint32_t numPages;
    uint32_t pageSize;
    uint32_t firstDirtyPage;
    uint32_t dirtyCount;
    int eof;
    int writingAtEOF;
};



static void get_current_page_info(MXFFileSysData *sysData, int64_t *pagePosition, uint32_t *pageIndex,
                                  uint32_t *pageOffset)
{
    int64_t pageNumber;

    assert(sysData->position >= 0);

    pageNumber = sysData->position / sysData->pageSize;

    *pagePosition = pageNumber * sysData->pageSize;
    *pageIndex    = (uint32_t)(pageNumber % sysData->numPages);
    *pageOffset   = (uint32_t)(sysData->position - (*pagePosition));
}

static int flush_dirty_pages(MXFFileSysData *sysData, uint32_t pageIndex, uint32_t pagesRequired)
{
    uint32_t remPages = pagesRequired;
    uint32_t cleanIndex = pageIndex;
    uint32_t numCleanPages;
    uint32_t numWrite;
    int doWrite;
    uint32_t i;

    if (sysData->dirtyCount == 0 || pagesRequired == 0)
        return 1;

    while (sysData->dirtyCount > 0 && remPages > 0) {
        if (sysData->pages[cleanIndex].isDirty) {
            doWrite = 1;
            if (cleanIndex >= sysData->firstDirtyPage) {
                assert(cleanIndex - sysData->firstDirtyPage < sysData->dirtyCount);
                numCleanPages = sysData->dirtyCount - (cleanIndex - sysData->firstDirtyPage);
            } else {
                assert((sysData->firstDirtyPage + sysData->dirtyCount) % sysData->numPages > cleanIndex);
                numCleanPages = ((sysData->firstDirtyPage + sysData->dirtyCount) % sysData->numPages) - cleanIndex;
                assert(numCleanPages <= sysData->dirtyCount);
            }
            /* need to flush to the end to ensure dirty pages are contiguous */
            if (cleanIndex != sysData->firstDirtyPage && remPages < numCleanPages)
                remPages = numCleanPages;

            if (numCleanPages > sysData->numPages - cleanIndex)
                numCleanPages = sysData->numPages - cleanIndex;
        } else {
            doWrite = 0;
            if (cleanIndex >= sysData->firstDirtyPage)
                numCleanPages = sysData->numPages - cleanIndex;
            else
                numCleanPages = sysData->firstDirtyPage - cleanIndex;
        }
        if (numCleanPages > remPages)
            numCleanPages = remPages;

        if (doWrite) {
            numWrite = (numCleanPages - 1) * sysData->pageSize +
                        sysData->pages[cleanIndex + numCleanPages - 1].size;

            CHK_ORET(mxf_file_seek(sysData->target, sysData->pages[cleanIndex].position, SEEK_SET));
            CHK_ORET(mxf_file_write(sysData->target, &sysData->cacheData[cleanIndex * sysData->pageSize], numWrite) ==
                        numWrite);

            for (i = 0 ; i < numCleanPages; i++)
                sysData->pages[cleanIndex + i].isDirty = 0;
            sysData->dirtyCount -= numCleanPages;

            if (cleanIndex == sysData->firstDirtyPage)
                sysData->firstDirtyPage = (sysData->firstDirtyPage + numCleanPages) % sysData->numPages;
        }

        cleanIndex = (cleanIndex + numCleanPages) % sysData->numPages;
        remPages -= numCleanPages;
    }

    return 1;
}

static void cache_file_close(MXFFileSysData *sysData)
{
    if (sysData->target) {
        flush_dirty_pages(sysData, 0, sysData->numPages);
        mxf_file_close(&sysData->target);
    }

    SAFE_FREE(sysData->pages);
    SAFE_FREE(sysData->allocCacheData);
}

static uint32_t cache_file_read(MXFFileSysData *sysData, uint8_t *data, uint32_t count)
{
    int64_t pagePosition;
    uint32_t pageIndex;
    uint32_t pageOffset;
    uint32_t numRead;
    uint32_t numPagesRead;
    uint32_t remCount = count;
    uint32_t i;

    while (remCount > 0) {
        get_current_page_info(sysData, &pagePosition, &pageIndex, &pageOffset);

        if (sysData->pages[pageIndex].position == pagePosition) {
            if (pageOffset >= sysData->pages[pageIndex].size)
                break; /* positioned beyond end of file. Note that growing files are not yet supported */

            numRead = sysData->pages[pageIndex].size - pageOffset;
            if (numRead > remCount)
                numRead = remCount;
            if (data)
                memcpy(&data[count - remCount], &sysData->cacheData[pageIndex * sysData->pageSize + pageOffset], numRead);
            remCount -= numRead;
            sysData->position += numRead;
        } else {
            /* read as many pages as possible in one read call */
            /* note: we assume that the data to be read is not already in the cache */
            numPagesRead = (remCount + sysData->pageSize - 1) / sysData->pageSize;
            if (numPagesRead > sysData->numPages - pageIndex)
                numPagesRead = sysData->numPages - pageIndex;

            CHK_ORET(flush_dirty_pages(sysData, pageIndex, numPagesRead));

            CHK_ORET(mxf_file_seek(sysData->target, pagePosition, SEEK_SET));
            numRead = mxf_file_read(sysData->target, &sysData->cacheData[pageIndex * sysData->pageSize],
                                    numPagesRead * sysData->pageSize);

            if (numRead == 0)
                break;

            numPagesRead = numRead / sysData->pageSize;
            for (i = 0; i < numPagesRead; i++) {
                sysData->pages[pageIndex + i].size     = sysData->pageSize;
                sysData->pages[pageIndex + i].position = pagePosition + i * sysData->pageSize;
            }
            if (numRead != numPagesRead * sysData->pageSize) {
                sysData->pages[pageIndex + i].size     = numRead - numPagesRead * sysData->pageSize;
                sysData->pages[pageIndex + i].position = pagePosition + i * sysData->pageSize;
            }
        }
    }

    sysData->eof = (count > 0 && remCount > 0);

    return count - remCount;
}

static uint32_t cache_file_write(MXFFileSysData *sysData, const uint8_t *data, uint32_t count)
{
    int64_t pagePosition;
    uint32_t pageIndex;
    uint32_t pageOffset;
    uint32_t numWrite;
    uint32_t numPagesWrite;
    uint32_t remCount = count;
    uint32_t numRead;
    int64_t originalPosition;
    int originalEOF;
    uint32_t i;

    while (remCount > 0) {
        get_current_page_info(sysData, &pagePosition, &pageIndex, &pageOffset);

        if (sysData->pages[pageIndex].position == -1) {
            sysData->pages[pageIndex].size     = 0;
            sysData->pages[pageIndex].position = pagePosition;
        }

        if (sysData->pages[pageIndex].position == pagePosition) {
            if (!sysData->pages[pageIndex].isDirty) {
                if (sysData->dirtyCount > 0) {
                    /* sequence of dirty pages must be contiguous - flush all dirty pages if there is a gap */
                    uint32_t prevPageIndex = (pageIndex == 0 ? sysData->numPages - 1 : pageIndex - 1);
                    if (sysData->pages[prevPageIndex].position != pagePosition - sysData->pageSize ||
                        !sysData->pages[prevPageIndex].isDirty)
                    {
                        CHK_ORET(flush_dirty_pages(sysData, 0, sysData->numPages));
                    }
                }
                sysData->pages[pageIndex].isDirty = 1;
                if (sysData->dirtyCount == 0)
                    sysData->firstDirtyPage = pageIndex;
                sysData->dirtyCount++;
            }

            numWrite = sysData->pageSize - pageOffset;
            if (numWrite > remCount)
                numWrite = remCount;
            memcpy(&sysData->cacheData[pageIndex * sysData->pageSize + pageOffset], &data[count - remCount], numWrite);
            if (pageOffset + numWrite > sysData->pages[pageIndex].size)
                sysData->pages[pageIndex].size = pageOffset + numWrite;
            remCount -= numWrite;
            sysData->position += numWrite;
        } else {
            numPagesWrite = (remCount + sysData->pageSize - 1) / sysData->pageSize;
            if (numPagesWrite > sysData->numPages)
                numPagesWrite = sysData->numPages;

            if (sysData->writingAtEOF) {
                CHK_ORET(flush_dirty_pages(sysData, pageIndex, numPagesWrite));
                for (i = 0; i < numPagesWrite; i++) {
                    sysData->pages[(pageIndex + i) % sysData->numPages].size     = 0;
                    sysData->pages[(pageIndex + i) % sysData->numPages].position = -1;
                }
            } else {
                originalPosition = sysData->position;
                originalEOF      = sysData->eof;

                numRead = cache_file_read(sysData, NULL, numPagesWrite * sysData->pageSize);
                if (numRead < numPagesWrite * sysData->pageSize) {
                    CHK_ORET(sysData->eof);
                    sysData->writingAtEOF = 1;
                }

                sysData->position = originalPosition;
                sysData->eof      = originalEOF;
            }
        }
    }

    return count - remCount;
}

static int cache_file_getchar(MXFFileSysData *sysData)
{
    uint8_t data;
    if (cache_file_read(sysData, &data, 1) != 1)
        return EOF;

    return data;
}

static int cache_file_putchar(MXFFileSysData *sysData, int c)
{
    uint8_t data = (uint8_t)c;
    if (cache_file_write(sysData, &data, 1) != 1)
        return EOF;

    return data;
}

static int cache_file_eof(MXFFileSysData *sysData)
{
    return sysData->eof;
}

static int cache_file_seek(MXFFileSysData *sysData, int64_t offset, int whence)
{
    int64_t newPosition = 0;
    int64_t fileSize;
    int64_t pagePosition;
    uint32_t pageIndex;
    uint32_t pageOffset;
    int64_t targetPosition;

    switch (whence)
    {
        case SEEK_SET:
            newPosition = offset;
            break;
        case SEEK_CUR:
            newPosition = sysData->position + offset;
            break;
        case SEEK_END:
            fileSize = mxf_file_size(sysData->target);
            newPosition = fileSize + offset;
            break;
        default:
            return 0;
    }
    if (newPosition < 0)
        return 0;

    sysData->eof          = 0;
    sysData->position     = newPosition;
    sysData->writingAtEOF = 0;

    get_current_page_info(sysData, &pagePosition, &pageIndex, &pageOffset);
    if (sysData->pages[pageIndex].position == pagePosition)
        targetPosition = pagePosition + sysData->pageSize; /* page has been read and so seek to after the page */
    else
        targetPosition = pagePosition; /* page will need to be read so seek to the start of the page */

    return mxf_file_seek(sysData->target, targetPosition, SEEK_SET);
}

static int64_t cache_file_tell(MXFFileSysData *sysData)
{
    return sysData->position;
}

static int cache_file_is_seekable(MXFFileSysData *sysData)
{
    return mxf_file_is_seekable(sysData->target);
}

static int64_t cache_file_size(MXFFileSysData *sysData)
{
    int64_t size = mxf_file_size(sysData->target);

    if (sysData->dirtyCount > 0) {
        /* if there are dirty pages then check the end of the last dirty page */
        uint32_t lastDirtyPage = (sysData->firstDirtyPage + sysData->dirtyCount - 1) % sysData->numPages;
        if (sysData->pages[lastDirtyPage].position + sysData->pages[lastDirtyPage].size > size)
            size = sysData->pages[lastDirtyPage].position + sysData->pages[lastDirtyPage].size;
    }

    return size;
}

static void free_cache_file(MXFFileSysData *sysData)
{
    free(sysData);
}



int mxf_cache_file_open(MXFFile *target, uint32_t in_cachePageSize, uint32_t cacheSize, MXFCacheFile **cacheFile)
{
    MXFFile *newMXFFile = NULL;
    MXFFileSysData *newDiskFile = NULL;
    Page *newPages = NULL;
    unsigned char *newCacheData = NULL;
    uint32_t numPages;
    uint32_t sysPageSize;
    uint32_t cachePageSize = in_cachePageSize;

    sysPageSize = mxf_get_system_page_size();
    CHK_ORET(sysPageSize > 0);

    if (cachePageSize == 0) {
        cachePageSize = sysPageSize;
    } else if (cachePageSize % sysPageSize != 0) {
        cachePageSize = (cachePageSize + sysPageSize - 1) & ~(sysPageSize - 1);
        if (in_cachePageSize != 0) {
            mxf_log_warn("MXF cache file page size %u is not a multiple of page size %u. "
                         "Changed cache page size to %u\n",
                         in_cachePageSize, sysPageSize, cachePageSize);
        }
    }

    numPages = cacheSize / cachePageSize;

    CHK_ORET(numPages > 0);
    CHK_ORET(numPages < UINT32_MAX / 2); /* required to prevent overflows in calculations */

    CHK_MALLOC_ORET(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(MXFFile));
    CHK_MALLOC_OFAIL(newDiskFile, MXFFileSysData);
    memset(newDiskFile, 0, sizeof(MXFFileSysData));
    CHK_MALLOC_ARRAY_OFAIL(newCacheData, unsigned char, numPages * cachePageSize + sysPageSize);
    CHK_MALLOC_ARRAY_OFAIL(newPages, Page, numPages);
    memset(newPages, 0, sizeof(newPages[0]) * numPages);
    newPages[0].position = -1;

    newDiskFile->target         = target;
    newDiskFile->allocCacheData = newCacheData;
    newDiskFile->cacheData      = (unsigned char*)(((uintptr_t)newCacheData + sysPageSize - 1) & ~(uintptr_t)(sysPageSize - 1));
    newDiskFile->cacheSize      = numPages * cachePageSize;
    newDiskFile->pages          = newPages;
    newDiskFile->numPages       = numPages;
    newDiskFile->pageSize       = cachePageSize;

    newDiskFile->cacheFile.mxfFile = newMXFFile;

    newMXFFile->close         = cache_file_close;
    newMXFFile->read          = cache_file_read;
    newMXFFile->write         = cache_file_write;
    newMXFFile->get_char      = cache_file_getchar;
    newMXFFile->put_char      = cache_file_putchar;
    newMXFFile->eof           = cache_file_eof;
    newMXFFile->seek          = cache_file_seek;
    newMXFFile->tell          = cache_file_tell;
    newMXFFile->is_seekable   = cache_file_is_seekable;
    newMXFFile->size          = cache_file_size;
    newMXFFile->free_sys_data = free_cache_file;
    newMXFFile->sysData       = newDiskFile;
    newMXFFile->minLLen       = target->minLLen;
    newMXFFile->runinLen      = target->runinLen;


    *cacheFile = &newDiskFile->cacheFile;
    return 1;

fail:
    SAFE_FREE(newMXFFile);
    SAFE_FREE(newDiskFile);
    SAFE_FREE(newPages);
    SAFE_FREE(newCacheData);
    return 0;
}

MXFFile* mxf_cache_file_get_file(MXFCacheFile *cacheFile)
{
    return cacheFile->mxfFile;
}

uint32_t mxf_cache_file_get_dirty_count(MXFCacheFile *cacheFile)
{
    return cacheFile->mxfFile->sysData->dirtyCount;
}

uint32_t mxf_cache_file_flush(MXFCacheFile *cacheFile, uint32_t minSize)
{
    MXFFileSysData *sysData = cacheFile->mxfFile->sysData;
    uint32_t originalDirtyCount = sysData->dirtyCount;

    if (minSize == 0 || sysData->dirtyCount == 0)
        return 0;

    flush_dirty_pages(sysData, sysData->firstDirtyPage, (minSize + sysData->pageSize - 1) / sysData->pageSize);

    return (originalDirtyCount - sysData->dirtyCount) * sysData->pageSize;
}

