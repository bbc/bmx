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

#ifndef BMX_BYTE_ARRAY_H_
#define BMX_BYTE_ARRAY_H_


#include <bmx/BMXTypes.h>


namespace bmx
{


class ByteArray
{
public:
    ByteArray();
    ByteArray(uint32_t size);
    ~ByteArray();

    void SetAllocBlockSize(uint32_t block_size);

    unsigned char* GetBytes() const;
    uint32_t GetSize() const;

    void Append(const unsigned char *bytes, uint32_t size);
    unsigned char* GetBytesAvailable() const;
    uint32_t GetSizeAvailable() const;
    void SetSize(uint32_t size);
    void IncrementSize(uint32_t inc);

    void CopyBytes(const unsigned char *bytes, uint32_t size);
    void AssignBytes(unsigned char *bytes, uint32_t size);

    void Grow(uint32_t min_size);
    void Allocate(uint32_t min_size);
    void Reallocate(uint32_t min_size);

    uint32_t GetAllocatedSize() const;

    void Clear();

private:
    unsigned char *mBytes;
    uint32_t mSize;
    bool mIsCopy;
    uint32_t mAllocatedSize;
    uint32_t mAllocBlockSize;
};


};



#endif

