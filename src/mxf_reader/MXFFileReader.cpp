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
#define __STDC_LIMIT_MACROS

#include <cstdio>

#include <algorithm>
#include <memory>
#include <set>

#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>
#include <bmx/essence_parser/AVCIEssenceParser.h>
#include <bmx/st436/ST436Element.h>
#include <bmx/MXFHTTPFile.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define TO_ESS_READER_POS(pos)      (pos + mFileOrigin)
#define FROM_ESS_READER_POS(pos)    (pos - mFileOrigin)

#define CONVERT_INTERNAL_DUR(dur)   convert_duration_higher(dur, mExternalSampleSequences[i], mExternalSampleSequenceSizes[i])
#define CONVERT_EXTERNAL_DUR(dur)   convert_duration_lower(dur, mExternalSampleSequences[i], mExternalSampleSequenceSizes[i])
#define CONVERT_INTERNAL_POS(pos)   convert_position_higher(pos, mExternalSampleSequences[i], mExternalSampleSequenceSizes[i])
#define CONVERT_EXTERNAL_POS(pos)   convert_position_lower(pos, mExternalSampleSequences[i], mExternalSampleSequenceSizes[i])

#define CHECK_SUPPORT_READ_LIMITS                                                       \
    do {                                                                                \
        if (!IsComplete())                                                              \
            BMX_EXCEPTION(("Read limits are not supported when the file is "            \
                           "incomplete or duration is unknown"));                       \
    } while (0)

#define CHECK_SUPPORT_PC_RO_INFO                                                        \
    do {                                                                                \
        if (!IsComplete())                                                              \
            BMX_EXCEPTION(("Precharge and rollout information are not available "       \
                           "when the file is incomplete or duration is unknown"));      \
    } while (0)



static const char *RESULT_STRINGS[] =
{
    "success",
    "could not open file",
    "invalid or not an MXF file",
    "not supported",
    "header metadata not found",
    "invalid header metadata",
    "no essence available",
    "no essence index table",
    "incomplete index table",
    "general error",
};


#define THROW_RESULT(result)                                                        \
    do {                                                                            \
        log_warn("Open error '%s' near %s:%d\n", #result, __FILE__, __LINE__);      \
        throw result;                                                               \
    } while (0)



static bool compare_track_reader(const MXFTrackReader *left_reader, const MXFTrackReader *right_reader)
{
    const MXFTrackInfo *left = left_reader->GetTrackInfo();
    const MXFTrackInfo *right = right_reader->GetTrackInfo();

    // data kind
    if (left->data_def != right->data_def)
        return left->data_def < right->data_def;

    // track number
    if (left->material_track_number != 0) {
        return right->material_track_number == 0 ||
               left->material_track_number < right->material_track_number;
    }
    if (right->material_track_number != 0)
        return false;

    // track id
    if (left->material_track_id != 0) {
        return right->material_track_id == 0 ||
               left->material_track_id < right->material_track_id;
    }
    return right->material_track_id == 0;
}



string MXFFileReader::ResultToString(OpenResult result)
{
    size_t index = (size_t)(result);
    BMX_ASSERT(index < BMX_ARRAY_SIZE(RESULT_STRINGS));

    return RESULT_STRINGS[index];
}


MXFFileReader::MXFFileReader()
: MXFReader()
{
    BMX_ASSERT(MXF_RESULT_FAIL + 1 == BMX_ARRAY_SIZE(RESULT_STRINGS));

    mFileId = (size_t)(-1);
    mFile = 0;
    mEmptyFrames = false;
    mEmptyFramesSet = false;
    mHeaderMetadata = 0;
    mMXFVersion = 0;
    mOPLabel = g_Null_UL;
    mGuessedWrappingType = MXF_FRAME_WRAPPED;
    mWrappingType = MXF_UNKNOWN_WRAPPING_TYPE;
    mBodySID = 0;
    mIndexSID = 0;
    mReadStartPosition = 0;
    mReadDuration = -1;
    mFileOrigin = 0;
    mEssenceReader = 0;
    mRequireFrameInfoCount = 0;
    mST436ManifestCount = 2;

    mDataModel = new DataModel();

    mPackageResolver = new DefaultMXFPackageResolver();
    mOwnPackageResolver = true;

    mFileFactory = new DefaultMXFFileFactory();
    mOwnFilefactory = true;
}

MXFFileReader::~MXFFileReader()
{
    if (mOwnPackageResolver)
        delete mPackageResolver;
    if (mOwnFilefactory)
        delete mFileFactory;
    delete mEssenceReader;
    delete mFile;
    delete mHeaderMetadata;
    delete mDataModel;

    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++)
        delete mInternalTrackReaders[i];

    // mPackageResolver owns external readers
}

void MXFFileReader::SetPackageResolver(MXFPackageResolver *resolver, bool take_ownership)
{
    if (mOwnPackageResolver)
        delete mPackageResolver;

    mPackageResolver = resolver;
    mOwnPackageResolver = take_ownership;
}

void MXFFileReader::SetFileFactory(MXFFileFactory *factory, bool take_ownership)
{
    if (mOwnFilefactory)
        delete mFileFactory;

    mFileFactory = factory;
    mOwnFilefactory = take_ownership;
}

void MXFFileReader::SetEmptyFrames(bool enable)
{
    mEmptyFrames = enable;
    mEmptyFramesSet = true;

    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++)
        mTrackReaders[i]->SetEmptyFrames(enable);
}

void MXFFileReader::SetST436ManifestFrameCount(uint32_t count)
{
    mST436ManifestCount = count;
}

void MXFFileReader::SetFileIndex(MXFFileIndex *file_index, bool take_ownership)
{
    if (mFileId != (size_t)(-1))
        mFileId = file_index->RegisterFile(mFileIndex->GetEntry(mFileId));

    MXFReader::SetFileIndex(file_index, take_ownership);

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++)
        mExternalReaders[i]->SetFileIndex(file_index, false);
}

MXFFileReader::OpenResult MXFFileReader::Open(string filename)
{
    File *file = 0;
    try
    {
        file = mFileFactory->OpenRead(filename);

        OpenResult result;
        if (filename.empty())
            result = Open(file, URI("stdin:"), URI(), "");
        else
            result = Open(file, filename);
        if (result != MXF_RESULT_SUCCESS)
            delete file;

        return result;
    }
    catch (...)
    {
        delete file;
        return MXF_RESULT_OPEN_FAIL;
    }
}

MXFFileReader::OpenResult MXFFileReader::Open(File *file, string filename)
{
    try
    {
        URI rel_uri, abs_uri;
        if (mxf_http_is_url(filename)) {
            BMX_CHECK(abs_uri.Parse(filename));
        } else {
            BMX_CHECK(abs_uri.ParseFilename(filename));
            if (abs_uri.IsRelative()) {
                rel_uri = abs_uri;

                URI base_uri;
                BMX_CHECK(base_uri.ParseDirectory(get_cwd()));
                abs_uri.MakeAbsolute(base_uri);
            }
        }

        return Open(file, abs_uri, rel_uri, filename);
    }
    catch (...)
    {
        return MXF_RESULT_OPEN_FAIL;
    }
}

