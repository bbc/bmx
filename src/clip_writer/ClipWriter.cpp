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
using namespace mxfpp;
using namespace bmx;



ClipWriter* ClipWriter::OpenNewAS02Clip(string bundle_directory, bool create_bundle_dir, Rational frame_rate,
                                        MXFFileFactory *file_factory, bool take_factory_ownership)
{
    AS02Bundle *bundle = AS02Bundle::OpenNew(bundle_directory, create_bundle_dir, file_factory, take_factory_ownership);
    return new ClipWriter(bundle, AS02Version::OpenNewPrimary(bundle, frame_rate));
}

ClipWriter* ClipWriter::OpenNewOP1AClip(int flavour, File *file, Rational frame_rate)
{
    return new ClipWriter(new OP1AFile(flavour, file, frame_rate));
}

ClipWriter* ClipWriter::OpenNewAvidClip(int flavour, Rational frame_rate, MXFFileFactory *file_factory,
                                        bool take_factory_ownership, string filename_prefix)
{
    return new ClipWriter(new AvidClip(flavour, frame_rate, file_factory, take_factory_ownership, filename_prefix));
}

ClipWriter* ClipWriter::OpenNewD10Clip(int flavour, File *file, Rational frame_rate)
{
    return new ClipWriter(new D10File(flavour, file, frame_rate));
}

ClipWriter* ClipWriter::OpenNewRDD9Clip(int flavour, File *file, Rational frame_rate)
{
    return new ClipWriter(new RDD9File(flavour, file, frame_rate));
}

ClipWriter* ClipWriter::OpenNewWaveClip(WaveIO *file)
{
    return new ClipWriter(new WaveWriter(file, true));
}

ClipWriter::ClipWriter(AS02Bundle *bundle, AS02Clip *clip)
{
    mType = CW_AS02_CLIP_TYPE;
    mAS02Bundle = bundle;
    mAS02Clip = clip;
    mOP1AClip = 0;
    mAvidClip = 0;
    mD10Clip = 0;
    mRDD9Clip = 0;
    mWaveClip = 0;
}

ClipWriter::ClipWriter(OP1AFile *clip)
{
    mType = CW_OP1A_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mOP1AClip = clip;
    mAvidClip = 0;
    mD10Clip = 0;
    mRDD9Clip = 0;
    mWaveClip = 0;
}

ClipWriter::ClipWriter(AvidClip *clip)
{
    mType = CW_AVID_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mOP1AClip = 0;
    mAvidClip = clip;
    mD10Clip = 0;
    mRDD9Clip = 0;
    mWaveClip = 0;
}

ClipWriter::ClipWriter(D10File *clip)
{
    mType = CW_D10_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mOP1AClip = 0;
    mAvidClip = 0;
    mD10Clip = clip;
    mRDD9Clip = 0;
    mWaveClip = 0;
}

ClipWriter::ClipWriter(RDD9File *clip)
{
    mType = CW_RDD9_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mOP1AClip = 0;
    mAvidClip = 0;
    mD10Clip = 0;
    mRDD9Clip = clip;
    mWaveClip = 0;
}

ClipWriter::ClipWriter(WaveWriter *clip)
{
    mType = CW_WAVE_CLIP_TYPE;
    mAS02Bundle = 0;
    mAS02Clip = 0;
    mOP1AClip = 0;
    mAvidClip = 0;
    mD10Clip = 0;
    mRDD9Clip = 0;
    mWaveClip = clip;
}

