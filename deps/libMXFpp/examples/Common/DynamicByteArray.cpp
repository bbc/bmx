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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>

#include <libMXF++/MXFException.h>

#include "CommonTypes.h"
#include "DynamicByteArray.h"

using namespace std;
using namespace mxfpp;


#define SAFE_ARRAY_DELETE(var) \
{ \
    delete [] var; \
    var = 0; \
}



DynamicByteArray::DynamicByteArray()
: _bytes(0), _size(0), _isCopy(false), _allocatedSize(0), _increment(256)
{}

DynamicByteArray::DynamicByteArray(uint32_t size)
: _bytes(0), _size(0), _isCopy(false), _allocatedSize(0), _increment(256)
{
    allocate(size);
}

DynamicByteArray::~DynamicByteArray()
{
    if (!_isCopy)
    {
        SAFE_ARRAY_DELETE(_bytes);
    }
}

void DynamicByteArray::setAllocIncrement(uint32_t increment)
{
    _increment = increment;
}

unsigned char *DynamicByteArray::getBytes() const
{
    return _bytes;
}

uint32_t DynamicByteArray::getAllocatedSize() const
{
    return _allocatedSize;
}

uint32_t DynamicByteArray::getSize() const
{
    return _size;
}

void DynamicByteArray::setBytes(const unsigned char *bytes, uint32_t size)
{
    setSize(0);
    append(bytes, size);
}

void DynamicByteArray::append(const unsigned char *bytes, uint32_t size)
{
    if (size == 0)
    {
        return;
    }

    if (_isCopy)
    {
        throw MXFException("Cannot append byte array data to a copy\n");
    }

    grow(size);

    memcpy(_bytes + _size, bytes, size);
    _size += size;
}

void DynamicByteArray::appendZeros(uint32_t size)
{
    if (size == 0)
    {
        return;
    }

    if (_isCopy)
    {
        throw MXFException("Cannot append byte array data to a copy\n");
    }

    grow(size);

    memset(_bytes + _size, 0, size);
    _size += size;
}

unsigned char *DynamicByteArray::getBytesAvailable() const
{
    if (_isCopy)
    {
        throw MXFException("Cannot get available byte array data for a copy\n");
    }

    if (_size == _allocatedSize)
    {
        return NULL;
    }

    return _bytes + _size;
}

uint32_t DynamicByteArray::getSizeAvailable() const
{
    return _allocatedSize - _size;
}

void DynamicByteArray::setSize(uint32_t size)
{
    if (size > _allocatedSize)
    {
        throw MXFException("Cannot set byte array size > allocated size\n");
    }

    _size = size;
}

void DynamicByteArray::setCopy(unsigned char *bytes, uint32_t size)
{
    clear();

    _bytes = bytes;
    _size = size;
    _allocatedSize = 0;
    _isCopy = 1;
}

void DynamicByteArray::grow(uint32_t size)
{
    if (_isCopy)
    {
        throw MXFException("Cannot grow a byte array that is a copy\n");
    }

    if (_size + size > _allocatedSize)
    {
        reallocate(_size + size);
    }
}

void DynamicByteArray::allocate(uint32_t size)
{
    if (_isCopy)
    {
        throw MXFException("Cannot allocate a byte array that is a copy\n");
    }

    SAFE_ARRAY_DELETE(_bytes);
    _size = 0;
    _allocatedSize = 0;

    _bytes = new unsigned char[size];
    _allocatedSize = size;
}

void DynamicByteArray::minAllocate(uint32_t min_size)
{
    if (_allocatedSize < min_size)
        allocate(min_size);
}

void DynamicByteArray::reallocate(uint32_t size)
{
    unsigned char *newBytes;

    if (_isCopy)
    {
        throw MXFException("Cannot reallocate a byte array that is a copy\n");
    }

    newBytes = new unsigned char[size];
    if (_size > 0)
    {
        memcpy(newBytes, _bytes, _size);
    }

    SAFE_ARRAY_DELETE(_bytes);
    _bytes = newBytes;
    _allocatedSize = size;

    if (size < _size)
    {
        _size = size;
    }
}

void DynamicByteArray::clear()
{
    if (_isCopy)
    {
        _bytes = 0;
        _size = 0;
        _allocatedSize = 0;
        _isCopy = false;
    }
    else
    {
        SAFE_ARRAY_DELETE(_bytes);
        _size = 0;
        _allocatedSize = 0;
    }
}

unsigned char& DynamicByteArray::operator[](uint32_t index) const
{
    return _bytes[index];
}


