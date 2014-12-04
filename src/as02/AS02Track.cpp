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

#define __STDC_FORMAT_MACROS

#include <cstring>
#include <cstdio>

#include <memory>

#include <bmx/as02/AS02Track.h>
#include <bmx/as02/AS02DVTrack.h>
#include <bmx/as02/AS02D10Track.h>
#include <bmx/as02/AS02AVCITrack.h>
#include <bmx/as02/AS02UncTrack.h>
#include <bmx/as02/AS02MPEG2LGTrack.h>
#include <bmx/as02/AS02PCMTrack.h>
#include <bmx/as02/AS02Clip.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static uint32_t TIMECODE_TRACK_ID   = 901;
static uint32_t VIDEO_TRACK_ID      = 1001;
static uint32_t AUDIO_TRACK_ID      = 2001;

static const char TIMECODE_TRACK_NAME[] = "Timecode";
static const char VIDEO_TRACK_NAME[]    = "Video";
static const char AUDIO_TRACK_NAME[]    = "Audio";


typedef struct
{
    EssenceType essence_type;
    mxfRational sample_rate[10];
} AS02SampleRateSupport;

static const AS02SampleRateSupport AS02_SAMPLE_RATE_SUPPORT[] =
{
    {IEC_DV25,                 {{25, 1}, {30000, 1001}, {0, 0}}},
    {DVBASED_DV25,             {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV50,                     {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV100_1080I,              {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV100_720P,               {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {D10_30,                   {{25, 1}, {30000, 1001}, {0, 0}}},
    {D10_40,                   {{25, 1}, {30000, 1001}, {0, 0}}},
    {D10_50,                   {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI200_1080I,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI200_1080P,            {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI200_720P,             {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI100_1080I,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI100_1080P,            {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI100_720P,             {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI50_1080I,             {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI50_1080P,             {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI50_720P,              {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {UNC_SD,                   {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {UNC_HD_1080I,             {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {UNC_HD_1080P,             {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {UNC_HD_720P,              {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {UNC_UHD_3840,             {{-1, -1}, {0, 0}}},
    {MPEG2LG_422P_HL_1080I,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_422P_HL_1080P,    {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_422P_HL_720P,     {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1920_1080I, {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1920_1080P, {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1440_1080I, {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1440_1080P, {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_720P,       {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MPEG2LG_MP_H14_1080I,     {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_H14_1080P,     {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {WAVE_PCM,                 {{48000,1}, {0, 0}}},
};



static string get_track_clip_name(string &clip_name, bool is_video, uint32_t track_number)
{
    char buffer[16];
    bmx_snprintf(buffer, sizeof(buffer), "__track_%s%u", is_video ? "v" : "a", track_number);

    return clip_name + buffer;
}



bool AS02Track::IsSupported(EssenceType essence_type, mxfRational sample_rate)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(AS02_SAMPLE_RATE_SUPPORT); i++) {
        if (essence_type == AS02_SAMPLE_RATE_SUPPORT[i].essence_type) {
            if (AS02_SAMPLE_RATE_SUPPORT[i].sample_rate[0].numerator < 0)
                return true;

            size_t j = 0;
            while (AS02_SAMPLE_RATE_SUPPORT[i].sample_rate[j].numerator) {
                if (sample_rate == AS02_SAMPLE_RATE_SUPPORT[i].sample_rate[j])
                    return true;
                j++;
            }
        }
    }

    return false;
}

AS02Track* AS02Track::OpenNew(AS02Clip *clip, File *file, string rel_uri, uint32_t track_index,
                              EssenceType essence_type)
{
    switch (essence_type)
    {
        case IEC_DV25:
        case DVBASED_DV25:
        case DV50:
        case DV100_1080I:
        case DV100_720P:
            return new AS02DVTrack(clip, track_index, essence_type, file, rel_uri);
        case D10_30:
        case D10_40:
        case D10_50:
            return new AS02D10Track(clip, track_index, essence_type, file, rel_uri);
        case AVCI200_1080I:
        case AVCI200_1080P:
        case AVCI200_720P:
        case AVCI100_1080I:
        case AVCI100_1080P:
        case AVCI100_720P:
        case AVCI50_1080I:
        case AVCI50_1080P:
        case AVCI50_720P:
            return new AS02AVCITrack(clip, track_index, essence_type, file, rel_uri);
        case UNC_SD:
        case UNC_HD_1080I:
        case UNC_HD_1080P:
        case UNC_HD_720P:
        case UNC_UHD_3840:
            return new AS02UncTrack(clip, track_index, essence_type, file, rel_uri);
        case MPEG2LG_422P_HL_1080I:
        case MPEG2LG_422P_HL_1080P:
        case MPEG2LG_422P_HL_720P:
        case MPEG2LG_MP_HL_1920_1080I:
        case MPEG2LG_MP_HL_1920_1080P:
        case MPEG2LG_MP_HL_1440_1080I:
        case MPEG2LG_MP_HL_1440_1080P:
        case MPEG2LG_MP_HL_720P:
        case MPEG2LG_MP_H14_1080I:
        case MPEG2LG_MP_H14_1080P:
            return new AS02MPEG2LGTrack(clip, track_index, essence_type, file, rel_uri);
        case WAVE_PCM:
            return new AS02PCMTrack(clip, track_index, file, rel_uri);
        default:
            BMX_ASSERT(false);
    }

    return 0;
}

AS02Track::AS02Track(AS02Clip *clip, uint32_t track_index, EssenceType essence_type,
                     File *mxf_file, string rel_uri)
{
    mClip = clip;
    mTrackIndex = track_index;
    mSampleSize = 0;
    mOutputTrackNumber = 0;
    mOutputTrackNumberSet = false;
    mContainerDuration = 0;
    mContainerSize = 0;
    mOutputStartOffset = 0;
    mOutputEndOffset = 0;
    mMXFFile = mxf_file;
    mRelativeURL = rel_uri;
    mIsPicture = true;
    mTrackNumber = 0;
    mIndexSID = 1;
    mBodySID = 2;
    mLLen = 4;
    mEssenceElementLLen = 4;
    mKAGSize = 1;
    mxf_generate_umid(&mMaterialPackageUID);
    mxf_generate_umid(&mFileSourcePackageUID);
    mHeaderMetadataEndPos = 0;
    mCBEIndexSegment = 0;
    mMaterialPackage = 0;
    mFileSourcePackage = 0;
    mHaveLowerLevelSourcePackage = false;
    mLowerLevelSourcePackage = 0;
    mLowerLevelSourcePackageUID = g_Null_UMID;
    mLowerLevelTrackId = 0;

    mEssenceType = essence_type;
    mDescriptorHelper = MXFDescriptorHelper::Create(essence_type);
    mDescriptorHelper->SetFlavour(MXFDESC_SMPTE_377_1_FLAVOUR);

    mManifestFile = clip->GetBundle()->GetManifest()->RegisterFile(rel_uri, ESSENCE_COMPONENT_FILE_ROLE);
    mManifestFile->SetId(mFileSourcePackageUID);

    mEssenceOnlyChecksum.Init(MD5_CHECKSUM);

    // use fill key with correct version number
    g_KLVFill_key = g_CompliantKLVFill_key;

    mDataModel = new DataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);
}

AS02Track::~AS02Track()
{
    delete mDescriptorHelper;
    delete mMXFFile;
    delete mDataModel;
    delete mHeaderMetadata;
    delete mCBEIndexSegment;
}

void AS02Track::SetFileSourcePackageUID(mxfUMID package_uid)
{
    mFileSourcePackageUID = package_uid;

    mManifestFile->SetId(mFileSourcePackageUID);
}

void AS02Track::SetOutputTrackNumber(uint32_t track_number)
{
    mOutputTrackNumber = track_number;
    mOutputTrackNumberSet = true;
}

void AS02Track::SetMICType(MICType type)
{
    mManifestFile->SetMICType(type);
}

void AS02Track::SetMICScope(MICScope scope)
{
    mManifestFile->SetMICScope(scope);
}

void AS02Track::SetLowerLevelSourcePackage(SourcePackage *package, uint32_t track_id, string uri)
{
    BMX_CHECK(!mHaveLowerLevelSourcePackage);

#if 0   // TODO: allow dark strong referenced sets to be cloned without resulting in an error
    mLowerLevelSourcePackage = dynamic_cast<SourcePackage*>(package->clone(mHeaderMetadata));
#else
    mLowerLevelSourcePackage = 0;
#endif
    mLowerLevelSourcePackageUID = package->getPackageUID();
    mLowerLevelTrackId = track_id;
    mLowerLevelURI = uri;

    mHaveLowerLevelSourcePackage = true;
}

void AS02Track::SetLowerLevelSourcePackage(mxfUMID package_uid, uint32_t track_id)
{
    BMX_CHECK(!mHaveLowerLevelSourcePackage);

    mLowerLevelSourcePackageUID = package_uid;
    mLowerLevelTrackId = track_id;

    mHaveLowerLevelSourcePackage = true;
}

void AS02Track::SetOutputStartOffset(int64_t offset)
{
    BMX_CHECK(offset >= 0);
    mOutputStartOffset = offset;
}

void AS02Track::SetOutputEndOffset(int64_t offset)
{
    BMX_CHECK(offset <= 0);
    mOutputEndOffset = offset;
}

void AS02Track::PrepareWrite()
{
    BMX_ASSERT(mMXFFile);

    mSampleSize = GetSampleSize();

    CreateHeaderMetadata();
    CreateFile();
}

void AS02Track::CompleteWrite()
{
    BMX_ASSERT(mMXFFile);


    // complete writing of samples

    PostSampleWriting(mMXFFile->getPartitions().back());


    if (!HaveCBEIndexTable() && HaveVBEIndexEntries()) {
        // write a index partition pack and VBE index table

        mMXFFile->openMemoryFile(8192);

        Partition &index_partition = mMXFFile->createPartition();
        index_partition.setKey(&MXF_PP_K(OpenComplete, Body));
        index_partition.setIndexSID(mIndexSID);
        index_partition.setBodySID(0);
        index_partition.write(mMXFFile);

        WriteVBEIndexTable(&index_partition);

        mMXFFile->updatePartitions();
        mMXFFile->closeMemoryFile();
    }


    // update metadata sets with duration

    UpdatePackageMetadata(mMaterialPackage);
    UpdatePackageMetadata(mFileSourcePackage);


    // write the footer partition pack

    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer));
    footer_partition.setIndexSID(0);
    footer_partition.setBodySID(0);
    footer_partition.write(mMXFFile);


    // write the RIP

    mMXFFile->writeRIP();



    // re-write header to memory

    mMXFFile->seek(0, SEEK_SET);
    mMXFFile->openMemoryFile(8192);
    mMXFFile->setMemoryPartitionIndexes(0, 0); // overwriting and updating from header pp


    // update and re-write the header partition pack

    Partition &header_partition = mMXFFile->getPartition(0);
    header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
    header_partition.setFooterPartition(footer_partition.getThisPartition());
    header_partition.write(mMXFFile);


    // re-write the header metadata in the header partition

    PositionFillerWriter pos_filler_writer(mHeaderMetadataEndPos);
    mHeaderMetadata->write(mMXFFile, &header_partition, &pos_filler_writer);


    if (HaveCBEIndexTable()) {
        // update and re-write the CBE index table segment

        Partition &index_partition = mMXFFile->getPartition(1);
        index_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
        index_partition.setFooterPartition(footer_partition.getThisPartition());
        index_partition.write(mMXFFile);

        WriteCBEIndexTable(&mMXFFile->getPartition(1));
    }

    // update header and index partition packs and flush memory writes to file

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();


    // update and re-write the body partition packs

    mMXFFile->updateBodyPartitions(&MXF_PP_K(ClosedComplete, Body));


    // done with the file
    delete mMXFFile;
    mMXFFile = 0;


    // finalize checksum and update manifest
    if (mManifestFile->GetMICScope() == ESSENCE_ONLY_MIC_SCOPE) {
        if (mManifestFile->GetMICType() == MD5_MIC_TYPE) {
            mEssenceOnlyChecksum.Final();
            mManifestFile->SetMIC(MD5_MIC_TYPE, ESSENCE_ONLY_MIC_SCOPE, mEssenceOnlyChecksum.GetDigestString());
        }
    }
}

void AS02Track::UpdatePackageMetadata(GenericPackage *package)
{
    SourcePackage *source_package = dynamic_cast<SourcePackage*>(package);
    FileDescriptor *file_descriptor = 0;
    if (source_package && source_package->haveDescriptor())
        file_descriptor = dynamic_cast<FileDescriptor*>(source_package->getDescriptor());

    vector<GenericTrack*> tracks = package->getTracks();

    // update track origin in file source package tracks and
    // duration in sequences, timecode components and source clips
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *track = dynamic_cast<Track*>(tracks[i]);
        BMX_ASSERT(track);

        if (source_package)
            track->setOrigin(mOutputStartOffset);

        Sequence *sequence = dynamic_cast<Sequence*>(track->getSequence());
        BMX_ASSERT(sequence);
        vector<StructuralComponent*> components = sequence->getStructuralComponents();
        if (sequence->getDuration() < 0) {
            if (source_package)
                sequence->setDuration(GetDuration());
            else
                sequence->setDuration(GetOutputDuration(false));

            BMX_ASSERT(components.size() == 1);
            if (source_package)
                components[0]->setDuration(GetDuration());
            else
                components[0]->setDuration(GetOutputDuration(false));
        }
        if (source_package && components.size() == 1) {
            TimecodeComponent *timecode_component = dynamic_cast<TimecodeComponent*>(components[0]);
            if (timecode_component) {
                Timecode sp_start_timecode = mClip->mStartTimecode;
                sp_start_timecode.AddOffset(- mOutputStartOffset, GetSampleRate());
                timecode_component->setRoundedTimecodeBase(sp_start_timecode.GetRoundedTCBase());
                timecode_component->setDropFrame(sp_start_timecode.IsDropFrame());
                timecode_component->setStartTimecode(sp_start_timecode.GetOffset());
            }
        }
    }

    // update the container duration in the file descriptor
    if (file_descriptor)
        file_descriptor->setContainerDuration(mContainerDuration);
}

mxfUL AS02Track::GetEssenceContainerUL() const
{
    return mDescriptorHelper->GetEssenceContainerUL();
}

mxfRational AS02Track::GetSampleRate() const
{
    return mDescriptorHelper->GetSampleRate();
}

pair<mxfUMID, uint32_t> AS02Track::GetSourceReference() const
{
    pair<mxfUMID, uint32_t> source_ref;
    source_ref.first = mFileSourcePackageUID;
    source_ref.second = (mIsPicture ? VIDEO_TRACK_ID : AUDIO_TRACK_ID);

    return source_ref;
}

uint32_t AS02Track::GetSampleSize()
{
    return mDescriptorHelper->GetSampleSize();
}

bool AS02Track::HasValidDuration() const
{
    return mContainerDuration - mOutputStartOffset + mOutputEndOffset >= 0;
}

int64_t AS02Track::GetOutputDuration(bool clip_frame_rate) const
{
    BMX_ASSERT(!clip_frame_rate || mClip->mClipFrameRate == GetSampleRate());

    if (mContainerDuration - mOutputStartOffset + mOutputEndOffset <= 0)
        return 0;

    return mContainerDuration - mOutputStartOffset + mOutputEndOffset;
}

int64_t AS02Track::GetDuration() const
{
    if (mContainerDuration + mOutputEndOffset <= 0)
        return 0;

    return mContainerDuration + mOutputEndOffset;
}

int64_t AS02Track::GetContainerDuration() const
{
    return mContainerDuration;
}

int64_t AS02Track::ConvertClipDuration(int64_t clip_duration) const
{
    BMX_ASSERT(mClip->mClipFrameRate == GetSampleRate());
    return clip_duration;
}

mxfRational& AS02Track::GetClipFrameRate() const
{
    return mClip->mClipFrameRate;
}

void AS02Track::WriteCBEIndexTable(Partition *partition)
{
    BMX_ASSERT(mSampleSize > 0);

    if (!mCBEIndexSegment) {
        mxfUUID uuid;
        mxf_generate_uuid(&uuid);

        mCBEIndexSegment = new IndexTableSegment();
        mCBEIndexSegment->setInstanceUID(uuid);
        mCBEIndexSegment->setIndexEditRate(GetSampleRate());
        mCBEIndexSegment->setIndexDuration(0); // will be updated when writing is completed (2nd WriteIndexTable() call)
        mCBEIndexSegment->setIndexSID(mIndexSID);
        mCBEIndexSegment->setBodySID(mBodySID);
        if (mIsPicture) {
            // frame wrapped include KL
            mCBEIndexSegment->setEditUnitByteCount(mxfKey_extlen + mEssenceElementLLen + mSampleSize);
        } else {
            // clip wrapped
            mCBEIndexSegment->setEditUnitByteCount(mSampleSize);
        }
    } else {
        mCBEIndexSegment->setIndexDuration(mContainerDuration);
    }

    mCBEIndexSegment->write(mMXFFile, partition, 0);
}

void AS02Track::UpdateEssenceOnlyChecksum(const unsigned char *data, uint32_t size)
{
    if (data && size > 0 && mManifestFile->GetMICScope() == ESSENCE_ONLY_MIC_SCOPE) {
        if (mManifestFile->GetMICType() == MD5_MIC_TYPE)
            mEssenceOnlyChecksum.Update(data, size);
    }
}

void AS02Track::CreateHeaderMetadata()
{
    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(mClip->mCreationDate);
    preface->setVersion(MXF_PREFACE_VER(1, 3));
    preface->setOperationalPattern(MXF_OP_L(1a, UniTrack_Stream_Internal));
    preface->appendEssenceContainers(GetEssenceContainerUL());
    preface->setDMSchemes(vector<mxfUL>());

    // Preface - Identification
    Identification *ident = new Identification(mHeaderMetadata);
    preface->appendIdentifications(ident);
    ident->initialise(mClip->mCompanyName, mClip->mProductName, mClip->mVersionString, mClip->mProductUID);
    if (mClip->mProductVersion.major != 0 || mClip->mProductVersion.minor != 0 || mClip->mProductVersion.patch != 0 ||
        mClip->mProductVersion.build != 0 || mClip->mProductVersion.release != 0)
    {
        ident->setProductVersion(mClip->mProductVersion);
    }
    ident->setModificationDate(mClip->mCreationDate);
    ident->setThisGenerationUID(mClip->mGenerationUID);

    // Preface - ContentStorage
    ContentStorage* content_storage = new ContentStorage(mHeaderMetadata);
    preface->setContentStorage(content_storage);

    // Preface - ContentStorage - EssenceContainerData
    EssenceContainerData *ess_container_data = new EssenceContainerData(mHeaderMetadata);
    content_storage->appendEssenceContainerData(ess_container_data);
    ess_container_data->setLinkedPackageUID(mFileSourcePackageUID);
    ess_container_data->setIndexSID(mIndexSID);
    ess_container_data->setBodySID(mBodySID);

    // Preface - ContentStorage - MaterialPackage
    mMaterialPackage = new MaterialPackage(mHeaderMetadata);
    content_storage->appendPackages(mMaterialPackage);
    mMaterialPackage->setPackageUID(mMaterialPackageUID);
    mMaterialPackage->setPackageCreationDate(mClip->mCreationDate);
    mMaterialPackage->setPackageModifiedDate(mClip->mCreationDate);
    if (!mClip->mClipName.empty())
        mMaterialPackage->setName(get_track_clip_name(mClip->mClipName, mIsPicture, mOutputTrackNumber));

    // Preface - ContentStorage - MaterialPackage - Timecode Track
    Track *timecode_track = new Track(mHeaderMetadata);
    mMaterialPackage->appendTracks(timecode_track);
    timecode_track->setTrackName(TIMECODE_TRACK_NAME);
    timecode_track->setTrackID(TIMECODE_TRACK_ID);
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(GetSampleRate());
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
    timecode_component->setRoundedTimecodeBase(mClip->mStartTimecode.GetRoundedTCBase());
    timecode_component->setDropFrame(mClip->mStartTimecode.IsDropFrame());
    timecode_component->setStartTimecode(mClip->mStartTimecode.GetOffset());

    // Preface - ContentStorage - MaterialPackage - Timeline Track
    Track *track = new Track(mHeaderMetadata);
    mMaterialPackage->appendTracks(track);
    track->setTrackName(mIsPicture ? VIDEO_TRACK_NAME : AUDIO_TRACK_NAME);
    track->setTrackID(mIsPicture ? VIDEO_TRACK_ID : AUDIO_TRACK_ID);
    track->setTrackNumber(0);
    track->setEditRate(GetSampleRate());
    track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
    sequence = new Sequence(mHeaderMetadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(mIsPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
    sequence->setDuration(-1); // updated when writing completed

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
    SourceClip *source_clip = new SourceClip(mHeaderMetadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(mIsPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
    source_clip->setDuration(-1); // updated when writing completed
    source_clip->setStartPosition(0);
    source_clip->setSourceTrackID(mIsPicture ? VIDEO_TRACK_ID : AUDIO_TRACK_ID);
    source_clip->setSourcePackageID(mFileSourcePackageUID);

    // Preface - ContentStorage - SourcePackage
    mFileSourcePackage = new SourcePackage(mHeaderMetadata);
    content_storage->appendPackages(mFileSourcePackage);
    mFileSourcePackage->setPackageUID(mFileSourcePackageUID);
    mFileSourcePackage->setPackageCreationDate(mClip->mCreationDate);
    mFileSourcePackage->setPackageModifiedDate(mClip->mCreationDate);
    preface->setPrimaryPackage(mFileSourcePackage);

    // Preface - ContentStorage - SourcePackage - Timecode Track
    timecode_track = new Track(mHeaderMetadata);
    mFileSourcePackage->appendTracks(timecode_track);
    timecode_track->setTrackName(TIMECODE_TRACK_NAME);
    timecode_track->setTrackID(TIMECODE_TRACK_ID);
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(GetSampleRate());
    timecode_track->setOrigin(0); // could be updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timecode Track - Sequence
    sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(-1); // updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timecode Track - TimecodeComponent
    timecode_component = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(timecode_component);
    timecode_component->setDataDefinition(MXF_DDEF_L(Timecode));
    timecode_component->setDuration(-1); // updated when writing completed
    Timecode sp_start_timecode = mClip->mStartTimecode;
    sp_start_timecode.AddOffset(- mOutputStartOffset, GetSampleRate());
    timecode_component->setRoundedTimecodeBase(sp_start_timecode.GetRoundedTCBase());
    timecode_component->setDropFrame(sp_start_timecode.IsDropFrame());
    timecode_component->setStartTimecode(sp_start_timecode.GetOffset());

    // Preface - ContentStorage - SourcePackage - Timeline Track
    track = new Track(mHeaderMetadata);
    mFileSourcePackage->appendTracks(track);
    track->setTrackName(mIsPicture ? VIDEO_TRACK_NAME : AUDIO_TRACK_NAME);
    track->setTrackID(mIsPicture ? VIDEO_TRACK_ID : AUDIO_TRACK_ID);
    track->setTrackNumber(mTrackNumber);
    track->setEditRate(GetSampleRate());
    track->setOrigin(0); // could be updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
    sequence = new Sequence(mHeaderMetadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(mIsPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
    sequence->setDuration(-1); // updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
    source_clip = new SourceClip(mHeaderMetadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(mIsPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
    source_clip->setDuration(-1); // updated when writing completed
    source_clip->setStartPosition(0);
    if (mHaveLowerLevelSourcePackage) {
        source_clip->setSourcePackageID(mLowerLevelSourcePackageUID);
        source_clip->setSourceTrackID(mLowerLevelTrackId);
    } else {
        source_clip->setSourceTrackID(0);
        source_clip->setSourcePackageID(g_Null_UMID);
    }

    // Preface - ContentStorage - SourcePackage - FileDescriptor
    FileDescriptor *descriptor = mDescriptorHelper->CreateFileDescriptor(mHeaderMetadata);
    mFileSourcePackage->setDescriptor(descriptor);
    descriptor->setLinkedTrackID(mIsPicture ? VIDEO_TRACK_ID : AUDIO_TRACK_ID);

    // Preface - ContentStorage - (lower-level) SourcePackage
    if (mLowerLevelSourcePackage) {
        content_storage->appendPackages(mLowerLevelSourcePackage);
        if (!mLowerLevelURI.empty()) {
            NetworkLocator *network_locator = new NetworkLocator(mHeaderMetadata);
            mLowerLevelSourcePackage->getDescriptor()->appendLocators(network_locator);
            network_locator->setURLString(mLowerLevelURI);
        }
    }
}

void AS02Track::CreateFile()
{
    // set minimum llen

    mMXFFile->setMinLLen(mLLen);


    // write header partition and essence partition pack to memory first

    mMXFFile->openMemoryFile(8192);


    // write the header partition pack and header metadata

    Partition &header_partition = mMXFFile->createPartition();
    header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setVersion(1, 3);  // v1.3 - smpte 377-1
    header_partition.setIndexSID(0);
    header_partition.setBodySID(0);
    header_partition.setKagSize(mKAGSize);
    header_partition.setOperationalPattern(&MXF_OP_L(1a, UniTrack_Stream_Internal));
    header_partition.addEssenceContainer(GetEssenceContainerUL());
    header_partition.write(mMXFFile);

    KAGFillerWriter reserve_filler_writer(&header_partition, mClip->mReserveMinBytes);
    mHeaderMetadata->write(mMXFFile, &header_partition, &reserve_filler_writer);
    mHeaderMetadataEndPos = mMXFFile->tell();  // need this position when we re-write the header metadata


    if (HaveCBEIndexTable()) {
        // write the CBE index partition pack and index table segment

        Partition &index_partition = mMXFFile->createPartition();
        index_partition.setKey(&MXF_PP_K(OpenComplete, Body));
        index_partition.setIndexSID(mIndexSID);
        index_partition.setBodySID(0);
        index_partition.write(mMXFFile);

        WriteCBEIndexTable(&index_partition);
    }


    // write the essence data partition pack

    Partition &ess_partition = mMXFFile->createPartition();
    ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
    ess_partition.setIndexSID(0);
    ess_partition.setBodySID(mBodySID);
    ess_partition.setBodyOffset(0);
    ess_partition.write(mMXFFile);


    // update partitions and flush memory

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();


    PreSampleWriting();
}

