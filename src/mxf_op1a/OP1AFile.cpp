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

#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/mxf_op1a/OP1APCMTrack.h>
#include <bmx/mxf_helper/MXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Version.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const uint32_t TIMECODE_TRACK_ID         = 901;
static const uint32_t FIRST_PICTURE_TRACK_ID    = 1001;
static const uint32_t FIRST_SOUND_TRACK_ID      = 2001;
static const uint32_t FIRST_DATA_TRACK_ID       = 3001;

static const char TIMECODE_TRACK_NAME[]         = "TC1";

static const uint32_t INDEX_SID                 = 1;
static const uint32_t BODY_SID                  = 2;

static const uint8_t MIN_LLEN                   = 4;

static const uint32_t MEMORY_WRITE_CHUNK_SIZE   = 8192;



static bool compare_track(OP1ATrack *left, OP1ATrack *right)
{
    return left->GetDataDef() < right->GetDataDef();
}



OP1AFile::OP1AFile(int flavour, mxfpp::File *mxf_file, mxfRational frame_rate)
{
    BMX_CHECK(frame_rate == FRAME_RATE_23976 ||
              frame_rate == FRAME_RATE_24 ||
              frame_rate == FRAME_RATE_25 ||
              frame_rate == FRAME_RATE_2997 ||
              frame_rate == FRAME_RATE_30 ||
              frame_rate == FRAME_RATE_50 ||
              frame_rate == FRAME_RATE_5994 ||
              frame_rate == FRAME_RATE_60);

    mFlavour = flavour;
    mMXFFile = mxf_file;
    mFrameRate = frame_rate;
    mEditRate = frame_rate;
    mStartTimecode = Timecode(frame_rate, false);
    mCompanyName = get_bmx_company_name();
    mProductName = get_bmx_library_name();
    mProductVersion = get_bmx_mxf_product_version();
    mVersionString = get_bmx_mxf_version_string();
    mProductUID = get_bmx_product_uid();
    mReserveMinBytes = 8192;
    mInputDuration = -1;
    mxf_get_timestamp_now(&mCreationDate);
    mxf_generate_uuid(&mGenerationUID);
    mxf_generate_umid(&mMaterialPackageUID);
    mxf_generate_umid(&mFileSourcePackageUID);
    mFrameWrapped = true;
    mOutputStartOffset = 0;
    mOutputEndOffset = 0;
    mHaveANCTrack = false;
    mHaveVBITrack = false;
    mDataModel = 0;
    mHeaderMetadata = 0;
    mHeaderMetadataEndPos = 0;
    mMaterialPackage = 0;
    mFileSourcePackage = 0;
    mFirstWrite = true;
    mPartitionInterval = 0;
    mPartitionFrameCount = 0;
    mKAGSize = ((flavour & OP1A_512_KAG_FLAVOUR) ? 512 : 1);
    mEssencePartitionKAGSize = mKAGSize;
    mSupportCompleteSinglePass = false;
    mFooterPartitionOffset = 0;
    mMXFChecksumFile = 0;

    mIndexTable = new OP1AIndexTable(INDEX_SID, BODY_SID, frame_rate, (flavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR));
    mCPManager = new OP1AContentPackageManager(mMXFFile, mIndexTable, frame_rate, mEssencePartitionKAGSize, MIN_LLEN);

    if (flavour & OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR) {
        mMXFChecksumFile = mxf_checksum_file_open(mMXFFile->getCFile(), MD5_CHECKSUM);
        mMXFFile->swapCFile(mxf_checksum_file_get_file(mMXFChecksumFile));
    }

    if ((flavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR)) {
        mFlavour |= OP1A_BODY_PARTITIONS_FLAVOUR | OP1A_ZERO_MP_TRACK_NUMBER_FLAVOUR;
        ReserveHeaderMetadataSpace(2 * 1024 * 1024 + 8192);
        SetAddSystemItem(true);
    }

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

void OP1AFile::SetHaveInputUserTimecode(bool enable)
{
    mCPManager->SetHaveInputUserTimecode(enable);
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

void OP1AFile::SetInputDuration(int64_t duration)
{
    if (mFlavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)
        mInputDuration = duration;
}

void OP1AFile::SetPartitionInterval(int64_t frame_count)
{
    BMX_CHECK(frame_count >= 0);
    mPartitionInterval = frame_count;
}

void OP1AFile::SetClipWrapped(bool enable)
{
    BMX_CHECK(mTracks.empty());
    mFrameWrapped = !enable;
    mCPManager->SetClipWrapped(enable);
}

void OP1AFile::SetAddSystemItem(bool enable)
{
    if (enable) {
        mCPManager->RegisterSystemItem();
        mIndexTable->RegisterSystemItem();
    }
}

void OP1AFile::SetOutputStartOffset(int64_t offset)
{
    BMX_CHECK(offset >= 0);
    BMX_CHECK(!mSupportCompleteSinglePass); // start offset must be set before metadata is prepared in this case
    mOutputStartOffset = offset;
}

void OP1AFile::SetOutputEndOffset(int64_t offset)
{
    BMX_CHECK(offset <= 0);
    BMX_CHECK(!mSupportCompleteSinglePass); // end offset must be set before metadata is prepared in this case
    mOutputEndOffset = offset;
}

OP1ATrack* OP1AFile::CreateTrack(EssenceType essence_type)
{
    if (essence_type == ANC_DATA) {
        if (mHaveANCTrack)
            BMX_EXCEPTION(("Only a single ST 436 ANC track is allowed"));
        mHaveANCTrack = true;
    }
    if (essence_type == VBI_DATA) {
        if (mHaveVBITrack)
            BMX_EXCEPTION(("Only a single ST 436 VBI track is allowed"));
        mHaveVBITrack = true;
    }

    MXFDataDefEnum data_def = convert_essence_type_to_data_def(essence_type);
    uint32_t track_id;
    uint8_t track_type_number;
    if (data_def == MXF_PICTURE_DDEF)
        track_id = FIRST_PICTURE_TRACK_ID;
    else if (data_def == MXF_SOUND_DDEF)
        track_id = FIRST_SOUND_TRACK_ID;
    else
        track_id = FIRST_DATA_TRACK_ID;
    track_id += mTrackCounts[data_def];
    track_type_number = mTrackCounts[data_def];
    mTrackCounts[data_def]++;

    BMX_CHECK_M(mFrameWrapped || (mTracks.empty() && data_def == MXF_SOUND_DDEF && mTrackCounts[data_def] == 1),
                ("OP-1A clip wrapping only supported for a single sound track"));

    mTracks.push_back(OP1ATrack::Create(this, (uint32_t)mTracks.size(), track_id, track_type_number, mEditRate,
                                        essence_type));
    mTrackMap[mTracks.back()->GetTrackIndex()] = mTracks.back();

    if (!mFrameWrapped) {
        mEditRate = mTracks.back()->GetEditRate();
        mIndexTable->SetEditRate(mEditRate);
    }

    return mTracks.back();
}

void OP1AFile::PrepareHeaderMetadata()
{
    if (mHeaderMetadata)
        return;

    BMX_CHECK(!mTracks.empty());

    // sort tracks, picture - sound - data
    stable_sort(mTracks.begin(), mTracks.end(), compare_track);

    map<MXFDataDefEnum, uint32_t> last_track_number;
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (!mTracks[i]->IsOutputTrackNumberSet())
            mTracks[i]->SetOutputTrackNumber(last_track_number[mTracks[i]->GetDataDef()] + 1);
        last_track_number[mTracks[i]->GetDataDef()] = mTracks[i]->GetOutputTrackNumber();
    }

    if ((mFlavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR)) {
        if (mIndexTable->IsCBE())
            mIndexTable->SetExtensions(MXF_OPT_BOOL_TRUE, MXF_OPT_BOOL_TRUE, MXF_OPT_BOOL_TRUE);
        else
            mIndexTable->SetExtensions(MXF_OPT_BOOL_FALSE, MXF_OPT_BOOL_FALSE, MXF_OPT_BOOL_FALSE);
    }

    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->PrepareWrite(mTrackCounts[mTracks[i]->GetDataDef()]);
    mCPManager->SetStartTimecode(mStartTimecode);
    mCPManager->PrepareWrite();
    mIndexTable->PrepareWrite();

    if (mTracks.size() > 1)
        mEssenceContainerULs.insert(MXF_EC_L(MultipleWrappings));
    for (i = 0; i < mTracks.size(); i++)
        mEssenceContainerULs.insert(mTracks[i]->GetEssenceContainerUL());

    if ((mFlavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR) && mInputDuration >= 0) {
        if (mPartitionInterval == 0 && mIndexTable->IsCBE()) {
            mSupportCompleteSinglePass = true;
            mIndexTable->SetInputDuration(mInputDuration);
        } else {
            if (!mIndexTable->IsCBE()) {
                log_warn("Closing and completing the header partition in a single pass write is not supported "
                         "because essence edit unit size is variable\n");
            }
            if (mPartitionInterval != 0) {
                log_warn("Closing and completing the header partition in a single pass write is not supported "
                         "because partition interval is non-zero\n");
            }
            mSupportCompleteSinglePass = false;
        }
    }

    CreateHeaderMetadata();
}

void OP1AFile::PrepareWrite()
{
    mReserveMinBytes += 256; // account for extra bytes when updating header metadata

    if (!mHeaderMetadata)
        PrepareHeaderMetadata();

    CreateFile();
}

void OP1AFile::WriteUserTimecode(Timecode user_timecode)
{
    mCPManager->WriteUserTimecode(user_timecode);

    WriteContentPackages(false);
}

void OP1AFile::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (!data || size == 0)
        return;
    BMX_CHECK(data && size && num_samples);

    GetTrack(track_index)->WriteSamplesInt(data, size, num_samples);

    WriteContentPackages(false);
}

