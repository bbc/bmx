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

#define __STDC_LIMIT_MACROS

#include <cstring>

#include <bmx/wave/WaveWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


// prevent buffer size exceeding 50MB
#define MAX_BUFFER_SIZE     (50 * 1000 * 1000)



WaveWriter::WaveWriter(WaveIO *output, bool take_ownership)
{
    mOutput = output;
    mOwnOutput = take_ownership;
    mStartTimecodeSet = false;
    mSetSampleCount = -1;
    mBEXT = new WaveBEXT();
    mSamplingRate = SAMPLING_RATE_48K;
    mSamplingRateSet = false;
    mQuantizationBits = 16;
    mQuantizationBitsSet = false;
    mChannelCount = 0;
    mChannelBlockAlign = (mQuantizationBits + 7) / 8;
    mBlockAlign = 0;
    mBufferSize = 0;
    mSampleCount = 0;
    mJunkChunkFilePosition = 0;
    mBEXTFilePosition = 0;
    mDataChunkFilePosition = 0;
    mFactChunkFilePosition = 0;
    mUseRF64 = false;
    mSetSize = -1;
    mSetDataSize = -1;
}

WaveWriter::~WaveWriter()
{
    delete mBEXT;
    if (mOwnOutput)
        delete mOutput;

    size_t i;
    for (i = 0; i < mBufferSegments.size(); i++)
        delete mBufferSegments[i];

    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];
}

void WaveWriter::SetStartTimecode(Timecode start_timecode)
{
    mStartTimecode = start_timecode;
    mStartTimecodeSet = true;
}

void WaveWriter::SetSampleCount(int64_t count)
{
    mSetSampleCount = count;
}

WaveTrackWriter* WaveWriter::CreateTrack()
{
    mTracks.push_back(new WaveTrackWriter(this, (uint32_t)mTracks.size()));
    return mTracks.back();
}

void WaveWriter::PrepareWrite()
{
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        mTracks[i]->mStartChannel = mChannelCount;
        mChannelCount += mTracks[i]->mChannelCount;
    }
    mBlockAlign = mChannelBlockAlign * mChannelCount;

    if (mStartTimecodeSet) {
        mBEXT->SetTimeReference(convert_position(mStartTimecode.GetOffset(),
                                                 mSamplingRate.numerator,
                                                 mStartTimecode.GetRoundedTCBase(),
                                                 ROUND_AUTO));
    }

    if (mSetSampleCount >= 0) {
        mSetDataSize = mSetSampleCount * mBlockAlign;
        mSetSize = 4 + (8 + 28) + (8 + mBEXT->GetSize()) + (8 + 16) + (8 + 4) + (8 + mSetDataSize);
        if (mSetSize > UINT32_MAX)
            mUseRF64 = true;
    }

    if (mUseRF64) {
        mOutput->WriteTag("RF64");
        mOutput->WriteSize(-1);
    } else {
        mOutput->WriteTag("RIFF");
        if (mSetSize >= 0)
            mOutput->WriteSize((uint32_t)mSetSize);
        else
            mOutput->WriteSize(0);
    }
    mOutput->WriteTag("WAVE");

    mJunkChunkFilePosition = mOutput->Tell();
    if (mUseRF64) {
        mOutput->WriteTag("ds64");
        mOutput->WriteSize(28);
        mOutput->WriteUInt64((uint64_t)mSetSize);
        mOutput->WriteUInt64((uint64_t)mSetDataSize);
        mOutput->WriteUInt64((uint64_t)mSetSampleCount);
        mOutput->WriteUInt32(0);
    } else {
        mOutput->WriteTag("JUNK");
        mOutput->WriteSize(28);
        mOutput->WriteZeros(28);
    }

    mBEXTFilePosition = mOutput->Tell();
    mBEXT->Write(mOutput);

    mOutput->WriteTag("fmt ");
    mOutput->WriteSize(16);
    mOutput->WriteUInt16(1); // PCM
    mOutput->WriteUInt16(mChannelCount);
    mOutput->WriteUInt32(mSamplingRate.numerator);
    mOutput->WriteUInt32(mBlockAlign * mSamplingRate.numerator); // bytes per second
    mOutput->WriteUInt16(mBlockAlign);
    mOutput->WriteUInt16(mQuantizationBits);

    mFactChunkFilePosition = mOutput->Tell();
    mOutput->WriteTag("fact");
    mOutput->WriteSize(4);
    if (mUseRF64)
        mOutput->WriteUInt32((uint32_t)(-1));
    else if (mSetSampleCount >= 0)
        mOutput->WriteUInt32((uint32_t)mSetSampleCount);
    else
        mOutput->WriteUInt32(0);

    mDataChunkFilePosition = mOutput->Tell();
    mOutput->WriteTag("data");
    if (mUseRF64)
        mOutput->WriteSize(-1);
    else if (mSetDataSize >= 0)
        mOutput->WriteSize((uint32_t)mSetDataSize);
    else
        mOutput->WriteSize(0);
}