MXFFileReader::OpenResult MXFFileReader::Open(File *file, const URI &abs_uri, const URI &rel_uri, const string &filename)
{
    OpenResult result;

    try
    {
        mFile = file;
        mFileId = mFileIndex->RegisterFile(abs_uri, rel_uri, filename);

        // read the header partition pack and check the operational pattern
        if (!file->readHeaderPartition()) {
            log_error("Failed to find and read header partition\n");
            THROW_RESULT(MXF_RESULT_INVALID_FILE);
        }
        Partition &header_partition = file->getPartition(0);

        if (!mxf_is_op_atom(header_partition.getOperationalPattern()) &&
            !mxf_is_op_1a(header_partition.getOperationalPattern()) &&
            !mxf_is_op_1b(header_partition.getOperationalPattern()))
        {
            log_error("Operational pattern is not supported\n");
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        }


        mOPLabel = *header_partition.getOperationalPattern();


        // get or guess the essence wrapping type

        vector<mxfUL> essence_labels = header_partition.getEssenceContainers();
        size_t i;
        for (i = 0; i < essence_labels.size(); i++) {
            mWrappingType = mxf_get_essence_wrapping_type(&essence_labels[i]);
            if (mWrappingType != MXF_UNKNOWN_WRAPPING_TYPE)
                break;
        }
        if (mWrappingType == MXF_UNKNOWN_WRAPPING_TYPE) {
            // guess the wrapping type based on the OP
            mGuessedWrappingType = (mxf_is_op_atom(&mOPLabel) ? MXF_CLIP_WRAPPED : MXF_FRAME_WRAPPED);
        }


        // try read all partitions find and get last partition with header metadata

        bool file_is_complete;
        if (mFile->isSeekable()) {
            file_is_complete = mFile->readPartitions();
            if (!file_is_complete) {
                BMX_ASSERT(mFile->getPartitions().size() == 1);
                if (mFile->getPartition(0).isClosed() || mFile->getPartition(0).getFooterPartition() != 0)
                    log_warn("Failed to read all partitions. File may be incomplete or invalid\n");
            }
        } else {
            file_is_complete = false;
        }
        const vector<Partition*> &partitions = mFile->getPartitions();
        Partition *metadata_partition = 0;
        for (i = partitions.size(); i > 0 ; i--) {
            if (partitions[i - 1]->getHeaderByteCount() > 0) {
                metadata_partition = partitions[i - 1];
                break;
            }
        }
        if (!metadata_partition)
            THROW_RESULT(MXF_RESULT_NO_HEADER_METADATA);


        // read and process the header metadata

        mxfKey key;
        uint8_t llen;
        uint64_t len;
        if (mFile->isSeekable()) {
            mFile->seek(metadata_partition->getThisPartition(), SEEK_SET);
            mFile->readKL(&key, &llen, &len);
            mFile->skip(len);
        }
        mFile->readNextNonFillerKL(&key, &llen, &len);
        BMX_CHECK(mxf_is_header_metadata(&key));

        mHeaderMetadata = new AvidHeaderMetadata(mDataModel);
        mHeaderMetadata->read(mFile, metadata_partition, &key, llen, len);

        ProcessMetadata(metadata_partition);


        // create internal essence reader
        if (!mInternalTrackReaders.empty()) {
            mEssenceReader = new EssenceReader(this, file_is_complete);

            CheckRequireFrameInfo();
            if (mRequireFrameInfoCount > 0)
                ExtractFrameInfo();
        } else {
            mWrappingType = MXF_UNKNOWN_WRAPPING_TYPE;
        }

        if (IsComplete()) {
            if (mIndexSID && mEssenceReader && mEssenceReader->GetIndexedDuration() < mDuration) {
                log_warn("Essence index duration %"PRId64" is less than track duration %"PRId64"\n",
                         mEssenceReader->GetIndexedDuration(), mDuration);
            }
            if (GetMaxPrecharge(0, true) != GetMaxPrecharge(0, false)) {
                log_warn("Possibly not enough precharge available (available=%d, required=%d)\n",
                         GetMaxPrecharge(0, true), GetMaxPrecharge(0, false));
            }
            if (GetMaxRollout(mDuration - 1, true) != GetMaxRollout(mDuration - 1, false)) {
                log_warn("Possibly not enough rollout available (available=%d, required=%d)\n",
                         GetMaxRollout(mDuration - 1, true), GetMaxRollout(mDuration - 1, false));
            }

            SetReadLimits();
        } else if (mDuration > 0) {
            SetReadLimits(- mOrigin, mOrigin + mDuration, false);
        }


        if (mEmptyFramesSet) {
            for (i = 0; i < mTrackReaders.size(); i++)
                mTrackReaders[i]->SetEmptyFrames(mEmptyFrames);
        }


        result = MXF_RESULT_SUCCESS;
    }
    catch (const OpenResult &ex)
    {
        result = ex;
    }
    catch (const BMXException &ex)
    {
        log_error("BMX exception: %s\n", ex.what());
        result = MXF_RESULT_FAIL;
    }
    catch (...)
    {
        result = MXF_RESULT_FAIL;
    }

    // clean up
    if (result != MXF_RESULT_SUCCESS) {
        mFile = 0;
        delete mEssenceReader;
        mEssenceReader = 0;
        delete mHeaderMetadata;
        mHeaderMetadata = 0;
        delete mDataModel;
        mDataModel = 0;

        size_t i;
        for (i = 0; i < mInternalTrackReaders.size(); i++)
            delete mInternalTrackReaders[i];

        mTrackReaders.clear();
        mInternalTrackReaders.clear();
        mInternalTrackReaderNumberMap.clear();
        mExternalReaders.clear();
        mExternalSampleSequences.clear();
        mExternalSampleSequenceSizes.clear();
        mExternalTrackReaders.clear();
    }

    return result;
}

MXFFileReader* MXFFileReader::GetFileReader(size_t file_id)
{
    MXFFileReader *reader = 0;
    if (file_id == mFileId) {
        reader = this;
    } else {
        size_t i;
        for (i = 0; i < mExternalReaders.size(); i++) {
            reader = mExternalReaders[i]->GetFileReader(file_id);
            if (reader)
                break;
        }
    }

    return reader;
}

vector<size_t> MXFFileReader::GetFileIds(bool internal_ess_only) const
{
    set<size_t> file_id_set;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        vector<size_t> track_file_ids = mTrackReaders[i]->GetFileIds(internal_ess_only);
        file_id_set.insert(track_file_ids.begin(), track_file_ids.end());
    }
    if (!internal_ess_only)
        file_id_set.insert(mFileId);

    vector<size_t> file_ids;
    file_ids.insert(file_ids.begin(), file_id_set.begin(), file_id_set.end());

    return file_ids;
}

bool MXFFileReader::IsComplete() const
{
    if (mDuration < 0 || (mEssenceReader && !mEssenceReader->IsComplete()))
        return false;

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsComplete())
            return false;
    }

    return true;
}

bool MXFFileReader::IsSeekable() const
{
    if (mEssenceReader && !mFile->isSeekable())
        return false;

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsSeekable())
            return false;
    }

    return true;
}

void MXFFileReader::GetReadLimits(bool limit_to_available, int64_t *start_position, int64_t *duration) const
{
    CHECK_SUPPORT_READ_LIMITS;

    int16_t precharge = GetMaxPrecharge(0, limit_to_available);
    int16_t rollout = GetMaxRollout(mDuration - 1, limit_to_available);
    *start_position = 0 + precharge;
    *duration = - precharge + mDuration + rollout;
}

void MXFFileReader::SetReadLimits()
{
    CHECK_SUPPORT_READ_LIMITS;

    int64_t start_position;
    int64_t duration;
    GetReadLimits(false, &start_position, &duration);
    SetReadLimits(start_position, duration, true);
}

void MXFFileReader::SetReadLimits(int64_t start_position, int64_t duration, bool seek_to_start)
{
    mReadStartPosition = start_position;
    mReadDuration = duration;

    if (InternalIsEnabled())
        mEssenceReader->SetReadLimits(TO_ESS_READER_POS(start_position), duration);

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        int64_t external_start_position = CONVERT_INTERNAL_POS(start_position);
        int64_t external_duration;
        if (duration == 0)
            external_duration = 0;
        else
            external_duration = CONVERT_INTERNAL_DUR(start_position + duration) - external_start_position;
        mExternalReaders[i]->SetReadLimits(external_start_position, external_duration, false /* seek done below */);
    }

    if (seek_to_start)
        Seek(start_position);
}

uint32_t MXFFileReader::Read(uint32_t num_samples, bool is_top)
{
    mReadError = false;
    mReadErrorMessage.clear();

    if (mRequireFrameInfoCount > 0) {
        ExtractFrameInfo();
        if (mRequireFrameInfoCount > 0) {
            mReadError = true;
            mReadErrorMessage = "Failed to extract information from frame(s)";
            return 0;
        }
    }

    int64_t current_position = GetPosition();

    StartRead();
    try
    {
        if (is_top) {
            SetNextFramePosition(mEditRate, current_position);
            SetNextFrameTrackPositions();
        }

        uint32_t max_num_read = 0;
        if (InternalIsEnabled())
            max_num_read = mEssenceReader->Read(num_samples);

        size_t i;
        for (i = 0; i < mExternalReaders.size(); i++) {
            if (!mExternalReaders[i]->IsEnabled())
                continue;

            int64_t external_current_position = CONVERT_INTERNAL_POS(current_position);

            // ensure external reader is in sync
            if (mExternalReaders[i]->GetPosition() != external_current_position)
                mExternalReaders[i]->Seek(external_current_position);


            uint32_t num_external_samples = (uint32_t)convert_duration_higher(num_samples,
                                                                              current_position,
                                                                              mExternalSampleSequences[i],
                                                                              mExternalSampleSequenceSizes[i]);

            uint32_t external_num_read = mExternalReaders[i]->Read(num_external_samples, false);
            if (external_num_read < num_external_samples && mExternalReaders[i]->ReadError())
                throw BMXException(mExternalReaders[i]->ReadErrorMessage());

            uint32_t internal_num_read = (uint32_t)convert_duration_lower(external_num_read,
                                                                          external_current_position,
                                                                          mExternalSampleSequences[i],
                                                                          mExternalSampleSequenceSizes[i]);

            if (internal_num_read > max_num_read)
                max_num_read = internal_num_read;
        }

        BMX_ASSERT(max_num_read <= num_samples);

        CompleteRead();

        return max_num_read;
    }
    catch (const MXFException &ex)
    {
        mReadErrorMessage = ex.getMessage();
    }
    catch (const BMXException &ex)
    {
        mReadErrorMessage = ex.what();
    }
    catch (...)
    {
    }

    mReadError = true;
    AbortRead();
    Seek(current_position);

    return 0;
}

