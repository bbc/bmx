/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <cstring>

#include <memory>

#include <bmx/BitBuffer.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



GetBitBuffer::GetBitBuffer(const unsigned char *data, uint32_t size)
{
    mData = data;
    mSize = size;
    mPos = 0;
    mBitSize = (uint64_t)size << 3;
    mBitPos = 0;
}

void GetBitBuffer::GetBytes(uint32_t request_size, const unsigned char **data, uint32_t *size)
{
    BMX_ASSERT(!(mBitPos & 0x07));

    *size = request_size;
    if ((*size) > mSize - mPos)
        *size = mSize - mPos;
    if ((*size) > 0) {
        *data = &mData[mPos];
        mPos += *size;
        mBitPos += (uint64_t)(*size) << 3;
    } else {
        *data = 0;
    }
}

bool GetBitBuffer::GetUInt8(uint8_t *value)
{
    BMX_ASSERT(!(mBitPos & 0x07));

    if (mPos >= mSize)
        return false;

    *value = mData[mPos];
    mPos++;
    mBitPos += 8;

    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, uint8_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    *value = (uint8_t)u64_value;
    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, uint16_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    *value = (uint16_t)u64_value;
    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, uint32_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    *value = (uint32_t)u64_value;
    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, uint64_t *value)
{
    if (num_bits > mBitSize - mBitPos)
        return false;

    if (num_bits > 0)
        GetBits(mData, &mPos, &mBitPos, num_bits, value);
    else
        *value = 0;

    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, int8_t *value)
{
    int64_t i64_value;
    if (!GetBits(num_bits, &i64_value))
        return false;

    *value = (int32_t)i64_value;
    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, int16_t *value)
{
    int64_t i64_value;
    if (!GetBits(num_bits, &i64_value))
        return false;

    *value = (int32_t)i64_value;
    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, int32_t *value)
{
    int64_t i64_value;
    if (!GetBits(num_bits, &i64_value))
        return false;

    *value = (int32_t)i64_value;
    return true;
}

bool GetBitBuffer::GetBits(uint8_t num_bits, int64_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    int64_t sign = (int64_t)(UINT64_C(1) << (num_bits - 1));
    *value = ((int64_t)u64_value ^ sign) - sign;

    return true;
}

void GetBitBuffer::SetBitPos(uint64_t pos)
{
    mBitPos = pos;
    mPos = (uint32_t)(mBitPos >> 3);
}

void GetBitBuffer::GetBits(const unsigned char *data, uint32_t *pos_io, uint64_t *bit_pos_io, uint8_t num_bits,
                           uint64_t *value)
{
    BMX_ASSERT(num_bits > 0 || num_bits <= 64);

    const unsigned char *data_ptr = &data[*pos_io];
    uint8_t min_consume_bits = (uint8_t)(((*bit_pos_io) & 0x07) + num_bits);
    int16_t consumed_bits = 0;
    *value = 0;
    while (consumed_bits < min_consume_bits) {
        *value = ((*value) << 8) | (*data_ptr);
        data_ptr++;
        consumed_bits += 8;
    }

    *value >>= consumed_bits - min_consume_bits;
    if (consumed_bits > 64)
        *value |= ((uint64_t)data[*pos_io]) << (64 - (consumed_bits - min_consume_bits));
    *value &= UINT64_MAX >> (64 - num_bits);

    *bit_pos_io += num_bits;
    *pos_io      = (uint32_t)((*bit_pos_io) >> 3);
}



PutBitBuffer::PutBitBuffer(ByteArray *w_buffer)
{
    mWBuffer = w_buffer;
    mRWBytes = 0;
    mRWBytesSize = 0;
    mRWBytesPos = 0;
    mBitSize = (uint64_t)w_buffer->GetSize() << 3;
}

PutBitBuffer::PutBitBuffer(unsigned char *bytes, uint32_t size)
{
    mWBuffer = 0;
    mRWBytes = bytes;
    mRWBytesSize = size;
    mRWBytesPos = 0;
    mBitSize = 0;
}

uint32_t PutBitBuffer::GetSize() const
{
    if (mWBuffer)
        return mWBuffer->GetSize();
    else
        return mRWBytesPos;
}

void PutBitBuffer::PutBytes(const unsigned char *data, uint32_t size)
{
    BMX_ASSERT(!(mBitSize & 0x07));

    Append(data, size);
    mBitSize += (uint64_t)size << 3;
}

void PutBitBuffer::PutUInt8(uint8_t value)
{
    BMX_ASSERT(!(mBitSize & 0x07));

    Append(&value, 1);
    mBitSize += 8;
}

void PutBitBuffer::PutBits(uint8_t num_bits, uint8_t value)
{
    PutBits(num_bits, (uint64_t)value);
}

