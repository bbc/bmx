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

#include <cstring>
#include <cstdio>

#include <memory>

#include <bmx/mxf_reader/EssenceReader.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>
#include <bmx/mxf_helper/SoundMXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



EssenceReader::EssenceReader(MXFFileReader *file_reader, bool file_is_complete)
: mEssenceChunkHelper(file_reader), mIndexTableHelper(file_reader)
{
    mFileReader = file_reader;
    mFile = file_reader->mFile;
    mFileIsComplete = file_is_complete;
    mFrameMetadataReader = new FrameMetadataReader(file_reader);
    mReadStartPosition = 0;
    mReadDuration = 0;
    mPosition = 0;
    mImageStartOffset = 0;
    mImageEndOffset = 0;
    mBasePosition = -1;
    mFilePosition = -1;
    mNextKey = g_Null_Key;
    mNextLLen = 0;
    mNextLen = 0;
    mAtCPStart = false;
    mEssenceStartKey = g_Null_Key;
    mLastKnownFilePosition = -1;
    mLastKnownBasePosition = -1;
    mHaveFooter = file_is_complete;
    mBaseReadError = false;


    // get ImageStartOffset and ImageEndOffset properties which are used in Avid uncompressed files
    if (mFileReader->GetInternalTrackReader(0)->GetTrackInfo()->data_def == MXF_PICTURE_DDEF) {
        auto_ptr<MXFDescriptorHelper> helper(MXFDescriptorHelper::Create(
            mFileReader->GetInternalTrackReader(0)->GetFileDescriptor(),
            mFileReader->GetMXFVersion(),
            mFileReader->GetInternalTrackReader(0)->GetTrackInfo()->essence_container_label));
        PictureMXFDescriptorHelper *picture_helper = dynamic_cast<PictureMXFDescriptorHelper*>(helper.get());

        uint32_t alignment = picture_helper->GetImageAlignmentOffset();
        mImageStartOffset = picture_helper->GetImageStartOffset();
        mImageEndOffset = picture_helper->GetImageEndOffset();
        if (alignment > 1 && mImageStartOffset == 0 && mImageEndOffset == 0) {
            // Avid uncompressed Alpha file was found to have ImageAlignmentOffset set to 8192 but
            // the ImageEndOffset property was not set
            mImageEndOffset = (alignment - (picture_helper->GetSampleSize() % alignment)) % alignment;
            if (mImageEndOffset != 0) {
                log_warn("File with a non-zero ImageAlignmentOffset is missing a non-zero "
                         "ImageStartOffset or ImageEndOffset. Assuming ImageEndOffset %u\n",
                         mImageEndOffset);
            }
        }
    }

    // if file is complete then read the index table segments, essence container layout and
    // determine the essence wrapping type
    if (file_is_complete) {
        if (mFileReader->mIndexSID)
            mIndexTableHelper.ExtractIndexTable();

        // first edit unit size is used to determine the essence wrapping type
        int64_t first_edit_unit_size = 0;
        if (mIndexTableHelper.HaveEditUnitSize(0)) {
            int64_t offset;
            mIndexTableHelper.GetEditUnit(0, &offset, &first_edit_unit_size);
        }
        mEssenceChunkHelper.CreateEssenceChunkIndex(first_edit_unit_size);
        BMX_ASSERT(mEssenceChunkHelper.IsComplete());

        // if the essence wrapping type still unknown then go with the guessed type
        if (mFileReader->mWrappingType == MXF_UNKNOWN_WRAPPING_TYPE)
            mFileReader->mWrappingType = mFileReader->mGuessedWrappingType;

        if (mIndexTableHelper.IsComplete()) {
            BMX_CHECK(mIndexTableHelper.GetEditRate() == mFileReader->GetEditRate());
            mIndexTableHelper.SetEssenceDataSize(mEssenceChunkHelper.GetEssenceDataSize());
        } else {
            if (mEssenceChunkHelper.GetEssenceDataSize() == 0) {
                // mark index table as complete if there is no essence data to index
                mIndexTableHelper.SetEditRate(mFileReader->GetEditRate());
                mIndexTableHelper.SetIsComplete();
            } else if (mFileReader->mIndexSID) {
                log_warn("Missing index table segments (IndexSID %u) for essence data with size %"PRId64"\n",
                         mFileReader->mIndexSID, mEssenceChunkHelper.GetEssenceDataSize());
            }
        }
    } else if (mFileReader->mWrappingType == MXF_UNKNOWN_WRAPPING_TYPE) {
        mFileReader->mWrappingType = mFileReader->mGuessedWrappingType;
    }

    // if index table is empty (incomplete) then fill in some known index table information
    if (!mIndexTableHelper.IsComplete()) {
        mIndexTableHelper.SetEditRate(mFileReader->GetEditRate());

        // require a known constant edit unit size for clip wrapped essence
        // TODO: support clip wrapped essence with variable or unknown edit unit size using essence parsers
        if (mFileReader->IsClipWrapped()) {
            uint32_t edit_unit_size = GetConstantEditUnitSize();
            if (edit_unit_size > 0)
                mIndexTableHelper.SetConstantEditUnitSize(mFileReader->GetEditRate(), edit_unit_size);
            else
                log_warn("Failed to set a constant edit unit size for clip wrapped essence data\n");
        }

        if (mEssenceChunkHelper.IsComplete())
            mIndexTableHelper.SetEssenceDataSize(mEssenceChunkHelper.GetEssenceDataSize());
    }

    // if essence container layout and index table are complete then check
    // that the last indexed edit unit is available in the essence container
    if (mEssenceChunkHelper.IsComplete() &&
        mIndexTableHelper.IsComplete() &&
        mIndexTableHelper.GetDuration() > 0)
    {
        int64_t last_unit_offset, last_unit_size;
        mIndexTableHelper.GetEditUnit(mIndexTableHelper.GetDuration() - 1, &last_unit_offset, &last_unit_size);
        if (mEssenceChunkHelper.GetEssenceDataSize() < last_unit_offset + last_unit_size) {
            BMX_EXCEPTION(("Last edit unit (offset %"PRId64", size %"PRId64") not available in "
                                "essence container (size %"PRId64")",
                          last_unit_offset, last_unit_size, mEssenceChunkHelper.GetEssenceDataSize()));
        }
    }


    // set read limits
    mReadStartPosition = 0;
    if (mIndexTableHelper.IsComplete())
        mReadDuration = mIndexTableHelper.GetDuration();
    else
        mReadDuration = INT64_MAX;
}

