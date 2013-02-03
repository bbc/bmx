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
#define __STDC_LIMIT_MACROS

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <ctime>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <bmx/apps/AppUtils.h>
#include "ps_avci_header_data.h"
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

typedef struct
{
    const char *format_str;
    AVCIHeaderFormat format;
} AVCIHeaderFormatInfo;


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

static const AVCIHeaderFormatInfo AVCI_HEADER_FORMAT_INFO[] =
{
    {"AVC-Intra 100 1080i50",     {AVCI100_1080I,  {25, 1}}},
    {"AVC-Intra 100 1080i59.94",  {AVCI100_1080I,  {30000, 1001}}},
    {"AVC-Intra 100 1080p25",     {AVCI100_1080P,  {25, 1}}},
    {"AVC-Intra 100 1080p29.97",  {AVCI100_1080P,  {30000, 1001}}},
    {"AVC-Intra 100 720p25",      {AVCI100_720P,   {25, 1}}},
    {"AVC-Intra 100 720p50",      {AVCI100_720P,   {50, 1}}},
    {"AVC-Intra 100 720p29.97",   {AVCI100_720P,   {30000, 1001}}},
    {"AVC-Intra 100 720p59.94",   {AVCI100_720P,   {60000, 1001}}},
    {"AVC-Intra 50 1080i50",      {AVCI50_1080I,   {25, 1}}},
    {"AVC-Intra 50 1080i29.97",   {AVCI50_1080I,   {30000, 1001}}},
    {"AVC-Intra 50 1080p25",      {AVCI50_1080P,   {25, 1}}},
    {"AVC-Intra 50 1080p29.97",   {AVCI50_1080P,   {30000, 1001}}},
    {"AVC-Intra 50 720p25",       {AVCI50_720P,    {25, 1}}},
    {"AVC-Intra 50 720p50",       {AVCI50_720P,    {50, 1}}},
    {"AVC-Intra 50 720p29.97",    {AVCI50_720P,    {30000, 1001}}},
    {"AVC-Intra 50 720p59.94",    {AVCI50_720P,    {60000, 1001}}},
};



static bool parse_hex_string(const char *hex_str, unsigned char *octets, size_t octets_size)
{
    size_t i = 0, s = 0;

#define DECODE_HEX_CHAR(c)                  \
    if ((c) >= 'a' && (c) <= 'f')           \
        octets[i] |= 10 + ((c) - 'a');      \
    else if ((c) >= 'A' && (c) <= 'F')      \
        octets[i] |= 10 + ((c) - 'A');      \
    else if ((c) >= '0' && (c) <= '9')      \
        octets[i] |= ((c) - '0');           \
    else                                    \
        break;

    while (hex_str[s] && i < octets_size) {
        if (hex_str[s] != '.' && hex_str[s] != '-') {
            octets[i] = 0;
            DECODE_HEX_CHAR(hex_str[s])
            octets[i] <<= 4;
            s++;
            DECODE_HEX_CHAR(hex_str[s])
            i++;
        }
        s++;
    }

    return i == octets_size;
}



size_t bmx::get_num_avci_header_formats()
{
    return BMX_ARRAY_SIZE(AVCI_HEADER_FORMAT_INFO);
}

const char* bmx::get_avci_header_format_string(size_t index)
{
    BMX_ASSERT(index < BMX_ARRAY_SIZE(AVCI_HEADER_FORMAT_INFO));
    return AVCI_HEADER_FORMAT_INFO[index].format_str;
}


size_t bmx::get_num_ps_avci_header_formats()
{
    return BMX_ARRAY_SIZE(PS_AVCI_HEADER_DATA);
}

const char* bmx::get_ps_avci_header_format_string(size_t index)
{
    BMX_ASSERT(index < BMX_ARRAY_SIZE(PS_AVCI_HEADER_DATA));
    return PS_AVCI_HEADER_DATA[index].format_str;
}


