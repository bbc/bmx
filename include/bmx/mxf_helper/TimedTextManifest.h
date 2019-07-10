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

#ifndef BMX_TIMED_TEXT_MANIFEST_H_
#define BMX_TIMED_TEXT_MANIFEST_H_

#include <string>
#include <vector>

#include <bmx/BMXTypes.h>


namespace bmx
{


typedef enum
{
    IMSC_1_TEXT_PROFILE,
    IMSC_1_IMAGE_PROFILE,
    IMSC_1_1_TEXT_PROFILE,
    IMSC_1_1_IMAGE_PROFILE,
} TimedTextProfile;


class TimedTextAncillaryResource
{
public:
    TimedTextAncillaryResource();

    std::string filename;
    mxfUUID resource_id;
    std::string mime_type;
    uint32_t stream_id;
};


class TimedTextManifest
{
public:
    TimedTextManifest();
    virtual ~TimedTextManifest();

    TimedTextProfile GetProfile() const { return mProfile; }
    std::string GetProfileDesignator() const;
    std::string GetEncoding() const { return mEncoding; }

    bool HaveResourceId() const;
    UUID GetResourceId() const { return mResourceId; }

    bool HaveLanguages() const { return !mLanguages.empty(); }
    const std::vector<std::string>& GetLanguages() const { return mLanguages; }
    std::string GetLanguagesString() const;

    std::vector<TimedTextAncillaryResource>& GetAncillaryResources() { return mAncillaryResources; }

    std::string GetTTFilename() const { return mTTFilename; }

public:
    void SetProfileDesignator(const std::string &designator);
    void SetLanguagesString(const std::string &languages_str);

public:
    void Reset();

public:
    std::string mTTFilename;
    TimedTextProfile mProfile;
    std::string mEncoding;
    UUID mResourceId;
    std::vector<std::string> mLanguages;
    std::vector<TimedTextAncillaryResource> mAncillaryResources;
    int64_t mStart;
};


};

#endif