EssenceReader::~EssenceReader()
{
    size_t i;
    for (i = 0; i < mTrackFrames.size(); i++)
        delete mTrackFrames[i];
    delete mFrameMetadataReader;
}

void EssenceReader::SetReadLimits(int64_t start_position, int64_t duration)
{
    if (mIndexTableHelper.IsComplete()) {
        mReadStartPosition = LegitimisePosition(start_position);
        if (duration <= 0 || mIndexTableHelper.GetDuration() == 0)
            mReadDuration = 0;
        else
            mReadDuration = LegitimisePosition(start_position + duration - 1) - mReadStartPosition + 1;
    } else {
        if (start_position < 0)
            mReadStartPosition = 0;
        else
            mReadStartPosition = start_position;
        if (duration < 0)
            mReadDuration = 0;
        else
            mReadDuration = duration;
    }
}

uint32_t EssenceReader::Read(uint32_t num_samples)
{
    // init track frames
    size_t i;
    for (i = 0; i < mTrackFrames.size(); i++)
        delete mTrackFrames[i];
    mTrackFrames.clear();
    mFrameMetadataReader->Reset();
    for (i = 0; i < mFileReader->GetNumInternalTrackReaders(); i++) {
        if (mFileReader->GetInternalTrackReader(i)->IsEnabled()) {
            mTrackFrames.push_back(mFileReader->GetInternalTrackReader(i)->GetFrameBuffer()->CreateFrame());
            mTrackFrames.back()->request_num_samples = num_samples;
        } else {
            mTrackFrames.push_back(0);
        }
    }

    uint32_t actual_read_num_samples = 0;
    int64_t end_position = mPosition + num_samples;

    // read samples if within read limits
    if (mReadDuration > 0 &&
        mPosition < mReadStartPosition + mReadDuration &&
        end_position > 0)
    {
        // adjust sample count and seek to start of data if needed
        uint32_t first_sample_offset = 0;
        uint32_t read_num_samples = num_samples;
        if (mPosition < 0) {
            first_sample_offset = (uint32_t)(-mPosition);
            read_num_samples -= first_sample_offset;
            Seek(0);
        }
        if (mPosition + read_num_samples > mReadStartPosition + mReadDuration)
            read_num_samples -= (uint32_t)(mPosition + read_num_samples - (mReadStartPosition + mReadDuration));
        BMX_ASSERT(read_num_samples > 0);

        // read the samples
        int64_t start_position = mPosition;
        if (mFileReader->IsClipWrapped())
            actual_read_num_samples = ReadClipWrappedSamples(read_num_samples);
        else
            actual_read_num_samples = ReadFrameWrappedSamples(read_num_samples);

        // add frame metadata and information associated with first sample in frame
        int64_t essence_offset = 0;
        int8_t temporal_offset = 0;
        int8_t key_frame_offset = 0;
        uint8_t flags = 0;
        if (mIndexTableHelper.HaveEditUnit(start_position)) {
            mIndexTableHelper.GetEditUnit(start_position, &temporal_offset, &key_frame_offset,
                                          &flags, &essence_offset);
        }
        Frame *frame;
        for (i = 0; i < mFileReader->GetNumInternalTrackReaders(); i++) {
            frame = mTrackFrames[i];
            if (frame) {
                frame->first_sample_offset = first_sample_offset;
                frame->temporal_offset     = temporal_offset;
                frame->key_frame_offset    = key_frame_offset;
                frame->flags               = flags;

                mFrameMetadataReader->InsertFrameMetadata(frame,
                    mFileReader->GetInternalTrackReader(i)->GetTrackInfo()->file_track_number);
            }
        }
    }

    // push frames
    for (i = 0; i < mFileReader->GetNumInternalTrackReaders(); i++) {
        if (mTrackFrames[i]) {
            mFileReader->GetInternalTrackReader(i)->GetFrameBuffer()->PushFrame(mTrackFrames[i]);
            mTrackFrames[i] = 0;
        }
    }

    // always be positioned num_samples after previous position
    if (mPosition != end_position)
        Seek(end_position);

    return actual_read_num_samples;
}

