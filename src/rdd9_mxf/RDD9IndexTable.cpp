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

#include <algorithm>

#include <bmx/rdd9_mxf/RDD9ContentPackage.h>
#include <bmx/MXFUtils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



// MAX_INDEX_SEGMENT_SIZE <
//      (65535 [2-byte max len]
//        - (80 [segment header] + 12 [delta entry array header] + 6 [delta entry] + 22 [index entry array header]))
#define MAX_INDEX_SEGMENT_SIZE      65000
#define INDEX_ENTRIES_INCREMENT     250

#define MAX_GOP_SIZE                15

#define MAX_CACHE_ENTRIES           250



static bool compare_element(const RDD9IndexTableElement *left, const RDD9IndexTableElement *right)
{
    return left->element_type < right->element_type;
}



RDD9IndexEntry::RDD9IndexEntry()
{
    temporal_offset = 0;
    key_frame_offset = 0;
    flags = 0;
    can_start_partition = true;
}

RDD9IndexEntry::RDD9IndexEntry(int8_t temporal_offset_, int8_t key_frame_offset_, uint8_t flags_,
                               bool can_start_partition_)
{
    temporal_offset = temporal_offset_;
    key_frame_offset = key_frame_offset_;
    flags = flags_;
    can_start_partition = can_start_partition_;
}

bool RDD9IndexEntry::IsDefault()
{
    return temporal_offset == 0 && key_frame_offset == 0 && flags == 0;
}

bool RDD9IndexEntry::IsCompatible(const RDD9IndexEntry &entry)
{
    // compatible if current entry is the default entry or the new entry equals the current entry
    return (temporal_offset == 0 && key_frame_offset == 0 && flags == 0) ||
           (temporal_offset == entry.temporal_offset && key_frame_offset == entry.key_frame_offset &&
               flags == entry.flags);
}



RDD9DeltaEntry::RDD9DeltaEntry()
{
    pos_table_index = 0;
    slice = 0;
    element_delta = 0;
}



RDD9IndexTableElement::RDD9IndexTableElement(uint32_t track_index_, ElementType element_type_, bool is_cbe_,
                                             bool apply_temporal_reordering_)
{
    track_index = track_index_;
    element_type = element_type_;
    is_cbe = is_cbe_;
    apply_temporal_reordering = apply_temporal_reordering_;
    slice_offset = 0;
    element_size = 0;
}

void RDD9IndexTableElement::CacheIndexEntry(int64_t position, int8_t temporal_offset, int8_t key_frame_offset,
                                            uint8_t flags, bool can_start_partition)
{
    BMX_CHECK(mIndexEntryCache.size() <= MAX_CACHE_ENTRIES);

    mIndexEntryCache[position] = RDD9IndexEntry(temporal_offset, key_frame_offset, flags, can_start_partition);
}

void RDD9IndexTableElement::UpdateIndexEntry(int64_t position, int8_t temporal_offset)
{
    BMX_ASSERT(mIndexEntryCache.find(position) != mIndexEntryCache.end());

    mIndexEntryCache[position].temporal_offset = temporal_offset;
}

bool RDD9IndexTableElement::TakeIndexEntry(int64_t position, RDD9IndexEntry *entry)
{
    map<int64_t, RDD9IndexEntry>::iterator result = mIndexEntryCache.find(position);
    if (result == mIndexEntryCache.end())
        return false;

    *entry = result->second;
    mIndexEntryCache.erase(result);

    return true;
}

bool RDD9IndexTableElement::CanStartPartition(int64_t position)
{
    if (is_cbe)
        return true;

    BMX_ASSERT(mIndexEntryCache.find(position) != mIndexEntryCache.end());
    return (mIndexEntryCache[position].can_start_partition);
}



RDD9IndexTableSegment::RDD9IndexTableSegment(uint32_t index_sid, uint32_t body_sid, Rational frame_rate,
                                             int64_t start_position, uint32_t index_entry_size, uint32_t slice_count,
                                             mxfOptBool single_index_location, mxfOptBool single_essence_location,
                                             mxfOptBool forward_index_direction)
{
    mIndexEntrySize = index_entry_size;

    mEntries.SetAllocBlockSize(INDEX_ENTRIES_INCREMENT * index_entry_size);

    // InstanceUID is set when writing to account for repeats
    mSegment.setIndexEditRate(frame_rate);
    mSegment.setIndexStartPosition(start_position);
    mSegment.setIndexDuration(0);
    mSegment.setIndexSID(index_sid);
    mSegment.setBodySID(body_sid);
    mSegment.setEditUnitByteCount(0);
    mSegment.setSliceCount(slice_count);
    mSegment.setSingleIndexLocation(single_index_location);
    mSegment.setSingleEssenceLocation(single_essence_location);
    mSegment.setForwardIndexDirection(forward_index_direction);
}

