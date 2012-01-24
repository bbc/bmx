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


static void md5_mxf_file_close(MXFFileSysData *sys_data)
{
    if (sys_data->target)
        mxf_file_close(&sys_data->target);
}

static uint32_t md5_mxf_file_read(MXFFileSysData *sys_data, uint8_t *data, uint32_t count)
{
    (void)sys_data;
    (void)data;
    (void)count;

    return 0;
}

static uint32_t md5_mxf_file_write(MXFFileSysData *sys_data, const uint8_t *data, uint32_t count)
{
    uint32_t result = mxf_file_write(sys_data->target, data, count);

    if (result > 0)
        md5_update(&sys_data->md5_context, data, count);

    return result;
}

static int md5_mxf_file_getc(MXFFileSysData *sys_data)
{
    (void)sys_data;

    return EOF;
}

static int md5_mxf_file_putc(MXFFileSysData *sys_data, int c)
{
    int result = mxf_file_putc(sys_data->target, c);

    if (result != EOF) {
        unsigned char byte = (unsigned char)c;
        md5_update(&sys_data->md5_context, &byte, 1);
    }

    return result;
}

static int md5_mxf_file_eof(MXFFileSysData *sys_data)
{
    return mxf_file_eof(sys_data->target);
}

static int md5_mxf_file_seek(MXFFileSysData *sys_data, int64_t offset, int whence)
{
    (void)sys_data;
    (void)offset;
    (void)whence;

    return 0;
}

static int64_t md5_mxf_file_tell(MXFFileSysData *sys_data)
{
    return mxf_file_tell(sys_data->target);
}

static int md5_mxf_file_is_seekable(MXFFileSysData *sys_data)
{
    (void)sys_data;

    return 0;
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

MXFMD5WrapperFile* bmx::md5_wrap_mxf_file(MXFFile *target)
{
    MXFFile *md5_mxf_file = 0;
    try
    {
        // using malloc() because mxf_file_close will call free()
        BMX_CHECK((md5_mxf_file = (MXFFile*)malloc(sizeof(MXFFile))) != 0);
        BMX_CHECK((md5_mxf_file->sysData = (MXFFileSysData*)malloc(sizeof(MXFFileSysData))) != 0);

        md5_mxf_file->sysData->target = target;
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

        md5_mxf_file->minLLen = target->minLLen;

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

void bmx::md5_wrap_finalize(MXFMD5WrapperFile *md5_wrapper_file, unsigned char digest[16])
{
    md5_final(digest, &md5_wrapper_file->mxf_file->sysData->md5_context);
}