void EssenceReader::Seek(int64_t position)
{
    mPosition = position;

    if (position >= mReadStartPosition && position < mReadStartPosition + mReadDuration)
        SeekEssence(position, false);
}

bool EssenceReader::GetIndexEntry(MXFIndexEntryExt *entry, int64_t position)
{
    if (mIndexTableHelper.GetIndexEntry(entry, position)) {
        mxfKey element_key;
        mEssenceChunkHelper.GetKeyAndFilePosition(entry->container_offset, entry->edit_unit_size, &element_key,
                                                  &entry->file_offset);
        return true;
    }

    return false;
}

int64_t EssenceReader::LegitimisePosition(int64_t position)
{
    if (position < 0 || mIndexTableHelper.GetDuration() == 0)
        return 0;
    else if (position >= mIndexTableHelper.GetDuration())
        return mIndexTableHelper.GetDuration() - 1;
    else
        return position;
}

bool EssenceReader::IsComplete() const
{
    return mEssenceChunkHelper.IsComplete() && mIndexTableHelper.IsComplete();
}

uint32_t EssenceReader::ReadClipWrappedSamples(uint32_t num_samples)
{
    // for incomplete clip wrapped files only support seeking to position 0
    if (!IsComplete() && mPosition == 0 && !SeekEssence(mPosition, true))
        return 0;

    Frame *frame = mTrackFrames[0];

    int64_t current_file_position = mFile->tell();
    uint32_t total_num_samples = 0;
    while (total_num_samples < num_samples) {
        // get maximum number of contiguous samples that can be read in one go
        uint32_t num_cont_samples;
        int64_t file_position, size;
        mxfKey element_key;
        if (mImageStartOffset || mImageEndOffset) {
            GetEditUnitGroup(mPosition, 1, &element_key, &file_position, &size, &num_cont_samples);
        } else {
            GetEditUnitGroup(mPosition, num_samples - total_num_samples, &element_key, &file_position, &size,
                             &num_cont_samples);
        }

        if (frame) {
            BMX_CHECK(size >= mImageStartOffset + mImageEndOffset);

            if (current_file_position != file_position)
                mFile->seek(file_position, SEEK_SET);
            current_file_position = file_position;

            BMX_CHECK(size <= UINT32_MAX);
            frame->Grow((uint32_t)size);
            uint32_t num_read = mFile->read(frame->GetBytesAvailable(), (uint32_t)size);
            current_file_position += num_read;
            BMX_CHECK(num_read == size);

            size -= mImageEndOffset;
            if (mImageStartOffset > 0) {
                memmove(frame->GetBytesAvailable(),
                        frame->GetBytesAvailable() + mImageStartOffset,
                        (uint32_t)(size - mImageStartOffset));
                size -= mImageStartOffset;
            }

            if (frame->IsEmpty()) {
                frame->ec_position         = mPosition;
                frame->temporal_reordering = mIndexTableHelper.GetTemporalReordering(0);
                frame->cp_file_position    = current_file_position - mImageEndOffset - size;
                frame->file_position       = frame->cp_file_position;
                frame->file_id             = mFileReader->GetFileId();
                frame->element_key         = element_key;
            }

            frame->IncrementSize((uint32_t)size);
            frame->num_samples += num_cont_samples;
        } else {
            mFile->seek(file_position + size, SEEK_SET);
            current_file_position = file_position + size;
        }

        mPosition += num_cont_samples;
        total_num_samples += num_cont_samples;
    }

    BMX_ASSERT(total_num_samples == num_samples);
    return num_samples;
}

