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

#include <cstdio>

#include <set>
#include <memory>

#include <im/as02/AS02Version.h>
#include <im/mxf_helper/MXFDescriptorHelper.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



static const int HEADER_PARTITION   = 0;
static const int FOOTER_PARTITION   = 1;

static uint32_t TIMECODE_TRACK_ID       = 901;
static uint32_t FIRST_VIDEO_TRACK_ID    = 1001;
static uint32_t FIRST_AUDIO_TRACK_ID    = 2001;

static const char TIMECODE_TRACK_NAME[]     = "Timecode";
static const char VIDEO_TRACK_NAME_PREFIX[] = "Video";
static const char AUDIO_TRACK_NAME_PREFIX[] = "Audio";



static string get_version_track_name(const char *prefix, int count)
{
    char buffer[32];
    sprintf(buffer, "%s%d", prefix, count + 1);
    return buffer;
}



AS02Version* AS02Version::OpenNewPrimary(AS02Bundle *bundle, mxfRational frame_rate)
{
    string filepath;
    string rel_uri;
    filepath = bundle->CreatePrimaryVersionFilepath(&rel_uri);
    File *file = File::openNew(filepath);

    return new AS02Version(bundle, filepath, rel_uri, file, frame_rate);
}

AS02Version* AS02Version::OpenNew(AS02Bundle *bundle, string name, mxfRational frame_rate)
{
    string filepath;
    string rel_uri;
    filepath = bundle->CreateVersionFilepath(name, &rel_uri);
    File *file = File::openNew(filepath);

    return new AS02Version(bundle, filepath, rel_uri, file, frame_rate);
}

AS02Version::AS02Version(AS02Bundle *bundle, string filepath, string rel_uri, mxfpp::File *mxf_file,
                         mxfRational frame_rate)
: AS02Clip(bundle, filepath, frame_rate)
{
    mMXFFile = mxf_file;
    mxf_generate_umid(&mMaterialPackageUID);
    mDataModel = 0;
    mHeaderMetadata = 0;
    mHeaderMetadataStartPos = 0;
    mHeaderMetadataEndPos = 0;

    mManifestFile = bundle->GetManifest()->RegisterFile(rel_uri, VERSION_FILE_ROLE);
    mManifestFile->SetId(mMaterialPackageUID);

    // use fill key with correct version number
    g_KLVFill_key = g_CompliantKLVFill_key;
}

AS02Version::~AS02Version()
{
    delete mMXFFile;
    delete mDataModel;
    delete mHeaderMetadata;
}

void AS02Version::PrepareWrite()
{
    AS02Clip::PrepareWrite();

    CreateHeaderMetadata();
    CreateFile();
}

void AS02Version::CompleteWrite()
{
    IM_ASSERT(mMXFFile);

    AS02Clip::CompleteWrite();


    // update metadata sets with duration

    UpdatePackageMetadata();


    // write the footer partition pack

    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer));
    footer_partition.setIndexSID(0);
    footer_partition.setBodySID(0);
    footer_partition.write(mMXFFile);


    // write the RIP

    mMXFFile->writeRIP();



    // re-write the header metadata in the header partition

    mMXFFile->seek(mHeaderMetadataStartPos, SEEK_SET);
    PositionFillerWriter pos_filler_writer(mHeaderMetadataEndPos);
    mHeaderMetadata->write(mMXFFile, &mMXFFile->getPartition(HEADER_PARTITION), &pos_filler_writer);


    // update and re-write the partition packs

    mMXFFile->getPartition(HEADER_PARTITION).setKey(&MXF_PP_K(ClosedComplete, Header));
    mMXFFile->updatePartitions();


    // done with the file
    delete mMXFFile;
    mMXFFile = 0;
}

