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

#include <cstdio>
#include <cstring>

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

#include <bmx/mxf_reader/IndexTableHelper.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


// temporal offset (1) + key frame offset (1) + flags (1) + stream offset (8)
#define INTERNAL_INDEX_ENTRY_SIZE   11

#define RUNTIME_INDEX_SEGMENT_SIZE  1500

#define GET_TEMPORAL_OFFSET(pos)    (*((int8_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE]))
#define GET_KEY_FRAME_OFFSET(pos)   (*((int8_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE + 1]))
#define GET_FLAGS(pos)              (*((uint8_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE + 2]))
#define GET_STREAM_OFFSET(pos)      (*((int64_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE + 3]))

#define SEG_END(seg)    (seg->getIndexStartPosition() + seg->getIndexDuration())
#define SEG_START(seg)  (seg->getIndexStartPosition())
#define SEG_DUR(seg)    (seg->getIndexDuration())



static int add_frame_offset_index_entry(void *data, uint32_t num_entries, MXFIndexTableSegment *segment,
                                        int8_t temporal_offset, int8_t key_frame_offset, uint8_t flags,
                                        uint64_t stream_offset, uint32_t *slice_offset, mxfRational *pos_table)
{
    IndexTableHelperSegment *helper = static_cast<IndexTableHelperSegment*>(data);

    (void)segment;
    (void)slice_offset;
    (void)pos_table;

    // file offsets use int64_t whilst the index segment stream offset is uint64_t
    BMX_CHECK(stream_offset <= INT64_MAX);

    helper->AppendIndexEntry(num_entries, temporal_offset, key_frame_offset, flags, (int64_t)stream_offset);

    return 1;
}



IndexTableHelperSegment::IndexTableHelperSegment()
: IndexTableSegment()
{
    mIndexEntries = 0;
    mAllocIndexEntries = 0;
    mNumIndexEntries = 0;
    mEntriesStart = 0;
    mHaveExtraIndexEntries = false;
    mHavePairedIndexEntries = false;
    mIndexEndOffset = 0;
    mEssenceStartOffset = 0;
    mIsFileIndexSegment = false;
}

IndexTableHelperSegment::~IndexTableHelperSegment()
{
    delete [] mIndexEntries;
}

void IndexTableHelperSegment::ReadIndexTableSegment(File *file, uint64_t segment_len)
{
    mIsFileIndexSegment = true;

    // free existing segment which will be replaced
    mxf_free_index_table_segment(&_cSegment);

    // use Avid function to support non-standard Avid index table segments
    BMX_CHECK(mxf_avid_read_index_table_segment_2(file->getCFile(), segment_len,
                                                  mxf_default_add_delta_entry, 0,
                                                  add_frame_offset_index_entry, this,
                                                  &_cSegment));
}

void IndexTableHelperSegment::ProcessIndexTableSegment(Rational expected_edit_rate)
{
    setIndexEditRate(normalize_rate(getIndexEditRate()));

    if (expected_edit_rate.numerator != 0 && expected_edit_rate != getIndexEditRate()) {
        BMX_EXCEPTION(("Index table segment edit rate %d/%d differs from expected edit rate %d/%d",
                       getIndexEditRate().numerator, getIndexEditRate().denominator,
                       expected_edit_rate.numerator, expected_edit_rate.denominator));
    }

    // samples files produced by Ardendo product 'ardftp' had an invalid index duration -1
    if (getIndexDuration() < 0) {
        log_warn("Index duration %"PRId64" is invalid. Assuming index duration 0 instead.\n", getIndexDuration());
        setIndexDuration(0);
    }

    if (getEditUnitByteCount() == 0) {
        if (getIndexDuration() == 0) {
            if (mNumIndexEntries > 0)
                BMX_EXCEPTION(("VBE index table segment with index entries but unknown duration is invalid"));
        } else if ((int64_t)mNumIndexEntries < getIndexDuration()) {
            if ((int64_t)mNumIndexEntries > 0) {
                BMX_EXCEPTION(("VBE index table is missing index entries"));
            } else {
                BMX_EXCEPTION(("CBE index table edit unit byte count is invalid or "
                               "VBE index table is missing index entries"));
            }
        }
    } else if (mNumIndexEntries > 0) {
        BMX_EXCEPTION(("VBE index table with non-zero edit unit byte count or "
                       "CBE index table with index entries is invalid"));
    }

    if (mNumIndexEntries > 1 && mNumIndexEntries > getIndexDuration()) {
        // Avid adds an extra entry which provides the end offset or size for the last frame
        if (GET_STREAM_OFFSET(0) == GET_STREAM_OFFSET(1)) {
            // eg. Avid MPEG-2 Long GOP (eg. XDCAM) has 2 identical entries per frame
            mHavePairedIndexEntries = true;
            if (mNumIndexEntries > getIndexDuration() * 2) {
                mHaveExtraIndexEntries = true;
                mIndexEndOffset = GET_STREAM_OFFSET(getIndexDuration() * 2);
            }
        } else {
            mHaveExtraIndexEntries = true;
            mIndexEndOffset = GET_STREAM_OFFSET(getIndexDuration());
        }
    }
}