uint32_t EssenceReader::ReadFrameWrappedSamples(uint32_t num_samples)
{
    int64_t start_position = mPosition;

    map<uint32_t, MXFTrackReader*> enabled_track_readers;
    uint32_t i;
    for (i = 0; i < num_samples; i++) {
        int64_t cp_file_position;
        int64_t size;
        if (!SeekEssence(mPosition, true))
            return i;
        if (mIndexTableHelper.HaveEditUnitSize(mPosition)) {
            mxfKey dummy_key = g_Null_Key;
            GetEditUnit(mPosition, &dummy_key, &cp_file_position, &size);
            BMX_ASSERT(cp_file_position == mFilePosition);
        } else if (mIndexTableHelper.HaveEditUnitOffset(mPosition)) {
            size = 0;
            cp_file_position = mEssenceChunkHelper.GetFilePosition(mIndexTableHelper.GetEditUnitOffset(mPosition));
            BMX_ASSERT(cp_file_position == mFilePosition);
        } else {
            size = 0;
            cp_file_position = mFilePosition;
        }

        mxfKey key;
        uint8_t llen;
        uint64_t len;
        int64_t cp_num_read = 0;
        while ((size == 0 || cp_num_read < size) &&
               ReadEssenceKL(cp_num_read == 0, &key, &llen, &len))
        {
            cp_num_read += mxfKey_extlen + llen;

            bool processed_metadata = mFrameMetadataReader->ProcessFrameMetadata(&key, len);

            if (!processed_metadata && (mxf_is_gc_essence_element(&key) || mxf_avid_is_essence_element(&key))) {
                uint32_t track_number = mxf_get_track_number(&key);
                MXFTrackReader *track_reader = 0;
                Frame *frame = 0;
                if (enabled_track_readers.find(track_number) == enabled_track_readers.end()) {
                    // frame does not yet exist - create it if track is enabled
                    track_reader = mFileReader->GetInternalTrackReaderByNumber(track_number);
                    if (start_position == mPosition && track_reader && track_reader->IsEnabled()) {
                        frame = mTrackFrames[track_reader->GetTrackIndex()];

                        BMX_CHECK(cp_num_read <= UINT32_MAX);

                        frame->ec_position         = start_position;
                        frame->cp_file_position    = cp_file_position;
                        frame->file_position       = cp_file_position + cp_num_read - (mxfKey_extlen + llen);
                        frame->kl_size             = mxfKey_extlen + llen;
                        frame->file_id             = mFileReader->GetFileId();
                        frame->element_key         = key;
                        if (mIndexTableHelper.HaveEditUnit(start_position)) {
                            frame->temporal_reordering =
                                mIndexTableHelper.GetTemporalReordering((uint32_t)(cp_num_read - (mxfKey_extlen + llen)));
                        }

                        enabled_track_readers[track_number] = track_reader;
                    } else {
                        enabled_track_readers[track_number] = 0;
                    }
                } else {
                    // frame exists if track is enabled - get it
                    track_reader = enabled_track_readers[track_number];
                    if (track_reader)
                        frame = mTrackFrames[track_reader->GetTrackIndex()];
                }

                if (frame) {
                    BMX_CHECK(len <= UINT32_MAX);

                    frame->Grow((uint32_t)len);
                    uint32_t num_read = mFile->read(frame->GetBytesAvailable(), (uint32_t)len);
                    BMX_CHECK(num_read == len);
                    frame->IncrementSize((uint32_t)len);
                    frame->num_samples++;
                } else {
                    mFile->skip(len);
                }
            } else if (!processed_metadata) {
                mFile->skip(len);
            }

            cp_num_read += len;
        }
        if (size != 0 && cp_num_read != size) {
           BMX_EXCEPTION(("Read content package size (0x%"PRIx64") does not match size in index (0x%"PRIx64") "
                          "at file position 0x%"PRIx64,
                          cp_num_read, size, mFileReader->mFile->tell()));
        }

        if (size == 0) {
            mIndexTableHelper.UpdateIndex(mPosition, mEssenceChunkHelper.GetEssenceOffset(cp_file_position),
                                          cp_num_read);
        }

        mPosition++;
    }

    return num_samples;
}

