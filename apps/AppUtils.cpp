/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#define __STDC_FORMAT_MACROS

#include <cstdio>
#include <cstring>

#include "AppUtils.h"
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



typedef struct
{
    const char *color_str;
    Color color;
} ColorMap;


static const ColorMap COLOR_MAP[] =
{
    {"white",   COLOR_WHITE},
    {"red",     COLOR_RED},
    {"yellow",  COLOR_YELLOW},
    {"green",   COLOR_GREEN},
    {"cyan",    COLOR_CYAN},
    {"blue",    COLOR_BLUE},
    {"magenta", COLOR_MAGENTA},
    {"black",   COLOR_BLACK},
};



bool bmx::parse_timecode(const char *tc_str, Rational frame_rate, Timecode *timecode)
{
    int hour, min, sec, frame;
    char c;

    if (sscanf(tc_str, "%d:%d:%d%c%d", &hour, &min, &sec, &c, &frame) != 5)
        return false;

    timecode->Init(frame_rate, (c != ':'), hour, min, sec, frame);
    return true;
}

bool bmx::parse_position(const char *position_str, Timecode start_timecode, Rational frame_rate, int64_t *position)
{
    if (position_str[0] == 'o') {
        // ignore drop frame indictor for offset
        Rational nondrop_rate;
        if (frame_rate.denominator == 1001) {
            nondrop_rate.numerator = get_rounded_tc_base(frame_rate);
            nondrop_rate.denominator = 1;
        } else {
            nondrop_rate = frame_rate;
        }

        Timecode timecode;
        if (!parse_timecode(position_str + 1, nondrop_rate, &timecode))
            return false;

        *position = timecode.GetOffset();
        return true;
    }

    Timecode timecode;
    if (!parse_timecode(position_str, frame_rate, &timecode))
        return false;

    *position = timecode.GetOffset() - start_timecode.GetOffset();
    return true;
}

bool bmx::parse_partition_interval(const char *partition_interval_str, Rational frame_rate, int64_t *partition_interval)
{
    bool in_seconds = (strchr(partition_interval_str, 's') != 0);

    if (sscanf(partition_interval_str, "%"PRId64, partition_interval) != 1)
        return false;

    if (in_seconds)
        *partition_interval = (*partition_interval) * frame_rate.numerator / frame_rate.denominator;

    return true;
}

bool bmx::parse_bool(const char *bool_str, bool *value)
{
    if (strcmp(bool_str, "true") == 0)
        *value = true;
    else if (strcmp(bool_str, "false") == 0)
        *value = false;
    else
        return false;

    return true;
}

bool bmx::parse_color(const char *color_str, Color *color)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(COLOR_MAP); i++) {
        if (strcmp(COLOR_MAP[i].color_str, color_str) == 0) {
            *color = COLOR_MAP[i].color;
            return true;
        }
    }

    return false;
}

string bmx::create_mxf_track_filename(const char *prefix, uint32_t track_number, bool is_picture)
{
    char buffer[16];
    sprintf(buffer, "_%s%u.mxf", (is_picture ? "v" : "a"), track_number);

    string filename = prefix;
    return filename.append(buffer);
}

