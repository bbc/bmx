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
#include <cstring>

#include <bmx/d10_mxf/D10File.h>
#include <bmx/mxf_helper/MXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Version.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const uint32_t KAG_SIZE                = 0x200;
static const mxfRational AUDIO_SAMPLING_RATE  = SAMPLING_RATE_48K;
static const uint32_t MEMORY_WRITE_CHUNK_SIZE = 8192;



typedef struct
{
    EssenceType essence_type;
    mxfRational frame_rate;
    mxfUL ec_label;
} EssenceContainerULTable;

static const EssenceContainerULTable ESS_CONTAINER_UL_TABLE[] =
{
    {D10_30,   {25, 1},        MXF_EC_L(D10_30_625_50_defined_template)},
    {D10_30,   {30000, 1001},  MXF_EC_L(D10_30_525_60_defined_template)},
    {D10_40,   {25, 1},        MXF_EC_L(D10_40_625_50_defined_template)},
    {D10_40,   {30000, 1001},  MXF_EC_L(D10_40_525_60_defined_template)},
    {D10_50,   {25, 1},        MXF_EC_L(D10_50_625_50_defined_template)},
    {D10_50,   {30000, 1001},  MXF_EC_L(D10_50_525_60_defined_template)},
};



static mxfUL get_essence_container_ul(EssenceType essence_type, mxfRational frame_rate)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(ESS_CONTAINER_UL_TABLE); i++) {
        if (ESS_CONTAINER_UL_TABLE[i].essence_type == essence_type &&
            ESS_CONTAINER_UL_TABLE[i].frame_rate == frame_rate)
        {
            return ESS_CONTAINER_UL_TABLE[i].ec_label;
        }
    }

    BMX_ASSERT(false);
    return ESS_CONTAINER_UL_TABLE[0].ec_label;
}



mxfUMID D10File::CreatePackageUID()
{
    mxfUMID package_uid;
    mxf_generate_umid(&package_uid);
    return package_uid;
}

D10File::D10File(int flavour, mxfpp::File *mxf_file, mxfRational frame_rate)
{
    BMX_CHECK(frame_rate == FRAME_RATE_25 || frame_rate == FRAME_RATE_2997);

    mFlavour = flavour;
    mMXFFile = mxf_file;
    mFrameRate = frame_rate;
    mStartTimecode = Timecode(frame_rate, false);
    mCompanyName = get_bmx_company_name();
    mProductName = get_bmx_library_name();
    mProductVersion = get_bmx_mxf_product_version();
    mVersionString = get_bmx_mxf_version_string();
    mProductUID = get_bmx_product_uid();
    mReserveMinBytes = 8192;
    mForceWriteCBEDuration0 = false;
    mInputDuration = -1;
    mxf_get_timestamp_now(&mCreationDate);
    mxf_generate_uuid(&mGenerationUID);
    mMaterialPackageUID = D10File::CreatePackageUID();
    mFileSourcePackageUID = D10File::CreatePackageUID();
    mPictureTrack = 0;
    mFirstSoundTrack = 0;
    memset(&mEssenceContainerUL, 0, sizeof(mEssenceContainerUL));
    mDataModel = 0;
    mHeaderMetadata = 0;
    mHeaderMetadataEndPos = 0;
    mMaterialPackage = 0;
    mFileSourcePackage = 0;
    mCPManager = new D10ContentPackageManager(frame_rate);
    mIndexSegment = 0;
    mFirstWrite = true;
    mRequireBodyPartition = false;
    mMXFChecksumFile = 0;

    mTrackIdHelper.SetId("TimecodeTrack", 1);
    mTrackIdHelper.SetId("PictureTrack",  2);
    mTrackIdHelper.SetId("SoundTrack",    3);
    mTrackIdHelper.SetStartId(XML_TRACK_TYPE, 9001);

    mStreamIdHelper.SetId("IndexStream", 1);
    mStreamIdHelper.SetId("BodyStream",  2);
    mStreamIdHelper.SetStartId(GENERIC_STREAM_TYPE, 10);

    if ((flavour & D10_SINGLE_PASS_MD5_WRITE_FLAVOUR)) {
        mMXFChecksumFile = mxf_checksum_file_open(mMXFFile->getCFile(), MD5_CHECKSUM);
        mMXFFile->swapCFile(mxf_checksum_file_get_file(mMXFChecksumFile));
    }
    if ((flavour & D10_AS11_FLAVOUR))
        ReserveHeaderMetadataSpace(4 * 1024 * 1024 + 8192);
}

