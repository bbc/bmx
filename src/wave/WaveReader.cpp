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

#define __STDC_FORMAT_MACROS

#include <cstring>

#include <bmx/wave/WaveReader.h>
#include <bmx/wave/WaveWriter.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



WaveReader* WaveReader::Open(WaveIO *input, bool take_ownership)
{
    try
    {
        WaveChunkId id;
        if (!input->ReadId(&id)) {
            log_error("File is not a Wave file\n");
            throw false;
        }

        bool require_ds64;
        if (id == "RIFF") {
            require_ds64 = false;
        } else if (id == "RF64" || id == "BW64") {
            require_ds64 = true;
        } else {
            log_error("File is not a Wave file\n");
            throw false;
        }

        uint32_t size = input->ReadSize();

        if (!input->ReadId(&id) || id != "WAVE") {
            log_error("File is not a Wave file\n");
            throw false;
        }

        return new WaveReader(input, take_ownership, require_ds64, size);
    }
    catch (const BMXException &ex)
    {
        log_error("Wave file open exception: %s\n", ex.what());
        if (take_ownership)
            delete input;
        return 0;
    }
    catch (...)
    {
        log_error("Unknown Wave file open exception\n");
        if (take_ownership)
            delete input;
        return 0;
    }
}

string WaveReader::GetBuiltinChunkListString()
{
    return WaveWriter::GetBuiltinChunkListString();
}

bool WaveReader::IsBuiltinChunk(WaveChunkId id)
{
    return WaveWriter::IsBuiltinChunk(id);
}

WaveReader::WaveReader(WaveIO *input, bool take_ownership, bool require_ds64, uint32_t riff_size)
{
    mInput = input;
    mOwnInput = take_ownership;
    mQuantizationBits = 0;
    mSamplingRate = ZERO_RATIONAL;
    mChannelCount = 0;
    mChannelBlockAlign = 0;
    mBlockAlign = 0;
    mBEXT = 0;
    mCHNA = 0;
    mSampleCount = 0;
    mReadStartPosition = 0;
    mReadDuration = 0;
    mDataStartFilePosition = 0;
    mPosition = 0;

    ReadChunks(require_ds64, riff_size);

    // 1 track per channel
    uint16_t i;
    for (i = 0; i < mChannelCount; i++)
        mTracks.push_back(new WaveTrackReader(this, i, 1));

    // default read everything and position at start of sample data
    SetReadLimits();
}

WaveReader::~WaveReader()
{
    delete mBEXT;
    delete mCHNA;
    if (mOwnInput)
        delete mInput;

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];
    for (i = 0; i < mChunks.size(); i++)
        delete mChunks[i];
}

void WaveReader::SetReadLimits()
{
    SetReadLimits(0, mSampleCount, true);
}

void WaveReader::SetReadLimits(int64_t start_position, int64_t duration, bool seek_to_start)
{
    mReadStartPosition = start_position;
    mReadDuration = duration;

    if (seek_to_start)
        Seek(start_position);
}

uint32_t WaveReader::Read(uint32_t num_samples)
{
    int64_t end_position = mPosition + num_samples;

    // check read limits
    if (mSampleCount <= 0 ||
        mPosition >= mSampleCount ||
        mPosition >= mReadStartPosition + mReadDuration ||
        end_position <= 0 ||
        end_position <= mReadStartPosition)
    {
        // always be positioned num_samples after previous position
        Seek(end_position);
        return 0;
    }

    // adjust the sample count if needed
    // seek if at the start of the data
    uint32_t first_sample_offset = 0;
    uint32_t read_num_samples = num_samples;
    if (mPosition <= mReadStartPosition) {
        first_sample_offset = (uint32_t)(mReadStartPosition - mPosition);
        read_num_samples -= first_sample_offset;
        Seek(mReadStartPosition);
    }
    if (end_position > mReadStartPosition + mReadDuration)
        read_num_samples -= (uint32_t)(end_position - (mReadStartPosition + mReadDuration));
    BMX_ASSERT(read_num_samples > 0);


    // read samples
    bool have_read = false;
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (!mTracks[i]->IsEnabled())
            continue;

        Frame *frame = mTracks[i]->GetFrameBuffer()->CreateFrame();
        frame->Grow(read_num_samples * mTracks[i]->GetBlockAlign());

        // read samples directly into the frame if there is only 1 track; else read into read buffer
        if (!have_read) {
            unsigned char *buffer;
            if (mTracks.size() == 1) {
                buffer = frame->GetBytesAvailable();
            } else {
                mReadBuffer.Allocate(read_num_samples * mBlockAlign);
                buffer = mReadBuffer.GetBytes();
            }

            uint32_t bytes_read = mInput->Read(buffer, read_num_samples * mBlockAlign);
            if (bytes_read < read_num_samples * mBlockAlign) {
                log_error("Failed to read %u samples of %u\n",
                          read_num_samples - bytes_read / mBlockAlign, read_num_samples);
                read_num_samples = bytes_read / mBlockAlign;
            }

            if (mTracks.size() > 1)
                mReadBuffer.SetSize(read_num_samples * mBlockAlign);

            have_read = true;
        }

        if (mTracks.size() > 1) {
            deinterleave_audio(mReadBuffer.GetBytes(), mReadBuffer.GetSize(),
                               mQuantizationBits, mChannelCount, (uint16_t)i,
                               frame->GetBytesAvailable(), frame->GetSizeAvailable());
        }

        frame->SetSize(read_num_samples * mTracks[i]->GetBlockAlign());

        frame->edit_rate            = mSamplingRate;
        frame->position             = mPosition;
        frame->track_edit_rate      = mSamplingRate;
        frame->track_position       = mPosition;
        frame->request_num_samples  = num_samples;
        frame->first_sample_offset  = first_sample_offset;
        frame->num_samples          = read_num_samples;
        frame->file_position        = mDataStartFilePosition + mPosition * mBlockAlign;

        mTracks[i]->GetFrameBuffer()->PushFrame(frame);
    }
    if (have_read)
        mPosition += read_num_samples;


    // always be positioned num_samples after previous position
    if (mPosition < end_position)
        Seek(end_position);


    return read_num_samples;
}