RDD9IndexTableSegment::~RDD9IndexTableSegment()
{
}

bool RDD9IndexTableSegment::RequireNewSegment(uint8_t can_start_partition)
{
    return mEntries.GetSize() >= MAX_INDEX_SEGMENT_SIZE ||
          (mEntries.GetSize() >= (MAX_INDEX_SEGMENT_SIZE - MAX_GOP_SIZE * mIndexEntrySize) && can_start_partition);
}

void RDD9IndexTableSegment::AddIndexEntry(const RDD9IndexEntry *entry, int64_t stream_offset,
                                          vector<uint32_t> slice_cp_offsets)
{
    BMX_ASSERT(mIndexEntrySize == 11 + slice_cp_offsets.size() * 4);
    mEntries.Grow(mIndexEntrySize);

    unsigned char *entry_bytes = mEntries.GetBytesAvailable();
    mxf_set_int8(entry->temporal_offset,  &entry_bytes[0]);
    mxf_set_int8(entry->key_frame_offset, &entry_bytes[1]);
    mxf_set_uint8(entry->flags,           &entry_bytes[2]);
    mxf_set_int64(stream_offset,          &entry_bytes[3]);
    size_t i;
    for (i = 0; i < slice_cp_offsets.size(); i++)
        mxf_set_uint32(slice_cp_offsets[i], &entry_bytes[11 + i * 4]);

    mEntries.IncrementSize(mIndexEntrySize);

    mSegment.incrementIndexDuration();
}

void RDD9IndexTableSegment::UpdateIndexEntry(int64_t segment_position, int8_t temporal_offset)
{
    BMX_ASSERT(segment_position * mIndexEntrySize < mEntries.GetSize());

    mxf_set_int8(temporal_offset, &mEntries.GetBytes()[segment_position * mIndexEntrySize]);
}

uint32_t RDD9IndexTableSegment::GetDuration() const
{
    return (uint32_t)mSegment.getIndexDuration();
}



RDD9IndexTable::RDD9IndexTable(uint32_t index_sid, uint32_t body_sid, Rational edit_rate, bool repeat_in_footer)
{
    mIndexSID = index_sid;
    mBodySID = body_sid;
    mEditRate = edit_rate;
    mRepeatInFooter = repeat_in_footer;
    mSingleIndexLocation = MXF_OPT_BOOL_NOT_PRESENT;
    mSingleEssenceLocation = MXF_OPT_BOOL_NOT_PRESENT;
    mForwardIndexDirection = MXF_OPT_BOOL_NOT_PRESENT;
    mSliceCount = 0;
    mIndexEntrySize = 0;
    mDuration = 0;
    mStreamOffset = 0;
}

RDD9IndexTable::~RDD9IndexTable()
{
    size_t i;
    for (i = 0; i < mIndexElements.size(); i++)
        delete mIndexElements[i];

    for (i = 0; i < mIndexSegments.size(); i++)
        delete mIndexSegments[i];
    for (i = 0; i < mWrittenIndexSegments.size(); i++)
        delete mWrittenIndexSegments[i];
}

void RDD9IndexTable::SetExtensions(mxfOptBool single_index_location, mxfOptBool single_essence_location,
                                   mxfOptBool forward_index_direction)
{
    mSingleIndexLocation   = single_index_location;
    mSingleEssenceLocation = single_essence_location;
    mForwardIndexDirection = forward_index_direction;
}

void RDD9IndexTable::RegisterSystemItem()
{
    mIndexElements.push_back(new RDD9IndexTableElement(0, RDD9IndexTableElement::SYSTEM_ITEM, true, false));
}

void RDD9IndexTable::RegisterPictureTrackElement(uint32_t track_index)
{
    mIndexElements.push_back(new RDD9IndexTableElement(track_index, RDD9IndexTableElement::PICTURE_ELEMENT, false, true));
    mIndexElementsMap[track_index] = mIndexElements.back();
}

void RDD9IndexTable::RegisterSoundTrackElement(uint32_t track_index)
{
    mIndexElements.push_back(new RDD9IndexTableElement(track_index, RDD9IndexTableElement::SOUND_ELEMENT, true, false));
    mIndexElementsMap[track_index] = mIndexElements.back();
}

