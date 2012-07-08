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

#include <bmx/MD5.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Logging.h>
#include <bmx/BMXException.h>


using namespace std;
using namespace bmx;


struct bmx::MXFMD5WrapperFile
{
    MXFFile *mxf_file;
};

struct MXFFileSysData
{
    MXFMD5WrapperFile md5_wrapper_file;
    MXFFile *target;
    MD5Context md5_context;
    int64_t position;
    int64_t md5_position;
    bool force_update;
};



static void connect_mxf_vlog(MXFLogLevel level, const char *format, va_list p_arg)
{
    vlog2((LogLevel)level, "libMXF", format, p_arg);
}

static void connect_mxf_log(MXFLogLevel level, const char *format, ...)
{
    va_list p_arg;
    va_start(p_arg, format);
    vlog2((LogLevel)level, "libMXF", format, p_arg);
    va_end(p_arg);
}


static bool update_md5_to_position(MXFMD5WrapperFile *md5_wrapper_file, int64_t position)
{
    MXFFile *mxf_file = md5_wrapper_file->mxf_file;
    MXFFileSysData *sys_data = mxf_file->sysData;

    if (sys_data->md5_position >= position)
        return true;

    // set force update to false to avoid endless update loop
    bool original_force_update = sys_data->force_update;
    sys_data->force_update = false;

    bool result = false;
    unsigned char *buffer = 0;
    try
    {
        if (!mxf_file_seek(mxf_file, sys_data->md5_position, SEEK_SET))
            throw false;

        const int buffer_size = 8192;
        buffer = new unsigned char[buffer_size];
        uint32_t num_read;
        while (sys_data->md5_position < position) {
            num_read = buffer_size;
            if (sys_data->md5_position + num_read > position)
                num_read = (uint32_t)(position - sys_data->md5_position);
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


static void md5_mxf_file_close(MXFFileSysData *sys_data)
{
    if (sys_data->target)
        mxf_file_close(&sys_data->target);
}

static uint32_t md5_mxf_file_read(MXFFileSysData *sys_data, uint8_t *data, uint32_t count)
{
    if (sys_data->force_update &&
        !update_md5_to_position(&sys_data->md5_wrapper_file, sys_data->position))
    {
        return 0;
    }

    uint32_t result = mxf_file_read(sys_data->target, data, count);

    if (result > 0) {
        if (sys_data->position          <= sys_data->md5_position &&
            sys_data->position + result >  sys_data->md5_position)
        {
            uint32_t md5_count = (uint32_t)(sys_data->position + result - sys_data->md5_position);
            md5_update(&sys_data->md5_context,
                       &data[(uint32_t)(sys_data->md5_position - sys_data->position)],
                       md5_count);
            sys_data->md5_position += md5_count;
        }
        sys_data->position += result;
    }

    return result;
}

static uint32_t md5_mxf_file_write(MXFFileSysData *sys_data, const uint8_t *data, uint32_t count)
{
    BMX_CHECK_M(sys_data->position == sys_data->md5_position,
                ("File modification not supported when using the MD5 MXF file"));

    uint32_t result = mxf_file_write(sys_data->target, data, count);

    if (result > 0) {
        md5_update(&sys_data->md5_context, data, result);
        sys_data->md5_position += result;
        sys_data->position += result;
    }

    return result;
}

static int md5_mxf_file_getc(MXFFileSysData *sys_data)
{
    if (sys_data->force_update &&
        !update_md5_to_position(&sys_data->md5_wrapper_file, sys_data->position))
    {
        return EOF;
    }

    int result = mxf_file_getc(sys_data->target);

    if (result != EOF) {
        if (sys_data->position == sys_data->md5_position) {
            unsigned char byte = (unsigned char)result;
            md5_update(&sys_data->md5_context, &byte, 1);
            sys_data->md5_position++;
        }
        sys_data->position++;
    }

    return result;
}

static int md5_mxf_file_putc(MXFFileSysData *sys_data, int c)
{
    BMX_CHECK_M(sys_data->position == sys_data->md5_position,
                ("File modification not supported when using the MD5 MXF file"));

    int result = mxf_file_putc(sys_data->target, c);

    if (result != EOF) {
        unsigned char byte = (unsigned char)c;
        md5_update(&sys_data->md5_context, &byte, 1);
        sys_data->md5_position++;
        sys_data->position++;
    }

    return result;
}

static int md5_mxf_file_eof(MXFFileSysData *sys_data)
{
    return mxf_file_eof(sys_data->target);
}

static int md5_mxf_file_seek(MXFFileSysData *sys_data, int64_t offset, int whence)
{
    // if possible, seek using the md5 update if forced to update
    if (sys_data->force_update) {
        if (whence == SEEK_SET && offset > sys_data->md5_position)
            return update_md5_to_position(&sys_data->md5_wrapper_file, offset);
        else if (whence == SEEK_CUR && sys_data->position + offset > sys_data->md5_position)
            return update_md5_to_position(&sys_data->md5_wrapper_file, sys_data->position + offset);
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
            !update_md5_to_position(&sys_data->md5_wrapper_file, sys_data->position))
        {
            return 0;
        }
    }

    return result;
}

static int64_t md5_mxf_file_tell(MXFFileSysData *sys_data)
{
    int64_t result = mxf_file_tell(sys_data->target);

    // check still in sync
    if (result != sys_data->position)
        return -1;

    return result;
}

static int md5_mxf_file_is_seekable(MXFFileSysData *sys_data)
{
    return mxf_file_is_seekable(sys_data->target);
}

static int64_t md5_mxf_file_size(MXFFileSysData *sys_data)
{
    return mxf_file_size(sys_data->target);
}


static void free_md5_mxf_file(MXFFileSysData *sys_data)
{
    free(sys_data);
}



void bmx::connect_libmxf_logging()
{
    g_mxfLogLevel = (MXFLogLevel)LOG_LEVEL;
    mxf_vlog = connect_mxf_vlog;
    mxf_log = connect_mxf_log;
}

int64_t bmx::convert_tc_offset(mxfRational in_edit_rate, int64_t in_offset, uint16_t out_tc_base)
{
    return convert_position(in_offset, out_tc_base, get_rounded_tc_base(in_edit_rate), ROUND_AUTO);
}

string bmx::get_track_name(bool is_video, uint32_t track_number)
{
    char buffer[32];
    sprintf(buffer, "%s%d", (is_video ? "V" : "A"), track_number);
    return buffer;
}

void bmx::decode_afd(uint8_t afd, uint16_t mxf_version, uint8_t *code, Rational *aspect_ratio)
{
    if (mxf_version < MXF_PREFACE_VER(1, 3) && !(afd & 0x20) && !(afd & 0x40)) {
        *code = afd & 0x0f;
        if (aspect_ratio)
            *aspect_ratio = ZERO_RATIONAL;
    } else {
        *code = (afd >> 3) & 0x0f;
        if (aspect_ratio) {
            if (afd & 0x04)
                *aspect_ratio = ASPECT_RATIO_16_9;
            else
                *aspect_ratio = ASPECT_RATIO_4_3;
        }
    }
}

uint8_t bmx::encode_afd(uint8_t code, Rational aspect_ratio)
{
    if (aspect_ratio.numerator == 0)
        return code & 0x0f;
    else
        return ((code & 0x0f) << 3) | ((aspect_ratio == ASPECT_RATIO_16_9) << 2);
}

string bmx::convert_utf16_string(const mxfUTF16Char *utf16_str)
{
    size_t utf8_size = mxf_utf16_to_utf8(0, utf16_str, 0);
    if (utf8_size == (size_t)(-1))
        return "";
    utf8_size += 1;
    char *utf8_str = new char[utf8_size];
    mxf_utf16_to_utf8(utf8_str, utf16_str, utf8_size);

    string result = utf8_str;
    delete [] utf8_str;

    return result;
}

string bmx::convert_utf16_string(const unsigned char *utf16_str_in, uint16_t size_in)
{
    uint16_t utf16_size = mxf_get_utf16string_size(utf16_str_in, size_in);
    mxfUTF16Char *utf16_str = new mxfUTF16Char[utf16_size];
    mxf_get_utf16string(utf16_str_in, size_in, utf16_str);

    string result = convert_utf16_string(utf16_str);
    delete [] utf16_str;

    return result;
}

MXFMD5WrapperFile* bmx::md5_wrap_mxf_file(MXFFile *target)
{
    MXFFile *md5_mxf_file = 0;
    try
    {
        // using malloc() because mxf_file_close will call free()
        BMX_CHECK((md5_mxf_file = (MXFFile*)malloc(sizeof(MXFFile))) != 0);
        memset(md5_mxf_file, 0, sizeof(MXFFile));
        BMX_CHECK((md5_mxf_file->sysData = (MXFFileSysData*)malloc(sizeof(MXFFileSysData))) != 0);
        memset(md5_mxf_file->sysData, 0, sizeof(MXFFileSysData));

        md5_mxf_file->sysData->target       = target;
        md5_mxf_file->sysData->position     = mxf_file_tell(target);
        md5_mxf_file->sysData->md5_position = 0;
        md5_mxf_file->sysData->force_update = false;
        md5_init(&md5_mxf_file->sysData->md5_context);

        md5_mxf_file->sysData->md5_wrapper_file.mxf_file = md5_mxf_file;

        md5_mxf_file->close         = md5_mxf_file_close;
        md5_mxf_file->read          = md5_mxf_file_read;
        md5_mxf_file->write         = md5_mxf_file_write;
        md5_mxf_file->get_char      = md5_mxf_file_getc;
        md5_mxf_file->put_char      = md5_mxf_file_putc;
        md5_mxf_file->eof           = md5_mxf_file_eof;
        md5_mxf_file->seek          = md5_mxf_file_seek;
        md5_mxf_file->tell          = md5_mxf_file_tell;
        md5_mxf_file->is_seekable   = md5_mxf_file_is_seekable;
        md5_mxf_file->size          = md5_mxf_file_size;
        md5_mxf_file->free_sys_data = free_md5_mxf_file;

        md5_mxf_file->minLLen       = target->minLLen;
        md5_mxf_file->runinLen      = target->runinLen;

        return &md5_mxf_file->sysData->md5_wrapper_file;
    }
    catch (...)
    {
        if (md5_mxf_file) {
            if (md5_mxf_file->sysData)
                md5_mxf_file->sysData->target = 0; // ownership returns to the caller
            mxf_file_close(&md5_mxf_file);
        }
        throw;
    }
}

MXFFile* bmx::md5_wrap_get_file(MXFMD5WrapperFile *md5_wrapper_file)
{
    return md5_wrapper_file->mxf_file;
}

void bmx::md5_wrap_force_update(MXFMD5WrapperFile *md5_wrapper_file)
{
    md5_wrapper_file->mxf_file->sysData->force_update = true;
}

bool bmx::md5_wrap_finalize(MXFMD5WrapperFile *md5_wrapper_file, unsigned char digest[16])
{
    MXFFile *mxf_file = md5_wrapper_file->mxf_file;
    MXFFileSysData *sys_data = mxf_file->sysData;

    int64_t file_size = mxf_file_size(mxf_file);
    if (!update_md5_to_position(md5_wrapper_file, file_size))
        return false;

    md5_final(digest, &sys_data->md5_context);

    return true;
}

