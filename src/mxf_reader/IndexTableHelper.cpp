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

#include <memory>

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

#define GET_TEMPORAL_OFFSET(pos)    (*((int8_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE]))
#define GET_KEY_FRAME_OFFSET(pos)   (*((int8_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE + 1]))
#define GET_FLAGS(pos)              (*((uint8_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE + 2]))
#define GET_STREAM_OFFSET(pos)      (*((int64_t*)&mIndexEntries[(pos) * INTERNAL_INDEX_ENTRY_SIZE + 3]))



static int add_frame_offset_index_entry(void *data, uint32_t num_entries, MXFIndexTableSegment *segment,
                                        int8_t temporal_offset, int8_t key_frame_offset, uint8_t flags,
                                        uint64_t stream_offset, uint32_t *slice_offset, mxfRational *pos_table)
{
    IndexTableHelperSegment *helper = static_cast<IndexTableHelperSegment*>(data);

    (void)segment;
    (void)slice_offset;
    (void)pos_table;

    helper->AppendIndexEntry(num_entries, temporal_offset, key_frame_offset, flags, stream_offset);

    return 1;
}



IndexTableHelperSegment::IndexTableHelperSegment()
: IndexTableSegment()
{
    mIndexEntries = 0;
    mNumIndexEntries = 0;
    mAllocIndexEntries = 0;
    mHaveExtraIndexEntries = false;
    mHavePairedIndexEntries = false;
    mIndexEndOffset = 0;
    mEssenceStartOffset = 0;
}

IndexTableHelperSegment::~IndexTableHelperSegment()
{
    delete [] mIndexEntries;
}

void IndexTableHelperSegment::ParseIndexTableSegment(File *file, uint64_t segment_len)
{
    // free existing segment which will be replaced
    mxf_free_index_table_segment(&_cSegment);

    // use Avid function to support non-standard Avid index table segments
    BMX_CHECK(mxf_avid_read_index_table_segment_2(file->getCFile(), segment_len,
                                                 mxf_default_add_delta_entry, 0,
                                                 add_frame_offset_index_entry, this,
                                                 &_cSegment));
    setIndexEditRate(normalize_rate(getIndexEditRate()));

    // samples files produced by Ardendo product 'ardftp' had an invalid index duration -1
    if (getIndexDuration() < 0) {
        log_warn("Index duration %"PRId64" is invalid. Assuming index duration 0 instead.\n", getIndexDuration());
        setIndexDuration(0);
    }

    BMX_CHECK((mNumIndexEntries == 0 && getEditUnitByteCount() > 0) ||
             ((int64_t)mNumIndexEntries >= getIndexDuration()));
    BMX_CHECK(getIndexDuration() > 0 || mNumIndexEntries == 0);

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
        *temporal_offset = 0;
        *key_frame_offset = 0;
        *flags = 0x80; // reference frame
        *stream_offset = mEssenceStartOffset + rel_position * getEditUnitByteCount();
    } else {
        if (mHavePairedIndexEntries)
            rel_position *= 2;

        *temporal_offset = GET_TEMPORAL_OFFSET(rel_position);
        if (mHavePairedIndexEntries)
            *temporal_offset /= 2;
        *key_frame_offset = GET_KEY_FRAME_OFFSET(rel_position);
        if (mHavePairedIndexEntries)
            *key_frame_offset /= 2;
        *flags = GET_FLAGS(rel_position);
        *stream_offset = GET_STREAM_OFFSET(rel_position);
    }

    return 0;
}

void IndexTableHelperSegment::AppendIndexEntry(uint32_t num_entries, int8_t temporal_offset, int8_t key_frame_offset,
                                               uint8_t flags, uint64_t stream_offset)
{
    if (!mIndexEntries) {
        mIndexEntries = new unsigned char[INTERNAL_INDEX_ENTRY_SIZE * num_entries];
        mAllocIndexEntries = num_entries;
        mNumIndexEntries = 0;
    }
    BMX_ASSERT(mNumIndexEntries < mAllocIndexEntries);

    // file offsets use int64_t whilst the index segment stream offset is uint64_t
    BMX_CHECK((stream_offset & 0x8000000000000000LL) == 0);

    GET_TEMPORAL_OFFSET(mNumIndexEntries) = temporal_offset;
    GET_KEY_FRAME_OFFSET(mNumIndexEntries) = key_frame_offset;
    GET_FLAGS(mNumIndexEntries) = flags;
    GET_STREAM_OFFSET(mNumIndexEntries) = (int64_t)stream_offset;

    mNumIndexEntries++;
}

bool IndexTableHelperSegment::HaveFixedEditUnitByteCount()
{
    return getEditUnitByteCount() != 0;
}

uint32_t IndexTableHelperSegment::GetFixedEditUnitByteCount()
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



IndexTableHelper::IndexTableHelper(MXFFileReader *file_reader)
{
    mFileReader = file_reader;
    mLastEditUnitSegment = 0;
    mHaveFixedEditUnitByteCount = true;
    mFixedEditUnitByteCount = 0;
    mEssenceDataSize = 0;
    mDuration = 0;
}

