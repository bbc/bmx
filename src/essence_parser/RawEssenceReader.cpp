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

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <bmx/essence_parser/RawEssenceReader.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define READ_BLOCK_SIZE         8192
#define PARSE_FRAME_START_SIZE  8192



RawEssenceReader::RawEssenceReader(EssenceSource *essence_source)
{
    mEssenceSource = essence_source;
    mMaxReadLength = 0;
    mTotalReadLength = 0;
    mMaxSampleSize = 0;
    mFixedSampleSize = 0;
    mEssenceParser = 0;
    mSampleDataSize = 0;
    mNumSamples = 0;
    mReadFirstSample = false;
    mLastSampleRead = false;

    mSampleBuffer.SetAllocBlockSize(READ_BLOCK_SIZE);
}

RawEssenceReader::~RawEssenceReader()
{
    delete mEssenceSource;
    delete mEssenceParser;
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
    ShiftSampleData(0, mSampleDataSize);
    mSampleDataSize = 0;
    mNumSamples = 0;


    if (mFixedSampleSize == 0) {
        uint32_t i;
        for (i = 0; i < num_samples; i++) {
            if (!ReadAndParseSample())
                break;
        }
    } else {
        ReadBytes(mFixedSampleSize * num_samples - mSampleBuffer.GetSize());
        if (mSampleBuffer.GetSize() < mFixedSampleSize * num_samples)
            mLastSampleRead = true;

        mNumSamples = mSampleBuffer.GetSize() / mFixedSampleSize;
        mSampleBuffer.SetSize(mNumSamples * mFixedSampleSize);
        mSampleDataSize = mNumSamples * mFixedSampleSize;
    }

    return mNumSamples;
}

uint32_t RawEssenceReader::GetSampleSize() const
{
    BMX_CHECK(mNumSamples > 0 && (mFixedSampleSize > 0 || mNumSamples == 1));
    return mSampleDataSize / mNumSamples;
}

void RawEssenceReader::Reset()
{
    if (!mEssenceSource->SeekStart())
        throw BMXException("Failed to seek to essence start: %s", mEssenceSource->GetStrError().c_str());

    mTotalReadLength = 0;
    mSampleBuffer.SetSize(0);
    mSampleDataSize = 0;
    mNumSamples = 0;
    mReadFirstSample = false;
    mLastSampleRead = false;
}

bool RawEssenceReader::ReadAndParseSample()
{
    BMX_CHECK(mEssenceParser);

    uint32_t sample_start_offset = mSampleDataSize;
    uint32_t sample_num_read = mSampleBuffer.GetSize() - sample_start_offset;
    uint32_t num_read;

    if (!mReadFirstSample) {
        // find the start of the first sample

        sample_num_read += ReadBytes(PARSE_FRAME_START_SIZE);
        uint32_t offset = mEssenceParser->ParseFrameStart(mSampleBuffer.GetBytes() + sample_start_offset, sample_num_read);
        if (offset == ESSENCE_PARSER_NULL_OFFSET) {
            log_warn("Failed to find start of raw essence sample\n");
            mLastSampleRead = true;
            return false;
        }

        // shift start of first sample to offset 0
        if (offset > 0) {
            ShiftSampleData(sample_start_offset, sample_start_offset + offset);
            sample_num_read -= offset;
        }

        mReadFirstSample = true;
    } else {
        sample_num_read += ReadBytes(READ_BLOCK_SIZE);
    }

    uint32_t sample_size = 0;
    while (true) {
        sample_size = mEssenceParser->ParseFrameSize(mSampleBuffer.GetBytes() + sample_start_offset, sample_num_read);
        if (sample_size != ESSENCE_PARSER_NULL_OFFSET)
            break;

        BMX_CHECK_M(mMaxSampleSize == 0 || mSampleBuffer.GetSize() - sample_start_offset <= mMaxSampleSize,
                   ("Max raw sample size (%u) exceeded", mMaxSampleSize));

        num_read = ReadBytes(READ_BLOCK_SIZE);
        if (num_read == 0)
            break;

        sample_num_read += num_read;
    }

    if (sample_size == ESSENCE_PARSER_NULL_FRAME_SIZE) {
        // invalid or null sample data
        mLastSampleRead = true;
        return false;
    } else if (sample_size == ESSENCE_PARSER_NULL_OFFSET) {
        // assume remaining data is valid sample data
        mLastSampleRead = true;
        if (sample_num_read > 0) {
            mSampleDataSize = mSampleBuffer.GetSize();
            mNumSamples++;
        }
        return false;
    }

    mSampleDataSize += sample_size;
    mNumSamples++;
    return true;
}

uint32_t RawEssenceReader::ReadBytes(uint32_t size)
{
    BMX_ASSERT(mMaxReadLength == 0 || mTotalReadLength <= mMaxReadLength);

    uint32_t actual_size = size;
    if (mMaxReadLength > 0 && mTotalReadLength + size > mMaxReadLength)
        actual_size = (uint32_t)(mMaxReadLength - mTotalReadLength);
    if (actual_size == 0)
        return 0;

    mSampleBuffer.Grow(actual_size);
    uint32_t num_read = mEssenceSource->Read(mSampleBuffer.GetBytesAvailable(), actual_size);
    if (num_read < actual_size && mEssenceSource->HaveError())
        log_error("Failed to read from raw essence source: %s\n", mEssenceSource->GetStrError().c_str());

    mTotalReadLength += num_read;
    mSampleBuffer.IncrementSize(num_read);

    return num_read;
}

void RawEssenceReader::ShiftSampleData(uint32_t to_offset, uint32_t from_offset)
{
    BMX_ASSERT(to_offset <= from_offset);
    BMX_ASSERT(from_offset <= mSampleBuffer.GetSize());

    uint32_t size = mSampleBuffer.GetSize() - from_offset;
    if (size > 0)
        memmove(mSampleBuffer.GetBytes() + to_offset, mSampleBuffer.GetBytes() + from_offset, size);
    mSampleBuffer.SetSize(to_offset + size);
}