void MXFFileReader::Seek(int64_t position)
{
    if (InternalIsEnabled())
        mEssenceReader->Seek(TO_ESS_READER_POS(position));

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        mExternalReaders[i]->Seek(CONVERT_INTERNAL_POS(position));
    }
}

int64_t MXFFileReader::GetPosition() const
{
    int64_t position = 0;
    if (InternalIsEnabled()) {
        position = FROM_ESS_READER_POS(mEssenceReader->GetPosition());
    } else {
        size_t i;
        for (i = 0; i < mExternalReaders.size(); i++) {
            if (!mExternalReaders[i]->IsEnabled())
                continue;

            position = CONVERT_EXTERNAL_POS(mExternalReaders[i]->GetPosition());
            break;
        }
    }

    return position;
}

int16_t MXFFileReader::GetMaxPrecharge(int64_t position, bool limit_to_available) const
{
    CHECK_SUPPORT_PC_RO_INFO;

    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    int64_t max_start_position = INT64_MIN;
    int64_t precharge = 0;
    if (InternalIsEnabled()) {
        precharge = GetInternalPrecharge(target_position, limit_to_available);
        if (limit_to_available) {
            int64_t start_position, duration;
            GetInternalAvailableReadLimits(&start_position, &duration);
            max_start_position = start_position;
        }
    }

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        int16_t ext_reader_precharge = mExternalReaders[i]->GetMaxPrecharge(CONVERT_INTERNAL_POS(target_position),
                                                                            limit_to_available);
        if (ext_reader_precharge != 0) {
            BMX_CHECK_M(mExternalReaders[i]->GetEditRate() == mEditRate,
                       ("Currently only support precharge in external reader if "
                        "external reader edit rate equals group edit rate"));
            if (ext_reader_precharge < precharge)
                precharge = ext_reader_precharge;
        }

        if (limit_to_available) {
            int64_t ext_start_position, ext_duration;
            mExternalReaders[i]->GetReadLimits(true, &ext_start_position, &ext_duration);
            int64_t int_max_start_position = CONVERT_EXTERNAL_POS(ext_start_position);
            if (int_max_start_position > max_start_position)
                max_start_position = int_max_start_position;
        }
    }

    if (limit_to_available && precharge < max_start_position - target_position)
        precharge = max_start_position - target_position;

    return precharge < 0 ? (int16_t)precharge : 0;
}

int16_t MXFFileReader::GetMaxRollout(int64_t position, bool limit_to_available) const
{
    CHECK_SUPPORT_PC_RO_INFO;

    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    int64_t min_end_position = INT64_MAX;
    int64_t rollout = 0;
    if (InternalIsEnabled()) {
        rollout = GetInternalRollout(target_position, limit_to_available);
        if (limit_to_available) {
            int64_t start_position, duration;
            GetInternalAvailableReadLimits(&start_position, &duration);
            min_end_position = start_position + duration;
        }
    }

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        int16_t ext_reader_rollout = mExternalReaders[i]->GetMaxRollout(CONVERT_INTERNAL_POS(target_position + 1) - 1,
                                                                        limit_to_available);
        if (ext_reader_rollout != 0) {
            BMX_CHECK_M(mExternalReaders[i]->GetEditRate() == mEditRate,
                       ("Currently only support rollout in external reader if "
                        "external reader edit rate equals group edit rate"));
            if (ext_reader_rollout > rollout)
                rollout = ext_reader_rollout;
        }

        if (limit_to_available) {
            int64_t ext_start_position, ext_duration;
            mExternalReaders[i]->GetReadLimits(true, &ext_start_position, &ext_duration);
            int64_t int_min_end_position = CONVERT_EXTERNAL_DUR(ext_start_position + ext_duration);
            if (int_min_end_position < min_end_position)
                min_end_position = int_min_end_position;
        }
    }

    if (limit_to_available && rollout > min_end_position - target_position)
        rollout = min_end_position - target_position;

    return rollout > 0 ? (int16_t)rollout : 0;
}

bool MXFFileReader::HaveFixedLeadFillerOffset() const
{
    int64_t fixed_offset = 0;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        // note that edit_rate and lead_filler_offset are from this MXF file's material package
        int64_t offset = convert_position(mTrackReaders[i]->GetTrackInfo()->edit_rate,
                                          mTrackReaders[i]->GetTrackInfo()->lead_filler_offset,
                                          mEditRate,
                                          ROUND_UP);
        if (i == 0)
            fixed_offset = offset;
        else if (fixed_offset != offset)
            return false;
    }

    return true;
}

int64_t MXFFileReader::GetFixedLeadFillerOffset() const
{
    int64_t fixed_offset = 0;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        // note that edit_rate and lead_filler_offset are from this MXF file's material package
        int64_t offset = convert_position(mTrackReaders[i]->GetTrackInfo()->edit_rate,
                                          mTrackReaders[i]->GetTrackInfo()->lead_filler_offset,
                                          mEditRate,
                                          ROUND_UP);
        if (i == 0)
            fixed_offset = offset;
        else if (fixed_offset != offset)
            return 0; // default to 0 if HaveFixedLeadFillerOffset returns false
    }

    return fixed_offset;
}

MXFTrackReader* MXFFileReader::GetTrackReader(size_t index) const
{
    BMX_CHECK(index < mTrackReaders.size());
    return mTrackReaders[index];
}

bool MXFFileReader::IsEnabled() const
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            return true;
    }

    return false;
}

int16_t MXFFileReader::GetTrackPrecharge(size_t track_index, int64_t clip_position, int16_t clip_precharge) const
{
    CHECK_SUPPORT_PC_RO_INFO;

    if (clip_precharge >= 0)
        return 0;

    MXFTrackReader *track_reader = GetTrackReader(track_index);

    BMX_CHECK_M(track_reader->GetEditRate() == mEditRate,
               ("Currently only support precharge in external reader if "
                "external reader edit rate equals group edit rate"));
    (void)clip_position;

    return clip_precharge;
}

int16_t MXFFileReader::GetTrackRollout(size_t track_index, int64_t clip_position, int16_t clip_rollout) const
{
    CHECK_SUPPORT_PC_RO_INFO;

    if (clip_rollout <= 0)
        return 0;

    MXFTrackReader *track_reader = GetTrackReader(track_index);

    BMX_CHECK_M(track_reader->GetEditRate() == mEditRate,
               ("Currently only support rollout in external reader if "
                "external reader edit rate equals group edit rate"));
    (void)clip_position;

    return clip_rollout;
}

void MXFFileReader::SetNextFramePosition(Rational edit_rate, int64_t position)
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->SetNextFramePosition(edit_rate, position);
    }
}

void MXFFileReader::SetNextFrameTrackPositions()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled()) {
            mTrackReaders[i]->GetMXFFrameBuffer()->SetNextFrameTrackPosition(
                mTrackReaders[i]->GetEditRate(), mTrackReaders[i]->GetPosition());
        }
    }
}

void MXFFileReader::SetTemporaryFrameBuffer(bool enable)
{
    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++)
        mInternalTrackReaders[i]->GetMXFFrameBuffer()->SetTemporaryBuffer(enable);
}

