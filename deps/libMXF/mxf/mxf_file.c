/*
 * Wraps a files, buffers etc. in an MXF file and provides low-level functions
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Stuart Cunningham
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
#include <sys/stat.h>
#include <errno.h>

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif

#include <mxf/mxf.h>
#include <mxf/mxf_stream_file.h>
#include <mxf/mxf_macros.h>


#if defined(_MSC_VER) && (_MSC_VER < 1400)
#error Visual C++ 2005 or later is required. Earlier versions do not support 64-bit stream I/O
#endif



/* size of buffer used to skip data by reading and discarding */
#define SKIP_BUFFER_SIZE        2048

#define MAX_ZEROS_BUFFER_SIZE   16384
#define ZEROS_BUFFER_INCREMENT  2048


typedef enum
{
    NEW_MODE,
    READ_MODE,
    MODIFY_MODE,
} OpenMode;

struct MXFFileSysData
{
    FILE *file;
    OpenMode mode;
    int isSeekable;
};


static int check_file_is_seekable(FILE *file, int *isSeekable)
{
#if defined(_WIN32)
    struct _stati64 buf;
#else
    struct stat buf;
#endif
    char errorBuf[128];

#if defined(_WIN32)
    if (_fstati64(_fileno(file), &buf) != 0)
#else
    if (fstat(fileno(file), &buf) != 0)
#endif
    {
        mxf_log_error("unexpected fstat error when checking seek support: %s\n",
                      mxf_strerror(errno, errorBuf, sizeof(errorBuf)));
        return 0;
    }

#if defined(_WIN32)
    *isSeekable = ((buf.st_mode & S_IFMT) == S_IFREG);
#else
    *isSeekable = S_ISREG(buf.st_mode);
#endif

    return 1;
}


static void disk_file_close(MXFFileSysData *sysData)
{
    if (sysData->file &&
        sysData->file != stdin && sysData->file != stdout && sysData->file != stderr)
    {
        fclose(sysData->file);
    }
    sysData->file = NULL;
}

static uint32_t disk_file_read(MXFFileSysData *sysData, uint8_t *data, uint32_t count)
{
    char errorBuf[128];
    uint32_t result = (uint32_t)fread(data, 1, count, sysData->file);
    if (result != count && ferror(sysData->file))
        mxf_log_error("fread failed: %s\n", mxf_strerror(errno, errorBuf, sizeof(errorBuf)));
    return result;
}

static uint32_t disk_file_write(MXFFileSysData *sysData, const uint8_t *data, uint32_t count)
{
    char errorBuf[128];
    uint32_t result = (uint32_t)fwrite(data, 1, count, sysData->file);
    if (result != count)
        mxf_log_error("fwrite failed: %s\n", mxf_strerror(errno, errorBuf, sizeof(errorBuf)));
    return result;
}

static int disk_file_getchar(MXFFileSysData *sysData)
{
    return fgetc(sysData->file);
}

static int disk_file_putchar(MXFFileSysData *sysData, int c)
{
    return fputc(c, sysData->file);
}

static int disk_file_eof(MXFFileSysData *sysData)
{
    return feof(sysData->file);
}

static int disk_file_seek(MXFFileSysData *sysData, int64_t offset, int whence)
{
#if defined(_WIN32)
    return _fseeki64(sysData->file, offset, whence) == 0;
#else
    return fseeko(sysData->file, offset, whence) == 0;
#endif
}

static int64_t disk_file_tell(MXFFileSysData *sysData)
{
#if defined(_WIN32)
    return _ftelli64(sysData->file);
#else
    return ftello(sysData->file);
#endif
}

static int disk_file_is_seekable(MXFFileSysData *sysData)
{
    return sysData->isSeekable;
}

static int64_t disk_file_size(MXFFileSysData *sysData)
{
#if defined(_WIN32)
    struct _stati64 statBuf;
#else
    struct stat statBuf;
#endif

    // flush user-space data because fstat uses the stream's integer descriptor
    if (sysData->mode == NEW_MODE || sysData->mode == MODIFY_MODE)
        fflush(sysData->file);

#if defined(_WIN32)
    if (_fstati64(_fileno(sysData->file), &statBuf) != 0)
#else
    if (fstat(fileno(sysData->file), &statBuf) != 0)
#endif
    {
        return -1;
    }

    return statBuf.st_size;
}

