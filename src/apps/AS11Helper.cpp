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
#include <cerrno>

#include <bmx/apps/AS11Helper.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/as11/UKDPPDMS.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const PropertyInfo AS11_CORE_PROPERTY_INFO[] =
{
    {"SeriesTitle",                 MXF_ITEM_K(AS11CoreFramework, AS11SeriesTitle)},
    {"ProgrammeTitle",              MXF_ITEM_K(AS11CoreFramework, AS11ProgrammeTitle)},
    {"EpisodeTitleNumber",          MXF_ITEM_K(AS11CoreFramework, AS11EpisodeTitleNumber)},
    {"ShimName",                    MXF_ITEM_K(AS11CoreFramework, AS11ShimName)},
    {"ShimVersion",                 MXF_ITEM_K(AS11CoreFramework, AS11ShimVersion)},
    {"AudioTrackLayout",            MXF_ITEM_K(AS11CoreFramework, AS11AudioTrackLayout)},
    {"PrimaryAudioLanguage",        MXF_ITEM_K(AS11CoreFramework, AS11PrimaryAudioLanguage)},
    {"ClosedCaptionsPresent",       MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsPresent)},
    {"ClosedCaptionsType",          MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsType)},
    {"ClosedCaptionsLanguage",      MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsLanguage)},

    {0, g_Null_Key},
};

static const PropertyInfo AS11_SEGMENTATION_PROPERTY_INFO[] =
{
    {"PartNumber",                  MXF_ITEM_K(AS11SegmentationFramework,   AS11PartNumber)},
    {"PartTotal",                   MXF_ITEM_K(AS11SegmentationFramework,   AS11PartTotal)},

    {0, g_Null_Key},
};

static const PropertyInfo UK_DPP_PROPERTY_INFO[] =
{
    {"ProductionNumber",            MXF_ITEM_K(UKDPPFramework, UKDPPProductionNumber)},
    {"Synopsis",                    MXF_ITEM_K(UKDPPFramework, UKDPPSynopsis)},
    {"Originator",                  MXF_ITEM_K(UKDPPFramework, UKDPPOriginator)},
    {"CopyrightYear",               MXF_ITEM_K(UKDPPFramework, UKDPPCopyrightYear)},
    {"OtherIdentifier",             MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifier)},
    {"OtherIdentifierType",         MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifierType)},
    {"Genre",                       MXF_ITEM_K(UKDPPFramework, UKDPPGenre)},
    {"Distributor",                 MXF_ITEM_K(UKDPPFramework, UKDPPDistributor)},
    {"PictureRatio",                MXF_ITEM_K(UKDPPFramework, UKDPPPictureRatio)},
    {"ThreeD",                      MXF_ITEM_K(UKDPPFramework, UKDPP3D)},
    {"3D",                          MXF_ITEM_K(UKDPPFramework, UKDPP3D)},
    {"ThreeDType",                  MXF_ITEM_K(UKDPPFramework, UKDPP3DType)},
    {"3DType",                      MXF_ITEM_K(UKDPPFramework, UKDPP3DType)},
    {"ProductPlacement",            MXF_ITEM_K(UKDPPFramework, UKDPPProductPlacement)},
    {"PSEPass",                     MXF_ITEM_K(UKDPPFramework, UKDPPPSEPass)},
    {"PSEManufacturer",             MXF_ITEM_K(UKDPPFramework, UKDPPPSEManufacturer)},
    {"PSEVersion",                  MXF_ITEM_K(UKDPPFramework, UKDPPPSEVersion)},
    {"VideoComments",               MXF_ITEM_K(UKDPPFramework, UKDPPVideoComments)},
    {"SecondaryAudioLanguage",      MXF_ITEM_K(UKDPPFramework, UKDPPSecondaryAudioLanguage)},
    {"TertiaryAudioLanguage",       MXF_ITEM_K(UKDPPFramework, UKDPPTertiaryAudioLanguage)},
    {"AudioLoudnessStandard",       MXF_ITEM_K(UKDPPFramework, UKDPPAudioLoudnessStandard)},
    {"AudioComments",               MXF_ITEM_K(UKDPPFramework, UKDPPAudioComments)},
    {"LineUpStart",                 MXF_ITEM_K(UKDPPFramework, UKDPPLineUpStart)},
    {"IdentClockStart",             MXF_ITEM_K(UKDPPFramework, UKDPPIdentClockStart)},
    {"TotalNumberOfParts",          MXF_ITEM_K(UKDPPFramework, UKDPPTotalNumberOfParts)},
    {"TotalProgrammeDuration",      MXF_ITEM_K(UKDPPFramework, UKDPPTotalProgrammeDuration)},
    {"AudioDescriptionPresent",     MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionPresent)},
    {"AudioDescriptionType",        MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionType)},
    {"OpenCaptionsPresent",         MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsPresent)},
    {"OpenCaptionsType",            MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsType)},
    {"OpenCaptionsLanguage",        MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsLanguage)},
    {"SigningPresent",              MXF_ITEM_K(UKDPPFramework, UKDPPSigningPresent)},
    {"SignLanguage",                MXF_ITEM_K(UKDPPFramework, UKDPPSignLanguage)},
    {"CompletionDate",              MXF_ITEM_K(UKDPPFramework, UKDPPCompletionDate)},
    {"TextlessElementsExist",       MXF_ITEM_K(UKDPPFramework, UKDPPTextlessElementsExist)},
    {"ProgrammeHasText",            MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeHasText)},
    {"ProgrammeTextLanguage",       MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeTextLanguage)},
    {"ContactEmail",                MXF_ITEM_K(UKDPPFramework, UKDPPContactEmail)},
    {"ContactTelephoneNumber",      MXF_ITEM_K(UKDPPFramework, UKDPPContactTelephoneNumber)},

    {0, g_Null_Key},
};

