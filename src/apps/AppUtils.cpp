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
#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/writer_helper/VC2WriterHelper.h>
#include "ps_avci_header_data.h"
#include <bmx/Utils.h>
#include <bmx/Version.h>
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

typedef struct
{
    ClipWriterType type;
    ClipSubType sub_type;
    const char *str;
} ClipWriterTypeStringMap;

typedef struct
{
    const char *name;
    UL label;
} NameLabelMap;


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
    {"AVC-Intra 100 1080p50",     {AVCI100_1080P,  {50, 1}}},
    {"AVC-Intra 100 1080p59.94",  {AVCI100_1080P,  {60000, 1001}}},
    {"AVC-Intra 50 1080p50",      {AVCI50_1080P,   {50, 1}}},
    {"AVC-Intra 50 1080p59.94",   {AVCI50_1080P,   {60000, 1001}}},
    {"AVC-Intra 50 1080p23.98",   {AVCI50_1080P,   {24000, 1001}}},
    {"AVC-Intra 50 720p23.98",    {AVCI50_720P,    {24000, 1001}}},
    {"AVC-Intra 100 1080p23.98",  {AVCI100_1080P,  {24000, 1001}}},
    {"AVC-Intra 100 720p23.98",   {AVCI100_720P,   {24000, 1001}}},
    {"AVC-Intra 200 1080i50",     {AVCI200_1080I,  {25, 1}}},
    {"AVC-Intra 200 1080i59.94",  {AVCI200_1080I,  {30000, 1001}}},
    {"AVC-Intra 200 1080p25",     {AVCI200_1080P,  {25, 1}}},
    {"AVC-Intra 200 1080p29.97",  {AVCI200_1080P,  {30000, 1001}}},
    {"AVC-Intra 200 1080p23.98",  {AVCI200_1080P,  {24000, 1001}}},
    {"AVC-Intra 200 720p25",      {AVCI200_720P,   {25, 1}}},
    {"AVC-Intra 200 720p50",      {AVCI200_720P,   {50, 1}}},
    {"AVC-Intra 200 720p29.97",   {AVCI200_720P,   {30000, 1001}}},
    {"AVC-Intra 200 720p59.94",   {AVCI200_720P,   {60000, 1001}}},
    {"AVC-Intra 200 720p23.98",   {AVCI200_720P,   {24000, 1001}}},
    {"AVC-Intra 200 1080p50",     {AVCI200_1080P,  {50, 1}}},
    {"AVC-Intra 200 1080p59.94",  {AVCI200_1080P,  {60000, 1001}}},
};

