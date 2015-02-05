/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#define __STDC_LIMIT_MACROS

#include <cstdio>

#include "RDD6MetadataXML.h"
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



const char bmx::RDD6_NAMESPACE[] = "http://bbc.co.uk/rd/rdd6/201502";


const XMLEnumInfo bmx::PROGRAM_CONFIG_ENUM[] =
{
    {"5.1+2",               0 },
    {"5.1+1+1",             1 },
    {"4+4",                 2 },
    {"4+2+2",               3 },
    {"4+2+1+1",             4 },
    {"4+1+1+1+1",           5 },
    {"2+2+2+2",             6 },
    {"2+2+2+1+1",           7 },
    {"2+2+1+1+1+1",         8 },
    {"2+1+1+1+1+1+1",       9 },
    {"1+1+1+1+1+1+1+1",     10},
    {"5.1",                 11},
    {"4+2",                 12},
    {"4+1+1",               13},
    {"2+2+2",               14},
    {"2+2+1+1",             15},
    {"2+1+1+1+1",           16},
    {"1+1+1+1+1+1",         17},
    {"4",                   18},
    {"2+2",                 19},
    {"2+1+1",               20},
    {"1+1+1+1",             21},
    {"7.1",                 22},
    {"7.1_screen",          23},
    {0, 0}
};

const XMLEnumInfo bmx::FRAME_RATE_ENUM[] =
{
    {"23.98",    1},
    {"24",       2},
    {"25",       3},
    {"29.97",    4},
    {"30",       5},
    {0, 0}
};

const XMLEnumInfo bmx::DATARATE_ENUM[] =
{
    {"32",     0 },
    {"40",     1 },
    {"48",     2 },
    {"56",     3 },
    {"64",     4 },
    {"80",     5 },
    {"96",     6 },
    {"112",    7 },
    {"128",    8 },
    {"160",    9 },
    {"192",    10},
    {"224",    11},
    {"256",    12},
    {"320",    13},
    {"384",    14},
    {"448",    15},
    {"512",    16},
    {"576",    17},
    {"640",    18},
    // 19-30 = reserved
    // 31 = not specified
    {0, 0}
};

// use when acmod = 1
const XMLEnumInfo bmx::BSMOD_1_ENUM[] =
{
    {"main_complete",               0},
    {"main_music_and_effects",      1},
    {"assoc_visually_impaired",     2},
    {"assoc_hearing_impaired",      3},
    {"assoc_dialogue",              4},
    {"assoc_commentary",            5},
    {"assoc_emergency",             6},
    {"assoc_voice_over",            7}, // acmod = 1
    {0, 0}
};

// use when acmod = 2 to 7
const XMLEnumInfo bmx::BSMOD_2_ENUM[] =
{
    {"main_complete",               0},
    {"main_music_and_effects",      1},
    {"assoc_visually_impaired",     2},
    {"assoc_hearing_impaired",      3},
    {"assoc_dialogue",              4},
    {"assoc_commentary",            5},
    {"assoc_emergency",             6},
    {"main_karaoke",                7}, // acmod = 2 to 7
    {0, 0}
};

const XMLEnumInfo bmx::ACMOD_ENUM[] =
{
    // 0 not permitted
    {"1/0",         1},
    {"2/0",         2},
    {"3/0",         3},
    {"2/1",         4},
    {"3/1",         5},
    {"2/2",         6},
    {"3/2",         7},
    {0, 0}
};

const XMLEnumInfo bmx::CMIXLEVEL_ENUM[] =
{
    {"-3.0",   0},
    {"-4.5",   1},
    {"-6.0",   2},
    {"-4.5",   3}, // reserved; treat as -4.5
    {0, 0}
};

const XMLEnumInfo bmx::SURMIXLEVEL_ENUM[] =
{
    {"-3.0",   0},
    {"-6.0",   1},
    {"-inf",   2},
    {"-6.0",   3}, // reserved; treat as -6.0
    {0, 0}
};

const XMLEnumInfo bmx::ROOM_TYPE_ENUM[] =
{
    // 0 = not indicated
    {"large_room",  1},
    {"small_room",  2},
    // 3 = reserved; treated as not indicated
    {0, 0}
};