void RDD9IndexTable::RegisterDataTrackElement(uint32_t track_index, bool is_cbe)
{
    mIndexElements.push_back(new RDD9IndexTableElement(track_index, RDD9IndexTableElement::DATA_ELEMENT, is_cbe, false));
    mIndexElementsMap[track_index] = mIndexElements.back();
}

void RDD9IndexTable::PrepareWrite()
{
    // order elements: system item - picture - sound - data elements
    stable_sort(mIndexElements.begin(), mIndexElements.end(), compare_element);

    mIndexEntrySize = 11;
    mSliceCount = 0;
    size_t i;
    for (i = 0; i < mIndexElements.size(); i++) {
        if (i > 0 && !mIndexElements[i - 1]->is_cbe) {
            mSliceCount++;
            mIndexEntrySize += 4;
        }
        mIndexElements[i]->slice_offset = mSliceCount;
    }

    mIndexSegments.push_back(new RDD9IndexTableSegment(mIndexSID, mBodySID, mEditRate, 0, mIndexEntrySize,
                                                       mSliceCount, mSingleIndexLocation, mSingleEssenceLocation,
                                                       mForwardIndexDirection));
}

void RDD9IndexTable::AddIndexEntry(uint32_t track_index, int64_t position, int8_t temporal_offset,
                                   int8_t key_frame_offset, uint8_t flags, bool can_start_partition)
{
    BMX_ASSERT(position >= mDuration);
    BMX_ASSERT(mIndexElementsMap.find(track_index) != mIndexElementsMap.end());

    mIndexElementsMap[track_index]->CacheIndexEntry(position, temporal_offset, key_frame_offset, flags,
                                                    can_start_partition);
}

void RDD9IndexTable::UpdateIndexEntry(uint32_t track_index, int64_t position, int8_t temporal_offset)
{
    BMX_ASSERT(position >= 0);
    BMX_ASSERT(mIndexElementsMap.find(track_index) != mIndexElementsMap.end());

    if (position >= mDuration) {
        mIndexElementsMap[track_index]->UpdateIndexEntry(position, temporal_offset);
    } else {
        int64_t end_offset = mDuration - position;
        size_t i = mIndexSegments.size() - 1;
        while (end_offset > mIndexSegments[i]->GetDuration()) {
            end_offset -= mIndexSegments[i]->GetDuration();
            i--;
        }
        mIndexSegments[i]->UpdateIndexEntry(mIndexSegments[i]->GetDuration() - end_offset, temporal_offset);
    }
}

bool RDD9IndexTable::CanStartPartition()
{
    size_t i;
    for (i = 0; i < mIndexElements.size(); i++) {
        if (!mIndexElements[i]->CanStartPartition(mDuration))
            return false;
    }

    return true;
}

void RDD9IndexTable::UpdateIndex(uint32_t size, const vector<uint32_t> &element_sizes)
{
    BMX_ASSERT(element_sizes.size() == mIndexElements.size());

    if (mDuration == 0)
        CreateDeltaEntries(element_sizes);
    else
        CheckDeltaEntries(element_sizes);

    UpdateVBEIndex(element_sizes);

    mDuration++;
    mStreamOffset += size;
}

bool RDD9IndexTable::HaveSegments()
{
    return !mIndexSegments.empty() && mIndexSegments[0]->GetDuration() > 0;
}

bool RDD9IndexTable::HaveWrittenSegments()
{
    return !mWrittenIndexSegments.empty();
}

void RDD9IndexTable::WriteSegments(File *mxf_file, Partition *partition)
{
    BMX_ASSERT(HaveSegments());
    BMX_ASSERT(mDuration > 0);

    partition->markIndexStart(mxf_file);

    if (partition->isFooter() && mRepeatInFooter)
        WriteVBESegments(mxf_file, mWrittenIndexSegments);
    WriteVBESegments(mxf_file, mIndexSegments);

    partition->fillToKag(mxf_file);
    partition->markIndexEnd(mxf_file);


    if (!partition->isFooter() && mRepeatInFooter) {
        size_t i;
        for (i = 0; i < mIndexSegments.size(); i++)
            mWrittenIndexSegments.push_back(mIndexSegments[i]);
    } else {
        size_t i;
        for (i = 0; i < mIndexSegments.size(); i++)
            delete mIndexSegments[i];
        if (partition->isFooter() && mRepeatInFooter) {
            for (i = 0; i < mWrittenIndexSegments.size(); i++)
                delete mWrittenIndexSegments[i];
            mWrittenIndexSegments.clear();
        }
    }
    mIndexSegments.clear();
}

