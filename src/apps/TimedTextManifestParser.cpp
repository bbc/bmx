/*
 * Copyright (C) 2018, British Broadcasting Corporation
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

#include <bmx/apps/TimedTextManifestParser.h>

#include <stdio.h>

#include <bmx/apps/PropertyFileParser.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/Utils.h>
#include <bmx/URI.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


typedef enum
{
    PARSE_TT_FILE_STATE,
    PARSE_RESOURCE_FILE_STATE,
} ParseState;

typedef struct
{
    TimedTextProfile profile;
    const char *suffix;
} IMSCProfileSuffixMap;

static IMSCProfileSuffixMap IMSC_PROFILE_SUFFIX_MAP[] =
{
    {IMSC_1_TEXT_PROFILE,      "imsc1/text"},
    {IMSC_1_IMAGE_PROFILE,     "imsc1/image"},
    {IMSC_1_1_TEXT_PROFILE,    "imsc1.1/text"},
    {IMSC_1_1_IMAGE_PROFILE,   "imsc1.1/image"},
};




static bool create_abs_file_path(const string &manifest_filename, const string &item_filename,
                                 string *abs_item_file_path)
{
    // convert a relative (to the manifest file) filename into a absolute file path
    URI item_uri;
    if (!item_uri.ParseFilename(item_filename)) {
        log_error("Failed to parse 'file' into URI from timed text manifest\n");
        return false;
    }
    if (item_uri.IsRelative()) {
        URI mf_uri;
        mf_uri.ParseFilename(manifest_filename);

        if (mf_uri.IsRelative()) {
            URI cwd_uri;
            cwd_uri.ParseDirectory(get_cwd());
            mf_uri.MakeAbsolute(cwd_uri);
        }

        item_uri.MakeAbsolute(mf_uri);
    }

    *abs_item_file_path = item_uri.ToFilename();
    return true;
}

static bool parse_profile(const string &suffix, TimedTextProfile *profile)
{
    for (size_t i = 0; i < BMX_ARRAY_SIZE(IMSC_PROFILE_SUFFIX_MAP); i++) {
        if (suffix == IMSC_PROFILE_SUFFIX_MAP[i].suffix) {
            *profile = IMSC_PROFILE_SUFFIX_MAP[i].profile;
            return true;
        }
    }

    return false;
}

static bool parse_languages(const string &lang_str, vector<string> *languages_out)
{
    vector<string> languages = split_string(lang_str, ',', true, true);
    size_t i;
    for (i = 0; i < languages.size(); i++) {
        if (languages[i].empty()) {
            log_error("Empty language element\n");
            return false;
        }
    }

    *languages_out = languages;
    return true;
}

static bool parse_position(const string &value, Timecode start_timecode, Rational frame_rate,
                           int64_t *int64_value)
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

    return false;
}


TimedTextManifestParser::TimedTextManifestParser()
: TimedTextManifest()
{
    Reset();
}

TimedTextManifestParser::~TimedTextManifestParser()
{
}

bool TimedTextManifestParser::Parse(const string &filename, Timecode start_tc, Rational frame_rate)
{
    Reset();

    PropertyFileParser property_parser;
    if (!property_parser.Open(filename)) {
        log_error("Failed to open timed text manifest\n");
        return false;
    }


    ParseState parse_state = PARSE_TT_FILE_STATE;
    string mf_tt_filename;
    string name;
    string value;
    while (property_parser.ParseNext(&name, &value)) {
        if (parse_state == PARSE_TT_FILE_STATE) {
            if (name == "file") {
                mf_tt_filename = value;
            } else if (name == "profile") {
                if (!parse_profile(value, &mProfile)) {
                    log_error("Failed to parse timed text profile '%s'\n", value.c_str());
                    return false;
                }
            } else if (name == "encoding") {
                mEncoding = value;
            } else if (name == "resource_id") {
                if (!parse_uuid(value.c_str(), &mResourceId)) {
                    log_error("Failed to parse timed text resource_id '%s'\n", value.c_str());
                    return false;
                }
            } else if (name == "languages") {
                if (!parse_languages(value, &mLanguages)) {
                    log_error("Failed to parse timed text languages '%s'\n", value.c_str());
                    return false;
                }
            } else if (name == "start") {
                if (!parse_position(value, start_tc, frame_rate, &mStart)) {
                    log_error("Failed to parse timed text start '%s'\n", value.c_str());
                    return false;
                }
            } else if (name == "resource_file") {
                TimedTextAncillaryResource new_resource;
                new_resource.filename = value;
                new_resource.resource_id = g_Null_UUID;
                mAncillaryResources.push_back(new_resource);
                parse_state = PARSE_RESOURCE_FILE_STATE;
            } else {
                log_error("Unknown timed text file property '%s'\n", name.c_str());
                return false;
            }
        } else if (parse_state == PARSE_RESOURCE_FILE_STATE) {
            TimedTextAncillaryResource *resource = &mAncillaryResources.back();
            if (name == "resource_file") {
                TimedTextAncillaryResource new_resource;
                new_resource.filename = value;
                new_resource.resource_id = g_Null_UUID;
                mAncillaryResources.push_back(new_resource);
            } else if (name == "resource_id") {
                if (!parse_uuid(value.c_str(), &resource->resource_id)) {
                    log_error("Failed to parse timed text resource resource_id '%s'\n", value.c_str());
                    return false;
                }
            } else if (name == "mime_type") {
                resource->mime_type = value;
            } else {
                log_error("Unknown timed text resource file property '%s'\n", name.c_str());
                return false;
            }
        }
    }
    if (property_parser.HaveError()) {
        return false;
    }

    if (mf_tt_filename.empty()) {
        log_error("Empty or missing 'file' in timed text manifest\n");
        return false;
    }
    if (mEncoding.empty()) {
        log_error("Empty or missing 'encoding' in timed text manifest\n");
        return false;
    }
    size_t i;
    for (i = 0; i < mAncillaryResources.size(); i++) {
        TimedTextAncillaryResource *resource = &mAncillaryResources[i];
        if (resource->filename.empty()) {
            log_error("Empty 'resource_file' in timed text manifest resource section\n");
            return false;
        }
        if (resource->resource_id == g_Null_UUID) {
            log_error("Empty or missing 'resource_id' in timed text manifest resource section\n");
            return false;
        }
        if (resource->mime_type.empty()) {
            log_error("Empty or missing 'mime_type' in timed text manifest resource section\n");
            return false;
        }
    }

    if (!create_abs_file_path(filename, mf_tt_filename, &mTTFilename)) {
        return false;
    }
    for (i = 0; i < mAncillaryResources.size(); i++) {
        if (!create_abs_file_path(filename, mAncillaryResources[i].filename, &mAncillaryResources[i].filename)) {
            return false;
        }
    }

    return true;
}

bool TimedTextManifestParser::CheckCanReadTTFile()
{
    FILE *file = fopen(mTTFilename.c_str(), "rb");
    if (!file)
        return false;

    fclose(file);
    return true;
}
