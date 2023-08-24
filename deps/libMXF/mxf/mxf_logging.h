/*
 * libMXF logging functions
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

#ifndef MXF_LOGGING_H_
#define MXF_LOGGING_H_


#include <stdarg.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef enum
{
    MXF_DLOG = 0,
    MXF_ILOG,
    MXF_WLOG,
    MXF_ELOG
} MXFLogLevel;


typedef void (*mxf_vlog_func) (MXFLogLevel level, const char *format, va_list p_arg);
typedef void (*mxf_log_func) (MXFLogLevel level, const char *format, ...);

/* is set by default to the mxf_log_default function */
extern mxf_vlog_func mxf_vlog;
extern mxf_log_func mxf_log;
extern MXFLogLevel g_mxfLogLevel;


/* outputs to stderr for MXF_ELOG or stdout for the other levels */
void mxf_vlog_default(MXFLogLevel level, const char *format, va_list p_arg);
void mxf_log_default(MXFLogLevel level, const char *format, ...);


/* sets mxf_log to log to the file 'filename' */
int mxf_log_file_open(const char *filename);

void mxf_log_file_flush(void);
void mxf_log_file_close(void);


/* log level in function name */
void mxf_log_debug(const char *format, ...);
void mxf_log_info(const char *format, ...);
void mxf_log_warn(const char *format, ...);
void mxf_log_error(const char *format, ...);



#ifdef __cplusplus
}
#endif


#endif