void OP1AFile::CompleteWrite()
{
    BMX_ASSERT(mMXFFile);


    // write any remaining content packages (e.g. first AVCI cp if duration equals 1)

    WriteContentPackages(true);


    // check that the duration is valid

    BMX_CHECK_M(mIndexTable->GetDuration() - mOutputStartOffset + mOutputEndOffset >= 0,
               ("Invalid output start %"PRId64" / end %"PRId64" offsets. Output duration %"PRId64" is negative",
                mOutputStartOffset, mOutputEndOffset, mIndexTable->GetDuration() - mOutputStartOffset + mOutputEndOffset));

    if (mSupportCompleteSinglePass) {
        BMX_CHECK_M(mIndexTable->GetDuration() == mInputDuration,
                    ("Single pass write failed because OP-1A container duration %"PRId64" "
                     "does not equal set input duration %"PRId64,
                     mIndexTable->GetDuration(), mInputDuration));
    }


    // complete write for tracks

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->CompleteWrite();


    // update metadata sets with duration

    if (!mSupportCompleteSinglePass)
        UpdatePackageMetadata();


    // first write and complete last part to memory

    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);


    // non-minimal partition flavour: write any remaining VBE index segments

    if (!(mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) &&
        !(mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR) &&
        mIndexTable->IsVBE() && mIndexTable->HaveSegments())
    {
        Partition &index_partition = mMXFFile->createPartition();
        index_partition.setKey(&MXF_PP_K(ClosedComplete, Body)); // will be complete when memory flushed
        index_partition.setIndexSID(INDEX_SID);
        index_partition.setBodySID(0);
        index_partition.setKagSize(mKAGSize);
        index_partition.write(mMXFFile);

        mIndexTable->WriteSegments(mMXFFile, &index_partition, true);
    }


    // write the footer partition pack

    if (mSupportCompleteSinglePass) {
        BMX_CHECK_M(mMXFFile->tell() == (int64_t)mMXFFile->getPartition(0).getFooterPartition(),
                    ("Single pass write failed because footer partition offset %"PRId64" "
                     "is not at expected offset %"PRId64,
                     mMXFFile->tell(), mMXFFile->getPartition(0).getFooterPartition()));
    }

    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer)); // will be complete when memory flushed
    if (((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) &&
          mIndexTable->IsVBE() && mIndexTable->HaveSegments())
    {
        footer_partition.setIndexSID(INDEX_SID);
    }
    else
    {
        footer_partition.setIndexSID(0);
    }
    footer_partition.setBodySID(0);
    footer_partition.setKagSize(mKAGSize);
    footer_partition.write(mMXFFile);


    // write (complete) header metadata if it is a single pass write and the header partition is incomplete

    if ((mFlavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR) && !mSupportCompleteSinglePass) {
        KAGFillerWriter reserve_filler_writer(&footer_partition, mReserveMinBytes);
        mHeaderMetadata->write(mMXFFile, &footer_partition, &reserve_filler_writer);
    }


    // minimal/body partitions flavour: write any remaining VBE index segments

    if (((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) &&
        mIndexTable->IsVBE() && mIndexTable->HaveSegments())
    {
        mIndexTable->WriteSegments(mMXFFile, &footer_partition, true);
    }


    // write the RIP

    mMXFFile->writeRIP();


    // update final in-memory partitions and flush to file

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();



    // update previous partitions if not writing in a single pass

    if (!(mFlavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)) {
        // re-write header and cbe index partition to memory

        mMXFFile->seek(0, SEEK_SET);
        mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);
        mMXFFile->setMemoryPartitionIndexes(0, 0); // overwriting and updating from header pp


        // re-write the header partition pack

        Partition &header_partition = mMXFFile->getPartition(0);
        header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
        header_partition.setFooterPartition(footer_partition.getThisPartition());
        header_partition.write(mMXFFile);


        // re-write the header metadata

        PositionFillerWriter pos_filler_writer(mHeaderMetadataEndPos);
        mHeaderMetadata->write(mMXFFile, &header_partition, &pos_filler_writer);


        // re-write the CBE index table segment(s)

        if (!mFirstWrite && mIndexTable->IsCBE()) {
            Partition *index_partition;
            if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) {
                index_partition = &mMXFFile->getPartition(0);
            } else {
                index_partition = &mMXFFile->getPartition(1);
                index_partition->setKey(&MXF_PP_K(ClosedComplete, Body));
                index_partition->setFooterPartition(footer_partition.getThisPartition());
                index_partition->write(mMXFFile);
            }

            mIndexTable->WriteSegments(mMXFFile, index_partition, true);
        }


        // update header and index partition packs and flush memory writes to file

        mMXFFile->updatePartitions();
        mMXFFile->closeMemoryFile();


        // update and re-write the body partition packs

        if (!(mFlavour & OP1A_NO_BODY_PART_UPDATE_FLAVOUR))
            mMXFFile->updateBodyPartitions(&MXF_PP_K(ClosedComplete, Body));
    }


    // finalize md5

    if (mMXFChecksumFile) {
        mxf_checksum_file_final(mMXFChecksumFile);
        mMD5DigestStr = mxf_checksum_file_digest_str(mMXFChecksumFile);
    }


    // done with the file
    delete mMXFFile;
    mMXFFile = 0;
}

