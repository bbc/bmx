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
#include <bmx/mxf_op1a/OP1ATimedTextTrack.h>
#include <bmx/mxf_op1a/OP1ARDD36Track.h>
#include <bmx/mxf_helper/MXFDescriptorHelper.h>
#include <bmx/mxf_helper/MXFMCALabelHelper.h>
#include <bmx/wave/WaveChunk.h>
#include <bmx/BMXMXFIO.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Version.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;

// some large number that is not expected to be exceeded and avoids memory allocation exceptions
// maximum buffer requirement is expected to be the coding delay / maximum temporal offset
#define MAX_BUFFERED_CONTENT_PACKAGES   200

#define HAVE_PRIMARY_EC   (mIndexTable != 0)

static const char TIMECODE_TRACK_NAME[]         = "TC1";
static const uint8_t MIN_LLEN                   = 4;
static const uint32_t MEMORY_WRITE_CHUNK_SIZE   = 8192;



static bool compare_track(OP1ATrack *left, OP1ATrack *right)
{
    return left->GetDataDef() < right->GetDataDef();
}



mxfUMID OP1AFile::CreatePackageUID()
{
    mxfUMID package_uid;
    mxf_generate_umid(&package_uid);
    return package_uid;
}

OP1AFile::OP1AFile(int flavour, mxfpp::File *mxf_file, mxfRational frame_rate)
{
    mFlavour = flavour;
    mMXFFile = mxf_file;
    mFrameRate = frame_rate;
    mEditRate = frame_rate;
    mStartTimecode = Timecode(frame_rate, false);
    mAddTimecodeTrack = true;
    mCompanyName = get_bmx_company_name();
    mProductName = get_bmx_library_name();
    mProductVersion = get_bmx_mxf_product_version();
    mVersionString = get_bmx_mxf_version_string();
    mProductUID = get_bmx_product_uid();
    mReserveMinBytes = 8192;
    mInputDuration = -1;
    mxf_get_timestamp_now(&mCreationDate);
    mxf_generate_uuid(&mGenerationUID);
    mMaterialPackageUID = OP1AFile::CreatePackageUID();
    mFileSourcePackageUID = OP1AFile::CreatePackageUID();
    mFrameWrapped = true;
    mOPLabel = MXF_OP_L(1a, UniTrack_Stream_Internal);
    mOutputStartOffset = 0;
    mOutputEndOffset = 0;
    mHaveANCTrack = false;
    mHaveVBITrack = false;
    mTimedTextTrackCount = 0;
    mDataModel = 0;
    mHeaderMetadata = 0;
    mHavePreparedHeaderMetadata = false;
    mHeaderMetadataEndPos = 0;
    mMaterialPackage = 0;
    mFileSourcePackage = 0;
    mFirstWrite = true;
    mWaitForIndexComplete = false;
    mPartitionInterval = 0;
    mPartitionFrameCount = 0;
    mKAGSize = ((flavour & OP1A_512_KAG_FLAVOUR) || (flavour & OP1A_ARD_ZDF_XDF_PROFILE_FLAVOUR) ? 512 : 1);
    mEssencePartitionKAGSize = mKAGSize;
    mSupportCompleteSinglePass = false;
    mFooterPartitionOffset = 0;
    mMXFChecksumFile = 0;
    mCBEIndexPartitionIndex = 0;
    mSetPrimaryPackage = false;
    mIndexFollowsEssence = false;
    mSignalST3792 = false;

    mTrackIdHelper.SetId("TimecodeTrack", 901);
    mTrackIdHelper.SetStartId(MXF_PICTURE_DDEF, 1001);
    mTrackIdHelper.SetStartId(MXF_SOUND_DDEF,   2001);
    mTrackIdHelper.SetStartId(MXF_DATA_DDEF,    3001);
    mTrackIdHelper.SetStartId(XML_TRACK_TYPE,   9001);

    mStreamIdHelper.SetId("IndexStream", 1);
    mStreamIdHelper.SetId("BodyStream",  2);
    mStreamIdHelper.SetStartId(STREAM_TYPE, 10);

    mDataModel = new DataModel();
    mHeaderMetadata = new HeaderMetadata(mDataModel);

    mIndexTable = new OP1AIndexTable(mStreamIdHelper.GetId("IndexStream"), mStreamIdHelper.GetId("BodyStream"), frame_rate,
                                     (flavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR) || (flavour & OP1A_ARD_ZDF_XDF_PROFILE_FLAVOUR));
    mCPManager = new OP1AContentPackageManager(mMXFFile, mIndexTable, frame_rate, mEssencePartitionKAGSize, MIN_LLEN);

    if (flavour & OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR) {
        mMXFChecksumFile = mxf_checksum_file_open(mMXFFile->getCFile(), MD5_CHECKSUM);
        mMXFFile->swapCFile(mxf_checksum_file_get_file(mMXFChecksumFile));
    }

    if ((flavour & OP1A_ARD_ZDF_XDF_PROFILE_FLAVOUR)) {
        mFlavour |= OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR;
        mFlavour |= OP1A_BODY_PARTITIONS_FLAVOUR;
        SetAddSystemItem(true);
        SetFieldMark(true);
        SetSignalST3792(true);

        // Some open GOP XAVC files are missing the two leading B-frames in the first GOP.
        // We therefore subtract 2 frames to ensure that the index duration is not exceeded.
        SetPartitionInterval(60 * (int64_t)(0.167 * frame_rate.numerator / frame_rate.denominator) - 2);
    } else if ((flavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR)) {
        mFlavour |= OP1A_BODY_PARTITIONS_FLAVOUR;
        ReserveHeaderMetadataSpace(2 * 1024 * 1024 + 8192);
        SetAddSystemItem(true);
    } else if ((flavour & OP1A_AS11_FLAVOUR)) {
        ReserveHeaderMetadataSpace(4 * 1024 * 1024 + 8192);
    } else if ((flavour & OP1A_IMF_FLAVOUR)) {
        mReserveMinBytes = mReserveMinBytes < 8192 ? 8192: mReserveMinBytes;
        SetPrimaryPackage(true);
        SetAddTimecodeTrack(false);
        SetIndexFollowsEssence(true);
        SetPartitionInterval((int64_t)(60 * frame_rate.numerator / frame_rate.denominator));
    } else if ((flavour & OP1A_SYSTEM_ITEM_FLAVOUR)) {
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
    for (i = 0; i < mXMLTracks.size(); i++)
        delete mXMLTracks[i];
    for (i = 0; i < mOwnedWaveChunks.size(); i++)
        delete mOwnedWaveChunks[i];

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

void OP1AFile::SetAddTimecodeTrack(bool enable)
{
    mAddTimecodeTrack = enable;
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

void OP1AFile::SetFieldMark(bool enable)
{
    mCPManager->SetFieldMark(enable);
}

void OP1AFile::SetRepeatIndexTable(bool enable)
{
    mIndexTable->SetRepeatIndexTable(enable);
}

void OP1AFile::ForceWriteCBEDuration0(bool enable)
{
    mIndexTable->ForceWriteCBEDuration0(enable);
}

void OP1AFile::SetPrimaryPackage(bool enable)
{
    mSetPrimaryPackage = enable;
}

void OP1AFile::SetIndexFollowsEssence(bool enable)
{
    mIndexFollowsEssence = enable;
}

void OP1AFile::SetSignalST3792(bool enable)
{
    mSignalST3792 = enable;
}

uint32_t OP1AFile::AddWaveChunk(WaveChunk *chunk, bool take_ownership)
{
    uint32_t stream_id = mStreamIdHelper.GetNextId(STREAM_TYPE);
    mWaveChunks[stream_id] = chunk;

    if (take_ownership)
        mOwnedWaveChunks.push_back(chunk);

    return stream_id;
}

uint32_t OP1AFile::AddADMWaveChunk(WaveChunk *chunk, bool take_ownership, const vector<UL> &profile_and_level_uls)
{
    uint32_t stream_id = AddWaveChunk(chunk, take_ownership);

    ADMWaveChunkInfo info;
    info.profile_and_level_uls = profile_and_level_uls;
    mADMWaveChunkInfo[stream_id] = info;

    return stream_id;
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
    if (essence_type == TIMED_TEXT) {
        mTimedTextTrackCount++;
    }

    MXFDataDefEnum data_def = convert_essence_type_to_data_def(essence_type);
    uint32_t track_id = mTrackIdHelper.GetNextId(data_def);
    uint8_t track_type_number = mTrackCounts[data_def];
    mTrackCounts[data_def]++;

    BMX_CHECK_M(mFrameWrapped || (mTracks.empty() && data_def == MXF_SOUND_DDEF && mTrackCounts[data_def] == 1),
                ("OP-1A clip wrapping only supported for a single sound track"));

    mTracks.push_back(OP1ATrack::Create(this, (uint32_t)mTracks.size(), track_id, track_type_number, mEditRate,
                                        essence_type));
    mTrackMap[mTracks.back()->GetTrackIndex()] = mTracks.back();

    if (essence_type != TIMED_TEXT && !mFrameWrapped) {
        mEditRate = mTracks.back()->GetEditRate();
        mIndexTable->SetEditRate(mEditRate);
    }

    return mTracks.back();
}

OP1AXMLTrack* OP1AFile::CreateXMLTrack()
{
    uint32_t track_id    = mTrackIdHelper.GetNextId(XML_TRACK_TYPE);
    uint32_t track_index = (uint32_t)mTracks.size();
    uint32_t stream_id   = mStreamIdHelper.GetNextId(STREAM_TYPE);

    mXMLTracks.push_back(new OP1AXMLTrack(this, track_index, track_id, stream_id));
    return mXMLTracks.back();
}

void OP1AFile::PrepareHeaderMetadata()
{
    if (mHavePreparedHeaderMetadata)
        return;

    BMX_CHECK(!mTracks.empty());

    // if the file only contains timed text tracks then delete the common
    // mIndexTable and mCPManager as they will not be used
    // HAVE_PRIMARY_EC will now be false
    if (mTimedTextTrackCount >= mTracks.size()) {
        delete mIndexTable;
        mIndexTable = 0;
        delete mCPManager;
        mCPManager = 0;
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

    if (HAVE_PRIMARY_EC &&
        (mFlavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR))
    {
        if (mIndexTable->IsCBE() && !(mFlavour & OP1A_ARD_ZDF_XDF_PROFILE_FLAVOUR))
            mIndexTable->SetExtensions(MXF_OPT_BOOL_TRUE, MXF_OPT_BOOL_TRUE, MXF_OPT_BOOL_TRUE);
        else
            mIndexTable->SetExtensions(MXF_OPT_BOOL_FALSE, MXF_OPT_BOOL_FALSE, MXF_OPT_BOOL_FALSE);
    }

    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->PrepareWrite(mTrackCounts[mTracks[i]->GetDataDef()]);
    if (HAVE_PRIMARY_EC) {
        Timecode start_timecode = mStartTimecode;
        start_timecode.AddOffset(- mOutputStartOffset, mFrameRate);
        mCPManager->SetStartTimecode(start_timecode);
        mCPManager->PrepareWrite();
        mIndexTable->PrepareWrite();
    }

    if (mTracks.size() - mTimedTextTrackCount > 1)
        mEssenceContainerULs.insert(MXF_EC_L(MultipleWrappings));
    for (i = 0; i < mTracks.size(); i++)
        mEssenceContainerULs.insert(mTracks[i]->GetEssenceContainerUL());

    if (HAVE_PRIMARY_EC &&
        (mFlavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR) && mInputDuration >= 0)
    {
        if (mPartitionInterval == 0 &&
                mIndexTable->IsCBE() &&
                !mIndexFollowsEssence &&
                mTimedTextTrackCount == 0)
        {
            mSupportCompleteSinglePass = true;
            mIndexTable->SetInputDuration(mInputDuration);
        }
        else
        {
            if (mIndexTable && !mIndexTable->IsCBE()) {
                log_warn("Closing and completing the header partition in a single pass write is not supported "
                         "because essence edit unit size is variable\n");
            }
            if (mPartitionInterval != 0) {
                log_warn("Closing and completing the header partition in a single pass write is not supported "
                         "because partition interval is non-zero\n");
            }
            if (mIndexTable->IsCBE() && mIndexFollowsEssence) {
                log_warn("Index that follows essence in a single pass write is not supported\n");
            }
        }
    } else if (!HAVE_PRIMARY_EC && mTimedTextTrackCount > 0) {
        if (mInputDuration < 0) {
            BMX_EXCEPTION(("Duration needs to be set for a file containing timed text tracks only"));
        }
        size_t i;
        for (i = 0; i < mTracks.size(); i++) {
            OP1ATimedTextTrack *tt_track = dynamic_cast<OP1ATimedTextTrack*>(mTracks[i]);
            if (tt_track) {
                tt_track->SetDuration(mInputDuration);
            }
        }
    }

    CreateHeaderMetadata();

    mHavePreparedHeaderMetadata = true;
}

void OP1AFile::PrepareWrite()
{
    mReserveMinBytes += 8192; // account for extra bytes when updating header metadata

    if (!mHavePreparedHeaderMetadata)
        PrepareHeaderMetadata();

    CreateFile();
}

void OP1AFile::WriteUserTimecode(Timecode user_timecode)
{
    if (HAVE_PRIMARY_EC) {
        mCPManager->WriteUserTimecode(user_timecode);
        WriteContentPackages(false);
    }
}

void OP1AFile::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (!data || size == 0)
        return;
    BMX_CHECK(data && size && num_samples);

    GetTrack(track_index)->WriteSamplesInt(data, size, num_samples);

    WriteContentPackages(false);
}

void OP1AFile::UpdateHeaderMetadata()
{
    BMX_ASSERT(mMXFFile);

    int64_t file_pos = mMXFFile->tell();

    // re-write header partition to memory first

    mMXFFile->seek(0, SEEK_SET);
    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);
    mMXFFile->setMemoryPartitionIndexes(0, 0); // overwriting and updating from header pp

    // re-write the header partition pack

    Partition &header_partition = mMXFFile->getPartition(0);
    header_partition.write(mMXFFile);

    // re-write the header metadata

    PositionFillerWriter pos_filler_writer(mHeaderMetadataEndPos);
    mHeaderMetadata->write(mMXFFile, &header_partition, &pos_filler_writer);

    // update header and flush memory writes to file

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();

    mMXFFile->seek(file_pos, SEEK_SET);
}

void OP1AFile::CompleteWrite()
{
    BMX_ASSERT(mMXFFile);


    // complete metadata for tracks

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        mTracks[i]->CompleteWrite();


    if (HAVE_PRIMARY_EC) {
        // warn if the index table requires updates (e.g. temporal offsets)

        if (mIndexTable->RequireUpdatesAtEnd(mOutputEndOffset)) {
            log_warn("Ignoring incomplete index table information\n");
        }
        mIndexTable->IgnoreRequiredUpdates();


        // write any remaining content packages (e.g. first AVCI cp if duration equals 1)

        WriteContentPackages(true);


        // check that the duration is valid

        BMX_CHECK_M(mIndexTable->GetDuration() - mOutputStartOffset + mOutputEndOffset >= 0,
                    ("Invalid output start %" PRId64 " / end %" PRId64 " offsets. Output duration %" PRId64 " is negative",
                     mOutputStartOffset, mOutputEndOffset, mIndexTable->GetDuration() - mOutputStartOffset + mOutputEndOffset));

        if (mSupportCompleteSinglePass) {
            BMX_CHECK_M(mIndexTable->GetDuration() == mInputDuration,
                        ("Single pass write failed because OP-1A container duration %" PRId64 " "
                         "does not equal set input duration %" PRId64,
                         mIndexTable->GetDuration(), mInputDuration));
        }
    }


    // update metadata sets with duration

    if (!mSupportCompleteSinglePass)
        UpdatePackageMetadata();


    // first write and complete last part to memory

    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);


    // write any remaining VBE index segments or follow CBE index segments

    if (HAVE_PRIMARY_EC &&
        !(mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) &&
        (!(mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_ARD_ZDF_XDF_PROFILE_FLAVOUR) ||
            (mIndexTable->RepeatIndexTable() && mIndexTable->HaveWritten())) &&
        ((mIndexTable->IsVBE() && mIndexTable->HaveSegments()) ||
            (mIndexTable->IsCBE() && !mIndexTable->HaveWritten() && mIndexTable->GetDuration() > 0)))
    {
        Partition &index_partition = mMXFFile->createPartition();
        index_partition.setKey(&MXF_PP_K(ClosedComplete, Body)); // will be complete when memory flushed
        index_partition.setIndexSID(mStreamIdHelper.GetId("IndexStream"));
        index_partition.setBodySID(0);
        index_partition.setKagSize(mKAGSize);
        index_partition.write(mMXFFile);

        // CBE index table is written at the end for the index follows essence case
        if (mIndexTable->IsCBE())
            mCBEIndexPartitionIndex = mMXFFile->getPartitions().size() - 1;

        mIndexTable->WriteSegments(mMXFFile, &index_partition, true);
    }


    // write the footer partition pack

    if (mSupportCompleteSinglePass) {
        BMX_CHECK_M(mMXFFile->tell() == (int64_t)mMXFFile->getPartition(0).getFooterPartition(),
                    ("Single pass write failed because footer partition offset %" PRId64 " "
                     "is not at expected offset %" PRId64,
                     mMXFFile->tell(), mMXFFile->getPartition(0).getFooterPartition()));
    }

    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer)); // will be complete when memory flushed
    if (HAVE_PRIMARY_EC && mIndexTable->HaveFooterSegments())
        footer_partition.setIndexSID(mStreamIdHelper.GetId("IndexStream"));
    else
        footer_partition.setIndexSID(0);
    footer_partition.setBodySID(0);
    footer_partition.setKagSize(mKAGSize);
    footer_partition.write(mMXFFile);


    // write (complete) header metadata if it is a single pass write and the header partition is incomplete

    if ((mFlavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR) && !mSupportCompleteSinglePass) {
        KAGFillerWriter reserve_filler_writer(&footer_partition, mReserveMinBytes);
        mHeaderMetadata->write(mMXFFile, &footer_partition, &reserve_filler_writer);
    }


    // minimal/body partitions flavour: write any remaining index segments

    if (HAVE_PRIMARY_EC && mIndexTable->HaveFooterSegments()) {
        if (mIndexTable->IsCBE() && !mIndexTable->HaveWritten()) {
            // CBE index table that is not a repeat
            mCBEIndexPartitionIndex = mMXFFile->getPartitions().size() - 1;
        }

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


        // re-write the CBE index table segment(s) in the header partition

        if (HAVE_PRIMARY_EC &&
            !mFirstWrite && mIndexTable->IsCBE() && mCBEIndexPartitionIndex == 0)
        {
            Partition &index_partition = mMXFFile->getPartition(mCBEIndexPartitionIndex);
            mIndexTable->WriteSegments(mMXFFile, &index_partition, true);
        }


        // update header and index partition packs and flush memory writes to file

        mMXFFile->updatePartitions();
        mMXFFile->closeMemoryFile();


        // update and re-write the generic stream partition packs

        mMXFFile->updateGenericStreamPartitions();


        // re-write the CBE index table segment(s) that are not in the header or footer partition

        if (HAVE_PRIMARY_EC &&
                !mFirstWrite &&
                mIndexTable->IsCBE() &&
                mCBEIndexPartitionIndex > 0 &&
                mCBEIndexPartitionIndex < mMXFFile->getPartitions().size() - 1)
        {
            Partition &index_partition = mMXFFile->getPartition(mCBEIndexPartitionIndex);
            mMXFFile->seek(index_partition.getThisPartition(), SEEK_SET);
            index_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
            index_partition.setFooterPartition(footer_partition.getThisPartition());
            index_partition.write(mMXFFile);
            mIndexTable->WriteSegments(mMXFFile, &index_partition, true);
        }


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
    if (HAVE_PRIMARY_EC)
        return mIndexTable->GetDuration();
    else
        return mInputDuration; // timed text tracks only file
}