int IndexTableHelperSegment::GetEditUnit(int64_t position, int8_t *temporal_offset, int8_t *key_frame_offset,
                                         uint8_t *flags, int64_t *stream_offset)
{
    if (position < getIndexStartPosition() ||
        (getIndexDuration() > 0 && position >= getIndexStartPosition() + getIndexDuration()))
    {
        return position < getIndexStartPosition() ? -2 : -1;
    }

    int64_t rel_position = position - getIndexStartPosition();
    if (mNumIndexEntries == 0) {
        *temporal_offset  = 0;
        *key_frame_offset = 0;
        *flags            = 0x80; // reference frame
        *stream_offset    = mEssenceStartOffset + rel_position * getEditUnitByteCount();
    } else {
        if (mHavePairedIndexEntries)
            rel_position *= 2;

        *temporal_offset  = GET_TEMPORAL_OFFSET(rel_position);
        *key_frame_offset = GET_KEY_FRAME_OFFSET(rel_position);
        *flags            = GET_FLAGS(rel_position);
        *stream_offset    = GET_STREAM_OFFSET(rel_position);

        if (mHavePairedIndexEntries) {
            *temporal_offset  /= 2;
            *key_frame_offset /= 2;
        }
    }

    return 0;
}

bool IndexTableHelperSegment::CanAppendIndexEntry() const
{
    return mEntriesStart + mNumIndexEntries < mAllocIndexEntries;
}

void IndexTableHelperSegment::AppendIndexEntry(uint32_t num_entries, int8_t temporal_offset, int8_t key_frame_offset,
                                               uint8_t flags, int64_t stream_offset)
{
    if (!mIndexEntries) {
        BMX_CHECK(num_entries > 0);
        mIndexEntries = new unsigned char[INTERNAL_INDEX_ENTRY_SIZE * num_entries];
        mAllocIndexEntries = num_entries;
    }

    BMX_CHECK(CanAppendIndexEntry());

    int64_t entry_pos = mEntriesStart + mNumIndexEntries;
    GET_TEMPORAL_OFFSET(entry_pos)  = temporal_offset;
    GET_KEY_FRAME_OFFSET(entry_pos) = key_frame_offset;
    GET_FLAGS(entry_pos)            = flags;
    GET_STREAM_OFFSET(entry_pos)    = stream_offset;

    mNumIndexEntries++;
}

bool IndexTableHelperSegment::HaveConstantEditUnitSize()
{
    return getEditUnitByteCount() > 0;
}

uint32_t IndexTableHelperSegment::GetEditUnitSize()
{
    return getEditUnitByteCount();
}

void IndexTableHelperSegment::SetEssenceStartOffset(int64_t offset)
{
    BMX_ASSERT(mNumIndexEntries == 0);
    mEssenceStartOffset = offset;
}

int64_t IndexTableHelperSegment::GetEssenceEndOffset()
{
    BMX_ASSERT(mNumIndexEntries == 0 && getIndexDuration() > 0);
    return mEssenceStartOffset + getEditUnitByteCount() * getIndexDuration();
}

void IndexTableHelperSegment::SetHavePairedIndexEntries()
{
    mHavePairedIndexEntries = true;
}

void IndexTableHelperSegment::UpdateStartPosition(int64_t position)
{
    BMX_ASSERT(mNumIndexEntries > 0);
    BMX_ASSERT(position >= getIndexStartPosition());
    BMX_ASSERT(position - getIndexStartPosition() < getIndexDuration());

    uint32_t diff_entries = (uint32_t)(getIndexStartPosition() - position);
    if (mHavePairedIndexEntries)
        diff_entries *= 2;
    mEntriesStart += diff_entries;
    setIndexStartPosition(position);
}