void RDD9IndexTable::CreateDeltaEntries(const vector<uint32_t> &element_sizes)
{
    mDeltaEntries.clear();

    uint8_t prev_slice_offset = 0;
    uint32_t element_delta = 0;
    size_t i;
    for (i = 0; i < mIndexElements.size(); i++) {
        if (mIndexElements[i]->slice_offset != prev_slice_offset)
            element_delta = 0;

        RDD9DeltaEntry entry;
        entry.pos_table_index = (mIndexElements[i]->apply_temporal_reordering ? -1 : 0);
        entry.slice           = mIndexElements[i]->slice_offset;
        entry.element_delta   = element_delta;
        mDeltaEntries.push_back(entry);

        prev_slice_offset = mIndexElements[i]->slice_offset;
        element_delta += element_sizes[i];

        if (mIndexElements[i]->is_cbe)
            mIndexElements[i]->element_size = element_sizes[i];
    }
    if (mDeltaEntries.size() == 1 &&
        mDeltaEntries[0].pos_table_index == 0 &&
        mDeltaEntries[0].slice == 0 &&
        mDeltaEntries[0].element_delta == 0)
    {
        // no need for delta entry array
        mDeltaEntries.clear();
    }
}

void RDD9IndexTable::CheckDeltaEntries(const vector<uint32_t> &element_sizes)
{
    size_t i;
    for (i = 0; i < mIndexElements.size(); i++) {
        if (mIndexElements[i]->is_cbe) {
            BMX_CHECK_M(mIndexElements[i]->element_size == element_sizes[i],
                       ("The size of a fixed size element data in content package changed"));
        }
    }
}

void RDD9IndexTable::UpdateVBEIndex(const vector<uint32_t> &element_sizes)
{
    bool can_start_partition = CanStartPartition(); // check before any TakeIndexEntry calls

    uint32_t slice_cp_offset = 0;
    vector<uint32_t> slice_cp_offsets;
    uint8_t prev_slice_offset = 0;
    RDD9IndexEntry entry;
    size_t i;
    for (i = 0; i < mIndexElements.size(); i++) {
        // get non-default entry if exists
        RDD9IndexEntry element_entry;
        if (mIndexElements[i]->TakeIndexEntry(mDuration, &element_entry) && !element_entry.IsDefault()) {
            BMX_CHECK(entry.IsCompatible(element_entry));
            entry = element_entry;
        }

        if (mIndexElements[i]->slice_offset != prev_slice_offset) {
            slice_cp_offsets.push_back(slice_cp_offset);
            prev_slice_offset = mIndexElements[i]->slice_offset;
        }
        slice_cp_offset += element_sizes[i];
    }

    if (mIndexSegments.empty() || mIndexSegments.back()->RequireNewSegment(can_start_partition)) {
        mIndexSegments.push_back(new RDD9IndexTableSegment(mIndexSID, mBodySID, mEditRate, mDuration,
                                                           mIndexEntrySize, mSliceCount, mSingleIndexLocation,
                                                           mSingleEssenceLocation, mForwardIndexDirection));
    }

    mIndexSegments.back()->AddIndexEntry(&entry, mStreamOffset, slice_cp_offsets);
}

void RDD9IndexTable::WriteVBESegments(File *mxf_file, vector<RDD9IndexTableSegment*> &segments)
{
    size_t i;
    for (i = 0; i < segments.size(); i++) {
        IndexTableSegment *segment = segments[i]->GetSegment();
        ByteArray *entries = segments[i]->GetEntries();

        mxfUUID uuid;
        mxf_generate_uuid(&uuid);
        segment->setInstanceUID(uuid);

        // Note: RDD9 states that PosTableCount is not encoded but mxf_write_index_table_segment will write it with
        //       the default value 0
        segment->writeHeader(mxf_file, (uint32_t)mDeltaEntries.size(), (uint32_t)segment->getIndexDuration());

        if (!mDeltaEntries.empty()) {
            segment->writeDeltaEntryArrayHeader(mxf_file, (uint32_t)mDeltaEntries.size());
            size_t j;
            for (j = 0; j < mDeltaEntries.size(); j++) {
                segment->writeDeltaEntry(mxf_file, mDeltaEntries[j].pos_table_index, mDeltaEntries[j].slice,
                                         mDeltaEntries[j].element_delta);
            }
        }

        segment->writeIndexEntryArrayHeader(mxf_file, mSliceCount, 0, (uint32_t)segment->getIndexDuration());
        mxf_file->write(entries->GetBytes(), entries->GetSize());
    }
}

