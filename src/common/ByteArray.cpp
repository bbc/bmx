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

#include <cstring>

#include <bmx/ByteArray.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



ByteArray::ByteArray()
{
    mBytes = 0;
    mSize = 0;
    mIsCopy = false;
    mAllocatedSize = 0;
    mAllocBlockSize = 256;
}

ByteArray::ByteArray(uint32_t size)
{
    mBytes = 0;
    mSize = 0;
    mIsCopy = false;
    mAllocatedSize = 0;
    mAllocBlockSize = 256;

    Allocate(size);
}

ByteArray::~ByteArray()
{
    if (!mIsCopy)
        delete [] mBytes;
}

void ByteArray::SetAllocBlockSize(uint32_t block_size)
{
    BMX_CHECK(block_size > 0);
    mAllocBlockSize = block_size;
}

unsigned char* ByteArray::GetBytes() const
{
    return mBytes;
}

uint32_t ByteArray::GetSize() const
{
    return mSize;
}

void ByteArray::Append(const unsigned char *bytes, uint32_t size)
{
    BMX_ASSERT(!mIsCopy);

    if (size == 0)
        return;

    Grow(size);

    memcpy(mBytes + mSize, bytes, size);
    mSize += size;
}

unsigned char* ByteArray::GetBytesAvailable() const
{
    BMX_ASSERT(!mIsCopy);

    if (mSize == mAllocatedSize)
        return 0;

    return mBytes + mSize;
}

uint32_t ByteArray::GetSizeAvailable() const
{
    return mAllocatedSize - mSize;
}

void ByteArray::SetSize(uint32_t size)
{
    if (size > mAllocatedSize)
        BMX_EXCEPTION(("Cannot set byte array size > allocated size"));

    mSize = size;
}

void ByteArray::IncrementSize(uint32_t inc)
{
    if (mSize + inc > mAllocatedSize)
        BMX_EXCEPTION(("Cannot set byte array size > allocated size"));

    mSize += inc;
}

void ByteArray::CopyBytes(const unsigned char *bytes, uint32_t size)
{
    SetSize(0);
    Append(bytes, size);
}

void ByteArray::AssignBytes(unsigned char *bytes, uint32_t size)
{
    Clear();

    mBytes = bytes;
    mSize = size;
    mAllocatedSize = 0;
    mIsCopy = true;
}

void ByteArray::Grow(uint32_t min_size)
{
    BMX_ASSERT(!mIsCopy);

    if (mSize + min_size > mAllocatedSize)
        Reallocate(mSize + min_size);
}

void ByteArray::Allocate(uint32_t min_size)
{
    BMX_ASSERT(!mIsCopy);

    if (mAllocatedSize >= min_size)
        return;

    uint32_t size = ((min_size / mAllocBlockSize) + 1) * mAllocBlockSize;

    delete [] mBytes;
    mBytes = 0;
    mSize = 0;
    mAllocatedSize = 0;

    mBytes = new unsigned char[size];
    memset(mBytes, 0, size);
    mAllocatedSize = size;
}

void ByteArray::Reallocate(uint32_t min_size)
{
    BMX_ASSERT(!mIsCopy);

    if (mAllocatedSize >= min_size)
        return;

    if (mSize == 0) {
        delete [] mBytes;
        mBytes = 0;
    }

    uint32_t size = ((min_size / mAllocBlockSize) + 1) * mAllocBlockSize;

    unsigned char *newBytes = new unsigned char[size];
    if (mSize > 0) {
        memcpy(newBytes, mBytes, mSize);
        delete [] mBytes;
        mBytes = 0;
    }
    memset(&newBytes[mSize], 0, size - mSize);
    mBytes = newBytes;
    mAllocatedSize = size;

    if (size < mSize)
        mSize = size;
}

uint32_t ByteArray::GetAllocatedSize() const
{
    return mAllocatedSize;
}

void ByteArray::Clear()
{
    if (mIsCopy) {
        mBytes = 0;
        mSize = 0;
        mAllocatedSize = 0;
        mIsCopy = false;
    } else {
        delete [] mBytes;
        mBytes = 0;
        mSize = 0;
        mAllocatedSize = 0;
    }
}