static void free_disk_file(MXFFileSysData *sysData)
{
    free(sysData);
}


static int disk_file_open(FILE *file, OpenMode mode, MXFFile **mxfFile)
{
    MXFFile *newMXFFile = NULL;
    MXFFileSysData *newDiskFile = NULL;
    int isSeekable = 0;

    if (!check_file_is_seekable(file, &isSeekable))
        goto fail;

    CHK_MALLOC_OFAIL(newMXFFile, MXFFile);
    memset(newMXFFile, 0, sizeof(MXFFile));
    CHK_MALLOC_OFAIL(newDiskFile, MXFFileSysData);
    memset(newDiskFile, 0, sizeof(MXFFileSysData));

    newDiskFile->file       = file;
    newDiskFile->isSeekable = isSeekable;
    newDiskFile->mode       = mode;

    newMXFFile->close         = disk_file_close;
    newMXFFile->read          = disk_file_read;
    newMXFFile->write         = disk_file_write;
    newMXFFile->get_char      = disk_file_getchar;
    newMXFFile->put_char      = disk_file_putchar;
    newMXFFile->eof           = disk_file_eof;
    newMXFFile->seek          = disk_file_seek;
    newMXFFile->tell          = disk_file_tell;
    newMXFFile->is_seekable   = disk_file_is_seekable;
    newMXFFile->size          = disk_file_size;
    newMXFFile->free_sys_data = free_disk_file;
    newMXFFile->sysData       = newDiskFile;

    if (!isSeekable) {
        MXFFile *newStreamMXFFile = NULL;
        if (!mxf_stream_file_wrap(newMXFFile, mode == READ_MODE, &newStreamMXFFile))
            goto fail;
        *mxfFile = newStreamMXFFile;
    } else {
        *mxfFile = newMXFFile;
    }

    return 1;

fail:
    if (file != stdin && file != stdout && file != stderr)
        fclose(file);
    SAFE_FREE(newMXFFile);
    SAFE_FREE(newDiskFile);
    return 0;
}


int mxf_disk_file_open_new(const char *filename, MXFFile **mxfFile)
{
    FILE *file = fopen(filename, "w+b");
    if (!file)
        return 0;

    return disk_file_open(file, NEW_MODE, mxfFile);
}

int mxf_disk_file_open_read(const char *filename, MXFFile **mxfFile)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
        return 0;

    return disk_file_open(file, READ_MODE, mxfFile);
}

int mxf_disk_file_open_modify(const char *filename, MXFFile **mxfFile)
{
    FILE *file = fopen(filename, "r+b");
    if (!file)
        return 0;

    return disk_file_open(file, MODIFY_MODE, mxfFile);
}


int mxf_stdin_wrap_read(MXFFile **mxfFile)
{
#if defined(_WIN32)
    char errorBuf[128];
    int res = _setmode(_fileno(stdin), _O_BINARY);
    if (res == -1) {
        mxf_log_error("failed to set 'stdin' to binary mode: %s\n", mxf_strerror(errno, errorBuf, sizeof(errorBuf)));
        return 0;
    }
#endif

    return disk_file_open(stdin, READ_MODE, mxfFile);
}

int mxf_stdout_wrap_write(MXFFile **mxfFile)
{
#if defined(_WIN32)
    char errorBuf[128];
    int res = _setmode(_fileno(stdout), _O_BINARY);
    if (res == -1) {
        mxf_log_error("failed to set 'stdout' to binary mode: %s\n", mxf_strerror(errno, errorBuf, sizeof(errorBuf)));
        return 0;
    }
#endif

    return disk_file_open(stdout, NEW_MODE, mxfFile);
}


void mxf_file_close(MXFFile **mxfFile)
{
    mxf_file_close_2(mxfFile, free);
}