static const ClipWriterTypeStringMap CLIP_WRITER_TYPE_STRING_MAP[] =
{
    {CW_UNKNOWN_CLIP_TYPE,   NO_CLIP_SUB_TYPE,    "unknown"},
    {CW_AS02_CLIP_TYPE,      NO_CLIP_SUB_TYPE,    "AS-02"},
    {CW_OP1A_CLIP_TYPE,      AS11_CLIP_SUB_TYPE,  "AS-11 MXF OP-1A"},
    {CW_OP1A_CLIP_TYPE,      NO_CLIP_SUB_TYPE,    "MXF OP-1A"},
    {CW_AVID_CLIP_TYPE,      NO_CLIP_SUB_TYPE,    "Avid MXF"},
    {CW_D10_CLIP_TYPE,       AS11_CLIP_SUB_TYPE,  "AS-11 D-10 MXF"},
    {CW_D10_CLIP_TYPE,       NO_CLIP_SUB_TYPE,    "D-10 MXF"},
    {CW_RDD9_CLIP_TYPE,      AS10_CLIP_SUB_TYPE,  "AS-10 RDD9 MXF"},
    {CW_RDD9_CLIP_TYPE,      AS11_CLIP_SUB_TYPE,  "AS-11 RDD9 MXF"},
    {CW_RDD9_CLIP_TYPE,      NO_CLIP_SUB_TYPE,    "RDD9 MXF"},
    {CW_WAVE_CLIP_TYPE,      NO_CLIP_SUB_TYPE,    "Wave"},
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


string bmx::get_app_version_info(const char *app_name)
{
    char buffer[256];

    bmx_snprintf(buffer, sizeof(buffer), "%s, %s v%s, %s %s",
                 app_name,
                 get_bmx_library_name().c_str(),
                 get_bmx_version_string().c_str(),
                 __DATE__, __TIME__);

    if (!get_bmx_scm_version_string().empty()) {
        size_t len = strlen(buffer);
        bmx_snprintf(&buffer[len], sizeof(buffer) - len, " (scm %s)", get_bmx_scm_version_string().c_str());
    }

    return buffer;
}


const char* bmx::clip_type_to_string(ClipWriterType clip_type, ClipSubType sub_clip_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(CLIP_WRITER_TYPE_STRING_MAP); i++) {
        if (clip_type     == CLIP_WRITER_TYPE_STRING_MAP[i].type &&
            sub_clip_type == CLIP_WRITER_TYPE_STRING_MAP[i].sub_type)
        {
            return CLIP_WRITER_TYPE_STRING_MAP[i].str;
        }
    }

    return "";
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

void bmx::check_avid_avci_stop_bit(const unsigned char *input_data, const unsigned char *ps_data, size_t data_size,
                                   bool *missing_stop_bit, bool *other_differences)
{
    BMX_ASSERT(data_size >= AVCI_HEADER_SIZE);

    // check whether there are any differences
    size_t diff_offset = AVCI_HEADER_SIZE;
    size_t i;
    for (i = 0; i < AVCI_HEADER_SIZE; i++) {
        if (input_data[i] != ps_data[i]) {
            if (diff_offset < AVCI_HEADER_SIZE)
                break;
            diff_offset = i;
        }
    }
    if (diff_offset >= AVCI_HEADER_SIZE) {
        // no differences
        *missing_stop_bit  = false;
        *other_differences = false;
        return;
    } else if (i < AVCI_HEADER_SIZE) {
        // multiple differences
        *missing_stop_bit  = false;
        *other_differences = true;
        return;
    }

    // check that ps_data has a stop bit that input_data does not have
    bool found_stop_bit = false;
    unsigned char xor_byte = input_data[diff_offset] ^ ps_data[diff_offset];
    unsigned char cmp_bit;
    int s;
    for (s = 0; s < 8; s++) {
        cmp_bit = (1 << s);
        if ((xor_byte & cmp_bit)) {
            if (found_stop_bit || (input_data[diff_offset] & cmp_bit))
                break;
            found_stop_bit = true;
        }
    }
    if (s < 8) {
        *missing_stop_bit  = false;
        *other_differences = true;
        return;
    }

    // check that the remaining bytes are padding followed by a PPS NAL unit
    uint32_t state = (uint32_t)(-1);
    for (i = diff_offset + 1; i < AVCI_HEADER_SIZE - 1; i++) {
        state <<= 8;
        state |= input_data[i];
        if (input_data[i])
            break;
    }
    if ((state & 0x00ffffff) == 0x000001 &&     // start prefix
        (input_data[i + 1] & 0x1f) == 8)        // PPS == 8
    {
        *missing_stop_bit  = true;
        *other_differences = false;
    }
    else
    {
        *missing_stop_bit  = false;
        *other_differences = true;
    }
}


bool bmx::parse_log_level(const char *level_str, LogLevel *level)
{
    unsigned int value;
    if (sscanf(level_str, "%u", &value) != 1)
        return false;

    if (value > ERROR_LOG)
        *level = ERROR_LOG;
    else
        *level = (LogLevel)value;

    return true;
}

bool bmx::parse_frame_rate(const char *rate_str, Rational *frame_rate)
{
    unsigned int num, den;
    if (sscanf(rate_str, "%u/%u", &num, &den) == 2) {
        frame_rate->numerator   = num;
        frame_rate->denominator = den;
        return true;
    } else if (sscanf(rate_str, "%u", &num) == 1) {
        if (num == 23976) {
            *frame_rate = FRAME_RATE_23976;
        } else if (num == 2997) {
            *frame_rate = FRAME_RATE_2997;
        } else if (num == 5994) {
            *frame_rate = FRAME_RATE_5994;
        } else {
            frame_rate->numerator   = num;
            frame_rate->denominator = 1;
        }
        return true;
    }

    return false;
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

    if (in_seconds) {
        float sec;
        if (sscanf(partition_interval_str, "%f", &sec) != 1)
            return false;
        *partition_interval = (int64_t)(sec * frame_rate.numerator / frame_rate.denominator);
        return true;
    } else {
        return sscanf(partition_interval_str, "%" PRId64, partition_interval) == 1;
    }
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
            if (sscanf(format_str_ptr, "%" PRIszt, &index) != 1 || index > BMX_ARRAY_SIZE(AVCI_HEADER_FORMAT_INFO))
                return false;
            input.formats.push_back(AVCI_HEADER_FORMAT_INFO[index].format);

            format_str_ptr = strchr(format_str_ptr, ',');
            if (format_str_ptr)
                format_str_ptr++;
        }
    }

    input.filename = filename;
    if (sscanf(offset_str, "%" PRId64, &input.offset) != 1)
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

