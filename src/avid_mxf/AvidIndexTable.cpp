/*
 * Copyright (C) 2018, British Broadcasting Corporation
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

#include <bmx/MXFUtils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include "bmx/avid_mxf/AvidIndexTable.h"

using namespace std;
using namespace bmx;
using namespace mxfpp;



// MAX_INDEX_SEGMENT_SIZE <= 65535 [2-byte max len]
#define MAX_INDEX_SEGMENT_SIZE      0xffff
#define INDEX_ENTRY_SIZE            11



AvidIndexEntry::AvidIndexEntry()
    : stream_offset(0)
    , temporal_offset(0)
    , key_frame_offset(0)
    , flags(0)
    , can_start_partition(true)
{

}

AvidIndexEntry::AvidIndexEntry(int8_t temporal_offset_, int8_t key_frame_offset_,
        uint8_t flags_, int64_t stream_offset_, bool can_start_partition_)
    : stream_offset(stream_offset_)
    , temporal_offset(temporal_offset_)
    , key_frame_offset(key_frame_offset_)
    , flags(flags_)
    , can_start_partition(can_start_partition_)
{

}


AvidIndexTable::AvidIndexTable(uint32_t index_sid, uint32_t body_sid, mxfRational edit_rate)
    : mIndexSID(index_sid)
    , mBodySID(body_sid)
    , mEditRate(edit_rate)
{

}

AvidIndexTable::~AvidIndexTable()
{

}


void AvidIndexTable::AddIndexEntry(int64_t position, int8_t temporal_offset,
                                   int8_t key_frame_offset, uint8_t flags,
                                   int64_t stream_offset, bool can_start_partition,
                                   bool require_update)
{
    uint32_t index_pos = (uint32_t)position;

    BMX_ASSERT(index_pos >= mIndexEntries.size());

    if (index_pos >= mIndexEntries.size())
        mIndexEntries.resize(index_pos + 1);
    mIndexEntries[index_pos] = AvidIndexEntry(temporal_offset, key_frame_offset,
            flags, stream_offset, can_start_partition);
    if (require_update)
        require_updates.insert(index_pos);
}

void AvidIndexTable::UpdateIndexEntry(int64_t position, int8_t temporal_offset,
                                      int8_t key_frame_offset, uint8_t flags)
{
    BMX_ASSERT(position >= 0);
    uint32_t index_pos = (uint32_t)position;

    // must update an already existing entry
    BMX_ASSERT(index_pos < mIndexEntries.size());

    mIndexEntries[index_pos].temporal_offset = temporal_offset;
    mIndexEntries[index_pos].key_frame_offset = key_frame_offset;
    mIndexEntries[index_pos].flags = flags;

    require_updates.erase(index_pos);
}


void AvidIndexTable::WriteVBEIndexTable(mxfpp::File *mxf_file, mxfpp::Partition *partition)
{
    partition->markIndexStart(mxf_file);

    // separate index table into segments of (MAX_INDEX_SEGMENT_SIZE / INDEX_ENTRY_SIZE) entries;
    // each entry _should_ start with an I-frame (can_start_partition == true)
    uint32_t begin = 0;
    uint32_t end = (uint32_t)mIndexEntries.size();

    while (begin != end) {
        uint32_t seg_end = min(begin + MAX_INDEX_SEGMENT_SIZE / INDEX_ENTRY_SIZE, end);

        if (seg_end != end) {
            for (uint32_t pos = seg_end; pos > begin; pos--) {
                 if (mIndexEntries[pos].can_start_partition) {
                     seg_end = pos;
                     break;
                 }
            }
        }

        WriteIndexSegmentHeader(mxf_file, begin, seg_end);
        WriteIndexSegmentArray(mxf_file, begin, seg_end);
        begin = seg_end;
    }

    partition->fillToKag(mxf_file);
    partition->markIndexEnd(mxf_file);
}

void AvidIndexTable::WriteIndexSegmentHeader(mxfpp::File *mxf_file, uint32_t begin, uint32_t end)
{
    const uint32_t num_index_entries = end - begin;
    BMX_ASSERT(num_index_entries >= 1);

    mxfUUID uuid;
    mxf_generate_uuid(&uuid);

    IndexTableSegment segment;
    segment.setIndexEditRate(mEditRate);
    segment.setIndexSID(mIndexSID);
    segment.setBodySID(mBodySID);
    segment.setEditUnitByteCount(0);
    segment.setInstanceUID(uuid);
    segment.setIndexStartPosition(begin);
    segment.setIndexDuration(num_index_entries);
    segment.writeHeader(mxf_file, 0, num_index_entries);
    segment.writeAvidIndexEntryArrayHeader(mxf_file, 0, 0, num_index_entries);
}

void AvidIndexTable::WriteIndexSegmentArray(mxfpp::File *mxf_file, uint32_t begin, uint32_t end)
{
    BMX_ASSERT(begin < end);
    BMX_ASSERT(end <= mIndexEntries.size());

    const uint32_t size = end - begin;
    ByteArray index_segment(size * INDEX_ENTRY_SIZE);
    index_segment.SetSize(size * INDEX_ENTRY_SIZE);

    for (uint32_t pos = 0; pos != size; pos++) {
        AvidIndexEntry const& entry = mIndexEntries[pos + begin];
        unsigned char* bytes = index_segment.GetBytes() + pos * INDEX_ENTRY_SIZE;

        mxf_set_int8(entry.temporal_offset * 2,  bytes + 0);    // temporal offset
        mxf_set_int8(entry.key_frame_offset * 2, bytes + 1);    // key frame offset
        mxf_set_uint8(entry.flags & 0x80,        bytes + 2);    // flags
        mxf_set_int64(entry.stream_offset,       bytes + 3);    // stream offset
    }

    mxf_file->write(index_segment.GetBytes(), index_segment.GetSize());
}