static const FrameworkInfo FRAMEWORK_INFO[] =
{
    {"AS11CoreFramework",           MXF_SET_K(AS11CoreFramework),           AS11_CORE_PROPERTY_INFO},
    {"AS11SegmentationFramework",   MXF_SET_K(AS11SegmentationFramework),   AS11_SEGMENTATION_PROPERTY_INFO},
    {"UKDPPFramework",              MXF_SET_K(UKDPPFramework),              UK_DPP_PROPERTY_INFO},

    {0, g_Null_Key, 0},
};



static string get_short_name(string name)
{
    if (name.compare(0, strlen("AS11"), "AS11") == 0)
        return name.substr(strlen("AS11"));
    else if (name.compare(0, strlen("UKDPP"), "UKDPP") == 0)
        return name.substr(strlen("UKDPP"));
    else
        return name;
}

static string trim_string(string value)
{
    size_t start;
    size_t len;

    // trim spaces from the start
    start = 0;
    while (start < value.size() && isspace(value[start]))
        start++;
    if (start >= value.size())
        return "";

    // trim spaces from the end by reducing the length
    len = value.size() - start;
    while (len > 0 && isspace(value[start + len - 1]))
        len--;

    return value.substr(start, len);
}

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
    if (sscanf(value.c_str(), "%"PRId64"", int64_value) == 1)
        return true;

    log_warn("Invalid int64 value '%s'\n", value.c_str());
    return false;
}

