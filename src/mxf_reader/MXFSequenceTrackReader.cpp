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

#include <cstring>

#include <set>

#include <bmx/mxf_reader/MXFSequenceTrackReader.h>
#include <bmx/mxf_reader/MXFSequenceReader.h>
#include <bmx/essence_parser/AVCIEssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define DISABLED_SEG_READ_LIMIT     (-9999)



MXFSequenceTrackReader::MXFSequenceTrackReader(MXFSequenceReader *sequence_reader, size_t track_index)
{
    mSequenceReader = sequence_reader;
    mTrackIndex = track_index;
    mTrackInfo = 0;
    mFileDescriptor = 0;
    mFileSourcePackage = 0;
    mEmptyFrames = false;
    mEmptyFramesSet = false;
    mIsEnabled = true;
    mReadStartPosition = 0;
    mReadDuration = -1;
    mEditRate = ZERO_RATIONAL;
    mPosition = 0;
    mDuration = 0;
    mOrigin = 0;
    mReadError = false;

    mFrameBuffer.SetTargetBuffer(new DefaultFrameBuffer(), true);
}

MXFSequenceTrackReader::~MXFSequenceTrackReader()
{
    delete mTrackInfo;
}

void MXFSequenceTrackReader::SetEmptyFrames(bool enable)
{
    mEmptyFrames = enable;
    mEmptyFramesSet = true;

    mFrameBuffer.SetEmptyFrames(enable);

    size_t i;
    for (i = 0; i < mTrackSegments.size(); i++)
        mTrackSegments[i]->SetEmptyFrames(enable);
}

bool MXFSequenceTrackReader::IsCompatible(MXFTrackReader *segment) const
{
    if (!segment->IsEnabled())
        return false;

    if (mTrackSegments.empty())
        return true;

    if (!mTrackSegments[0]->GetTrackInfo()->IsCompatible(segment->GetTrackInfo()) ||
        mTrackSegments[0]->GetTrackInfo()->material_track_number != segment->GetTrackInfo()->material_track_number)
    {
        return false;
    }

    if (mTrackSegments[0]->HaveAVCIHeader() != mTrackSegments[0]->HaveAVCIHeader())
        return false;
    if (mTrackSegments[0]->HaveAVCIHeader() &&
        memcmp(mTrackSegments[0]->GetAVCIHeader(), segment->GetAVCIHeader(), AVCI_HEADER_SIZE) != 0)
    {
        // TODO: not sure whether they need to be identical
        log_warn("Sequence segment's AVC-Intra header data are not identical\n");
    }

    return true;
}

void MXFSequenceTrackReader::AppendSegment(MXFTrackReader *segment)
{
    mSegmentOffsets.push_back(GetDuration());

    segment->SetFrameBuffer(&mFrameBuffer, false);

    if (mTrackSegments.empty()) {
        mTrackInfo = segment->GetTrackInfo()->Clone();
        mFileDescriptor = segment->GetFileDescriptor();
        mFileSourcePackage = segment->GetFileSourcePackage();
        mEditRate = segment->GetEditRate();
        mDuration = segment->GetDuration();
        mOrigin = segment->GetOrigin();
    } else {
        // not valid because multiple segments means there are multiple file source packages
        mFileSourcePackage = 0;

        // set non-constant properties to null
        const MXFTrackInfo *segment_track_info = segment->GetTrackInfo();
        if (segment_track_info->material_package_uid != mTrackInfo->material_package_uid) {
            mTrackInfo->material_package_uid = g_Null_UMID;
            mTrackInfo->material_track_id = 0;
        } else if (segment_track_info->material_track_id != mTrackInfo->material_track_id) {
            mTrackInfo->material_track_id = 0;
        }
        if (segment_track_info->material_track_number != mTrackInfo->material_track_number)
            mTrackInfo->material_track_number = 0;

        if (segment_track_info->file_package_uid != mTrackInfo->file_package_uid) {
            mTrackInfo->file_package_uid = g_Null_UMID;
            mTrackInfo->file_track_id = 0;
        } else if (segment_track_info->file_track_id != mTrackInfo->file_track_id) {
            mTrackInfo->file_track_id = 0;
        }
        if (segment_track_info->file_track_number != mTrackInfo->file_track_number)
            mTrackInfo->file_track_number = 0;

        BMX_ASSERT(segment->GetEditRate() == mTrackInfo->edit_rate);
        mTrackInfo->duration += segment->GetDuration();
        mDuration += segment->GetDuration();
    }

    if (mEmptyFramesSet)
        segment->SetEmptyFrames(mEmptyFrames);

    mTrackSegments.push_back(segment);
}