void EssenceReader::GetEditUnit(int64_t position, mxfKey *element_key, int64_t *file_position, int64_t *size)
{
    int64_t essence_offset, essence_size;
    mIndexTableHelper.GetEditUnit(position, &essence_offset, &essence_size);

    mEssenceChunkHelper.GetKeyAndFilePosition(essence_offset, essence_size, element_key, file_position);
    *size = essence_size;
}

void EssenceReader::GetEditUnitGroup(int64_t position, uint32_t max_samples, mxfKey *element_key, int64_t *file_position,
                                     int64_t *size, uint32_t *num_samples)
{
    BMX_CHECK(max_samples > 0);

    if (!mIndexTableHelper.HaveConstantEditUnitSize() || max_samples == 1) {
        GetEditUnit(position, element_key, file_position, size);
        *num_samples = 1;
        return;
    }

    int64_t first_file_position;
    int64_t first_size;
    mxfKey first_element_key;
    GetEditUnit(position, &first_element_key, &first_file_position, &first_size);

    // binary search to find number of contiguous edit units
    // first <= left <= right <= last
    // first to left is contiguous
    uint32_t left_num_samples = 1;
    uint32_t right_num_samples = max_samples;
    uint32_t last_num_samples = max_samples;

    int64_t right_file_position;
    int64_t right_size;
    mxfKey right_element_key;
    while (right_num_samples != left_num_samples) {
        GetEditUnit(position + right_num_samples - 1, &right_element_key, &right_file_position, &right_size);
        BMX_CHECK(right_size == mIndexTableHelper.GetEditUnitSize());

        if (right_file_position > first_file_position + mIndexTableHelper.GetEditUnitSize() * (right_num_samples - 1)) {
            // first to right is not contiguous - try halfway between left and right (round down)
            last_num_samples = right_num_samples;
            right_num_samples = (left_num_samples + right_num_samples) / 2;
        } else {
            BMX_CHECK(right_file_position == first_file_position + mIndexTableHelper.GetEditUnitSize() * (right_num_samples - 1));
            // first to right is contiguous - try halfway between right and last (round up)
            left_num_samples = right_num_samples;
            right_num_samples = (right_num_samples + last_num_samples + 1) / 2;
        }
    }

    *element_key   = first_element_key;
    *file_position = first_file_position;
    *size          = first_size * left_num_samples;
    *num_samples   = left_num_samples;
}