void MXFFileReader::ProcessMetadata(Partition *partition)
{
    Preface *preface = mHeaderMetadata->getPreface();
    mMXFVersion = preface->getVersion();

    // index packages from this file
    mPackageResolver->ExtractPackages(this);


    // create track readers for each material package picture, sound or data track

    mMaterialPackage = preface->findMaterialPackage();
    BMX_CHECK(mMaterialPackage);
    mMaterialPackageUID = mMaterialPackage->getPackageUID();
    if (mMaterialPackage->haveName())
        mMaterialPackageName = mMaterialPackage->getName();

    vector<SourcePackage*> file_source_packages = preface->findFileSourcePackages();
    if (file_source_packages.empty()) {
        log_error("File with no file source packages is not supported\n");
        THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
    }

    Track *infile_mp_track = 0;
    vector<GenericTrack*> mp_tracks = mMaterialPackage->getTracks();
    uint32_t skipped_track_count = 0;
    size_t i;
    for (i = 0; i < mp_tracks.size(); i++) {
        Track *mp_track = dynamic_cast<Track*>(mp_tracks[i]);
        if (!mp_track)
            continue;

        // skip if not picture, sound or data
        StructuralComponent *track_sequence = mp_track->getSequence();
        mxfUL data_def_ul = track_sequence->getDataDefinition();
        MXFDataDefEnum data_def = mxf_get_ddef_enum(&data_def_ul);
        if (data_def != MXF_PICTURE_DDEF && data_def != MXF_SOUND_DDEF && data_def != MXF_DATA_DDEF)
            continue;

        uint32_t mp_track_id = 0;
        if (mp_track->haveTrackID())
            mp_track_id = mp_track->getTrackID();
        else
            log_warn("Material track does not have a TrackID property\n");

        BMX_CHECK(mp_track->getOrigin() == 0);

        // skip if not a Sequence->SourceClip or SourceClip
        int64_t lead_filler_offset = 0;
        Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
        SourceClip *mp_source_clip = dynamic_cast<SourceClip*>(track_sequence);
        if (sequence) {
            vector<StructuralComponent*> components = sequence->getStructuralComponents();
            size_t j;
            for (j = 0; j < components.size(); j++) {
                mp_source_clip = dynamic_cast<SourceClip*>(components[j]);
                if (mp_source_clip) {
                    break;
                } else {
                    if (mxf_equals_key(components[j]->getKey(), &MXF_SET_K(Filler))) {
                        // lead Filler segments, e.g. used for P2 clips spanning multiple cards
                        lead_filler_offset += components[j]->getDuration();
                    } else if (mxf_equals_key(components[j]->getKey(), &MXF_SET_K(EssenceGroup))) {
                        // Essence Group used in Avid files, e.g. alpha component tracks
                        auto_ptr<ObjectIterator> choices(components[j]->getStrongRefArrayItem(
                            &MXF_ITEM_K(EssenceGroup, Choices)));
                        if (!choices->next())
                            BMX_EXCEPTION(("0 Choices found in EssenceGroup"));
                        mp_source_clip = dynamic_cast<SourceClip*>(choices->get());
                        if (!mp_source_clip) {
                            log_error("EssenceGroup choice that is not a SourceClip is not supported\n");
                            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
                        }
                        if (choices->next())
                            log_warn("Using the first SourceClip in EssenceGroup containing multiple choices\n");
                    } else {
                        log_error("StructuralComponent in Sequence is not a SourceClip, Filler or EssenceGroup\n");
                        THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
                    }
                }
            }
        }
        if (!mp_source_clip) {
            log_warn("Skipping material package track %u which has no SourceClip\n", mp_track_id);
            skipped_track_count++;
            continue;
        }

        // Avid files will have a non-zero start position if consolidation of a sequence
        // required the first couple of frames to be re-encoded. The start position is equivalent to
        // using origin to indicate precharge.
        if (mp_source_clip->getStartPosition() != 0) {
            if (mp_source_clip->getStartPosition() < 0) {
                log_error("A negative material package source clip StartPosition is not supported\n");
                THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
            }
            mxfUL op = preface->getOperationalPattern();
            if (!mxf_is_op_atom(&op)) {
                log_error("Non-zero material package source clip StartPosition is only supported in OP-Atom files\n");
                THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
            }
        }

        // skip if could not resolve the source clip
        vector<ResolvedPackage> resolved_packages = mPackageResolver->ResolveSourceClip(mp_source_clip);
        if (resolved_packages.empty()) {
            skipped_track_count++;
            continue;
        }

        // require top level file source package to be described in this file
        const ResolvedPackage *resolved_package = 0;
        size_t j;
        for (j = 0; j < resolved_packages.size(); j++) {
            if (resolved_packages[j].is_file_source_package && resolved_packages[j].file_reader == this) {
                resolved_package = &resolved_packages[j];
                break;
            }
        }
        if (!resolved_package) {
            log_error("An external top level file source package is not supported\n");
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        }
        SourcePackage *file_source_package = dynamic_cast<SourcePackage*>(resolved_package->package);
        BMX_CHECK(file_source_package);

        // check the material package track and file source package track data definitions match
        uint32_t fsp_track_id = 0;
        if (resolved_package->generic_track->haveTrackID())
            fsp_track_id = resolved_package->generic_track->getTrackID();
        Track *fsp_track = dynamic_cast<Track*>(resolved_package->generic_track);
        MXFDataDefEnum fsp_data_def = MXF_UNKNOWN_DDEF;
        if (fsp_track) {
            StructuralComponent *fsp_sequence = fsp_track->getSequence();
            mxfUL fsp_data_def_ul = fsp_sequence->getDataDefinition();
            fsp_data_def = mxf_get_ddef_enum(&fsp_data_def_ul);
        }
        if (!fsp_track || fsp_data_def != data_def) {
            log_error("Material package track %u data def does not match referenced "
                      "file source package track %u data def\n", mp_track_id, fsp_track_id);
            THROW_RESULT(MXF_RESULT_INVALID_FILE);
        }

        MXFTrackReader *track_reader = 0;
        if (resolved_package->external_essence) {
            track_reader = GetExternalTrackReader(mp_source_clip, file_source_package);
            if (!track_reader) {
                log_warn("Skipping material package track %u because external source track could not be found\n",
                         mp_track_id);
                skipped_track_count++;
                continue;
            }

            // change external track's material package info to internal material package info
            MXFTrackInfo *track_info = track_reader->GetTrackInfo();
            track_info->material_package_uid  = mMaterialPackage->getPackageUID();
            track_info->material_track_id     = mp_track_id;
            track_info->material_track_number = mp_track->getTrackNumber();
            track_info->edit_rate             = normalize_rate(mp_track->getEditRate());
            track_info->duration              = mp_source_clip->getDuration();
            track_info->lead_filler_offset    = lead_filler_offset;
        } else {
            track_reader = CreateInternalTrackReader(partition, mp_track, mp_source_clip,
                                                     data_def, resolved_package);
            if (!track_reader) {
                log_warn("Skipping material package track %u\n", mp_track_id);
                skipped_track_count++;
                continue;
            }
            track_reader->GetTrackInfo()->lead_filler_offset = lead_filler_offset;
        }
        mTrackReaders.push_back(track_reader);

        // this material package track will be used to extract timecodes later on
        if (!infile_mp_track)
            infile_mp_track = mp_track;
    }
    if (mTrackReaders.empty()) {
        if (skipped_track_count > 0)
            log_warn("Skipped %u material package tracks whilst processing header metadata\n", skipped_track_count);
        THROW_RESULT(MXF_RESULT_NO_ESSENCE);
    }

    // order tracks by material track number / id
    stable_sort(mTrackReaders.begin(), mTrackReaders.end(), compare_track_reader);


    // extract start timecodes and physical source package name
    GetStartTimecodes(preface, infile_mp_track);


    // get the body and index SIDs linked to (singular) internal essence file source package
    if (!mInternalTrackReaders.empty()) {
        ContentStorage *content_storage = preface->getContentStorage();
        vector<EssenceContainerData*> ess_data;
        if (content_storage->haveEssenceContainerData())
            ess_data = content_storage->getEssenceContainerData();
        if (ess_data.empty()) {
            log_error("Missing EssenceContainerData set\n");
            THROW_RESULT(MXF_RESULT_NO_ESSENCE);
        }

        // only support single essence container (not OP-1B with multiple internal essence file source packages)
        if (ess_data.size() != 1) {
            log_error("Multiple essence containers is not supported\n");
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        }
        EssenceContainerData *single_ess_data = ess_data[0];

        mxfUMID linked_package_uid = single_ess_data->getLinkedPackageUID();
        for (i = 0; i < mInternalTrackReaders.size(); i++) {
            if (!mxf_equals_umid(&mInternalTrackReaders[i]->GetTrackInfo()->file_package_uid, &linked_package_uid)) {
                log_error("Essence container data LinkedPackageUID does not link to internal file source package\n");
                THROW_RESULT(MXF_RESULT_NO_ESSENCE);
            }
        }

        mBodySID = single_ess_data->getBodySID();
        if (mBodySID == 0) {
            log_error("BodySID is 0\n");
            THROW_RESULT(MXF_RESULT_NO_ESSENCE);
        }

        mIndexSID = 0;
        if (single_ess_data->haveIndexSID())
            mIndexSID = single_ess_data->getIndexSID();
        if (mIndexSID == 0)
            log_warn("Essence container has no index table (IndexSID is 0)\n");
    }

    // disable unused external tracks, i.e. external tracks not contained in mExternalTrackReaders / mTrackReaders
    for (i = 0; i < mExternalReaders.size(); i++) {
        size_t j;
        for (j = 0; j < mExternalReaders[i]->GetNumTrackReaders(); j++) {
            size_t k;
            for (k = 0; k < mExternalTrackReaders.size(); k++) {
                if (mExternalTrackReaders[k] == mExternalReaders[i]->GetTrackReader(j))
                    break;
            }
            if (k >= mExternalTrackReaders.size())
                mExternalReaders[i]->GetTrackReader(j)->SetEnable(false);
        }
    }

    // set the clip edit rate if required, i.e. when there are no internal essence tracks
    BMX_ASSERT(mEditRate.numerator != 0 || mInternalTrackReaders.empty());
    if (mEditRate.numerator == 0) {
        // the lowest external edit rate is the clip edit rate
        float lowest_edit_rate = 1000000.0;
        size_t i;
        for (i = 0; i < mTrackReaders.size(); i++) {
            float track_edit_rate = mTrackReaders[i]->GetEditRate().numerator /
                                        (float)mTrackReaders[i]->GetEditRate().denominator;
            if (track_edit_rate < lowest_edit_rate) {
                mEditRate = mTrackReaders[i]->GetEditRate();
                lowest_edit_rate = track_edit_rate;
            }
        }
        BMX_CHECK(mEditRate.numerator != 0);
    }

    // extract the external track sample sequences which are used to convert external positions / durations
    for (i = 0; i < mExternalReaders.size(); i++) {
        vector<uint32_t> sample_sequence;
        if (!get_sample_sequence(mEditRate, mExternalReaders[i]->GetEditRate(), &sample_sequence)) {
            mxfRational external_edit_rate = mExternalReaders[i]->GetEditRate();
            log_error("Externally referenced file's edit rate %d/%d is incompatible with clip edit rate %d/%d\n",
                      external_edit_rate.numerator, external_edit_rate.denominator,
                      mEditRate.numerator, mEditRate.denominator);
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        }

        mExternalSampleSequences.push_back(sample_sequence);
        mExternalSampleSequenceSizes.push_back(get_sequence_size(sample_sequence));
    }

    // determine the clip duration which is the minimum track duration or unknown (-1)
    // Note that OP-1A and 1B require the tracks to have equal duration
    mDuration = -2;
    for (i = 0; i < mInternalTrackReaders.size(); i++) {
        if (mInternalTrackReaders[i]->GetTrackInfo()->duration < 0) {
            mDuration = -1;
            break;
        }
        int64_t track_duration = convert_duration(mInternalTrackReaders[i]->GetTrackInfo()->edit_rate,
                                                  mInternalTrackReaders[i]->GetTrackInfo()->duration,
                                                  mEditRate,
                                                  ROUND_AUTO);
        if (mDuration == -2 || track_duration < mDuration)
            mDuration = track_duration;
    }
    if (mDuration != -1) {
        for (i = 0; i < mExternalReaders.size(); i++) {
            if (mExternalReaders[i]->GetDuration() < 0) {
                mDuration = -1;
                break;
            }
            int64_t internal_duration = CONVERT_EXTERNAL_DUR(mExternalReaders[i]->GetDuration());
            if (mDuration == -2 || internal_duration < mDuration)
                mDuration = internal_duration;
        }
    }

    // force external readers to have the clip's duration
    if (mDuration >= 0) {
        for (i = 0; i < mExternalReaders.size(); i++)
            mExternalReaders[i]->ForceDuration(CONVERT_INTERNAL_POS(mDuration));
    }

    // the clip origin is the maximum track origin, i.e. maximum file or external file origin
    mOrigin = mFileOrigin;
    for (i = 0; i < mExternalReaders.size(); i++) {
        int64_t external_origin = CONVERT_EXTERNAL_POS(mExternalReaders[i]->GetOrigin());
        if (external_origin > mOrigin)
            mOrigin = external_origin;
    }
}

