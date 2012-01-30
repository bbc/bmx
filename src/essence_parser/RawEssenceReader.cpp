/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#define __STDC_FORMAT_MACROS

#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <bmx/essence_parser/RawEssenceReader.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define READ_BLOCK_SIZE         8192
#define PARSE_FRAME_START_SIZE  8192



RawEssenceReader::RawEssenceReader(FILE *raw_input)
{
    mRawInput = raw_input;
    mStartOffset = 0;
    mMaxReadLength = 0;
    mTotalReadLength = 0;
    mMaxSampleSize = 0;
    mFixedSampleSize = 0;
    mEssenceParser = 0;
    mSampleDataSize = 0;
    mNumSamples = 0;
    mReadFirstSample = false;
    mLastSampleRead = false;
    mSampleDataOffset = 0;

    mSampleData.SetAllocBlockSize(READ_BLOCK_SIZE);
}

RawEssenceReader::~RawEssenceReader()
{
    if (mRawInput)
        fclose(mRawInput);

    delete mEssenceParser;
}

void RawEssenceReader::SeekToOffset(int64_t offset)
{
    mStartOffset = offset;
    Reset();
}

void RawEssenceReader::SetMaxReadLength(int64_t len)
{
    mMaxReadLength = len;
}

void RawEssenceReader::SetFixedSampleSize(uint32_t size)
{
    mFixedSampleSize = size;
}

void RawEssenceReader::SetEssenceParser(EssenceParser *essence_parser)
{
    mEssenceParser = essence_parser;
}

void RawEssenceReader::SetCheckMaxSampleSize(uint32_t size)
{
    mMaxSampleSize = size;
}

uint32_t RawEssenceReader::ReadSamples(uint32_t num_samples)
{
    if (mLastSampleRead)
        return 0;

    // shift data from previous read to start of sample data
    // note that this is needed even if mFixedSampleSize > 0 because the previous read could have occurred
    // when mFixedSampleSize == 0
    ShiftSampleData(0, mSampleDataSize, mSampleData.GetSize() - mSampleDataSize);
    mSampleDataSize = 0;
    mNumSamples = 0;


    if (mFixedSampleSize == 0) {
        uint32_t i;
        for (i = 0; i < num_samples; i++) {
            if (!ReadAndParseSample())
                break;
        }
    } else {
        uint32_t num_read;
        if (!ReadBytes(mFixedSampleSize * num_samples - mSampleData.GetSize(), &num_read)) {
            mLastSampleRead = true;
            return 0;
        }

        if (mSampleData.GetSize() < mFixedSampleSize * num_samples)
            mLastSampleRead = true;

        mNumSamples = mSampleData.GetSize() / mFixedSampleSize;
        mSampleData.SetSize(mNumSamples * mFixedSampleSize);
        mSampleDataSize = mNumSamples * mFixedSampleSize;
    }

    return mNumSamples;
}

uint32_t RawEssenceReader::GetSampleSize() const
{
    BMX_CHECK(mNumSamples > 0 && (mFixedSampleSize > 0 || mNumSamples == 1));
    return mSampleDataSize / mNumSamples;
}

int64_t RawEssenceReader::GetNumRemFixedSizeSamples() const
{
    BMX_CHECK(mFixedSampleSize > 0);

    struct stat stat_buf;
#if defined(_WIN32)
    if (fstat(_fileno(mRawInput), &stat_buf) < 0)
#else
    if (fstat(fileno(mRawInput), &stat_buf) < 0)
#endif
        throw BMXException("Failed to stat raw file", strerror(errno));

    int64_t start_offset = mSampleDataOffset + mSampleDataSize;
    int64_t end_offset = stat_buf.st_size;
    if (mMaxReadLength > 0 && end_offset > mStartOffset + mMaxReadLength)
        end_offset = mStartOffset + mMaxReadLength;

    int64_t num_samples = (end_offset - start_offset) / mFixedSampleSize;

    return (num_samples < 0 ? 0 : num_samples);
}

void RawEssenceReader::Reset()
{
#if defined(_MSC_VER)
    if (_fseeki64(mRawInput, mStartOffset, SEEK_SET) != 0)
#else
    if (fseeko(mRawInput, mStartOffset, SEEK_SET) != 0)
#endif
        throw BMXException("Failed to seek to raw file start offset 0x%"PRIx64": %s", mStartOffset, strerror(errno));

    mSampleDataOffset = mStartOffset;
    mTotalReadLength = 0;
    mSampleData.SetSize(0);
    mSampleDataSize = 0;
    mNumSamples = 0;
    mReadFirstSample = false;
    mLastSampleRead = false;
}

