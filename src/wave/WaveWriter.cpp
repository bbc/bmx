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
#include <set>

#include <bmx/wave/WaveWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


// prevent buffer size exceeding 50MB
#define MAX_BUFFER_SIZE     (50 * 1000 * 1000)



static WaveChunkId BUILTIN_CHUNKS[] = {
    WAVE_CHUNK_ID("JUNK"), WAVE_CHUNK_ID("ds64"), WAVE_CHUNK_ID("fmt "),
    WAVE_CHUNK_ID("fact"), WAVE_CHUNK_ID("bext"), WAVE_CHUNK_ID("data"),
    WAVE_CHUNK_ID("chna")
};


string WaveWriter::GetBuiltinChunkListString()
{
    string builtin_list;
    for (size_t i = 0; i < BMX_ARRAY_SIZE(BUILTIN_CHUNKS); i++) {
        if (i > 0)
            builtin_list.append(", ");
        builtin_list.append("<" + get_wave_chunk_id_str(BUILTIN_CHUNKS[i]) + ">");
    }
    return builtin_list;
}

bool WaveWriter::IsBuiltinChunk(WaveChunkId id)
{
    for (size_t i = 0; i < BMX_ARRAY_SIZE(BUILTIN_CHUNKS); i++) {
        if (id == BUILTIN_CHUNKS[i])
            return true;
    }
    return false;
}

WaveWriter::WaveWriter(WaveIO *output, bool take_ownership)
{
    mOutput = output;
    mOwnOutput = take_ownership;
    mStartTimecodeSet = false;
    mSetSampleCount = -1;
    mBEXT = new WaveBEXT();
    mCHNA = 0;
    mOwnCHNA = false;
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
    mRequire64Bit = false;
    mWriteBW64 = false;
    mSetSize = -1;
    mSetDataSize = -1;
}

WaveWriter::~WaveWriter()
{
    delete mBEXT;
    if (mOwnCHNA)
        delete mCHNA;
    if (mOwnOutput)
        delete mOutput;

    size_t i;
    for (i = 0; i < mBufferSegments.size(); i++)
        delete mBufferSegments[i];

    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];

    map<WaveChunkId, WaveChunk*>::const_iterator iter;
    for (iter = mOwnedAdditionalChunks.begin(); iter != mOwnedAdditionalChunks.end(); iter++)
        delete iter->second;
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

void WaveWriter::AddCHNA(WaveCHNA *chna, bool take_ownership)
{
    RemoveChunk(chna->Id());

    mCHNA = chna;
    mOwnCHNA = take_ownership;
}

void WaveWriter::AddChunk(WaveChunk *chunk, bool take_ownership)
{
    RemoveChunk(chunk->Id());

    mAdditionalChunks[chunk->Id()] = chunk;
    if (take_ownership)
        mOwnedAdditionalChunks[chunk->Id()] = chunk;
}

bool WaveWriter::HaveChunk(WaveChunkId id)
{
    return (id == "chna" && mCHNA) || mAdditionalChunks.count(id) > 0;
}

void WaveWriter::AddADMAudioID(const WaveCHNA::AudioID &audio_id)
{
    if (!mCHNA) {
        mCHNA = new WaveCHNA();
        mOwnCHNA = true;
    }

    mCHNA->AppendAudioID(audio_id);
}

WaveTrackWriter* WaveWriter::CreateTrack()
{
    mTracks.push_back(new WaveTrackWriter(this, (uint32_t)mTracks.size()));
    return mTracks.back();
}

void WaveWriter::UpdateChannelCounts()
{
    mChannelCount = 0;

    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        WaveTrackWriter *track = mTracks[i];

        track->mStartChannel = mChannelCount;
        mChannelCount += track->mChannelCount;
    }
}

