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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS

#include <cstdio>

#include <algorithm>

#include <im/mxf_op1a/OP1AFile.h>
#include <im/mxf_helper/MXFDescriptorHelper.h>
#include <im/MXFUtils.h>
#include <im/Version.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



static uint32_t TIMECODE_TRACK_ID           = 901;
static uint32_t FIRST_PICTURE_TRACK_ID      = 1001;
static uint32_t FIRST_SOUND_TRACK_ID        = 2001;

static const char TIMECODE_TRACK_NAME[]     = "TC1";

static const uint32_t INDEX_SID             = 1;
static const uint32_t BODY_SID              = 2;

static const uint8_t MIN_LLEN               = 4;



static bool compare_track(OP1ATrack *left, OP1ATrack *right)
{
    return left->IsPicture() && !right->IsPicture();
}



OP1AFile* OP1AFile::OpenNew(int flavour, string filename, mxfRational frame_rate)
{
    return new OP1AFile(flavour, File::openNew(filename), frame_rate);
}

OP1AFile::OP1AFile(int flavour, mxfpp::File *mxf_file, mxfRational frame_rate)
{
    IM_CHECK((frame_rate.numerator == 25    && frame_rate.denominator == 1) ||
             (frame_rate.numerator == 50    && frame_rate.denominator == 1) ||
             (frame_rate.numerator == 30000 && frame_rate.denominator == 1001) ||
             (frame_rate.numerator == 60000 && frame_rate.denominator == 1001));

    mFlavour = flavour;
    mMXFFile = mxf_file;
    mFrameRate = frame_rate;
    mStartTimecode = Timecode(frame_rate, false);
    mCompanyName = get_im_company_name();
    mProductName = get_im_library_name();
    mProductVersion = get_im_mxf_product_version();
    mVersionString = get_im_version_string();
    mProductUID = get_im_product_uid();
    mReserveMinBytes = 8192;
    mxf_get_timestamp_now(&mCreationDate);
    mxf_generate_uuid(&mGenerationUID);
    mxf_generate_umid(&mMaterialPackageUID);
    mxf_generate_umid(&mFileSourcePackageUID);
    mOutputStartOffset = 0;
    mOutputEndOffset = 0;
    mPictureTrackCount = 0;
    mSoundTrackCount = 0;
    mDataModel = 0;
    mHeaderMetadata = 0;
    mHeaderMetadataStartPos = 0;
    mHeaderMetadataEndPos = 0;
    mCBEIndexTableStartPos = 0;
    mMaterialPackage = 0;
    mFileSourcePackage = 0;
    mFirstWrite = true;
    mPartitionInterval = 0;
    mPartitionFrameCount = 0;
    mKAGSize = ((flavour & OP1A_512_KAG_FLAVOUR) ? 512 : 1);
    mEssencePartitionKAGSize = mKAGSize;

    mIndexTable = new OP1AIndexTable(INDEX_SID, BODY_SID, frame_rate);
    mCPManager = new OP1AContentPackageManager(mEssencePartitionKAGSize, MIN_LLEN);

    // use fill key with correct version number
    g_KLVFill_key = g_CompliantKLVFill_key;
}

OP1AFile::~OP1AFile()
{
    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];

    delete mMXFFile;
    delete mDataModel;
    delete mHeaderMetadata;
    delete mCPManager;
    delete mIndexTable;
}

void OP1AFile::SetClipName(string name)
{
    mClipName = name;
}

void OP1AFile::SetStartTimecode(Timecode start_timecode)
{
    mStartTimecode = start_timecode;
}

void OP1AFile::SetProductInfo(string company_name, string product_name, mxfProductVersion product_version,
                              string version, mxfUUID product_uid)
{
    mCompanyName = company_name;
    mProductName = product_name;
    mProductVersion = product_version;
    mVersionString = version;
    mProductUID = product_uid;
}

void OP1AFile::SetCreationDate(mxfTimestamp creation_date)
{
    mCreationDate = creation_date;
}

void OP1AFile::SetGenerationUID(mxfUUID generation_uid)
{
    mGenerationUID = generation_uid;
}

