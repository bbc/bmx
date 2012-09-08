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



EssenceReader::EssenceReader(MXFFileReader *file_reader)
: mEssenceChunkHelper(file_reader), mIndexTableHelper(file_reader)
{
    mFileReader = file_reader;
    mFrameMetadataReader = new FrameMetadataReader(file_reader);
    mPosition = 0;
    mImageStartOffset = 0;
    mImageEndOffset = 0;
    mCachedKey = g_Null_Key;
    mCachedLLen = 0;
    mCachedLen = 0;
    mHaveCachedKL = false;

    auto_ptr<MXFDescriptorHelper> helper;
    PictureMXFDescriptorHelper *picture_helper = 0;;
    SoundMXFDescriptorHelper *sound_helper = 0;;
    if (mFileReader->IsClipWrapped()) {
        BMX_ASSERT(mFileReader->GetNumInternalTrackReaders() == 1);
        helper.reset(MXFDescriptorHelper::Create(
            mFileReader->GetInternalTrackReader(0)->GetFileDescriptor(),
            mFileReader->GetMXFVersion(),
            mFileReader->GetInternalTrackReader(0)->GetTrackInfo()->essence_container_label));
        picture_helper = dynamic_cast<PictureMXFDescriptorHelper*>(helper.get());
        sound_helper = dynamic_cast<SoundMXFDescriptorHelper*>(helper.get());
    }


    // read ImageStartOffset used in Avid uncompressed video and the
    // FirstFrameOffset Avid extension property
    int32_t avid_first_frame_offset = 0;
    if (mFileReader->IsClipWrapped() && picture_helper) {
        if (picture_helper->HaveAvidFirstFrameOffset())
            avid_first_frame_offset = picture_helper->GetAvidFirstFrameOffset();

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

    // extract essence container layout
    mEssenceChunkHelper.ExtractEssenceChunkIndex(avid_first_frame_offset);

    // extract essence container index
    if (mFileReader->mIndexSID) {
        if (!mIndexTableHelper.ExtractIndexTable() &&
            mEssenceChunkHelper.GetEssenceDataSize() > 0)
        {
            BMX_EXCEPTION(("Missing index table segments for essence data with size %"PRId64,
                           mEssenceChunkHelper.GetEssenceDataSize()));
        }
    } else {
        vector<uint32_t> sample_sequence;
        if (mFileReader->IsClipWrapped()) {
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
                case VC3_720P_1250:
                case VC3_720P_1251:
                case VC3_720P_1252:
                case VC3_1080P_1253:
                    mIndexTableHelper.SetConstantEditUnitSize(mFileReader->GetSampleRate(),
                                                              picture_helper->GetEditUnitSize());
                    break;
                case WAVE_PCM:
                    if (sound_helper->GetSamplingRate() == mFileReader->GetSampleRate())
                    {
                        mIndexTableHelper.SetConstantEditUnitSize(mFileReader->GetSampleRate(),
                                                                  sound_helper->GetSampleSize());
                    }
                    else if (get_sample_sequence(mFileReader->GetSampleRate(),
                                                 sound_helper->GetSamplingRate(),
                                                 &sample_sequence) &&
                             sample_sequence.size() == 1)
                    {
                        mIndexTableHelper.SetConstantEditUnitSize(mFileReader->GetSampleRate(),
                                                                  sample_sequence[0] * sound_helper->GetSampleSize());
                    }
                    else
                    {
                        // TODO: support sample sequences > 1 using special index table segment
                        // TODO: will fail later on
                        log_warn("Unsupported clip wrapped WAVE PCM rates with missing index table\n");
                        mIndexTableHelper.SetEditRate(mFileReader->GetSampleRate());
                    }
                    break;
                default:
                    // TODO: use essence parsers to dynamically index data
                    // TODO: will fail later on
                    log_warn("Unsupported clip wrapped essence data with missing index table\n");
                    mIndexTableHelper.SetEditRate(mFileReader->GetSampleRate());
                    break;
            }
        } else {
            mIndexTableHelper.SetEditRate(mFileReader->GetSampleRate());
        }
    }
    BMX_CHECK(mIndexTableHelper.GetEditRate() == mFileReader->GetSampleRate());
    mIndexTableHelper.SetEssenceDataSize(mEssenceChunkHelper.GetEssenceDataSize());

    // check there is sufficient essence container data
    if (mIndexTableHelper.GetDuration() > 0) {
        int64_t last_unit_offset, last_unit_size;
        mIndexTableHelper.GetEditUnit(mIndexTableHelper.GetDuration() - 1, &last_unit_offset, &last_unit_size);
        BMX_CHECK_M(mEssenceChunkHelper.GetEssenceDataSize() >= last_unit_offset + last_unit_size,
                   ("Last edit unit (offset %"PRId64", size %"PRId64") not available in "
                        "essence container (size %"PRId64")",
                    last_unit_offset, last_unit_size, mEssenceChunkHelper.GetEssenceDataSize()));
    }


    // set read limits
    mReadStartPosition = 0;
    if (mIndexTableHelper.IsComplete())
        mReadDuration = mIndexTableHelper.GetDuration();
    else
        mReadDuration = INT64_MAX;

    // set file position at essence data start
    Seek(0);

#if 0
    {
        printf("Index: duration=%"PRId64"\n", mIndexTableHelper.GetDuration());
        int64_t file_position, size;
        int64_t i;
        for (i = 0; i < mIndexTableHelper.GetDuration(); i++) {
            GetEditUnit(i, &file_position, &size);
            printf("edit unit %"PRId64": file_pos=%"PRId64", size=%"PRId64"\n", i, file_position, size);
        }
    }
#endif
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
    BMX_CHECK(duration >= 0);

    mReadStartPosition = LegitimisePosition(start_position);
    if (duration == 0 || (mIndexTableHelper.IsComplete() && mIndexTableHelper.GetDuration() == 0))
        mReadDuration = 0;
    else if (!mIndexTableHelper.IsComplete())
        mReadDuration = duration;
    else
        mReadDuration = LegitimisePosition(start_position + duration - 1) - mReadStartPosition + 1;
}