const XMLEnumInfo bmx::DMIXMOD_ENUM[] =
{
    // 0 = not indicated
    {"lt_rt",   1},
    {"lo_ro",   2},
    // 3 = reserved; treated as not indicated
    {0, 0}
};

const XMLEnumInfo bmx::LTRTCMIXLEVEL_ENUM[] =
{
    {"3.0",   0},
    {"1.5",   1},
    {"0.0",   2},
    {"-1.5",  3},
    {"-3.0",  4},
    {"-4.5",  5},
    {"-6.0",  6},
    {"-inf",  7},
    {0, 0}
};

const XMLEnumInfo bmx::LTRTSURMIXLEVEL_ENUM[] =
{
    {"3.0",   0},
    {"1.5",   1},
    {"0.0",   2},
    {"-1.5",  3},
    {"-3.0",  4},
    {"-4.5",  5},
    {"-6.0",  6},
    {"-inf",  7},
    {0, 0}
};

const XMLEnumInfo bmx::ADCONVTYPE_ENUM[] =
{
    {"standard",    0},
    {"hdcd",        1},
    {0, 0}
};

const XMLEnumInfo bmx::COMPR_ENUM[] =
{
    {"none",            0},
    {"film_standard",   1},
    {"film_light",      2},
    {"music_standard",  3},
    {"music_light",     4},
    {"speech",          5},
    {0, 0}
};



uint8_t bmx::parse_xml_uint8(const string &context, const string &str)
{
    int int_value = parse_xml_int(context, str);
    if (int_value < 0 || int_value > UINT8_MAX)
        throw BMXException("Failed to parse uint8 value '%s' for '%s'", str.c_str(), context.c_str());

    return (uint8_t)int_value;
}

int8_t bmx::parse_xml_int8(const string &context, const string &str)
{
    int int_value = parse_xml_int(context, str);
    if (int_value < INT8_MIN || int_value > INT8_MAX)
        throw BMXException("Failed to parse int8 value '%s' for '%s'", str.c_str(), context.c_str());

    return (int8_t)int_value;
}

uint16_t bmx::parse_xml_uint16(const string &context, const string &str)
{
    int int_value = parse_xml_int(context, str);
    if (int_value < 0 || int_value > UINT16_MAX)
        throw BMXException("Failed to parse uint16 value '%s' for '%s'", str.c_str(), context.c_str());

    return (uint16_t)int_value;
}

int16_t bmx::parse_xml_int16(const string &context, const string &str)
{
    int int_value = parse_xml_int(context, str);
    if (int_value < INT16_MIN || int_value > INT16_MAX)
        throw BMXException("Failed to parse int16 value '%s' for '%s'", str.c_str(), context.c_str());

    return (int16_t)int_value;
}

int bmx::parse_xml_int(const string &context, const string &str)
{
    int value;
    if (sscanf(str.c_str(), "%d", &value) != 1)
        throw BMXException("Failed to parse int value '%s' for '%s'", str.c_str(), context.c_str());

    return value;
}

uint8_t bmx::parse_xml_hex_uint8(const string &context, const string &str)
{
    int int_value;
    if (sscanf(str.c_str(), "0x%x", &int_value) != 1 || int_value < 0 || int_value > UINT8_MAX)
        throw BMXException("Failed to parse hex uint8 value '%s' for '%s'", str.c_str(), context.c_str());

    return (uint8_t)int_value;
}

uint16_t bmx::parse_xml_hex_uint16(const string &context, const string &str)
{
    int int_value;
    if (sscanf(str.c_str(), "0x%x", &int_value) != 1 || int_value < 0 || int_value > UINT16_MAX)
        throw BMXException("Failed to parse hex uint16 value '%s' for '%s'", str.c_str(), context.c_str());

    return (uint16_t)int_value;
}

bool bmx::parse_xml_bool(const string &context, const string &str)
{
    if (str == "true" || str == "1")
        return true;
    else if (str == "false" || str == "0")
        return false;

    throw BMXException("Failed to parse boolean value '%s' for '%s'", str.c_str(), context.c_str());
}