MXFTrackReader* MXFSequenceTrackReader::GetSegment(size_t index)
{
    BMX_CHECK(index < mTrackSegments.size());
    return mTrackSegments[index];
}

void MXFSequenceTrackReader::UpdateReadLimits()
{
    mReadStartPosition = 0;
    mReadDuration = 0;

    if (mTrackSegments.empty())
        return;


    // get read start position from first enabled segment
    size_t i;
    for (i = 0; i < mTrackSegments.size(); i++) {
        if (mTrackSegments[i]->GetReadStartPosition() > DISABLED_SEG_READ_LIMIT)
            break;
        mReadStartPosition += mTrackSegments[i]->GetDuration();
    }
    if (i >= mTrackSegments.size())
        return; // nothing enabled

    mReadStartPosition += mTrackSegments[i]->GetReadStartPosition();
    mReadDuration = mTrackSegments[i]->GetReadDuration();

    // get read end position from the last enabled segment
    for (i = i + 1; i < mTrackSegments.size(); i++) {
        if (mTrackSegments[i]->GetReadStartPosition() <= DISABLED_SEG_READ_LIMIT)
            break;
        mReadDuration += mTrackSegments[i]->GetReadDuration();
    }
}

void MXFSequenceTrackReader::SetEnable(bool enable)
{
    mIsEnabled = enable;

    size_t i;
    for (i = 0; i < mTrackSegments.size(); i++)
        mTrackSegments[i]->SetEnable(enable);
}

void MXFSequenceTrackReader::SetFrameBuffer(FrameBuffer *frame_buffer, bool take_ownership)
{
    mFrameBuffer.SetTargetBuffer(frame_buffer, take_ownership);
}

vector<size_t> MXFSequenceTrackReader::GetFileIds(bool internal_ess_only) const
{
    set<size_t> file_id_set;
    size_t i;
    for (i = 0; i < mTrackSegments.size(); i++) {
        vector<size_t> seg_file_ids = mTrackSegments[i]->GetFileIds(internal_ess_only);
        file_id_set.insert(seg_file_ids.begin(), seg_file_ids.end());
    }

    vector<size_t> file_ids;
    file_ids.insert(file_ids.begin(), file_id_set.begin(), file_id_set.end());

    return file_ids;
}

void MXFSequenceTrackReader::GetReadLimits(bool limit_to_available, int64_t *start_position, int64_t *duration) const
{
    int16_t precharge = GetPrecharge(0, limit_to_available);
    int16_t rollout = GetRollout(mDuration - 1, limit_to_available);
    *start_position = 0 + precharge;
    *duration = - precharge + mDuration + rollout;
}

void MXFSequenceTrackReader::SetReadLimits()
{
    int64_t start_position;
    int64_t duration;
    GetReadLimits(false, &start_position, &duration);
    SetReadLimits(start_position, duration, true);
}

void MXFSequenceTrackReader::SetReadLimits(int64_t start_position, int64_t duration, bool seek_to_start)
{
    MXFTrackReader *start_segment, *end_segment;
    int64_t start_segment_position;
    int64_t end_segment_duration;
    GetSegmentPosition(start_position, &start_segment, &start_segment_position);
    GetSegmentPosition(start_position + duration, &end_segment, &end_segment_duration);

    if (start_segment == end_segment) {
        start_segment->SetReadLimits(start_segment_position, end_segment_duration, false);
    } else {
        // note that start segment has 0 rollout
        start_segment->SetReadLimits(start_segment_position,
                                     start_segment->GetDuration() - start_segment_position,
                                     false);
        // note that end segment has 0 pre-charge
        end_segment->SetReadLimits(0, end_segment_duration, false);
    }

    // effectively disable segments before the start segment and after the end segment
    size_t i;
    for (i = 0; i < mTrackSegments.size(); i++) {
        if (mTrackSegments[i] == start_segment)
            break;

        mTrackSegments[i]->SetReadLimits(DISABLED_SEG_READ_LIMIT, 0, false);
    }
    for (; i < mTrackSegments.size(); i++) {
        if (mTrackSegments[i] == end_segment)
            break;
    }
    for (i = i + 1; i < mTrackSegments.size(); i++)
        mTrackSegments[i]->SetReadLimits(DISABLED_SEG_READ_LIMIT, 0, false);


    UpdateReadLimits();
    mSequenceReader->UpdateReadLimits();


    if (seek_to_start)
        Seek(start_position);
}

