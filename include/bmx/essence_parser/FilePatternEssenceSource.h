/*
 * Copyright (C) 2021, British Broadcasting Corporation
 * All Rights Reserved.
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

#ifndef BMX_FILE_PATTERN_ESSENCE_SOURCE_H_
#define BMX_FILE_PATTERN_ESSENCE_SOURCE_H_

#include <map>

#include <bmx/essence_parser/EssenceSource.h>
#include <bmx/ByteArray.h>



namespace bmx
{

class FilePatternEssenceSource : public EssenceSource
{
public:
    FilePatternEssenceSource(bool fill_gaps);
    virtual ~FilePatternEssenceSource();

    bool Open(const std::string &pattern, int64_t start_offset);

public:
    virtual uint32_t Read(unsigned char *data, uint32_t size);
    virtual bool SeekStart();
    virtual bool Skip(int64_t offset);

    virtual bool HaveError() const { return mErrno != 0; }
    virtual int GetErrno() const   { return mErrno; }
    virtual std::string GetStrError() const;

private:
    int64_t ReadOrSkip(unsigned char *data, uint32_t size, int64_t skip_offset);
    bool NextFile();
    bool BufferFile();

    std::string GetCurrentFilePath() const { return mDirname + "/" + mCurrentFilename; }

private:
    bool mFillGaps;
    int64_t mStartOffset;
    int64_t mCurrentOffset;
    int mErrno;
    std::string mDirname;
    std::map<int, std::string> mFilenames;
    std::map<int, std::string>::const_iterator mNextFilenamesIter;
    int64_t mCurrentNumber;
    std::string mCurrentFilename;
    bmx::ByteArray mFileBuffer;
    uint32_t mFileBufferOffset;
};


};


#endif