void WaveWriter::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (size == 0)
        return;

    WaveTrackWriter *track = GetTrack(track_index);
    uint16_t track_block_align = track->mChannelCount * mChannelBlockAlign;
    BMX_CHECK(size >= num_samples * track_block_align);

    uint32_t input_sample_count = 0;
    uint32_t output_sample_count = 0;

    if (mTracks.size() == 1) {
        // no buffering required
        mOutput->Write(data, num_samples * track_block_align);
        input_sample_count = num_samples;
        output_sample_count = num_samples;
    } else {
        // need to buffer
        BufferSegment *segment;
        size_t segment_index;
        uint32_t segment_offset;
        if (track->mSampleCount == mSampleCount) {
            // create new segment
            BMX_CHECK(mBufferSize < MAX_BUFFER_SIZE);
            mBufferSegments.push_back(new BufferSegment);
            segment = mBufferSegments.back();
            segment->start_sample_count = mSampleCount;
            segment->num_samples = num_samples;
            segment->channel_count = 0;
            segment->data.Allocate(mBlockAlign * num_samples);
            segment->data.SetSize(mBlockAlign * num_samples);
            memset(segment->data.GetBytes(), 0, segment->data.GetSize());

            mBufferSize += mBlockAlign * num_samples;
            segment_index = 0;
            segment_offset = 0;
            input_sample_count = num_samples;
            output_sample_count = num_samples;
        } else {
            // get existing segment
            BMX_ASSERT(!mBufferSegments.empty());
            for (segment_index = 0; segment_index < mBufferSegments.size() - 1; segment_index++) {
                if (track->mSampleCount < mBufferSegments[segment_index + 1]->start_sample_count)
                    break;
            }
            segment = mBufferSegments[segment_index];
            segment_offset = (uint32_t)(track->mSampleCount - segment->start_sample_count);
            input_sample_count = segment->num_samples - segment_offset;
            if (input_sample_count > num_samples)
                input_sample_count = num_samples;
        }

        // copy samples to segment
        unsigned char *output_ptr = segment->data.GetBytes() + segment_offset * mBlockAlign +
                                    track->mStartChannel * mChannelBlockAlign;
        const unsigned char *input_ptr = data;
        uint32_t i;
        for (i = 0; i < input_sample_count; i++) {
            memcpy(output_ptr, input_ptr, track_block_align);
            output_ptr += mBlockAlign;
            input_ptr += track_block_align;
        }
        if (segment_offset + input_sample_count == segment->num_samples)
            segment->channel_count += track->mChannelCount;

        // write complete buffer segments
        if (segment_index == 0 && segment->channel_count == mChannelCount) {
            mOutput->Write(segment->data.GetBytes(), segment->data.GetSize());
            mBufferSize -= segment->data.GetSize();
            delete segment;
            mBufferSegments.pop_front();
        }
    }

    track->mSampleCount += input_sample_count;
    mSampleCount += output_sample_count;


    if (input_sample_count < num_samples) {
        WriteSamples(track_index, data + input_sample_count * track_block_align,
                     (num_samples - input_sample_count) * track_block_align,
                     num_samples - input_sample_count);
    }
}

