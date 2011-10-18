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

#ifndef __IM_INDEX_TABLE_HELPER_H__
#define __IM_INDEX_TABLE_HELPER_H__

#include <vector>

#include <libMXF++/MXF.h>

#include <im/mxf_reader/MXFIndexEntryExt.h>



namespace im
{


class MXFReader;


class IndexTableHelperSegment : public mxfpp::IndexTableSegment
{
public:
    IndexTableHelperSegment();
    virtual ~IndexTableHelperSegment();

    void ParseIndexTableSegment(mxfpp::File *file, uint64_t segment_len);

    bool HaveExtraIndexEntries() const { return mHaveExtraIndexEntries; }
    int64_t GetIndexEndOffset() const { return mIndexEndOffset; }

    int GetEditUnit(int64_t position, int8_t *temporal_offset, int8_t *key_frame_offset, uint8_t *flags,
                    int64_t *stream_offset);

public:
    void AppendIndexEntry(uint32_t num_entries, int8_t temporal_offset, int8_t key_frame_offset, uint8_t flags,
                          uint64_t stream_offset);

    bool HaveFixedEditUnitByteCount();
    uint32_t GetFixedEditUnitByteCount();

    void SetEssenceStartOffset(int64_t offset);
    int64_t GetEssenceEndOffset();

private:
    unsigned char *mIndexEntries;
    uint32_t mAllocIndexEntries;
    uint32_t mNumIndexEntries;

    bool mHaveExtraIndexEntries;
    int64_t mIndexEndOffset;
    bool mHavePairedIndexEntries;

    int64_t mEssenceStartOffset;
};


class IndexTableHelper
{
public:
    IndexTableHelper(MXFReader *parent_reader);
    ~IndexTableHelper();

    void ExtractIndexTable();
    void SetEssenceDataSize(int64_t size);

    mxfRational GetEditRate();
    int64_t GetDuration() { return mDuration; }

    bool HaveFixedEditUnitByteCount() const { return mHaveFixedEditUnitByteCount; }
    uint32_t GetFixedEditUnitByteCount() const { return mFixedEditUnitByteCount; }

    void GetEditUnit(int64_t position, int8_t *temporal_offset, int8_t *key_frame_offset, uint8_t *flags,
                     int64_t *offset, int64_t *size);
    void GetEditUnit(int64_t position, int64_t *offset, int64_t *size);
    int64_t GetEditUnitOffset(int64_t position);

    bool GetTemporalReordering(uint32_t element_index);

    bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position);

private:
    MXFReader *mParentReader;

    std::vector<IndexTableHelperSegment*> mSegments;
    size_t mLastEditUnitSegment;

    bool mHaveFixedEditUnitByteCount;
    uint32_t mFixedEditUnitByteCount;

    int64_t mEssenceDataSize;
    
    int64_t mDuration;
};


};



#endif

