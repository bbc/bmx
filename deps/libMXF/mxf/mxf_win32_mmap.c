/*
 * Copyright (C) 2015, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Thomas Gritzan (phygon@users.sf.net)
 *         Philip de Nier
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

#if (_MSC_VER >= 1700) // Visual C++ 2010 has conflicts with stdint.h and so disable this include for that version
#include <intsafe.h>
#else
#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

#include <mxf/mxf.h>
#include <mxf/mxf_win32_mmap.h>
#include <mxf/mxf_macros.h>


#define FILE_GROW_CHUNK_SIZE    (32 * 1024 * 1024)    // 32 MB

typedef enum
{
    NEW_MODE,
    READ_MODE,
    MODIFY_MODE,
} OpenMode;

struct MXFFileSysData
{
    HANDLE file;
    HANDLE hmap;

    OpenMode openMode;
    BOOL seekable;

    uint32_t defaultViewSize;

    uint64_t diskFileSize;      // current file size on disk; will be increased in chunk when writing
    uint64_t logicalFileSize;   // logical file size; file will be resized to this when closed

    uint64_t offset;    // location of pData in file
    uint8_t *pData;     // pointer to memory mapped data

    uint32_t cursor;    // read cursor in pData
    uint32_t size;      // number of bytes in pData
};


static void mxf_win32_log_errorId(DWORD dwMessageId, const char *file, int line)
{
    LPSTR lpMsgBuf = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwMessageId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf,
        0, NULL
    );

    if (lpMsgBuf != NULL)
    {
        LPSTR lpNewline = strchr(lpMsgBuf, '\r');
        if (lpNewline != NULL)
        {
            *lpNewline = '\0';      // remove trailing newline character
        }
        mxf_log_error("Reason: %s (0x%08x), in %s:%d\n", lpMsgBuf, dwMessageId, file, line);
        LocalFree(lpMsgBuf);
    }
}


static BOOL mxf_win32_mmap_CreateFileMapping(MXFFileSysData *sysData)
{
    DWORD flProtect;

    if (sysData->hmap != NULL)
    {
        CloseHandle(sysData->hmap);
        sysData->hmap = NULL;
    }

    flProtect = sysData->openMode == READ_MODE ? PAGE_READONLY : PAGE_READWRITE;

    sysData->hmap = CreateFileMappingW(sysData->file, NULL, flProtect,
                        HIDWORD(sysData->diskFileSize), LODWORD(sysData->diskFileSize), NULL);
    return sysData->hmap != NULL;
}

static BOOL mxf_win32_mmap_MapViewOfFile(MXFFileSysData *sysData)
{
    uint32_t viewSize;
    DWORD dwDesiredAccess;

    // unmap old view
    if (sysData->pData != NULL)
    {
        UnmapViewOfFile(sysData->pData);
        sysData->pData = NULL;
        sysData->size = 0;
    }

    // map new view
    viewSize = sysData->defaultViewSize;
    if (sysData->offset + viewSize > sysData->diskFileSize)
    {
        if (sysData->openMode == READ_MODE)
        {
            viewSize = (uint32_t)(sysData->diskFileSize - sysData->offset);
        }
        else /* NEW_MODE, MODIFY_MODE */
        {
            int32_t chunkSize;

            sysData->diskFileSize = sysData->offset + viewSize;

            chunkSize = sysData->defaultViewSize;
            if (sysData->diskFileSize > FILE_GROW_CHUNK_SIZE)
            {
                chunkSize = FILE_GROW_CHUNK_SIZE;
            }

            // round diskFileSize up to a multiple of chunkSize; file size increments in too small steps hurt the performance
            if (sysData->diskFileSize % chunkSize)
            {
                sysData->diskFileSize += chunkSize - sysData->diskFileSize % chunkSize;
            }

            CHK_OFAIL(mxf_win32_mmap_CreateFileMapping(sysData));
        }
    }

    dwDesiredAccess = sysData->openMode == READ_MODE ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE);

    sysData->size = viewSize;
    sysData->pData = (uint8_t*)MapViewOfFile(sysData->hmap, dwDesiredAccess,
                        HIDWORD(sysData->offset), LODWORD(sysData->offset), viewSize);
    return sysData->pData != NULL;

fail:
    mxf_win32_log_errorId(GetLastError(), __FILENAME__, __LINE__);
    return FALSE;
}

static void mxf_win32_mmap_close(MXFFileSysData *sysData)
{
    if (sysData->pData != NULL)
    {
        UnmapViewOfFile(sysData->pData);
        sysData->pData = NULL;
    }
    if (sysData->hmap != NULL)
    {
        CloseHandle(sysData->hmap);
        sysData->hmap = NULL;
    }
    if (sysData->file != INVALID_HANDLE_VALUE)
    {
        // truncate file to its logical size
        if (sysData->openMode != READ_MODE && sysData->logicalFileSize < sysData->diskFileSize)
        {
            LARGE_INTEGER newSize;
            newSize.QuadPart = sysData->logicalFileSize;
            if (SetFilePointerEx(sysData->file, newSize, NULL, FILE_BEGIN))
            {
                SetEndOfFile(sysData->file);
            }
        }

        CloseHandle(sysData->file);
        sysData->file = INVALID_HANDLE_VALUE;
    }
}

