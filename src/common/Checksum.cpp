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

#include <cstring>
#include <cerrno>

#include <bmx/Checksum.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



string Checksum::CalcFileChecksum(const string &filename, ChecksumType type)
{
    vector<ChecksumType> types;
    types.push_back(type);

    vector<string> result = CalcFileChecksums(filename, types);
    if (result.empty())
        return "";

    return result.front();
}

string Checksum::CalcFileChecksum(FILE *file, ChecksumType type)
{
    vector<ChecksumType> types;
    types.push_back(type);

    vector<string> result = CalcFileChecksums(file, types);
    if (result.empty())
        return "";

    return result.front();
}

vector<string> Checksum::CalcFileChecksums(const string &filename, const vector<ChecksumType> &types)
{
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
        log_warn("Failed to open file '%s' to calculate checksum: %s\n", filename.c_str(), bmx_strerror(errno).c_str());
        return vector<string>();
    }

    vector<string> result = CalcFileChecksums(file, types);
    if (result.empty())
        log_warn("Failed to calculate checksum for file '%s'\n", filename.c_str());

    fclose(file);

    return result;
}

vector<string> Checksum::CalcFileChecksums(FILE *file, const vector<ChecksumType> &types)
{
    vector<Checksum> checksums;
    size_t i;
    for (i = 0; i < types.size(); i++)
        checksums.push_back(Checksum(types[i]));

    const size_t buffer_size = 8192;
    unsigned char *buffer = new unsigned char[buffer_size];
    size_t num_read = buffer_size;
    while (num_read == buffer_size) {
        num_read = fread(buffer, 1, buffer_size, file);
        if (num_read != buffer_size && ferror(file)) {
            log_warn("Read failure when calculating checksum: %s\n", bmx_strerror(errno).c_str());
            delete [] buffer;
            return vector<string>();
        }

        if (num_read > 0) {
            for (i = 0; i < checksums.size(); i++)
                checksums[i].Update(buffer, (uint32_t)num_read);
        }
    }
    delete [] buffer;

    vector<string> result;
    for (i = 0; i < checksums.size(); i++) {
        checksums[i].Final();
        result.push_back(checksums[i].GetDigestString());
    }

    return result;
}


Checksum::Checksum()
{
    Init(MD5_CHECKSUM);
}

Checksum::Checksum(ChecksumType type)
{
    Init(type);
}

Checksum::~Checksum()
{
}

void Checksum::Init(ChecksumType type)
{
    mType = type;
    memset(&mCRC32Context, 0, sizeof(mCRC32Context));
    memset(&mMD5Context, 0, sizeof(mMD5Context));
    memset(&mSHA1Context, 0, sizeof(mSHA1Context));
    memset(mDigest, 0, sizeof(mDigest));

    switch (type)
    {
        case CRC32_CHECKSUM: crc32_init(&mCRC32Context); break;
        case MD5_CHECKSUM:   md5_init(&mMD5Context); break;
        case SHA1_CHECKSUM:  sha1_init(&mSHA1Context); break;
    }
}

void Checksum::Update(const unsigned char *data, uint32_t size)
{
    switch (mType)
    {
        case CRC32_CHECKSUM: crc32_update(&mCRC32Context, data, size); break;
        case MD5_CHECKSUM:   md5_update(&mMD5Context, data, size); break;
        case SHA1_CHECKSUM:  sha1_update(&mSHA1Context, data, size); break;
    }
}

void Checksum::Final()
{
    switch (mType)
    {
        case CRC32_CHECKSUM: crc32_final(&mCRC32Context); break;
        case MD5_CHECKSUM:   md5_final(mDigest, &mMD5Context); break;
        case SHA1_CHECKSUM:  sha1_final(mDigest, &mSHA1Context); break;
    }
}

size_t Checksum::GetDigestSize() const
{
    switch (mType)
    {
        case CRC32_CHECKSUM: return 4;
        case MD5_CHECKSUM:   return 16;
        case SHA1_CHECKSUM:  return 20;
        default:             return 0;
    }
}

void Checksum::GetDigest(unsigned char *digest, size_t size) const
{
    switch (mType)
    {
        case CRC32_CHECKSUM:
            BMX_CHECK(size >= 4);
            digest[0] = (unsigned char)((mCRC32Context >> 24) & 0xff);
            digest[1] = (unsigned char)((mCRC32Context >> 16) & 0xff);
            digest[2] = (unsigned char)((mCRC32Context >> 8)  & 0xff);
            digest[3] = (unsigned char)( mCRC32Context        & 0xff);
            break;
        case MD5_CHECKSUM:
            BMX_CHECK(size >= 16);
            memcpy(digest, mDigest, 16);
            break;
        case SHA1_CHECKSUM:
            BMX_CHECK(size >= 20);
            memcpy(digest, mDigest, 20);
            break;
    }
}

string Checksum::GetDigestString() const
{
    switch (mType)
    {
        case CRC32_CHECKSUM: return crc32_digest_str(mCRC32Context);
        case MD5_CHECKSUM:   return md5_digest_str(mDigest);
        case SHA1_CHECKSUM:  return sha1_digest_str(mDigest);
        default:             return "";
    }
}

