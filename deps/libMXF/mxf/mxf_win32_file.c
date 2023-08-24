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
#include <string.h>
#include <stdio.h>
#include <windows.h>

#include <mxf/mxf.h>
#include <mxf/mxf_win32_file.h>
#include <mxf/mxf_macros.h>


typedef enum
{
    NEW_MODE,
    READ_MODE,
    MODIFY_MODE,
} OpenMode;

struct MXFFileSysData
{
    HANDLE *file;
};



static void win32_file_close(MXFFileSysData *sysData)
{
    if (sysData->file != INVALID_HANDLE_VALUE) {
        CloseHandle(sysData->file);
        sysData->file = INVALID_HANDLE_VALUE;
    }
}

static uint32_t win32_file_read(MXFFileSysData *sysData, uint8_t *data, uint32_t count)
{
    DWORD num_read;
    if (!ReadFile(sysData->file, data, count, &num_read, NULL))
        return 0;

    return num_read;
}

static uint32_t win32_file_write(MXFFileSysData *sysData, const uint8_t *data, uint32_t count)
{
    DWORD num_write;
    if (!WriteFile(sysData->file, data, count, &num_write, NULL))
        return 0;

    return num_write;
}

static int win32_file_getchar(MXFFileSysData *sysData)
{
    uint8_t data;
    if (win32_file_read(sysData, &data, 1) != 1)
        return EOF;

    return data;
}

static int win32_file_putchar(MXFFileSysData *sysData, int c)
{
    uint8_t data = (uint8_t)c;
    if (win32_file_write(sysData, &data, 1) != 1)
        return EOF;

    return data;
}

static int win32_file_seek(MXFFileSysData *sysData, int64_t offset, int whence)
{
    LARGE_INTEGER li_offset;
    li_offset.QuadPart = offset;

    switch (whence)
    {
        case SEEK_CUR:
            return SetFilePointerEx(sysData->file, li_offset, NULL, FILE_CURRENT);
        case SEEK_SET:
            return SetFilePointerEx(sysData->file, li_offset, NULL, FILE_BEGIN);
        case SEEK_END:
            return SetFilePointerEx(sysData->file, li_offset, NULL, FILE_END);
        default:
            return 0;
    }
}

static int64_t win32_file_tell(MXFFileSysData *sysData)
{
    LARGE_INTEGER zero_offset;
    LARGE_INTEGER pos;

    zero_offset.QuadPart = 0;
    if (!SetFilePointerEx(sysData->file, zero_offset, &pos, FILE_CURRENT))
        return -1;

    return pos.QuadPart;
}

static int64_t win32_file_size(MXFFileSysData *sysData)
{
    LARGE_INTEGER size;
    if (!GetFileSizeEx(sysData->file, &size))
        return -1;

    return size.QuadPart;
}

static int win32_file_eof(MXFFileSysData *sysData)
{
    return win32_file_tell(sysData) >= win32_file_size(sysData);
}

static int win32_file_is_seekable(MXFFileSysData *sysData)
{
    return GetFileType(sysData->file) == FILE_TYPE_DISK;
}

static void free_win32_file(MXFFileSysData *sysData)
{
    free(sysData);
}

static int mxf_win32_file_open(const char *in_filename, int flags, OpenMode mode, MXFFile **mxfFile)
{
    MXFFile *newMXFFile = NULL;
    MXFFileSysData *newDiskFile = NULL;
    int attrs_and_flags = FILE_ATTRIBUTE_NORMAL;
#if !defined (_UNICODE)
    const char *filename = in_filename;
#else
    wchar_t *filename = NULL;
    size_t size;

    CHK_OFAIL((size = mbstowcs(NULL, in_filename, 0)) != (size_t)(-1));
    size += 1;
    CHK_MALLOC_ARRAY_OFAIL(filename, wchar_t, size);
    mbstowcs(filename, in_filename, size);
#endif

    CHK_MALLOC_OFAIL(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(MXFFile));
    CHK_MALLOC_OFAIL(newDiskFile, MXFFileSysData);
    memset(newDiskFile, 0, sizeof(MXFFileSysData));
    newDiskFile->file = INVALID_HANDLE_VALUE;

    if (flags & MXF_WIN32_FLAG_RANDOM_ACCESS)
        attrs_and_flags |= FILE_FLAG_RANDOM_ACCESS;
    else if (flags & MXF_WIN32_FLAG_SEQUENTIAL_SCAN)
        attrs_and_flags |= FILE_FLAG_SEQUENTIAL_SCAN;

    switch (mode)
    {
        case NEW_MODE:
            newDiskFile->file = CreateFile(filename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
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
    if (newDiskFile->file == INVALID_HANDLE_VALUE)
        goto fail;

#if defined (_UNICODE)
    SAFE_FREE(filename);
#endif

    newMXFFile->close         = win32_file_close;
    newMXFFile->read          = win32_file_read;
    newMXFFile->write         = win32_file_write;
    newMXFFile->get_char      = win32_file_getchar;
    newMXFFile->put_char      = win32_file_putchar;
    newMXFFile->eof           = win32_file_eof;
    newMXFFile->seek          = win32_file_seek;
    newMXFFile->tell          = win32_file_tell;
    newMXFFile->is_seekable   = win32_file_is_seekable;
    newMXFFile->size          = win32_file_size;

    newMXFFile->free_sys_data = free_win32_file;
    newMXFFile->sysData       = newDiskFile;


    *mxfFile = newMXFFile;
    return 1;

fail:
    SAFE_FREE(newMXFFile);
    SAFE_FREE(newDiskFile);
#if defined (_UNICODE)
    SAFE_FREE(filename);
#endif
    return 0;
}



int mxf_win32_file_open_new(const char *filename, int flags, MXFFile **mxfFile)
{
    return mxf_win32_file_open(filename, flags, NEW_MODE, mxfFile);
}

int mxf_win32_file_open_read(const char *filename, int flags, MXFFile **mxfFile)
{
    return mxf_win32_file_open(filename, flags, READ_MODE, mxfFile);
}

int mxf_win32_file_open_modify(const char *filename, int flags, MXFFile **mxfFile)
{
    return mxf_win32_file_open(filename, flags, MODIFY_MODE, mxfFile);
}