int64_t OP1AFile::GetDuration() const
{
    int64_t container_duration = GetContainerDuration();

    if (container_duration - mOutputStartOffset + mOutputEndOffset <= 0)
        return 0;

    return container_duration - mOutputStartOffset + mOutputEndOffset;
}

int64_t OP1AFile::GetContainerDuration() const
{
    return mIndexTable->GetDuration();
}

OP1ATrack* OP1AFile::GetTrack(uint32_t track_index)
{
    BMX_ASSERT(track_index < mTracks.size());
    return mTrackMap[track_index];
}

void OP1AFile::CreateHeaderMetadata()
{
    BMX_ASSERT(!mHeaderMetadata);

    int64_t material_track_duration = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_duration = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_origin = 0; // could be updated if not writing in a single pass
    if (mSupportCompleteSinglePass) {
        // values are known and will not be updated
        material_track_duration = mInputDuration - mOutputStartOffset + mOutputEndOffset;
        if (material_track_duration < 0)
            material_track_duration = 0;
        source_track_duration = mInputDuration + mOutputEndOffset;
        source_track_origin = mOutputStartOffset;
    }

    // create the header metadata
    mDataModel = new DataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);


    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(mCreationDate);
    preface->setVersion((mFlavour & OP1A_377_2004_FLAVOUR) ? MXF_PREFACE_VER(1, 2) : MXF_PREFACE_VER(1, 3));
    if (mTracks.size() <= 1)
        preface->setOperationalPattern(MXF_OP_L(1a, UniTrack_Stream_Internal));
    else
        preface->setOperationalPattern(MXF_OP_L(1a, MultiTrack_Stream_Internal));
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        preface->appendEssenceContainers(*iter);
    preface->setDMSchemes(vector<mxfUL>());
    if ((mFlavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        preface->setIsRIPPresent(true);

    // Preface - Identification
    Identification *ident = new Identification(mHeaderMetadata);
    preface->appendIdentifications(ident);
    ident->initialise(mCompanyName, mProductName, mVersionString, mProductUID);
    if (mProductVersion.major != 0 || mProductVersion.minor != 0 || mProductVersion.patch != 0 ||
        mProductVersion.build != 0 || mProductVersion.release != 0)
    {
        ident->setProductVersion(mProductVersion);
    }
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
    timecode_track->setEditRate(mEditRate);
    timecode_track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timecode Track - Sequence
    Sequence *sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(material_track_duration);

    // Preface - ContentStorage - MaterialPackage - Timecode Track - TimecodeComponent
    TimecodeComponent *timecode_component = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(timecode_component);
    timecode_component->setDataDefinition(MXF_DDEF_L(Timecode));
    timecode_component->setDuration(material_track_duration);
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
    timecode_track->setEditRate(mEditRate);
    timecode_track->setOrigin(source_track_origin);

    // Preface - ContentStorage - SourcePackage - Timecode Track - Sequence
    sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(source_track_duration);

    // Preface - ContentStorage - SourcePackage - Timecode Track - TimecodeComponent
    timecode_component = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(timecode_component);
    timecode_component->setDataDefinition(MXF_DDEF_L(Timecode));
    timecode_component->setDuration(source_track_duration);
    Timecode sp_start_timecode = mStartTimecode;
    sp_start_timecode.AddOffset(- mOutputStartOffset, mFrameRate);
    timecode_component->setRoundedTimecodeBase(sp_start_timecode.GetRoundedTCBase());
    timecode_component->setDropFrame(sp_start_timecode.IsDropFrame());
    timecode_component->setStartTimecode(sp_start_timecode.GetOffset());

    // Preface - ContentStorage - SourcePackage - (Multiple) File Descriptor
    if (mTracks.size() > 1) {
        MultipleDescriptor *mult_descriptor = new MultipleDescriptor(mHeaderMetadata);
        mFileSourcePackage->setDescriptor(mult_descriptor);
        mult_descriptor->setSampleRate(mEditRate);
        mult_descriptor->setEssenceContainer(MXF_EC_L(MultipleWrappings));
        if (mSupportCompleteSinglePass)
            mult_descriptor->setContainerDuration(mInputDuration);
    }

    // MaterialPackage and file SourcePackage Tracks and FileDescriptor
    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->AddHeaderMetadata(mHeaderMetadata, mMaterialPackage, mFileSourcePackage);
}

void OP1AFile::CreateFile()
{
    BMX_ASSERT(mHeaderMetadata);


    // set minimum llen

    mMXFFile->setMinLLen(MIN_LLEN);


    // write to memory until essence data writing starts or there is no essence and the file is completed

    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);


    // write the header metadata

    Partition &header_partition = mMXFFile->createPartition();
    if (mSupportCompleteSinglePass)
        header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
    else
        header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setVersion(1, ((mFlavour & OP1A_377_2004_FLAVOUR) ? 2 : 3));
    if (((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) &&
        mIndexTable->IsCBE())
    {
        header_partition.setIndexSID(INDEX_SID);
    }
    else
    {
        header_partition.setIndexSID(0);
    }
    if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR))
        header_partition.setBodySID(BODY_SID);
    else
        header_partition.setBodySID(0);
    header_partition.setKagSize(mKAGSize);
    if (mTracks.size() <= 1)
        header_partition.setOperationalPattern(&MXF_OP_L(1a, UniTrack_Stream_Internal));
    else
        header_partition.setOperationalPattern(&MXF_OP_L(1a, MultiTrack_Stream_Internal));
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        header_partition.addEssenceContainer(&(*iter));
    header_partition.write(mMXFFile);

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

    BMX_ASSERT(mFileSourcePackage->haveDescriptor());
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
        BMX_ASSERT(sequence);
        vector<StructuralComponent*> components = sequence->getStructuralComponents();
        if (sequence->getDuration() < 0) {
            sequence->setDuration(duration);
            BMX_ASSERT(components.size() == 1);
            components[0]->setDuration(duration);
        }
        if (components.size() == 1) {
            TimecodeComponent *timecode_component = dynamic_cast<TimecodeComponent*>(components[0]);
            if (timecode_component) {
                Timecode start_timecode = mStartTimecode;
                start_timecode.AddOffset(- origin, mFrameRate);
                timecode_component->setRoundedTimecodeBase(start_timecode.GetRoundedTCBase());
                timecode_component->setDropFrame(start_timecode.IsDropFrame());
                timecode_component->setStartTimecode(start_timecode.GetOffset());
            }
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
                mCPManager->UpdateIndexTable(mCPManager->HaveContentPackages(2) ? 2 : 1);

                if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) {
                    mIndexTable->WriteSegments(mMXFFile, &mMXFFile->getPartition(0), false);
                } else {
                    Partition &index_partition = mMXFFile->createPartition();
                    if (mSupportCompleteSinglePass)
                        index_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
                    else
                        index_partition.setKey(&MXF_PP_K(OpenComplete, Body));
                    index_partition.setIndexSID(INDEX_SID);
                    index_partition.setBodySID(0);
                    index_partition.setKagSize(mKAGSize);
                    index_partition.write(mMXFFile);

                    mIndexTable->WriteSegments(mMXFFile, &index_partition, false);
                }
            }

            if (!(mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR))
                start_ess_partition = true;
            mFirstWrite = false;
        }
        else if (mPartitionInterval > 0 &&
                 mPartitionFrameCount >= mPartitionInterval && mIndexTable->CanStartPartition())
        {
            BMX_ASSERT(!mSupportCompleteSinglePass);

            start_ess_partition = true;
            mPartitionFrameCount = 0;

            // write VBE index table segments and ensure new essence partition is started

            if (mIndexTable->IsVBE()) {
                BMX_ASSERT(mIndexTable->HaveSegments());
                BMX_ASSERT(!mSupportCompleteSinglePass);

                mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);

                Partition &index_partition = mMXFFile->createPartition();
                index_partition.setKey(&MXF_PP_K(OpenComplete, Body));
                index_partition.setIndexSID(INDEX_SID);
                if ((mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) {
                    index_partition.setBodySID(BODY_SID);
                    index_partition.setKagSize(mEssencePartitionKAGSize);
                    index_partition.setBodyOffset(ess_partition_body_offset);
                    start_ess_partition = false;
                } else {
                    index_partition.setBodySID(0);
                    index_partition.setKagSize(mKAGSize);
                }
                index_partition.write(mMXFFile);

                mIndexTable->WriteSegments(mMXFFile, &index_partition, true);

                mMXFFile->updatePartitions();
                mMXFFile->closeMemoryFile();
            }
        }

        if (start_ess_partition) {
            Partition &ess_partition = mMXFFile->createPartition();
            if (mSupportCompleteSinglePass)
                ess_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
            else
                ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
            ess_partition.setIndexSID(0);
            ess_partition.setBodySID(BODY_SID);
            ess_partition.setKagSize(mEssencePartitionKAGSize);
            ess_partition.setBodyOffset(ess_partition_body_offset);
            ess_partition.write(mMXFFile);
        }

        if (mMXFFile->isMemoryFileOpen()) {
            if (mSupportCompleteSinglePass)
                SetPartitionsFooterOffset();
            mMXFFile->updatePartitions();
            mMXFFile->closeMemoryFile();
        }

        mCPManager->WriteNextContentPackage();

        if (mPartitionInterval > 0)
            mPartitionFrameCount++;
    }

    if (end_of_samples) {
        mCPManager->CompleteWrite();

        if (mMXFFile->isMemoryFileOpen()) {
            if (mSupportCompleteSinglePass)
                SetPartitionsFooterOffset();
            mMXFFile->updatePartitions();
            mMXFFile->closeMemoryFile();
        }
    }
}

void OP1AFile::SetPartitionsFooterOffset()
{
    BMX_ASSERT(mSupportCompleteSinglePass);

    uint32_t first_size = 0, non_first_size = 0;
    mIndexTable->GetCBEEditUnitSize(&first_size, &non_first_size);

    mFooterPartitionOffset = mMXFFile->tell();
    if (mInputDuration > 0)
        mFooterPartitionOffset += first_size + (mInputDuration - 1) * non_first_size;

    size_t i;
    for (i = 0; i < mMXFFile->getPartitions().size(); i++)
        mMXFFile->getPartitions()[i]->setFooterPartition(mFooterPartitionOffset);
}

