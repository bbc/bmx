/*
 * Copyright (C) 2022, British Broadcasting Corporation
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

#include <cerrno>

#include <bmx/BMXIO.h>
#include <bmx/ByteArray.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>
#include <bmx/Utils.h>

using namespace std;
using namespace bmx;



BMXIO::BMXIO()
{
}

BMXIO::~BMXIO()
{
}

uint32_t BMXIO::Read(unsigned char *data, uint32_t size)
{
    (void)data;
    (void)size;
    BMX_ASSERT(false);
    return 0;
}

void BMXIO::Read(FILE *file)
{
    ByteArray buffer;
    buffer.Allocate(8192);

    while (true) {
        uint32_t num_read = Read(buffer.GetBytesAvailable(), buffer.GetAllocatedSize());

        if (num_read > 0) {
            size_t num_write = fwrite(buffer.GetBytesAvailable(), 1, num_read, file);
            if (num_write != num_read) {
                BMX_EXCEPTION(("Failed to write to output file: %s",
                               bmx_strerror(errno).c_str()));
            }
        }

        if (num_read < buffer.GetAllocatedSize())
            break;
    }
}

void BMXIO::Read(BMXIO *bmx_io)
{
    ByteArray buffer;
    buffer.Allocate(8192);

    while (true) {
        uint32_t num_read = Read(buffer.GetBytesAvailable(), buffer.GetAllocatedSize());

        if (num_read > 0) {
            uint32_t num_write = bmx_io->Write(buffer.GetBytesAvailable(), num_read);
            if (num_write != num_read)
                BMX_EXCEPTION(("Failed to write to output file"));
        }

        if (num_read < buffer.GetAllocatedSize())
            break;
    }
}

uint32_t BMXIO::Write(const unsigned char *data, uint32_t size)
{
    (void)data;
    (void)size;
    BMX_ASSERT(false);
    return 0;
}

void BMXIO::Write(FILE *file)
{
    ByteArray buffer;
    buffer.Allocate(8192);

    while (true) {
        size_t num_read = fread(buffer.GetBytesAvailable(), 1, buffer.GetAllocatedSize(), file);
        if (ferror(file)) {
            BMX_EXCEPTION(("Failed to read from input file: %s",
                           bmx_strerror(errno).c_str()));
        }

        if (num_read > 0) {
            uint32_t num_write = Write(buffer.GetBytesAvailable(), num_read);
            if (num_write != num_read)
                BMX_EXCEPTION(("Failed to write to output file"));
        }

        if (num_read < buffer.GetAllocatedSize())
            break;
    }
}

void BMXIO::Write(BMXIO *bmx_io)
{
    ByteArray buffer;
    buffer.Allocate(8192);

    while (true) {
        uint32_t num_read = bmx_io->Read(buffer.GetBytesAvailable(), buffer.GetAllocatedSize());

        if (num_read > 0) {
            uint32_t num_write = Write(buffer.GetBytesAvailable(), num_read);
            if (num_write != num_read)
                BMX_EXCEPTION(("Failed to write to output file"));
        }

        if (num_read < buffer.GetAllocatedSize())
            break;
    }
}

bool BMXIO::Seek(int64_t offset, int whence)
{
    (void)offset;
    (void)whence;
    BMX_ASSERT(false);
    return 0;
}

int64_t BMXIO::Tell()
{
    BMX_ASSERT(false);
    return 0;
}

int64_t BMXIO::Size()
{
    BMX_ASSERT(false);
    return 0;
}
