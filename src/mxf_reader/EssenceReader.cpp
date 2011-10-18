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

#include <memory>

#include <im/mxf_reader/EssenceReader.h>
#include <im/mxf_reader/MXFReader.h>
#include <im/mxf_helper/PictureMXFDescriptorHelper.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace im;
using namespace mxfpp;



EssenceReader::EssenceReader(MXFReader *parent_reader)
: mEssenceChunkHelper(parent_reader), mIndexTableHelper(parent_reader)
{
    mParentReader = parent_reader;
    mPosition = 0;

    // read Avid first frame offset extension property
    int32_t avid_first_frame_offset = 0;
    if (mParentReader->IsClipWrapped()) {
        IM_ASSERT(mParentReader->GetNumInternalTrackReaders() == 1);
        auto_ptr<MXFDescriptorHelper> helper(MXFDescriptorHelper::Create(
            mParentReader->GetInternalTrackReader(0)->GetFileDescriptor(),
            mParentReader->GetInternalTrackReader(0)->GetTrackInfo()->essence_container_label));

        PictureMXFDescriptorHelper *picture_helper = dynamic_cast<PictureMXFDescriptorHelper*>(helper.get());
        if (picture_helper && picture_helper->HaveAvidFirstFrameOffset())
            avid_first_frame_offset = picture_helper->GetAvidFirstFrameOffset();
    }

    // extract essence container layout
    mEssenceChunkHelper.ExtractEssenceChunkIndex(avid_first_frame_offset);

    // extract essence container index
    mIndexTableHelper.ExtractIndexTable();
    IM_CHECK(mIndexTableHelper.GetEditRate() == mParentReader->mSampleRate);
    mIndexTableHelper.SetEssenceDataSize(mEssenceChunkHelper.GetEssenceDataSize());

    // check there is sufficient essence container data
    if (mIndexTableHelper.GetDuration() > 0) {
        int64_t last_unit_offset, last_unit_size;
        mIndexTableHelper.GetEditUnit(mIndexTableHelper.GetDuration() - 1, &last_unit_offset, &last_unit_size);
        IM_CHECK_M(mEssenceChunkHelper.GetEssenceDataSize() >= last_unit_offset + last_unit_size,
                   ("Last edit unit (offset %"PRId64", size %"PRId64") not available in "
                        "essence container (size %"PRId64")",
                    last_unit_offset, last_unit_size, mEssenceChunkHelper.GetEssenceDataSize()));
    }

    // set read limits
    mReadStartPosition = 0;
    mReadEndPosition = mIndexTableHelper.GetDuration();

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
}

void EssenceReader::SetReadLimits(int64_t start_position, int64_t end_position)
{
    IM_CHECK(end_position >= start_position);

    mReadStartPosition = LegitimisePosition(start_position);
    mReadEndPosition = LegitimisePosition(end_position - 1) + 1;
}

