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

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <ctime>
#include <cerrno>

#include <bmx/Logging.h>
#include <bmx/Utils.h>

using namespace std;
using namespace bmx;



static void stdio_vlog2(LogLevel level, const char *source, const char *format, va_list p_arg);
static void stdio_vlog(LogLevel level, const char *format, va_list p_arg);
static void stdio_log(LogLevel level, const char *format, ...);

log_func bmx::log = stdio_log;
vlog_func bmx::vlog = stdio_vlog;
vlog2_func bmx::vlog2 = stdio_vlog2;
LogLevel bmx::LOG_LEVEL = INFO_LOG;

static FILE *LOG_FILE = 0;



static void log_message(FILE *file, LogLevel level, const char *source, const char *format, va_list p_arg)
{
    switch (level)
    {
        case DEBUG_LOG:
            fprintf(file, "Debug");
            break;
        case INFO_LOG:
            fprintf(file, "Info");
            break;
        case WARN_LOG:
            fprintf(file, "Warning");
            break;
        case ERROR_LOG:
            fprintf(file, "ERROR");
            break;
    }

    if (source)
        fprintf(file, " (%s): ", source);
    else
        fprintf(file, ": ");

    vfprintf(file, format, p_arg);
}

static void stdio_vlog2(LogLevel level, const char *source, const char *format, va_list p_arg)
{
    if (level < LOG_LEVEL)
        return;

    if (level == ERROR_LOG)
        log_message(stderr, level, source, format, p_arg);
    else
        log_message(stdout, level, source, format, p_arg);
}

static void stdio_vlog(LogLevel level, const char *format, va_list p_arg)
{
    stdio_vlog2(level, 0, format, p_arg);
}

static void stdio_log(LogLevel level, const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    stdio_vlog2(level, 0, format, p_arg);
    va_end(p_arg);
}

static void file_vlog2(LogLevel level, const char *source, const char *format, va_list p_arg)
{
    char time_str[128];
    const time_t t = time(0);
    const struct tm *gmt = gmtime(&t);

    if (level < LOG_LEVEL || !LOG_FILE)
        return;

    if (LOG_FILE != stderr && LOG_FILE != stdout) {
        if (gmt) {
            strftime(time_str, 128, "%Y-%m-%d %H:%M:%S", gmt);
            fprintf(LOG_FILE, "(%s) ", time_str);
        } else {
            fprintf(LOG_FILE, "(?) ");
        }
    }

    log_message(LOG_FILE, level, source, format, p_arg);
}

static void file_vlog(LogLevel level, const char *format, va_list p_arg)
{
    file_vlog2(level, 0, format, p_arg);
}

static void file_log(LogLevel level, const char* format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    file_vlog2(level, 0, format, p_arg);
    va_end(p_arg);
}



bool bmx::open_log_file(string filename)
{
    close_log_file();

    LOG_FILE = fopen(filename.c_str(), "wb");
    if (!LOG_FILE) {
        fprintf(stderr, "Failed to open log file '%s': %s\n", filename.c_str(), bmx_strerror(errno).c_str());
        return false;
    }

    vlog2 = file_vlog2;
    vlog = file_vlog;
    log = file_log;
    return true;
}

void bmx::close_log_file()
{
    if (LOG_FILE) {
        if (LOG_FILE != stdout && LOG_FILE != stderr)
            fclose(LOG_FILE);
        LOG_FILE = 0;
    }
    vlog2 = stdio_vlog2;
    vlog = stdio_vlog;
    log = stdio_log;
}

void bmx::set_stdout_log_file()
{
    close_log_file();
    LOG_FILE = stdout;
    vlog2 = file_vlog2;
    vlog = file_vlog;
    log = file_log;
}

void bmx::set_stderr_log_file()
{
    close_log_file();
    LOG_FILE = stderr;
    vlog2 = file_vlog2;
    vlog = file_vlog;
    log = file_log;
}

void bmx::set_stdio_log_file()
{
    close_log_file();
    vlog2 = stdio_vlog2;
    vlog = stdio_vlog;
    log = stdio_log;
}

void bmx::flush_log()
{
    if (LOG_FILE) {
        fflush(LOG_FILE);
    } else {
        fflush(stderr);
        fflush(stdout);
    }
}

void bmx::log_debug(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    vlog2(DEBUG_LOG, 0, format, p_arg);
    va_end(p_arg);
}

void bmx::log_info(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    vlog2(INFO_LOG, 0, format, p_arg);
    va_end(p_arg);
}

void bmx::log_warn(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    vlog2(WARN_LOG, 0, format, p_arg);
    va_end(p_arg);
}

void bmx::log_error(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    vlog2(ERROR_LOG, 0, format, p_arg);
    va_end(p_arg);
}

void bmx::log_error_nl(const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    vlog2(ERROR_LOG, 0, format, p_arg);
    va_end(p_arg);

    // add newline
    if (LOG_FILE)
        fprintf(LOG_FILE, "\n");
    else
        fprintf(stderr, "\n");
}