void WaveWriter::CompleteWrite()
{
    // write remaining buffered samples
    if (!mBufferSegments.empty()) {
        log_warn("Wave tracks with unequal duration\n");

        size_t i;
        for (i = 0; i < mBufferSegments.size(); i++) {
            mOutput->Write(mBufferSegments[i]->data.GetBytes(), mBufferSegments[i]->data.GetSize());
            mBufferSize -= mBufferSegments[i]->data.GetSize();
            delete mBufferSegments[i];
        }
        mBufferSegments.clear();
    }

    if (mStartTimecodeSet) {
        mBEXT->SetTimeReference(convert_position(mStartTimecode.GetOffset(),
                                                 mSamplingRate.numerator,
                                                 mStartTimecode.GetRoundedTCBase(),
                                                 ROUND_AUTO));
    }


    int64_t riff_size = mOutput->Size() - 8;
    int64_t data_size = riff_size - mDataChunkFilePosition;

    // add pad byte if data size is odd
    if ((data_size & 1))
        mOutput->PutChar(0);

    if (mUseRF64 || riff_size > UINT32_MAX) {
        if (!mUseRF64) {
            mOutput->Seek(mDataChunkFilePosition + 4, SEEK_SET);
            mOutput->WriteSize((uint32_t)(-1));

            mOutput->Seek(mFactChunkFilePosition + 8, SEEK_SET);
            mOutput->WriteUInt32((uint32_t)(-1));
        }

        if (mBEXT->WasUpdated()) {
            mOutput->Seek(mBEXTFilePosition, SEEK_SET);
            mBEXT->Write(mOutput);
        }

        if (!mUseRF64 || riff_size != mSetSize || mSampleCount != mSetSampleCount) {
            mOutput->Seek(mJunkChunkFilePosition, SEEK_SET);
            mOutput->WriteTag("ds64");
            mOutput->WriteSize(28);
            mOutput->WriteUInt64((uint64_t)riff_size);
            mOutput->WriteUInt64((uint64_t)data_size);
            mOutput->WriteUInt64((uint64_t)mSampleCount);
            mOutput->WriteUInt32(0);
        }

        if (!mUseRF64) {
            mOutput->Seek(0, SEEK_SET);
            mOutput->WriteTag("RF64");
            mOutput->WriteSize((uint32_t)(-1));
        }
    } else {
        if (riff_size != mSetSize) {
            mOutput->Seek(mDataChunkFilePosition + 4, SEEK_SET);
            mOutput->WriteSize((uint32_t)data_size);
        }

        if (mSampleCount != mSetSampleCount) {
            mOutput->Seek(mFactChunkFilePosition + 8, SEEK_SET);
            mOutput->WriteUInt32((uint32_t)mSampleCount);
        }

        if (mBEXT->WasUpdated()) {
            mOutput->Seek(mBEXTFilePosition, SEEK_SET);
            mBEXT->Write(mOutput);
        }

        if (riff_size != mSetSize) {
            mOutput->Seek(4, SEEK_SET);
            mOutput->WriteSize((uint32_t)riff_size);
        }
    }
}

WaveTrackWriter* WaveWriter::GetTrack(uint32_t track_index)
{
    BMX_CHECK(track_index < mTracks.size());
    return mTracks[track_index];
}

void WaveWriter::SetSamplingRate(Rational sampling_rate)
{
    BMX_CHECK(sampling_rate.denominator == 1);
    BMX_CHECK_M(!mSamplingRateSet || sampling_rate == mSamplingRate,
                ("Variable sampling rate not supported in wave writer"));

    mSamplingRate = sampling_rate;
    mSamplingRateSet = true;
}

void WaveWriter::SetQuantizationBits(uint16_t bits)
{
    BMX_CHECK(bits > 0 && bits <= 32);
    BMX_CHECK_M(!mQuantizationBitsSet || bits == mQuantizationBits,
                ("Variable quantization bits not supported in wave writer"));

    mQuantizationBits = bits;
    mQuantizationBitsSet = true;

    mChannelBlockAlign = (mQuantizationBits + 7) / 8;
}

