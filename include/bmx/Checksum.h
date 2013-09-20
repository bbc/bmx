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

#ifndef BMX_CHECKSUM_H_
#define BMX_CHECKSUM_H_

#include <vector>

#include <bmx/CRC32.h>
#include <bmx/MD5.h>
#include <bmx/SHA1.h>



namespace bmx
{


typedef enum
{
    CRC32_CHECKSUM,
    MD5_CHECKSUM,
    SHA1_CHECKSUM,
} ChecksumType;


class Checksum
{
public:
    static std::string CalcFileChecksum(const std::string &filename, ChecksumType type);
    static std::string CalcFileChecksum(FILE *file, ChecksumType type);

    static std::vector<std::string> CalcFileChecksums(const std::string &filename,
                                                      const std::vector<ChecksumType> &types);
    static std::vector<std::string> CalcFileChecksums(FILE *file, const std::vector<ChecksumType> &types);

public:
    Checksum();
    Checksum(ChecksumType type);
    ~Checksum();

    void Init(ChecksumType type);

    void Update(const unsigned char *data, uint32_t size);
    void Final();

    ChecksumType GetType() const { return mType; }

    size_t GetDigestSize() const;
    void GetDigest(unsigned char *digest, size_t size) const;
    std::string GetDigestString() const;

private:
    ChecksumType mType;
    uint32_t mCRC32Context;
    MD5Context mMD5Context;
    SHA1Context mSHA1Context;
    unsigned char mDigest[20];
};


};



#endif