uint32_t EssenceReader::Read(uint32_t num_samples)
{
    // init track frames
    size_t i;
    for (i = 0; i < mTrackFrames.size(); i++)
        delete mTrackFrames[i];
    mTrackFrames.clear();
    mTrackFrames.assign(mFileReader->GetNumInternalTrackReaders(), 0);
    mFrameMetadataReader->Reset();

    // check read limits
    if (mReadDuration == 0 ||
        mPosition >= mReadStartPosition + mReadDuration ||
        mPosition + num_samples <= 0)
    {
        // always be positioned num_samples after previous position
        Seek(mPosition + num_samples);
        return 0;
    }

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


    int64_t start_position = mPosition;

    if (mFileReader->IsClipWrapped())
        ReadClipWrappedSamples(read_num_samples);
    else
        ReadFrameWrappedSamples(read_num_samples);


    // add information for first sample in frame

    int64_t index_offset = 0;
    int64_t index_size = 0;
    int8_t temporal_offset = 0;
    int8_t key_frame_offset = 0;
    uint8_t flags = 0;
    if (HaveEditUnit(start_position)) {
        mIndexTableHelper.GetEditUnit(start_position, &temporal_offset, &key_frame_offset,
                                      &flags, &index_offset, &index_size);
    }

    Frame *frame;
    for (i = 0; i < mFileReader->GetNumInternalTrackReaders(); i++) {
        frame = mTrackFrames[i];
        if (frame) {
            frame->first_sample_offset = first_sample_offset;
            frame->temporal_offset     = temporal_offset;
            frame->key_frame_offset    = key_frame_offset;
            frame->flags               = flags;
        }
    }


    // complete and push frames
    for (i = 0; i < mFileReader->GetNumInternalTrackReaders(); i++) {
        if (mTrackFrames[i]) {
            mFrameMetadataReader->InsertFrameMetadata(mTrackFrames[i],
                    mFileReader->GetInternalTrackReader(i)->GetTrackInfo()->file_track_number);
            mFileReader->GetInternalTrackReader(i)->GetFrameBuffer()->PushFrame(mTrackFrames[i]);
            mTrackFrames[i] = 0;
        }
    }


    // always be positioned num_samples after previous position
    if (read_num_samples < num_samples)
        Seek(mPosition + (num_samples - read_num_samples));

    return read_num_samples;
}

