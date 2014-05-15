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

#include <bmx/st436/RDD6BitBuffer.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



RDD6GetBitBuffer::RDD6GetBitBuffer(const unsigned char *data_a, uint32_t size_a,
                                   const unsigned char *data_b, uint32_t size_b)
{
    BMX_ASSERT((uint64_t)size_a + (uint64_t)size_b < UINT32_MAX);

    mDataA = data_a;
    mSizeA = size_a;
    mPosA = 0;
    mBitSizeA = (uint64_t)size_a << 3;
    mBitPosA = 0;
    mDataB = data_b;
    mSizeB = size_b;
    mPosB = 0;
    mBitSizeB = (uint64_t)size_b << 3;
    mBitPosB = 0;
}

uint32_t RDD6GetBitBuffer::GetRemSize() const
{
    return (mSizeA - mPosA) + (mSizeB - mPosB);
}

uint64_t RDD6GetBitBuffer::GetRemBitSize() const
{
    return (mBitSizeA - mBitPosA) + (mBitSizeB - mBitPosB);
}

void RDD6GetBitBuffer::GetBytes(uint32_t size,
                                const unsigned char **data_a, uint32_t *size_a,
                                const unsigned char **data_b, uint32_t *size_b)
{
    BMX_ASSERT(!((mBitPosA & 0x07) || (mBitPosB & 0x07)));

    *size_a = size;
    if ((*size_a) > mSizeA - mPosA)
        *size_a = mSizeA - mPosA;
    if ((*size_a) > 0) {
        *data_a = &mDataA[mPosA];
        mPosA += *size_a;
        mBitPosA += (*size_a) << 3;
    } else {
        *data_a = 0;
    }

    *size_b = size - (*size_a);
    if ((*size_b) > mSizeB - mPosB)
        *size_b = mSizeB - mPosB;
    if ((*size_b) > 0) {
        *data_b = &mDataB[mPosB];
        mPosB += *size_b;
        mBitPosB += (*size_b) << 3;
    } else {
        *data_b = 0;
    }
}

bool RDD6GetBitBuffer::GetUInt8(uint8_t *value)
{
    BMX_ASSERT(!((mBitPosA & 0x07) || (mBitPosB & 0x07)));

    if (mPosA < mSizeA) {
        *value = mDataA[mPosA];
        mPosA++;
        mBitPosA += 8;
        return true;
    } else if (mPosB < mSizeB) {
        *value = mDataB[mPosB];
        mPosB++;
        mBitPosB += 8;
        return true;
    } else {
        return false;
    }
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, uint8_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    *value = (uint8_t)u64_value;
    return true;
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, uint16_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    *value = (uint16_t)u64_value;
    return true;
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, uint32_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    *value = (uint32_t)u64_value;
    return true;
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, uint64_t *value)
{
    if (GetRemBitSize() < num_bits)
        return false;

    *value = 0;

    uint8_t num_bits_a = num_bits;
    if (num_bits_a > mBitSizeA - mBitPosA)
        num_bits_a = (uint8_t)(mBitSizeA - mBitPosA);
    if (num_bits_a > 0) {
        uint64_t value_a;
        GetBits(mDataA, &mPosA, &mBitPosA, num_bits_a, &value_a);
        *value = value_a << (num_bits - num_bits_a);
    }

    uint8_t num_bits_b = num_bits - num_bits_a;
    BMX_ASSERT(num_bits_b <= mBitSizeB - mBitPosB);
    if (num_bits_b > 0) {
        uint64_t value_b;
        GetBits(mDataB, &mPosB, &mBitPosB, num_bits_b, &value_b);
        *value |= value_b;
    }

    return true;
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, int8_t *value)
{
    int64_t i64_value;
    if (!GetBits(num_bits, &i64_value))
        return false;

    *value = (int32_t)i64_value;
    return true;
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, int16_t *value)
{
    int64_t i64_value;
    if (!GetBits(num_bits, &i64_value))
        return false;

    *value = (int32_t)i64_value;
    return true;
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, int32_t *value)
{
    int64_t i64_value;
    if (!GetBits(num_bits, &i64_value))
        return false;

    *value = (int32_t)i64_value;
    return true;
}

bool RDD6GetBitBuffer::GetBits(uint8_t num_bits, int64_t *value)
{
    uint64_t u64_value;
    if (!GetBits(num_bits, &u64_value))
        return false;

    int64_t sign = (int64_t)(UINT64_C(1) << (num_bits - 1));
    *value = ((int64_t)u64_value ^ sign) - sign;

    return true;
}

void RDD6GetBitBuffer::SetBitPos(uint64_t pos)
{
    if (pos < mBitSizeA) {
        mBitPosA = pos;
        mPosA = (uint32_t)(mBitPosA >> 3);
        mBitPosB = 0;
        mPosB = 0;
    } else {
        mBitPosA = mBitSizeA;
        mPosA = mSizeA;
        mBitPosB = pos - mBitSizeA;
        mPosB = (uint32_t)(mBitPosB >> 3);
    }
}

void RDD6GetBitBuffer::GetBits(const unsigned char *data, uint32_t *pos_io, uint64_t *bit_pos_io, uint8_t num_bits,
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

