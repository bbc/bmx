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
#include <algorithm>

#include <bmx/apps/AS10Helper.h>
#include <bmx/as10/AS10DMS.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const PropertyInfo AS10_CORE_PROPERTY_INFO[] =
{
    {"ShimName",                    MXF_ITEM_K(AS10CoreFramework, AS10ShimName),                0},
    {"Type",                        MXF_ITEM_K(AS10CoreFramework, AS10Type),                    0},
    {"MainTitle",                   MXF_ITEM_K(AS10CoreFramework, AS10MainTitle),               0},
    {"SubTitle",                    MXF_ITEM_K(AS10CoreFramework, AS10SubTitle),                0},
    {"TitleDescription",            MXF_ITEM_K(AS10CoreFramework, AS10TitleDescription),        0},
    {"OrganizationName",            MXF_ITEM_K(AS10CoreFramework, AS10OrganizationName),        0},
    {"PersonName",                  MXF_ITEM_K(AS10CoreFramework, AS10PersonName),              0},
    {"LocationDescription",         MXF_ITEM_K(AS10CoreFramework, AS10LocationDescription),     0},
    {"CommonSpanningID",            MXF_ITEM_K(AS10CoreFramework, AS10CommonSpanningID),        0},
    {"SpanningNumber",              MXF_ITEM_K(AS10CoreFramework, AS10SpanningNumber),          0},
    {"CumulativeDuration",          MXF_ITEM_K(AS10CoreFramework, AS10CumulativeDuration),      0},

    {0, g_Null_Key, 0},
};

static const FrameworkInfo AS10_FRAMEWORK_INFO[] =
{
    {"AS10CoreFramework",           MXF_SET_K(AS10CoreFramework),           AS10_CORE_PROPERTY_INFO},

    {0, g_Null_Key, 0},
};


static string get_short_name(string name)
{
    if (name.compare(0, strlen("AS10"), "AS10") == 0)
        return name.substr(strlen("AS10"));
    else
        return name;
}



AS10Helper::AS10Helper()
{
    mSourceInfo = 0;
    mWriterHelper = 0;
    mAS10FrameworkHelper = 0;
}

AS10Helper::~AS10Helper()
{
    delete mSourceInfo;
    delete mWriterHelper;
    delete mAS10FrameworkHelper;
}

void AS10Helper::ReadSourceInfo(MXFFileReader *source_file)
{
    mSourceInfo = new AS10Info();
    mSourceInfo->Read(source_file->GetHeaderMetadata());

    if (mSourceInfo->core && mSourceInfo->core->haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10MainTitle)))
        mSourceMainTitle = mSourceInfo->core->GetMainTitle();
}

bool AS10Helper::SupportFrameworkType(const char *type_str)
{
    FrameworkType type;
    return ParseFrameworkType(type_str, &type);
}

bool AS10Helper::ParseFrameworkFile(const char *type_str, const char *filename)
{
    FrameworkType type;
    if (!ParseFrameworkType(type_str, &type)) {
        log_warn("Unknown AS-10 framework type '%s'\n", type_str);
        return false;
    }

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

bool AS10Helper::SetFrameworkProperty(const char *type_str, const char *name, const char *value)
{
    FrameworkType type;
    if (!ParseFrameworkType(type_str, &type)) {
        log_warn("Unknown AS-10 framework type '%s'\n", type_str);
        return false;
    }

    SetFrameworkProperty(type, name, value);
    return true;
}

bool AS10Helper::HaveMainTitle() const
{
    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS10_CORE_FRAMEWORK_TYPE &&
            get_short_name(mFrameworkProperties[i].name) == "MainTitle")
        {
            return true;
        }
    }

    return !mSourceMainTitle.empty();
}

string AS10Helper::GetMainTitle() const
{
    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS10_CORE_FRAMEWORK_TYPE &&
            get_short_name(mFrameworkProperties[i].name) == "MainTitle")
        {
            return mFrameworkProperties[i].value;
        }
    }

    return mSourceMainTitle;
}

const char* AS10Helper::GetShimName() const
{
    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS10_CORE_FRAMEWORK_TYPE &&
            get_short_name(mFrameworkProperties[i].name) == "ShimName")
        {
            return mFrameworkProperties[i].value.c_str();
        }
    }
    return NULL;
}

void AS10Helper::AddMetadata(ClipWriter *clip)
{
    if (clip->GetType() != CW_RDD9_CLIP_TYPE) {
        BMX_EXCEPTION(("AS-10 is not supported in clip type '%s'",
                       clip_type_to_string(clip->GetType(), NO_CLIP_SUB_TYPE)));
    }

    mWriterHelper = new AS10WriterHelper(clip);

    Timecode start_tc   = mWriterHelper->GetClip()->GetStartTimecode();
    Rational frame_rate = mWriterHelper->GetClip()->GetFrameRate();

    if (mSourceInfo) {
        if (mSourceInfo->core) {
            AS10CoreFramework::RegisterObjectFactory(clip->GetHeaderMetadata());
            AS10CoreFramework *core_copy =
                dynamic_cast<AS10CoreFramework*>(mSourceInfo->core->clone(clip->GetHeaderMetadata()));
            mAS10FrameworkHelper = new FrameworkHelper(core_copy, AS10_FRAMEWORK_INFO, start_tc, frame_rate);
        }
    }

    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS10_CORE_FRAMEWORK_TYPE) {
            if (!mAS10FrameworkHelper) {
                AS10CoreFramework *core_fw = new AS10CoreFramework(mWriterHelper->GetClip()->GetHeaderMetadata());
                mAS10FrameworkHelper = new FrameworkHelper(core_fw, AS10_FRAMEWORK_INFO, start_tc, frame_rate);
            }
            BMX_CHECK_M(mAS10FrameworkHelper->SetProperty(get_short_name(mFrameworkProperties[i].name), mFrameworkProperties[i].value),
                        ("Failed to set AS10CoreFramework property '%s' to '%s'",
                         mFrameworkProperties[i].name.c_str(), mFrameworkProperties[i].value.c_str()));
        }
    }


    if (mAS10FrameworkHelper) {
        mWriterHelper->InsertAS10CoreFramework(dynamic_cast<AS10CoreFramework*>(mAS10FrameworkHelper->GetFramework()));
        BMX_CHECK_M(mAS10FrameworkHelper->GetFramework()->validate(true), ("AS10 Framework validation failed"));
    }

}

void AS10Helper::Complete()
{
    BMX_ASSERT(mWriterHelper);
}

bool AS10Helper::ParseFrameworkType(const char *type_str, FrameworkType *type) const
{
    if (strcmp(type_str, "as10") == 0) {
        *type = AS10_CORE_FRAMEWORK_TYPE;
        return true;
    } else {
        return false;
    }
}

void AS10Helper::SetFrameworkProperty(FrameworkType type, string name, string value)
{
    FrameworkProperty framework_property;
    framework_property.type = type;
    framework_property.name = name;
    framework_property.value = value;

    mFrameworkProperties.push_back(framework_property);
}