void OP1AFile::SetMaterialPackageUID(mxfUMID package_uid)
{
    mMaterialPackageUID = package_uid;
}

void OP1AFile::SetFileSourcePackageUID(mxfUMID package_uid)
{
    mFileSourcePackageUID = package_uid;
}

void OP1AFile::ReserveHeaderMetadataSpace(uint32_t min_bytes)
{
    mReserveMinBytes = min_bytes;
}

void OP1AFile::SetPartitionInterval(int64_t frame_count)
{
    IM_CHECK(frame_count >= 0);
    mPartitionInterval = frame_count;
}

void OP1AFile::SetOutputStartOffset(int64_t offset)
{
    IM_CHECK(offset >= 0);
    mOutputStartOffset = offset;
}

void OP1AFile::SetOutputEndOffset(int64_t offset)
{
    IM_CHECK(offset <= 0);
    mOutputEndOffset = offset;
}

OP1ATrack* OP1AFile::CreateTrack(OP1AEssenceType essence_type)
{
    uint32_t track_id;
    uint8_t track_type_number;
    if ((essence_type == OP1A_PCM)) {
        track_id = FIRST_SOUND_TRACK_ID + mSoundTrackCount;
        track_type_number = mSoundTrackCount;
        mSoundTrackCount++;
    } else {
        track_id = FIRST_PICTURE_TRACK_ID + mPictureTrackCount;
        track_type_number = mPictureTrackCount;
        mPictureTrackCount++;
    }

    mTracks.push_back(OP1ATrack::Create(this, mTracks.size(), track_id, track_type_number, mFrameRate, essence_type));
    mTrackMap[mTracks.back()->GetTrackIndex()] = mTracks.back();

    return mTracks.back();
}

void OP1AFile::PrepareHeaderMetadata()
{
    if (mHeaderMetadata)
        return;

    IM_CHECK(!mTracks.empty());

    // sort tracks, picture followed by sound
    stable_sort(mTracks.begin(), mTracks.end(), compare_track);

    uint32_t last_picture_track_number = 0;
    uint32_t last_sound_track_number = 0;
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (mTracks[i]->IsPicture()) {
            if (!mTracks[i]->IsMaterialTrackNumberSet())
                mTracks[i]->SetMaterialTrackNumber(last_picture_track_number + 1);
            last_picture_track_number = mTracks[i]->GetMaterialTrackNumber();
        } else {
            if (!mTracks[i]->IsMaterialTrackNumberSet())
                mTracks[i]->SetMaterialTrackNumber(last_sound_track_number + 1);
            last_sound_track_number = mTracks[i]->GetMaterialTrackNumber();
        }
    }

    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->PrepareWrite(mPictureTrackCount, mSoundTrackCount);
    mCPManager->PrepareWrite();
    mIndexTable->PrepareWrite();

    if (mTracks.size() > 1)
        mEssenceContainerULs.insert(MXF_EC_L(MultipleWrappings));
    for (i = 0; i < mTracks.size(); i++)
        mEssenceContainerULs.insert(mTracks[i]->GetEssenceContainerUL());

    CreateHeaderMetadata();
}

void OP1AFile::PrepareWrite()
{
    if (!mHeaderMetadata)
        PrepareHeaderMetadata();

    CreateFile();
}

void OP1AFile::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (!data || size == 0)
        return;
    IM_CHECK(data && size && num_samples);

    GetTrack(track_index)->WriteSamplesInt(data, size, num_samples);

    WriteContentPackages(false);
}

