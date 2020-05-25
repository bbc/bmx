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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bmx/ByteBuffer.h>

using namespace std;
using namespace bmx;


ByteBuffer::ByteBuffer(const unsigned char *data, uint32_t size, bool big_endian)
{
    mData = data;
    mSize = size;
    mPos = 0;
    mBigEndian = big_endian;
}

uint8_t ByteBuffer::GetUInt8()
{
    if (mPos + 1 > mSize)
        throw InsufficientBytes();

    uint8_t value = mData[mPos];
    mPos++;

    return value;
}

uint16_t ByteBuffer::GetUInt16()
{
    if (mPos + 2 > mSize)
        throw InsufficientBytes();

    uint16_t value;
    if (mBigEndian) {
        value = ((uint16_t)mData[mPos] << 8) |
                mData[mPos + 1];
    } else {
        value = mData[mPos] |
                ((uint16_t)mData[mPos + 1] << 8);
    }
    mPos += 2;

    return value;
}

uint32_t ByteBuffer::GetUInt32()
{
    if (mPos + 4 > mSize)
        throw InsufficientBytes();

    uint32_t value;
    if (mBigEndian) {
        value = ((uint32_t)mData[mPos] << 24) |
                ((uint32_t)mData[mPos + 1] << 16) |
                ((uint32_t)mData[mPos + 2] << 8) |
                mData[mPos + 3];
    } else {
        value = mData[mPos] |
                ((uint32_t)mData[mPos + 1] << 8) |
                ((uint32_t)mData[mPos + 2] << 16) |
                ((uint32_t)mData[mPos + 3] << 24);
    }
    mPos += 4;

    return value;
}

uint64_t ByteBuffer::GetUInt64()
{
    if (mPos + 8 > mSize)
        throw InsufficientBytes();

    uint64_t value;
    if (mBigEndian) {
        value = ((uint64_t)mData[mPos] << 56) |
                ((uint64_t)mData[mPos + 1] << 48) |
                ((uint64_t)mData[mPos + 2] << 40) |
                ((uint64_t)mData[mPos + 3] << 32) |
                ((uint64_t)mData[mPos + 4] << 24) |
                ((uint64_t)mData[mPos + 5] << 16) |
                ((uint64_t)mData[mPos + 6] << 8) |
                mData[mPos + 7];
    } else {
        value = mData[mPos] |
                ((uint64_t)mData[mPos + 1] << 8) |
                ((uint64_t)mData[mPos + 2] << 16) |
                ((uint64_t)mData[mPos + 3] << 24) |
                ((uint64_t)mData[mPos + 4] << 32) |
                ((uint64_t)mData[mPos + 5] << 40) |
                ((uint64_t)mData[mPos + 6] << 48) |
                ((uint64_t)mData[mPos + 7] << 56);
    }
    mPos += 8;

    return value;
}

vector<unsigned char> ByteBuffer::GetBytes(uint32_t size)
{
    if (mPos + size > mSize)
        throw InsufficientBytes();

    return vector<unsigned char>(&mData[mPos], &mData[mPos + size]);
}

void ByteBuffer::Skip(uint32_t size)
{
    if (mPos + size > mSize)
        throw InsufficientBytes();

    mPos += size;
}

void ByteBuffer::SetPos(uint32_t pos)
{
    if (pos > mSize)
        throw InsufficientBytes();

    mPos = pos;
}


ByteBufferLengthContext::ByteBufferLengthContext(ByteBuffer &data, uint32_t length)
: mData(data), mEndPos(data.GetPos() + length)
{
    if (mEndPos > mData.GetSize())
        throw InsufficientBytes();
}

ByteBufferLengthContext::~ByteBufferLengthContext()
{
    mData.SetPos(mEndPos);
}
