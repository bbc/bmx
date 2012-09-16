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

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cerrno>

#include <bmx/wave/WaveFileIO.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



WaveFileIO* WaveFileIO::OpenRead(string filename)
{
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file)
        BMX_EXCEPTION(("Failed to open wave file '%s' for reading: %s", filename.c_str(), bmx_strerror(errno).c_str()));

    return new WaveFileIO(file, true);
}

WaveFileIO* WaveFileIO::OpenNew(string filename)
{
    FILE *file = fopen(filename.c_str(), "wb");
    if (!file)
        BMX_EXCEPTION(("Failed to open wave file '%s' for writing: %s", filename.c_str(), bmx_strerror(errno).c_str()));

    return new WaveFileIO(file, false);
}

WaveFileIO::WaveFileIO(FILE *file, bool read_only)
: WaveIO()
{
    mFile = file;
    mReadOnly = read_only;
}

WaveFileIO::~WaveFileIO()
{
    if (mFile)
        fclose(mFile);
}

uint32_t WaveFileIO::Read(unsigned char *data, uint32_t size)
{
    size_t result = fread(data, 1, size, mFile);
    if (result != size && ferror(mFile))
        log_error("Failed to read %u bytes: %s\n", size, bmx_strerror(errno).c_str());

    return (uint32_t)result;
}

int WaveFileIO::GetChar()
{
    return fgetc(mFile);
}

uint32_t WaveFileIO::Write(const unsigned char *data, uint32_t size)
{
    BMX_ASSERT(!mReadOnly);

    size_t result = fwrite(data, 1, size, mFile);
    if (result != size)
        log_error("Failed to write %u bytes: %s\n", size, bmx_strerror(errno).c_str());

    return (uint32_t)result;
}

int WaveFileIO::PutChar(int c)
{
    BMX_ASSERT(!mReadOnly);

    return fputc(c, mFile);
}

bool WaveFileIO::Seek(int64_t offset, int whence)
{
#if defined(_WIN32)
    return _fseeki64(mFile, offset, whence) == 0;
#else
    return fseeko(mFile, offset, whence) == 0;
#endif
}

int64_t WaveFileIO::Tell()
{
#if defined(_WIN32)
    return _ftelli64(mFile);
#else
    return ftello(mFile);
#endif
}

int64_t WaveFileIO::Size()
{
    // flush user-space data because fstat uses the stream's integer descriptor
    if (!mReadOnly)
        fflush(mFile);

#if defined(_WIN32)
    struct _stati64 stat_buf;
    if (_fstati64(_fileno(mFile), &stat_buf) != 0)
#else
    struct stat stat_buf;
    if (fstat(fileno(mFile), &stat_buf) != 0)
#endif
    {
        return -1;
    }

    return stat_buf.st_size;
}