uint32_t EssenceReader::Read(uint32_t num_samples, int64_t frame_position)
{
    // check read limits
    if (mReadStartPosition == mReadEndPosition ||
        mPosition >= mReadEndPosition ||
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
        first_sample_offset = -mPosition;
        read_num_samples -= first_sample_offset;
        Seek(0);
    }
    if (mPosition + read_num_samples > mReadEndPosition)
        read_num_samples -= mPosition + read_num_samples - mReadEndPosition;
    IM_ASSERT(read_num_samples > 0);


    size_t i;
    for (i = 0; i < mParentReader->GetNumInternalTrackReaders(); i++)
        mParentReader->GetInternalTrackReader(i)->Reset(frame_position);


    int64_t start_position = mPosition;

    if (mParentReader->IsClipWrapped())
        ReadClipWrappedSamples(read_num_samples, frame_position);
    else
        ReadFrameWrappedSamples(read_num_samples, frame_position);


    // add information for first sample in frame

    int64_t index_offset, index_size;
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;

    mIndexTableHelper.GetEditUnit(start_position, &temporal_offset, &key_frame_offset,
                                  &flags, &index_offset, &index_size);

    MXFFrame *frame;
    for (i = 0; i < mParentReader->GetNumInternalTrackReaders(); i++) {
        if (mParentReader->GetInternalTrackReader(i)->IsEnabled()) {
            frame = mParentReader->GetInternalTrackReader(i)->GetFrameBuffer()->GetFrame(frame_position);
            if (frame) {
                frame->SetFirstSampleOffset(first_sample_offset);
                frame->SetTemporalOffset(temporal_offset);
                frame->SetKeyFrameOffset(key_frame_offset);
                frame->SetFlags(flags);
            }
        }
    }


    // signal that frame is complete

    for (i = 0; i < mParentReader->GetNumInternalTrackReaders(); i++) {
        if (mParentReader->GetInternalTrackReader(i)->IsEnabled()) {
            frame = mParentReader->GetInternalTrackReader(i)->GetFrameBuffer()->GetFrame(frame_position);
            if (frame)
                frame->Complete();
        }
    }


    // always be positioned num_samples after previous position
    if (read_num_samples < num_samples)
        Seek(mPosition + (num_samples - read_num_samples));

    return read_num_samples;
}