static bool parse_duration(string value, Rational frame_rate, int64_t *int64_value)
{
    if (value.find(":") == string::npos) {
        if (sscanf(value.c_str(), "%"PRId64"", int64_value) == 1)
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
        if (sscanf(value.c_str(), "%"PRId64"", int64_value) == 1)
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

static bool parse_as11_timestamp(string value, mxfTimestamp *timestamp_value)
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
        if (v1 < 256 && v1 < 256) {
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
    while (u8_str[len] && unicode_len < max_unicode_len) {
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



FrameworkHelper::FrameworkHelper(AS11WriterHelper *writer_helper, DMFramework *framework)
{
    mFramework = framework;
    mStartTimecode = writer_helper->GetClip()->GetStartTimecode();
    mFrameRate = writer_helper->GetClip()->GetFrameRate();
    BMX_ASSERT(writer_helper->GetClip()->GetDataModel()->findSetDef(framework->getKey(), &mSetDef));

    const FrameworkInfo *framework_info = FRAMEWORK_INFO;
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
}

bool FrameworkHelper::SetProperty(string name, string value)
{
    string short_name = get_short_name(name);
    const PropertyInfo *property_info = mFrameworkInfo->property_info;
    while (property_info->name) {
        if (short_name == property_info->name)
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

    switch (c_item_def->typeId)
    {
        case MXF_UTF16STRING_TYPE:
        {
            bool invalid = false;
            bool truncated = false;
            size_t max_unicode_len = 127;
            if (property_info->item_key == MXF_ITEM_K(UKDPPFramework, UKDPPSynopsis))
                max_unicode_len = 250;
            size_t clip_len = get_utf8_clip_len(value.c_str(), max_unicode_len, &invalid, &truncated);
            if (truncated) {
                if (invalid) {
                    log_warn("Truncating string property %s::%s to %"PRIszt" chars because it contains invalid UTF-8 data\n",
                             mFrameworkInfo->name, name.c_str(), clip_len);
                } else {
                    log_warn("Truncating string property %s::%s because it's length exceeds %"PRIszt" unicode chars\n",
                             mFrameworkInfo->name, name.c_str(), max_unicode_len);
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
            if (!parse_as11_timestamp(value, &timestamp_value)) {
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
        default:
            BMX_ASSERT(false);
            return false;
    }

    return true;
}



AS11Helper::AS11Helper()
{
    mFillerCompleteSegments = false;
    mSourceInfo = 0;
    mWriterHelper = 0;
    mAS11FrameworkHelper = 0;
    mUKDPPFrameworkHelper = 0;
    mHaveUKDPPTotalNumberOfParts = false;
    mHaveUKDPPTotalProgrammeDuration = false;
}

AS11Helper::~AS11Helper()
{
    delete mSourceInfo;
    delete mWriterHelper;
    delete mAS11FrameworkHelper;
    delete mUKDPPFrameworkHelper;
}

void AS11Helper::ReadSourceInfo(MXFFileReader *source_file)
{
    int64_t read_start_pos;
    int64_t read_duration;
    source_file->GetReadLimits(false, &read_start_pos, &read_duration);
    if (read_start_pos != source_file->GetReadStartPosition() ||
        read_duration  != source_file->GetReadDuration())
    {
        BMX_EXCEPTION(("Copying AS-11 descriptive metadata is currently only supported for complete file duration transfers"));
    }

    mSourceInfo = new AS11Info();
    mSourceInfo->Read(source_file->GetHeaderMetadata());

    mSourceStartTimecode = source_file->GetMaterialTimecode(0);

    if (mSourceInfo->core && mSourceInfo->core->haveItem(&MXF_ITEM_K(AS11CoreFramework, AS11ProgrammeTitle)))
        mSourceProgrammeTitle = mSourceInfo->core->GetProgrammeTitle();

    int64_t offset = 0;
    size_t i;
    for (i = 0; i < mSourceInfo->segmentation.size(); i++) {
        DMSegment *seg = dynamic_cast<DMSegment*>(mSourceInfo->segmentation[i]);
        if (seg) {
            AS11SegmentationFramework *framework = dynamic_cast<AS11SegmentationFramework*>(seg->getDMFramework());
            BMX_CHECK(framework);

            Timecode seg_start_tc = mSourceStartTimecode;
            seg_start_tc.AddOffset(offset, mSourceInfo->segmentation_rate);

            AS11TCSegment segment;
            segment.part_number  = framework->GetPartNumber();
            segment.part_total   = framework->GetPartTotal();
            segment.start        = seg_start_tc;
            segment.duration     = mSourceInfo->segmentation[i]->getDuration();
            mSegments.push_back(segment);
        }
        offset += mSourceInfo->segmentation[i]->getDuration();
    }

    mFillerCompleteSegments = true;
}

bool AS11Helper::ParseFrameworkFile(const char *type_str, const char *filename)
{
    FrameworkType type;
    if (!ParseFrameworkType(type_str, &type))
        return false;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, bmx_strerror(errno).c_str());
        return false;
    }

    int line_num = 0;
    int c = '1';
    while (c != EOF) {
        // move file pointer past the newline characters
        while ((c = fgetc(file)) != EOF && (c == '\r' || c == '\n')) {
            if (c == '\n')
                line_num++;
        }

        string name, value;
        bool parse_name = true;
        while (c != EOF && (c != '\r' && c != '\n')) {
            if (c == ':' && parse_name) {
                parse_name = false;
            } else {
                if (parse_name)
                    name += c;
                else
                    value += c;
            }

            c = fgetc(file);
        }
        if (!name.empty()) {
            if (parse_name) {
                fprintf(stderr, "Failed to parse line %d\n", line_num);
                fclose(file);
                return false;
            }

            SetFrameworkProperty(type, trim_string(name), trim_string(value));
        }

        if (c == '\n')
            line_num++;
    }

    fclose(file);

    return true;
}

bool AS11Helper::ParseSegmentationFile(const char *filename, Rational frame_rate)
{
    mSegments.clear();
    mFillerCompleteSegments = false;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, bmx_strerror(errno).c_str());
        return false;
    }

    const uint16_t rounded_rate = get_rounded_tc_base(frame_rate);
    bool last_tc_xx = false;
    int line_num = 0;
    int c = '1';
    while (c != EOF) {
        // move file pointer past the newline characters
        while ((c = fgetc(file)) != EOF && (c == '\r' || c == '\n')) {
            if (c == '\n')
                line_num++;
        }

        // skip leading space
        while (c == ' ' || c == '\t')
            c = fgetc(file);

        // line
        string line_str;
        while (c != EOF && (c != '\r' && c != '\n')) {
            line_str += c;
            c = fgetc(file);
        }

        int part_num = 0, part_total = 0;
        int som_h = 0, som_m = 0, som_s = 0, som_f = 0;
        int dur_h = 0, dur_m = 0, dur_s = 0, dur_f = 0;
        char xc = 0;
        if (!line_str.empty()) {
            if (sscanf(line_str.c_str(), "%d/%d %d:%d:%d:%d %d:%d:%d:%d",
                       &part_num, &part_total,
                       &som_h, &som_m, &som_s, &som_f,
                       &dur_h, &dur_m, &dur_s, &dur_f) != 10 &&
                (sscanf(line_str.c_str(), "%d/%d %d:%d:%d:%d %c",
                       &part_num, &part_total,
                       &som_h, &som_m, &som_s, &som_f, &xc) != 7 ||
                   (xc != 'x' && xc != 'X')))
            {
                fprintf(stderr, "Failed to parse segment line %d\n", line_num);
                fclose(file);
                mSegments.clear();
                return false;
            }

            AS11TCSegment segment;
            segment.part_number  = part_num;
            segment.part_total   = part_total;
            segment.start.Init(frame_rate, false, som_h, som_m, som_s, som_f);
            segment.duration     = 0;
            if (!xc) {
                segment.duration = (int64_t)dur_h * 60 * 60 * rounded_rate +
                                   (int64_t)dur_m * 60 * rounded_rate +
                                   (int64_t)dur_s * rounded_rate +
                                   (int64_t)dur_f;
            }
            mSegments.push_back(segment);

            if (xc) {
                last_tc_xx = true;
                break;
            }
        }

        if (c == '\n')
            line_num++;
    }

    mFillerCompleteSegments = !last_tc_xx;


    fclose(file);

    return true;
}

bool AS11Helper::SetFrameworkProperty(const char *type_str, const char *name, const char *value)
{
    FrameworkType type;
    if (!ParseFrameworkType(type_str, &type))
        return false;

    SetFrameworkProperty(type, name, value);
    return true;
}

bool AS11Helper::HaveProgrammeTitle() const
{
    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS11_CORE_FRAMEWORK_TYPE &&
            get_short_name(mFrameworkProperties[i].name) == "ProgrammeTitle")
        {
            return true;
        }
    }

    return !mSourceProgrammeTitle.empty();
}

string AS11Helper::GetProgrammeTitle() const
{
    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS11_CORE_FRAMEWORK_TYPE &&
            get_short_name(mFrameworkProperties[i].name) == "ProgrammeTitle")
        {
            return mFrameworkProperties[i].value;
        }
    }

    return mSourceProgrammeTitle;
}

void AS11Helper::InsertFrameworks(ClipWriter *clip)
{
    if (clip->GetType() != CW_OP1A_CLIP_TYPE && clip->GetType() != CW_D10_CLIP_TYPE) {
        BMX_EXCEPTION(("AS-11 is not supported in clip type '%s'",
                       clip_type_to_string(clip->GetType(), NO_CLIP_SUB_TYPE).c_str()));
    }

    mWriterHelper = new AS11WriterHelper(clip);

    if (mSourceInfo) {
        if (mSourceInfo->core) {
            AS11CoreFramework::RegisterObjectFactory(clip->GetHeaderMetadata());
            AS11CoreFramework *core_copy =
                dynamic_cast<AS11CoreFramework*>(mSourceInfo->core->clone(clip->GetHeaderMetadata()));
            mAS11FrameworkHelper = new FrameworkHelper(mWriterHelper, core_copy);
        }
        if (mSourceInfo->ukdpp) {
            UKDPPFramework::RegisterObjectFactory(clip->GetHeaderMetadata());
            UKDPPFramework *ukdpp_copy =
                dynamic_cast<UKDPPFramework*>(mSourceInfo->ukdpp->clone(clip->GetHeaderMetadata()));
            mUKDPPFrameworkHelper = new FrameworkHelper(mWriterHelper, ukdpp_copy);
        }
    }

    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS11_CORE_FRAMEWORK_TYPE) {
            if (!mAS11FrameworkHelper) {
                mAS11FrameworkHelper = new FrameworkHelper(mWriterHelper,
                                                new AS11CoreFramework(mWriterHelper->GetClip()->GetHeaderMetadata()));
            }
            BMX_CHECK_M(mAS11FrameworkHelper->SetProperty(mFrameworkProperties[i].name, mFrameworkProperties[i].value),
                        ("Failed to set AS11CoreFramework property '%s' to '%s'",
                         mFrameworkProperties[i].name.c_str(), mFrameworkProperties[i].value.c_str()));
        } else {
            if (!mUKDPPFrameworkHelper) {
                mUKDPPFrameworkHelper = new FrameworkHelper(mWriterHelper,
                                                new UKDPPFramework(mWriterHelper->GetClip()->GetHeaderMetadata()));
            }
            BMX_CHECK_M(mUKDPPFrameworkHelper->SetProperty(mFrameworkProperties[i].name, mFrameworkProperties[i].value),
                        ("Failed to set UKDPPCoreFramework property '%s' to '%s'",
                         mFrameworkProperties[i].name.c_str(), mFrameworkProperties[i].value.c_str()));
        }
    }


    if (mAS11FrameworkHelper) {
        mWriterHelper->InsertAS11CoreFramework(dynamic_cast<AS11CoreFramework*>(mAS11FrameworkHelper->GetFramework()));
        BMX_CHECK_M(mAS11FrameworkHelper->GetFramework()->validate(true), ("AS11 Framework validation failed"));
    }

    if (!mSegments.empty())
        mWriterHelper->InsertTCSegmentation(mSegments);

    if (mUKDPPFrameworkHelper) {
        mWriterHelper->InsertUKDPPFramework(dynamic_cast<UKDPPFramework*>(mUKDPPFrameworkHelper->GetFramework()));

        // make sure UKDPPTotalNumberOfParts and UKDPPTotalProgrammeDuration are set (default 0) for validation
        UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(mUKDPPFrameworkHelper->GetFramework());
        if (dpp_framework->haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalNumberOfParts)))
            mHaveUKDPPTotalNumberOfParts = true;
        else
            dpp_framework->SetTotalNumberOfParts(0);
        if (dpp_framework->haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalProgrammeDuration)))
            mHaveUKDPPTotalProgrammeDuration = true;
        else
            dpp_framework->SetTotalProgrammeDuration(0);

        BMX_CHECK_M(dpp_framework->validate(true), ("UK DPP Framework validation failed"));
    }
}

