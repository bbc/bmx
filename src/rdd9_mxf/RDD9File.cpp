/*
 * Copyright (C) 2012, British Broadcasting Corporation
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
#include <cstring>

#include <algorithm>

#include <bmx/rdd9_mxf/RDD9File.h>
#include <bmx/Version.h>
#include <bmx/MXFUtils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define MAX_GOP_SIZE    15

static const uint32_t TIMECODE_TRACK_ID         = 1;
static const uint32_t FIRST_PICTURE_TRACK_ID    = 2;
static const uint32_t FIRST_SOUND_TRACK_ID      = 4;
static const uint32_t FIRST_DATA_TRACK_ID       = 101;
static const uint32_t KAG_SIZE                  = 0x200;
static const uint32_t INDEX_SID                 = 1;
static const uint32_t BODY_SID                  = 2;
static const uint32_t MEMORY_WRITE_CHUNK_SIZE   = 8192;



static bool compare_track(RDD9Track *left, RDD9Track *right)
{
    return left->GetDataDef() < right->GetDataDef();
}



RDD9File::RDD9File(int flavour, mxfpp::File *mxf_file, Rational frame_rate)
{
    BMX_CHECK(frame_rate == FRAME_RATE_23976 ||
              frame_rate == FRAME_RATE_24 ||
              frame_rate == FRAME_RATE_25 ||
              frame_rate == FRAME_RATE_2997 ||
              frame_rate == FRAME_RATE_50 ||
              frame_rate == FRAME_RATE_5994);

    if (frame_rate == FRAME_RATE_24)
        log_warn("Frame rate is 24Hz; RDD9 does not list this rate in the supported rates list\n");

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
    mxf_get_timestamp_now(&mCreationDate);
    mxf_generate_uuid(&mGenerationUID);
    mxf_generate_umid(&mMaterialPackageUID);
    mxf_generate_umid(&mFileSourcePackageUID);
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
    mPartitionInterval = 10 * frame_rate.numerator / frame_rate.denominator;
    mPartitionFrameCount = 0;
    mMXFChecksumFile = 0;

    mIndexTable = new RDD9IndexTable(INDEX_SID, BODY_SID, frame_rate, (flavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR));
    mCPManager = new RDD9ContentPackageManager(mMXFFile, mIndexTable, frame_rate);

    if (flavour & RDD9_SINGLE_PASS_MD5_WRITE_FLAVOUR) {
        mMXFChecksumFile = mxf_checksum_file_open(mMXFFile->getCFile(), MD5_CHECKSUM);
        mMXFFile->swapCFile(mxf_checksum_file_get_file(mMXFChecksumFile));
    }

    if ((flavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        ReserveHeaderMetadataSpace(2 * 1024 * 1024 + 8192);

    if (!(flavour & RDD9_SMPTE_377_2004_FLAVOUR)) {
        // use fill key with correct version number
        g_KLVFill_key = g_CompliantKLVFill_key;
    }
}

RDD9File::~RDD9File()
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

void RDD9File::SetClipName(string name)
{
    mClipName = name;
}

void RDD9File::SetStartTimecode(Timecode start_timecode)
{
    mStartTimecode = start_timecode;
}

void RDD9File::SetHaveInputUserTimecode(bool enable)
{
    mCPManager->SetHaveInputUserTimecode(enable);
}

void RDD9File::SetProductInfo(string company_name, string product_name, mxfProductVersion product_version,
                              string version, mxfUUID product_uid)
{
    mCompanyName    = company_name;
    mProductName    = product_name;
    mProductVersion = product_version;
    mVersionString  = version;
    mProductUID     = product_uid;
}

void RDD9File::SetCreationDate(mxfTimestamp creation_date)
{
    mCreationDate = creation_date;
}

void RDD9File::SetGenerationUID(mxfUUID generation_uid)
{
    mGenerationUID = generation_uid;
}

void RDD9File::SetMaterialPackageUID(mxfUMID package_uid)
{
    mMaterialPackageUID = package_uid;
}

void RDD9File::SetFileSourcePackageUID(mxfUMID package_uid)
{
    mFileSourcePackageUID = package_uid;
}

void RDD9File::ReserveHeaderMetadataSpace(uint32_t min_bytes)
{
    mReserveMinBytes = min_bytes;
}

void RDD9File::SetPartitionInterval(int64_t frame_count)
{
    BMX_CHECK(frame_count >= 0);
    mPartitionInterval = frame_count;
}

void RDD9File::SetOutputStartOffset(int64_t offset)
{
    BMX_CHECK(offset >= 0);
    mOutputStartOffset = offset;
}

void RDD9File::SetOutputEndOffset(int64_t offset)
{
    BMX_CHECK(offset <= 0);
    mOutputEndOffset = offset;
}

RDD9Track* RDD9File::CreateTrack(EssenceType essence_type)
{
    if (essence_type == ANC_DATA) {
        if (mHaveANCTrack)
            BMX_EXCEPTION(("Only a single ST 436 ANC track is allowed"));
        mHaveANCTrack = true;
    }
    else if (essence_type == VBI_DATA) {
        if (mHaveVBITrack)
            BMX_EXCEPTION(("Only a single ST 436 VBI track is allowed"));
        mHaveVBITrack = true;
    }

    MXFDataDefEnum data_def = convert_essence_type_to_data_def(essence_type);
    uint32_t track_id;
    uint8_t track_type_number;

    if (data_def == MXF_PICTURE_DDEF) {
        if (mTrackCounts[MXF_PICTURE_DDEF] > 1)
            BMX_EXCEPTION(("A maximum of 2 MPEG-2 Long GOP picture tracks are supported"));
        track_id = FIRST_PICTURE_TRACK_ID;
    } else if (data_def == MXF_SOUND_DDEF) {
        track_id = FIRST_SOUND_TRACK_ID;
    } else {
        track_id = FIRST_DATA_TRACK_ID;
    }

    track_id += mTrackCounts[data_def];
    track_type_number = mTrackCounts[data_def];
    mTrackCounts[data_def]++;

    mTracks.push_back(RDD9Track::Create(this, (uint32_t)mTracks.size(), track_id, track_type_number, mEditRate,
                                        essence_type));
    mTrackMap[mTracks.back()->GetTrackIndex()] = mTracks.back();

    return mTracks.back();
}

void RDD9File::PrepareHeaderMetadata()
{
    if (mHeaderMetadata)
        return;

    if (mTrackCounts[MXF_PICTURE_DDEF] == 0)
        BMX_EXCEPTION(("Require a MPEG-2 Long GOP picture track"));

    // check number of audio tracks
    uint8_t ac = mTrackCounts[MXF_SOUND_DDEF];
    if (ac == 0)
        BMX_EXCEPTION(("Require at least 1 sound track"));
    if (mFlavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR) {
        if ((ac != 8) && (ac != 16))
            log_warn("There are %u audio tracks; ARD ZDF profile requires 8 or 16 audio tracks\n", ac);
    }
    else if ((ac != 2) && (ac != 4) && (ac != 8)) {
        log_warn("There are %u audio tracks; RDD9 requires 2, 4, or 8 audio tracks\n", ac);
    }

    // sort tracks, picture - sound - data
    stable_sort(mTracks.begin(), mTracks.end(), compare_track);


    map<MXFDataDefEnum, uint32_t> last_track_number;
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (!mTracks[i]->IsOutputTrackNumberSet())
            mTracks[i]->SetOutputTrackNumber(last_track_number[mTracks[i]->GetDataDef()] + 1);
        last_track_number[mTracks[i]->GetDataDef()] = mTracks[i]->GetOutputTrackNumber();
    }

    if ((mFlavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        mIndexTable->SetExtensions(MXF_OPT_BOOL_FALSE, MXF_OPT_BOOL_FALSE, MXF_OPT_BOOL_FALSE);


    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->PrepareWrite(mTrackCounts[mTracks[i]->GetDataDef()]);

    mCPManager->SetStartTimecode(mStartTimecode);
    mCPManager->PrepareWrite();
    mIndexTable->PrepareWrite();

    for (i = 0; i < mTracks.size(); i++)
        mEssenceContainerULs.insert(mTracks[i]->GetEssenceContainerUL());
    // ST 377-2004 Sony RDD9 sample files didn't include the MultipleWrappings label in the partition packs and preface
    if (!(mFlavour & RDD9_SMPTE_377_2004_FLAVOUR))
        mEssenceContainerULs.insert(MXF_EC_L(MultipleWrappings));

    CreateHeaderMetadata();
}

void RDD9File::PrepareWrite()
{
    mReserveMinBytes += 256; // account for extra bytes when updating header metadata

    if (!mHeaderMetadata)
        PrepareHeaderMetadata();

    CreateFile();
}

void RDD9File::WriteUserTimecode(Timecode user_timecode)
{
    mCPManager->WriteUserTimecode(user_timecode);

    WriteContentPackages(false);
}

void RDD9File::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (!data || size == 0)
        return;

    BMX_CHECK(data && size && num_samples);

    GetTrack(track_index)->WriteSamplesInt(data, size, num_samples);

    WriteContentPackages(false);
}

void RDD9File::CompleteWrite()
{
    BMX_ASSERT(mMXFFile);


    // write any remaining content packages, i.e. unknown sound sequence offset has prevented write

    WriteContentPackages(true);


    // complete write for tracks

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->CompleteWrite();


    // update metadata sets with duration

    UpdatePackageMetadata();


    // write footer partition to memory

    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);


    // write the footer partition pack

    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer));
    if (mIndexTable->HaveSegments() ||
        ((mFlavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR) && mIndexTable->HaveWrittenSegments()))
    {
        footer_partition.setIndexSID(INDEX_SID);
    }
    else
    {
        footer_partition.setIndexSID(0);
    }
    footer_partition.setBodySID(0);
    footer_partition.setKagSize(KAG_SIZE);
    footer_partition.write(mMXFFile);


    // write the complete header metadata if it is a single pass write

    if ((mFlavour & RDD9_SINGLE_PASS_WRITE_FLAVOUR)) {
        KAGFillerWriter reserve_filler_writer(&footer_partition, mReserveMinBytes);
        mHeaderMetadata->write(mMXFFile, &footer_partition, &reserve_filler_writer);
    }


    // write any remaining VBE index segments

    if (mIndexTable->HaveSegments() ||
        ((mFlavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR) && mIndexTable->HaveWrittenSegments()))
    {
        mIndexTable->WriteSegments(mMXFFile, &footer_partition);
    }


    // write the RIP

    mMXFFile->writeRIP();


    // update footer partition and flush to file

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();


    // update header partition if not writing in a single pass

    if (!(mFlavour & RDD9_SINGLE_PASS_WRITE_FLAVOUR)) {
        // re-write header to memory

        mMXFFile->seek(0, SEEK_SET);
        mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);
        mMXFFile->setMemoryPartitionIndexes(0, 0); // overwriting and updating header pp


        // update and re-write the header partition pack

        Partition &header_partition = mMXFFile->getPartition(0);
        header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
        header_partition.setFooterPartition(footer_partition.getThisPartition());
        header_partition.write(mMXFFile);


        // re-write the header metadata

        PositionFillerWriter pos_filler_writer(mHeaderMetadataEndPos);
        mHeaderMetadata->write(mMXFFile, &header_partition, &pos_filler_writer);


        // update header partition and flush to file

        mMXFFile->updatePartitions();
        mMXFFile->closeMemoryFile();


        // update body partition status and re-write the partition packs

        if (!(mFlavour & RDD9_NO_BODY_PART_UPDATE_FLAVOUR))
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

int64_t RDD9File::GetDuration() const
{
    int64_t container_duration = GetContainerDuration();

    if (container_duration - mOutputStartOffset + mOutputEndOffset <= 0)
        return 0;
    else
        return container_duration - mOutputStartOffset + mOutputEndOffset;
}

int64_t RDD9File::GetContainerDuration() const
{
    return mIndexTable->GetDuration();
}

RDD9Track* RDD9File::GetTrack(uint32_t track_index)
{
    BMX_ASSERT(track_index < mTracks.size());
    return mTrackMap[track_index];
}

void RDD9File::CreateHeaderMetadata()
{
    BMX_ASSERT(!mHeaderMetadata);

    int64_t material_track_duration = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_duration   = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_origin     =  0; // could be updated if not writing in a single pass


    // create the header metadata
    mDataModel = new DataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);


    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(mCreationDate);
    if ((mFlavour & RDD9_SMPTE_377_2004_FLAVOUR))
        preface->setVersion(MXF_PREFACE_VER(1, 2));
    else
        preface->setVersion(MXF_PREFACE_VER(1, 3));
    preface->setOperationalPattern(MXF_OP_L(1a, MultiTrack_Stream_Internal));
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        preface->appendEssenceContainers(*iter);
    preface->setDMSchemes(vector<mxfUL>());
    if ((mFlavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR))
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
    timecode_track->setTrackName("TC1");
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
    timecode_track->setTrackName("TC1");
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
    MultipleDescriptor *mult_descriptor = new MultipleDescriptor(mHeaderMetadata);
    mFileSourcePackage->setDescriptor(mult_descriptor);
    mult_descriptor->setSampleRate(mEditRate);
    mult_descriptor->setEssenceContainer(MXF_EC_L(MultipleWrappings));


    // MaterialPackage and file SourcePackage Tracks and FileDescriptor
    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->AddHeaderMetadata(mHeaderMetadata, mMaterialPackage, mFileSourcePackage);
}

void RDD9File::CreateFile()
{
    BMX_ASSERT(mHeaderMetadata);


    // set minimum llen

    mMXFFile->setMinLLen(4);


    // write the header metadata

    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);

    Partition &header_partition = mMXFFile->createPartition();
    header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    if ((mFlavour & RDD9_SMPTE_377_2004_FLAVOUR))
        header_partition.setVersion(1, 2);
    else
        header_partition.setVersion(1, 3);
    header_partition.setIndexSID(0);
    header_partition.setBodySID(0);
    header_partition.setKagSize(KAG_SIZE);
    header_partition.setOperationalPattern(&MXF_OP_L(1a, MultiTrack_Stream_Internal));
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        header_partition.addEssenceContainer(&(*iter));
    header_partition.write(mMXFFile);

    KAGFillerWriter reserve_filler_writer(&header_partition, mReserveMinBytes);
    mHeaderMetadata->write(mMXFFile, &header_partition, &reserve_filler_writer);
    mHeaderMetadataEndPos = mMXFFile->tell();  // need this position when we re-write the header metadata

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();
}

void RDD9File::UpdatePackageMetadata()
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
    BMX_ASSERT(mult_descriptor);
    vector<GenericDescriptor*> sub_descriptors = mult_descriptor->getSubDescriptorUIDs();
    size_t i;
    for (i = 0; i < sub_descriptors.size(); i++) {
        file_descriptor = dynamic_cast<FileDescriptor*>(sub_descriptors[i]);
        if (file_descriptor)
            file_descriptor->setContainerDuration(container_duration);
    }
}

void RDD9File::UpdateTrackMetadata(GenericPackage *package, int64_t origin, int64_t duration)
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

void RDD9File::WriteContentPackages(bool final_write)
{
    while (mCPManager->HaveContentPackage(final_write))
    {
        // start body partition at first write or when # frames per partition close to exceeding maximum
        if (mFirstWrite ||
            (mPartitionInterval > 0 && mPartitionFrameCount > 0 &&
                mPartitionFrameCount + MAX_GOP_SIZE >= mPartitionInterval && mIndexTable->CanStartPartition()))
        {
            mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);

            Partition &body_partition = mMXFFile->createPartition();
            body_partition.setKey(&MXF_PP_K(OpenComplete, Body));
            if (mIndexTable->HaveSegments())
                body_partition.setIndexSID(INDEX_SID);
            else
                body_partition.setIndexSID(0);
            body_partition.setBodySID(BODY_SID);
            body_partition.setKagSize(KAG_SIZE);
            body_partition.setBodyOffset(mIndexTable->GetStreamOffset());
            body_partition.write(mMXFFile);

            if (mIndexTable->HaveSegments())
                mIndexTable->WriteSegments(mMXFFile, &body_partition);

            mMXFFile->updatePartitions();
            mMXFFile->closeMemoryFile();

            mFirstWrite = false;
            mPartitionFrameCount = 0;
        }

        mCPManager->WriteNextContentPackage();

        if (mPartitionInterval > 0)
            mPartitionFrameCount++;
    }
}

