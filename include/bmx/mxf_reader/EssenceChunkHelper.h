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

#ifndef BMX_ESSENCE_CHUNK_HELPER_H_
#define BMX_ESSENCE_CHUNK_HELPER_H_

#include <vector>

#include <bmx/BMXTypes.h>



namespace bmx
{


class MXFFileReader;


class EssenceChunk
{
public:
    EssenceChunk();

    int64_t file_position;
    int64_t essence_offset;
    int64_t size;
    bool is_complete;
    size_t partition_id;
    mxfKey element_key;
};



class EssenceChunkHelper
{
public:
    EssenceChunkHelper(MXFFileReader *file_reader);
    ~EssenceChunkHelper();

    void CreateEssenceChunkIndex(int64_t first_edit_unit_size);

    void AppendChunk(size_t partition_id, int64_t file_position, const mxfKey *element_key, uint8_t element_llen,
                     uint64_t element_len);
    void UpdateLastChunk(int64_t file_position, bool is_end);
    void SetIsComplete();

    size_t GetNumIndexedPartitions() const { return mNumIndexedPartitions; }

public:
    bool IsComplete() const { return mIsComplete; }

    bool HaveFilePosition(int64_t essence_offset);

    int64_t GetEssenceDataSize() const;
    void GetKeyAndFilePosition(int64_t essence_offset, int64_t size, mxfKey *element_key, int64_t *position);
    int64_t GetFilePosition(int64_t essence_offset);
    int64_t GetEssenceOffset(int64_t file_position);

private:
    void EssenceOffsetUpdate(int64_t essence_offset);
    void FilePositionUpdate(int64_t file_position);

private:
    MXFFileReader *mFileReader;
    uint32_t mAvidFirstFrameOffset;
    std::vector<EssenceChunk> mEssenceChunks;
    size_t mLastEssenceChunk;
    size_t mNumIndexedPartitions;
    bool mIsComplete;
};


};



#endif