D10File::~D10File()
{
    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];
    for (i = 0; i < mXMLTracks.size(); i++)
        delete mXMLTracks[i];

    delete mMXFFile;
    delete mDataModel;
    delete mHeaderMetadata;
    delete mCPManager;
    delete mIndexSegment;
}

void D10File::SetClipName(string name)
{
    mClipName = name;
}

void D10File::SetStartTimecode(Timecode start_timecode)
{
    mStartTimecode = start_timecode;
}

void D10File::SetHaveInputUserTimecode(bool enable)
{
    mCPManager->SetHaveInputUserTimecode(enable);
}

void D10File::SetSoundSequenceOffset(uint8_t offset)
{
    mCPManager->SetSoundSequenceOffset(offset);
}

void D10File::SetMuteSoundFlags(uint8_t flags)
{
    mCPManager->SetMuteSoundFlags(flags);
}

void D10File::SetInvalidSoundFlags(uint8_t flags)
{
    mCPManager->SetInvalidSoundFlags(flags);
}

void D10File::SetProductInfo(string company_name, string product_name, mxfProductVersion product_version,
                             string version, mxfUUID product_uid)
{
    mCompanyName = company_name;
    mProductName = product_name;
    mProductVersion = product_version;
    mVersionString = version;
    mProductUID = product_uid;
}

void D10File::SetCreationDate(mxfTimestamp creation_date)
{
    mCreationDate = creation_date;
}

void D10File::SetGenerationUID(mxfUUID generation_uid)
{
    mGenerationUID = generation_uid;
}

void D10File::SetMaterialPackageUID(mxfUMID package_uid)
{
    mMaterialPackageUID = package_uid;
}

void D10File::SetFileSourcePackageUID(mxfUMID package_uid)
{
    mFileSourcePackageUID = package_uid;
}

void D10File::ReserveHeaderMetadataSpace(uint32_t min_bytes)
{
    mReserveMinBytes = min_bytes;
}
void D10File::ForceWriteCBEDuration0(bool enable)
{
    mForceWriteCBEDuration0 = enable;
}

void D10File::SetInputDuration(int64_t duration)
{
    if (mFlavour & D10_SINGLE_PASS_WRITE_FLAVOUR)
        mInputDuration = duration;
}

D10Track* D10File::CreateTrack(EssenceType essence_type)
{
    uint32_t track_index = (uint32_t)mTracks.size();
    mTracks.push_back(D10Track::Create(this, track_index, mFrameRate, essence_type));
    mTrackMap[track_index] = mTracks.back();

    if (mTracks.back()->IsPicture()) {
        BMX_CHECK(!mPictureTrack);
        mPictureTrack = dynamic_cast<D10MPEGTrack*>(mTracks.back());
    } else if (!mFirstSoundTrack) {
        mFirstSoundTrack = dynamic_cast<D10PCMTrack*>(mTracks.back());
    }

    return mTracks.back();
}

D10XMLTrack* D10File::CreateXMLTrack()
{
    uint32_t track_id    = mTrackIdHelper.GetNextId(XML_TRACK_TYPE);
    uint32_t track_index = (uint32_t)mTracks.size();
    uint32_t stream_id   = mStreamIdHelper.GetNextId(GENERIC_STREAM_TYPE);

    mXMLTracks.push_back(new D10XMLTrack(this, track_index, track_id, stream_id));
    return mXMLTracks.back();
}

void D10File::PrepareHeaderMetadata()
{
    if (mHeaderMetadata)
        return;

    BMX_CHECK(mPictureTrack);
    BMX_CHECK(!(mFlavour & D10_SINGLE_PASS_WRITE_FLAVOUR) || mInputDuration >= 0);

    mEssenceContainerUL = get_essence_container_ul(mPictureTrack->GetEssenceType(), mFrameRate);

    uint32_t last_sound_track_number = 0;
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (!mTracks[i]->IsPicture()) {
            if (!mTracks[i]->IsOutputTrackNumberSet())
                mTracks[i]->SetOutputTrackNumber(last_sound_track_number + 1);
            last_sound_track_number = mTracks[i]->GetOutputTrackNumber();
        }
    }

    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->PrepareWrite();

    mCPManager->SetEssenceContainerUL(mEssenceContainerUL);
    mCPManager->SetStartTimecode(mStartTimecode);
    mCPManager->PrepareWrite();

    CreateHeaderMetadata();
}

