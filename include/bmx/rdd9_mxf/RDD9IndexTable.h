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

#ifndef BMX_RDD9_INDEX_TABLE_H_
#define BMX_RDD9_INDEX_TABLE_H_

#include <vector>

#include <bmx/ByteArray.h>



namespace bmx
{


class RDD9File;


class RDD9IndexEntry
{
public:
    RDD9IndexEntry();
    RDD9IndexEntry(int8_t temporal_offset_, int8_t key_frame_offset_, uint8_t flags_, bool can_start_partition_);

    bool IsDefault();
    bool IsCompatible(const RDD9IndexEntry &entry);

public:
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    bool can_start_partition;
};


class RDD9DeltaEntry
{
public:
    RDD9DeltaEntry();

    int8_t pos_table_index;
    uint8_t slice;
    uint32_t element_delta;
};


class RDD9IndexTableElement
{
public:
    typedef enum
    {
        SYSTEM_ITEM,
        PICTURE_ELEMENT,
        SOUND_ELEMENT,
        DATA_ELEMENT,
    } ElementType;

public:
    RDD9IndexTableElement(uint32_t track_index_, ElementType element_type_, bool is_cbe_,
                          bool apply_temporal_reordering_);

    void CacheIndexEntry(int64_t position, int8_t temporal_offset, int8_t key_frame_offset, uint8_t flags,
                         bool can_start_partition);
    void UpdateIndexEntry(int64_t position, int8_t temporal_offset);
    bool TakeIndexEntry(int64_t position, RDD9IndexEntry *entry);

    bool CanStartPartition(int64_t position);

public:
    uint32_t track_index;
    ElementType element_type;
    bool is_cbe;
    bool apply_temporal_reordering;

    uint8_t slice_offset;

    uint32_t element_size;

private:
    std::map<int64_t, RDD9IndexEntry> mIndexEntryCache;
};


class RDD9IndexTableSegment
{
public:
    RDD9IndexTableSegment(uint32_t index_sid, uint32_t body_sid, Rational edit_rate, int64_t start_position,
                          uint32_t index_entry_size, uint32_t slice_count,
                          mxfOptBool single_index_location, mxfOptBool single_essence_location,
                          mxfOptBool forward_index_direction);
    ~RDD9IndexTableSegment();

    bool RequireNewSegment(uint8_t flags);
    void AddIndexEntry(const RDD9IndexEntry *entry, int64_t stream_offset, std::vector<uint32_t> slice_cp_offsets);
    void UpdateIndexEntry(int64_t segment_position, int8_t temporal_offset);

    void AddCBEIndexEntries(uint32_t edit_unit_byte_count, uint32_t num_entries);

    uint32_t GetDuration() const;

    mxfpp::IndexTableSegment* GetSegment() { return &mSegment; }
    ByteArray* GetEntries() { return &mEntries; }

private:
    mxfpp::IndexTableSegment mSegment;
    ByteArray mEntries;
    uint32_t mIndexEntrySize;
};


class RDD9IndexTable
{
public:
    RDD9IndexTable(uint32_t index_sid, uint32_t body_sid, Rational edit_rate, bool repeat_in_footer);
    ~RDD9IndexTable();

    void SetExtensions(mxfOptBool single_index_location, mxfOptBool single_essence_location,
                       mxfOptBool forward_index_direction);

    void RegisterSystemItem();
    void RegisterPictureTrackElement(uint32_t track_index);
    void RegisterSoundTrackElement(uint32_t track_index);
    void RegisterDataTrackElement(uint32_t track_index, bool is_cbe);

    void PrepareWrite();

public:
    int64_t GetDuration() const     { return mDuration; }
    int64_t GetStreamOffset() const { return mStreamOffset; }

public:
    void AddIndexEntry(uint32_t track_index, int64_t position, int8_t temporal_offset,
                       int8_t key_frame_offset, uint8_t flags, bool can_start_partition);
    void UpdateIndexEntry(uint32_t track_index, int64_t position, int8_t temporal_offset);

    bool CanStartPartition();

    void UpdateIndex(uint32_t size, const std::vector<uint32_t> &element_sizes);

public:
    bool HaveSegments();
    bool HaveWrittenSegments();
    void WriteSegments(mxfpp::File *mxf_file, mxfpp::Partition *partition);

private:
    void CreateDeltaEntries(const std::vector<uint32_t> &element_sizes);
    void CheckDeltaEntries(const std::vector<uint32_t> &element_sizes);

    void UpdateVBEIndex(const std::vector<uint32_t> &element_sizes);

    void WriteVBESegments(mxfpp::File *mxf_file, std::vector<RDD9IndexTableSegment*> &segments);

private:
    uint32_t mIndexSID;
    uint32_t mBodySID;
    mxfOptBool mSingleIndexLocation;
    mxfOptBool mSingleEssenceLocation;
    mxfOptBool mForwardIndexDirection;
    Rational mEditRate;
    bool mRepeatInFooter;

    std::vector<RDD9IndexTableElement*> mIndexElements;
    std::map<uint32_t, RDD9IndexTableElement*> mIndexElementsMap;
    uint8_t mSliceCount;
    uint32_t mIndexEntrySize;

    std::vector<RDD9DeltaEntry> mDeltaEntries;

    std::vector<RDD9IndexTableSegment*> mIndexSegments;
    int64_t mDuration;
    int64_t mStreamOffset;

    std::vector<RDD9IndexTableSegment*> mWrittenIndexSegments;
};


};



#endif

