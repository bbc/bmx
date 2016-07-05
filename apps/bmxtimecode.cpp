/*
 * Copyright (C) 2016, British Broadcasting Corporation
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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <bmx/BMXTypes.h>
#include <bmx/Utils.h>
#include <bmx/apps/AppUtils.h>

using namespace std;
using namespace bmx;


static bool parse_frame_rate(const char *rate_str, Rational *frame_rate)
{
    unsigned int num, den;
    if (sscanf(rate_str, "%u/%u", &num, &den) == 2) {
        frame_rate->numerator   = num;
        frame_rate->denominator = den;
        return true;
    } else if (sscanf(rate_str, "%u", &num) == 1) {
        frame_rate->numerator   = num;
        frame_rate->denominator = 1;
        return true;
    }

    return false;
}

static void usage(const char *name)
{
    fprintf(stderr, "%s [--drop] [--non-drop] [--rate (<n>|<n>/<d>)] (all|<timecode>|<frame count>)\n", name);
}

int main(int argc, const char **argv)
{
    bool drop_frame = false;
    bool drop_frame_set = false;
    Rational frame_rate = FRAME_RATE_25;
    const char *value_str = 0;
    int cmdln_index = 1;

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++) {
        if (strcmp(argv[cmdln_index], "--drop") == 0) {
            drop_frame = true;
            drop_frame_set = true;
        } else if (strcmp(argv[cmdln_index], "--non-drop") == 0) {
            drop_frame = false;
            drop_frame_set = true;
        } else if (strcmp(argv[cmdln_index], "--rate") == 0) {
            if (cmdln_index + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            if (!::parse_frame_rate(argv[cmdln_index + 1], &frame_rate)) {
                usage(argv[0]);
                return 1;
            }
            cmdln_index++;
        } else {
            break;
        }
    }

    if (cmdln_index + 1 != argc) {
        usage(argv[0]);
        return 1;
    }
    if (strcmp(argv[cmdln_index], "all") != 0)
        value_str = argv[cmdln_index];


    Timecode timecode;
    int64_t offset;
    if (!value_str) {
        timecode.Init(frame_rate, drop_frame, 0);
        int64_t max_offset = timecode.GetMaxOffset();
        int64_t i;
        for (i = 0; i < max_offset; i++) {
            timecode.Init(frame_rate, drop_frame, i);
            printf("%8" PRId64 ": %s\n", i, get_timecode_string(timecode).c_str());
        }
    } else if (parse_timecode(value_str, frame_rate, &timecode)) {
        if (drop_frame_set) {
            timecode.Init(frame_rate, drop_frame, timecode.GetOffset());
            printf("%s\n", get_timecode_string(timecode).c_str());
        }
        printf("%" PRId64 "\n", timecode.GetOffset());
    } else if (sscanf(value_str, "%" PRId64, &offset) == 1) {
        timecode.Init(frame_rate, drop_frame, offset);
        printf("%s\n", get_timecode_string(timecode).c_str());
    } else {
        fprintf(stderr, "Failed to parse timecode or frame offset\n");
        return 1;
    }

    return 0;
}