OP1ATrack* OP1AFile::GetTrack(uint32_t track_index)
{
    BMX_ASSERT(track_index < mTracks.size());
    return mTrackMap[track_index];
}

uint32_t OP1AFile::CreateStreamId()
{
    return mStreamIdHelper.GetNextId(STREAM_TYPE);
}

uint32_t OP1AFile::GetWaveChunkStreamID(WaveChunkId chunk_id)
{
    uint32_t stream_id = (uint32_t)(-1);
    map<uint32_t, WaveChunk*>::const_iterator iter;
    for (iter = mWaveChunks.begin(); iter != mWaveChunks.end(); iter++) {
        if (iter->second->Id() == chunk_id) {
            if (stream_id != (uint32_t)(-1)) {
                BMX_EXCEPTION(("Multiple wave chunks with ID '%s' exist in OP1a output file", get_wave_chunk_id_str(chunk_id).c_str()));
            }
            stream_id = iter->first;
        }
    }

    if (stream_id == (uint32_t)(-1)) {
        BMX_EXCEPTION(("Wave chunk '%s' does not exist in OP1a output file", get_wave_chunk_id_str(chunk_id).c_str()));
    }

    return stream_id;
}

uint32_t OP1AFile::GetADMWaveChunkStreamID(WaveChunkId chunk_id)
{
    uint32_t stream_id = GetWaveChunkStreamID(chunk_id);

    if (mADMWaveChunkInfo.count(stream_id) == 0) {
        BMX_EXCEPTION(("Wave chunk '%s' is not identified as containing ADM audio metadata (using an ADMAudioMetadataSubDescriptor)", get_wave_chunk_id_str(chunk_id).c_str()));
    }

    return stream_id;
}

