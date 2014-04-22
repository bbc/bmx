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

#ifndef BMX_RDD6_BIT_BUFFER_H_
#define BMX_RDD6_BIT_BUFFER_H_


#include <bmx/BitBuffer.h>



namespace bmx
{



class RDD6GetBitBuffer
{
public:
    RDD6GetBitBuffer(const unsigned char *data_a, uint32_t size_a,
                     const unsigned char *data_b, uint32_t size_b);

    uint32_t GetRemSize() const;
    uint64_t GetRemBitSize() const;

    void GetBytes(uint32_t size,
                  const unsigned char **data_a, uint32_t *size_a,
                  const unsigned char **data_b, uint32_t *size_b);

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

private:
    void GetBits(const unsigned char *data, uint32_t *pos_io, uint64_t *bit_pos_io, uint8_t num_bits, uint64_t *value);

private:
    const unsigned char *mDataA;
    uint32_t mSizeA;
    uint32_t mPosA;
    uint64_t mBitSizeA;
    uint64_t mBitPosA;
    const unsigned char *mDataB;
    uint32_t mSizeB;
    uint32_t mPosB;
    uint64_t mBitSizeB;
    uint64_t mBitPosB;
};


};



#endif