void PutBitBuffer::PutBits(uint8_t num_bits, uint16_t value)
{
    PutBits(num_bits, (uint64_t)value);
}

void PutBitBuffer::PutBits(uint8_t num_bits, uint32_t value)
{
    PutBits(num_bits, (uint64_t)value);
}

void PutBitBuffer::PutBits(uint8_t num_bits, uint64_t value)
{
    uint8_t byte_bitpos = (uint8_t)(mBitSize & 0x07);
    uint8_t next_byte_bitpos = (uint8_t)((mBitSize + num_bits) & 0x07);
    uint8_t num_bytes = (byte_bitpos + num_bits + 7) / 8;
    uint8_t incr_size = num_bytes;
    if (byte_bitpos != 0)
        incr_size -= 1;
    Grow(incr_size);

    unsigned char *bytes = GetBytesAvailable();
    if (byte_bitpos != 0)
        bytes--;
    uint64_t shift_value; // bits shifted to msb
    if ((64 - num_bits - byte_bitpos) >= 0)
        shift_value = value << (64 - num_bits - byte_bitpos);
    else
        shift_value = value >> (- (64 - num_bits - byte_bitpos)); // existing + new bits > 64
    uint8_t first_byte_mask = (uint8_t)(0xff << (8 - byte_bitpos));
    uint8_t last_byte_mask = 0;
    if (next_byte_bitpos != 0)
        last_byte_mask = (uint8_t)(0xff >> next_byte_bitpos);
    uint8_t existing_byte_mask;
    uint8_t i;
    for (i = 0; i < num_bytes; i++) {
        existing_byte_mask = 0;
        if (i == 0)
            existing_byte_mask |= first_byte_mask;
        if (i == num_bytes - 1)
            existing_byte_mask |= last_byte_mask;
        bytes[i] = (bytes[i] & existing_byte_mask) | (uint8_t)((shift_value >> 56) & 0xff);
        shift_value <<= 8;
        if (i == 0) // reinstate bits shifted out if (64 - num_bits - byte_bitpos) < 0
            shift_value |= ((value << 8) >> byte_bitpos) & 0xff;
    }

    IncrementSize(incr_size);
    mBitSize += num_bits;
}

void PutBitBuffer::PutBits(uint8_t num_bits, int8_t value)
{
    PutBits(num_bits, (int64_t)value);
}

void PutBitBuffer::PutBits(uint8_t num_bits, int16_t value)
{
    PutBits(num_bits, (int64_t)value);
}

void PutBitBuffer::PutBits(uint8_t num_bits, int32_t value)
{
    PutBits(num_bits, (int64_t)value);
}

void PutBitBuffer::PutBits(uint8_t num_bits, int64_t value)
{
    uint64_t sign = UINT64_C(1) << (num_bits - 1);
    uint64_t u_value;
    if (value < 0)
        u_value = sign | (value & (sign - 1));
    else
        u_value = value;

    PutBits(num_bits, u_value);
}

void PutBitBuffer::SetBitPos(uint64_t pos)
{
    BMX_ASSERT(mRWBytes); // currently only implemented for mRWBytes
    mBitSize    = pos;
    mRWBytesPos = (uint32_t)((pos + 7) >> 3);
}

void PutBitBuffer::Append(const unsigned char *bytes, uint32_t size)
{
    if (mWBuffer) {
        mWBuffer->Append(bytes, size);
    } else {
        BMX_CHECK(mRWBytesPos + size <= mRWBytesSize);
        memcpy(&mRWBytes[mRWBytesPos], bytes, size);
        mRWBytesPos += size;
    }
}

unsigned char* PutBitBuffer::GetBytesAvailable() const
{
    if (mWBuffer) {
        return mWBuffer->GetBytesAvailable();
    } else {
        if (mRWBytesPos >= mRWBytesSize)
            return 0;
        else
            return &mRWBytes[mRWBytesPos];
    }
}

void PutBitBuffer::IncrementSize(uint32_t inc)
{
    if (mWBuffer) {
        mWBuffer->IncrementSize(inc);
    } else {
        BMX_CHECK(inc <= mRWBytesSize && mRWBytesPos <= mRWBytesSize - inc);
        mRWBytesPos += inc;
    }
}

void PutBitBuffer::Grow(uint32_t min_size)
{
    if (mWBuffer) {
        mWBuffer->Grow(min_size);
        if (min_size > 0)
            memset(mWBuffer->GetBytesAvailable(), 0, min_size);
    } else {
        BMX_CHECK(min_size <= mRWBytesSize && mRWBytesPos <= mRWBytesSize - min_size);
    }
}

