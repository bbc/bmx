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

#ifndef BMX_INDEX_TABLE_HELPER_H_
#define BMX_INDEX_TABLE_HELPER_H_

#include <vector>
#include <memory>

#include <libMXF++/MXF.h>

#include <bmx/mxf_reader/MXFIndexEntryExt.h>



namespace bmx
{


class MXFFileReader;


class IndexTableHelperSegment : public mxfpp::IndexTableSegment
{
public:
    IndexTableHelperSegment();
    virtual ~IndexTableHelperSegment();

    void ReadIndexTableSegment(mxfpp::File *file, uint64_t segment_len);
    void ProcessIndexTableSegment(Rational expected_edit_rate);

    bool HaveExtraIndexEntries() const { return mHaveExtraIndexEntries; }
    int64_t GetIndexEndOffset() const  { return mIndexEndOffset; }

    int GetEditUnit(int64_t position, int8_t *temporal_offset, int8_t *key_frame_offset, uint8_t *flags,
                    int64_t *stream_offset);

public:
    bool CanAppendIndexEntry() const;
    void AppendIndexEntry(uint32_t num_entries, int8_t temporal_offset, int8_t key_frame_offset, uint8_t flags,
                          int64_t stream_offset);

    bool HaveConstantEditUnitSize();
    uint32_t GetEditUnitSize();

    void SetEssenceStartOffset(int64_t offset);
    int64_t GetEssenceEndOffset();

    bool IsFileIndexSegment() const { return mIsFileIndexSegment; }

    void SetHavePairedIndexEntries();
    bool HavePairedIndexEntries() const { return mHavePairedIndexEntries; }

    void UpdateStartPosition(int64_t position);
    void UpdateDuration(int64_t duration);

    void CopyIndexEntries(const IndexTableHelperSegment *segment, uint32_t duration);

private:
    unsigned char *mIndexEntries;
    uint32_t mAllocIndexEntries;
    uint32_t mNumIndexEntries;
    uint32_t mEntriesStart;

    bool mHaveExtraIndexEntries;
    int64_t mIndexEndOffset;
    bool mHavePairedIndexEntries;

    int64_t mEssenceStartOffset;

    bool mIsFileIndexSegment;
};


class IndexTableHelper
{
public:
    IndexTableHelper(MXFFileReader *file_reader);
    ~IndexTableHelper();

    void ExtractIndexTable();

    void SetEssenceDataSize(int64_t size);

    void SetEditRate(Rational edit_rate);
    void SetConstantEditUnitSize(Rational edit_rate, uint32_t size);

    int64_t ReadIndexTableSegment(uint64_t len);
    void UpdateIndex(int64_t position, int64_t essence_offset, int64_t size);
    void SetIsComplete();

public:
    bool IsComplete() const { return mIsComplete; }

    Rational GetEditRate() const { return mEditRate; }
    int64_t GetDuration() const  { return mDuration; }

    bool HaveConstantEditUnitSize() const { return mEditUnitSize > 0; }
    uint32_t GetEditUnitSize()      const { return mEditUnitSize; }

    bool HaveEditUnit(int64_t position) const;
    void GetEditUnit(int64_t position, int8_t *temporal_offset, int8_t *key_frame_offset, uint8_t *flags,
                     int64_t *offset, int64_t *size = 0);
    void GetEditUnit(int64_t position, int64_t *offset, int64_t *size);
    bool HaveEditUnitOffset(int64_t position) const;
    int64_t GetEditUnitOffset(int64_t position);
    bool HaveEditUnitSize(int64_t position) const;

    bool GetTemporalReordering(uint32_t element_index);

    bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position);

private:
    void InsertCBEIndexSegment(std::auto_ptr<IndexTableHelperSegment> &new_segment_ap);
    void InsertVBEIndexSegment(std::auto_ptr<IndexTableHelperSegment> &new_segment_ap);

    IndexTableHelperSegment* CreateStartSegment(IndexTableHelperSegment *segment, uint32_t duration);

private:
    MXFFileReader *mFileReader;
    mxfpp::File *mFile;

    bool mIsComplete;

    std::vector<IndexTableHelperSegment*> mSegments;
    size_t mLastEditUnitSegment;

    uint32_t mEditUnitSize;

    int64_t mEssenceDataSize;

    Rational mEditRate;
    int64_t mDuration;
};


};



#endif