bool bmx::parse_clip_type(const char *clip_type_str, ClipWriterType *clip_type, ClipSubType *clip_sub_type)
{
    if (strcmp(clip_type_str, "as02") == 0) {
        *clip_type     = CW_AS02_CLIP_TYPE;
        *clip_sub_type = NO_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "as11op1a") == 0) {
        *clip_type     = CW_OP1A_CLIP_TYPE;
        *clip_sub_type = AS11_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "as11d10") == 0) {
        *clip_type     = CW_D10_CLIP_TYPE;
        *clip_sub_type = AS11_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "as11rdd9") == 0) {
        *clip_type     = CW_RDD9_CLIP_TYPE;
        *clip_sub_type = AS11_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "op1a") == 0) {
        *clip_type     = CW_OP1A_CLIP_TYPE;
        *clip_sub_type = NO_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "avid") == 0) {
        *clip_type     = CW_AVID_CLIP_TYPE;
        *clip_sub_type = NO_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "d10") == 0) {
        *clip_type     = CW_D10_CLIP_TYPE;
        *clip_sub_type = NO_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "rdd9") == 0) {
        *clip_type     = CW_RDD9_CLIP_TYPE;
        *clip_sub_type = NO_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "as10") == 0) {
        *clip_type     = CW_RDD9_CLIP_TYPE;
        *clip_sub_type = AS10_CLIP_SUB_TYPE;
    } else if (strcmp(clip_type_str, "wave") == 0) {
        *clip_type     = CW_WAVE_CLIP_TYPE;
        *clip_sub_type = NO_CLIP_SUB_TYPE;
    } else {
        return false;
    }

    return true;
}

bool bmx::parse_mic_type(const char *mic_type_str, MICType *mic_type)
{
    if (strcmp(mic_type_str, "md5") == 0)
        *mic_type = MD5_MIC_TYPE;
    else if (strcmp(mic_type_str, "none") == 0)
        *mic_type = NONE_MIC_TYPE;
    else
        return false;

    return true;
}

bool bmx::parse_klv_opt(const char *klv_opt_str, mxfKey *key, uint32_t *track_num)
{
    size_t len = strlen(klv_opt_str);
    if (len == 1) {
        if (klv_opt_str[0] != 's')
            return false;
        *track_num = 0;
        *key = g_Null_Key;
    } else if (len >= 32) {
        if (!parse_hex_string(klv_opt_str, (unsigned char*)&key->octet0, 16))
            return false;
        *track_num = 0;
    } else if (len >= 8) {
        unsigned char buffer[4];
        size_t parse_offset = 0;
        if (klv_opt_str[0] == '0' && klv_opt_str[1] == 'x') // skip '0x' prefix
            parse_offset = 2;
        if (!parse_hex_string(&klv_opt_str[parse_offset], buffer, sizeof(buffer)))
            return false;
        *track_num = ((uint32_t)buffer[0] << 24) |
                     ((uint32_t)buffer[1] << 16) |
                     ((uint32_t)buffer[2] << 8) |
                      (uint32_t)buffer[3];
        *key = g_Null_Key;
    } else {
        return false;
    }

    return true;
}