uint64_t bmx::parse_xml_smpte_timecode(const string &context, const string &str)
{
    char c;
    int hour, min, sec, frame;
    if (sscanf(str.c_str(), "%d:%d:%d%c%d", &hour, &min, &sec, &c, &frame) != 5)
        throw BMXException("Failed to parse smpte timecode value '%s' for '%s'", str.c_str(), context.c_str());

    uint64_t value = ((((uint64_t)hour  / 10) & 0x03) << 52) | ((((uint64_t)hour  % 10) & 0x0f) << 48) |
                     ((((uint64_t)min   / 10) & 0x07) << 36) | ((((uint64_t)min   % 10) & 0x0f) << 32) |
                     ((((uint64_t)sec   / 10) & 0x07) << 20) | ((((uint64_t)sec   % 10) & 0x0f) << 16) |
                     ((((uint64_t)frame / 10) & 0x03) <<  4) |  (((uint64_t)frame % 10) & 0x0f);
    if (c != ':')
        value |= 0x40;

    return value;
}

void bmx::parse_xml_timecode(const std::string &context, const std::string &str,
                             uint8_t *timecode_1_e, uint8_t *timecode_2_e,
                             uint16_t *timecode_1, uint16_t *timecode_2)
{
    int hour = 0, min = 0, sec = 0, frame = 0, frame_64 = 0;

    int res = sscanf(str.c_str(), "%d:%d:%d:%d+%d/64", &hour, &min, &sec, &frame, &frame_64);
    if (res == 5) {
        *timecode_1_e = 1;
        *timecode_2_e = 1;
    } else {
        res = sscanf(str.c_str(), "%d:%d:%d", &hour, &min, &sec);
        if (res == 3) {
            *timecode_1_e = 0;
            *timecode_2_e = 1;
        } else {
            res = sscanf(str.c_str(), "%d:%d+%d/64", &sec, &frame, &frame_64);
            if (res == 3) {
                *timecode_1_e = 1;
                *timecode_2_e = 0;
            } else {
                throw BMXException("Failed to parse timecode value '%s' for '%s'", str.c_str(), context.c_str());
            }
        }
    }

    hour     %= 24;
    min      %= 60;
    sec      %= 60;
    frame    %= 30;
    frame_64 %= 64;

    if (*timecode_2_e) {
        *timecode_1 = ((uint16_t)hour << 9) |
                      ((uint16_t)min  << 3) |
                      ((uint16_t)sec  >> 3);
    }
    if (*timecode_1_e) {
        *timecode_2 = (((uint16_t)sec & 0x07) << 11) |
                       ((uint16_t)frame       << 6)  |
                        (uint16_t)frame_64;
    }
}

int bmx::parse_xml_enum(const string &context, const XMLEnumInfo *info, const string &str)
{
    const XMLEnumInfo *ptr = info;
    while (ptr->name) {
        if (ptr->name == str)
            return ptr->value;
        ptr++;
    }

    throw BMXException("Failed to parse enumerated string '%s' for '%s'", str.c_str(), context.c_str());
}

size_t bmx::parse_xml_bytes(const string &context, const string &str, unsigned char *bytes, size_t size)
{
    if (str.empty())
        return 0;

#define DECODE_HEX_CHAR(c)                                      \
    if ((c) >= '0' && (c) <= '9')                               \
        bytes[s] |= ((c) - '0');                                \
    else if ((c) >= 'a' && (c) <= 'f')                          \
        bytes[s] |= 10 + ((c) - 'a');                           \
    else if ((c) >= 'A' && (c) <= 'F')                          \
        bytes[s] |= 10 + ((c) - 'A');                           \
    else                                                        \
        throw BMXException("Failed to parse bytes for '%s'",    \
                           context.c_str());

    size_t i, s;
    for (i = 0, s = 0; i < str.size() && s < size; i += 2, s++) {
        bytes[s] = 0;
        DECODE_HEX_CHAR(str[i])
        bytes[s] <<= 4;
        DECODE_HEX_CHAR(str[i + 1])
    }

    return s;
}


string bmx::unparse_xml_uint8(uint8_t value)
{
    return unparse_xml_int(value);
}