MXFTrackReader* MXFFileReader::CreateInternalTrackReader(Partition *partition,
                                                         Track *mp_track, SourceClip *mp_source_clip,
                                                         MXFDataDefEnum data_def, const ResolvedPackage *resolved_package)
{
    SourcePackage *file_source_package = dynamic_cast<SourcePackage*>(resolved_package->package);
    BMX_CHECK(file_source_package);

    Track *fsp_track = dynamic_cast<Track*>(resolved_package->generic_track);
    BMX_CHECK(fsp_track);


    // set or check the clip edit rate

    Rational fsp_edit_rate = normalize_rate(fsp_track->getEditRate());
    if (mEditRate.numerator == 0) {
        mEditRate = fsp_edit_rate;
    } else if (mEditRate != fsp_edit_rate) {
        BMX_EXCEPTION(("FSP track edit rate %d/%d does not match existing edit rate %d/%d",
                       fsp_edit_rate.numerator, fsp_edit_rate.denominator,
                       mEditRate.numerator, mEditRate.denominator));
    }


    // get track origin (pre-charge)

    int64_t origin = fsp_track->getOrigin();
    if (origin < 0) {
        log_error("Negative track origin %"PRId64" in top-level file Source Package not supported\n", origin);
        THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
    }

    // Avid start position > 0 is equivalent to origin in the file source package
    if (mp_source_clip->getStartPosition() > 0) {
        origin += convert_position(normalize_rate(mp_track->getEditRate()),
                                   mp_source_clip->getStartPosition(),
                                   normalize_rate(fsp_track->getEditRate()),
                                   ROUND_AUTO);
    }

    if (!mInternalTrackReaders.empty() && origin != mFileOrigin) {
        log_error("Tracks with different track origins, %"PRId64" != %"PRId64", is not supported\n",
                  origin, mFileOrigin);
        THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
    }
    mFileOrigin = origin;


    // get the file descriptor

    GenericDescriptor *descriptor = file_source_package->getDescriptor();
    FileDescriptor *file_desc     = dynamic_cast<FileDescriptor*>(descriptor);
    MultipleDescriptor *mult_desc = dynamic_cast<MultipleDescriptor*>(descriptor);
    if (mult_desc) {
        file_desc = 0; // need to find it in the child descriptors

        BMX_CHECK(fsp_track->haveTrackID());
        uint32_t fsp_track_id = fsp_track->getTrackID();

        vector<GenericDescriptor*> child_descs = mult_desc->getSubDescriptorUIDs();
        size_t i;
        for (i = 0; i < child_descs.size(); i++) {
            FileDescriptor *child_file_desc = dynamic_cast<FileDescriptor*>(child_descs[i]);
            if (!child_file_desc || !child_file_desc->haveLinkedTrackID())
                continue;
            if (child_file_desc->getLinkedTrackID() == fsp_track_id) {
                file_desc = child_file_desc;
                break;
            }
        }
        if (!file_desc) {
            log_warn("Failed to find file descriptor for source package track %u\n", fsp_track_id);
            return 0;
        }
    }


    // fill in track info

    auto_ptr<MXFTrackInfo> track_info;
    MXFPictureTrackInfo *picture_track_info = 0;
    MXFSoundTrackInfo *sound_track_info = 0;
    MXFDataTrackInfo *data_track_info = 0;
    if (data_def == MXF_PICTURE_DDEF) {
        picture_track_info = new MXFPictureTrackInfo();
        track_info.reset(picture_track_info);
    } else if (data_def == MXF_SOUND_DDEF) {
        sound_track_info = new MXFSoundTrackInfo();
        track_info.reset(sound_track_info);
    } else {
        BMX_ASSERT(data_def == MXF_DATA_DDEF);
        data_track_info = new MXFDataTrackInfo();
        track_info.reset(data_track_info);
    }

    track_info->material_package_uid  = mMaterialPackage->getPackageUID();
    if (mp_track->haveTrackID())
        track_info->material_track_id = mp_track->getTrackID();
    track_info->material_track_number = mp_track->getTrackNumber();
    track_info->file_package_uid      = file_source_package->getPackageUID();
    track_info->edit_rate             = normalize_rate(mp_track->getEditRate());
    track_info->duration              = mp_source_clip->getDuration();
    if (fsp_track->haveTrackID())
        track_info->file_track_id     = fsp_track->getTrackID();
    track_info->file_track_number     = fsp_track->getTrackNumber();
    BMX_CHECK(track_info->file_track_number != 0);

    if (fsp_edit_rate != track_info->edit_rate) {
        log_warn("Unsupported FSP track edit rate %d/%d that does not equal MP track edit rate %d/%d\n",
                 fsp_edit_rate.numerator, fsp_edit_rate.denominator,
                 track_info->edit_rate.numerator, track_info->edit_rate.denominator);
    }

    // use the essence container label in the partition to workaround issue with Avid files where
    // the essence container label in the descriptor is a generic KLV label
    // Also workaround an issue with Blackmagic Design, DaVinci Resolve, 10.0b_lite,
    // Avid compatible MXF OP-Atom files where the essence container label in the partition pack
    // is set to the picture coding label
    if (mxf_is_op_atom(partition->getOperationalPattern())) {
        vector<mxfUL> ec_labels = partition->getEssenceContainers();
        if (ec_labels.size() == 1) {
            track_info->essence_container_label = ec_labels[0];
            GenericPictureEssenceDescriptor *picture_desc = dynamic_cast<GenericPictureEssenceDescriptor*>(file_desc);
            if (picture_desc && picture_desc->havePictureEssenceCoding()) {
                mxfUL pc_label = picture_desc->getPictureEssenceCoding();
                if (mxf_equals_ul(&track_info->essence_container_label, &pc_label)) {
                    log_error("Essence container label in the partition pack is set to the picture coding label\n");
                    // set to null so that this alternative essence container label is ignored
                    // in the MXFDescriptorHelper sub-classes
                    track_info->essence_container_label = g_Null_UL;
                }
            }
        }
    }

    if (data_def == MXF_PICTURE_DDEF)
        ProcessPictureDescriptor(file_desc, picture_track_info);
    else if (data_def == MXF_SOUND_DDEF)
        ProcessSoundDescriptor(file_desc, sound_track_info);
    else
        ProcessDataDescriptor(file_desc, data_track_info);


    MXFFileTrackReader *track_reader = new MXFFileTrackReader(this, mInternalTrackReaders.size(), track_info.get(),
                                                              file_desc, file_source_package);
    mInternalTrackReaders.push_back(track_reader);
    track_info.release();
    mInternalTrackReaderNumberMap[mInternalTrackReaders.back()->GetTrackInfo()->file_track_number] = track_reader;

    return mInternalTrackReaders.back();
}