uint32_t EssenceReader::GetConstantEditUnitSize()
{
    BMX_ASSERT(mFileReader->GetNumInternalTrackReaders() == 1);
    auto_ptr<MXFDescriptorHelper> helper(MXFDescriptorHelper::Create(
        mFileReader->GetInternalTrackReader(0)->GetFileDescriptor(),
        mFileReader->GetMXFVersion(),
        mFileReader->GetInternalTrackReader(0)->GetTrackInfo()->essence_container_label));
    PictureMXFDescriptorHelper *picture_helper = dynamic_cast<PictureMXFDescriptorHelper*>(helper.get());
    SoundMXFDescriptorHelper *sound_helper     = dynamic_cast<SoundMXFDescriptorHelper*>(helper.get());

    uint32_t edit_unit_size = 0;
    vector<uint32_t> sample_sequence;
    switch (helper->GetEssenceType())
    {
        case IEC_DV25:
        case DVBASED_DV25:
        case DV50:
        case DV100_1080I:
        case DV100_720P:
        case UNC_SD:
        case UNC_HD_1080I:
        case UNC_HD_1080P:
        case UNC_HD_720P:
        case UNC_UHD_3840:
        case AVID_10BIT_UNC_SD:
        case AVID_10BIT_UNC_HD_1080I:
        case AVID_10BIT_UNC_HD_1080P:
        case AVID_10BIT_UNC_HD_720P:
        case AVID_ALPHA_SD:
        case AVID_ALPHA_HD_1080I:
        case AVID_ALPHA_HD_1080P:
        case AVID_ALPHA_HD_720P:
        case VC3_1080P_1235:
        case VC3_1080P_1237:
        case VC3_1080P_1238:
        case VC3_1080I_1241:
        case VC3_1080I_1242:
        case VC3_1080I_1243:
        case VC3_1080I_1244:
        case VC3_720P_1250:
        case VC3_720P_1251:
        case VC3_720P_1252:
        case VC3_1080P_1253:
        case VC3_720P_1258:
        case VC3_1080P_1259:
        case VC3_1080I_1260:
            edit_unit_size = picture_helper->GetEditUnitSize();
            break;
        case WAVE_PCM:
            if (get_sample_sequence(mFileReader->GetEditRate(), sound_helper->GetSamplingRate(), &sample_sequence) &&
                sample_sequence.size() == 1)
            {
                edit_unit_size = sample_sequence[0] * sound_helper->GetSampleSize();
            }
            break;
        default:
            break;
    }

    return edit_unit_size;
}

bool EssenceReader::SeekEssence(int64_t base_position, bool for_read)
{
    try
    {
        BMX_ASSERT(base_position >= 0);

        if (mAtCPStart && base_position == mBasePosition)
            return true;

        // if the file position is known then seek to it
        int64_t file_position;
        if (mLastKnownBasePosition == base_position)
            file_position = mLastKnownFilePosition;
        else
            file_position = GetIndexedFilePosition(base_position);
        if (file_position >= 0) {
            mFile->seek(file_position, SEEK_SET);
            SetContentPackageStart(base_position, file_position, true);
            return true;
        }

        BMX_ASSERT(!mEssenceChunkHelper.IsComplete() || !mIndexTableHelper.IsComplete());
        if (!for_read)
            return true;

        // position file at start of first or last known content package
        if (mBasePosition < 0) {
            if (!SeekContentPackageStart())
                return false;
            SetContentPackageStart(0, -1, false);
        } else if (mBasePosition < mLastKnownBasePosition || mBaseReadError) {
            BMX_ASSERT(mLastKnownBasePosition < base_position);
            mFile->seek(mLastKnownFilePosition, SEEK_SET);
            SetContentPackageStart(mLastKnownBasePosition, mLastKnownFilePosition, true);
        }

        // read until the requested position or fail
        mxfKey key;
        uint8_t llen;
        uint64_t len;
        int64_t cp_num_read = 0;
        int64_t next_file_position;
        int64_t next_base_position;
        while (mBasePosition < base_position) {
            if (!ReadFirstEssenceKL(&key, &llen, &len))
                return false;
            cp_num_read = mxfKey_extlen + llen + len;
            next_file_position = mFilePosition;
            next_base_position = mBasePosition;
            if (mBasePosition < base_position) {
                mFile->skip(len);
                ResetState();
                while (ReadNonfirstEssenceKL(&key, &llen, &len)) {
                    cp_num_read += mxfKey_extlen + llen + len;
                    mFile->skip(len);
                }
                mIndexTableHelper.UpdateIndex(next_base_position,
                                              mEssenceChunkHelper.GetEssenceOffset(next_file_position),
                                              cp_num_read);
            }
        }

        return true;
    }
    catch (...)
    {
        ResetState();
        mBaseReadError = true;
        throw;
    }
}

bool EssenceReader::ReadEssenceKL(bool first_element, mxfKey *key_out, uint8_t *llen_out, uint64_t *len_out)
{
    if (first_element) {
        if (!ReadFirstEssenceKL(key_out, llen_out, len_out))
            return false;
        ResetState();
        return true;
    } else {
        return ReadNonfirstEssenceKL(key_out, llen_out, len_out);
    }
}

