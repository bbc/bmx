/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <mxf/mxf.h>

#include <bmx/MXFChecksumFile.h>
#include <bmx/Logging.h>
#include <bmx/BMXException.h>


using namespace std;
using namespace bmx;


struct bmx::MXFChecksumFile
{
    MXFFile *mxf_file;
};

struct MXFFileSysData
{
    MXFChecksumFile checksum_file;
    MXFFile *target;
    Checksum *checksum;
    int64_t position;
    int64_t checksum_position;
    bool force_update;
    bool checksum_final;
};


static bool update_checksum_to_position(MXFChecksumFile *checksum_file, int64_t position)
{
    MXFFile *mxf_file = checksum_file->mxf_file;
    MXFFileSysData *sys_data = mxf_file->sysData;

    if (sys_data->checksum_final || sys_data->checksum_position >= position)
        return true;

    // set force update to false to avoid endless update loop
    bool original_force_update = sys_data->force_update;
    sys_data->force_update = false;

    bool result = false;
    unsigned char *buffer = 0;
    try
    {
        if (!mxf_file_seek(mxf_file, sys_data->checksum_position, SEEK_SET))
            throw false;

        const int buffer_size = 8192;
        buffer = new unsigned char[buffer_size];
        uint32_t num_read;
        while (sys_data->checksum_position < position) {
            num_read = buffer_size;
            if (sys_data->checksum_position + num_read > position)
                num_read = (uint32_t)(position - sys_data->checksum_position);
            if (mxf_file_read(mxf_file, buffer, num_read) != num_read)
                throw false;
        }
        delete [] buffer;
        buffer = 0;

        result = true;
    }
    catch (...)
    {
        delete [] buffer;
        result = false;
    }

    sys_data->force_update = original_force_update;

    return result;
}

static void update_checksum_to_nonseekable_end(MXFChecksumFile *checksum_file)
{
    MXFFile *mxf_file = checksum_file->mxf_file;
    MXFFileSysData *sys_data = mxf_file->sysData;

    if (sys_data->checksum_final)
        return;

    // set force update to false to avoid endless update loop
    bool original_force_update = sys_data->force_update;
    sys_data->force_update = false;

    const uint32_t buffer_size = 8192;
    unsigned char *buffer = new unsigned char[buffer_size];
    while (mxf_file_read(mxf_file, buffer, buffer_size) == buffer_size)
    {}
    // TODO: ferror/errno needs to filter up to here so that error conditions aren't ignored
    delete [] buffer;

    sys_data->force_update = original_force_update;
}


static void checksum_file_close(MXFFileSysData *sys_data)
{
    if (sys_data->target)
        mxf_file_close(&sys_data->target);
}

static uint32_t checksum_file_read(MXFFileSysData *sys_data, uint8_t *data, uint32_t count)
{
    if (sys_data->force_update &&
        !update_checksum_to_position(&sys_data->checksum_file, sys_data->position))
    {
        return 0;
    }

    uint32_t result = mxf_file_read(sys_data->target, data, count);

    if (result > 0) {
        if (!sys_data->checksum_final &&
            sys_data->position          <= sys_data->checksum_position &&
            sys_data->position + result >  sys_data->checksum_position)
        {
            uint32_t checksum_count = (uint32_t)(sys_data->position + result - sys_data->checksum_position);
            sys_data->checksum->Update(&data[(uint32_t)(sys_data->checksum_position - sys_data->position)],
                                       checksum_count);
            sys_data->checksum_position += checksum_count;
        }
        sys_data->position += result;
    }

    return result;
}

static uint32_t checksum_file_write(MXFFileSysData *sys_data, const uint8_t *data, uint32_t count)
{
    BMX_CHECK_M(sys_data->position == sys_data->checksum_position,
                ("File modification not supported when using the MXF checksum file"));

    uint32_t result = mxf_file_write(sys_data->target, data, count);

    if (result > 0) {
        // sys_data->position == sys_data->checksum_position
        if (!sys_data->checksum_final) {
            sys_data->checksum->Update(data, result);
            sys_data->checksum_position += result;
        }
        sys_data->position += result;
    }

    return result;
}

static int checksum_file_getc(MXFFileSysData *sys_data)
{
    if (sys_data->force_update &&
        !update_checksum_to_position(&sys_data->checksum_file, sys_data->position))
    {
        return EOF;
    }

    int result = mxf_file_getc(sys_data->target);

    if (result != EOF) {
        if (!sys_data->checksum_final && sys_data->position == sys_data->checksum_position) {
            unsigned char byte = (unsigned char)result;
            sys_data->checksum->Update(&byte, 1);
            sys_data->checksum_position++;
        }
        sys_data->position++;
    }

    return result;
}

static int checksum_file_putc(MXFFileSysData *sys_data, int c)
{
    BMX_CHECK_M(sys_data->position == sys_data->checksum_position,
                ("File modification not supported when using the MXF Checksum file"));

    int result = mxf_file_putc(sys_data->target, c);

    if (result != EOF) {
        // sys_data->position == sys_data->checksum_position
        if (!sys_data->checksum_final) {
            unsigned char byte = (unsigned char)c;
            sys_data->checksum->Update(&byte, 1);
            sys_data->checksum_position++;
        }
        sys_data->position++;
    }

    return result;
}

static int checksum_file_eof(MXFFileSysData *sys_data)
{
    return mxf_file_eof(sys_data->target);
}