void IndexTableHelperSegment::UpdateDuration(int64_t duration)
{
    BMX_ASSERT(mNumIndexEntries > 0);
    BMX_ASSERT(duration > 0 && duration <= getIndexDuration());

    uint32_t diff_entries = (uint32_t)(getIndexDuration() - duration);
    if (mHavePairedIndexEntries)
        diff_entries *= 2;
    mNumIndexEntries -= diff_entries;
    setIndexDuration(duration);
}

void IndexTableHelperSegment::CopyIndexEntries(const IndexTableHelperSegment *from_segment, uint32_t duration)
{
    BMX_ASSERT(!mIndexEntries);
    BMX_ASSERT(from_segment->mIndexEntries);

    uint32_t num_entries = duration;
    if (from_segment->mHavePairedIndexEntries)
        num_entries *= 2;

    mIndexEntries = new unsigned char[INTERNAL_INDEX_ENTRY_SIZE * num_entries];
    memcpy(mIndexEntries, from_segment->mIndexEntries, INTERNAL_INDEX_ENTRY_SIZE * num_entries);
    mAllocIndexEntries = num_entries;
    mNumIndexEntries = num_entries;
    mHavePairedIndexEntries = from_segment->mHavePairedIndexEntries;
}




IndexTableHelper::IndexTableHelper(MXFFileReader *file_reader)
{
    mFileReader = file_reader;
    mFile = file_reader->mFile;
    mIsComplete = false;
    mLastEditUnitSegment = 0;
    mEditUnitSize = 0;
    mEssenceDataSize = 0;
    mEditRate = ZERO_RATIONAL;
    mDuration = 0;
}

IndexTableHelper::~IndexTableHelper()
{
    size_t i;
    for (i = 0; i < mSegments.size(); i++)
        delete mSegments[i];
}

void IndexTableHelper::ExtractIndexTable()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    const vector<Partition*> &partitions = mFileReader->mFile->getPartitions();
    size_t i;
    for (i = 0; i < partitions.size(); i++) {
        const Partition *partition = partitions[i];

        if (partition->getIndexSID() != mFileReader->mIndexSID)
            continue;

        // find the start of the first index table segment
        mFile->seek(partition->getThisPartition(), SEEK_SET);
        mFile->readKL(&key, &llen, &len);
        mFile->skip(len);
        while (true)
        {
            mFile->readNextNonFillerKL(&key, &llen, &len);

            if (mxf_is_partition_pack(&key) || mxf_is_index_table_segment(&key))
                break;
            else if (mxf_is_header_metadata(&key) && partition->getHeaderByteCount() > mxfKey_extlen + llen + len)
                mFile->skip(partition->getHeaderByteCount() - (mxfKey_extlen + llen));
            else
                mFile->skip(len);
        }

        // parse the index table segments
        if (mxf_is_index_table_segment(&key)) {
            uint64_t num_read = mxfKey_extlen + llen;
            uint64_t index_byte_count = partition->getIndexByteCount();
            while (true)
            {
                if (mxf_is_index_table_segment(&key)) {
                    ReadIndexTableSegment(len);
                } else if (mxf_is_filler(&key)) {
                    mFile->skip(len);
                } else {
                    num_read -= mxfKey_extlen + llen;
                    break;
                }
                num_read += len;

                if (index_byte_count > 0 && num_read >= index_byte_count)
                    break;
                if (index_byte_count == 0 && i == partitions.size() - 1 && mFile->tell() >= mFile->size())
                    break;

                mFile->readKL(&key, &llen, &len);
                num_read += mxfKey_extlen + llen;
            }
            if (index_byte_count > 0 && num_read != index_byte_count) {
                log_warn("Index byte count %"PRIu64" does not equal value in partition pack %"PRIu64"\n",
                         num_read, index_byte_count);
            }
        } else {
            log_warn("Failed to find an index table segment in partition with IndexSID = %u\n", mFileReader->mIndexSID);
        }
    }

    mIsComplete = !mSegments.empty();
}

void IndexTableHelper::SetEssenceDataSize(int64_t size)
{
    mEssenceDataSize = size;

    // calc index duration if unknown and edit unit size is constant
    if (mDuration == 0 && mEditUnitSize > 0)
        mDuration = size / mEditUnitSize;
}