bool RawEssenceReader::ReadAndParseSample()
{
    BMX_CHECK(mEssenceParser);

    uint32_t sample_start_offset = mSampleDataSize;
    uint32_t sample_num_read = mSampleData.GetSize() - sample_start_offset;
    uint32_t num_read;

    if (!mReadFirstSample) {
        // find the start of the first sample

        if (!ReadBytes(PARSE_FRAME_START_SIZE, &num_read)) {
            mLastSampleRead = true;
            return false;
        }
        sample_num_read += num_read;

        uint32_t offset = mEssenceParser->ParseFrameStart(mSampleData.GetBytes() + sample_start_offset, sample_num_read);
        if (offset == ESSENCE_PARSER_NULL_OFFSET) {
            log_warn("Failed to find start of raw essence sample\n");
            mLastSampleRead = true;
            return false;
        }

        // shift start of first sample to offset 0
        if (offset > 0) {
            ShiftSampleData(sample_start_offset, sample_start_offset + offset, sample_num_read - offset);
            sample_num_read -= offset;
        }

        mReadFirstSample = true;
    } else {
        if (!ReadBytes(READ_BLOCK_SIZE, &num_read)) {
            mLastSampleRead = true;
            return false;
        }
        sample_num_read += num_read;
    }

    uint32_t sample_size = 0;
    while (true) {
        sample_size = mEssenceParser->ParseFrameSize(mSampleData.GetBytes() + sample_start_offset, sample_num_read);
        if (sample_size != ESSENCE_PARSER_NULL_OFFSET)
            break;

        BMX_CHECK_M(mMaxSampleSize == 0 || mSampleData.GetSize() - sample_start_offset <= mMaxSampleSize,
                   ("Max raw sample size (%u) exceeded", mMaxSampleSize));

        if (!ReadBytes(READ_BLOCK_SIZE, &num_read)) {
            mLastSampleRead = true;
            return false;
        }
        sample_num_read += num_read;

        if (num_read == 0)
            break;
    }

    if (sample_size == 0) {
        // invalid or null sample data
        mLastSampleRead = true;
        return false;
    } else if (sample_size == ESSENCE_PARSER_NULL_OFFSET) {
        // assume remaining data is valid sample data
        mLastSampleRead = true;
        if (sample_num_read > 0) {
            mSampleDataSize = mSampleData.GetSize();
            mNumSamples++;
        }
        return false;
    }

    mSampleDataSize += sample_size;
    mNumSamples++;
    return true;
}

bool RawEssenceReader::ReadBytes(uint32_t size, uint32_t *num_read_out)
{
    BMX_ASSERT(mMaxReadLength == 0 || mTotalReadLength <= mMaxReadLength);

    uint32_t actual_size = size;
    if (mMaxReadLength > 0 && mTotalReadLength + size > mMaxReadLength)
        actual_size = mMaxReadLength - mTotalReadLength;
    if (actual_size == 0) {
        *num_read_out = 0;
        return true;
    }

    mSampleData.Grow(actual_size);
    uint32_t num_read = fread(mSampleData.GetBytesAvailable(), 1, actual_size, mRawInput);
    if (ferror(mRawInput)) {
        log_warn("Failed to read from raw file: %s\n", strerror(errno));
        return false;
    }
    mTotalReadLength += num_read;

    mSampleData.IncrementSize(num_read);

    *num_read_out = num_read;
    return true;
}

void RawEssenceReader::ShiftSampleData(uint32_t to_offset, uint32_t from_offset, uint32_t size)
{
    BMX_ASSERT(mSampleData.GetSize() >= (from_offset - to_offset));
    BMX_ASSERT(from_offset + size <= mSampleData.GetSize());

    if (mSampleData.GetSize() > (from_offset - to_offset)) {
        memmove(mSampleData.GetBytes() + to_offset, mSampleData.GetBytes() + from_offset, size);
        mSampleData.SetSize(mSampleData.GetSize() - (from_offset - to_offset));
    } else {
        mSampleData.SetSize(0);
    }

    if (to_offset == 0)
        mSampleDataOffset += from_offset;
}

