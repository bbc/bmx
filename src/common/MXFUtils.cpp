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

#include <mxf/mxf.h>

#include <im/MXFUtils.h>
#include <im/Logging.h>


using namespace std;
using namespace im;



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



void im::connect_libmxf_logging()
{
    g_mxfLogLevel = (MXFLogLevel)LOG_LEVEL;
    mxf_vlog = connect_mxf_vlog;
    mxf_log = connect_mxf_log;
}

int64_t im::convert_tc_offset(mxfRational in_edit_rate, int64_t in_offset, uint16_t out_tc_base)
{
    return convert_position(in_offset, out_tc_base, get_rounded_tc_base(in_edit_rate), ROUND_AUTO);
}

string im::get_track_name(bool is_video, uint32_t track_number)
{
    char buffer[32];
    sprintf(buffer, "%s%d", (is_video ? "V" : "A"), track_number);
    return buffer;
}