uint32_t MXFSequenceTrackReader::Read(uint32_t num_samples, bool is_top)
{
    if (!mIsEnabled)
        return 0;

    if (is_top) {
        mSequenceReader->SetNextFramePosition(mEditRate, mPosition);
        mSequenceReader->SetNextFrameTrackPositions();
    }

    MXFTrackReader *segment;
    int64_t segment_position;
    GetSegmentPosition(mPosition, &segment, &segment_position);

    uint32_t total_num_read = 0;
    MXFTrackReader *prev_segment;
    do {
        // seek if segment is out of position
        if (segment_position != segment->GetPosition())
            segment->Seek(segment_position);

        uint32_t num_read = segment->Read(num_samples - total_num_read, false);

        total_num_read += num_read;

        prev_segment = segment;
        GetSegmentPosition(mPosition + total_num_read, &segment, &segment_position);
    }
    while (total_num_read < num_samples && segment != prev_segment);


    // always be positioned num_samples after previous position
    mPosition += num_samples;

    return total_num_read;
}

void MXFSequenceTrackReader::Seek(int64_t position)
{
    BMX_CHECK(!mTrackSegments.empty());

    MXFTrackReader *segment;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_position);

    segment->Seek(segment_position);

    mPosition = position;
}

bool MXFSequenceTrackReader::GetIndexEntry(MXFIndexEntryExt *entry, int64_t position) const
{
    BMX_CHECK(!mTrackSegments.empty());

    // TODO: need to adjust index entries for the sequence or add source file info to give the entry context
    MXFTrackReader *segment;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_position);

    return segment->GetIndexEntry(entry, segment_position);
}

int16_t MXFSequenceTrackReader::GetPrecharge(int64_t position, bool limit_to_available) const
{
    BMX_CHECK(!mTrackSegments.empty());

    MXFTrackReader *segment;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_position);

    return segment->GetPrecharge(segment_position, limit_to_available);
}

int16_t MXFSequenceTrackReader::GetRollout(int64_t position, bool limit_to_available) const
{
    BMX_CHECK(!mTrackSegments.empty());

    MXFTrackReader *segment;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_position);

    return segment->GetRollout(segment_position, limit_to_available);
}

void MXFSequenceTrackReader::SetNextFramePosition(Rational edit_rate, int64_t position)
{
    mSequenceReader->SetNextFramePosition(edit_rate, position);
}

SourcePackage* MXFSequenceTrackReader::GetFileSourcePackage() const
{
    BMX_CHECK(mFileSourcePackage);
    return mFileSourcePackage;
}

bool MXFSequenceTrackReader::HaveAVCIHeader() const
{
    BMX_CHECK(!mTrackSegments.empty());

    return mTrackSegments.front()->HaveAVCIHeader();
}

const unsigned char* MXFSequenceTrackReader::GetAVCIHeader() const
{
    BMX_CHECK(!mTrackSegments.empty());

    return mTrackSegments.front()->GetAVCIHeader();
}

void MXFSequenceTrackReader::GetSegmentPosition(int64_t position, MXFTrackReader **segment,
                                                int64_t *segment_position) const
{
    BMX_CHECK(!mTrackSegments.empty());

    size_t i;
    for (i = 0; i < mSegmentOffsets.size(); i++) {
        if (position < mSegmentOffsets[i])
            break;
    }

    if (i == 0) {
        *segment = mTrackSegments[0];
        *segment_position = position;
    } else {
        *segment = mTrackSegments[i - 1];
        *segment_position = position - mSegmentOffsets[i - 1];
    }
}

void MXFSequenceTrackReader::UpdatePosition(size_t segment_index)
{
    BMX_ASSERT(!mTrackSegments.empty());
    BMX_ASSERT(segment_index < mTrackSegments.size());

    mPosition = mSegmentOffsets[segment_index] + mTrackSegments[segment_index]->GetPosition();
}