bool bmx::parse_anc_data_types(const char *types_str, set<ANCDataType> *types)
{
    const char *types_str_ptr = types_str;

    while (types_str_ptr && *types_str_ptr) {
        while (*types_str_ptr && (*types_str_ptr == ' ' || *types_str_ptr == ','))
            types_str_ptr++;
        if (!(*types_str_ptr))
            break;

        if (strncmp(types_str_ptr, "all", strlen("all")) == 0) {
            types->clear();
            types->insert(ALL_ANC_DATA);
            break;
        } else if (strncmp(types_str_ptr, "st2020", strlen("st2020")) == 0) {
            types->insert(ST2020_ANC_DATA);
        } else if (strncmp(types_str_ptr, "st2016", strlen("st2016")) == 0) {
            types->insert(ST2016_ANC_DATA);
        } else if (strncmp(types_str_ptr, "sdp", strlen("sdp")) == 0) {
            types->insert(RDD8_SDP_ANC_DATA);
        } else if (strncmp(types_str_ptr, "st12", strlen("st12")) == 0) {
            types->insert(ST12M_ANC_DATA);
        } else if (strncmp(types_str_ptr, "st334", strlen("st334")) == 0) {
            types->insert(ST334_ANC_DATA);
        } else {
            return false;
        }

        types_str_ptr = strchr(types_str_ptr, ',');
    }

    return !types->empty();
}

bool bmx::parse_checksum_type(const char *type_str, ChecksumType *type)
{
    if (strcmp(type_str, "crc32") == 0)
        *type = CRC32_CHECKSUM;
    else if (strcmp(type_str, "md5") == 0)
        *type = MD5_CHECKSUM;
    else if (strcmp(type_str, "sha1") == 0)
        *type = SHA1_CHECKSUM;
    else
        return false;

    return true;
}

bool bmx::parse_rdd6_lines(const char *lines_str, uint16_t *lines)
{
    const char *line_1_str = lines_str;
    const char *line_2_str = strchr(lines_str, ',');
    if (line_2_str)
        line_2_str++;

    unsigned int line_1, line_2;
    if (sscanf(line_1_str, "%u", &line_1) != 1)
        return false;
    if (!line_2_str)
        line_2 = line_1;
    else if (sscanf(line_2_str, "%u", &line_2) != 1)
        return false;

    lines[0] = line_1;
    lines[1] = line_2;

    return true;
}

bool bmx::parse_track_indexes(const char *tracks_str, set<size_t> *track_indexes)
{
    size_t first_index, last_index;
    const char *tracks_str_ptr = tracks_str;
    while (tracks_str_ptr) {
        size_t result = sscanf(tracks_str_ptr, "%" PRIszt "-%" PRIszt, &first_index, &last_index);
        if (result == 2) {
            if (first_index > last_index)
                return false;
            size_t index;
            for (index = first_index; index <= last_index; index++)
                track_indexes->insert(index);
        } else if (result == 1) {
            track_indexes->insert(first_index);
        } else {
            return false;
        }

        tracks_str_ptr = strchr(tracks_str_ptr, ',');
        if (tracks_str_ptr)
            tracks_str_ptr++;
    }

    return true;
}

bool bmx::parse_mxf_auid(const char *mxf_auid_str, UL *mxf_auid)
{
    if (strncmp(mxf_auid_str, "urn:smpte:ul:", 13) == 0)
        return parse_uuid(&mxf_auid_str[13], (UUID*)mxf_auid);

    // MXF AUID type has UL as-is and UUID half-swapped
    UUID uuid;
    const char *uuid_str = mxf_auid_str;
    if (strncmp(mxf_auid_str, "urn:uuid:", 9) == 0)
        uuid_str = &mxf_auid_str[9];
    if (!parse_uuid(uuid_str, &uuid))
        return false;
    mxf_swap_uid(mxf_auid, (const mxfUID*)&uuid);

    return true;
}

