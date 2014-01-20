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

#define __STDC_FORMAT_MACROS

#include <cstring>
#include <cstdarg>

#include <bmx/apps/AppInfoWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_utils.h>

using namespace std;
using namespace bmx;



static void print_hex_quad(char *buffer, size_t buffer_size, const unsigned char *bytes, size_t num_bytes)
{
    static const char hex_chars[] = "0123456789abcdef";

    BMX_ASSERT((num_bytes & 3) == 0);
    BMX_ASSERT(num_bytes >= 4 && buffer_size >= num_bytes * 2 + ((num_bytes - 4) >> 2));

    size_t index = 0;
    size_t i, j;
    for (i = 0; i < num_bytes; i += 4) {
        if (i != 0)
            buffer[index++] = '.';
        for (j = 0; j < 4; j++) {
            buffer[index++] = hex_chars[(bytes[i + j] >> 4) & 0x0f];
            buffer[index++] = hex_chars[ bytes[i + j]       & 0x0f];
        }
    }
    buffer[index] = '\0';
}



AppInfoWriter::AppInfoWriter()
{
    mClipEditRate = ZERO_RATIONAL;
    mIsAnnotation = false;
}

AppInfoWriter::~AppInfoWriter()
{
}

void AppInfoWriter::SetClipEditRate(Rational rate)
{
    mClipEditRate = rate;
}

void AppInfoWriter::RegisterCCName(const string &name, const string &cc_name)
{
    mCCNames[name] = cc_name;
}

void AppInfoWriter::StartAnnotations()
{
    mIsAnnotation = true;
}

void AppInfoWriter::EndAnnotations()
{
    mIsAnnotation = false;
}

void AppInfoWriter::WriteStringItem(const string &name, const string &value)
{
    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, value));
    else
        WriteItem(name, value);
}