void IndexTableHelper::SetEditRate(Rational edit_rate)
{
    BMX_ASSERT(mSegments.empty());

    mEditRate = edit_rate;
}

void IndexTableHelper::SetConstantEditUnitSize(Rational edit_rate, uint32_t size)
{
    BMX_ASSERT(mSegments.empty());
    BMX_ASSERT(size > 0);

    mSegments.push_back(new IndexTableHelperSegment());
    IndexTableHelperSegment *segment = mSegments.back();
    segment->setIndexEditRate(edit_rate);
    segment->setEditUnitByteCount(size);

    mEditRate = edit_rate;
    mEditUnitSize = size;
}

int64_t IndexTableHelper::ReadIndexTableSegment(uint64_t len)
{
    auto_ptr<IndexTableHelperSegment> new_segment(new IndexTableHelperSegment());
    new_segment->ReadIndexTableSegment(mFileReader->mFile, len);
    try
    {
        new_segment->ProcessIndexTableSegment(mEditRate);
    }
    catch (const BMXException&)
    {
        log_warn("Ignoring index table segment that is invalid or could not be processed\n");
        return -1;
    }

    int64_t end_offset = -1;
    if (new_segment->getIndexDuration() >= 0)
        end_offset = new_segment->getIndexStartPosition() + new_segment->getIndexDuration();

    if (new_segment->HaveConstantEditUnitSize())
        InsertCBEIndexSegment(new_segment);
    else
        InsertVBEIndexSegment(new_segment);
    // don't use new_segment from here onwards

    if (mSegments.size() == 1)
        mEditRate = mSegments.back()->getIndexEditRate();

    return end_offset;
}

void IndexTableHelper::UpdateIndex(int64_t position, int64_t essence_offset, int64_t size)
{
    BMX_ASSERT(position <= mDuration);

    // TODO: size might be useful
    (void)size;

    if (position < mDuration || (mDuration == 0 && HaveConstantEditUnitSize()))
        return;

    if (HaveConstantEditUnitSize()) {
        BMX_EXCEPTION(("Index table with constant edit unit size and duration %"PRId64
                       " does not cover position %"PRId64, mDuration, position));
    }

    if (!mSegments.empty() &&
        !mSegments.back()->IsFileIndexSegment() &&
        mSegments.back()->CanAppendIndexEntry())
    {
        mSegments.back()->AppendIndexEntry(0, 0, 0, 0, essence_offset);
        if (mSegments.back()->HavePairedIndexEntries())
            mSegments.back()->AppendIndexEntry(0, 0, 0, 0, essence_offset);
        mSegments.back()->incrementIndexDuration();
    }
    else
    {
        auto_ptr<IndexTableHelperSegment> segment(new IndexTableHelperSegment());
        segment->setIndexEditRate(mEditRate);
        segment->setIndexStartPosition(position);
        segment->setIndexDuration(1);
        segment->AppendIndexEntry(RUNTIME_INDEX_SEGMENT_SIZE, 0, 0, 0, essence_offset);
        if (!mSegments.empty() && mSegments.back()->HavePairedIndexEntries())
            mSegments.back()->AppendIndexEntry(0, 0, 0, 0, essence_offset);

        mSegments.push_back(segment.release());
    }

    mDuration++;
}

void IndexTableHelper::SetIsComplete()
{
    mIsComplete = true;

    // recalculate mDuration if SetEssenceDataSize was called before the index table segment was read
    SetEssenceDataSize(mEssenceDataSize);
}

bool IndexTableHelper::HaveEditUnit(int64_t position) const
{
    return !mSegments.empty() && position >= 0 && position < mDuration;
}