ClipWriter::~ClipWriter()
{
    delete mAS02Bundle;
    delete mAS02Clip;
    delete mOP1AClip;
    delete mAvidClip;
    delete mD10Clip;
    delete mRDD9Clip;
    delete mWaveClip;

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
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->SetClipName(name);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetClipName(name);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->SetClipName(name);
            break;
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->SetClipName(name);
            break;
        case CW_WAVE_CLIP_TYPE:
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
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->SetStartTimecode(start_timecode);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetStartTimecode(start_timecode);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->SetStartTimecode(start_timecode);
            break;
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->SetStartTimecode(start_timecode);
            break;
        case CW_WAVE_CLIP_TYPE:
            mWaveClip->SetStartTimecode(start_timecode);
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::SetAuxiliaryTimecodes(const vector<Timecode> &aux_timecodes)
{
    switch (mType)
    {
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetAuxiliaryTimecodes(aux_timecodes);
            break;
        default:
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
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->SetProductInfo(company_name, product_name, product_version, version, product_uid);
            break;
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::SetCreationDate(mxfTimestamp creation_date)
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->SetCreationDate(creation_date);
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->SetCreationDate(creation_date);
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->SetCreationDate(creation_date);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->SetCreationDate(creation_date);
            break;
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->SetCreationDate(creation_date);
            break;
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::ReserveHeaderMetadataSpace(uint32_t min_bytes)
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->ReserveHeaderMetadataSpace(min_bytes);
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->ReserveHeaderMetadataSpace(min_bytes);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->ReserveHeaderMetadataSpace(min_bytes);
            break;
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->ReserveHeaderMetadataSpace(min_bytes);
            break;
        case CW_AVID_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::ForceWriteCBEDuration0(bool enable)
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->ForceWriteCBEDuration0(enable);
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->ForceWriteCBEDuration0(enable);
            break;
        case CW_RDD9_CLIP_TYPE:
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

uint32_t ClipWriter::AddWaveChunk(WaveChunk *chunk, bool take_ownership)
{
    uint32_t id = 0;

    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            id = mOP1AClip->AddWaveChunk(chunk, take_ownership);
            break;
        case CW_WAVE_CLIP_TYPE:
            mWaveClip->AddChunk(chunk, take_ownership);
            // No id and so 0 is always returned
            break;
        case CW_D10_CLIP_TYPE:
        case CW_RDD9_CLIP_TYPE:
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return id;
}

uint32_t ClipWriter::AddADMWaveChunk(WaveChunk *chunk, bool take_ownership, const std::vector<UL> &profile_and_level_uls)
{
    uint32_t id = 0;

    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            id = mOP1AClip->AddADMWaveChunk(chunk, take_ownership, profile_and_level_uls);
            break;
        case CW_WAVE_CLIP_TYPE:
            // It is the same as any other chunk
            mWaveClip->AddChunk(chunk, take_ownership);
            // No id and so 0 is always returned
            break;
        case CW_D10_CLIP_TYPE:
        case CW_RDD9_CLIP_TYPE:
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return id;
}

ClipWriterTrack* ClipWriter::CreateTrack(EssenceType essence_type, string track_filename)
{
    ClipWriterTrack *track = 0;
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            track = new ClipWriterTrack(essence_type, mAS02Clip->CreateTrack(essence_type));
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
        case CW_RDD9_CLIP_TYPE:
            track = new ClipWriterTrack(essence_type, mRDD9Clip->CreateTrack(essence_type));
            break;
        case CW_WAVE_CLIP_TYPE:
            BMX_CHECK(essence_type == WAVE_PCM);
            track = new ClipWriterTrack(essence_type, mWaveClip->CreateTrack());
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    mTracks.push_back(track);
    return track;
}

ClipWriterTrack* ClipWriter::CreateXMLTrack()
{
    ClipWriterTrack *track = 0;
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            track = new ClipWriterTrack(mOP1AClip->CreateXMLTrack());
            break;
        case CW_D10_CLIP_TYPE:
            track = new ClipWriterTrack(mD10Clip->CreateXMLTrack());
            break;
        case CW_RDD9_CLIP_TYPE:
            track = new ClipWriterTrack(mRDD9Clip->CreateXMLTrack());
            break;
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    mTracks.push_back(track);
    return track;
}

void ClipWriter::PrepareHeaderMetadata()
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->PrepareHeaderMetadata();
            break;
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->PrepareHeaderMetadata();
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->PrepareHeaderMetadata();
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->PrepareHeaderMetadata();
            break;
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->PrepareHeaderMetadata();
            break;
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::PrepareWrite()
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            mAS02Clip->PrepareWrite();
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
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->PrepareWrite();
            break;
        case CW_WAVE_CLIP_TYPE:
            mWaveClip->PrepareWrite();
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
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->CompleteWrite();
            break;
        case CW_AVID_CLIP_TYPE:
            mAvidClip->CompleteWrite();
            break;
        case CW_D10_CLIP_TYPE:
            mD10Clip->CompleteWrite();
            break;
        case CW_RDD9_CLIP_TYPE:
            mRDD9Clip->CompleteWrite();
            break;
        case CW_WAVE_CLIP_TYPE:
            mWaveClip->CompleteWrite();
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

void ClipWriter::UpdateHeaderMetadata()
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            mOP1AClip->UpdateHeaderMetadata();
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }
}

HeaderMetadata* ClipWriter::GetHeaderMetadata() const
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetHeaderMetadata();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetHeaderMetadata();
        case CW_RDD9_CLIP_TYPE:
            return mRDD9Clip->GetHeaderMetadata();
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return 0;
}