static int mxf_win32_mmap_seek(MXFFileSysData *sysData, int64_t offset, int whence)
{
    int64_t newPosition = 0;
    BOOL mustGrowFile;
    uint32_t newCursor;
    uint64_t newOffset;

    switch (whence)
    {
    case SEEK_SET:
        newPosition = offset;
        break;
    case SEEK_CUR:
        newPosition = offset + sysData->offset + sysData->cursor;
        break;
    case SEEK_END:
        newPosition = offset + sysData->logicalFileSize;
        break;
    default:
        return FALSE;
    }

    if (newPosition < 0)
        return FALSE;

    mustGrowFile = (uint64_t)newPosition >= sysData->diskFileSize;
    if (mustGrowFile && sysData->openMode == READ_MODE)
        return FALSE;

    newCursor = newPosition % sysData->defaultViewSize;
    newOffset = newPosition - newCursor;

    sysData->cursor = newCursor;

    if (sysData->offset != newOffset || mustGrowFile)
    {
        // remap view if we moved the position outside of the current view
        sysData->offset = newOffset;
        CHK_OFAIL(mxf_win32_mmap_MapViewOfFile(sysData));
    }

    return TRUE;

fail:
    mxf_win32_log_errorId(GetLastError(), __FILENAME__, __LINE__);
    return FALSE;
}

static uint32_t mxf_win32_mmap_read(MXFFileSysData *sysData, uint8_t *data, uint32_t count)
{
    uint32_t total_read = 0;

    while (count > 0)
    {
        // remap view of file
        if (sysData->cursor >= sysData->size)
        {
            if (!mxf_win32_mmap_seek(sysData, 0, SEEK_CUR))
            {
                break;
            }
        }

        // copy from memory mapped file to data array
        __try
        {
            uint32_t avail = min(sysData->size - sysData->cursor, count);
            if (avail > 0)
            {
                memcpy(data + total_read, sysData->pData + sysData->cursor, avail);
                sysData->cursor += avail;
                count -= avail;
                total_read += avail;
            }
        }
        __except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            // failed to read from the file mapping; return current value of "total_read"
            break;
        }
    }

    return total_read;
}

static uint32_t mxf_win32_mmap_write(MXFFileSysData *sysData, const uint8_t *data, uint32_t count)
{
    uint32_t total_written = 0;

    while (count > 0)
    {
        // remap view of file
        if (sysData->cursor >= sysData->size)
        {
            if (!mxf_win32_mmap_seek(sysData, 0, SEEK_CUR))
            {
                break;
            }
        }

        // copy from data array to memory mapped file
        __try
        {
            uint32_t avail = min(sysData->size - sysData->cursor, count);
            if (avail > 0)
            {
                memcpy(sysData->pData + sysData->cursor, data + total_written, avail);
                sysData->cursor += avail;
                count -= avail;
                total_written += avail;
            }
        }
        __except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            // failed to write to the file mapping; return current value of "total_written"
            break;
        }
    }

    // move EOF, if data was written after the current EOF
    if (sysData->logicalFileSize < sysData->offset + sysData->cursor)
    {
        sysData->logicalFileSize = sysData->offset + sysData->cursor;
    }

    return total_written;
}

static int mxf_win32_mmap_getchar(MXFFileSysData *sysData)
{
    uint8_t data;
    if (mxf_win32_mmap_read(sysData, &data, 1) != 1)
        return EOF;

    return data;
}

static int mxf_win32_mmap_putchar(MXFFileSysData *sysData, int c)
{
    uint8_t data = (uint8_t)c;
    if (mxf_win32_mmap_write(sysData, &data, 1) != 1)
        return EOF;

    return data;
}

static int64_t mxf_win32_mmap_tell(MXFFileSysData *sysData)
{
    return sysData->offset + sysData->cursor;
}

static int64_t mxf_win32_mmap_size(MXFFileSysData *sysData)
{
    return sysData->logicalFileSize;
}

static int mxf_win32_mmap_eof(MXFFileSysData *sysData)
{
    return sysData->offset + sysData->cursor >= sysData->logicalFileSize;
}

static int mxf_win32_mmap_is_seekable(MXFFileSysData *sysData)
{
    return sysData->seekable;
}

static void free_win32_mmap(MXFFileSysData *sysData)
{
    SAFE_FREE(sysData);
}

