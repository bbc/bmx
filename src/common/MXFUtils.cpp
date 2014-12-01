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

#include <mxf/mxf.h>

#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Logging.h>
#include <bmx/BMXException.h>


using namespace std;
using namespace bmx;


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

string bmx::get_track_name(MXFDataDefEnum data_def, uint32_t track_number)
{
    const char *ddef_letter = "X";
    switch (data_def)
    {
        case MXF_PICTURE_DDEF:  ddef_letter = "V"; break;
        case MXF_SOUND_DDEF:    ddef_letter = "A"; break;
        case MXF_DATA_DDEF:     ddef_letter = "D"; break;
        case MXF_TIMECODE_DDEF: ddef_letter = "T"; break;
        case MXF_DM_DDEF:       ddef_letter = "M"; break;
        case MXF_UNKNOWN_DDEF:  ddef_letter = "X"; break;
    }

    char buffer[16];
    bmx_snprintf(buffer, sizeof(buffer), "%s%u", ddef_letter, track_number);
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

MXFDataDefEnum bmx::convert_essence_type_to_data_def(EssenceType essence_type)
{
    switch (get_generic_essence_type(essence_type))
    {
        case PICTURE_ESSENCE: return MXF_PICTURE_DDEF;
        case SOUND_ESSENCE:   return MXF_SOUND_DDEF;
        case DATA_ESSENCE:    return MXF_DATA_DDEF;
        default:              return MXF_UNKNOWN_DDEF;
    }
}