MXFTrackReader* MXFFileReader::GetExternalTrackReader(SourceClip *mp_source_clip, SourcePackage *file_source_package)
{
    // resolve package using available locators
    GenericDescriptor *descriptor = file_source_package->getDescriptor();
    vector<Locator*> locators;
    if (descriptor->haveLocators())
        locators = descriptor->getLocators();
    vector<ResolvedPackage> resolved_packages = mPackageResolver->ResolveSourceClip(mp_source_clip, locators);
    if (resolved_packages.empty()) {
        log_warn("Failed to resolve external essence (SourcePackageID: %s, SourceTrackID: %u)\n",
                 get_umid_string(mp_source_clip->getSourcePackageID()).c_str(), mp_source_clip->getSourceTrackID());
        return 0;
    }

    // require external file to have internal essence
    const ResolvedPackage *resolved_package = 0;
    size_t i;
    for (i = 0; i < resolved_packages.size(); i++) {
        if (resolved_packages[i].is_file_source_package && !resolved_packages[i].external_essence) {
            resolved_package = &resolved_packages[i];
            break;
        }
    }
    if (!resolved_package) {
        log_warn("Failed to resolve external essence (SourcePackageID: %s, SourceTrackID: %u)\n",
                 get_umid_string(mp_source_clip->getSourcePackageID()).c_str(), mp_source_clip->getSourceTrackID());
        return 0;
    }

    MXFTrackReader *external_track_reader =
        resolved_package->file_reader->GetInternalTrackReaderById(resolved_package->track_id);
    if (!external_track_reader) {
        log_warn("Failed to resolve track in external essence (SourcePackageID: %s, SourceTrackID: %u)\n",
                 get_umid_string(mp_source_clip->getSourcePackageID()).c_str(), mp_source_clip->getSourceTrackID());
        return 0;
    }

    // don't support external tracks referenced by multiple material tracks
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i] == external_track_reader) {
            log_error("Tracks referenced by multiple material tracks is not supported\n");
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        }
    }

    // add external reader if not already present
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (mExternalReaders[i] == resolved_package->file_reader)
            break;
    }
    if (i >= mExternalReaders.size()) {
        resolved_package->file_reader->SetFileIndex(mFileIndex, false);
        mExternalReaders.push_back(resolved_package->file_reader);
    }

    mExternalTrackReaders.push_back(external_track_reader);
    return external_track_reader;
}

void MXFFileReader::GetStartTimecodes(Preface *preface, Track *infile_mp_track)
{
    Timecode start_timecode;
    GenericPackage *ref_package;
    Track *ref_track;
    int64_t ref_offset;

    // try get start timecodes from the material package, file source package and physical source package
    // also get the physical source package name

    if (GetStartTimecode(mMaterialPackage, 0, 0, &start_timecode))
        mMaterialStartTimecode = new Timecode(start_timecode);

    if (infile_mp_track) {
        if (GetReferencedPackage(preface, infile_mp_track, 0, FILE_SOURCE_PACKAGE_TYPE,
                                 &ref_package, &ref_track, &ref_offset))
        {
            if (GetStartTimecode(ref_package, ref_track, ref_offset, &start_timecode))
                mFileSourceStartTimecode = new Timecode(start_timecode);

            if (GetReferencedPackage(preface, ref_track, ref_offset, PHYSICAL_SOURCE_PACKAGE_TYPE,
                                     &ref_package, &ref_track, &ref_offset))
            {
                if (GetStartTimecode(ref_package, ref_track, ref_offset, &start_timecode))
                    mPhysicalSourceStartTimecode = new Timecode(start_timecode);

                if (ref_package->haveName())
                    mPhysicalSourcePackageName = ref_package->getName();
            }
        }
    }
}

bool MXFFileReader::GetStartTimecode(GenericPackage *package, Track *ref_track, int64_t offset, Timecode *timecode)
{
    // find the first track with a timecode component
    TimecodeComponent *tc_component = 0;
    vector<GenericTrack*> tracks = package->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *track = dynamic_cast<Track*>(tracks[i]);
        if (!track)
            continue;

        StructuralComponent *track_sequence = track->getSequence();
        mxfUL data_def_ul = track_sequence->getDataDefinition();
        if (!mxf_is_timecode(&data_def_ul))
            continue;

        Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
        tc_component = dynamic_cast<TimecodeComponent*>(track_sequence);
        if (sequence) {
            vector<StructuralComponent*> components = sequence->getStructuralComponents();
            size_t j;
            for (j = 0; j < components.size(); j++) {
                tc_component = dynamic_cast<TimecodeComponent*>(components[j]);
                if (tc_component)
                    break;
            }
        }
        if (tc_component)
            break;
    }
    if (!tc_component)
        return false;


    // the timecode offset is 0 or it is the offset in the referenced track converted to an offset in the timecode track
    BMX_ASSERT(offset == 0 || ref_track);
    int64_t tc_offset = offset;
    if (ref_track)
        tc_offset = convert_tc_offset(normalize_rate(ref_track->getEditRate()), offset,
                                      tc_component->getRoundedTimecodeBase());

    timecode->Init(tc_component->getRoundedTimecodeBase(),
                   tc_component->getDropFrame(),
                   tc_component->getStartTimecode() + tc_offset);
    return true;
}

bool MXFFileReader::GetReferencedPackage(Preface *preface, Track *track, int64_t offset_in, PackageType package_type,
                                         GenericPackage **ref_package_out, Track **ref_track_out,
                                         int64_t *ref_offset_out)
{
    // get the source clip
    StructuralComponent *track_sequence = track->getSequence();
    Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
    SourceClip *source_clip = dynamic_cast<SourceClip*>(track_sequence);
    if (sequence) {
        vector<StructuralComponent*> components = sequence->getStructuralComponents();
        size_t i;
        for (i = 0; i < components.size(); i++) {
            source_clip = dynamic_cast<SourceClip*>(components[i]);
            if (source_clip)
                break;
        }
    }
    if (!source_clip)
        return false;

    // find the referenced package and timeline track
    GenericPackage *ref_package = preface->findPackage(source_clip->getSourcePackageID());
    if (!ref_package)
        return false;
    GenericTrack *ref_generic_track = ref_package->findTrack(source_clip->getSourceTrackID());
    if (!ref_generic_track)
        return false;
    Track *ref_track = dynamic_cast<Track*>(ref_generic_track);
    if (!ref_track)
        return false;

    int64_t ref_offset = convert_position(normalize_rate(track->getEditRate()),
                                          source_clip->getStartPosition() + offset_in,
                                          normalize_rate(ref_track->getEditRate()),
                                          ROUND_AUTO);
    ref_offset += ref_track->getOrigin();

    // check the package type and try next referenced package if wrong type
    bool type_match = false;
    if (package_type == MATERIAL_PACKAGE_TYPE) {
        type_match = (dynamic_cast<MaterialPackage*>(ref_package) != 0);
    } else {
        SourcePackage *source_package = dynamic_cast<SourcePackage*>(ref_package);
        if (source_package && source_package->haveDescriptor()) {
            GenericDescriptor *descriptor = source_package->getDescriptorLight();
            if (descriptor) {
                if (package_type == FILE_SOURCE_PACKAGE_TYPE)
                    type_match = (dynamic_cast<FileDescriptor*>(descriptor) != 0);
                else
                    type_match = mDataModel->isSubclassOf(descriptor, &MXF_SET_K(PhysicalDescriptor));
            }
        }
    }
    if (!type_match) {
        if (ref_track == track && offset_in == ref_offset) {
            // avoid infinite recursion on malformed files
            log_warn("Track %d references itself\n", track->getTrackID());
            return false;
        }
        return GetReferencedPackage(preface, ref_track, ref_offset, package_type,
                                    ref_package_out, ref_track_out, ref_offset_out);
    }


    *ref_package_out = ref_package;
    *ref_track_out   = ref_track;
    *ref_offset_out  = ref_offset;

    return true;
}