void mxf_file_close_2(MXFFile **mxfFile, void (*free_func)(void*))
{
    if (!(*mxfFile))
        return;

    free((*mxfFile)->zerosBuffer);

    if ((*mxfFile)->sysData) {
        (*mxfFile)->close((*mxfFile)->sysData);
        if ((*mxfFile)->free_sys_data)
            (*mxfFile)->free_sys_data((*mxfFile)->sysData);
    }

    free_func(*mxfFile);
    *mxfFile = NULL;
}

uint32_t mxf_file_read(MXFFile *mxfFile, uint8_t *data, uint32_t count)
{
    return mxfFile->read(mxfFile->sysData, data, count);
}

uint32_t mxf_file_write(MXFFile *mxfFile, const uint8_t *data, uint32_t count)
{
    return mxfFile->write(mxfFile->sysData, data, count);
}

int mxf_file_getc(MXFFile *mxfFile)
{
    return mxfFile->get_char(mxfFile->sysData);
}

int mxf_file_putc(MXFFile *mxfFile, int c)
{
    return mxfFile->put_char(mxfFile->sysData, c);
}

int mxf_file_eof(MXFFile *mxfFile)
{
    return mxfFile->eof(mxfFile->sysData);
}

int mxf_file_seek(MXFFile *mxfFile, int64_t offset, int whence)
{
    return mxfFile->seek(mxfFile->sysData, offset, whence);
}

int64_t mxf_file_tell(MXFFile *mxfFile)
{
    return mxfFile->tell(mxfFile->sysData);
}

int mxf_file_is_seekable(MXFFile *mxfFile)
{
    return mxfFile->is_seekable(mxfFile->sysData);
}

int64_t mxf_file_size(MXFFile *mxfFile)
{
    return mxfFile->size(mxfFile->sysData);
}


void mxf_file_set_min_llen(MXFFile *mxfFile, uint8_t llen)
{
    mxfFile->minLLen = llen;
}

uint8_t mxf_get_min_llen(MXFFile *mxfFile)
{
    if (mxfFile->minLLen != 0)
        return mxfFile->minLLen;

    return 1;
}


int mxf_read_uint8(MXFFile *mxfFile, uint8_t *value)
{
    uint8_t buffer[1];
    CHK_ORET(mxf_file_read(mxfFile, buffer, 1) == 1);

    *value = buffer[0];

    return 1;
}

int mxf_read_uint16(MXFFile *mxfFile, uint16_t *value)
{
    uint8_t buffer[2];
    CHK_ORET(mxf_file_read(mxfFile, buffer, 2) == 2);

    *value = (buffer[0] << 8) |
              buffer[1];

    return 1;
}

int mxf_read_uint32(MXFFile *mxfFile, uint32_t *value)
{
    uint8_t buffer[4];
    CHK_ORET(mxf_file_read(mxfFile, buffer, 4) == 4);

    *value = (buffer[0] << 24) |
             (buffer[1] << 16) |
             (buffer[2] << 8) |
              buffer[3];

    return 1;
}

int mxf_read_uint64(MXFFile *mxfFile, uint64_t *value)
{
    uint8_t buffer[8];
    CHK_ORET(mxf_file_read(mxfFile, buffer, 8) == 8);

    *value = ((uint64_t)buffer[0] << 56) |
             ((uint64_t)buffer[1] << 48) |
             ((uint64_t)buffer[2] << 40) |
             ((uint64_t)buffer[3] << 32) |
             ((uint64_t)buffer[4] << 24) |
             ((uint64_t)buffer[5] << 16) |
             ((uint64_t)buffer[6] << 8) |
                        buffer[7];

    return 1;
}

int mxf_read_int8(MXFFile *mxfFile, int8_t *value)
{
    return mxf_read_uint8(mxfFile, (uint8_t*)value);
}

int mxf_read_int16(MXFFile *mxfFile, int16_t *value)
{
    return mxf_read_uint16(mxfFile, (uint16_t*)value);
}

int mxf_read_int32(MXFFile *mxfFile, int32_t *value)
{
    return mxf_read_uint32(mxfFile, (uint32_t*)value);
}

int mxf_read_int64(MXFFile *mxfFile, int64_t *value)
{
    return mxf_read_uint64(mxfFile, (uint64_t*)value);
}