int64_t EssenceReader::GetIndexedFilePosition(int64_t base_position)
{
    int64_t file_position = -1;
    if (mIndexTableHelper.HaveEditUnitOffset(base_position)) {
        int64_t ess_offset = mIndexTableHelper.GetEditUnitOffset(base_position);
        if (mEssenceChunkHelper.HaveFilePosition(ess_offset))
            file_position = mEssenceChunkHelper.GetFilePosition(ess_offset);
    }

    return file_position;
}

void EssenceReader::SetContentPackageStart(int64_t base_position, int64_t file_position_in, bool pos_at_key)
{
    mBasePosition = base_position;
    mFilePosition = file_position_in;
    if (mFilePosition < 0)
        mFilePosition = mFile->tell();
    if (mFileReader->IsFrameWrapped() && !pos_at_key) {
        BMX_ASSERT(mNextLLen != 0);
        mFilePosition -= mxfKey_extlen + mNextLLen;
    }

    if (mBasePosition > mLastKnownBasePosition) {
        mLastKnownBasePosition = mBasePosition;
        mLastKnownFilePosition = mFilePosition;
        if (!mEssenceChunkHelper.IsComplete())
            mEssenceChunkHelper.UpdateLastChunk(mFilePosition, false);
    }

    if (pos_at_key)
        ResetNextKL();
    mAtCPStart = true;
    mBaseReadError = false;
}

bool EssenceReader::ReadFirstEssenceKL(mxfKey *key_out, uint8_t *llen_out, uint64_t *len_out)
{
    try
    {
        // read the first element's KL
        if (!mAtCPStart) {
            if (!SeekContentPackageStart())
                return false;
            SetContentPackageStart(mBasePosition + 1, -1, false);
        } else if (mNextKey == g_Null_Key) {
            mFile->readKL(&mNextKey, &mNextLLen, &mNextLen);
            if (mEssenceStartKey == g_Null_Key)
                mEssenceStartKey = mNextKey;
            else if (mNextKey != mEssenceStartKey)
                BMX_EXCEPTION(("First element in content package has different key than before"));
        }
        // else have already read first element's KL

        *key_out  = mNextKey;
        *llen_out = mNextLLen;
        *len_out  = mNextLen;

        return true;
    }
    catch (...)
    {
        ResetState();
        mBaseReadError = true;
        throw;
    }
}

bool EssenceReader::ReadNonfirstEssenceKL(mxfKey *key_out, uint8_t *llen_out, uint64_t *len_out)
{
    try
    {
        mxfKey key;
        uint8_t llen;
        uint64_t len;

        BMX_ASSERT(mNextKey == g_Null_Key && !mAtCPStart);

        mFile->readKL(&key, &llen, &len);

        // return false if th KL belongs to the next content package or the next partition has started
        if (mxf_equals_key(&key, &mEssenceStartKey)) {
            SetNextKL(&key, llen, len);
            SetContentPackageStart(mBasePosition + 1, -1, false);
            return false;
        } else if (mxf_is_partition_pack(&key)) {
            mEssenceChunkHelper.UpdateLastChunk(mFile->tell() - mxfKey_extlen - llen, true);
            if (!mHaveFooter && mxf_is_footer_partition_pack(&key))
                SetHaveFooter();
            SetNextKL(&key, llen, len);
            return false;
        }

        *key_out  = key;
        *llen_out = llen;
        *len_out  = len;

        return true;
    }
    catch (...)
    {
        ResetState();
        mBaseReadError = true;
        throw;
    }
}