void WaveWriter::PrepareWrite()
{
    set<WaveChunkId> builtin_chunks_set(BUILTIN_CHUNKS, BUILTIN_CHUNKS + BMX_ARRAY_SIZE(BUILTIN_CHUNKS));

    // Update the channel count and track start channels
    UpdateChannelCounts();

    // Ensure ADM <chna> chunk is before other chunks. Filter out built-in chunks.
    // Write BW64 if there are ADM chunks.
    WaveChunk *chna_chunk = 0;
    if (mCHNA)
        mWriteBW64 = true;

    vector<WaveChunk*> write_chunks;
    map<WaveChunkId, WaveChunk*>::const_iterator iter;
    for (iter = mAdditionalChunks.begin(); iter != mAdditionalChunks.end(); iter++) {
        WaveChunk *chunk = iter->second;

        if (chunk->Id() == "axml" || chunk->Id() == "bxml" || chunk->Id() == "sxml") {
            mWriteBW64 = true;
            write_chunks.push_back(chunk);
        } else if (chunk->Id() == "chna") {
            // Accept a <chna> id passed in using AddChunk() rather than AddCHNA()
            // Don't add it to write_chunks here because it needs to be written first
            mWriteBW64 = true;
            chna_chunk = chunk;
        } else if (!builtin_chunks_set.count(chunk->Id())) {
            write_chunks.push_back(chunk);
        }
    }

    mBlockAlign = mChannelBlockAlign * mChannelCount;

    if (!mWriteBW64 && mStartTimecodeSet) {
        mBEXT->SetTimeReference(convert_position(mStartTimecode.GetOffset(),
                                                 mSamplingRate.numerator,
                                                 mStartTimecode.GetRoundedTCBase(),
                                                 ROUND_AUTO));
    }

    if (mSetSampleCount >= 0) {
        mSetDataSize = mSetSampleCount * mBlockAlign;
        mSetSize = 4 + (8 + 28) + (8 + 16) + (8 + mSetDataSize);
        if (!mWriteBW64)
            mSetSize += (8 + 4) + (8 + mBEXT->GetAlignedSize());  // fact and bext chunks
        size_t i;
        for (i = 0; i < write_chunks.size(); i++)
            mSetSize += 8 + write_chunks[i]->AlignedSize();

        if (mSetSize > UINT32_MAX)
            mRequire64Bit = true;
    }

    if (mRequire64Bit) {
        if (mWriteBW64)
            mOutput->WriteId("BW64");
        else
            mOutput->WriteId("RF64");
        mOutput->WriteSize(-1);
    } else {
        mOutput->WriteId("RIFF");
        if (mSetSize >= 0)
            mOutput->WriteSize((uint32_t)mSetSize);
        else
            mOutput->WriteSize(0);
    }
    mOutput->WriteId("WAVE");

    mJunkChunkFilePosition = mOutput->Tell();
    if (mRequire64Bit) {
        mOutput->WriteId("ds64");
        mOutput->WriteSize(28);
        mOutput->WriteUInt64((uint64_t)mSetSize);
        mOutput->WriteUInt64((uint64_t)mSetDataSize);
        if (mWriteBW64)
            mOutput->WriteUInt64(0);  // dummy / not used
        else
            mOutput->WriteUInt64((uint64_t)mSetSampleCount);
        mOutput->WriteUInt32(0);
    } else {
        mOutput->WriteId("JUNK");
        mOutput->WriteSize(28);
        mOutput->WriteZeros(28);
    }

    if (!mWriteBW64) {
        mBEXTFilePosition = mOutput->Tell();
        mBEXT->Write(mOutput);
    }

    mOutput->WriteId("fmt ");
    mOutput->WriteSize(16);
    mOutput->WriteUInt16(1); // PCM
    mOutput->WriteUInt16(mChannelCount);
    mOutput->WriteUInt32(mSamplingRate.numerator);
    mOutput->WriteUInt32(mBlockAlign * mSamplingRate.numerator); // bytes per second
    mOutput->WriteUInt16(mBlockAlign);
    mOutput->WriteUInt16(mQuantizationBits);

    if (!mWriteBW64) {
        mFactChunkFilePosition = mOutput->Tell();
        mOutput->WriteId("fact");
        mOutput->WriteSize(4);
        if (mRequire64Bit)
            mOutput->WriteUInt32((uint32_t)(-1));
        else if (mSetSampleCount >= 0)
            mOutput->WriteUInt32((uint32_t)mSetSampleCount);
        else
            mOutput->WriteUInt32(0);
    }

    if (mCHNA)
        mCHNA->Write(mOutput);
    else if (chna_chunk)
        mOutput->WriteChunk(chna_chunk);

    size_t i;
    for (i = 0; i < write_chunks.size(); i++)
        mOutput->WriteChunk(write_chunks[i]);

    mDataChunkFilePosition = mOutput->Tell();
    mOutput->WriteId("data");
    if (mRequire64Bit)
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

    if (!mWriteBW64 && mStartTimecodeSet) {
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

    if (mRequire64Bit || riff_size > UINT32_MAX) {
        if (!mRequire64Bit) {
            mOutput->Seek(mDataChunkFilePosition + 4, SEEK_SET);
            mOutput->WriteSize((uint32_t)(-1));

            if (!mWriteBW64) {
                mOutput->Seek(mFactChunkFilePosition + 8, SEEK_SET);
                mOutput->WriteUInt32((uint32_t)(-1));
            }
        }

        if (!mWriteBW64 && mBEXT->WasUpdated()) {
            mOutput->Seek(mBEXTFilePosition, SEEK_SET);
            mBEXT->Write(mOutput);
        }

        if (!mRequire64Bit || riff_size != mSetSize || mSampleCount != mSetSampleCount) {
            mOutput->Seek(mJunkChunkFilePosition, SEEK_SET);
            mOutput->WriteId("ds64");
            mOutput->WriteSize(28);
            mOutput->WriteUInt64((uint64_t)riff_size);
            mOutput->WriteUInt64((uint64_t)data_size);
            mOutput->WriteUInt64((uint64_t)mSampleCount);
            mOutput->WriteUInt32(0);
        }

        if (!mRequire64Bit) {
            mOutput->Seek(0, SEEK_SET);
            if (mWriteBW64)
                mOutput->WriteId("BW64");
            else
                mOutput->WriteId("RF64");
            mOutput->WriteSize((uint32_t)(-1));
        }
    } else {
        if (riff_size != mSetSize) {
            mOutput->Seek(mDataChunkFilePosition + 4, SEEK_SET);
            mOutput->WriteSize((uint32_t)data_size);
        }

        if (!mWriteBW64) {
            if (mSampleCount != mSetSampleCount) {
                mOutput->Seek(mFactChunkFilePosition + 8, SEEK_SET);
                mOutput->WriteUInt32((uint32_t)mSampleCount);
            }

            if (mBEXT->WasUpdated()) {
                mOutput->Seek(mBEXTFilePosition, SEEK_SET);
                mBEXT->Write(mOutput);
            }
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

void WaveWriter::RemoveChunk(WaveChunkId id)
{
    if (id == "chna") {
        if (mOwnCHNA)
            delete mCHNA;
        mCHNA = 0;
        mOwnCHNA = false;
    }

    if (mAdditionalChunks.count(id)) {
        mAdditionalChunks.erase(id);

        if (mOwnedAdditionalChunks.count(id)) {
            delete mOwnedAdditionalChunks[id];
            mOwnedAdditionalChunks.erase(id);
        }
    }
}