bool bmx::parse_bytes_size(const char *size_str, int64_t *size_out)
{
    double sizef;
    if (sscanf(size_str, "%lf", &sizef) != 1 || sizef < 0.0)
        return false;

    const char *suffix = size_str;
    while ((*suffix >= '0' && *suffix <= '9') || *suffix == '.')
        suffix++;

    if (suffix) {
        if (*suffix == 'k' || *suffix == 'K')
            sizef *= 1024.0;
        if (*suffix == 'm' || *suffix == 'M')
            sizef *= 1048576.0;
        if (*suffix == 'g' || *suffix == 'G')
            sizef *= 1073741824.0;
        if (*suffix == 't' || *suffix == 'T')
            sizef *= 1099511627776.0;
    }

    *size_out = (int64_t)(sizef + 0.5);
    return true;
}

bool bmx::parse_signal_standard(const char *str, MXFSignalStandard *value)
{
    static const char* enum_strings[] =
    {
        "none", "bt601", "bt1358", "st347", "st274", "st296", "st349", "st428"
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(enum_strings); i++) {
        if (strcmp(str, enum_strings[i]) == 0) {
            *value = (MXFSignalStandard)i;
            return true;
        }
    }

    return false;
}

bool bmx::parse_frame_layout(const char *str, MXFFrameLayout *value)
{
    static const char* enum_strings[] =
    {
        "fullframe", "separatefield", "singlefield", "mixedfield", "segmentedframe"
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(enum_strings); i++) {
        if (strcmp(str, enum_strings[i]) == 0) {
            *value = (MXFFrameLayout)i;
            return true;
        }
    }

    return false;
}

bool bmx::parse_field_dominance(const char *str, uint8_t *field_num)
{
    int value;
    if (sscanf(str, "%d", &value) == 1 && value >= 1 && value <= 2) {
        *field_num = value;
        return true;
    }

    return false;
}

bool bmx::parse_transfer_ch(const char *str, UL *label)
{
    static const NameLabelMap name_label_map[] =
    {
        {"bt470",     ITUR_BT470_TRANSFER_CH},
        {"bt709",     ITUR_BT709_TRANSFER_CH},
        {"st240",     SMPTE240M_TRANSFER_CH},
        {"st274",     SMPTE_274M_296M_TRANSFER_CH},
        {"bt1361",    ITU1361_TRANSFER_CH},
        {"linear",    LINEAR_TRANSFER_CH},
        {"dcdm",      SMPTE_DCDM_TRANSFER_CH},
        {"iec61966",  IEC6196624_XVYCC_TRANSFER_CH},
        {"bt2020",    ITU2020_TRANSFER_CH},
        {"st2084",    SMPTE_ST2084_TRANSFER_CH},
        {"hlg",       HLG_OETF_TRANSFER_CH},
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(name_label_map); i++) {
        if (strcmp(str, name_label_map[i].name) == 0) {
            *label = name_label_map[i].label;
            return true;
        }
    }

    return parse_mxf_auid(str, label);
}

bool bmx::parse_coding_equations(const char *str, UL *label)
{
    static const NameLabelMap name_label_map[] =
    {
        {"bt601",   ITUR_BT601_CODING_EQ},
        {"bt709",   ITUR_BT709_CODING_EQ},
        {"st240",   SMPTE_240M_CODING_EQ},
        {"ycgco",   Y_CG_CO_CODING_EQ},
        {"gbr",     GBR_CODING_EQ},
        {"bt2020",  ITU2020_NCL_CODING_EQ},
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(name_label_map); i++) {
        if (strcmp(str, name_label_map[i].name) == 0) {
            *label = name_label_map[i].label;
            return true;
        }
    }

    return parse_mxf_auid(str, label);
}