void AppInfoWriter::WriteBoolItem(const string &name, bool value)
{
    char buffer[16];

    if (value)
        bmx_snprintf(buffer, sizeof(buffer), "true");
    else
        bmx_snprintf(buffer, sizeof(buffer), "false");

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, uint8_t value, bool hex)
{
    char buffer[32];

    if (hex)
        bmx_snprintf(buffer, sizeof(buffer), "0x%02x", value);
    else
        bmx_snprintf(buffer, sizeof(buffer), "%u", value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, uint16_t value, bool hex)
{
    char buffer[32];

    if (hex)
        bmx_snprintf(buffer, sizeof(buffer), "0x%04x", value);
    else
        bmx_snprintf(buffer, sizeof(buffer), "%u", value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, uint32_t value, bool hex)
{
    char buffer[32];

    if (hex)
        bmx_snprintf(buffer, sizeof(buffer), "0x%08x", value);
    else
        bmx_snprintf(buffer, sizeof(buffer), "%u", value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, uint64_t value, bool hex)
{
    char buffer[32];

    if (hex)
        bmx_snprintf(buffer, sizeof(buffer), "0x%016"PRIx64, value);
    else
        bmx_snprintf(buffer, sizeof(buffer), "%"PRIu64, value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, int8_t value)
{
    char buffer[32];

    bmx_snprintf(buffer, sizeof(buffer), "%d", value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, int16_t value)
{
    char buffer[32];

    bmx_snprintf(buffer, sizeof(buffer), "%d", value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, int32_t value)
{
    char buffer[32];

    bmx_snprintf(buffer, sizeof(buffer), "%d", value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteIntegerItem(const string &name, int64_t value)
{
    char buffer[32];

    bmx_snprintf(buffer, sizeof(buffer), "%"PRId64, value);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteRationalItem(const string &name, Rational value)
{
    char buffer[32];

    bmx_snprintf(buffer, sizeof(buffer), "%d/%d", value.numerator, value.denominator);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteTimecodeItem(const string &name, Timecode timecode)
{
    char buffer[32];

    bmx_snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d%c%02d",
                 timecode.GetHour(),
                 timecode.GetMin(),
                 timecode.GetSec(),
                 timecode.IsDropFrame() ? ';' : ':',
                 timecode.GetFrame());

    if (mIsAnnotation) {
        mAnnotations.push_back(make_pair(name, buffer));
    } else {
        if (timecode.GetRoundedTCBase() != get_rounded_tc_base(mClipEditRate)) {
            StartAnnotations();
            WriteIntegerItem("rate", timecode.GetRoundedTCBase());
            EndAnnotations();
        }
        WriteItem(name, buffer);
    }
}

void AppInfoWriter::WriteAUIDItem(const string &name, UL value)
{
    // UL as-is, UUID half-swapped
    if (mxf_is_ul((mxfUID*)&value)) {
        WriteULItem(name, &value);
    } else {
        UUID uuid;
        memcpy(&uuid.octet0, &value.octet8, 8);
        memcpy(&uuid.octet8, &value.octet0, 8);
        WriteUUIDItem(name, &uuid);
    }
}

void AppInfoWriter::WriteIDAUItem(const string &name, UUID value)
{
    // UL half-swapped, UUID as-is
    if (!mxf_is_ul((mxfUID*)&value)) {
        UL ul;
        memcpy(&ul.octet0, &value.octet8, 8);
        memcpy(&ul.octet8, &value.octet0, 8);
        WriteULItem(name, &ul);
    } else {
        WriteUUIDItem(name, &value);
    }
}

void AppInfoWriter::WriteUMIDItem(const string &name, UMID value)
{
    char buffer[128];

    bmx_snprintf(buffer, sizeof(buffer), "urn:smpte:umid:");
    size_t len = strlen(buffer);
    print_hex_quad(&buffer[len], sizeof(buffer) - len, &value.octet0, 32);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteTimestampItem(const string &name, Timestamp value)
{
    char buffer[64];

    bmx_snprintf(buffer, sizeof(buffer), "%d-%02u-%02u %02u:%02u:%02u.%03u",
                 value.year, value.month, value.day,
                 value.hour, value.min, value.sec, value.qmsec * 4);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteDateOnlyItem(const string &name, Timestamp value)
{
    char buffer[64];

    bmx_snprintf(buffer, sizeof(buffer), "%d-%02u-%02u", value.year, value.month, value.day);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteVersionTypeItem(const string &name, mxfVersionType value)
{
    char buffer[16];

    bmx_snprintf(buffer, sizeof(buffer), "%u.%u", (value >> 8) & 0xff, value & 0xff);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteProductVersionItem(const string &name, mxfProductVersion value)
{
    char buffer[64];

    bmx_snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u.%u",
                 value.major, value.minor, value.patch, value.build, value.release);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteDurationItem(const string &name, int64_t duration, Rational rate)
{
    string duration_str;
    if (duration >= 0)
        duration_str = get_duration_string(duration, rate);

    if (mIsAnnotation) {
        mAnnotations.push_back(make_pair(name, duration_str));
    } else {
        StartAnnotations();
        WriteIntegerItem("count", duration);
        if (rate != mClipEditRate)
            WriteIntegerItem("rate", get_rounded_tc_base(rate));
        EndAnnotations();

        WriteItem(name, duration_str);
    }
}

void AppInfoWriter::WritePositionItem(const string &name, int64_t position, Rational rate)
{
    string position_str;
    if (position >= 0) {
        position_str = get_duration_string(position, rate);
    } else {
        position_str = "-";
        position_str.append(get_duration_string(- position, rate));
    }

    if (mIsAnnotation) {
        mAnnotations.push_back(make_pair(name, position_str));
    } else {
        StartAnnotations();
        WriteIntegerItem("count", position);
        if (rate != mClipEditRate)
            WriteIntegerItem("rate", get_rounded_tc_base(rate));
        EndAnnotations();

        WriteItem(name, position_str);
    }
}

void AppInfoWriter::WriteEnumItem(const string &name, const EnumInfo *enum_info, int enum_value)
{
    if (mIsAnnotation) {
        WriteEnumStringItem(name, enum_info, enum_value);
    } else {
        const char *enum_name = "";
        const EnumInfo *enum_info_ptr = enum_info;
        while (enum_info_ptr->name) {
            if (enum_info_ptr->value == enum_value) {
                enum_name = enum_info_ptr->name;
                break;
            }
            enum_info_ptr++;
        }

        StartAnnotations();
        WriteIntegerItem("value", enum_value);
        EndAnnotations();

        WriteItem(name, enum_name);
    }
}

void AppInfoWriter::WriteEnumStringItem(const string &name, const EnumInfo *enum_info, int enum_value)
{
    const char *enum_name = "";
    const EnumInfo *enum_info_ptr = enum_info;
    while (enum_info_ptr->name) {
        if (enum_info_ptr->value == enum_value) {
            enum_name = enum_info_ptr->name;
            break;
        }
        enum_info_ptr++;
    }

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, enum_name));
    else
        WriteItem(name, enum_name);
}

void AppInfoWriter::WriteFormatItem(const string &name, const char *format, ...)
{
    char buffer[1024];

    va_list p_arg;
    va_start(p_arg, format);
    bmx_vsnprintf(buffer, sizeof(buffer), format, p_arg);
    va_end(p_arg);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteULItem(const string &name, const UL *value)
{
    char buffer[64];

    bmx_snprintf(buffer, sizeof(buffer), "urn:smpte:ul:");
    size_t len = strlen(buffer);
    print_hex_quad(&buffer[len], sizeof(buffer) - len, &value->octet0, 16);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppInfoWriter::WriteUUIDItem(const string &name, const UUID *value)
{
    char buffer[64];

    bmx_snprintf(buffer, sizeof(buffer),
                 "urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 value->octet0,  value->octet1,  value->octet2,  value->octet3,
                 value->octet4,  value->octet5,  value->octet6,  value->octet7,
                 value->octet8,  value->octet9,  value->octet10, value->octet11,
                 value->octet12, value->octet13, value->octet14, value->octet15);

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

string AppInfoWriter::CamelCaseName(const string &name) const
{
    map<string, string>::const_iterator cc_result = mCCNames.find(name);
    if (cc_result != mCCNames.end())
        return cc_result->second;

    string cc_name;
    cc_name.reserve(name.size());

    bool underscore = true;
    size_t i;
    for (i = 0; i < name.size(); i++) {
        if (name[i] == '_') {
            underscore = true;
        } else {
            if (underscore && name[i] >= 'a' && name[i] <= 'z')
                cc_name.append(1, 'A' + (name[i] - 'a'));
            else
                cc_name.append(1, name[i]);
            underscore = false;
        }
    }

    return cc_name;
}