void EssenceReader::Seek(int64_t position)
{
    if (position >= 0 && position < mReadEndPosition) {
        int64_t file_position, size;
        GetEditUnit(position, &file_position, &size);
        mParentReader->mFile->seek(file_position, SEEK_SET);
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
        mEssenceChunkHelper.GetEditUnit(entry->container_offset, entry->edit_unit_size, &entry->file_offset);
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

void EssenceReader::ReadClipWrappedSamples(uint32_t num_samples, int64_t frame_position)
{
    File *mxf_file = mParentReader->mFile;

    MXFFrame *frame = 0;
    if (mParentReader->GetInternalTrackReader(0)->IsEnabled())
        frame = mParentReader->GetInternalTrackReader(0)->GetFrameBuffer()->CreateFrame(frame_position);

    int64_t current_file_position = mxf_file->tell();
    uint32_t total_num_samples = 0;
    while (total_num_samples < num_samples) {
        // get maximum number of contiguous samples that can be read in one go
        uint32_t num_cont_samples;
        int64_t file_position, size;
        GetEditUnitGroup(mPosition, num_samples - total_num_samples, &file_position, &size, &num_cont_samples);

        if (frame) {
            if (current_file_position != file_position)
                mxf_file->seek(file_position, SEEK_SET);
            current_file_position = file_position;

            if (frame->IsEmpty()) {
                frame->SetCPFilePosition(current_file_position);
                frame->SetFilePosition(current_file_position);

                if (!frame->TemporalReorderingWasSet())
                    frame->SetTemporalReordering(mIndexTableHelper.GetTemporalReordering(0));
            }

            frame->Grow(size);
            uint32_t num_read = mxf_file->read(frame->GetBytesAvailable(), size);
            current_file_position += num_read;
            IM_CHECK(num_read == size);
            frame->IncrementSize(size);
            frame->IncNumSamples(num_cont_samples);
        } else {
            mxf_file->seek(file_position + size, SEEK_SET);
            current_file_position = file_position + size;
        }

        mPosition += num_cont_samples;
        total_num_samples += num_cont_samples;
    }
}

void EssenceReader::ReadFrameWrappedSamples(uint32_t num_samples, int64_t frame_position)
{
    File *mxf_file = mParentReader->mFile;

    int64_t start_position = mPosition;

    map<uint32_t, MXFTrackReader*> enabled_track_readers;
    int64_t current_file_position = mxf_file->tell();
    uint32_t i;
    for (i = 0; i < num_samples; i++) {
        int64_t cp_file_position, size;
        GetEditUnit(mPosition, &cp_file_position, &size);
        if (current_file_position != cp_file_position)
            mxf_file->seek(cp_file_position, SEEK_SET);
        current_file_position = cp_file_position;

        mxfKey key;
        uint8_t llen;
        uint64_t len;
        int64_t cp_num_read = 0;
        while (cp_num_read < size) {
            mxf_file->readKL(&key, &llen, &len);
            cp_num_read += mxfKey_extlen + llen;

            if (mxf_is_gc_essence_element(&key) || mxf_avid_is_essence_element(&key)) {
                uint32_t track_number = mxf_get_track_number(&key);
                MXFTrackReader *track_reader = 0;
                MXFFrame *frame = 0;
                if (enabled_track_readers.find(track_number) == enabled_track_readers.end()) {
                    // frame does not yet exist - create it if track is enabled
                    track_reader = mParentReader->GetInternalTrackReaderByNumber(track_number);
                    if (start_position == mPosition && track_reader && track_reader->IsEnabled()) {
                        frame = track_reader->GetFrameBuffer()->CreateFrame(frame_position);
                        frame->SetCPFilePosition(cp_file_position);
                        frame->SetFilePosition(cp_file_position + cp_num_read);
                        enabled_track_readers[track_number] = track_reader;
                    } else {
                        enabled_track_readers[track_number] = 0;
                    }
                } else {
                    // frame exists if track is enabled - get it
                    track_reader = enabled_track_readers[track_number];
                    if (track_reader)
                        frame = track_reader->GetFrame(frame_position);
                }

                if (frame) {
                    if (!frame->TemporalReorderingWasSet()) {
                        frame->SetTemporalReordering(
                            mIndexTableHelper.GetTemporalReordering(cp_num_read - (mxfKey_extlen + llen)));
                    }

                    frame->Grow(len);
                    uint32_t num_read = mxf_file->read(frame->GetBytesAvailable(), len);
                    IM_CHECK(num_read == len);
                    frame->IncrementSize(len);
                    frame->IncNumSamples(1);
                } else {
                    mxf_file->skip(len);
                }
            } else {
                mxf_file->skip(len);
            }

            cp_num_read += len;
        }
        IM_CHECK_M(cp_num_read == size,
                   ("Read content package size (0x%"PRIx64") does not match size in index (0x%"PRIx64") "
                    "at file position 0x%"PRIx64,
                        cp_num_read, size, current_file_position + cp_num_read));

        current_file_position += size;
        mPosition++;
    }
}

void EssenceReader::GetEditUnit(int64_t position, int64_t *file_position, int64_t *size)
{
    int64_t index_offset, index_size;
    mIndexTableHelper.GetEditUnit(position, &index_offset, &index_size);

    mEssenceChunkHelper.GetEditUnit(index_offset, index_size, file_position);
    *size = index_size;
}

void EssenceReader::GetEditUnitGroup(int64_t position, uint32_t max_samples, int64_t *file_position, int64_t *size,
                                     uint32_t *num_samples)
{
    IM_CHECK(max_samples > 0);

    if (!mIndexTableHelper.HaveFixedEditUnitByteCount() || max_samples == 1) {
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
        IM_CHECK(right_size == mIndexTableHelper.GetFixedEditUnitByteCount());

        if (right_file_position > first_file_position + mIndexTableHelper.GetFixedEditUnitByteCount() * (right_num_samples - 1)) {
            // first to right is not contiguous - try halfway between left and right (round down)
            last_num_samples = right_num_samples;
            right_num_samples = (left_num_samples + right_num_samples) / 2;
        } else {
            IM_CHECK(right_file_position == first_file_position + mIndexTableHelper.GetFixedEditUnitByteCount() * (right_num_samples - 1));
            // first to right is contiguous - try halfway between right and last (round up)
            left_num_samples = right_num_samples;
            right_num_samples = (right_num_samples + last_num_samples + 1) / 2;
        }
    }

    *file_position = first_file_position;
    *size = first_size * left_num_samples;
    *num_samples = left_num_samples;
}