void OP1AFile::CompleteWrite()
{
    IM_ASSERT(mMXFFile);


    // write any remaining content packages (e.g. first AVCI cp if duration equals 1)

    WriteContentPackages(true);


    // complete write for tracks

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->CompleteWrite();


    // non-minimal partition flavour: write any remaining VBE index segments

    if (!(mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) &&
        mIndexTable->IsVBE() && mIndexTable->HaveSegments())
    {
        Partition &index_partition = mMXFFile->createPartition();
        index_partition.setKey(&MXF_PP_K(OpenIncomplete, Body));
        index_partition.setIndexSID(INDEX_SID);
        index_partition.setBodySID(0);
        index_partition.setKagSize(mKAGSize);
        index_partition.write(mMXFFile);
        index_partition.fillToKag(mMXFFile);

        mIndexTable->WriteSegments(mMXFFile, &index_partition);
    }


    // write the footer partition pack

    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer));
    if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) && mIndexTable->IsVBE() && mIndexTable->HaveSegments())
        footer_partition.setIndexSID(INDEX_SID);
    else
        footer_partition.setIndexSID(0);
    footer_partition.setBodySID(0);
    footer_partition.setKagSize(mKAGSize);
    footer_partition.write(mMXFFile);
    footer_partition.fillToKag(mMXFFile);


    // minimal partitions flavour: write any remaining VBE index segments

    if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) &&
        mIndexTable->IsVBE() && mIndexTable->HaveSegments())
    {
        mIndexTable->WriteSegments(mMXFFile, &footer_partition);
    }


    // write the RIP

    mMXFFile->writeRIP();



    // update metadata sets with duration

    UpdatePackageMetadata();


    // re-write the header metadata in the header partition

    mMXFFile->seek(mHeaderMetadataStartPos, SEEK_SET);
    PositionFillerWriter pos_filler_writer(mHeaderMetadataEndPos);
    mHeaderMetadata->write(mMXFFile, &mMXFFile->getPartition(0), &pos_filler_writer);


    // re-write the CBE index table segment(s)

    if (!mFirstWrite && mIndexTable->IsCBE()) {
        mMXFFile->seek(mCBEIndexTableStartPos, SEEK_SET);
        mIndexTable->WriteSegments(mMXFFile,
                                   &mMXFFile->getPartition(((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) ? 0 : 1)));
    }


    // update and re-write the partition packs

    const std::vector<Partition*> &partitions = mMXFFile->getPartitions();
    for (i = 0; i < partitions.size(); i++) {
        if (mxf_is_header_partition_pack(partitions[i]->getKey()))
            partitions[i]->setKey(&MXF_PP_K(ClosedComplete, Header));
        else if (mxf_is_body_partition_pack(partitions[i]->getKey()))
            partitions[i]->setKey(&MXF_PP_K(ClosedComplete, Body));
    }
    mMXFFile->updatePartitions();


    // done with the file
    delete mMXFFile;
    mMXFFile = 0;
}

int64_t OP1AFile::GetDuration()
{
    int64_t container_duration = GetContainerDuration();
    IM_CHECK_M(container_duration - mOutputStartOffset + mOutputEndOffset >= 0,
               ("Invalid output start %"PRId64" / end %"PRId64" offsets. Output duration %"PRId64" is negative",
                mOutputStartOffset, mOutputEndOffset, container_duration - mOutputStartOffset + mOutputEndOffset));

    return container_duration - mOutputStartOffset + mOutputEndOffset;
}

int64_t OP1AFile::GetContainerDuration()
{
    return mIndexTable->GetDuration();
}

OP1ATrack* OP1AFile::GetTrack(uint32_t track_index)
{
    IM_ASSERT(track_index < mTracks.size());
    return mTrackMap[track_index];
}