bool bmx::parse_frame_rate(const char *rate_str, Rational *frame_rate)
{
    unsigned int value;
    if (sscanf(rate_str, "%u", &value) != 1)
        return false;

    if (value == 23976)
        *frame_rate = FRAME_RATE_23976;
    else if (value == 24)
        *frame_rate = FRAME_RATE_24;
    else if (value == 25)
        *frame_rate = FRAME_RATE_25;
    else if (value == 2997)
        *frame_rate = FRAME_RATE_2997;
    else if (value == 50)
        *frame_rate = FRAME_RATE_50;
    else if (value == 5994)
        *frame_rate = FRAME_RATE_5994;
    else
        return false;

    return true;
}

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
    for (i = 0; i < BMX_ARRAY_SIZE(COLOR_MAP); i++) {
        if (strcmp(COLOR_MAP[i].color_str, color_str) == 0) {
            *color = COLOR_MAP[i].color;
            return true;
        }
    }

    return false;
}

bool bmx::parse_avci_header(const char *format_str, const char *filename, const char *offset_str,
                            vector<AVCIHeaderInput> *avci_header_inputs)
{
    AVCIHeaderInput input;

    if (strcmp(format_str, "all") == 0) {
        size_t i;
        for (i = 0; i < BMX_ARRAY_SIZE(AVCI_HEADER_FORMAT_INFO); i++)
            input.formats.push_back(AVCI_HEADER_FORMAT_INFO[i].format);
    } else {
        size_t index;
        const char *format_str_ptr = format_str;
        while (format_str_ptr) {
            if (sscanf(format_str_ptr, "%"PRIszt, &index) != 1 || index > BMX_ARRAY_SIZE(AVCI_HEADER_FORMAT_INFO))
                return false;
            input.formats.push_back(AVCI_HEADER_FORMAT_INFO[index].format);

            format_str_ptr = strchr(format_str_ptr, ',');
            if (format_str_ptr)
                format_str_ptr++;
        }
    }

    input.filename = filename;
    if (sscanf(offset_str, "%"PRId64, &input.offset) != 1)
        return false;

    avci_header_inputs->push_back(input);
    return true;
}

bool bmx::parse_d10_sound_flags(const char *flags_str, uint8_t *out_flags)
{
    uint8_t flags = 0;
    uint8_t i = 0;
    while (i < 8 && flags_str[i]) {
        flags <<= 1;
        if (flags_str[i] == '1')
            flags |= 1;
        else if (flags_str[i] != '0')
            return false;
        i++;
    }
    if (flags_str[i])
        return false;

    *out_flags = flags;
    return true;
}

bool bmx::parse_timestamp(const char *timestamp_str, Timestamp *timestamp)
{
    int year;
    unsigned int month, day, hour, min, sec, qmsec;

    if (sscanf(timestamp_str, "%d-%u-%uT%u:%u:%u:%u", &year, &month, &day, &hour, &min, &sec, &qmsec) != 7)
        return false;

    timestamp->year  = (int16_t)year;
    timestamp->month = (uint8_t)month;
    timestamp->day   = (uint8_t)day;
    timestamp->hour  = (uint8_t)hour;
    timestamp->min   = (uint8_t)min;
    timestamp->sec   = (uint8_t)sec;
    timestamp->qmsec = (uint8_t)qmsec;

    return true;
}

bool bmx::parse_umid(const char *umid_str, UMID *umid)
{
    return parse_hex_string(umid_str, (unsigned char*)&umid->octet0, 32);
}

bool bmx::parse_uuid(const char *uuid_str, UUID *uuid)
{
    return parse_hex_string(uuid_str, (unsigned char*)&uuid->octet0, 16);
}

bool bmx::parse_product_version(const char *version_str, mxfProductVersion *version)
{
    unsigned int major, minor, patch, build, release;

    if (sscanf(version_str, "%u.%u.%u.%u.%u", &major, &minor, &patch, &build, &release) != 5)
        return false;

    version->major   = (uint16_t)major;
    version->minor   = (uint16_t)minor;
    version->patch   = (uint16_t)patch;
    version->build   = (uint16_t)build;
    version->release = (uint16_t)release;

    return true;
}

bool bmx::parse_product_info(const char **info_strings, size_t info_strings_size,
                             string *company_name, string *product_name, mxfProductVersion *product_version,
                             string *version, UUID *product_uid)
{
    if (info_strings_size > 0)
        *company_name = info_strings[0];
    else
        company_name->clear();
    if (info_strings_size > 1)
        *product_name = info_strings[1];
    else
        product_name->clear();
    if (info_strings_size > 2) {
        if (!parse_product_version(info_strings[2], product_version))
            return false;
    } else {
        memset(product_version, 0, sizeof(*product_version));
    }
    if (info_strings_size > 3)
        *version = info_strings[3];
    else
        version->clear();
    if (info_strings_size > 4) {
        if (!parse_uuid(info_strings[4], product_uid))
            return false;
    } else {
        memset(product_uid, 0, sizeof(*product_uid));
    }

    return true;
}