int mxf_write_uint8(MXFFile *mxfFile, uint8_t value)
{
    CHK_ORET(mxf_file_write(mxfFile, &value, 1) == 1);

    return 1;
}

int mxf_write_uint16(MXFFile *mxfFile, uint16_t value)
{
    uint8_t buffer[2];
    buffer[0] = (uint8_t)((value >> 8) & 0xff);
    buffer[1] = (uint8_t)( value       & 0xff);
    CHK_ORET(mxf_file_write(mxfFile, buffer, 2) == 2);

    return 1;
}

int mxf_write_uint32(MXFFile *mxfFile, uint32_t value)
{
    uint8_t buffer[4];
    buffer[0] = (uint8_t)((value >> 24) & 0xff);
    buffer[1] = (uint8_t)((value >> 16) & 0xff);
    buffer[2] = (uint8_t)((value >> 8)  & 0xff);
    buffer[3] = (uint8_t)( value        & 0xff);
    CHK_ORET(mxf_file_write(mxfFile, buffer, 4) == 4);

    return 1;
}

int mxf_write_uint64(MXFFile *mxfFile, uint64_t value)
{
    uint8_t buffer[8];
    buffer[0] = (uint8_t)((value >> 56) & 0xff);
    buffer[1] = (uint8_t)((value >> 48) & 0xff);
    buffer[2] = (uint8_t)((value >> 40) & 0xff);
    buffer[3] = (uint8_t)((value >> 32) & 0xff);
    buffer[4] = (uint8_t)((value >> 24) & 0xff);
    buffer[5] = (uint8_t)((value >> 16) & 0xff);
    buffer[6] = (uint8_t)((value >> 8)  & 0xff);
    buffer[7] = (uint8_t)( value        & 0xff);
    CHK_ORET(mxf_file_write(mxfFile, buffer, 8) == 8);

    return 1;
}

int mxf_write_int8(MXFFile *mxfFile, int8_t value)
{
    return mxf_write_uint8(mxfFile, *(uint8_t*)&value);
}

int mxf_write_int16(MXFFile *mxfFile, int16_t value)
{
    return mxf_write_uint16(mxfFile, *(uint16_t*)&value);
}

int mxf_write_int32(MXFFile *mxfFile, int32_t value)
{
    return mxf_write_uint32(mxfFile, *(uint32_t*)&value);
}

int mxf_write_int64(MXFFile *mxfFile, int64_t value)
{
    return mxf_write_uint64(mxfFile, *(uint64_t*)&value);
}


int mxf_read_ul(MXFFile *mxfFile, mxfUL *value)
{
    CHK_ORET(mxf_file_read(mxfFile, (uint8_t*)value, 16) == 16);

    return 1;
}

int mxf_read_k(MXFFile *mxfFile, mxfKey *key)
{
    CHK_ORET(mxf_file_read(mxfFile, (uint8_t*)key, 16) == 16);

    return 1;
}

int mxf_read_l(MXFFile *mxfFile, uint8_t *llen, uint64_t *len)
{
    int i;
    int c;
    uint64_t length;
    uint8_t llength;

    CHK_ORET((c = mxf_file_getc(mxfFile)) != EOF);

    length = 0;
    llength = 1;
    if (c < 0x80) {
        length = c;
    } else {
        uint8_t bytesToRead = ((uint8_t)c) & 0x7f;
        CHK_ORET(bytesToRead <= 8);
        for (i = 0; i < bytesToRead; i++) {
            CHK_ORET((c = mxf_file_getc(mxfFile)) != EOF);
            length <<= 8;
            length |= (uint8_t)c;
        }
        llength += bytesToRead;
    }

    *llen = llength;
    *len = length;

    return 1;
}

int mxf_read_kl(MXFFile *mxfFile, mxfKey *key, uint8_t *llen, uint64_t *len)
{
    CHK_ORET(mxf_read_k(mxfFile, key));
    CHK_ORET(mxf_read_l(mxfFile, llen, len));

    return 1;
}


int mxf_read_key(MXFFile *mxfFile, mxfKey *value)
{
    return mxf_read_ul(mxfFile, (mxfUL*)value);
}