void D10File::PrepareWrite()
{
    mReserveMinBytes += 256; // account for extra bytes when updating header metadata

    if (!mHeaderMetadata)
        PrepareHeaderMetadata();

    CreateFile();
}

void D10File::WriteUserTimecode(Timecode user_timecode)
{
    mCPManager->WriteUserTimecode(user_timecode);

    while (mCPManager->HaveContentPackage())
        mCPManager->WriteNextContentPackage(mMXFFile);
}

void D10File::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (!data || size == 0)
        return;
    BMX_CHECK(data && size && num_samples);

    GetTrack(track_index)->WriteSamplesInt(data, size, num_samples);

    while (mCPManager->HaveContentPackage()) {
        if (mFirstWrite && mRequireBodyPartition) {
            if (mInputDuration >= 0)
                BMX_EXCEPTION(("XML track's Generic Stream partition is currently incompatible with single pass flavours"));

            Partition &ess_partition = mMXFFile->createPartition();
            ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
            ess_partition.setBodySID(mStreamIdHelper.GetId("BodyStream"));
            ess_partition.write(mMXFFile);

            mFirstWrite = false;
        }

        mCPManager->WriteNextContentPackage(mMXFFile);
    }
}

void D10File::CompleteWrite()
{
    BMX_ASSERT(mMXFFile);

    // write remaining content packages if duration < sound sample sequence size

    mCPManager->FinalWrite(mMXFFile);

    if (mInputDuration >= 0) {
        BMX_CHECK_M(mCPManager->GetDuration() == mInputDuration,
                    ("Single pass write failed because D10 output duration %" PRId64 " "
                     "does not equal set input duration %" PRId64,
                     mCPManager->GetDuration(), mInputDuration));

        BMX_CHECK_M(mMXFFile->tell() == (int64_t)mMXFFile->getPartition(0).getFooterPartition(),
                    ("Single pass write failed because footer partition offset %" PRId64 " "
                     "is not at expected offset %" PRId64,
                     mMXFFile->tell(), mMXFFile->getPartition(0).getFooterPartition()));
    }


    // write the footer partition pack

    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer));
    footer_partition.setIndexSID(0);
    footer_partition.setBodySID(0);
    footer_partition.write(mMXFFile);


    // write the RIP

    mMXFFile->writeRIP();


    if (mInputDuration < 0) {
        // update metadata sets with duration

        UpdatePackageMetadata();


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


        // update and re-write the index table segment

        mIndexSegment->setIndexDuration(mForceWriteCBEDuration0 ? 0 : GetDuration());
        mIndexSegment->write(mMXFFile, &header_partition, 0);


        // update partition pack and flush memory writes to file

        mMXFFile->updatePartitions();
        mMXFFile->closeMemoryFile();


        // re-write the generic stream partition packs

        mMXFFile->updateGenericStreamPartitions();


        // update body partition status and re-write the partition packs

        if (mRequireBodyPartition)
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

D10Track* D10File::GetTrack(uint32_t track_index)
{
    BMX_ASSERT(track_index < mTracks.size());
    return mTrackMap[track_index];
}

void D10File::CreateHeaderMetadata()
{
    BMX_ASSERT(!mHeaderMetadata);


    // create the header metadata
    mDataModel = new DataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);


    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(mCreationDate);
    preface->setVersion(MXF_PREFACE_VER(1, 2));
    preface->setOperationalPattern(MXF_OP_L(1a, MultiTrack_Stream_Internal));
    preface->appendEssenceContainers(mEssenceContainerUL);
    if (mXMLTracks.empty())
        preface->setDMSchemes(vector<mxfUL>());
    else
        preface->appendDMSchemes(MXF_DM_L(RP2057));

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
    ess_container_data->setIndexSID(mStreamIdHelper.GetId("IndexStream"));
    ess_container_data->setBodySID(mStreamIdHelper.GetId("BodyStream"));

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
    timecode_track->setTrackID(mTrackIdHelper.GetId("TimecodeTrack"));
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(mFrameRate);
    timecode_track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timecode Track - Sequence
    Sequence *sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(mInputDuration >= 0 ? mInputDuration : -1);

    // Preface - ContentStorage - MaterialPackage - Timecode Track - TimecodeComponent
    TimecodeComponent *timecode_component = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(timecode_component);
    timecode_component->setDataDefinition(MXF_DDEF_L(Timecode));
    timecode_component->setDuration(mInputDuration >= 0 ? mInputDuration : -1);
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
    timecode_track->setTrackID(1);
    timecode_track->setTrackNumber(0);
    timecode_track->setEditRate(mFrameRate);
    timecode_track->setOrigin(0);

    // Preface - ContentStorage - SourcePackage - Timecode Track - Sequence
    sequence = new Sequence(mHeaderMetadata);
    timecode_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(mInputDuration >= 0 ? mInputDuration : -1);

    // Preface - ContentStorage - SourcePackage - Timecode Track - TimecodeComponent
    timecode_component = new TimecodeComponent(mHeaderMetadata);
    sequence->appendStructuralComponents(timecode_component);
    timecode_component->setDataDefinition(MXF_DDEF_L(Timecode));
    timecode_component->setDuration(mInputDuration >= 0 ? mInputDuration : -1);
    timecode_component->setRoundedTimecodeBase(mStartTimecode.GetRoundedTCBase());
    timecode_component->setDropFrame(mStartTimecode.IsDropFrame());
    timecode_component->setStartTimecode(mStartTimecode.GetOffset());

    // Preface - ContentStorage - MaterialPackage/SourcePackage - Timeline Tracks
    // picture track followed by sound track
    uint32_t i;
    for (i = 0; i < 2; i++)
    {
        bool is_picture = (i == 0);
        mxfUL data_def = (is_picture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        string track_name = get_track_name((is_picture ? MXF_PICTURE_DDEF : MXF_SOUND_DDEF), (is_picture ? 1 : i));
        uint32_t track_id = (is_picture ? mTrackIdHelper.GetId("PictureTrack") : mTrackIdHelper.GetId("SoundTrack"));

        // Preface - ContentStorage - MaterialPackage - Timeline Track
        Track *track = new Track(mHeaderMetadata);
        mMaterialPackage->appendTracks(track);
        track->setTrackName(track_name);
        track->setTrackID(track_id);
        track->setTrackNumber(0);
        track->setEditRate(mFrameRate);
        track->setOrigin(0);

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
        Sequence *sequence = new Sequence(mHeaderMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(data_def);
        sequence->setDuration(mInputDuration >= 0 ? mInputDuration : -1);

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
        SourceClip *source_clip = new SourceClip(mHeaderMetadata);
        sequence->appendStructuralComponents(source_clip);
        source_clip->setDataDefinition(data_def);
        source_clip->setDuration(mInputDuration >= 0 ? mInputDuration : -1);
        source_clip->setStartPosition(0);
        source_clip->setSourcePackageID(mFileSourcePackageUID);
        source_clip->setSourceTrackID(i + 2);

        // Preface - ContentStorage - SourcePackage - Timeline Track
        track = new Track(mHeaderMetadata);
        mFileSourcePackage->appendTracks(track);
        track->setTrackName(track_name);
        track->setTrackID(track_id);
        track->setTrackNumber(is_picture ? MXF_D10_PICTURE_TRACK_NUM(0x00) : MXF_D10_SOUND_TRACK_NUM(0x00));
        track->setEditRate(mFrameRate);
        track->setOrigin(0);

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
        sequence = new Sequence(mHeaderMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(data_def);
        sequence->setDuration(mInputDuration >= 0 ? mInputDuration : -1);

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
        source_clip = new SourceClip(mHeaderMetadata);
        sequence->appendStructuralComponents(source_clip);
        source_clip->setDataDefinition(data_def);
        source_clip->setDuration(mInputDuration >= 0 ? mInputDuration : -1);
        source_clip->setStartPosition(0);
        source_clip->setSourceTrackID(0);
        source_clip->setSourcePackageID(g_Null_UMID);
    }

    // Preface - ContentStorage - SourcePackage - MultipleDescriptor
    MultipleDescriptor *mult_descriptor = new MultipleDescriptor(mHeaderMetadata);
    mFileSourcePackage->setDescriptor(mult_descriptor);
    mult_descriptor->setSampleRate(mFrameRate);
    mult_descriptor->setEssenceContainer(mEssenceContainerUL);
    if (mInputDuration >= 0)
        mult_descriptor->setContainerDuration(mInputDuration);

    // Preface - ContentStorage - SourcePackage - MultipleDescriptor - CDCIEssenceDescriptor
    CDCIEssenceDescriptor *cdci_descriptor =
        dynamic_cast<CDCIEssenceDescriptor*>(mPictureTrack->CreateFileDescriptor(mHeaderMetadata));
    mult_descriptor->appendSubDescriptorUIDs(cdci_descriptor);
    cdci_descriptor->setLinkedTrackID(mTrackIdHelper.GetId("PictureTrack"));
    cdci_descriptor->setEssenceContainer(mEssenceContainerUL);
    if (mInputDuration >= 0)
        cdci_descriptor->setContainerDuration(mInputDuration);

    // Preface - ContentStorage - SourcePackage - MultipleDescriptor - GenericSoundEssenceDescriptor
    GenericSoundEssenceDescriptor *sound_descriptor = new GenericSoundEssenceDescriptor(mHeaderMetadata);
    mult_descriptor->appendSubDescriptorUIDs(sound_descriptor);
    sound_descriptor->setLinkedTrackID(mTrackIdHelper.GetId("SoundTrack"));
    sound_descriptor->setSampleRate(mFrameRate);
    sound_descriptor->setEssenceContainer(mEssenceContainerUL);
    if (mInputDuration >= 0)
        sound_descriptor->setContainerDuration(mInputDuration);
    if (mFirstSoundTrack) {
        sound_descriptor->setAudioSamplingRate(mFirstSoundTrack->GetSamplingRate());
        sound_descriptor->setChannelCount(mCPManager->GetSoundChannelCount());
        sound_descriptor->setQuantizationBits(mFirstSoundTrack->GetQuantizationBits());
        if (mFirstSoundTrack->HaveSetLocked())
            sound_descriptor->setLocked(mFirstSoundTrack->GetLocked());
        if (mFirstSoundTrack->HaveSetAudioRefLevel())
            sound_descriptor->setAudioRefLevel(mFirstSoundTrack->GetAudioRefLevel());
    } else {
        sound_descriptor->setAudioSamplingRate(AUDIO_SAMPLING_RATE);
        sound_descriptor->setChannelCount(mCPManager->GetSoundChannelCount());
        sound_descriptor->setQuantizationBits(16);
    }

    for (i = 0; i < mXMLTracks.size(); i++)
        mXMLTracks[i]->AddHeaderMetadata(mHeaderMetadata, mMaterialPackage);
}

void D10File::CreateFile()
{
    BMX_ASSERT(mHeaderMetadata);

    // check whether a body partition is required

    mRequireBodyPartition = false;
    size_t i;
    for (i = 0; i < mXMLTracks.size(); i++) {
        D10XMLTrack *xml_track = mXMLTracks[i];
        if (xml_track->RequireStreamPartition()) {
            if (mInputDuration >= 0)
                BMX_EXCEPTION(("XML track's Generic Stream partition is currently incompatible with single pass flavours"));
            mRequireBodyPartition = true;
            break;
        }
    }


    // set minimum llen

    mMXFFile->setMinLLen(4);


    // write to memory

    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);


    // write the header metadata

    Partition &header_partition = mMXFFile->createPartition();
    if (mInputDuration >= 0)
        header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
    else
        header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setVersion(1, 2);  // v1.2 - smpte 377-2004
    header_partition.setIndexSID(mStreamIdHelper.GetId("IndexStream"));
    if (mRequireBodyPartition)
        header_partition.setBodySID(0);
    else
        header_partition.setBodySID(mStreamIdHelper.GetId("BodyStream"));
    header_partition.setKagSize(KAG_SIZE);
    header_partition.setOperationalPattern(&MXF_OP_L(1a, MultiTrack_Stream_Internal));
    header_partition.addEssenceContainer(&mEssenceContainerUL);
    header_partition.write(mMXFFile);

    KAGFillerWriter reserve_filler_writer(&header_partition, mReserveMinBytes);
    mHeaderMetadata->write(mMXFFile, &header_partition, &reserve_filler_writer);
    mHeaderMetadataEndPos = mMXFFile->tell();  // need this position when we re-write the header metadata


    // write cbe index table

    mxfUUID uuid;
    mxf_generate_uuid(&uuid);

    mIndexSegment = new IndexTableSegment();
    mIndexSegment->setInstanceUID(uuid);
    mIndexSegment->setIndexEditRate(mFrameRate);
    mIndexSegment->setIndexDuration(mInputDuration >= 0 ? mInputDuration : 0);
    mIndexSegment->setIndexSID(mStreamIdHelper.GetId("IndexStream"));
    mIndexSegment->setBodySID(mStreamIdHelper.GetId("BodyStream"));

    const std::vector<uint32_t> &ext_delta_entries = mCPManager->GetExtDeltaEntryArray();
    for (i = 0; i < ext_delta_entries.size(); i++)
        mIndexSegment->appendDeltaEntry(0, 0, ext_delta_entries[i]);
    mIndexSegment->setEditUnitByteCount(mCPManager->GetContentPackageSize());

    mIndexSegment->write(mMXFFile, &mMXFFile->getPartition(0), 0);


    // update partition pack and flush memory writes to file

    if (mInputDuration >= 0)
        header_partition.setFooterPartition(mMXFFile->tell() + mInputDuration * mCPManager->GetContentPackageSize());

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();


    // write generic stream partitions

    for (i = 0; i < mXMLTracks.size(); i++) {
        D10XMLTrack *xml_track = mXMLTracks[i];
        if (xml_track->RequireStreamPartition()) {
            if (mInputDuration >= 0)
                BMX_EXCEPTION(("XML track's Generic Stream partition is currently incompatible with single pass flavours"));

            Partition &stream_partition = mMXFFile->createPartition();
            stream_partition.setKey(&MXF_GS_PP_K(GenericStream));
            stream_partition.setBodySID(xml_track->GetStreamId());
            stream_partition.write(mMXFFile);
            xml_track->WriteStreamXMLData(mMXFFile);
            stream_partition.fillToKag(mMXFFile);
        }
    }
}

void D10File::UpdatePackageMetadata()
{
    int64_t output_duration = GetDuration();

    UpdateTrackMetadata(mMaterialPackage, output_duration);
    UpdateTrackMetadata(mFileSourcePackage, output_duration);


    BMX_ASSERT(mFileSourcePackage->haveDescriptor());
    FileDescriptor *file_descriptor = dynamic_cast<FileDescriptor*>(mFileSourcePackage->getDescriptor());
    if (file_descriptor)
        file_descriptor->setContainerDuration(output_duration);

    MultipleDescriptor *mult_descriptor = dynamic_cast<MultipleDescriptor*>(file_descriptor);
    if (mult_descriptor) {
        vector<GenericDescriptor*> sub_descriptors = mult_descriptor->getSubDescriptorUIDs();
        size_t i;
        for (i = 0; i < sub_descriptors.size(); i++) {
            file_descriptor = dynamic_cast<FileDescriptor*>(sub_descriptors[i]);
            if (file_descriptor)
                file_descriptor->setContainerDuration(output_duration);
        }
    }
}

void D10File::UpdateTrackMetadata(GenericPackage *package, int64_t duration)
{
    vector<GenericTrack*> tracks = package->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *track = dynamic_cast<Track*>(tracks[i]);
        if (!track)
            continue;

        Sequence *sequence = dynamic_cast<Sequence*>(track->getSequence());
        BMX_ASSERT(sequence);
        if (sequence->getDuration() < 0) {
            sequence->setDuration(duration);

            vector<StructuralComponent*> components = sequence->getStructuralComponents();
            BMX_CHECK(components.size() == 1);
            components[0]->setDuration(duration);
        }
    }
}

