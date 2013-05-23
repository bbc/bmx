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

#ifndef BMX_BIT_BUFFER_H_
#define BMX_BIT_BUFFER_H_


#include <bmx/BMXTypes.h>
#include <bmx/ByteArray.h>



namespace bmx
{



class GetBitBuffer
{
public:
    GetBitBuffer(const unsigned char *data, uint32_t size);

    const unsigned char* GetData() const { return mData; }

    uint32_t GetSize() const             { return mSize; }
    uint32_t GetPos() const              { return mPos; }
    uint32_t GetRemSize() const          { return mSize - mPos; }

    uint64_t GetBitSize() const          { return mBitSize; }
    uint64_t GetBitPos() const           { return mBitPos; }
    uint64_t GetRemBitSize() const       { return mBitSize - mBitPos; }


    void GetBytes(uint32_t request_size, const unsigned char **data, uint32_t *size);

    bool GetUInt8(uint8_t *value);

    bool GetBits(uint8_t num_bits, uint8_t *value);
    bool GetBits(uint8_t num_bits, uint16_t *value);
    bool GetBits(uint8_t num_bits, uint32_t *value);
    bool GetBits(uint8_t num_bits, uint64_t *value);

    bool GetBits(uint8_t num_bits, int8_t *value);
    bool GetBits(uint8_t num_bits, int16_t *value);
    bool GetBits(uint8_t num_bits, int32_t *value);
    bool GetBits(uint8_t num_bits, int64_t *value);

    void SetBitPos(uint64_t pos);

protected:
    void GetBits(const unsigned char *data, uint32_t *pos_io, uint64_t *bit_pos_io, uint8_t num_bits, uint64_t *value);

protected:
    const unsigned char *mData;
    uint32_t mSize;
    uint32_t mPos;
    uint64_t mBitSize;
    uint64_t mBitPos;
};


class PutBitBuffer
{
public:
    PutBitBuffer(ByteArray *w_buffer);
    PutBitBuffer(unsigned char *rw_bytes, uint32_t size);

    ByteArray* GetBuffer() const { return mWBuffer; }

    uint32_t GetSize() const;
    uint64_t GetBitSize() const  { return mBitSize; }

    void PutBytes(const unsigned char *data, uint32_t size);

    void PutUInt8(uint8_t value);

    void PutBits(uint8_t num_bits, uint8_t value);
    void PutBits(uint8_t num_bits, uint16_t value);
    void PutBits(uint8_t num_bits, uint32_t value);
    void PutBits(uint8_t num_bits, uint64_t value);
    void PutBits(uint8_t num_bits, int8_t value);
    void PutBits(uint8_t num_bits, int16_t value);
    void PutBits(uint8_t num_bits, int32_t value);
    void PutBits(uint8_t num_bits, int64_t value);

    void SetBitPos(uint64_t pos);

protected:
    ByteArray *mWBuffer;
    unsigned char *mRWBytes;
    uint32_t mRWBytesSize;
    uint32_t mRWBytesPos;
    uint64_t mBitSize;

private:
    void Append(const unsigned char *bytes, uint32_t size);
    unsigned char* GetBytesAvailable() const;
    void IncrementSize(uint32_t inc);
    void Grow(uint32_t min_size);
};


};



#endif