DataModel* ClipWriter::GetDataModel() const
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetDataModel();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetDataModel();
        case CW_RDD9_CLIP_TYPE:
            return mRDD9Clip->GetDataModel();
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return 0;
}

UniqueIdHelper* ClipWriter::GetTrackIdHelper()
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetTrackIdHelper();
        case CW_RDD9_CLIP_TYPE:
            return mRDD9Clip->GetTrackIdHelper();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetTrackIdHelper();
        case CW_AS02_CLIP_TYPE:
            return mAS02Clip->GetTrackIdHelper();
        case CW_AVID_CLIP_TYPE:
            return mAvidClip->GetTrackIdHelper();
        case CW_WAVE_CLIP_TYPE:
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return 0; // for the compiler
}

UniqueIdHelper* ClipWriter::GetStreamIdHelper()
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetStreamIdHelper();
        case CW_RDD9_CLIP_TYPE:
            return mRDD9Clip->GetStreamIdHelper();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetStreamIdHelper();
        case CW_AS02_CLIP_TYPE:
            return mAS02Clip->GetStreamIdHelper();
        case CW_AVID_CLIP_TYPE:
            return mAvidClip->GetStreamIdHelper();
        case CW_WAVE_CLIP_TYPE:
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return 0; // for the compiler
}

Rational ClipWriter::GetFrameRate() const
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            return mAS02Clip->GetFrameRate();
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetFrameRate();
        case CW_AVID_CLIP_TYPE:
            return mAvidClip->GetFrameRate();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetFrameRate();
        case CW_RDD9_CLIP_TYPE:
            return mRDD9Clip->GetFrameRate();
        case CW_WAVE_CLIP_TYPE:
            return mWaveClip->GetSamplingRate();
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return ZERO_RATIONAL;
}

Timecode ClipWriter::GetStartTimecode() const
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetStartTimecode();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetStartTimecode();
        case CW_RDD9_CLIP_TYPE:
            return mRDD9Clip->GetStartTimecode();
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return Timecode();
}

int64_t ClipWriter::GetDuration() const
{
    switch (mType)
    {
        case CW_AS02_CLIP_TYPE:
            return mAS02Clip->GetDuration();
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetDuration();
        case CW_AVID_CLIP_TYPE:
            return mAvidClip->GetDuration();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetDuration();
        case CW_RDD9_CLIP_TYPE:
            return mRDD9Clip->GetDuration();
        case CW_WAVE_CLIP_TYPE:
            return mWaveClip->GetDuration();
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return 0;
}

int64_t ClipWriter::GetInputDuration() const
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetInputDuration();
        case CW_D10_CLIP_TYPE:
            return mD10Clip->GetInputDuration();
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_RDD9_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    return -1;
}

uint32_t ClipWriter::GetWaveChunkStreamID(WaveChunkId chunk_id)
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetWaveChunkStreamID(chunk_id);
        case CW_D10_CLIP_TYPE:
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_RDD9_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    BMX_EXCEPTION(("Clip type does not support Wave chunks"));
    return 0;
}

uint32_t ClipWriter::GetADMWaveChunkStreamID(WaveChunkId chunk_id)
{
    switch (mType)
    {
        case CW_OP1A_CLIP_TYPE:
            return mOP1AClip->GetADMWaveChunkStreamID(chunk_id);
        case CW_D10_CLIP_TYPE:
        case CW_AS02_CLIP_TYPE:
        case CW_AVID_CLIP_TYPE:
        case CW_RDD9_CLIP_TYPE:
        case CW_WAVE_CLIP_TYPE:
            break;
        case CW_UNKNOWN_CLIP_TYPE:
            BMX_ASSERT(false);
            break;
    }

    BMX_EXCEPTION(("Clip type does not support Wave chunks"));
    return 0;
}

ClipWriterTrack* ClipWriter::GetTrack(uint32_t track_index)
{
    BMX_CHECK(track_index < mTracks.size());
    return mTracks[track_index];
}

