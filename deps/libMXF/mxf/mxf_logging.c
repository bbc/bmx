/*
 * libMXF logging functions
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

#include <time.h>


#include <mxf/mxf_logging.h>


mxf_vlog_func mxf_vlog       = mxf_vlog_default;
mxf_log_func  mxf_log        = mxf_log_default;
MXFLogLevel   g_mxfLogLevel  = MXF_DLOG;


static FILE *g_mxfFileLog = NULL;

static void logmsg(FILE *file, MXFLogLevel level, const char *format, va_list p_arg)
{
    switch (level)
    {
        case MXF_DLOG:
            fprintf(file, "Debug: ");
            break;
        case MXF_ILOG:
            fprintf(file, "Info: ");
            break;
        case MXF_WLOG:
            fprintf(file, "Warning: ");
            break;
        case MXF_ELOG:
            fprintf(file, "ERROR: ");
            break;
    };

    vfprintf(file, format, p_arg);
}

static void vlog_to_file(MXFLogLevel level, const char *format, va_list p_arg)
{
    char timeStr[128];
    const time_t t = time(NULL);
    const struct tm *gmt = gmtime(&t);

    if (level < g_mxfLogLevel)
    {
        return;
    }

    assert(gmt != NULL);
    assert(g_mxfFileLog != NULL);
    if (g_mxfFileLog == NULL || gmt == NULL)
    {
        return;
    }

    strftime(timeStr, 128, "%Y-%m-%d %H:%M:%S", gmt);
    fprintf(g_mxfFileLog, "(%s) ", timeStr);

    logmsg(g_mxfFileLog, level, format, p_arg);
}

static void log_to_file(MXFLogLevel level, const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    vlog_to_file(level, format, p_arg);
    va_end(p_arg);
}



void mxf_vlog_default(MXFLogLevel level, const char *format, va_list p_arg)
{
    if (level < g_mxfLogLevel)
    {
        return;
    }

    if (level == MXF_ELOG)
    {
        logmsg(stderr, level, format, p_arg);
    }
    else
    {
        logmsg(stdout, level, format, p_arg);
    }
}

void mxf_log_default(MXFLogLevel level, const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    mxf_vlog_default(level, format, p_arg);
    va_end(p_arg);
}

int mxf_log_file_open(const char *filename)
{
    if ((g_mxfFileLog = fopen(filename, "wb")) == NULL)
    {
        return 0;
    }

    mxf_vlog = vlog_to_file;
    mxf_log = log_to_file;
    return 1;
}

void mxf_log_file_flush(void)
{
    if (g_mxfFileLog != NULL)
    {
        fflush(g_mxfFileLog);
    }
}

void mxf_log_file_close(void)
{
    if (g_mxfFileLog != NULL)
    {
        fclose(g_mxfFileLog);
        g_mxfFileLog = NULL;
    }
}


void mxf_log_debug(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    mxf_vlog(MXF_DLOG, format, p_arg);
    va_end(p_arg);
}

void mxf_log_info(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    mxf_vlog(MXF_ILOG, format, p_arg);
    va_end(p_arg);
}

void mxf_log_warn(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    mxf_vlog(MXF_WLOG, format, p_arg);
    va_end(p_arg);
}

void mxf_log_error(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    mxf_vlog(MXF_ELOG, format, p_arg);
    va_end(p_arg);
}

