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

#include <cerrno>
#include <algorithm>
#if !defined(_WIN32)
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <bmx/essence_parser/FileEssenceSource.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace bmx;
using namespace std;


static const int MAX_FIFO_SIZE = 50000000; // 50MB


FileEssenceSource::FileEssenceSource()
{
    mFile = 0;
    mStartOffset = 0;
    mErrno = 0;
    mIsFifo = false;
    mIsFifoSeek = false;
}

FileEssenceSource::~FileEssenceSource()
{
    if (mFile)
        fclose(mFile);
}

bool FileEssenceSource::Open(const string &filename, int64_t start_offset)
{
    mStartOffset = start_offset;
    mErrno = 0;

    if (mFile) {
        fclose(mFile);
        mFile = 0;
    }

    mFile = fopen(filename.c_str(), "rb");
    if (!mFile) {
        mErrno = errno;
        return false;
    }

    if (mStartOffset != 0 && !SeekStart()) {
        fclose(mFile);
        mFile = 0;
        return false;
    }

#if !defined(_WIN32)
    // Keep this after the SeekStart above to disable buffering at that stage
    // Support for named pipes is not currently supported on Windows OS
    struct stat buf;
    memset(&buf, 0, sizeof(buf));
    int ret = stat(filename.c_str(), &buf);
    if (ret == 0 && S_ISFIFO(buf.st_mode)) {
        mIsFifo = true;
    }
#endif

    return true;
}

uint32_t FileEssenceSource::Read(unsigned char *data, uint32_t size)
{
    BMX_ASSERT(mFile);
    mErrno = 0;

    if (mIsFifoSeek && mFifoBuffer.size() > 0) {
       size_t len = min((size_t)size, mFifoBuffer.size());
       memcpy(data, &mFifoBuffer[0], len);
       mFifoBuffer = std::vector<unsigned char>(mFifoBuffer.begin() + len, mFifoBuffer.end());

       if (mFifoBuffer.size() > 0) {
         return (uint32_t)len;
       }

       size -= (uint32_t)len;
       data += len;
       size_t num_read = fread(data, 1, size, mFile);
       if (num_read < size && ferror(mFile))
          mErrno = errno;

       return (uint32_t)(num_read + len);
    }

    size_t num_read = fread(data, 1, size, mFile);
    if (num_read < size && ferror(mFile)) {
        mErrno = errno;
    } else {
        if (mIsFifo && !mIsFifoSeek && num_read > 0 && mFifoBuffer.size() < MAX_FIFO_SIZE) {
            mFifoBuffer.insert(mFifoBuffer.end(), data, data + num_read);
        }
    }

    return (uint32_t)num_read;
}

bool FileEssenceSource::SeekStart()
{
    BMX_ASSERT(mFile);
    mErrno = 0;

    if (mIsFifo) {
       mIsFifoSeek = true;
       return true;
    }

    int res;
#if defined(_WIN32)
    res = _fseeki64(mFile, mStartOffset, SEEK_SET);
#else
    res = fseeko(mFile, mStartOffset, SEEK_SET);
#endif
    if (res != 0)
        mErrno = errno;

    return mErrno == 0;
}

bool FileEssenceSource::Skip(int64_t offset)
{
    BMX_ASSERT(mFile);
    mErrno = 0;

    while (mIsFifo &&
           !mIsFifoSeek &&
           mErrno == 0 &&
           offset > 0 &&
           mFifoBuffer.size() < MAX_FIFO_SIZE)
    {
        unsigned char buffer[8192];
        uint32_t next_read = sizeof(buffer);
        if (next_read > offset) {
            next_read = (uint32_t)offset;
        }
        uint32_t num_read = Read(buffer, next_read);
        offset -= num_read;
        if (num_read < next_read) {
            break;
        }
    }

    int res;
#if defined(_WIN32)
    res = _fseeki64(mFile, offset, SEEK_CUR);
#else
    res = fseeko(mFile, offset, SEEK_CUR);
#endif
    if (res != 0)
        mErrno = errno;

    return mErrno == 0;
}

string FileEssenceSource::GetStrError() const
{
    return bmx_strerror(mErrno);
}

