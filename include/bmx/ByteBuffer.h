/*
 * Copyright (C) 2020, British Broadcasting Corporation
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

#ifndef BMX_BYTE_BUFFER_H_
#define BMX_BYTE_BUFFER_H_

#include <exception>
#include <vector>

#include <bmx/BMXTypes.h>


namespace bmx
{


class InsufficientBytes : public std::exception
{};


class ByteBuffer
{
public:
    ByteBuffer(const unsigned char *data, uint32_t size, bool big_endian);

    uint8_t GetUInt8();
    uint16_t GetUInt16();
    uint32_t GetUInt32();
    uint64_t GetUInt64();

    std::vector<unsigned char> GetBytes(uint32_t size);

    void Skip(uint32_t size);
    void SetPos(uint32_t pos);

    uint32_t GetSize() const  { return mSize; }
    uint32_t GetPos() const   { return mPos; }

protected:
    const unsigned char *mData;
    uint32_t mSize;
    uint32_t mPos;
    bool mBigEndian;
};


class ByteBufferLengthContext
{
public:
    ByteBufferLengthContext(ByteBuffer &data, uint32_t length);
    ~ByteBufferLengthContext();

private:
    ByteBuffer &mData;
    uint32_t mEndPos;
};


};


#endif