void EssenceReader::Seek(int64_t position)
{
    if (position >= 0 && position < mReadStartPosition + mReadDuration) {
        if (!mIndexTableHelper.IsComplete() &&
            position != 0 && position >= mIndexTableHelper.GetDuration())
        {
            BMX_EXCEPTION(("Seeking beyond indexed essence data for incomplete index table not yet supported"));
        }

        mFileReader->mFile->seek(GetFilePosition(position), SEEK_SET);
        mHaveCachedKL = false;
    }

    mPosition = position;
}

mxfRational EssenceReader::GetEditRate()
{
    return mIndexTableHelper.GetEditRate();
}

bool EssenceReader::GetIndexEntry(MXFIndexEntryExt *entry, int64_t position)
{
    if (mIndexTableHelper.GetIndexEntry(entry, position)) {
        entry->file_offset = mEssenceChunkHelper.GetFilePosition(entry->container_offset, entry->edit_unit_size);
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

void EssenceReader::ReadClipWrappedSamples(uint32_t num_samples)
{
    File *mxf_file = mFileReader->mFile;

    if (mFileReader->GetInternalTrackReader(0)->IsEnabled())
        mTrackFrames[0] = mFileReader->GetInternalTrackReader(0)->GetFrameBuffer()->CreateFrame();
    Frame *frame = mTrackFrames[0];

    int64_t current_file_position = mxf_file->tell();
    uint32_t total_num_samples = 0;
    while (total_num_samples < num_samples) {
        // get maximum number of contiguous samples that can be read in one go
        uint32_t num_cont_samples;
        int64_t file_position, size;
        if (mImageStartOffset || mImageEndOffset)
            GetEditUnitGroup(mPosition, 1, &file_position, &size, &num_cont_samples);
        else
            GetEditUnitGroup(mPosition, num_samples - total_num_samples, &file_position, &size, &num_cont_samples);

        if (frame) {
            BMX_CHECK(size >= mImageStartOffset + mImageEndOffset);

            if (current_file_position != file_position)
                mxf_file->seek(file_position, SEEK_SET);
            current_file_position = file_position;

            BMX_CHECK(size <= UINT32_MAX);
            frame->Grow((uint32_t)size);
            uint32_t num_read = mxf_file->read(frame->GetBytesAvailable(), (uint32_t)size);
            current_file_position += num_read;
            BMX_CHECK(num_read == size);

            size -= mImageEndOffset;
            if (mImageStartOffset > 0) {
                memmove(frame->GetBytesAvailable(),
                        frame->GetBytesAvailable() + mImageStartOffset,
                        (uint32_t)(size - mImageStartOffset));
                size -= mImageStartOffset;
            }

            frame->IncrementSize((uint32_t)size);
            frame->num_samples += num_cont_samples;

            if (frame->IsEmpty()) {
                frame->ec_position         = mPosition;
                frame->temporal_reordering = mIndexTableHelper.GetTemporalReordering(0);
                frame->cp_file_position    = current_file_position - mImageEndOffset - size;
                frame->file_position       = frame->cp_file_position;
            }
        } else {
            mxf_file->seek(file_position + size, SEEK_SET);
            current_file_position = file_position + size;
        }

        mPosition += num_cont_samples;
        total_num_samples += num_cont_samples;
    }
}

void EssenceReader::ReadFrameWrappedSamples(uint32_t num_samples)
{
    File *mxf_file = mFileReader->mFile;

    int64_t start_position = mPosition;

    map<uint32_t, MXFTrackReader*> enabled_track_readers;
    int64_t current_file_position = mxf_file->tell();
    uint32_t i;
    for (i = 0; i < num_samples; i++) {
        int64_t cp_file_position;
        int64_t size = 0;
        int64_t max_size = 0;
        if (HaveEditUnit(mPosition)) {
            GetEditUnit(mPosition, &cp_file_position, &size);
            if (current_file_position != cp_file_position) {
                mxf_file->seek(cp_file_position, SEEK_SET);
                current_file_position = cp_file_position;
            }
            max_size = size;
        } else {
            BMX_ASSERT(!mIndexTableHelper.IsComplete());
            // TODO: deal with multiple ess partitions which result in GetMaxContiguousSize failure
            max_size = mEssenceChunkHelper.GetMaxContiguousSize(current_file_position);
            cp_file_position = current_file_position;
        }

        mxfKey key, first_key;
        uint8_t llen;
        uint64_t len;
        int64_t cp_num_read = 0;
        while (cp_num_read < max_size) {
            if (mHaveCachedKL) {
                key = mCachedKey;
                llen = mCachedLLen;
                len = mCachedLen;
                mHaveCachedKL = false;
            } else {
                mxf_file->readKL(&key, &llen, &len);
            }
            if (cp_num_read == 0) {
                first_key = key;
            } else if (size == 0 && mxf_equals_key(&key, &first_key)) {
                mCachedKey = key;
                mCachedLLen = llen;
                mCachedLen = len;
                mHaveCachedKL = true;
                break;
            }
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
                        mTrackFrames[track_reader->GetTrackIndex()] = track_reader->GetFrameBuffer()->CreateFrame();
                        frame = mTrackFrames[track_reader->GetTrackIndex()];

                        BMX_CHECK(cp_num_read <= UINT32_MAX);

                        frame->ec_position         = start_position;
                        frame->cp_file_position    = cp_file_position;
                        frame->file_position       = cp_file_position + cp_num_read;
                        if (HaveEditUnit(start_position)) {
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
                    uint32_t num_read = mxf_file->read(frame->GetBytesAvailable(), (uint32_t)len);
                    BMX_CHECK(num_read == len);
                    frame->IncrementSize((uint32_t)len);
                    frame->num_samples++;
                } else {
                    mxf_file->skip(len);
                }
            } else if (!processed_metadata) {
                mxf_file->skip(len);
            }

            cp_num_read += len;
        }
        if (size != 0 && cp_num_read != size) {
           BMX_EXCEPTION(("Read content package size (0x%"PRIx64") does not match size in index (0x%"PRIx64") "
                          "at file position 0x%"PRIx64,
                          cp_num_read, size, current_file_position + cp_num_read));
        }

        current_file_position += cp_num_read;
        mPosition++;
    }
}

bool EssenceReader::HaveEditUnit(int64_t position)
{
    return position >= 0 && position < mIndexTableHelper.GetDuration();
}

void EssenceReader::GetEditUnit(int64_t position, int64_t *file_position, int64_t *size)
{
    int64_t index_offset, index_size;
    mIndexTableHelper.GetEditUnit(position, &index_offset, &index_size);

    *file_position = mEssenceChunkHelper.GetFilePosition(index_offset, index_size);
    *size = index_size;
}

void EssenceReader::GetEditUnitGroup(int64_t position, uint32_t max_samples, int64_t *file_position, int64_t *size,
                                     uint32_t *num_samples)
{
    BMX_CHECK(max_samples > 0);

    if (!mIndexTableHelper.HaveConstantEditUnitSize() || max_samples == 1) {
        GetEditUnit(position, file_position, size);
        *num_samples = 1;
        return;
    }

    int64_t first_file_position;
    int64_t first_size;
    GetEditUnit(position, &first_file_position, &first_size);

    // binary search to find number of contiguous edit units
    // first <= left <= right <= last
    // first to left is contiguous
    uint32_t left_num_samples = 1;
    uint32_t right_num_samples = max_samples;
    uint32_t last_num_samples = max_samples;

    int64_t right_file_position;
    int64_t right_size;
    while (right_num_samples != left_num_samples) {
        GetEditUnit(position + right_num_samples - 1, &right_file_position, &right_size);
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

    *file_position = first_file_position;
    *size = first_size * left_num_samples;
    *num_samples = left_num_samples;
}

int64_t EssenceReader::GetFilePosition(int64_t position)
{
    return mEssenceChunkHelper.GetFilePosition(mIndexTableHelper.GetEditUnitOffset(position));
}