static int checksum_file_seek(MXFFileSysData *sys_data, int64_t offset, int whence)
{
    // if possible, seek using the checksum update if forced to update
    if (sys_data->force_update) {
        if (whence == SEEK_SET && offset > sys_data->checksum_position)
            return update_checksum_to_position(&sys_data->checksum_file, offset);
        else if (whence == SEEK_CUR && sys_data->position + offset > sys_data->checksum_position)
            return update_checksum_to_position(&sys_data->checksum_file, sys_data->position + offset);
    }

    int result = mxf_file_seek(sys_data->target, offset, whence);

    if (result) {
        switch (whence)
        {
            case SEEK_SET:
                sys_data->position = offset;
                break;
            case SEEK_CUR:
                sys_data->position += offset;
                break;
            case SEEK_END:
            default:
                // call file_tell because a growing file could result in file_size being wrong at this point
                sys_data->position = mxf_file_tell(sys_data->target);
                break;
        }

        if (sys_data->force_update &&
            !update_checksum_to_position(&sys_data->checksum_file, sys_data->position))
        {
            return 0;
        }
    }

    return result;
}

static int64_t checksum_file_tell(MXFFileSysData *sys_data)
{
    int64_t result = mxf_file_tell(sys_data->target);

    // check still in sync
    if (result != sys_data->position)
        return -1;

    return result;
}

static int checksum_file_is_seekable(MXFFileSysData *sys_data)
{
    return mxf_file_is_seekable(sys_data->target);
}

static int64_t checksum_file_size(MXFFileSysData *sys_data)
{
    return mxf_file_size(sys_data->target);
}


static void free_checksum_file(MXFFileSysData *sys_data)
{
    if (sys_data) {
        delete sys_data->checksum;
        free(sys_data);
    }
}


MXFChecksumFile* bmx::mxf_checksum_file_open(MXFFile *target, ChecksumType type)
{
    MXFFile *checksum_file = 0;
    try
    {
        // using malloc() because mxf_file_close will call free()
        BMX_CHECK((checksum_file = (MXFFile*)malloc(sizeof(MXFFile))) != 0);
        memset(checksum_file, 0, sizeof(MXFFile));
        BMX_CHECK((checksum_file->sysData = (MXFFileSysData*)malloc(sizeof(MXFFileSysData))) != 0);
        memset(checksum_file->sysData, 0, sizeof(MXFFileSysData));

        checksum_file->sysData->target            = target;
        checksum_file->sysData->checksum          = new Checksum(type);
        checksum_file->sysData->position          = mxf_file_tell(target);
        checksum_file->sysData->checksum_position = 0;
        checksum_file->sysData->force_update      = false;
        checksum_file->sysData->checksum_final    = false;

        checksum_file->sysData->checksum_file.mxf_file = checksum_file;

        checksum_file->close         = checksum_file_close;
        checksum_file->read          = checksum_file_read;
        checksum_file->write         = checksum_file_write;
        checksum_file->get_char      = checksum_file_getc;
        checksum_file->put_char      = checksum_file_putc;
        checksum_file->eof           = checksum_file_eof;
        checksum_file->seek          = checksum_file_seek;
        checksum_file->tell          = checksum_file_tell;
        checksum_file->is_seekable   = checksum_file_is_seekable;
        checksum_file->size          = checksum_file_size;
        checksum_file->free_sys_data = free_checksum_file;

        checksum_file->minLLen       = target->minLLen;
        checksum_file->runinLen      = target->runinLen;

        return &checksum_file->sysData->checksum_file;
    }
    catch (...)
    {
        if (checksum_file) {
            if (checksum_file->sysData)
                checksum_file->sysData->target = 0; // ownership returns to the caller
            mxf_file_close(&checksum_file);
        }
        throw;
    }
}

MXFFile* bmx::mxf_checksum_file_get_file(MXFChecksumFile *checksum_file)
{
    return checksum_file->mxf_file;
}

void bmx::mxf_checksum_file_force_update(MXFChecksumFile *checksum_file)
{
    checksum_file->mxf_file->sysData->force_update = true;
}

bool bmx::mxf_checksum_file_final(MXFChecksumFile *checksum_file)
{
    MXFFile *mxf_file = checksum_file->mxf_file;
    MXFFileSysData *sys_data = mxf_file->sysData;

    if (sys_data->checksum_final)
        return true;

    if (mxf_file_is_seekable(mxf_file)) {
        int64_t file_pos = mxf_file_tell(mxf_file);
        int64_t file_size = mxf_file_size(mxf_file);
        if (!update_checksum_to_position(checksum_file, file_size))
            return false;
        if (!mxf_file_seek(mxf_file, file_pos, SEEK_SET))
            return false;
    } else {
        update_checksum_to_nonseekable_end(checksum_file);
    }

    sys_data->checksum->Final();
    sys_data->checksum_final = true;

    return true;
}

size_t bmx::mxf_checksum_file_digest_size(const MXFChecksumFile *checksum_file)
{
    return checksum_file->mxf_file->sysData->checksum->GetDigestSize();
}

void bmx::mxf_checksum_file_digest(const MXFChecksumFile *checksum_file, unsigned char *digest, size_t size)
{
    return checksum_file->mxf_file->sysData->checksum->GetDigest(digest, size);
}

string bmx::mxf_checksum_file_digest_str(const MXFChecksumFile *checksum_file)
{
    return checksum_file->mxf_file->sysData->checksum->GetDigestString();
}
