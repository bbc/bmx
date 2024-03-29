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

#ifndef BMX_WAVE_CHUNK_H_
#define BMX_WAVE_CHUNK_H_

#include <string>

#include <bmx/BMXTypes.h>
#include <bmx/BMXIO.h>


namespace bmx
{


#define WAVE_CHUNK_ID(cstr)    WaveChunkId({(uint8_t)((cstr)[0]), (uint8_t)((cstr)[1]), (uint8_t)((cstr)[2]), (uint8_t)((cstr)[3])})

std::string get_wave_chunk_id_str(WaveChunkId id);


};


bool operator==(const bmx::WaveChunkId &left, const bmx::WaveChunkId &right);
bool operator!=(const bmx::WaveChunkId &left, const bmx::WaveChunkId &right);
bool operator<(const bmx::WaveChunkId &left, const bmx::WaveChunkId &right);

bool operator==(const bmx::WaveChunkId &left, const char *right);
bool operator!=(const bmx::WaveChunkId &left, const char *right);


namespace bmx
{


class WaveChunk : public BMXIO
{
public:
    WaveChunk(WaveChunkId id);
    virtual ~WaveChunk();

    WaveChunkId Id() const { return mId; }

    virtual uint32_t Read(unsigned char *data, uint32_t size) = 0;
    virtual int64_t Tell() = 0;
    virtual int64_t Size() = 0;

public:
    int64_t AlignedSize() { return Size() + (Size() & 1); }

protected:
    WaveChunkId mId;
};


};


#endif