void IndexTableHelper::GetEditUnit(int64_t position, int8_t *temporal_offset, int8_t *key_frame_offset, uint8_t *flags,
                                   int64_t *offset, int64_t *size)
{
    BMX_ASSERT(!mSegments.empty());
    BMX_CHECK(mDuration == 0 || position < mDuration);

    int result = mSegments[mLastEditUnitSegment]->GetEditUnit(position, temporal_offset, key_frame_offset, flags,
                                                              offset);
    if (result < 0) {
        // TODO: binary search
        if (result == -2) {
            // segment is before mLastEditUnitSegment
            size_t i;
            for (i = mLastEditUnitSegment; i > 0; i--) {
                result = mSegments[i - 1]->GetEditUnit(position, temporal_offset, key_frame_offset, flags,
                                                       offset);
                if (result >= 0) {
                    mLastEditUnitSegment = i - 1;
                    break;
                } else if (result == -1) {
                    break;
                }
            }
        } else {
            // segment is after mLastEditUnitSegment
            size_t i;
            for (i = mLastEditUnitSegment + 1; i < mSegments.size(); i++) {
                result = mSegments[i]->GetEditUnit(position, temporal_offset, key_frame_offset, flags,
                                                   offset);
                if (result >= 0) {
                    mLastEditUnitSegment = i;
                    break;
                } else if (result == -2) {
                    break;
                }
            }
        }
    }
    BMX_CHECK_M(result == 0,
               ("Failed to find edit unit index information for position 0x%"PRIx64, position));

    if (size) {
        if (mEditUnitSize > 0) {
            *size = mEditUnitSize;
        } else if (mDuration == 0 || position + 1 < mDuration) {
            *size = GetEditUnitOffset(position + 1) - (*offset);
        } else if (mSegments.back()->HaveExtraIndexEntries()) {
            *size = mSegments.back()->GetIndexEndOffset() - (*offset);
        } else {
            if (mEssenceDataSize < (*offset)) {
                BMX_EXCEPTION(("Failed to calc valid last edit unit size because essence data size %"PRId64
                               " is too small", mEssenceDataSize));
            }
            *size = mEssenceDataSize - (*offset);
        }
    }
}

void IndexTableHelper::GetEditUnit(int64_t position, int64_t *offset, int64_t *size)
{
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    GetEditUnit(position, &temporal_offset, &key_frame_offset, &flags, offset, size);
}

bool IndexTableHelper::HaveEditUnitOffset(int64_t position) const
{
    return position == 0 || (!mSegments.empty() && position >= 0 && position < mDuration);
}

int64_t IndexTableHelper::GetEditUnitOffset(int64_t position)
{
    if (position == 0)
        return 0;

    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    int64_t offset;
    GetEditUnit(position, &temporal_offset, &key_frame_offset, &flags, &offset, 0);

    return offset;
}

bool IndexTableHelper::HaveEditUnitSize(int64_t position) const
{
    if (HaveConstantEditUnitSize())
        return true;
    else if (mSegments.empty())
        return false;
    else if (!HaveEditUnitOffset(position))
        return false;
    else if (HaveEditUnitOffset(position + 1))
        return true;
    else if (mSegments.back()->HaveExtraIndexEntries())
        return true;
    else if (mEssenceDataSize != 0)
        return true;
    else
        return false;
}

bool IndexTableHelper::GetTemporalReordering(uint32_t delta)
{
    BMX_ASSERT(!mSegments.empty());

    // TODO: is it possible to identify an element within the delta entry array with certainty when slice > 0?
    return mSegments[0]->haveDeltaEntryAtDelta(delta, 0) &&
           mSegments[0]->getDeltaEntryAtDelta(delta, 0)->posTableIndex == -1;
}

bool IndexTableHelper::GetIndexEntry(MXFIndexEntryExt *entry, int64_t position)
{
    if (position < 0 || position >= mDuration)
        return false;

    GetEditUnit(position, &entry->temporal_offset, &entry->key_frame_offset, &entry->flags,
                &entry->container_offset, &entry->edit_unit_size);
    return true;
}

