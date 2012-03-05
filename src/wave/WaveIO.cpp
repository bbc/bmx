/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#include <bmx/wave/WaveIO.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



WaveIO::WaveIO()
{
}

WaveIO::~WaveIO()
{
}

void WaveIO::WriteTag(const char *tag)
{
    Write((const unsigned char*)tag, 4);
}

void WaveIO::WriteZeros(uint32_t size)
{
    static const unsigned char zeros[256] = {0};

    uint32_t whole_count   = size / sizeof(zeros);
    uint32_t partial_count = size % sizeof(zeros);

    uint32_t i;
    for (i = 0; i < whole_count; i++)
        Write(zeros, sizeof(zeros));

    if (partial_count > 0)
        Write(zeros, partial_count);
}

void WaveIO::WriteString(const char *value, uint32_t len, uint32_t fixed_size)
{
    if (len < fixed_size) {
        Write((const unsigned char*)value, len);
        WriteZeros(fixed_size - len);
    } else {
        Write((const unsigned char*)value, fixed_size);
    }
}

void WaveIO::WriteUInt8(uint8_t value)
{
    PutChar(value);
}

void WaveIO::WriteUInt16(uint16_t value)
{
    PutChar((unsigned char)( value        & 0xff));
    PutChar((unsigned char)((value >> 8)  & 0xff));
}

void WaveIO::WriteUInt32(uint32_t value)
{
    PutChar((unsigned char)( value        & 0xff));
    PutChar((unsigned char)((value >> 8 ) & 0xff));
    PutChar((unsigned char)((value >> 16) & 0xff));
    PutChar((unsigned char)((value >> 24) & 0xff));
}

void WaveIO::WriteUInt64(uint64_t value)
{
    PutChar((unsigned char)( value        & 0xff));
    PutChar((unsigned char)((value >> 8 ) & 0xff));
    PutChar((unsigned char)((value >> 16) & 0xff));
    PutChar((unsigned char)((value >> 24) & 0xff));
    PutChar((unsigned char)((value >> 32) & 0xff));
    PutChar((unsigned char)((value >> 40) & 0xff));
    PutChar((unsigned char)((value >> 48) & 0xff));
    PutChar((unsigned char)((value >> 56) & 0xff));
}

void WaveIO::WriteInt8(int8_t value)
{
    PutChar(value);
}

void WaveIO::WriteInt16(int16_t value)
{
    PutChar((unsigned char)(  (uint16_t)value         & 0xff));
    PutChar((unsigned char)((((uint16_t)value) >> 8)  & 0xff));
}

void WaveIO::WriteInt32(int32_t value)
{
    PutChar((unsigned char)(( (uint32_t)value)        & 0xff));
    PutChar((unsigned char)((((uint32_t)value) >> 8 ) & 0xff));
    PutChar((unsigned char)((((uint32_t)value) >> 16) & 0xff));
    PutChar((unsigned char)((((uint32_t)value) >> 24) & 0xff));
}

void WaveIO::WriteInt64(int64_t value)
{
    PutChar((unsigned char)(( (uint64_t)value)        & 0xff));
    PutChar((unsigned char)((((uint64_t)value) >> 8 ) & 0xff));
    PutChar((unsigned char)((((uint64_t)value) >> 16) & 0xff));
    PutChar((unsigned char)((((uint64_t)value) >> 24) & 0xff));
    PutChar((unsigned char)((((uint64_t)value) >> 32) & 0xff));
    PutChar((unsigned char)((((uint64_t)value) >> 40) & 0xff));
    PutChar((unsigned char)((((uint64_t)value) >> 48) & 0xff));
    PutChar((unsigned char)((((uint64_t)value) >> 56) & 0xff));
}

bool WaveIO::ReadTag(char *tag)
{
    // could fail if reached eof
    return Read((unsigned char*)tag, 4) == 4;
}

void WaveIO::ReadString(char *value, uint32_t fixed_size)
{
    if (Read((unsigned char*)value, fixed_size) != fixed_size)
        BMX_EXCEPTION(("Failed to read wave string with fixed size %u\n", fixed_size));
}

uint8_t WaveIO::ReadUInt8()
{
    unsigned char buffer[1];
    BMX_CHECK(Read(buffer, 1) == 1);

    return (uint8_t)buffer[0];
}

uint16_t WaveIO::ReadUInt16()
{
    unsigned char buffer[2];
    BMX_CHECK(Read(buffer, 2) == 2);

    return ((uint16_t)buffer[0]     ) |
           ((uint16_t)buffer[1] << 8);
}

uint32_t WaveIO::ReadUInt32()
{
    unsigned char buffer[4];
    BMX_CHECK(Read(buffer, 4) == 4);

    return ((uint32_t)buffer[0]      ) |
           ((uint32_t)buffer[1] << 8 ) |
           ((uint32_t)buffer[2] << 16) |
           ((uint32_t)buffer[3] << 24);
}

uint64_t WaveIO::ReadUInt64()
{
    unsigned char buffer[8];
    BMX_CHECK(Read(buffer, 8) == 8);

    return ((uint64_t)buffer[0]      ) |
           ((uint64_t)buffer[1] << 8 ) |
           ((uint64_t)buffer[2] << 16) |
           ((uint64_t)buffer[3] << 24) |
           ((uint64_t)buffer[4] << 32) |
           ((uint64_t)buffer[5] << 40) |
           ((uint64_t)buffer[6] << 48) |
           ((uint64_t)buffer[7] << 56);
}

int8_t WaveIO::ReadInt8()
{
    return (int8_t)ReadUInt8();
}

int16_t WaveIO::ReadInt16()
{
    return (int16_t)ReadUInt16();
}

int32_t WaveIO::ReadInt32()
{
    return (int32_t)ReadUInt32();
}

int64_t WaveIO::ReadInt64()
{
    return (int64_t)ReadUInt64();
}

