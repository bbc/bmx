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

#ifndef __BMX_ESSENCE_READER_H__
#define __BMX_ESSENCE_READER_H__


#include <vector>

#include <bmx/frame/Frame.h>
#include <bmx/mxf_reader/EssenceChunkHelper.h>
#include <bmx/mxf_reader/IndexTableHelper.h>



namespace bmx
{


class MXFFileReader;


class EssenceReader
{
public:
    EssenceReader(MXFFileReader *file_reader);
    ~EssenceReader();

    void SetReadLimits(int64_t start_position, int64_t duration);

    uint32_t Read(uint32_t num_samples);
    void Seek(int64_t position);

    mxfRational GetEditRate();
    int64_t GetPosition() { return mPosition; }

    int64_t GetIndexedDuration() { return mIndexTableHelper.GetDuration(); }
    bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position);

    int64_t LegitimisePosition(int64_t position);

private:
    void ReadClipWrappedSamples(uint32_t num_samples);
    void ReadFrameWrappedSamples(uint32_t num_samples);

    void GetEditUnit(int64_t position, int64_t *file_position, int64_t *size);
    void GetEditUnitGroup(int64_t position, uint32_t max_samples, int64_t *file_position, int64_t *size,
                          uint32_t *num_samples);

private:
    MXFFileReader *mFileReader;

    EssenceChunkHelper mEssenceChunkHelper;
    IndexTableHelper mIndexTableHelper;

    int64_t mReadStartPosition;
    int64_t mReadDuration;
    int64_t mPosition;
    uint32_t mImageStartOffset;

    std::vector<Frame*> mTrackFrames;
};


};



#endif