void OP1AFile::CreateHeaderMetadata()
{
    IM_ASSERT(!mHeaderMetadata);


    // create the header metadata
    mDataModel = new DataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);


    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(mCreationDate);
    preface->setVersion((mFlavour & OP1A_377_2004_FLAVOUR) ? 258 : 259);
    preface->setOperationalPattern(MXF_OP_L(1a, MultiTrack_Stream_Internal));
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

    // Preface - ContentStorage - EssenceContainerData
    EssenceContainerData *ess_container_data = new EssenceContainerData(mHeaderMetadata);
    content_storage->appendEssenceContainerData(ess_container_data);
    ess_container_data->setLinkedPackageUID(mFileSourcePackageUID);
    ess_container_data->setIndexSID(INDEX_SID);
    ess_container_data->setBodySID(BODY_SID);

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
    timecode_track->setEditRate(mFrameRate);
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

    // Preface - ContentStorage - SourcePackage
    mFileSourcePackage = new SourcePackage(mHeaderMetadata);
    content_storage->appendPackages(mFileSourcePackage);
    mFileSourcePackage->setPackageUID(mFileSourcePackageUID);
    mFileSourcePackage->setPackageCreationDate(mCreationDate);
    mFileSourcePackage->setPackageModifiedDate(mCreationDate);

    // Preface - ContentStorage - SourcePackage - Timecode Track
    timecode_track = new Track(mHeaderMetadata);
    mFileSourcePackage->appendTracks(timecode_track);
    timecode_track->setTrackName(TIMECODE_TRACK_NAME);
    timecode_track->setTrackID(TIMECODE_TRACK_ID);
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(mFrameRate);
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
    Timecode sp_start_timecode = mStartTimecode;
    sp_start_timecode.AddOffset(- mOutputStartOffset, mFrameRate);
    timecode_component->setRoundedTimecodeBase(sp_start_timecode.GetRoundedTCBase());
    timecode_component->setDropFrame(sp_start_timecode.IsDropFrame());
    timecode_component->setStartTimecode(sp_start_timecode.GetOffset());

    // Preface - ContentStorage - SourcePackage - (Multiple) File Descriptor
    if (mTracks.size() > 1) {
        MultipleDescriptor *mult_descriptor = new MultipleDescriptor(mHeaderMetadata);
        mFileSourcePackage->setDescriptor(mult_descriptor);
        mult_descriptor->setSampleRate(mFrameRate);
        mult_descriptor->setEssenceContainer(MXF_EC_L(MultipleWrappings));
    }

    // MaterialPackage and file SourcePackage Tracks and FileDescriptor
    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->AddHeaderMetadata(mHeaderMetadata, mMaterialPackage, mFileSourcePackage);
}

void OP1AFile::CreateFile()
{
    IM_ASSERT(mHeaderMetadata);


    // set minimum llen

    mMXFFile->setMinLLen(MIN_LLEN);


    // write the header metadata

    Partition &header_partition = mMXFFile->createPartition();
    header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setVersion(1, ((mFlavour & OP1A_377_2004_FLAVOUR) ? 2 : 3));
    if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) && mIndexTable->IsCBE())
        header_partition.setIndexSID(INDEX_SID);
    else
        header_partition.setIndexSID(0);
    if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR))
        header_partition.setBodySID(BODY_SID);
    else
        header_partition.setBodySID(0);
    header_partition.setKagSize(mKAGSize);
    header_partition.setOperationalPattern(&MXF_OP_L(1a, MultiTrack_Stream_Internal));
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        header_partition.addEssenceContainer(&(*iter));
    header_partition.write(mMXFFile);
    header_partition.fillToKag(mMXFFile);

    mHeaderMetadataStartPos = mMXFFile->tell(); // need this position when we re-write the header metadata
    KAGFillerWriter reserve_filler_writer(&header_partition, mReserveMinBytes);
    mHeaderMetadata->write(mMXFFile, &header_partition, &reserve_filler_writer);
    mHeaderMetadataEndPos = mMXFFile->tell();  // need this position when we re-write the header metadata
}

void OP1AFile::UpdatePackageMetadata()
{
    int64_t output_duration = GetDuration();

    UpdateTrackMetadata(mMaterialPackage,   0,                  output_duration);
    UpdateTrackMetadata(mFileSourcePackage, mOutputStartOffset, output_duration + mOutputStartOffset);


    int64_t container_duration = GetContainerDuration();

    IM_ASSERT(mFileSourcePackage->haveDescriptor());
    FileDescriptor *file_descriptor = dynamic_cast<FileDescriptor*>(mFileSourcePackage->getDescriptor());
    if (file_descriptor)
        file_descriptor->setContainerDuration(container_duration);

    MultipleDescriptor *mult_descriptor = dynamic_cast<MultipleDescriptor*>(file_descriptor);
    if (mult_descriptor) {
        vector<GenericDescriptor*> sub_descriptors = mult_descriptor->getSubDescriptorUIDs();
        size_t i;
        for (i = 0; i < sub_descriptors.size(); i++) {
            file_descriptor = dynamic_cast<FileDescriptor*>(sub_descriptors[i]);
            if (file_descriptor)
                file_descriptor->setContainerDuration(container_duration);
        }
    }
}

