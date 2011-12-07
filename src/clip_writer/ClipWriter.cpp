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

#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/as02/AS02Version.h>
#include <bmx/Utils.h>
#include <bmx/MXFUtils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


typedef struct
{
    ClipWriterType type;
    const char *str;
} ClipWriterTypeStringMap;

static const ClipWriterTypeStringMap CLIP_WRITER_TYPE_STRING_MAP[] =
{
    {CW_UNKNOWN_CLIP_TYPE,   "unknown"},
    {CW_AS02_CLIP_TYPE,      "AS-02"},
    {CW_AS11_OP1A_CLIP_TYPE, "AS-11 OP-1A"},
    {CW_AS11_D10_CLIP_TYPE,  "AS-11 D-10"},
    {CW_OP1A_CLIP_TYPE,      "MXF OP-1A"},
    {CW_AVID_CLIP_TYPE,      "Avid MXF"},
    {CW_D10_CLIP_TYPE,       "D-10 MXF"},
};



ClipWriter* ClipWriter::OpenNewAS02Clip(string bundle_directory, bool create_bundle_dir, Rational frame_rate)
{
    AS02Bundle *bundle = AS02Bundle::OpenNew(bundle_directory, create_bundle_dir);
    return new ClipWriter(bundle, AS02Version::OpenNewPrimary(bundle, frame_rate));
}

ClipWriter* ClipWriter::OpenNewAS11OP1AClip(string filename, Rational frame_rate)
{
    return new ClipWriter(AS11Clip::OpenNewOP1AClip(filename, frame_rate));
}

ClipWriter* ClipWriter::OpenNewAS11D10Clip(string filename, Rational frame_rate)
{
    return new ClipWriter(AS11Clip::OpenNewD10Clip(filename, frame_rate));
}

ClipWriter* ClipWriter::OpenNewOP1AClip(int flavour, string filename, Rational frame_rate)
{
    return new ClipWriter(OP1AFile::OpenNew(flavour, filename, frame_rate));
}

ClipWriter* ClipWriter::OpenNewAvidClip(Rational frame_rate, string filename_prefix)
{
    return new ClipWriter(new AvidClip(frame_rate, filename_prefix));
}

ClipWriter* ClipWriter::OpenNewD10Clip(string filename, Rational frame_rate)
{
    return new ClipWriter(D10File::OpenNew(filename, frame_rate));
}

string ClipWriter::ClipWriterTypeToString(ClipWriterType clip_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(CLIP_WRITER_TYPE_STRING_MAP); i++) {
        if (clip_type == CLIP_WRITER_TYPE_STRING_MAP[i].type)
            return CLIP_WRITER_TYPE_STRING_MAP[i].str;
    }

    BMX_ASSERT(false);
    return "";
}

ClipWriter::ClipWriter(AS02Bundle *bundle, AS02Clip *clip)
{
    mType = CW_AS02_CLIP_TYPE;
    mAS02Bundle = bundle;
    mAS02Clip = clip;
    mAS11Clip = 0;
    mOP1AClip = 0;
    mAvidClip = 0;
    mD10Clip = 0;
}

ClipWriter::ClipWriter(AS11Clip *clip)
{
    if (clip->GetType() == AS11_OP1A_CLIP_TYPE)
        mType = CW_AS11_OP1A_CLIP_TYPE;
    else
        mType = CW_AS11_D10_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mAS11Clip = clip;
    mOP1AClip = 0;
    mAvidClip = 0;
    mD10Clip = 0;
}

ClipWriter::ClipWriter(OP1AFile *clip)
{
    mType = CW_OP1A_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mAS11Clip = 0;
    mOP1AClip = clip;
    mAvidClip = 0;
    mD10Clip = 0;
}

ClipWriter::ClipWriter(AvidClip *clip)
{
    mType = CW_AVID_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mAS11Clip = 0;
    mOP1AClip = 0;
    mAvidClip = clip;
    mD10Clip = 0;
}

ClipWriter::ClipWriter(D10File *clip)
{
    mType = CW_D10_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mAS11Clip = 0;
    mOP1AClip = 0;
    mAvidClip = 0;
    mD10Clip = clip;
}

ClipWriter::~ClipWriter()
{
    delete mAS02Bundle;
    delete mAS02Clip;
    delete mAS11Clip;
    delete mOP1AClip;
    delete mAvidClip;
    delete mD10Clip;

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];
}

void ClipWriter::SetClipName(string name)
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->SetClipName(name);
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Clip->SetClipName(name);
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->SetClipName(name);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetClipName(name);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->SetClipName(name);
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::SetStartTimecode(Timecode start_timecode)
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->SetStartTimecode(start_timecode);
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Clip->SetStartTimecode(start_timecode);
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->SetStartTimecode(start_timecode);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetStartTimecode(start_timecode);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->SetStartTimecode(start_timecode);
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::SetProductInfo(string company_name, string product_name, mxfProductVersion product_version,
                                string version, mxfUUID product_uid)
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Clip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

ClipWriterTrack* ClipWriter::CreateTrack(EssenceType essence_type, string track_filename)
{
    ClipWriterTrack *track = 0;
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            track = new ClipWriterTrack(essence_type, mAS02Clip->CreateTrack(essence_type));
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            track = new ClipWriterTrack(essence_type, mAS11Clip->CreateTrack(essence_type));
            break;
        case CW_OP1A_CLIP_TYPE:
            track = new ClipWriterTrack(essence_type, mOP1AClip->CreateTrack(essence_type));
            break;
        case CW_AVID_CLIP_TYPE:
            if (track_filename.empty())
                track = new ClipWriterTrack(essence_type, mAvidClip->CreateTrack(essence_type));
            else
                track = new ClipWriterTrack(essence_type, mAvidClip->CreateTrack(essence_type, track_filename));
            break;
        case CW_D10_CLIP_TYPE:
            track = new ClipWriterTrack(essence_type, mD10Clip->CreateTrack(essence_type));
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    mTracks.push_back(track);
    return track;
}

void ClipWriter::PrepareWrite()
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->PrepareWrite();
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Clip->PrepareWrite();
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->PrepareWrite();
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->PrepareWrite();
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->PrepareWrite();
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    ClipWriterTrack *track = GetTrack(track_index);
    track->WriteSamples(data, size, num_samples);
}

void ClipWriter::CompleteWrite()
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->CompleteWrite();
            mAS02Bundle->FinalizeBundle();
            break;
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            mAS11Clip->CompleteWrite();
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->CompleteWrite();
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->CompleteWrite();
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->CompleteWrite();
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

Rational ClipWriter::GetFrameRate() const
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            return mAS02Clip->GetFrameRate();
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            return mAS11Clip->GetFrameRate();
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetFrameRate();
        case CW_AVID_CLIP_TYPE:
            return mAvidClip->GetFrameRate();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetFrameRate();
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return ZERO_RATIONAL;
}

int64_t ClipWriter::GetDuration()
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            return mAS02Clip->GetDuration();
        case CW_AS11_OP1A_CLIP_TYPE:
        case CW_AS11_D10_CLIP_TYPE:
            return mAS11Clip->GetDuration();
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetDuration();
        case CW_AVID_CLIP_TYPE:
            return mAvidClip->GetDuration();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetDuration();
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return 0;
}

ClipWriterTrack* ClipWriter::GetTrack(uint32_t track_index)
{
    BMX_CHECK(track_index < mTracks.size());
    return mTracks[track_index];
}