int mxf_read_uid(MXFFile *mxfFile, mxfUID *value)
{
    return mxf_read_ul(mxfFile, (mxfUL*)value);
}

int mxf_read_uuid(MXFFile *mxfFile, mxfUUID *value)
{
    return mxf_read_ul(mxfFile, (mxfUL*)value);
}

int mxf_read_local_tag(MXFFile *mxfFile, mxfLocalTag *tag)
{
    return mxf_read_uint16(mxfFile, tag);
}

int mxf_read_local_tl(MXFFile *mxfFile, mxfLocalTag *tag, uint16_t *len)
{
    CHK_ORET(mxf_read_local_tag(mxfFile, tag));
    CHK_ORET(mxf_read_uint16(mxfFile, len));

    return 1;
}

int mxf_skip(MXFFile *mxfFile, uint64_t len)
{
    if (mxf_file_is_seekable(mxfFile)) {
        return mxf_file_seek(mxfFile, len, SEEK_CUR);
    } else {
        /* skip by reading and discarding data */
        uint8_t buffer[SKIP_BUFFER_SIZE];
        uint32_t numRead;
        uint64_t totalRead = 0;
        while (totalRead < len) {
            if (len - totalRead > SKIP_BUFFER_SIZE)
                numRead = SKIP_BUFFER_SIZE;
            else
                numRead = (uint32_t)(len - totalRead);
            if (mxf_file_read(mxfFile, buffer, numRead) != numRead)
                return 0;
            totalRead += numRead;
        }

        return 1;
    }
}


int mxf_write_local_tag(MXFFile *mxfFile, mxfLocalTag tag)
{
    return mxf_write_uint16(mxfFile, tag);
}

int mxf_write_local_tl(MXFFile *mxfFile, mxfLocalTag tag, uint16_t len)
{
    CHK_ORET(mxf_write_local_tag(mxfFile, tag));
    CHK_ORET(mxf_write_uint16(mxfFile, len));

    return 1;
}

int mxf_write_k(MXFFile *mxfFile, const mxfKey *key)
{
    CHK_ORET(mxf_file_write(mxfFile, (const uint8_t*)key, 16) == 16);

    return 1;
}

uint8_t mxf_write_l(MXFFile *mxfFile, uint64_t len)
{
    uint8_t llen = mxf_get_llen(mxfFile, len);

    CHK_ORET(mxf_write_fixed_l(mxfFile, llen, len));

    return llen;
}

int mxf_write_kl(MXFFile *mxfFile, const mxfKey *key, uint64_t len)
{
    CHK_ORET(mxf_write_k(mxfFile, key));
    CHK_ORET(mxf_write_l(mxfFile, len));

    return 1;
}

int mxf_write_fixed_l(MXFFile *mxfFile, uint8_t llen, uint64_t len)
{
    uint8_t buffer[9];
    uint8_t i;

    assert(llen > 0 && llen <= 9);

    if (llen == 1) {
        if (len >= 0x80) {
            mxf_log_error("Could not write BER length %" PRId64 " for llen equal 1" LOG_LOC_FORMAT, len, LOG_LOC_PARAMS);
            return 0;
        }

        if (mxf_file_putc(mxfFile, (int)len) != (int)len)
            return 0;
    } else {
        if (llen != 9 && (len >> ((llen - 1) * 8)) > 0) {
            mxf_log_error("Could not write BER length %" PRIu64 " for llen equal %u"
                          LOG_LOC_FORMAT, len, llen, LOG_LOC_PARAMS);
            return 0;
        }

        for (i = 0; i < llen - 1; i++)
            buffer[llen - 1 - i - 1] = (uint8_t)((len >> (i * 8)) & 0xff);

        CHK_ORET(mxf_file_putc(mxfFile, 0x80 + llen - 1) == 0x80 + llen - 1);
        CHK_ORET(mxf_file_write(mxfFile, buffer, llen - 1) == (uint8_t)(llen - 1));
    }

    return 1;
}

int mxf_write_fixed_kl(MXFFile *mxfFile, const mxfKey *key, uint8_t llen, uint64_t len)
{
    CHK_ORET(mxf_write_k(mxfFile, key));
    CHK_ORET(mxf_write_fixed_l(mxfFile, llen, len));

    return 1;
}