IndexTableHelper::~IndexTableHelper()
{
    size_t i;
    for (i = 0; i < mSegments.size(); i++)
        delete mSegments[i];
}

bool IndexTableHelper::ExtractIndexTable()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    File *mxf_file = mFileReader->mFile;

    const vector<Partition*> &partitions = mFileReader->mFile->getPartitions();
    size_t i;
    for (i = 0; i < partitions.size(); i++) {
        if (partitions[i]->getIndexSID() != mFileReader->mIndexSID)
            continue;

        // find the start of the first index table segment
        mxf_file->seek(partitions[i]->getThisPartition(), SEEK_SET);
        mxf_file->readKL(&key, &llen, &len);
        mxf_file->skip(len);
        while (true)
        {
            mxf_file->readNextNonFillerKL(&key, &llen, &len);

            if (mxf_is_partition_pack(&key))
                break;

            if (mxf_is_index_table_segment(&key))
                break;

            if (mxf_is_header_metadata(&key) && partitions[i]->getHeaderByteCount() > 0)
                mxf_file->skip(partitions[i]->getHeaderByteCount() - (mxfKey_extlen + llen));
            else
                mxf_file->skip(len);
        }

        // parse the index table segments
        while (mxf_is_index_table_segment(&key)) {

            auto_ptr<IndexTableHelperSegment> segment(new IndexTableHelperSegment());
            segment->ParseIndexTableSegment(mxf_file, len);

            // remove existing segments that have a start position <= this segment's start position
            size_t j;
            for (j = 0; j < mSegments.size(); j++) {
                if (segment->getIndexStartPosition() <= mSegments[j]->getIndexStartPosition()) {
                    // segment has been replaced
                    BMX_CHECK(segment->getIndexStartPosition() == mSegments[j]->getIndexStartPosition());
                    break;
                }
            }
            while (j < mSegments.size()) {
                delete mSegments[j];
                mSegments.erase(mSegments.begin() + j);
            }

            if (!mSegments.empty()) {
                // only support all index segments with fixed edit unit byte count or all variable edit units
                BMX_CHECK(segment->HaveFixedEditUnitByteCount() == mSegments.back()->HaveFixedEditUnitByteCount());
                // only support contiguous index segments
                BMX_CHECK(segment->getIndexStartPosition() == mSegments.back()->getIndexStartPosition() +
                                                                    mSegments.back()->getIndexDuration());

                if (segment->HaveFixedEditUnitByteCount())
                    segment->SetEssenceStartOffset(mSegments.back()->GetEssenceEndOffset());
            }
            mSegments.push_back(segment.release());

            if (mxf_file->tell() >= mxf_file->size())
                break;

            mxf_file->readNextNonFillerKL(&key, &llen, &len);
        }
    }
    if (mSegments.empty())
        return false;

    // calc total duration and determine fixed edit unit byte count
    for (i = 0; i < mSegments.size(); i++) {
        BMX_CHECK(mSegments[i]->getIndexDuration() > 0 || mSegments[i]->HaveFixedEditUnitByteCount());
        mDuration += mSegments[i]->getIndexDuration();

        if (mHaveFixedEditUnitByteCount) {
            if (!mSegments[i]->HaveFixedEditUnitByteCount() ||
                (mFixedEditUnitByteCount != 0 &&
                    mFixedEditUnitByteCount != mSegments[i]->GetFixedEditUnitByteCount()))
            {
                mHaveFixedEditUnitByteCount = false;
                mFixedEditUnitByteCount = 0;
            }
            else
            {
                mFixedEditUnitByteCount = mSegments[i]->GetFixedEditUnitByteCount();
            }
        }
    }

    return true;
}

void IndexTableHelper::SetEssenceDataSize(int64_t size)
{
    BMX_ASSERT(!mSegments.empty());

    mEssenceDataSize = size;

    // calc index duration if unknown
    if (mDuration == 0 && mHaveFixedEditUnitByteCount && mFixedEditUnitByteCount > 0)
        mDuration = size / mFixedEditUnitByteCount;
}

mxfRational IndexTableHelper::GetEditRate()
{
    BMX_ASSERT(!mSegments.empty());
    return mSegments[0]->getIndexEditRate();
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
        if (mDuration == 0 || position + 1 < mDuration) {
            *size = GetEditUnitOffset(position + 1) - (*offset);
        } else if (mSegments.back()->HaveExtraIndexEntries()) {
            *size = mSegments.back()->GetIndexEndOffset() - (*offset);
        } else {
            *size = mEssenceDataSize - (*offset);
            /* ignores junk found at the end of Avid clip wrapped essence with fixed edit unit size */
            if (mHaveFixedEditUnitByteCount && *size > mFixedEditUnitByteCount)
                *size = mFixedEditUnitByteCount;
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

int64_t IndexTableHelper::GetEditUnitOffset(int64_t position)
{
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    int64_t offset;
    GetEditUnit(position, &temporal_offset, &key_frame_offset, &flags, &offset, 0);

    return offset;
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

