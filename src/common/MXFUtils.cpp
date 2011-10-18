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

#include <algorithm>

#include <mxf/mxf.h>

#include <im/MXFUtils.h>
#include <im/Logging.h>


using namespace std;
using namespace im;


typedef struct
{
    mxfRational frame_rate;
    mxfRational sampling_rate;
    uint32_t sequence[11];
} SampleSequence;

static const SampleSequence SOUND_SAMPLE_SEQUENCES[] =
{
    {{25,    1},    {48000,1}, {1920, 0}},
    {{50,    1},    {48000,1}, {960, 0}},
    {{30000, 1001}, {48000,1}, {1602, 1601, 1602, 1601, 1602, 0}},
    {{60000, 1001}, {48000,1}, {801, 801, 800, 801, 801, 0}},
};

#define SOUND_SAMPLE_SEQUENCES_SIZE   (sizeof(SOUND_SAMPLE_SEQUENCES) / sizeof(SampleSequence))



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

bool im::get_sound_sample_sequence(mxfRational frame_rate, mxfRational sampling_rate,
                                   vector<uint32_t> *sample_sequence)
{
    size_t i;
    for (i = 0; i < SOUND_SAMPLE_SEQUENCES_SIZE; i++) {
        if (memcmp(&frame_rate,    &SOUND_SAMPLE_SEQUENCES[i].frame_rate,    sizeof(frame_rate)) == 0 &&
            memcmp(&sampling_rate, &SOUND_SAMPLE_SEQUENCES[i].sampling_rate, sizeof(sampling_rate)) == 0)
        {
            break;
        }
    }
    if (i >= SOUND_SAMPLE_SEQUENCES_SIZE)
        return false;

    size_t j = 0;
    while (SOUND_SAMPLE_SEQUENCES[i].sequence[j]) {
        sample_sequence->push_back(SOUND_SAMPLE_SEQUENCES[i].sequence[j]);
        j++;
    }

    return true;
}

void im::offset_sound_sample_sequence(vector<uint32_t> &sample_sequence, uint8_t offset)
{
    rotate(sample_sequence.begin(),
           sample_sequence.begin() + (offset % sample_sequence.size()),
           sample_sequence.end());
}

