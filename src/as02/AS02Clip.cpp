/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <algorithm>

#include <bmx/as02/AS02Clip.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Version.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static bool compare_track(AS02Track *left, AS02Track *right)
{
    return left->IsPicture() && !right->IsPicture();
}


AS02Clip::AS02Clip(AS02Bundle *bundle, string filepath, mxfRational frame_rate)
{
    BMX_CHECK(frame_rate == FRAME_RATE_23976 ||
              frame_rate == FRAME_RATE_24 ||
              frame_rate == FRAME_RATE_25 ||
              frame_rate == FRAME_RATE_2997 ||
              frame_rate == FRAME_RATE_30 ||
              frame_rate == FRAME_RATE_50 ||
              frame_rate == FRAME_RATE_5994 ||
              frame_rate == FRAME_RATE_60);

    mBundle = bundle;
    mClipFilename = strip_path(filepath);
    mClipFrameRate = frame_rate;
    mStartTimecode = Timecode(frame_rate, false);
    mCompanyName = get_bmx_company_name();
    mProductName = get_bmx_library_name();
    mProductVersion = get_bmx_mxf_product_version();
    mVersionString = get_bmx_mxf_version_string();
    mProductUID = get_bmx_product_uid();
    mReserveMinBytes = 8192;
    mxf_get_timestamp_now(&mCreationDate);
    mxf_generate_uuid(&mGenerationUID);
    mNextVideoTrackNumber = 1;
    mNextAudioTrackNumber = 1;
}

AS02Clip::~AS02Clip()
{
    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];
}

void AS02Clip::SetClipName(string name)
{
    mClipName = name;
}

void AS02Clip::SetStartTimecode(Timecode start_timecode)
{
    mStartTimecode = start_timecode;
}

void AS02Clip::SetProductInfo(string company_name, string product_name, mxfProductVersion product_version,
                              string version, mxfUUID product_uid)
{
    mCompanyName = company_name;
    mProductName = product_name;
    mProductVersion = product_version;
    mVersionString = version;
    mProductUID = product_uid;
}

void AS02Clip::SetCreationDate(mxfTimestamp creation_date)
{
    mCreationDate = creation_date;
}

void AS02Clip::SetGenerationUID(mxfUUID generation_uid)
{
    mGenerationUID = generation_uid;
}

void AS02Clip::ReserveHeaderMetadataSpace(uint32_t min_bytes)
{
    mReserveMinBytes = min_bytes;
}

AS02Track* AS02Clip::CreateTrack(EssenceType essence_type)
{
    bool is_video = (essence_type != WAVE_PCM);
    string filepath, rel_uri;
    uint32_t track_number;
    if (is_video) {
        track_number = mNextVideoTrackNumber;
        mNextVideoTrackNumber++;
    } else {
        track_number = mNextAudioTrackNumber;
        mNextAudioTrackNumber++;
    }
    filepath = mBundle->CreateEssenceComponentFilepath(mClipFilename, is_video, track_number, &rel_uri);

    mTracks.push_back(AS02Track::OpenNew(this, mBundle->GetFileFactory()->OpenNew(filepath), rel_uri,
                                         (uint32_t)mTracks.size(), essence_type));
    mTrackMap[mTracks.back()->GetTrackIndex()] = mTracks.back();

    return mTracks.back();
}

void AS02Clip::PrepareWrite()
{
    mReserveMinBytes += 256; // account for extra bytes when updating header metadata

    // sort tracks, picture followed by sound
    stable_sort(mTracks.begin(), mTracks.end(), compare_track);

    uint32_t last_picture_track_number = 0;
    uint32_t last_sound_track_number = 0;
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (mTracks[i]->IsPicture()) {
            if (!mTracks[i]->IsOutputTrackNumberSet())
                mTracks[i]->SetOutputTrackNumber(last_picture_track_number + 1);
            last_picture_track_number = mTracks[i]->GetOutputTrackNumber();
        } else {
            if (!mTracks[i]->IsOutputTrackNumberSet())
                mTracks[i]->SetOutputTrackNumber(last_sound_track_number + 1);
            last_sound_track_number = mTracks[i]->GetOutputTrackNumber();
        }
    }

    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->PrepareWrite();
}

void AS02Clip::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_CHECK(track_index < mTracks.size());

    mTrackMap[track_index]->WriteSamples(data, size, num_samples);
}

void AS02Clip::CompleteWrite()
{
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        BMX_CHECK_M(mTracks[i]->HasValidDuration(),
                   ("Invalid start/end offsets. Track %"PRIszt" has duration that is too small"));
    }

    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->CompleteWrite();
}

int64_t AS02Clip::GetDuration() const
{
    int64_t min_duration = -1;
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (min_duration < 0 || mTracks[i]->GetOutputDuration(true) < min_duration)
            min_duration = mTracks[i]->GetOutputDuration(true);
    }

    return (min_duration < 0 ? 0 : min_duration);
}

AS02Track* AS02Clip::GetTrack(uint32_t track_index)
{
    BMX_ASSERT(track_index < mTracks.size());

    return mTrackMap[track_index];
}