bool EssenceReader::SeekContentPackageStart()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    size_t partition_id;
    Partition *partition;
    bool have_start_key = (mEssenceStartKey != g_Null_Key);

    if (mxf_is_partition_pack(&mNextKey))
        ReadNextPartition(&mNextKey, mNextLLen, mNextLen);
    ResetNextKL();

    partition_id = mFile->getPartitions().size() - 1;
    partition = mFile->getPartitions()[partition_id];

    bool at_cp_start = false;
    while (!at_cp_start && !mFileIsComplete)
    {
        mFile->readNextNonFillerKL(&key, &llen, &len);

        if (mxf_is_partition_pack(&key))
        {
            if (partition->getBodySID() == mFileReader->mBodySID)
                mEssenceChunkHelper.UpdateLastChunk(mFile->tell() - mxfKey_extlen - llen, true);
            ReadNextPartition(&key, llen, len);
            partition_id++;
            partition = mFile->getPartitions()[partition_id];
        }
        else if (mxf_equals_key(&key, &g_RandomIndexPack_key))
        {
            if (!mHaveFooter)
                BMX_EXCEPTION(("Encountered a RIP key before a footer partition pack"));
            SetFileIsComplete();
        }
        else if (mxf_is_header_metadata(&key))
        {
            if (partition->getHeaderByteCount() > mxfKey_extlen + llen + len)
                mFile->skip(partition->getHeaderByteCount() - mxfKey_extlen - llen);
            else
                mFile->skip(len);
        }
        else if (mxf_is_index_table_segment(&key))
        {
            if (!mIndexTableHelper.IsComplete() && partition->getIndexSID() == mFileReader->mIndexSID) {
                int64_t end_offset = mIndexTableHelper.ReadIndexTableSegment(len);
                // if in footer then file is complete if the index table segment covers the last content package
                if (mHaveFooter && partition_id == mFile->getPartitions().size() - 1 &&
                    end_offset >= mIndexTableHelper.GetDuration())
                {
                    SetFileIsComplete();
                }
            } else {
                mFile->skip(len);
            }
        }
        else if (partition->getBodySID() == mFileReader->mBodySID &&
                 (( have_start_key && mxf_equals_key(&key, &mEssenceStartKey)) ||
                  (!have_start_key && (mxf_is_gc_essence_element(&key) || mxf_avid_is_essence_element(&key)))))
        {
            if (mFileReader->IsClipWrapped()) {
                // check whether this is the target essence container; skip and continue if not
                if (!mFileReader->GetInternalTrackReaderByNumber(mxf_get_track_number(&key))) {
                    mFile->skip(len);
                    continue;
                }
                if (!mEssenceChunkHelper.IsComplete())
                    mEssenceChunkHelper.AppendChunk(partition_id, mFile->tell(), &key, llen, len);
            } else {
                if (!mEssenceChunkHelper.IsComplete() &&
                    mEssenceChunkHelper.GetNumIndexedPartitions() < mFile->getPartitions().size())
                {
                    mEssenceChunkHelper.AppendChunk(partition_id, mFile->tell(), &key, llen, len);
                }
            }
            if (!have_start_key)
                mEssenceStartKey = key;

            SetNextKL(&key, llen, len);
            at_cp_start = true;
        }
        else
        {
            mFile->skip(len);
        }
    }

    return at_cp_start;
}

void EssenceReader::ReadNextPartition(const mxfKey *key, uint8_t llen, uint64_t len)
{
    int64_t partition_pos = mFile->tell() - mxfKey_extlen - llen;
    BMX_ASSERT(partition_pos >= 0 &&
               mFile->getPartitions().back()->getThisPartition() < (uint64_t)partition_pos);

    mFile->readNextPartition(key, len);

    Partition *partition = mFile->getPartitions().back();
    if (partition->getThisPartition() != (uint64_t)partition_pos) {
        log_warn("Updating (in-memory) partition property ThisPartition %"PRId64" to actual file position %"PRId64"\n",
                 partition->getThisPartition(), partition_pos);
        partition->setThisPartition(partition_pos);
    }

    if (!mHaveFooter && partition->isFooter()) {
        SetHaveFooter();
        if (partition->getIndexByteCount() == 0)
            SetFileIsComplete();
    }
}

void EssenceReader::SetHaveFooter()
{
    mHaveFooter = true;
    mEssenceChunkHelper.SetIsComplete();
    mIndexTableHelper.SetEssenceDataSize(mEssenceChunkHelper.GetEssenceDataSize());
}

void EssenceReader::SetFileIsComplete()
{
    if (!mHaveFooter)
        SetHaveFooter();
    mFileIsComplete = true;
    mIndexTableHelper.SetIsComplete();

    SetReadLimits(mReadStartPosition, mReadDuration);
}

void EssenceReader::SetNextKL(const mxfKey *key, uint8_t llen, uint64_t len)
{
    mNextKey   = *key;
    mNextLLen  = llen;
    mNextLen   = len;
}

void EssenceReader::ResetNextKL()
{
    SetNextKL(&g_Null_Key, 0, 0);
}

void EssenceReader::ResetState()
{
    ResetNextKL();
    mAtCPStart = false;
}