void AS11Helper::Complete()
{
    BMX_ASSERT(mWriterHelper);

    if (!mSegments.empty())
        mWriterHelper->CompleteSegmentation(mFillerCompleteSegments);

    if (mUKDPPFrameworkHelper) {
        // calculate or check total number of parts and programme duration
        UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(mUKDPPFrameworkHelper->GetFramework());
        if (mHaveUKDPPTotalNumberOfParts) {
            BMX_CHECK_M(mWriterHelper->GetTotalSegments() == dpp_framework->GetTotalNumberOfParts(),
                        ("UKDPPTotalNumberOfParts value %u does not equal actual total part count %u",
                         dpp_framework->GetTotalNumberOfParts(), mWriterHelper->GetTotalSegments()));
        } else {
            dpp_framework->SetTotalNumberOfParts(mWriterHelper->GetTotalSegments());
        }
        if (mHaveUKDPPTotalProgrammeDuration) {
            BMX_CHECK_M(dpp_framework->GetTotalProgrammeDuration() >= mWriterHelper->GetTotalSegmentDuration(),
                        ("UKDPPTotalProgrammeDuration value %"PRId64" is less than duration of parts in this "
                         "file %"PRId64,
                         dpp_framework->GetTotalProgrammeDuration(), mWriterHelper->GetTotalSegmentDuration()));
        } else {
            dpp_framework->SetTotalProgrammeDuration(mWriterHelper->GetTotalSegmentDuration());
        }
    }
}

bool AS11Helper::ParseFrameworkType(const char *type_str, FrameworkType *type) const
{
    if (strcmp(type_str, "as11") == 0) {
        *type = AS11_CORE_FRAMEWORK_TYPE;
        return true;
    } else if (strcmp(type_str, "dpp") == 0) {
        *type = DPP_FRAMEWORK_TYPE;
        return true;
    }

    log_warn("Unknown framework type '%s'\n", type_str);
    return false;
}

void AS11Helper::SetFrameworkProperty(FrameworkType type, std::string name, std::string value)
{
    FrameworkProperty framework_property;
    framework_property.type  = type;
    framework_property.name  = name;
    framework_property.value = value;

    mFrameworkProperties.push_back(framework_property);
}