void MXFFileReader::ProcessDescriptor(mxfpp::FileDescriptor *file_descriptor, MXFTrackInfo *track_info)
{
    track_info->essence_type = MXFDescriptorHelper::IsSupported(file_descriptor, track_info->essence_container_label);

    // set essence_container_label if not already set
    if (track_info->essence_container_label == g_Null_UL)
        track_info->essence_container_label = file_descriptor->getEssenceContainer();
}

void MXFFileReader::ProcessPictureDescriptor(FileDescriptor *file_descriptor, MXFPictureTrackInfo *picture_track_info)
{
    ProcessDescriptor(file_descriptor, picture_track_info);

    GenericPictureEssenceDescriptor *picture_descriptor =
        dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    BMX_CHECK(picture_descriptor);

    PictureMXFDescriptorHelper *picture_helper =
        PictureMXFDescriptorHelper::Create(file_descriptor, mMXFVersion, picture_track_info->essence_container_label);
    int32_t avid_resolution_id = 0;
    if (picture_helper->HaveAvidResolutionID())
        avid_resolution_id = picture_helper->GetAvidResolutionID();
    delete picture_helper;

    if (picture_descriptor->havePictureEssenceCoding())
        picture_track_info->picture_essence_coding_label = picture_descriptor->getPictureEssenceCoding();
    if (picture_descriptor->haveSignalStandard())
        picture_track_info->signal_standard = picture_descriptor->getSignalStandard();
    if (picture_descriptor->haveFrameLayout())
        picture_track_info->frame_layout = picture_descriptor->getFrameLayout();

    // fix legacy avid frame layout values for IEC DV-25, DVBased DV-25 and DVBased DV-50
    if ((avid_resolution_id == 0x8c || avid_resolution_id == 0x8d || avid_resolution_id == 0x8e) &&
        picture_track_info->frame_layout == MXF_MIXED_FIELDS)
    {
        picture_track_info->frame_layout = MXF_SEPARATE_FIELDS;
    }

    uint32_t frame_height_factor = 1;
    if (picture_track_info->frame_layout == MXF_SEPARATE_FIELDS)
        frame_height_factor = 2; // double the field height

    if (picture_descriptor->haveStoredWidth())
        picture_track_info->stored_width = picture_descriptor->getStoredWidth();
    if (picture_descriptor->haveStoredHeight())
        picture_track_info->stored_height = frame_height_factor * picture_descriptor->getStoredHeight();

    if (picture_descriptor->haveDisplayWidth())
        picture_track_info->display_width = picture_descriptor->getDisplayWidth();
    else
        picture_track_info->display_width = picture_track_info->stored_width;
    if (picture_descriptor->haveDisplayHeight())
        picture_track_info->display_height = frame_height_factor * picture_descriptor->getDisplayHeight();
    else
        picture_track_info->display_height = picture_track_info->stored_height;

    if (picture_descriptor->haveDisplayXOffset())
        picture_track_info->display_x_offset = picture_descriptor->getDisplayXOffset();
    if (picture_descriptor->haveDisplayYOffset())
        picture_track_info->display_y_offset = frame_height_factor * picture_descriptor->getDisplayYOffset();

    if (picture_descriptor->haveActiveFormatDescriptor()) {
        decode_afd(picture_descriptor->getActiveFormatDescriptor(), mMXFVersion, &picture_track_info->afd,
                   &picture_track_info->aspect_ratio);
    }
    if (picture_descriptor->haveAspectRatio())
        picture_track_info->aspect_ratio = picture_descriptor->getAspectRatio();


    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor);
    if (cdci_descriptor) {
        picture_track_info->is_cdci = true;
        if (cdci_descriptor->haveComponentDepth())
            picture_track_info->component_depth = cdci_descriptor->getComponentDepth();
        if (cdci_descriptor->haveHorizontalSubsampling())
            picture_track_info->horiz_subsampling = cdci_descriptor->getHorizontalSubsampling();
        if (cdci_descriptor->haveVerticalSubsampling())
            picture_track_info->vert_subsampling = cdci_descriptor->getVerticalSubsampling();
        if (cdci_descriptor->haveColorSiting())
            picture_track_info->color_siting = cdci_descriptor->getColorSiting();

        // fix legacy avid subsampling values for DVBased DV-25
        if (avid_resolution_id == 0x8c &&
            picture_track_info->horiz_subsampling == picture_track_info->vert_subsampling)
        {
            picture_track_info->horiz_subsampling = 4;
            picture_track_info->vert_subsampling = 1;
        }
    } else {
        picture_track_info->is_cdci = false;
    }
}

void MXFFileReader::ProcessSoundDescriptor(FileDescriptor *file_descriptor, MXFSoundTrackInfo *sound_track_info)
{
    ProcessDescriptor(file_descriptor, sound_track_info);

    GenericSoundEssenceDescriptor *sound_descriptor =
        dynamic_cast<GenericSoundEssenceDescriptor*>(file_descriptor);
    BMX_CHECK(sound_descriptor);

    if (sound_descriptor->haveAudioSamplingRate())
        sound_track_info->sampling_rate = normalize_rate(sound_descriptor->getAudioSamplingRate());

    if (sound_descriptor->haveChannelCount())
        sound_track_info->channel_count = sound_descriptor->getChannelCount();

    if (sound_descriptor->haveQuantizationBits())
        sound_track_info->bits_per_sample = sound_descriptor->getQuantizationBits();

    if (sound_descriptor->haveLocked()) {
        sound_track_info->locked = sound_descriptor->getLocked();
        sound_track_info->locked_set = true;
    }
    if (sound_descriptor->haveAudioRefLevel()) {
        sound_track_info->audio_ref_level = sound_descriptor->getAudioRefLevel();
        sound_track_info->audio_ref_level_set = true;
    }
    if (sound_descriptor->haveDialNorm()) {
        sound_track_info->dial_norm = sound_descriptor->getDialNorm();
        sound_track_info->dial_norm_set = true;
    }

    WaveAudioDescriptor *wave_descriptor = dynamic_cast<WaveAudioDescriptor*>(file_descriptor);
    if (wave_descriptor) {
        sound_track_info->block_align = wave_descriptor->getBlockAlign();
        if (wave_descriptor->haveSequenceOffset())
            sound_track_info->sequence_offset = wave_descriptor->getSequenceOffset();
    } else {
        if (sound_track_info->channel_count > 0) {
            sound_track_info->block_align = sound_track_info->channel_count *
                                                (uint16_t)((sound_track_info->bits_per_sample + 7) / 8);
        } else {
            // assuming channel count 1 is better than block align 0
            sound_track_info->block_align = (sound_track_info->bits_per_sample + 7) / 8;
        }
    }
}

void MXFFileReader::ProcessDataDescriptor(FileDescriptor *file_descriptor, MXFDataTrackInfo *data_track_info)
{
    ProcessDescriptor(file_descriptor, data_track_info);
}

MXFTrackReader* MXFFileReader::GetInternalTrackReader(size_t index) const
{
    BMX_CHECK(index < mInternalTrackReaders.size());
    return mInternalTrackReaders[index];
}

MXFTrackReader* MXFFileReader::GetInternalTrackReaderByNumber(uint32_t track_number) const
{
    map<uint32_t, MXFTrackReader*>::const_iterator result = mInternalTrackReaderNumberMap.find(track_number);
    if (result == mInternalTrackReaderNumberMap.end())
        return 0;

    return result->second;
}

MXFTrackReader* MXFFileReader::GetInternalTrackReaderById(uint32_t id) const
{
    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++) {
        if (mInternalTrackReaders[i]->GetTrackInfo()->file_track_id == id)
            return mInternalTrackReaders[i];
    }

    return 0;
}

void MXFFileReader::ForceDuration(int64_t duration)
{
    BMX_CHECK(duration <= mDuration);
    mDuration = duration;
}

bool MXFFileReader::GetInternalIndexEntry(MXFIndexEntryExt *entry, int64_t position) const
{
    BMX_ASSERT(mEssenceReader);

    return mEssenceReader->GetIndexEntry(entry, TO_ESS_READER_POS(position));
}

