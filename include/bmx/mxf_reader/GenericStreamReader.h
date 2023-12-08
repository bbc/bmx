/*
 * Copyright (C) 2022, British Broadcasting Corporation
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

#ifndef BMX_GENERIC_STREAM_READER_H_
#define BMX_GENERIC_STREAM_READER_H_

#include <bmx/BMXIO.h>
#include <bmx/BMXTypes.h>


namespace bmx
{


class GenericStreamReader : public BMXIO
{
public:
    GenericStreamReader(mxfpp::File *mxf_file, uint32_t stream_id, const std::vector<mxfKey> &expected_stream_keys);
    virtual ~GenericStreamReader();

public:
    // From BMXIO
    virtual uint32_t Read(unsigned char *data, uint32_t size);
    virtual int64_t Tell();
    virtual int64_t Size();

    virtual void Read(FILE *file);
    virtual void Read(BMXIO *bmx_io);

public:
    void Read(unsigned char **data, size_t *size);

    const std::vector<std::pair<int64_t, int64_t> >& GetFileOffsetRanges();

public:
    uint32_t GetStreamId() const { return mStreamId; }

    const mxfKey* GetStreamKey() const { return &mStreamKey; }

private:
    bool MatchesExpectedStreamKey(const mxfKey *key);
    void Read(FILE *file, BMXIO *bmx_io, unsigned char **data, size_t *size);

private:
    mxfpp::File *mMXFFile;
    uint32_t mStreamId;
    std::vector<mxfKey> mExpectedStreamKeys;
    mxfKey mStreamKey;

    bool mHaveFileOffsetRanges;
    std::vector<std::pair<int64_t, int64_t> > mFileOffsetRanges;
    int64_t mSize;
    int64_t mPosition;
    size_t mRangeIndex;
    int64_t mRangeOffset;
};


};



#endif