int mxf_write_ul(MXFFile *mxfFile, const mxfUL *label)
{
    return mxf_write_k(mxfFile, (const mxfKey*)label);
}

int mxf_write_uid(MXFFile *mxfFile, const mxfUID *uid)
{
    return mxf_write_k(mxfFile, (const mxfKey*)uid);
}

int mxf_write_uuid(MXFFile *mxfFile, const mxfUUID *uuid)
{
    return mxf_write_k(mxfFile, (const mxfKey*)uuid);
}


uint8_t mxf_get_llen(MXFFile *mxfFile, uint64_t len)
{
    uint8_t llen;

    if (len < 0x80) {
        llen = 1;
    } else {
        if (     (len >> 56) != 0)
            llen = 9;
        else if ((len >> 48) != 0)
            llen = 8;
        else if ((len >> 40) != 0)
            llen = 7;
        else if ((len >> 32) != 0)
            llen = 6;
        else if ((len >> 24) != 0)
            llen = 5;
        else if ((len >> 16) != 0)
            llen = 4;
        else if ((len >>  8) != 0)
            llen = 3;
        else
            llen = 2;
    }

    if (mxfFile && llen < mxfFile->minLLen)
        llen = mxfFile->minLLen;

    return llen;
}


int mxf_read_batch_header(MXFFile *mxfFile, uint32_t *len, uint32_t *eleLen)
{
    CHK_ORET(mxf_read_uint32(mxfFile, len));
    CHK_ORET(mxf_read_uint32(mxfFile, eleLen));

    return 1;
}

int mxf_write_batch_header(MXFFile *mxfFile, uint32_t len, uint32_t eleLen)
{
    CHK_ORET(mxf_write_uint32(mxfFile, len));
    CHK_ORET(mxf_write_uint32(mxfFile, eleLen));

    return 1;
}

int mxf_read_array_header(MXFFile *mxfFile, uint32_t *len, uint32_t *eleLen)
{
    return mxf_read_batch_header(mxfFile, len, eleLen);
}

int mxf_write_array_header(MXFFile *mxfFile, uint32_t len, uint32_t eleLen)
{
    return mxf_write_batch_header(mxfFile, len, eleLen);
}


int mxf_write_zeros(MXFFile *mxfFile, uint64_t len)
{
    uint64_t completeCount;
    uint32_t partialCount;
    uint64_t i;

    if (len == 0)
        return 1;

    if (mxfFile->zerosBufferSize < len &&
        mxfFile->zerosBufferSize < MAX_ZEROS_BUFFER_SIZE)
    {
        uint8_t *newBuffer;
        uint32_t newSize;
        if (len >= MAX_ZEROS_BUFFER_SIZE)
            newSize = MAX_ZEROS_BUFFER_SIZE;
        else
            newSize = (uint32_t)(len + (ZEROS_BUFFER_INCREMENT - (len % ZEROS_BUFFER_INCREMENT)));

        CHK_ORET((newBuffer = realloc(mxfFile->zerosBuffer, newSize)) != NULL);
        memset(newBuffer + mxfFile->zerosBufferSize, 0, newSize - mxfFile->zerosBufferSize);

        mxfFile->zerosBuffer = newBuffer;
        mxfFile->zerosBufferSize = newSize;
    }

    completeCount = len / mxfFile->zerosBufferSize;
    partialCount = (uint32_t)(len % mxfFile->zerosBufferSize);

    for (i = 0; i < completeCount; i++)
        CHK_ORET(mxf_file_write(mxfFile, mxfFile->zerosBuffer, mxfFile->zerosBufferSize) == mxfFile->zerosBufferSize);

    if (partialCount > 0)
        CHK_ORET(mxf_file_write(mxfFile, mxfFile->zerosBuffer, partialCount) == partialCount);

    return 1;
}


void mxf_set_runin_len(MXFFile *mxfFile, uint16_t runinLen)
{
    mxfFile->runinLen = runinLen;
}

uint16_t mxf_get_runin_len(MXFFile *mxfFile)
{
    return mxfFile->runinLen;
}