bool bmx::parse_avid_import_name(const char *import_name, URI *uri)
{
    if (strncmp(import_name, "file://", strlen("file://")) == 0)
    {
        return uri->Parse(import_name);
    }
    else if (((import_name[0] >= 'A' && import_name[0] <= 'Z') ||
                 (import_name[0] >= 'a' && import_name[0] <= 'z')) &&
             import_name[1] == ':')
    {
        uri->SetWindowsNameConvert(true);
        return uri->ParseFilename(import_name);
    }
    else if (import_name[0] == '/')
    {
        uri->SetWindowsNameConvert(false);
        return uri->ParseFilename(import_name);
    }
    else
    {
        if (!uri->ParseFilename(import_name))
            return false;

        if (uri->IsRelative()) {
            URI base_uri;
            base_uri.ParseDirectory(get_cwd());
            uri->MakeAbsolute(base_uri);
        }

        return true;
    }
}


string bmx::create_mxf_track_filename(const char *prefix, uint32_t track_number, bool is_picture)
{
    char buffer[16];
    bmx_snprintf(buffer, sizeof(buffer), "_%s%u.mxf", (is_picture ? "v" : "a"), track_number);

    string filename = prefix;
    return filename.append(buffer);
}


bool bmx::have_avci_header_data(EssenceType essence_type, Rational sample_rate,
                                vector<AVCIHeaderInput> &avci_header_inputs)
{
    size_t i;
    size_t j;
    for (i = 0; i < avci_header_inputs.size(); i++) {
        for (j = 0; j < avci_header_inputs[i].formats.size(); j++) {
            if (avci_header_inputs[i].formats[j].essence_type == essence_type &&
                avci_header_inputs[i].formats[j].sample_rate == sample_rate)
            {
                break;
            }
        }
        if (j < avci_header_inputs[i].formats.size())
            break;
    }

    return i < avci_header_inputs.size();
}

bool bmx::read_avci_header_data(EssenceType essence_type, Rational sample_rate,
                                vector<AVCIHeaderInput> &avci_header_inputs,
                                unsigned char *buffer, size_t buffer_size)
{
    BMX_ASSERT(buffer_size >= 512);

    size_t i;
    size_t j = 0;
    for (i = 0; i < avci_header_inputs.size(); i++) {
        for (j = 0; j < avci_header_inputs[i].formats.size(); j++) {
            if (avci_header_inputs[i].formats[j].essence_type == essence_type &&
                avci_header_inputs[i].formats[j].sample_rate == sample_rate)
            {
                break;
            }
        }
        if (j < avci_header_inputs[i].formats.size())
            break;
    }
    if (i >= avci_header_inputs.size())
        return false;


    FILE *file = fopen(avci_header_inputs[i].filename, "rb");
    if (!file) {
        log_error("Failed to open AVC-Intra header data input file '%s': %s\n",
                  avci_header_inputs[i].filename, bmx_strerror(errno).c_str());
        return false;
    }

    int64_t offset = avci_header_inputs[i].offset + j * 512;
#if defined(_WIN32)
    if (_fseeki64(file, offset, SEEK_SET) != 0) {
#else
    if (fseeko(file, offset, SEEK_SET) != 0) {
#endif
        log_error("Failed to seek to offset %"PRId64" in AVC-Intra header data input file '%s': %s\n",
                  offset, avci_header_inputs[i].filename, bmx_strerror(errno).c_str());
        fclose(file);
        return false;
    }

    if (fread(buffer, 512, 1, file) != 1) {
        log_error("Failed to read 512 bytes from AVC-Intra header data input file '%s' at offset %"PRId64": %s\n",
                  avci_header_inputs[i].filename, offset,
                  ferror(file) ? "read error" : "end of file");
        fclose(file);
        return false;
    }

    fclose(file);

    return true;
}


bool bmx::have_ps_avci_header_data(EssenceType essence_type, Rational sample_rate)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(PS_AVCI_HEADER_DATA); i++) {
        if (PS_AVCI_HEADER_DATA[i].essence_type == essence_type &&
            PS_AVCI_HEADER_DATA[i].sample_rate  == sample_rate)
        {
            return true;
        }
    }

    return false;
}