void OP1AFile::CreateHeaderMetadata()
{
    BMX_ASSERT(!mHavePreparedHeaderMetadata);

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
    if (mTracks.size() <= 1) {
        if (mTimedTextTrackCount > 0) {
            OP1ATimedTextTrack *tt_track = dynamic_cast<OP1ATimedTextTrack*>(mTracks[0]);
            if (tt_track->GetStart() > 0) {
                mOPLabel = MXF_OP_L(2a, UniTrack_Stream_Internal);
            } else {
                mOPLabel = MXF_OP_L(1a, UniTrack_Stream_Internal);
            }
        } else {
            mOPLabel = MXF_OP_L(1a, UniTrack_Stream_Internal);
        }
    } else {
        if (mTimedTextTrackCount > 0) {
            int64_t start = -1;
            int complexity = 1;
            size_t i;
            for (i = 0; i < mTracks.size() && complexity < 3; i++) {
                OP1ATimedTextTrack *tt_track = dynamic_cast<OP1ATimedTextTrack*>(mTracks[i]);
                if (tt_track) {
                    if (tt_track->GetStart() > 0) {
                        if (start < 0 || tt_track->GetStart() == start) {
                            complexity = 2;
                        } else {
                            complexity = 3;
                        }
                    } else if (start > 0) {
                        complexity = 3;
                    }
                    start = tt_track->GetStart();
                }
            }
            if (complexity == 1) {
                mOPLabel = MXF_OP_L(1b, MultiTrack_Stream_Internal);
            } else if (complexity == 2) {
                mOPLabel = MXF_OP_L(2b, MultiTrack_Stream_Internal);
            } else {
                mOPLabel = MXF_OP_L(3b, MultiTrack_Stream_Internal);
            }
        } else {
            mOPLabel = MXF_OP_L(1a, MultiTrack_Stream_Internal);
        }
    }

    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setLastModifiedDate(mCreationDate);
    preface->setVersion((mFlavour & OP1A_377_2004_FLAVOUR) ? MXF_PREFACE_VER(1, 2) : MXF_PREFACE_VER(1, 3));
    preface->setOperationalPattern(mOPLabel);
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        preface->appendEssenceContainers(*iter);
    if (mXMLTracks.empty())
        preface->setDMSchemes(vector<mxfUL>());
    else
        preface->appendDMSchemes(MXF_DM_L(RP2057));
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
    if (mTimedTextTrackCount < mTracks.size()) {
        EssenceContainerData *ess_container_data = new EssenceContainerData(mHeaderMetadata);
        content_storage->appendEssenceContainerData(ess_container_data);
        ess_container_data->setLinkedPackageUID(mFileSourcePackageUID);
        ess_container_data->setIndexSID(mStreamIdHelper.GetId("IndexStream"));
        ess_container_data->setBodySID(mStreamIdHelper.GetId("BodyStream"));
    }

    // Preface - ContentStorage - MaterialPackage
    mMaterialPackage = new MaterialPackage(mHeaderMetadata);
    content_storage->appendPackages(mMaterialPackage);
    mMaterialPackage->setPackageUID(mMaterialPackageUID);
    mMaterialPackage->setPackageCreationDate(mCreationDate);
    mMaterialPackage->setPackageModifiedDate(mCreationDate);
    if (!mClipName.empty())
        mMaterialPackage->setName(mClipName);

    if (mAddTimecodeTrack) {
        // Preface - ContentStorage - MaterialPackage - Timecode Track
        Track *timecode_track = new Track(mHeaderMetadata);
        mMaterialPackage->appendTracks(timecode_track);
        timecode_track->setTrackName(TIMECODE_TRACK_NAME);
        timecode_track->setTrackID(mTrackIdHelper.GetId("TimecodeTrack"));
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
    }

    if (mTimedTextTrackCount < mTracks.size()) {
        // Source Package for non-Timed Text content

        // Preface - ContentStorage - SourcePackage
        mFileSourcePackage = new SourcePackage(mHeaderMetadata);
        content_storage->appendPackages(mFileSourcePackage);
        mFileSourcePackage->setPackageUID(mFileSourcePackageUID);
        mFileSourcePackage->setPackageCreationDate(mCreationDate);
        mFileSourcePackage->setPackageModifiedDate(mCreationDate);
        if (mSetPrimaryPackage && mTimedTextTrackCount == 0) {
            // Only set Primary Package if there is a single file source package
            preface->setPrimaryPackage(mFileSourcePackage);
        }

        if (mAddTimecodeTrack) {
            // Preface - ContentStorage - SourcePackage - Timecode Track
            Track *timecode_track = new Track(mHeaderMetadata);
            mFileSourcePackage->appendTracks(timecode_track);
            timecode_track->setTrackName(TIMECODE_TRACK_NAME);
            timecode_track->setTrackID(901);
            timecode_track->setTrackNumber(0);
            timecode_track->setEditRate(mEditRate);
            timecode_track->setOrigin(source_track_origin);

            // Preface - ContentStorage - SourcePackage - Timecode Track - Sequence
            Sequence *sequence = new Sequence(mHeaderMetadata);
            timecode_track->setSequence(sequence);
            sequence->setDataDefinition(MXF_DDEF_L(Timecode));
            sequence->setDuration(source_track_duration);

            // Preface - ContentStorage - SourcePackage - Timecode Track - TimecodeComponent
            TimecodeComponent *timecode_component = new TimecodeComponent(mHeaderMetadata);
            sequence->appendStructuralComponents(timecode_component);
            timecode_component->setDataDefinition(MXF_DDEF_L(Timecode));
            timecode_component->setDuration(source_track_duration);
            Timecode sp_start_timecode = mStartTimecode;
            sp_start_timecode.AddOffset(- source_track_origin, mFrameRate);
            timecode_component->setRoundedTimecodeBase(sp_start_timecode.GetRoundedTCBase());
            timecode_component->setDropFrame(sp_start_timecode.IsDropFrame());
            timecode_component->setStartTimecode(sp_start_timecode.GetOffset());
        }

        // Preface - ContentStorage - SourcePackage - (Multiple) File Descriptor
        if (mTracks.size() - mTimedTextTrackCount > 1) {
            MultipleDescriptor *mult_descriptor = new MultipleDescriptor(mHeaderMetadata);
            mFileSourcePackage->setDescriptor(mult_descriptor);
            mult_descriptor->setSampleRate(mEditRate);
            mult_descriptor->setEssenceContainer(MXF_EC_L(MultipleWrappings));
            if (mSupportCompleteSinglePass)
                mult_descriptor->setContainerDuration(mInputDuration);
        }
    }

    // MaterialPackage and file SourcePackage Tracks and FileDescriptor
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        OP1ATimedTextTrack *tt_track = dynamic_cast<OP1ATimedTextTrack*>(mTracks[i]);
        if (tt_track) {
            UMID tt_package_uid;
            if (HAVE_PRIMARY_EC) {
                // multiple file source packages - create a new id
                mxf_generate_umid(&tt_package_uid);
            } else {
                // only a single file source package - use id which can be set by the user
                tt_package_uid = mFileSourcePackageUID;
            }

            SourcePackage *file_source_package = new SourcePackage(mHeaderMetadata);
            content_storage->appendPackages(file_source_package);
            file_source_package->setPackageUID(tt_package_uid);
            file_source_package->setPackageCreationDate(mCreationDate);
            file_source_package->setPackageModifiedDate(mCreationDate);
            if (mSetPrimaryPackage && !mFileSourcePackage) {
                // Only set Primary Package if there is a single file source package
                preface->setPrimaryPackage(file_source_package);
            }

            mTracks[i]->AddHeaderMetadata(mHeaderMetadata, mMaterialPackage, file_source_package);
        } else {
            mTracks[i]->AddHeaderMetadata(mHeaderMetadata, mMaterialPackage, mFileSourcePackage);
        }
    }
    for (i = 0; i < mXMLTracks.size(); i++)
        mXMLTracks[i]->AddHeaderMetadata(mHeaderMetadata, mMaterialPackage);

    // Preface - ContentStorage - SourcePackage - File Descriptor - Wave Chunk sub-descriptors
    if (mFileSourcePackage && mFileSourcePackage->getDescriptor()) {
        GenericDescriptor *descriptor = mFileSourcePackage->getDescriptor();
        map<uint32_t, WaveChunk*>::const_iterator iter;
        for (iter = mWaveChunks.begin(); iter != mWaveChunks.end(); iter++) {
            RIFFChunkDefinitionSubDescriptor *wave_chunk_subdesc = new RIFFChunkDefinitionSubDescriptor(mHeaderMetadata);
            descriptor->appendSubDescriptors(wave_chunk_subdesc);
            wave_chunk_subdesc->setRIFFChunkStreamID(iter->first);
            wave_chunk_subdesc->setRIFFChunkID(iter->second->Id());

            if (mADMWaveChunkInfo.count(iter->first)) {
                const ADMWaveChunkInfo &info = mADMWaveChunkInfo[iter->first];

                ADMAudioMetadataSubDescriptor *adm_audio_subdesc = new ADMAudioMetadataSubDescriptor(mHeaderMetadata);
                descriptor->appendSubDescriptors(adm_audio_subdesc);
                adm_audio_subdesc->setRIFFChunkStreamID_link1(iter->first);
                if (!info.profile_and_level_uls.empty())
                    adm_audio_subdesc->setADMProfileLevelULBatch(info.profile_and_level_uls);
            }
        }
    }

    // Signal ST 379-2 generic container compliance if mSignalST3792 or there is RDD 36 / ProRes video.
    // RDD 44 (RDD 36 / ProRes in MXF) requires ST 379-2 and specifically mentions ContainerConstraintSubDescriptor.
    bool have_rdd36 = false;
    for (i = 0; i < mTracks.size(); i++) {
        if (dynamic_cast<OP1ARDD36Track*>(mTracks[i])) {
            have_rdd36 = true;
            break;
        }
    }
    if (mSignalST3792 || have_rdd36) {
        GenericDescriptor *descriptor = dynamic_cast<GenericDescriptor*>(mFileSourcePackage->getDescriptor());
        descriptor->appendSubDescriptors(new ContainerConstraintsSubDescriptor(mHeaderMetadata));
    }
}