string bmx::unparse_xml_int8(int8_t value)
{
    return unparse_xml_int(value);
}

string bmx::unparse_xml_uint16(uint16_t value)
{
    return unparse_xml_int(value);
}

string bmx::unparse_xml_int16(int16_t value)
{
    return unparse_xml_int(value);
}

string bmx::unparse_xml_int(int value)
{
    char buf[16];
    bmx_snprintf(buf, sizeof(buf), "%d", value);

    return buf;
}

string bmx::unparse_xml_hex_uint8(uint8_t value)
{
    char buf[16];
    bmx_snprintf(buf, sizeof(buf), "0x%02x", value);

    return buf;
}

string bmx::unparse_xml_hex_uint16(uint16_t value)
{
    char buf[16];
    bmx_snprintf(buf, sizeof(buf), "0x%04x", value);

    return buf;
}

string bmx::unparse_xml_bool(bool value)
{
    if (value)
        return "true";
    else
        return "false";
}

string bmx::unparse_xml_smpte_timecode(uint64_t value)
{
    int hour, min, sec, frame;
    bool drop;
    hour =  (int)((((value >> 52) & 0x03) * 10) + ((value >> 48) & 0x0f));
    min =   (int)((((value >> 36) & 0x07) * 10) + ((value >> 32) & 0x0f));
    sec =   (int)((((value >> 20) & 0x07) * 10) + ((value >> 16) & 0x0f));
    frame = (int)((((value >>  4) & 0x03) * 10) + ((value      ) & 0x0f));
    drop =        (((value >>  4) & 0x04) != 0);

    char c;
    if (drop)
        c = ';';
    else
        c = ':';

    char buf[64];
    bmx_snprintf(buf, sizeof(buf), "%02d:%02d:%02d%c%02d",
                 hour, min, sec, c, frame);

    return buf;
}

string bmx::unparse_xml_timecode(uint8_t timecode_1_e, uint8_t timecode_2_e,
                                 uint16_t timecode_1, uint16_t timecode_2)
{
    BMX_ASSERT(timecode_1_e || timecode_2_e);

    char buf[64];
    int hour = 0, min = 0, sec = 0, frame = 0, frame_64 = 0;

    if (timecode_2_e) {
        // low resolution is present
        hour = (timecode_1 & 0x3e00) >> 9;
        min  = (timecode_1 & 0x01f8) >> 3;
        sec  = (timecode_1 & 0x0007) << 3;
    }
    if (timecode_1_e) {
        // high resolution is present
        sec      |= (timecode_2 & 0x3800) >> 11;
        frame     = (timecode_2 & 0x07c0) >> 6;
        frame_64  =  timecode_2 & 0x003f;
    }

    hour     %= 24;
    min      %= 60;
    sec      %= 60;
    frame    %= 30;
    frame_64 %= 64;

    if (timecode_1_e && timecode_2_e) {
        // both high and low resolution are present
        bmx_snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d+%d/64", hour, min, sec, frame, frame_64);
    } else if (timecode_2_e) {
        // only low resolution is present
        bmx_snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour, min, sec);
    } else { // timecode_1_e
        // only high resolution is present
        bmx_snprintf(buf, sizeof(buf), "%02d:%02d+%d/64", sec, frame, frame_64);
    }

    return buf;
}

string bmx::unparse_xml_enum(const string &context, const XMLEnumInfo *info, int value)
{
    const XMLEnumInfo *ptr = info;
    while (ptr->name) {
        if (ptr->value == value)
            return ptr->name;
        ptr++;
    }

    throw BMXException("Failed to unparse enumerated value %d for '%s'", value, context.c_str());
}

string bmx::unparse_xml_bytes(const unsigned char *bytes, size_t size)
{
    static const char hex_chars[] = "0123456789abcdef";

    if (!bytes || size == 0)
        return "";

    string str(2 * size, ' ');
    size_t i;
    for (i = 0; i < size; i++) {
        str[2 * i    ] = hex_chars[(bytes[i] & 0xf0) >> 4];
        str[2 * i + 1] = hex_chars[(bytes[i] & 0x0f)];
    }

    return str;
}

