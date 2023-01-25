/*
 * Copyright (C) 2016, British Broadcasting Corporation
 * All Rights Reserved.
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
#include <cerrno>

#include <bmx/apps/FrameworkHelper.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static bool parse_fw_bool(string value, bool *bool_value)
{
    if (value == "true" || value == "1") {
        *bool_value = true;
        return true;
    } else if (value == "false" || value == "0") {
        *bool_value = false;
        return true;
    }

    log_warn("Invalid boolean value '%s'\n", value.c_str());
    return false;
}

static bool parse_uint8(string value, uint8_t *uint8_value)
{
    unsigned int tmp;
    if (sscanf(value.c_str(), "%u", &tmp) == 1 && tmp < 256) {
        *uint8_value = (uint8_t)tmp;
        return true;
    }

    log_warn("Invalid uint8 value '%s'\n", value.c_str());
    return false;
}

static bool parse_uint16(string value, uint16_t *uint16_value)
{
    unsigned int tmp;
    if (sscanf(value.c_str(), "%u", &tmp) == 1 && tmp < 65536) {
        *uint16_value = (uint16_t)tmp;
        return true;
    }

    log_warn("Invalid uint16 value '%s'\n", value.c_str());
    return false;
}

static bool parse_int64(string value, int64_t *int64_value)
{
    if (sscanf(value.c_str(), "%" PRId64 "", int64_value) == 1)
        return true;

    log_warn("Invalid int64 value '%s'\n", value.c_str());
    return false;
}

static bool parse_duration(string value, Rational frame_rate, int64_t *int64_value)
{
    if (value.find(":") == string::npos) {
        if (sscanf(value.c_str(), "%" PRId64 "", int64_value) == 1)
            return true;
    } else {
        int hour, min, sec, frame;
        if (sscanf(value.c_str(), "%d:%d:%d:%d", &hour, &min, &sec, &frame) == 4) {
            uint16_t rounded_rate = get_rounded_tc_base(frame_rate);
            *int64_value = (int64_t)hour * 60 * 60 * rounded_rate +
                           (int64_t)min * 60 * rounded_rate +
                           (int64_t)sec * rounded_rate +
                           (int64_t)frame;
            return true;
        }
    }

    log_warn("Invalid duration value '%s'\n", value.c_str());
    return false;
}

static bool parse_position(string value, Timecode start_timecode, Rational frame_rate, int64_t *int64_value)
{
    if (value.find(":") == string::npos) {
        if (sscanf(value.c_str(), "%" PRId64 "", int64_value) == 1)
            return true;
    } else {
        int hour, min, sec, frame;
        char c;
        if (sscanf(value.c_str(), "%d:%d:%d%c%d", &hour, &min, &sec, &c, &frame) == 5) {
            Timecode tc(frame_rate, (c != ':'), hour, min, sec, frame);
            *int64_value = tc.GetOffset() - start_timecode.GetOffset();
            return true;
        }
    }

    log_warn("Invalid position value '%s'\n", value.c_str());
    return false;
}

static bool parse_as1n_timestamp(string value, mxfTimestamp *timestamp_value)
{
    int year;
    unsigned int month, day, hour, min, sec;
    if (sscanf(value.c_str(), "%d-%u-%uT%u:%u:%u ", &year, &month, &day, &hour, &min, &sec) == 6) {
        timestamp_value->year  = year;
        timestamp_value->month = month;
        timestamp_value->day   = day;
        timestamp_value->hour  = hour;
        timestamp_value->min   = min;
        timestamp_value->sec   = sec;
        timestamp_value->qmsec = 0;
        return true;
    } else if (sscanf(value.c_str(), "%d-%u-%u ", &year, &month, &day) == 3) {
        timestamp_value->year  = year;
        timestamp_value->month = month;
        timestamp_value->day   = day;
        timestamp_value->hour  = 0;
        timestamp_value->min   = 0;
        timestamp_value->sec   = 0;
        timestamp_value->qmsec = 0;
        return true;
    }

    log_warn("Invalid timestamp value '%s'\n", value.c_str());
    return false;
}

static bool parse_rational(string value, mxfRational *rational_value)
{
    int num, den;
    if (sscanf(value.c_str(), "%d/%d", &num, &den) == 2) {
        rational_value->numerator   = num;
        rational_value->denominator = den;
        return true;
    }

    log_warn("Invalid rational value '%s'\n", value.c_str());
    return false;
}

static bool parse_version_type(string value, mxfVersionType *vt_value)
{
    unsigned int v1, v2;
    if (sscanf(value.c_str(), "%u.%u", &v1, &v2) == 2) {
        if (v1 < 256 && v2 < 256) {
            *vt_value = (mxfVersionType)((v1 & 0xff) << 8 | (v2 & 0xff));
            return true;
        }
    } else if (sscanf(value.c_str(), "%u", &v1) == 1 && v1 < 65536) {
        *vt_value = (mxfVersionType)v1;
        return true;
    }

    log_warn("Invalid version type value '%s'\n", value.c_str());
    return false;
}

static size_t get_utf8_code_len(const char *u8_code)
{
    if ((unsigned char)u8_code[0] < 0x80)
    {
        return 1;
    }
    else if (((unsigned char)u8_code[0] & 0xe0) == 0xc0 &&
             ((unsigned char)u8_code[1] & 0xc0) == 0x80)
    {
        return 2;
    }
    else if (((unsigned char)u8_code[0] & 0xf0) == 0xe0 &&
             ((unsigned char)u8_code[1] & 0xc0) == 0x80 &&
             ((unsigned char)u8_code[2] & 0xc0) == 0x80)
    {
        return 3;
    }
    else if (((unsigned char)u8_code[0] & 0xf8) == 0xf0 &&
             ((unsigned char)u8_code[1] & 0xc0) == 0x80 &&
             ((unsigned char)u8_code[2] & 0xc0) == 0x80 &&
             ((unsigned char)u8_code[3] & 0xc0) == 0x80)
    {
        return 4;
    }

    return -1;
}

static size_t get_utf8_clip_len(const char *u8_str, size_t max_unicode_len, bool *invalid, bool *truncated)
{
    *invalid = false;
    size_t unicode_len = 0;
    size_t len = 0;
    while (u8_str[len] && (max_unicode_len == 0 || unicode_len < max_unicode_len)) {
        size_t code_len = get_utf8_code_len(&u8_str[len]);
        if (code_len == (size_t)(-1)) {
            *invalid = true;
            break;
        }
        len += code_len;
        unicode_len++;
    }
    *truncated = (u8_str[len] != 0);

    return len;
}



FrameworkHelper::FrameworkHelper(DMFramework *framework, const FrameworkInfo *info_in,
                                 Timecode start_tc, Rational frame_rate)
{
    mFramework = framework;
    mStartTimecode = start_tc;
    mFrameRate = frame_rate;

    DataModel *data_model = framework->getHeaderMetadata()->getDataModel();
    BMX_ASSERT(data_model->findSetDef(framework->getKey(), &mSetDef));

    const FrameworkInfo *framework_info = info_in;
    while (framework_info->name) {
        if (framework_info->set_key == *framework->getKey()) {
            mFrameworkInfo = framework_info;
            break;
        }
        framework_info++;
    }
    BMX_ASSERT(framework_info->name);
}

FrameworkHelper::~FrameworkHelper()
{
    delete mSetDef;
}

void FrameworkHelper::NormaliseStrings()
{
    // ensure UTF-16 string items have a null terminator by setting them again
    vector<MXFMetadataItem*> items = mFramework->getItems();
    size_t i;
    for (i = 0; i < items.size(); i++) {
        ItemDef *item_def;
        BMX_ASSERT(mSetDef->findItemDef(&items[i]->key, &item_def));
        if (item_def->getCItemDef()->typeId == MXF_UTF16STRING_TYPE) {
            mFramework->setStringItem(&items[i]->key, mFramework->getStringItem(&items[i]->key));
        }
        delete item_def;
    }
}

bool FrameworkHelper::SetProperty(const string &name, const string &value)
{
    const PropertyInfo *property_info = mFrameworkInfo->property_info;
    while (property_info->name) {
        if (name == property_info->name)
            break;
        property_info++;
    }
    if (!property_info->name) {
        log_warn("Unknown framework property %s::%s\n", mFrameworkInfo->name, name.c_str());
        return false;
    }

    ItemDef *item_def;
    BMX_ASSERT(mSetDef->findItemDef(&property_info->item_key, &item_def));
    MXFItemDef *c_item_def = item_def->getCItemDef();
    delete item_def;
    item_def = 0;

    switch (c_item_def->typeId)
    {
        case MXF_UTF16STRING_TYPE:
        {
            bool invalid = false;
            bool truncated = false;
            size_t clip_len = get_utf8_clip_len(value.c_str(), property_info->max_unicode_len, &invalid, &truncated);
            if (truncated) {
                if (invalid) {
                    log_warn("Truncating string property %s::%s to %" PRIszt " chars because it contains invalid UTF-8 data\n",
                             mFrameworkInfo->name, name.c_str(), clip_len);
                } else {
                    log_warn("Truncating string property %s::%s because it's length exceeds %" PRIszt " unicode chars\n",
                             mFrameworkInfo->name, name.c_str(), property_info->max_unicode_len);
                }
            }
            if (clip_len == 0)
                log_warn("Ignoring empty string property %s::%s\n", mFrameworkInfo->name, name.c_str());
            else
                mFramework->setStringItem(&c_item_def->key, value.substr(0, clip_len));
            break;
        }
        case MXF_UINT8_TYPE:
        {
            uint8_t uint8_value = 0;
            if (!parse_uint8(value, &uint8_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setUInt8Item(&c_item_def->key, uint8_value);
            break;
        }
        case MXF_UINT16_TYPE:
        {
            uint16_t uint16_value = 0;
            if (!parse_uint16(value, &uint16_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setUInt16Item(&c_item_def->key, uint16_value);
            break;
        }
        case MXF_POSITION_TYPE:
        {
            int64_t int64_value = 0;
            if (!parse_position(value, mStartTimecode, mFrameRate, &int64_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setInt64Item(&c_item_def->key, int64_value);
            break;
        }
        case MXF_LENGTH_TYPE:
        {
            int64_t int64_value = 0;
            if (!parse_duration(value, mFrameRate, &int64_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setInt64Item(&c_item_def->key, int64_value);
            break;
        }
        case MXF_INT64_TYPE:
        {
            int64_t int64_value = 0;
            if (!parse_int64(value, &int64_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setInt64Item(&c_item_def->key, int64_value);
            break;
        }
        case MXF_BOOLEAN_TYPE:
        {
            bool bool_value = false;
            if (!parse_fw_bool(value, &bool_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setBooleanItem(&c_item_def->key, bool_value);
            break;
        }
        case MXF_TIMESTAMP_TYPE:
        {
            mxfTimestamp timestamp_value = {0, 0, 0, 0, 0, 0, 0};
            if (!parse_as1n_timestamp(value, &timestamp_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setTimestampItem(&c_item_def->key, timestamp_value);
            break;
        }
        case MXF_RATIONAL_TYPE:
        {
            mxfRational rational_value = {0, 0};
            if (!parse_rational(value, &rational_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setRationalItem(&c_item_def->key, rational_value);
            break;
        }
        case MXF_VERSIONTYPE_TYPE:
        {
            mxfVersionType vt_value = 0;
            if (!parse_version_type(value, &vt_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n",  mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setVersionTypeItem(&c_item_def->key, vt_value);
            break;
        }
        case MXF_UMID_TYPE:
        {
            mxfUMID umid_value;
            if (!parse_umid(value.c_str(), &umid_value)) {
                log_warn("Invalid framework property value %s::%s '%s'\n", mFrameworkInfo->name, name.c_str(),
                         value.c_str());
                return false;
            }

            mFramework->setUMIDItem(&c_item_def->key, umid_value);
            break;
        }

        default:
            BMX_ASSERT(false);
            return false;
    }

    return true;
}
