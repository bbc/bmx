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

#include <climits>

#include <bmx/apps/AppMXFFileFactory.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



AppMXFFileFactory::AppMXFFileFactory()
{
    mChecksumInput = false;
    mInputChecksumType = MD5_CHECKSUM;
    mInputFlags = 0;
    mRWInterleaver = 0;
}

AppMXFFileFactory::~AppMXFFileFactory()
{
    mxf_free_rw_intl(&mRWInterleaver);
}

void AppMXFFileFactory::SetInputChecksum(ChecksumType type)
{
    mChecksumInput     = true;
    mInputChecksumType = type;
}

void AppMXFFileFactory::SetInputFlags(int flags)
{
    mInputFlags = flags;
}

void AppMXFFileFactory::SetRWInterleave(uint32_t rw_interleave_size)
{
    if (mRWInterleaver)
        mxf_free_rw_intl(&mRWInterleaver);

    uint32_t cache_size = 2 * 1024 * 1024;
    if (cache_size < rw_interleave_size) {
        BMX_CHECK(rw_interleave_size < UINT32_MAX / 2);
        cache_size = 2 * rw_interleave_size;
    }
    BMX_CHECK(mxf_create_rw_intl(rw_interleave_size, cache_size, &mRWInterleaver));
}

File* AppMXFFileFactory::OpenNew(string filename)
{
    MXFFile *mxf_file = 0;

    try
    {
#if defined(_WIN32)
        BMX_CHECK(mxf_win32_file_open_new(filename.c_str(), 0, &mxf_file));
#else
        BMX_CHECK(mxf_disk_file_open_new(filename.c_str(), &mxf_file));
#endif

        if (mRWInterleaver) {
            MXFFile *intl_mxf_file;
            BMX_CHECK(mxf_rw_intl_open(mRWInterleaver, mxf_file, 1, &intl_mxf_file));
            mxf_file = intl_mxf_file;
        }

        return new File(mxf_file);
    }
    catch (...)
    {
        mxf_file_close(&mxf_file);
        throw;
    }
}

File* AppMXFFileFactory::OpenRead(string filename)
{
    MXFFile *mxf_file = 0;

    try
    {
        if (filename.empty()) {
            BMX_CHECK(mxf_stdin_wrap_read(&mxf_file));
        } else {
#if defined(_WIN32)
            BMX_CHECK(mxf_win32_file_open_read(filename.c_str(), mInputFlags, &mxf_file));
#else
            BMX_CHECK(mxf_disk_file_open_read(filename.c_str(), &mxf_file));
#endif
        }

        if (mChecksumInput) {
            MXFChecksumFile *checksum_file = mxf_checksum_file_open(mxf_file, mInputChecksumType);
            mInputChecksumFiles.push_back(make_pair(filename, checksum_file));
            mxf_file = mxf_checksum_file_get_file(checksum_file);
        }

        if (mRWInterleaver) {
            MXFFile *intl_mxf_file;
            BMX_CHECK(mxf_rw_intl_open(mRWInterleaver, mxf_file, 0, &intl_mxf_file));
            mxf_file = intl_mxf_file;
        }

        return new File(mxf_file);
    }
    catch (...)
    {
        mxf_file_close(&mxf_file);
        throw;
    }
}

File* AppMXFFileFactory::OpenModify(string filename)
{
    MXFFile *mxf_file = 0;

    try
    {
#if defined(_WIN32)
        BMX_CHECK(mxf_win32_file_open_modify(filename.c_str(), 0, &mxf_file));
#else
        BMX_CHECK(mxf_disk_file_open_modify(filename.c_str(), &mxf_file));
#endif

        if (mRWInterleaver) {
            MXFFile *intl_mxf_file;
            BMX_CHECK(mxf_rw_intl_open(mRWInterleaver, mxf_file, 1, &intl_mxf_file));
            mxf_file = intl_mxf_file;
        }

        return new File(mxf_file);
    }
    catch (...)
    {
        mxf_file_close(&mxf_file);
        throw;
    }
}

void AppMXFFileFactory::ForceInputChecksumUpdate()
{
    size_t i;
    for (i = 0; i < mInputChecksumFiles.size(); i++)
        mxf_checksum_file_force_update(mInputChecksumFiles[i].second);
}

void AppMXFFileFactory::FinalizeInputChecksum()
{
    size_t i;
    for (i = 0; i < mInputChecksumFiles.size(); i++)
        BMX_CHECK(mxf_checksum_file_final(mInputChecksumFiles[i].second));
}

string AppMXFFileFactory::GetInputChecksumFilename(size_t index) const
{
    BMX_ASSERT(index < mInputChecksumFiles.size());
    return mInputChecksumFiles[index].first;
}

size_t AppMXFFileFactory::GetInputChecksumDigestSize(size_t index) const
{
    BMX_ASSERT(index < mInputChecksumFiles.size());
    return mxf_checksum_file_digest_size(mInputChecksumFiles[index].second);
}

void AppMXFFileFactory::GetInputChecksumDigest(size_t index, unsigned char *digest, size_t size) const
{
    BMX_ASSERT(index < mInputChecksumFiles.size());
    return mxf_checksum_file_digest(mInputChecksumFiles[index].second, digest, size);
}

string AppMXFFileFactory::GetInputChecksumDigestString(size_t index) const
{
    BMX_ASSERT(index < mInputChecksumFiles.size());
    return mxf_checksum_file_digest_str(mInputChecksumFiles[index].second);
}