void WaveReader::Seek(int64_t position)
{
    if (position >= 0 && position < mSampleCount &&
        position >= mReadStartPosition && position < mReadStartPosition + mReadDuration)
    {
        mInput->Seek(mDataStartFilePosition + position * mBlockAlign, SEEK_SET);
    }
    mPosition = position;
}

WaveTrackReader* WaveReader::GetTrack(uint32_t track_index) const
{
    BMX_CHECK(track_index < mTracks.size());
    return mTracks[track_index];
}

bool WaveReader::IsEnabled() const
{
    size_t i;
    for (i = 0; i < mTracks.size(); i++) {
        if (mTracks[i]->IsEnabled())
            return true;
    }

    return false;
}

WaveFileChunk* WaveReader::GetAdditionalChunk(uint32_t index) const
{
    BMX_CHECK(index < mChunks.size());
    return mChunks[index];
}

WaveFileChunk* WaveReader::GetAdditionalChunk(WaveChunkId id) const
{
    size_t i;
    for (i = 0; i < mChunks.size(); i++) {
        if (mChunks[i]->Id() == id)
            return mChunks[i];
    }

    return 0;
}

void WaveReader::ReadChunks(bool require_ds64, uint32_t riff_size)
{
    (void)riff_size;

    uint64_t actual_data_size = 0;
    bool have_ds64 = false;
    bool have_fmt  = false;
    bool have_fact = false;
    bool have_data = false;
    WaveChunkId id;

    while (true) {
        if (!mInput->ReadId(&id))
            break;

        // Use uint64_t for size to allow assignment of actual_data_size and check to skip the WORD alignment byte
        uint64_t size = mInput->ReadSize();
        if (require_ds64 && id == "ds64") {
            BMX_CHECK(size == 28);
            mInput->Skip(8); // riff size
            actual_data_size = mInput->ReadUInt64();
            mSampleCount = (int64_t)mInput->ReadUInt64();
            BMX_CHECK(mSampleCount >= 0);
            mInput->Skip(4);
            have_ds64 = true;
        } else if (id == "fmt ") {
            BMX_CHECK(size >= 16);
            uint16_t format_category = mInput->ReadUInt16();
            if (!(format_category == 1 || format_category == 0xFFFE)) { // PCM
                log_error("Unsupported non-PCM Wave format category 0x%04x\n", format_category);
                throw false;
            }
            mChannelCount = mInput->ReadUInt16();
            mSamplingRate.numerator = (int32_t)mInput->ReadUInt32();
            mSamplingRate.denominator = 1;
            mInput->Skip(4); // bytes per second
            mBlockAlign = mInput->ReadUInt16();
            mChannelBlockAlign = mBlockAlign / mChannelCount;
            mQuantizationBits = mInput->ReadUInt16();
            BMX_CHECK((mQuantizationBits + 7) / 8 * mChannelCount == mBlockAlign);
            if (size > 16)
                mInput->Skip(size - 16);
            have_fmt = true;
        } else if (id == "fact") {
            BMX_CHECK(size == 4);
            if (require_ds64) {
                BMX_CHECK(have_ds64);
                mInput->Skip(4);
            } else {
                mSampleCount = (int64_t)mInput->ReadUInt32();
            }
            have_fact = true;
        } else if (id == "bext") {
            delete mBEXT;
            mBEXT = new WaveBEXT();
            mBEXT->Read(mInput, (uint32_t)size);
        } else if (id == "chna") {
            delete mCHNA;
            mCHNA = new WaveCHNA();
            mCHNA->Read(mInput, (uint32_t)size);
        } else if (id == "data") {
            mDataStartFilePosition = mInput->Tell();
            if (require_ds64) {
                BMX_CHECK(have_ds64);
                size = actual_data_size;
            } else {
                actual_data_size = size;
            }
            mInput->Skip(actual_data_size);
            have_data = true;
        } else {
            if (id != "JUNK")
                mChunks.push_back(new WaveFileChunk(id, mInput, false, mInput->Tell(), (uint32_t)size));

            mInput->Skip(size);
        }

        // Skip the WORD alignment byte
        if ((size & 1))
            mInput->Skip(1);
    }

    if (!have_data) {
        log_error("No data chunk found in Wave file\n");
        throw false;
    }
    if (!have_fmt) {
        log_error("No fmt chunk found in Wave file\n");
        throw false;
    }
    if (mChannelCount == 0) {
        log_error("No channels in Wave file\n");
        throw false;
    }

    int64_t data_sample_count = (int64_t)(mBlockAlign > 0 ? actual_data_size / mBlockAlign : 0);
    if (have_fact) {
        if (mSampleCount > data_sample_count) {
            log_warn("Missing %" PRId64 " samples in Wave data chunk\n", mSampleCount - data_sample_count);
            mSampleCount = data_sample_count;
        } else if (mSampleCount < data_sample_count) {
            log_warn("Wave data chunk size %" PRIu64 " is larger than expected %" PRIu64 "\n",
                     actual_data_size, mSampleCount * mBlockAlign);
        }
    } else {
        mSampleCount = data_sample_count;
    }
}