void OP1AFile::CreateFile()
{
    BMX_ASSERT(mHavePreparedHeaderMetadata);

    // Check all the referenced MCA labels are present

    CheckMCALabels();


    // set minimum llen

    mMXFFile->setMinLLen(MIN_LLEN);


    // write to memory until generic stream / essence data writing starts or there is no essence and the file is completed

    mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);


    // write the header metadata

    Partition &header_partition = mMXFFile->createPartition();
    if (mSupportCompleteSinglePass)
        header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
    else
        header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setVersion(1, ((mFlavour & OP1A_377_2004_FLAVOUR) ? 2 : 3));
    if (HAVE_PRIMARY_EC &&
        ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) &&
        (mIndexTable->IsCBE() && !mIndexFollowsEssence))
    {
        header_partition.setIndexSID(mStreamIdHelper.GetId("IndexStream"));
    }
    else
    {
        header_partition.setIndexSID(0);
    }
    if (HAVE_PRIMARY_EC && (mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR))
        header_partition.setBodySID(mStreamIdHelper.GetId("BodyStream"));
    else
        header_partition.setBodySID(0);
    header_partition.setKagSize(mKAGSize);
    header_partition.setOperationalPattern(&mOPLabel);
    set<mxfUL>::const_iterator iter;
    for (iter = mEssenceContainerULs.begin(); iter != mEssenceContainerULs.end(); iter++)
        header_partition.addEssenceContainer(&(*iter));
    header_partition.write(mMXFFile);

    KAGFillerWriter reserve_filler_writer(&header_partition, mReserveMinBytes);
    mHeaderMetadata->write(mMXFFile, &header_partition, &reserve_filler_writer);
    mHeaderMetadataEndPos = mMXFFile->tell();  // need this position when we re-write the header metadata


    // write XML generic stream partitions

    size_t i;
    for (i = 0; i < mXMLTracks.size(); i++) {
        OP1AXMLTrack *xml_track = mXMLTracks[i];
        if (xml_track->RequireStreamPartition()) {
            if (header_partition.getIndexSID() || header_partition.getBodySID())
                BMX_EXCEPTION(("XML track's Generic Stream partition is currently incompatible with minimal partitions flavour"));
            if (mSupportCompleteSinglePass)
                BMX_EXCEPTION(("XML track's Generic Stream partition is currently incompatible with single pass flavour"));

            if (mMXFFile->isMemoryFileOpen())
                mMXFFile->closeMemoryFile();

            Partition &stream_partition = mMXFFile->createPartition();
            stream_partition.setKey(&MXF_GS_PP_K(GenericStream));
            stream_partition.setBodySID(xml_track->GetStreamId());
            stream_partition.write(mMXFFile);
            xml_track->WriteStreamXMLData(mMXFFile);
            stream_partition.fillToKag(mMXFFile);
        }
    }


    // write Wave chunk generic stream partitions

    map<uint32_t, WaveChunk*>::const_iterator wave_chunk_iter;
    for (wave_chunk_iter = mWaveChunks.begin(); wave_chunk_iter != mWaveChunks.end(); wave_chunk_iter++) {
        if (header_partition.getIndexSID() || header_partition.getBodySID())
            BMX_EXCEPTION(("Wave chunk's Generic Stream partition is currently incompatible with minimal partitions flavour"));
        if (mSupportCompleteSinglePass)
            BMX_EXCEPTION(("Wave chunk's Generic Stream partition is currently incompatible with single pass flavour"));

        if (mMXFFile->isMemoryFileOpen())
            mMXFFile->closeMemoryFile();

        Partition &stream_partition = mMXFFile->createPartition();
        stream_partition.setKey(&MXF_GS_PP_K(GenericStream));
        stream_partition.setBodySID(wave_chunk_iter->first);
        stream_partition.write(mMXFFile);

        uint8_t llen = mxf_get_llen(mMXFFile->getCFile(), wave_chunk_iter->second->Size());
        if (llen < 4)
            llen = 4;
        mMXFFile->writeFixedKL(&MXF_EE_K(WaveChunk), llen, wave_chunk_iter->second->Size());

        BMXMXFIO bmx_mxf_io(mMXFFile);
        dynamic_cast<BMXIO*>(&bmx_mxf_io)->Write(wave_chunk_iter->second);

        stream_partition.fillToKag(mMXFFile);
    }


    // write timed text track index and essence container

    for (i = 0; i < mTracks.size(); i++) {
        OP1ATimedTextTrack *tt_track = dynamic_cast<OP1ATimedTextTrack*>(mTracks[i]);
        if (!tt_track) {
            continue;
        }
        if (header_partition.getIndexSID() || header_partition.getBodySID())
            BMX_EXCEPTION(("Timed Text track's partitions are currently incompatible with minimal partitions flavour"));
        if (mSupportCompleteSinglePass)
            BMX_EXCEPTION(("Timed Text track's partitions are currently incompatible with single pass flavour"));

        if (mMXFFile->isMemoryFileOpen())
            mMXFFile->closeMemoryFile();

        // index is before timed text essence
        if (!mIndexFollowsEssence)
            WriteTimedTextIndexTable(tt_track);

        // write essence partition and container data
        Partition &ess_partition = mMXFFile->createPartition();
        if (mSupportCompleteSinglePass)
            ess_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
        else
            ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
        ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
        ess_partition.setIndexSID(0);
        ess_partition.setBodySID(tt_track->GetBodySID());
        ess_partition.setKagSize(mEssencePartitionKAGSize);
        ess_partition.setBodyOffset(0);
        ess_partition.write(mMXFFile);
        tt_track->WriteEssenceContainer(mMXFFile, &ess_partition);

        // index follows timed text essence
        if (mIndexFollowsEssence)
            WriteTimedTextIndexTable(tt_track);

        size_t k;
        for (k = 0; k < tt_track->GetNumAncillaryResources(); k++) {
            Partition &stream_partition = mMXFFile->createPartition();
            stream_partition.setKey(&MXF_GS_PP_K(GenericStream));
            stream_partition.setBodySID(tt_track->GetAncillaryResourceStreamId(k));
            // Note: ST 429-5 says no KLV Fill is permitted after the Partition Pack.
            //       ST 429-3, which ST 429-5 conforms to with some exceptions, states
            //       that the KAG size shall be 1. This requirement would result in no
            //       KLV Fill in bmx and probably most other implementations.
            //       However, a KLV Fill will be written here if KAG size != 1 because a
            //       KAG size != 1 would make the file not compliant with ST 429-5 and
            //       there is no reason to disallow a KLV Fill in a Generic Stream
            //       Container - ST 410 allows it.
            stream_partition.write(mMXFFile);
            tt_track->WriteAncillaryResource(mMXFFile, &ess_partition, k);
        }
    }
}

