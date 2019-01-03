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

#ifndef BMX_AVID_INDEX_TABLE_H_
#define BMX_AVID_INDEX_TABLE_H_

#include <deque>
#include <set>

#include <libMXF++/MXF.h>

#include <bmx/ByteArray.h>


namespace bmx
{


class AvidIndexEntry
{
public:
    AvidIndexEntry();
    AvidIndexEntry(int8_t temporal_offset_, int8_t key_frame_offset_, uint8_t flags_,
                   int64_t stream_offset_, bool can_start_partition_);

public:
    int64_t stream_offset;
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    bool can_start_partition;
};


class AvidIndexTable
{
public:
    AvidIndexTable(uint32_t index_sid, uint32_t body_sid, mxfRational edit_rate);
    ~AvidIndexTable();

public:
    void AddIndexEntry(int64_t position, int8_t temporal_offset,
                       int8_t key_frame_offset, uint8_t flags,
                       int64_t stream_offset, bool can_start_partition,
                       bool require_update);
    void UpdateIndexEntry(int64_t position, int8_t temporal_offset,
                          int8_t key_frame_offset, uint8_t flags);

public:
    void WriteVBEIndexTable(mxfpp::File *mxf_file, mxfpp::Partition *partition);

private:
    void WriteIndexSegmentHeader(mxfpp::File *mxf_file, uint32_t begin, uint32_t end);
    void WriteIndexSegmentArray(mxfpp::File *mxf_file, uint32_t begin, uint32_t end);

private:
    uint32_t mIndexSID;
    uint32_t mBodySID;
    mxfRational mEditRate;

    std::deque<AvidIndexEntry> mIndexEntries;
    std::set<uint32_t> require_updates;
};


};



#endif