bool bmx::parse_color_primaries(const char *str, UL *label)
{
    static const NameLabelMap name_label_map[] =
    {
        {"st170",   SMPTE170M_COLOR_PRIM},
        {"bt470",   ITU470_PAL_COLOR_PRIM},
        {"bt709",   ITU709_COLOR_PRIM},
        {"bt2020",  ITU2020_COLOR_PRIM},
        {"dcdm",    SMPTE_DCDM_COLOR_PRIM},
        {"p3",      P3D65_COLOR_PRIM},
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(name_label_map); i++) {
        if (strcmp(str, name_label_map[i].name) == 0) {
            *label = name_label_map[i].label;
            return true;
        }
    }

    return parse_mxf_auid(str, label);
}

bool bmx::parse_color_siting(const char *str, MXFColorSiting *value)
{
    static const char* enum_strings[] =
    {
        "cositing", "horizmp", "3tap", "quincunx", "bt601", "linealt", "vertmp"
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(enum_strings); i++) {
        if (strcmp(str, enum_strings[i]) == 0) {
            *value = (MXFColorSiting)i;
            return true;
        }
    }
    if (strcmp(str, "unknown") == 0) {
        *value = MXF_COLOR_SITING_UNKNOWN;
        return true;
    }

    return false;
}

bool bmx::parse_vc2_mode(const char *mode_str, int *vc2_mode_flags)
{
    int mode;
    if (sscanf(mode_str, "%d", &mode) != 1)
        return false;

    if (mode == 0)
        *vc2_mode_flags = VC2_PASSTHROUGH;
    else if (mode == 1)
        *vc2_mode_flags = VC2_PICTURE_ONLY | VC2_COMPLETE_SEQUENCES;
    else
        return false;

    return true;
}


string bmx::create_mxf_track_filename(const char *prefix, uint32_t track_number, MXFDataDefEnum data_def)
{
    const char *ddef_letter = "x";
    switch (data_def)
    {
        case MXF_PICTURE_DDEF:  ddef_letter = "v"; break;
        case MXF_SOUND_DDEF:    ddef_letter = "a"; break;
        case MXF_DATA_DDEF:     ddef_letter = "d"; break;
        case MXF_TIMECODE_DDEF: ddef_letter = "t"; break;
        case MXF_DM_DDEF:       ddef_letter = "m"; break;
        case MXF_UNKNOWN_DDEF:  ddef_letter = "x"; break;
    }

    char buffer[16];
    bmx_snprintf(buffer, sizeof(buffer), "_%s%u.mxf", ddef_letter, track_number);

    string filename = prefix;
    return filename.append(buffer);
}


bool bmx::have_avci_header_data(EssenceType essence_type, Rational sample_rate,
                                vector<AVCIHeaderInput> &avci_header_inputs)
{
    return read_avci_header_data(essence_type, sample_rate, avci_header_inputs, 0, 0);
}

bool bmx::read_avci_header_data(EssenceType essence_type, Rational sample_rate,
                                vector<AVCIHeaderInput> &avci_header_inputs,
                                unsigned char *buffer, size_t buffer_size)
{
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

    if (!buffer)
        return true;
    BMX_ASSERT(buffer_size >= 512);

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
        log_error("Failed to seek to offset %" PRId64 " in AVC-Intra header data input file '%s': %s\n",
                  offset, avci_header_inputs[i].filename, bmx_strerror(errno).c_str());
        fclose(file);
        return false;
    }

    if (fread(buffer, 512, 1, file) != 1) {
        log_error("Failed to read 512 bytes from AVC-Intra header data input file '%s' at offset %" PRId64 ": %s\n",
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
    return get_ps_avci_header_data(essence_type, sample_rate, 0, 0);
}

bool bmx::get_ps_avci_header_data(EssenceType essence_type, Rational sample_rate,
                                  unsigned char *buffer, size_t buffer_size)
{
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

    if (!buffer)
        return true;
    BMX_ASSERT(buffer_size >= 512);

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