void OP1AFile::WriteTimedTextIndexTable(OP1ATimedTextTrack *tt_track)
{
    Partition &index_partition = mMXFFile->createPartition();
    if (mSupportCompleteSinglePass)
        index_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
    else
        index_partition.setKey(&MXF_PP_K(OpenComplete, Body));
    index_partition.setIndexSID(tt_track->GetIndexSID());
    index_partition.setBodySID(0);
    index_partition.setKagSize(mKAGSize);
    index_partition.write(mMXFFile);

    tt_track->WriteIndexTable(mMXFFile, &index_partition);
}

void OP1AFile::UpdatePackageMetadata()
{
    int64_t output_duration = GetDuration();

    UpdateTrackMetadata(mMaterialPackage, 0, output_duration);

    int64_t container_duration;
    if (mFileSourcePackage) {
        UpdateTrackMetadata(mFileSourcePackage, mOutputStartOffset, output_duration + mOutputStartOffset);

        container_duration = GetContainerDuration();

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
    } else {
        container_duration = output_duration;
    }

    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        OP1ATimedTextTrack *tt_track = dynamic_cast<OP1ATimedTextTrack*>(mTracks[i]);
        if (tt_track) {
            tt_track->UpdateTrackMetadata(output_duration);
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
            if (components.size() == 1) {
                components[0]->setDuration(duration);
            } // else it's a Timed Text track which is handled separately
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
    if (!HAVE_PRIMARY_EC) {
        return;
    }

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
            // write CBE index table segments (if it doesn't follow essence) and ensure new essence partition is started

            if (mIndexTable->IsCBE() && !mIndexFollowsEssence) {
                // make sure edit unit byte count and delta entries are known
                mCPManager->UpdateIndexTable(mCPManager->HaveContentPackages(2) ? 2 : 1);

                if ((mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR) || (mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) {
                    mCBEIndexPartitionIndex = 0;
                } else {
                    Partition &index_partition = mMXFFile->createPartition();
                    if (mSupportCompleteSinglePass)
                        index_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
                    else
                        index_partition.setKey(&MXF_PP_K(OpenComplete, Body));
                    index_partition.setIndexSID(mStreamIdHelper.GetId("IndexStream"));
                    index_partition.setBodySID(0);
                    index_partition.setKagSize(mKAGSize);
                    index_partition.write(mMXFFile);

                    mCBEIndexPartitionIndex = mMXFFile->getPartitions().size() - 1;
                }
                mIndexTable->WriteSegments(mMXFFile, &mMXFFile->getPartition(mCBEIndexPartitionIndex), false);
            }

            if (!(mFlavour & OP1A_MIN_PARTITIONS_FLAVOUR))
                start_ess_partition = true;
            mFirstWrite = false;
        }
        else if (mWaitForIndexComplete)
        {
            if (!mIndexTable->RequireUpdatesAtPos(mCPManager->GetPosition() - 1)) {
                mWaitForIndexComplete = false;
                start_ess_partition = true;
            } else if (end_of_samples) {
                mWaitForIndexComplete = false;
            } else {
                if (mCPManager->HaveContentPackages(MAX_BUFFERED_CONTENT_PACKAGES + 1)) {
                    BMX_EXCEPTION(("Memory buffered content packages > %d whilst waiting for index updates",
                                   MAX_BUFFERED_CONTENT_PACKAGES));
                }
                break;
            }
        }
        else if (mPartitionInterval > 0 &&
                 mPartitionFrameCount >= mPartitionInterval && mIndexTable->CanStartPartition())
        {
            BMX_ASSERT(!mSupportCompleteSinglePass);

            mPartitionFrameCount = 0;
            if (mIndexTable->RequireUpdatesAtPos(mCPManager->GetPosition() - 1)) {
                if (!end_of_samples) {
                    mWaitForIndexComplete = true;
                    break;
                }
            } else {
                start_ess_partition = true;
            }
        }

        if (start_ess_partition) {
            // write VBE index table segments and ensure new essence partition is started

            if (mIndexTable->IsVBE() && mIndexTable->HaveSegments()) {
                BMX_ASSERT(!mSupportCompleteSinglePass);

                mMXFFile->openMemoryFile(MEMORY_WRITE_CHUNK_SIZE);

                Partition &body_partition = mMXFFile->createPartition();
                body_partition.setKey(&MXF_PP_K(OpenComplete, Body));
                body_partition.setIndexSID(mStreamIdHelper.GetId("IndexStream"));
                if ((mFlavour & OP1A_BODY_PARTITIONS_FLAVOUR)) {
                    // Index and essence are contained in the same body partition
                    body_partition.setBodySID(mStreamIdHelper.GetId("BodyStream"));
                    body_partition.setKagSize(mEssencePartitionKAGSize);
                    body_partition.setBodyOffset(ess_partition_body_offset);
                    start_ess_partition = false;
                } else {
                    // Index and essence are contained in separate body partitions
                    body_partition.setBodySID(0);
                    body_partition.setKagSize(mKAGSize);
                }
                body_partition.write(mMXFFile);

                mIndexTable->WriteSegments(mMXFFile, &body_partition, true);
            }

            if (start_ess_partition) {
                Partition &ess_partition = mMXFFile->createPartition();
                if (mSupportCompleteSinglePass)
                    ess_partition.setKey(&MXF_PP_K(ClosedComplete, Body));
                else
                    ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
                ess_partition.setIndexSID(0);
                ess_partition.setBodySID(mStreamIdHelper.GetId("BodyStream"));
                ess_partition.setKagSize(mEssencePartitionKAGSize);
                ess_partition.setBodyOffset(ess_partition_body_offset);
                ess_partition.write(mMXFFile);
            }
            // else the body_partition created above will contain the essence
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
    BMX_ASSERT(mSupportCompleteSinglePass && HAVE_PRIMARY_EC);

    uint32_t first_size = 0, non_first_size = 0;
    mIndexTable->GetCBEEditUnitSize(&first_size, &non_first_size);

    mFooterPartitionOffset = mMXFFile->tell();
    if (mInputDuration > 0)
        mFooterPartitionOffset += first_size + (mInputDuration - 1) * non_first_size;

    size_t i;
    for (i = 0; i < mMXFFile->getPartitions().size(); i++)
        mMXFFile->getPartitions()[i]->setFooterPartition(mFooterPartitionOffset);
}

void OP1AFile::CheckMCALabels()
{
    vector<MCALabelSubDescriptor*> mca_labels;
    size_t t;
    for (t = 0; t < mTracks.size(); t++) {
        OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mTracks[t]);
        if (!pcm_track)
            continue;

        const vector<MCALabelSubDescriptor*> &track_mca_labels = pcm_track->GetMCALabels();
        mca_labels.insert(mca_labels.end(), track_mca_labels.begin(), track_mca_labels.end());
    }

    if (!check_mca_labels(mca_labels))
      BMX_EXCEPTION(("Invalid MCA labels"));
}
