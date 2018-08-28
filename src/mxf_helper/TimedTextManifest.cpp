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

#include <bmx/mxf_helper/TimedTextManifest.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const char *TEXT_PROFILE_STRING = "http://www.w3.org/ns/ttml/profile/imsc1/text";
static const char *IMAGE_PROFILE_STRING = "http://www.w3.org/ns/ttml/profile/imsc1/image";


TimedTextAncillaryResource::TimedTextAncillaryResource()
{
    resource_id = g_Null_UUID;
    stream_id = 0;
}



TimedTextManifest::TimedTextManifest()
{
    Reset();
}

TimedTextManifest::~TimedTextManifest()
{
}

string TimedTextManifest::GetProfileDesignator() const
{
    if (mProfile == TT_TEXT_PROFILE)
        return TEXT_PROFILE_STRING;
    else
        return IMAGE_PROFILE_STRING;
}

bool TimedTextManifest::HaveResourceId() const
{
    return mResourceId != g_Null_UUID;
}

string TimedTextManifest::GetLanguagesString() const
{
    string result;
    size_t i;
    for (i = 0; i < mLanguages.size(); i++) {
        if (i != 0)
            result.append(",");
        result.append(mLanguages[i]);
    }

    return result;
}

void TimedTextManifest::SetProfileDesignator(const string &designator)
{
    if (designator == TEXT_PROFILE_STRING) {
        mProfile = TT_TEXT_PROFILE;
    } else if (designator == IMAGE_PROFILE_STRING) {
        mProfile = TT_IMAGE_PROFILE;
    } else {
        BMX_EXCEPTION(("Unsupported timed text profile %s", designator.c_str()));
    }
}

void TimedTextManifest::SetLanguagesString(const string &languages_str)
{
    mLanguages.clear();
    vector<string> languages = split_string(languages_str, ',', true);
    size_t i;
    for (i = 0; i < languages.size(); i++) {
        languages[i] = trim_string(languages[i]);
        if (!languages[i].empty()) {
            mLanguages.push_back(languages[i]);
        }
    }
}

void TimedTextManifest::Reset()
{
    mTTFilename.clear();
    mProfile = TT_TEXT_PROFILE;
    mEncoding.clear();
    mResourceId = g_Null_UUID;
    mLanguages.clear();
    mAncillaryResources.clear();
}