void AS02Version::CreateHeaderMetadata()
{
    IM_ASSERT(!mHeaderMetadata);

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mEssenceContainerULs.insert(mTracks[i]->GetEssenceContainerUL());


    // create the header metadata
    mDataModel = new DataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);


    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(mCreationDate);
    preface->setVersion(259);   // v1.3 - smpte 377-1
    preface->setOperationalPattern(MXF_OP_L(1b, MultiTrack_Stream_External));
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        preface->appendEssenceContainers(*iter);
    preface->setDMSchemes(vector<mxfUL>());

    // Preface - Identification
    Identification *ident = new Identification(mHeaderMetadata);
    preface->appendIdentifications(ident);
    ident->initialise(mCompanyName, mProductName, mVersionString, mProductUID);
    ident->setProductVersion(mProductVersion);
    ident->setModificationDate(mCreationDate);
    ident->setThisGenerationUID(mGenerationUID);

    // Preface - ContentStorage
    ContentStorage* content_storage = new ContentStorage(mHeaderMetadata);
    preface->setContentStorage(content_storage);

    // Preface - ContentStorage - MaterialPackage
    mMaterialPackage = new MaterialPackage(mHeaderMetadata);
    content_storage->appendPackages(mMaterialPackage);
    mMaterialPackage->setPackageUID(mMaterialPackageUID);
    mMaterialPackage->setPackageCreationDate(mCreationDate);
    mMaterialPackage->setPackageModifiedDate(mCreationDate);
    if (!mClipName.empty())
        mMaterialPackage->setName(mClipName);

    // Preface - ContentStorage - MaterialPackage - Timecode Track
    Track *timecode_track = new Track(mHeaderMetadata);
    mMaterialPackage->appendTracks(timecode_track);
    timecode_track->setTrackName(TIMECODE_TRACK_NAME);
    timecode_track->setTrackID(TIMECODE_TRACK_ID);
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(mClipFrameRate);
    timecode_track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timecode Track - Sequence
    Sequence *sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(-1); // updated when writing completed

    // Preface - ContentStorage - MaterialPackage - Timecode Track - TimecodeComponent
    TimecodeComponent *timecode_component = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(timecode_component);
    timecode_component->setDataDefinition(MXF_DDEF_L(Timecode));
    timecode_component->setDuration(-1); // updated when writing completed
    timecode_component->setRoundedTimecodeBase(mStartTimecode.GetRoundedTCBase());
    timecode_component->setDropFrame(mStartTimecode.IsDropFrame());
    timecode_component->setStartTimecode(mStartTimecode.GetOffset());

    int video_count = 0, audio_count = 0;
    for (i = 0; i < mTracks.size(); i++) {
        // Preface - ContentStorage - MaterialPackage - Timeline Track
        Track *track = new Track(mHeaderMetadata);
        mMaterialPackage->appendTracks(track);
        track->setTrackName(mTracks[i]->IsPicture() ?
                                get_version_track_name(VIDEO_TRACK_NAME_PREFIX, video_count) :
                                get_version_track_name(AUDIO_TRACK_NAME_PREFIX, audio_count));
        track->setTrackID(mTracks[i]->IsPicture() ?
                            FIRST_VIDEO_TRACK_ID + video_count :
                            FIRST_AUDIO_TRACK_ID + audio_count);
        track->setTrackNumber(mTracks[i]->GetClipTrackNumber());
        track->setEditRate(mTracks[i]->GetSampleRate());
        track->setOrigin(0);

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
        sequence = new Sequence(mHeaderMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(mTracks[i]->IsPicture() ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sequence->setDuration(-1); // updated when writing completed

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
        SourceClip *source_clip = new SourceClip(mHeaderMetadata);
        sequence->appendStructuralComponents(source_clip);
        source_clip->setDataDefinition(mTracks[i]->IsPicture() ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        source_clip->setDuration(-1); // updated when writing completed
        source_clip->setStartPosition(0);
        pair<mxfUMID, uint32_t> source_ref = mTracks[i]->GetSourceReference();
        source_clip->setSourcePackageID(source_ref.first);
        source_clip->setSourceTrackID(source_ref.second);

        if (mTracks[i]->IsPicture())
            video_count++;
        else
            audio_count++;
    }

    // clone file source packages
    for (i = 0; i < mTracks.size(); i++) {
        SourcePackage *file_source_package = dynamic_cast<SourcePackage*>(
                                                mTracks[i]->GetFileSourcePackage()->clone(mHeaderMetadata));
        mFilePackages.push_back(file_source_package);
        content_storage->appendPackages(file_source_package);
    }

    // add a network locator to the file descriptors
    IM_ASSERT(mTracks.size() == mFilePackages.size());
    for (i = 0; i < mFilePackages.size(); i++) {
        NetworkLocator *network_locator = new NetworkLocator(mHeaderMetadata);
        mFilePackages[i]->getDescriptor()->appendLocators(network_locator);
        network_locator->setURLString(mTracks[i]->GetRelativeURL());
    }
}

void AS02Version::CreateFile()
{
    IM_ASSERT(mHeaderMetadata);


    // set minimum llen

    mMXFFile->setMinLLen(4);


    // write the header partition pack and header metadata

    Partition &header_partition = mMXFFile->createPartition();
    header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setVersion(1, 3);  // v1.3 - smpte 377-1
    header_partition.setIndexSID(0);
    header_partition.setBodySID(0);
    header_partition.setKagSize(1);
    header_partition.setOperationalPattern(&MXF_OP_L(1b, MultiTrack_Stream_External));
    header_partition.write(mMXFFile);
    header_partition.fillToKag(mMXFFile);

    mHeaderMetadataStartPos = mMXFFile->tell(); // need this position when we re-write the header metadata
    KAGFillerWriter reserve_filler_writer(&header_partition, mReserveMinBytes);
    mHeaderMetadata->write(mMXFFile, &header_partition, &reserve_filler_writer);
    mHeaderMetadataEndPos = mMXFFile->tell();  // need this position when we re-write the header metadata
}

void AS02Version::UpdatePackageMetadata()
{
    // update the clone file packages

    IM_ASSERT(mFilePackages.size() == mTracks.size());
    size_t i;
    for (i = 0; i < mFilePackages.size(); i++)
        mTracks[i]->UpdatePackageMetadata(mFilePackages[i]);


    // update duration in sequences, timecode components and source clips in the material package

    vector<GenericTrack*> tracks = mMaterialPackage->getTracks();

    IM_ASSERT(mTracks.size() == tracks.size() - 1);  // track 0 is the timecode track
    for (i = 0; i < tracks.size(); i++) {
        Track *track = dynamic_cast<Track*>(tracks[i]);
        IM_ASSERT(track);

        Sequence *sequence = dynamic_cast<Sequence*>(track->getSequence());
        IM_ASSERT(sequence);
        if (i == 0) // timecode track
            sequence->setDuration(GetDuration());
        else
            sequence->setDuration(mTracks[i - 1]->GetOutputDuration(false));

        vector<StructuralComponent*> components = sequence->getStructuralComponents();
        size_t j;
        for (j = 0; j < components.size(); j++) {
            if (i == 0) // timecode track
                components[j]->setDuration(GetDuration());
            else
                components[j]->setDuration(mTracks[i - 1]->GetOutputDuration(false));
        }
    }
}

