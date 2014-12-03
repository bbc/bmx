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
#include <bmx/MXFHTTPFile.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



AppMXFFileFactory::AppMXFFileFactory()
{
    mInputFlags = 0;
    mRWInterleaver = 0;
    mHTTPMinReadSize = 64 * 1024;
}

AppMXFFileFactory::~AppMXFFileFactory()
{
    mxf_free_rw_intl(&mRWInterleaver);
}

void AppMXFFileFactory::SetInputChecksumTypes(const set<ChecksumType> &types)
{
    mInputChecksumTypes.clear();
    mInputChecksumTypes.insert(types.begin(), types.end());
}

void AppMXFFileFactory::AddInputChecksumType(ChecksumType type)
{
    mInputChecksumTypes.insert(type);
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

void AppMXFFileFactory::SetHTTPMinReadSize(uint32_t size)
{
    mHTTPMinReadSize = size;
}

File* AppMXFFileFactory::OpenNew(string filename)
{
    MXFFile *mxf_file = 0;

    if (mxf_http_is_url(filename))
        BMX_EXCEPTION(("HTTP file access is not supported for writing new files"));

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
        string uri_str;
        if (filename.empty()) {
            BMX_CHECK(mxf_stdin_wrap_read(&mxf_file));
            uri_str = "stdin:";
        } else {
            if (mxf_http_is_url(filename)) {
                mxf_file = mxf_http_file_open_read(filename, mHTTPMinReadSize);
                uri_str = filename;
            } else {
#if defined(_WIN32)
                BMX_CHECK(mxf_win32_file_open_read(filename.c_str(), mInputFlags, &mxf_file));
#else
                BMX_CHECK(mxf_disk_file_open_read(filename.c_str(), &mxf_file));
#endif
            }
        }

        if (!mInputChecksumTypes.empty()) {
            URI abs_uri;
            if (!uri_str.empty()) {
              abs_uri = URI(uri_str);
            } else {
                BMX_CHECK(abs_uri.ParseFilename(filename));
                if (abs_uri.IsRelative()) {
                    URI base_uri;
                    BMX_CHECK(base_uri.ParseDirectory(get_cwd()));
                    abs_uri.MakeAbsolute(base_uri);
                }
            }

            InputChecksumFile input_checksum_file;
            input_checksum_file.filename = filename;
            input_checksum_file.abs_uri = abs_uri;

            set<ChecksumType>::const_iterator types_iter;
            for (types_iter = mInputChecksumTypes.begin(); types_iter != mInputChecksumTypes.end(); types_iter++) {
                MXFChecksumFile *checksum_file = mxf_checksum_file_open(mxf_file, *types_iter);
                input_checksum_file.checksum_files.push_back(make_pair(*types_iter, checksum_file));
                mxf_file = mxf_checksum_file_get_file(checksum_file);
            }

            mInputChecksumFiles.push_back(input_checksum_file);
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

    if (mxf_http_is_url(filename))
        BMX_EXCEPTION(("HTTP file access is not supported for updating files"));

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
        mxf_checksum_file_force_update(mInputChecksumFiles[i].checksum_files.back().second);
}

void AppMXFFileFactory::FinalizeInputChecksum()
{
    size_t i;
    for (i = 0; i < mInputChecksumFiles.size(); i++) {
        size_t j;
        for (j = 0; j < mInputChecksumFiles[i].checksum_files.size(); j++)
            BMX_CHECK(mxf_checksum_file_final(mInputChecksumFiles[i].checksum_files[j].second));
    }
}

string AppMXFFileFactory::GetInputChecksumFilename(size_t file_index) const
{
    BMX_ASSERT(file_index < mInputChecksumFiles.size());
    return mInputChecksumFiles[file_index].filename;
}

URI AppMXFFileFactory::GetInputChecksumAbsURI(size_t file_index) const
{
    BMX_ASSERT(file_index < mInputChecksumFiles.size());
    return mInputChecksumFiles[file_index].abs_uri;
}

vector<ChecksumType> AppMXFFileFactory::GetInputChecksumTypes(size_t file_index) const
{
    BMX_ASSERT(file_index < mInputChecksumFiles.size());
    vector<ChecksumType> types;
    size_t i;
    for (i = 0; i < mInputChecksumFiles[file_index].checksum_files.size(); i++)
        types.push_back(mInputChecksumFiles[file_index].checksum_files[i].first);
    return types;
}

size_t AppMXFFileFactory::GetInputChecksumDigestSize(size_t file_index, ChecksumType type) const
{
    return mxf_checksum_file_digest_size(GetChecksumFile(file_index, type));
}

void AppMXFFileFactory::GetInputChecksumDigest(size_t file_index, ChecksumType type, unsigned char *digest,
                                               size_t size) const
{
    return mxf_checksum_file_digest(GetChecksumFile(file_index, type), digest, size);
}

string AppMXFFileFactory::GetInputChecksumDigestString(size_t file_index, ChecksumType type) const
{
    return mxf_checksum_file_digest_str(GetChecksumFile(file_index, type));
}

MXFChecksumFile* AppMXFFileFactory::GetChecksumFile(size_t file_index, ChecksumType type) const
{
    BMX_ASSERT(file_index < mInputChecksumFiles.size());

    MXFChecksumFile *checksum_file = 0;
    vector<ChecksumType> types;
    size_t i;
    for (i = 0; i < mInputChecksumFiles[file_index].checksum_files.size(); i++) {
        if (mInputChecksumFiles[file_index].checksum_files[i].first == type) {
            checksum_file = mInputChecksumFiles[file_index].checksum_files[i].second;
            break;
        }
    }
    BMX_ASSERT(checksum_file);

    return checksum_file;
}

