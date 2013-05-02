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

#ifndef BMX_ESSENCE_READER_H_
#define BMX_ESSENCE_READER_H_


#include <vector>

#include <bmx/frame/Frame.h>
#include <bmx/mxf_reader/FrameMetadataReader.h>
#include <bmx/mxf_reader/EssenceChunkHelper.h>
#include <bmx/mxf_reader/IndexTableHelper.h>



namespace bmx
{


class MXFFileReader;


class EssenceReader
{
public:
    friend class BaseReader;

public:
    EssenceReader(MXFFileReader *file_reader, bool file_is_complete);
    ~EssenceReader();

    void SetReadLimits(int64_t start_position, int64_t duration);

    uint32_t Read(uint32_t num_samples);
    void Seek(int64_t position);

    mxfRational GetEditRate() const    { return mIndexTableHelper.GetEditRate(); };
    int64_t GetPosition() const        { return mPosition; }
    int64_t GetIndexedDuration() const { return mIndexTableHelper.GetDuration(); }

    bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position);

    int64_t LegitimisePosition(int64_t position);

    bool IsComplete() const;

private:
    uint32_t ReadClipWrappedSamples(uint32_t num_samples);
    uint32_t ReadFrameWrappedSamples(uint32_t num_samples);

    void GetEditUnit(int64_t position, mxfKey *element_key, int64_t *file_position, int64_t *size);
    void GetEditUnitGroup(int64_t position, uint32_t max_samples, mxfKey *element_key, int64_t *file_position,
                          int64_t *size, uint32_t *num_samples);

    uint32_t GetConstantEditUnitSize();

private:
    bool SeekEssence(int64_t base_position, bool for_read);
    bool ReadEssenceKL(bool first_element, mxfKey *key, uint8_t *llen, uint64_t *len);

private:
    int64_t GetIndexedFilePosition(int64_t base_position);

    void SetContentPackageStart(int64_t base_position, int64_t file_position, bool pos_at_key);

    bool ReadFirstEssenceKL(mxfKey *key, uint8_t *llen, uint64_t *len);
    bool ReadNonfirstEssenceKL(mxfKey *key, uint8_t *llen, uint64_t *len);
    bool SeekContentPackageStart();

    void ReadNextPartition(const mxfKey *key, uint8_t llen, uint64_t len);

    void SetHaveFooter();
    void SetFileIsComplete();

    void SetNextKL(const mxfKey *key, uint8_t llen, uint64_t len);
    void ResetNextKL();
    void ResetState();

private:
    MXFFileReader *mFileReader;
    mxfpp::File *mFile;
    bool mFileIsComplete;

    EssenceChunkHelper mEssenceChunkHelper;
    IndexTableHelper mIndexTableHelper;

    FrameMetadataReader *mFrameMetadataReader;

    int64_t mReadStartPosition;
    int64_t mReadDuration;
    int64_t mPosition;
    uint32_t mImageStartOffset;
    uint32_t mImageEndOffset;

    std::vector<Frame*> mTrackFrames;

    int64_t mBasePosition;
    int64_t mFilePosition;
    mxfKey mNextKey;
    uint8_t mNextLLen;
    uint64_t mNextLen;
    bool mAtCPStart;
    mxfKey mEssenceStartKey;
    int64_t mLastKnownFilePosition;
    int64_t mLastKnownBasePosition;
    bool mHaveFooter;
    bool mBaseReadError;
};


};



#endif