static int mxf_win32_mmap_open(const TCHAR *filename, int flags, OpenMode mode, MXFFile **mxfFile)
{
    SYSTEM_INFO sysinfo = { 0 };
    MXFFile *newMXFFile = NULL;
    MXFFileSysData *newDiskFile = NULL;
    DWORD attrs_and_flags = FILE_ATTRIBUTE_NORMAL;
    LARGE_INTEGER fileSize;

    CHK_MALLOC_OFAIL(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(MXFFile));
    CHK_MALLOC_OFAIL(newDiskFile, MXFFileSysData);
    memset(newDiskFile, 0, sizeof(MXFFileSysData));
    newDiskFile->file = INVALID_HANDLE_VALUE;

    if (flags & MXF_WIN32_FLAG_RANDOM_ACCESS)
        attrs_and_flags |= FILE_FLAG_RANDOM_ACCESS;
    if (flags & MXF_WIN32_FLAG_SEQUENTIAL_SCAN)
        attrs_and_flags |= FILE_FLAG_SEQUENTIAL_SCAN;

    newDiskFile->openMode = mode;
    switch (mode)
    {
        case NEW_MODE:
            newDiskFile->file = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           NULL, CREATE_ALWAYS, attrs_and_flags, NULL);
            break;
        case READ_MODE:
            newDiskFile->file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           NULL, OPEN_EXISTING, attrs_and_flags, NULL);
            break;
        case MODIFY_MODE:
            newDiskFile->file = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           NULL, OPEN_EXISTING, attrs_and_flags, NULL);
            break;
    }
    CHK_OFAIL(newDiskFile->file != INVALID_HANDLE_VALUE);

    newDiskFile->seekable = GetFileType(newDiskFile->file) == FILE_TYPE_DISK;

    // allocation granularity (usually 64 KB; size of view should be a multiple of this) and file size
    GetSystemInfo(&sysinfo);
    newDiskFile->defaultViewSize = (sysinfo.dwAllocationGranularity * 16) * 4;      // 4 MB

    CHK_OFAIL(GetFileSizeEx(newDiskFile->file, &fileSize));

    newDiskFile->diskFileSize = fileSize.QuadPart;
    newDiskFile->logicalFileSize = fileSize.QuadPart;

    // since we cannot create a file mapping of size 0, initialize our empty file to be of any size greater than zero
    if (newDiskFile->openMode != READ_MODE && newDiskFile->diskFileSize == 0)
    {
        newDiskFile->diskFileSize = sysinfo.dwAllocationGranularity;
    }

    // create file mapping and map a view
    CHK_OFAIL(mxf_win32_mmap_CreateFileMapping(newDiskFile));

    CHK_OFAIL(mxf_win32_mmap_MapViewOfFile(newDiskFile));

    newMXFFile->close         = mxf_win32_mmap_close;
    newMXFFile->read          = mxf_win32_mmap_read;
    newMXFFile->write         = mxf_win32_mmap_write;
    newMXFFile->get_char      = mxf_win32_mmap_getchar;
    newMXFFile->put_char      = mxf_win32_mmap_putchar;
    newMXFFile->eof           = mxf_win32_mmap_eof;
    newMXFFile->seek          = mxf_win32_mmap_seek;
    newMXFFile->tell          = mxf_win32_mmap_tell;
    newMXFFile->is_seekable   = mxf_win32_mmap_is_seekable;
    newMXFFile->size          = mxf_win32_mmap_size;

    newMXFFile->free_sys_data = free_win32_mmap;
    newMXFFile->sysData       = newDiskFile;

    *mxfFile = newMXFFile;
    return 1;

fail:
    mxf_win32_log_errorId(GetLastError(), __FILENAME__, __LINE__);
    if (newDiskFile)
    {
        if (newDiskFile->pData)
            UnmapViewOfFile(newDiskFile->pData);
        if (newDiskFile->hmap)
            CloseHandle(newDiskFile->hmap);
        if (newDiskFile->file != INVALID_HANDLE_VALUE)
            CloseHandle(newDiskFile->file);
    }
    SAFE_FREE(newDiskFile);
    SAFE_FREE(newMXFFile);
    return 0;
}

static int mxf_win32_mmap_open_mb(const char *in_filename, int flags, OpenMode mode, MXFFile **mxfFile)
{
#ifndef _UNICODE
    return mxf_win32_mmap_open(in_filename, flags, mode, mxfFile);
#else
    int result = 0;
    wchar_t *filename = NULL;
    size_t size;

    CHK_OFAIL((size = mbstowcs(NULL, in_filename, 0)) != (size_t)(-1));
    size += 1;
    CHK_MALLOC_ARRAY_OFAIL(filename, wchar_t, size);
    mbstowcs(filename, in_filename, size);

    result = mxf_win32_mmap_open(filename, flags, mode, mxfFile);
    SAFE_FREE(filename);
    return result;

fail:
    SAFE_FREE(filename);
    return 0;
#endif
}


int mxf_win32_mmap_open_new(const char *filename, int flags, MXFFile **mxfFile)
{
    return mxf_win32_mmap_open_mb(filename, flags, NEW_MODE, mxfFile);
}

int mxf_win32_mmap_open_read(const char *filename, int flags, MXFFile **mxfFile)
{
    return mxf_win32_mmap_open_mb(filename, flags, READ_MODE, mxfFile);
}

int mxf_win32_mmap_open_modify(const char *filename, int flags, MXFFile **mxfFile)
{
    return mxf_win32_mmap_open_mb(filename, flags, MODIFY_MODE, mxfFile);
}