int16_t MXFFileReader::GetInternalPrecharge(int64_t position, bool limit_to_available) const
{
    CHECK_SUPPORT_PC_RO_INFO;
    BMX_ASSERT(mEssenceReader);

    if (!HaveInterFrameEncodingTrack())
        return 0;

    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    // no precharge if target position outside essence range
    if (FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(TO_ESS_READER_POS(target_position))) != target_position)
        return 0;

    int16_t precharge = 0;
    MXFIndexEntryExt index_entry;
    if (GetInternalIndexEntry(&index_entry, target_position)) {
        int8_t target_index_entry_offset = index_entry.temporal_offset;
        if (target_index_entry_offset != 0) {
            if (GetInternalIndexEntry(&index_entry, target_position + target_index_entry_offset))
                precharge = target_index_entry_offset + index_entry.key_frame_offset;
        } else {
            precharge = index_entry.key_frame_offset;
        }
    }

    if (precharge > 0) {
        log_warn("Unexpected positive precharge value %d\n", precharge);
    } else if (precharge < 0 && limit_to_available) {
        precharge = (int16_t)(FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(
                                TO_ESS_READER_POS(target_position + precharge))) - target_position);
    }

    return precharge < 0 ? precharge : 0;
}

int16_t MXFFileReader::GetInternalRollout(int64_t position, bool limit_to_available) const
{
    CHECK_SUPPORT_PC_RO_INFO;
    BMX_ASSERT(mEssenceReader);

    if (!HaveInterFrameEncodingTrack())
        return 0;

    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    // no rollout if target position outside essence range
    if (FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(TO_ESS_READER_POS(target_position))) != target_position)
        return 0;

    int16_t rollout = 0;
    MXFIndexEntryExt index_entry;
    if (GetInternalIndexEntry(&index_entry, target_position) && index_entry.temporal_offset > 0)
        rollout = index_entry.temporal_offset;

    if (rollout < 0) {
        log_warn("Unexpected negative rollout value %d\n", rollout);
    } else if (rollout > 0 && limit_to_available) {
        rollout = (int16_t)(FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(
                                TO_ESS_READER_POS(target_position + rollout))) - target_position);
    }

    return rollout > 0 ? rollout : 0;
}

void MXFFileReader::GetInternalAvailableReadLimits(int64_t *start_position, int64_t *duration) const
{
    CHECK_SUPPORT_PC_RO_INFO;

    int16_t precharge = GetInternalPrecharge(0, true);
    int16_t rollout   = GetInternalRollout(mDuration - 1, true);

    *start_position = 0 + precharge;
    *duration       = - precharge + mDuration + rollout;
}

bool MXFFileReader::InternalIsEnabled() const
{
    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++) {
        if (mInternalTrackReaders[i]->IsEnabled())
            return true;
    }

    return false;
}

bool MXFFileReader::HaveInterFrameEncodingTrack() const
{
    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++) {
        if (mInternalTrackReaders[i]->IsEnabled()) {
            EssenceType essence_type = mInternalTrackReaders[i]->GetTrackInfo()->essence_type;
            if (essence_type == MPEG2LG_422P_HL_1080I ||
                essence_type == MPEG2LG_422P_HL_1080P ||
                essence_type == MPEG2LG_422P_HL_720P ||
                essence_type == MPEG2LG_MP_HL_1920_1080I ||
                essence_type == MPEG2LG_MP_HL_1920_1080P ||
                essence_type == MPEG2LG_MP_HL_1440_1080I ||
                essence_type == MPEG2LG_MP_HL_1440_1080P ||
                essence_type == MPEG2LG_MP_HL_720P ||
                essence_type == MPEG2LG_MP_H14_1080I ||
                essence_type == MPEG2LG_MP_H14_1080P)
            {
                return true;
            }
        }
    }

    return false;
}

void MXFFileReader::CheckRequireFrameInfo()
{
    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++) {
        if (mInternalTrackReaders[i]->IsEnabled()) {
            MXFTrackInfo *track_info = mInternalTrackReaders[i]->GetTrackInfo();
            if (track_info->essence_type == D10_AES3_PCM ||
                track_info->essence_type == AVCI200_1080I ||
                track_info->essence_type == AVCI200_1080P ||
                track_info->essence_type == AVCI200_720P ||
                track_info->essence_type == AVCI100_1080I ||
                track_info->essence_type == AVCI100_1080P ||
                track_info->essence_type == AVCI100_720P ||
                track_info->essence_type == AVCI50_1080I ||
                track_info->essence_type == AVCI50_1080P ||
                track_info->essence_type == AVCI50_720P)
            {
                if (mRequireFrameInfoCount < 1)
                    mRequireFrameInfoCount = 1;
            }
            else if (track_info->essence_type == VBI_DATA ||
                     track_info->essence_type == ANC_DATA)
            {
                if (mRequireFrameInfoCount < mST436ManifestCount)
                    mRequireFrameInfoCount = mST436ManifestCount;
            }
        }
    }
}

void MXFFileReader::ExtractFrameInfo()
{
    int64_t ess_reader_pos = mEssenceReader->GetPosition();

    SetTemporaryFrameBuffer(true);
    mEssenceReader->Seek(0);

    bool have_first = false;
    Frame *frame = 0;
    try
    {
        size_t i;
        for (i = 0; i < mInternalTrackReaders.size(); i++) {
            MXFDataTrackInfo *data_info = dynamic_cast<MXFDataTrackInfo*>(mInternalTrackReaders[i]->GetTrackInfo());
            if (data_info) {
                data_info->vbi_manifest.clear();
                data_info->anc_manifest.clear();
            }
        }

        uint32_t f;
        for (f = 0; f < mRequireFrameInfoCount; f++) {
            if (mEssenceReader->Read(1) != 1)
                throw true;

            AVCIEssenceParser avci_parser;
            for (i = 0; i < mInternalTrackReaders.size(); i++) {
                Frame *frame = mInternalTrackReaders[i]->GetFrameBuffer()->GetLastFrame(true);
                if (!frame || frame->IsEmpty()) {
                    delete frame;
                    frame = 0;
                    continue;
                }

                MXFTrackInfo *track_info = mInternalTrackReaders[i]->GetTrackInfo();
                MXFPictureTrackInfo *picture_info = dynamic_cast<MXFPictureTrackInfo*>(track_info);
                MXFSoundTrackInfo *sound_info = dynamic_cast<MXFSoundTrackInfo*>(track_info);
                MXFDataTrackInfo *data_info = dynamic_cast<MXFDataTrackInfo*>(track_info);

                if (f == 0 && track_info->essence_type == D10_AES3_PCM)
                {
                    if (frame->GetSize() >= 4)
                        sound_info->d10_aes3_valid_flags = frame->GetBytes()[3];
                }
                else if (f == 0 &&
                            (track_info->essence_type == AVCI200_1080I ||
                             track_info->essence_type == AVCI200_1080P ||
                             track_info->essence_type == AVCI200_720P ||
                             track_info->essence_type == AVCI100_1080I ||
                             track_info->essence_type == AVCI100_1080P ||
                             track_info->essence_type == AVCI100_720P ||
                             track_info->essence_type == AVCI50_1080I ||
                             track_info->essence_type == AVCI50_1080P ||
                             track_info->essence_type == AVCI50_720P))
                {
                    avci_parser.ParseFrameInfo(frame->GetBytes(), frame->GetSize());
                    picture_info->have_avci_header = avci_parser.HaveSequenceParameterSet();
                    if (picture_info->have_avci_header) {
                        dynamic_cast<MXFFileTrackReader*>(mInternalTrackReaders[i])->SetAVCIHeader(
                            frame->GetBytes(), frame->GetSize());
                    } else {
                        log_warn("First frame in AVC-Intra track does not have sequence and picture parameter sets\n");
                    }
                }
                else if (track_info->essence_type == VBI_DATA ||
                         track_info->essence_type == ANC_DATA)
                {
                    ST436Element element(track_info->essence_type == VBI_DATA);
                    element.Parse(frame->GetBytes(), frame->GetSize());

                    if (track_info->essence_type == VBI_DATA) {
                        size_t i;
                        for (i = 0; i < element.lines.size(); i++) {
                            VBIManifestElement manifest_element;
                            manifest_element.Parse(&element.lines[i]);
                            data_info->AppendUniqueVBIElement(manifest_element);
                        }
                    } else {
                        size_t i;
                        for (i = 0; i < element.lines.size(); i++) {
                            ANCManifestElement manifest_element;
                            manifest_element.Parse(&element.lines[i]);
                            data_info->AppendUniqueANCElement(manifest_element);
                        }
                    }
                }

                delete frame;
                frame = 0;
                have_first = true;
            }
        }

        mRequireFrameInfoCount = 0;
    }
    catch (const bool &ex)
    {
        if (ex) {
            log_warn("Reached the end of the essence data whilst extracting information\n");
            if (have_first) // good enough to continue
                mRequireFrameInfoCount = 0;
        }
        delete frame;
    }
    catch (...)
    {
        delete frame;
    }

    SetTemporaryFrameBuffer(false);
    mEssenceReader->Seek(ess_reader_pos);
}

void MXFFileReader::StartRead()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->StartRead();
    }
}

void MXFFileReader::CompleteRead()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->CompleteRead();
    }
}

void MXFFileReader::AbortRead()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->AbortRead();
    }
}

