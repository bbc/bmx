/*
 * Copyright (C) 2019, British Broadcasting Corporation
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

#include <bmx/mxf_helper/TimedTextMXFResourceProvider.h>

#define __STDC_LIMIT_MACROS

#include <limits.h>
#include <stdio.h>

#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


TimedTextMXFResourceProvider::TimedTextMXFResourceProvider(mxfpp::File *file)
{
    mFile = file;
    mRemSize = 0;
    mRangeIndex = 0;
    mRangeRemSize = 0;
}

TimedTextMXFResourceProvider::~TimedTextMXFResourceProvider()
{
    delete mFile;
}

void TimedTextMXFResourceProvider::AddTimedTextResource(const vector<std::pair<int64_t, int64_t> > &ranges)
{
    mTimedTextResource = ranges;
}

void TimedTextMXFResourceProvider::AddAncillaryResource(uint32_t stream_id,
                                                        const vector<std::pair<int64_t, int64_t> > &ranges)
{
    mAncillaryResources[stream_id] = ranges;
}

int64_t TimedTextMXFResourceProvider::GetTimedTextResourceSize()
{
    int64_t size = 0;
    size_t i;
    for (i = 0; i < mTimedTextResource.size(); i++) {
        size += mTimedTextResource[i].second;
    }

    return size;
}

int64_t TimedTextMXFResourceProvider::GetAncillaryResourceSize(uint32_t stream_id)
{
    const vector<pair<int64_t, int64_t> > &ranges = mAncillaryResources.at(stream_id);
    int64_t size = 0;
    size_t i;
    for (i = 0; i < ranges.size(); i++) {
        size += ranges[i].second;
    }

    return size;
}

void TimedTextMXFResourceProvider::OpenTimedTextResource()
{
    mRemSize = GetTimedTextResourceSize();
    mRangeIndex = 0;
    mRangeRemSize = 0;
    mOpenRanges = mTimedTextResource;

    if (!mOpenRanges.empty()) {
        mRangeRemSize = mOpenRanges[0].second;
        mFile->seek(mOpenRanges[0].first, SEEK_SET);
    }
}

void TimedTextMXFResourceProvider::OpenAncillaryResource(uint32_t stream_id)
{
    mRemSize = GetAncillaryResourceSize(stream_id);
    mRangeIndex = 0;
    mRangeRemSize = 0;
    mOpenRanges = mAncillaryResources.at(stream_id);

    if (!mOpenRanges.empty()) {
        mRangeRemSize = mOpenRanges[0].second;
        mFile->seek(mOpenRanges[0].first, SEEK_SET);
    }
}

size_t TimedTextMXFResourceProvider::Read(unsigned char *buffer, size_t size)
{
    size_t target_read = size;
    if ((int64_t)target_read > mRemSize) {
        target_read = (size_t)mRemSize;
    }

    if (target_read == 0) {
        return 0;
    }

    size_t total_read = 0;
    while (total_read < target_read) {
        if (mRangeRemSize == 0 && mRangeIndex < mOpenRanges.size()) {
            mRangeIndex++;
            mRangeRemSize = mOpenRanges[mRangeIndex].second;
            mFile->seek(mOpenRanges[mRangeIndex].first, SEEK_SET);
        }

        uint32_t to_read;
        if (mRangeRemSize > UINT32_MAX) {
            to_read = UINT32_MAX;
        } else {
            to_read = (uint32_t)mRangeRemSize;
        }
        if (to_read > target_read - total_read) {
            to_read = (uint32_t)(target_read - total_read);
        }
        BMX_ASSERT(to_read <= mRemSize);
        BMX_ASSERT(to_read > 0);

        uint32_t num_read = mFile->read(&buffer[total_read], to_read);
        if (num_read != to_read) {
            BMX_EXCEPTION(("Failed to read timed text resource data"));
        }

        total_read += num_read;
        mRangeRemSize -= num_read;
        mRemSize -= num_read;
    }

    return total_read;
}