void OP1AFile::UpdateTrackMetadata(GenericPackage *package, int64_t origin, int64_t duration)
{
    vector<GenericTrack*> tracks = package->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *track = dynamic_cast<Track*>(tracks[i]);
        if (!track)
            continue;

        track->setOrigin(origin);

        Sequence *sequence = dynamic_cast<Sequence*>(track->getSequence());
        IM_ASSERT(sequence);
        if (sequence->getDuration() < 0) {
            sequence->setDuration(duration);

            vector<StructuralComponent*> components = sequence->getStructuralComponents();
            IM_CHECK(components.size() == 1);
            components[0]->setDuration(duration);
        }
    }
}

void OP1AFile::WriteContentPackages(bool end_of_samples)
{
    // for dual segment index tables (eg. AVCI), wait for first 2 content packages before writing
    if (!end_of_samples && mIndexTable->RequireIndexTableSegmentPair() &&
        mCPManager->GetPosition() == 0 && !mCPManager->HaveContentPackages(2))
    {
        return;
    }

    while (mCPManager->HaveContentPackage()) {

        bool start_ess_partition = false;
        int64_t ess_partition_body_offset = mIndexTable->GetStreamOffset(); // get before UpdateIndexTable below

        if (mFirstWrite)
        {
            // write CBE index table segments and ensure new essence partition is started

            if (mIndexTable->IsCBE()) {
                // make sure edit unit byte count and delta entries are known
                mCPManager->UpdateIndexTable(mIndexTable, mCPManager->HaveContentPackages(2) ? 2 : 1);

                if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR)) {
                    mCBEIndexTableStartPos = mMXFFile->tell(); // need this position when we re-write the index segment
                    mIndexTable->WriteSegments(mMXFFile, &mMXFFile->getPartition(0));
                } else {
                    Partition &index_partition = mMXFFile->createPartition();
                    index_partition.setKey(&MXF_PP_K(OpenIncomplete, Body));
                    index_partition.setIndexSID(INDEX_SID);
                    index_partition.setBodySID(0);
                    index_partition.setKagSize(mKAGSize);
                    index_partition.write(mMXFFile);
                    index_partition.fillToKag(mMXFFile);

                    mCBEIndexTableStartPos = mMXFFile->tell(); // need this position when we re-write the index segment
                    mIndexTable->WriteSegments(mMXFFile, &index_partition);
                }
            }

            if (!(mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR))
                start_ess_partition = true;
            mFirstWrite = false;
        }
        else if (mPartitionInterval > 0 &&
                 mPartitionFrameCount >= mPartitionInterval && mIndexTable->CanStartPartition())
        {
            // write VBE index table segments and ensure new essence partition is started

            if (mIndexTable->IsVBE()) {
                IM_ASSERT(mIndexTable->HaveSegments());

                Partition &index_partition = mMXFFile->createPartition();
                index_partition.setKey(&MXF_PP_K(OpenIncomplete, Body));
                index_partition.setIndexSID(INDEX_SID);
                index_partition.setBodySID(0);
                index_partition.setKagSize(mKAGSize);
                index_partition.write(mMXFFile);
                index_partition.fillToKag(mMXFFile);

                mIndexTable->WriteSegments(mMXFFile, &index_partition);
            }

            start_ess_partition = true;
            mPartitionFrameCount = 0;
        }

        if (start_ess_partition) {
            Partition &ess_partition = mMXFFile->createPartition();
            ess_partition.setKey(&MXF_PP_K(OpenIncomplete, Body));
            ess_partition.setIndexSID(0);
            ess_partition.setBodySID(BODY_SID);
            ess_partition.setKagSize(mEssencePartitionKAGSize);
            ess_partition.setBodyOffset(ess_partition_body_offset);
            ess_partition.write(mMXFFile);
            ess_partition.fillToKag(mMXFFile);
        }

        mCPManager->WriteNextContentPackage(mMXFFile, mIndexTable);

        if (mPartitionInterval > 0)
            mPartitionFrameCount++;
    }
}

