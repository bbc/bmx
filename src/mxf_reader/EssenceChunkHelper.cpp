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

#include <libMXF++/MXF.h>

#include <bmx/mxf_reader/EssenceChunkHelper.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



EssenceChunk::EssenceChunk()
{
    file_position = 0;
    offset = 0;
    size = 0;
}



EssenceChunkHelper::EssenceChunkHelper(MXFFileReader *file_reader)
{
    mFileReader = file_reader;
    mLastEssenceChunk = 0;
}

EssenceChunkHelper::~EssenceChunkHelper()
{
}

void EssenceChunkHelper::ExtractEssenceChunkIndex(uint32_t avid_first_frame_offset)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    File *mxf_file = mFileReader->mFile;

    const vector<Partition*> &partitions = mFileReader->mFile->getPartitions();
    size_t i;
    for (i = 0; i < partitions.size(); i++) {
        if (partitions[i]->getBodySID() != mFileReader->mBodySID)
            continue;

        int64_t partition_end;
        if (i + 1 < partitions.size())
            partition_end = partitions[i + 1]->getThisPartition();
        else
            partition_end = mxf_file->size();

        mxf_file->seek(partitions[i]->getThisPartition(), SEEK_SET);
        mxf_file->readKL(&key, &llen, &len);
        mxf_file->skip(len);
        while (!mxf_file->eof()) {

            mxf_file->readNextNonFillerKL(&key, &llen, &len);

            if (mxf_is_gc_essence_element(&key) || mxf_avid_is_essence_element(&key)) {
                if (mFileReader->IsClipWrapped()) {
                    // check whether this is the target essence container; skip and continue if not
                    if (!mFileReader->GetInternalTrackReaderByNumber(mxf_get_track_number(&key))) {
                        mxf_file->skip(len);
                        continue;
                    }
                }

                // check the essence container data is contiguous
                uint64_t body_offset = partitions[i]->getBodyOffset();
                if (mEssenceChunks.empty()) {
                    if (body_offset > 0) {
                        log_warn("Ignoring potential missing essence container data; "
                                 "partition pack's BodyOffset 0x%"PRIx64" > expected offset 0x00\n",
                                 body_offset);
                        body_offset = 0;
                    }
                } else if (body_offset > (uint64_t)(mEssenceChunks.back().offset + mEssenceChunks.back().size)) {
                    log_warn("Ignoring potential missing essence container data; "
                             "partition pack's BodyOffset 0x%"PRIx64" > expected offset 0x%"PRIx64"\n",
                             body_offset, mEssenceChunks.back().offset + mEssenceChunks.back().size);
                    body_offset = mEssenceChunks.back().offset + mEssenceChunks.back().size;
                } else if (body_offset < (uint64_t)(mEssenceChunks.back().offset + mEssenceChunks.back().size)) {
                    log_warn("Ignoring potential overlapping essence container data; "
                             "partition pack's BodyOffset 0x%"PRIx64" < expected offset 0x%"PRIx64"\n",
                             body_offset, mEssenceChunks.back().offset + mEssenceChunks.back().size);
                    body_offset = mEssenceChunks.back().offset + mEssenceChunks.back().size;
                }

                // add this partition's essence to the index
                EssenceChunk essence_chunk;
                essence_chunk.offset = body_offset;
                essence_chunk.file_position = mxf_file->tell();
                if (mFileReader->IsFrameWrapped()) {
                    essence_chunk.file_position -= mxfKey_extlen + llen;
                    essence_chunk.size = partition_end - essence_chunk.file_position;
                } else {
                    essence_chunk.size = len;
                    if (avid_first_frame_offset > 0 && mEssenceChunks.empty()) {
                        essence_chunk.file_position += avid_first_frame_offset;
                        essence_chunk.size -= avid_first_frame_offset;
                    }
                }
                BMX_CHECK(essence_chunk.size >= 0);
                if (essence_chunk.size > 0)
                    mEssenceChunks.push_back(essence_chunk);

                // continue with clip-wrapped to support multiple essence container elements in this partition
                if (mFileReader->IsClipWrapped()) {
                    mxf_file->skip(len);
                    continue;
                }

                break;
            }

            if (mxf_is_partition_pack(&key))
                break;

            if (mxf_is_header_metadata(&key) && partitions[i]->getHeaderByteCount() > 0)
                mxf_file->skip(partitions[i]->getHeaderByteCount() - (mxfKey_extlen + llen));
            else if (mxf_is_index_table_segment(&key) && partitions[i]->getIndexByteCount() > 0)
                mxf_file->skip(partitions[i]->getIndexByteCount() - (mxfKey_extlen + llen));
            else
                mxf_file->skip(len);
        }
    }


#if 0
    {
        size_t i;
        for (i = 0; i < mEssenceChunks.size(); i++)
            printf("Essence chunk: file_pos=0x%"PRIx64", offset=0x%"PRIx64" size=0x%"PRIx64"\n",
                   mEssenceChunks[i].file_position, mEssenceChunks[i].offset, mEssenceChunks[i].size);
    }
#endif
}

int64_t EssenceChunkHelper::GetEssenceDataSize() const
{
    if (mEssenceChunks.empty())
        return 0;

    return mEssenceChunks.back().offset + mEssenceChunks.back().size;
}

void EssenceChunkHelper::GetEditUnit(int64_t index_offset, int64_t index_size, int64_t *file_position)
{
    BMX_CHECK(!mEssenceChunks.empty());

    // TODO: use binary search
    if (mEssenceChunks[mLastEssenceChunk].offset > index_offset) {
        // edit unit is in chunk before mLastEssenceChunk
        size_t i;
        for (i = mLastEssenceChunk; i > 0; i--) {
            if (mEssenceChunks[i - 1].offset <= index_offset) {
                mLastEssenceChunk = i - 1;
                break;
            }
        }
    } else if (mEssenceChunks[mLastEssenceChunk].offset + mEssenceChunks[mLastEssenceChunk].size <= index_offset) {
        // edit unit is in chunk after mLastEssenceChunk
        size_t i;
        for (i = mLastEssenceChunk + 1; i < mEssenceChunks.size(); i++) {
            if (mEssenceChunks[i].offset + mEssenceChunks[i].size > index_offset) {
                mLastEssenceChunk = i;
                break;
            }
        }
    }

    BMX_CHECK_M(mEssenceChunks[mLastEssenceChunk].offset <= index_offset &&
               mEssenceChunks[mLastEssenceChunk].offset + mEssenceChunks[mLastEssenceChunk].size >= index_offset + index_size,
               ("Failed to find indexed edit unit (off=0x%"PRIx64",size=0x%"PRIx64") in essence data",
                   index_offset, index_size));

    (*file_position) = mEssenceChunks[mLastEssenceChunk].file_position +
                        (index_offset - mEssenceChunks[mLastEssenceChunk].offset);
}

