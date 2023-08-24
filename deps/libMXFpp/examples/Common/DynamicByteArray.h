/*
 * Copyright (C) 2008, British Broadcasting Corporation
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

#ifndef __DYNAMIC_BYTE_ARRAY_H__
#define __DYNAMIC_BYTE_ARRAY_H__

#include "CommonTypes.h"



class DynamicByteArray
{
public:
    DynamicByteArray();
    DynamicByteArray(uint32_t size);
    ~DynamicByteArray();

    void setAllocIncrement(uint32_t increment);

    unsigned char* getBytes() const;
    uint32_t getSize() const;
    uint32_t getAllocatedSize() const;

    void setBytes(const unsigned char *bytes, uint32_t size);

    void append(const unsigned char *bytes, uint32_t size);
    void appendZeros(uint32_t size);
    unsigned char* getBytesAvailable() const;
    uint32_t getSizeAvailable() const;
    void setSize(uint32_t size);

    void setCopy(unsigned char *bytes, uint32_t size);

    void grow(uint32_t size);
    void allocate(uint32_t size);
    void minAllocate(uint32_t min_size);
    void reallocate(uint32_t size);

    void clear();

    unsigned char& operator[](uint32_t index) const;

private:
    unsigned char* _bytes;
    uint32_t _size;
    bool _isCopy;
    uint32_t _allocatedSize;
    uint32_t _increment;
};



#endif