void IndexTableHelper::InsertCBEIndexSegment(auto_ptr<IndexTableHelperSegment> &new_segment_ap)
{
    IndexTableHelperSegment *new_segment = new_segment_ap.get();

    if (!mSegments.empty()) {
        if (!mSegments.back()->HaveConstantEditUnitSize()) {
            // existing VBE segments, either originating from file or runtime generated

            if (mSegments.size() > 1 || !mSegments.back()->IsFileIndexSegment())
                BMX_EXCEPTION(("Can't mix VBE and CBE index table segments"));

            if (SEG_DUR(new_segment) > 0 && SEG_DUR(new_segment) < SEG_DUR(mSegments.front())) {
                // ignore new segment if it covers less edit units than the runtime generated index segment
                new_segment_ap.reset(0);
                return;
            }

            // replace runtime generated index segment
            delete mSegments.front();
            mSegments.clear();
        } else {
            // existing CBE segments

            if (SEG_DUR(mSegments.back()) == 0 || SEG_START(new_segment) < SEG_END(mSegments.back())) {
                // ignore new segment that overlaps with existing segments
                new_segment_ap.reset(0);
                return;
            }

            // new segment indexes after last segment
            new_segment->SetEssenceStartOffset(mSegments.back()->GetEssenceEndOffset());
        }
    }

    if (( mSegments.empty() && SEG_START(new_segment) != 0) ||
        (!mSegments.empty() && SEG_START(new_segment) != SEG_END(mSegments.back())))
    {
        // TODO: add support for sparse index tables
        BMX_EXCEPTION(("Sparse index table is not supported"));
    }

    mSegments.push_back(new_segment);
    new_segment_ap.release();

    if (mSegments.size() == 1) {
        mEditUnitSize = new_segment->GetEditUnitSize();
        mDuration = new_segment->getIndexDuration();
    } else {
        if (new_segment->GetEditUnitSize() != mEditUnitSize)
            mEditUnitSize = 0;
        mDuration += new_segment->getIndexDuration();
    }
}

void IndexTableHelper::InsertVBEIndexSegment(auto_ptr<IndexTableHelperSegment> &new_segment_ap)
{
    IndexTableHelperSegment *new_segment = new_segment_ap.get();

    if (mEditUnitSize > 0)
        BMX_EXCEPTION(("Can't mix VBE and CBE index table segments"));

    // update or remove existing segments
    int64_t new_duration = 0;
    vector<IndexTableHelperSegment*>::iterator iter = mSegments.begin();
    while (iter != mSegments.end()) {
        IndexTableHelperSegment *segment = *iter;
        bool deleted_segment = false;

        if (SEG_END(segment) > SEG_START(new_segment) && SEG_START(segment) < SEG_END(new_segment)) {
            // existing and new segment overlap
            if (SEG_START(segment) >= SEG_START(new_segment) && SEG_END(segment) <= SEG_END(new_segment)) {
                // existing segment if completely covered by new segment
                delete segment;
                iter = mSegments.erase(iter);
                deleted_segment = true;
            } else if (SEG_START(segment) < SEG_START(new_segment) && SEG_END(segment) > SEG_END(new_segment)) {
                // existing segment covers and is larger than new segment
                iter = mSegments.insert(iter, CreateStartSegment(segment,
                                                                 (uint32_t)(SEG_START(new_segment) -
                                                                            SEG_START(segment))));
                segment->UpdateStartPosition(SEG_START(new_segment));
            } else if (SEG_START(segment) < SEG_START(new_segment)) {
                // existing segment starts before new segment
                segment->UpdateDuration(SEG_START(new_segment) - SEG_START(segment));
            } else {
                // existing segment ends after new segment
                segment->UpdateStartPosition(SEG_END(new_segment));
            }
        } else if (SEG_START(segment) >= SEG_END(new_segment)) {
            // existing (shortened) segment is after new segment
            iter = mSegments.insert(iter, new_segment);
            new_segment_ap.release();
            break;
        }

        if (!deleted_segment) {
            new_duration += (*iter)->getIndexDuration();
            iter++;
        }
    }
    if (iter == mSegments.end()) {
        if (( mSegments.empty() && SEG_START(new_segment) != 0) ||
            (!mSegments.empty() && SEG_START(new_segment) != SEG_END(mSegments.back())))
        {
            // TODO: add support for sparse index tables
            BMX_EXCEPTION(("Sparse index table is not supported"));
        }
        mSegments.push_back(new_segment);
        new_segment_ap.release();
        new_duration += new_segment->getIndexDuration();
    } else {
        while (iter != mSegments.end()) {
            new_duration += (*iter)->getIndexDuration();
            iter++;
        }
    }

    mDuration = new_duration;
}

IndexTableHelperSegment* IndexTableHelper::CreateStartSegment(IndexTableHelperSegment *segment, uint32_t duration)
{
    auto_ptr<IndexTableHelperSegment> new_segment(new IndexTableHelperSegment());
    new_segment->setIndexEditRate(segment->getIndexEditRate());
    new_segment->setIndexStartPosition(segment->getIndexStartPosition());
    new_segment->setIndexDuration(duration);
    new_segment->CopyIndexEntries(segment, duration);

    return new_segment.release();
}