bool bmx::get_ps_avci_header_data(EssenceType essence_type, Rational sample_rate,
                                  unsigned char *buffer, size_t buffer_size)
{
    BMX_ASSERT(buffer_size >= 512);

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(PS_AVCI_HEADER_DATA); i++) {
        if (PS_AVCI_HEADER_DATA[i].essence_type == essence_type &&
            PS_AVCI_HEADER_DATA[i].sample_rate  == sample_rate)
        {
            break;
        }
    }
    if (i >= BMX_ARRAY_SIZE(PS_AVCI_HEADER_DATA))
        return false;

    // byte 0...255: AUD + SPS + zero padding
    memcpy(buffer, PS_AVCI_AUD, PS_AUD_DATA_SIZE);
    memcpy(&buffer[PS_AUD_DATA_SIZE], PS_AVCI_HEADER_DATA[i].sps, PS_SPS_DATA_SIZE);
    memset(&buffer[PS_AUD_DATA_SIZE + PS_SPS_DATA_SIZE], 0, 256 - (PS_AUD_DATA_SIZE + PS_SPS_DATA_SIZE));

    // byte 256...511: PPS + zero padding
    memcpy(&buffer[256], PS_AVCI_HEADER_DATA[i].pps, PS_PPS_DATA_SIZE);
    memset(&buffer[256 + PS_PPS_DATA_SIZE], 0, 256 - PS_PPS_DATA_SIZE);

    return true;
}


void bmx::init_progress(float *next_update)
{
    *next_update = -1.0;
}

void bmx::print_progress(int64_t count, int64_t duration, float *next_update)
{
    if (duration == 0)
        return;

    if (count == 0 && (!next_update || *next_update <= 0.0)) {
        printf("  0.0%%\r");
        fflush(stdout);
        if (next_update)
            *next_update = 0.1f;
    } else {
        float progress = count / (float)duration * 100;
        if (!next_update || progress >= *next_update) {
            printf("  %.1f%%\r", progress);
            fflush(stdout);
            if (next_update) {
                *next_update += 0.1f;
                if (*next_update < progress)
                    *next_update = progress + 0.1f;
            }
        }
    }
}

void bmx::sleep_msec(uint32_t msec)
{
#if HAVE_NANOSLEEP
    struct timespec req, rem;
    int result;
    req.tv_sec  = msec / 1000;
    req.tv_nsec = (msec % 1000) * 1000000L;
    while (true) {
        result = nanosleep(&req, &rem);
        if (result == 0 || errno != EINTR)
            break;
        req = rem;
    }

#elif defined(_WIN32)
    Sleep(msec);

#else
    usleep((useconds_t)msec * 1000);
#endif
}

uint32_t bmx::get_tick_count()
{
#if HAVE_CLOCK_GETTIME
    struct timespec now;
#if defined(CLOCK_MONOTONIC_COARSE)
    if (clock_gettime(CLOCK_MONOTONIC_COARSE, &now) != 0)
#else
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
#endif
        return 0;
    return (uint32_t)(now.tv_sec * 1000LL + now.tv_nsec / 1000000L);

#elif defined(_WIN32)
    return GetTickCount();

#else
    struct timeval now;
    if (gettimeofday(&now, 0) != 0)
        return 0;
    return (uint32_t)(now.tv_sec * 1000LL + now.tv_usec / 1000);
#endif
}

uint32_t bmx::delta_tick_count(uint32_t from, uint32_t to)
{
    return (uint32_t)((int64_t)to - from + UINT32_MAX);
}

void bmx::rt_sleep(float rt_factor, uint32_t start_tick, Rational sample_rate, int64_t num_samples)
{
    uint32_t tick = get_tick_count();
    uint32_t target_tick = (uint32_t)(start_tick + 1000 * num_samples *
                                            sample_rate.denominator / (rt_factor * sample_rate.numerator));
    uint32_t delta_tick = delta_tick_count(tick, target_tick);
    if (delta_tick < 10000) // don't sleep if target_tick < tick or there are bogus tick values
        sleep_msec(delta_tick);
}

