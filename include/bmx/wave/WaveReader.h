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

#ifndef BMX_WAVE_READER_H_
#define BMX_WAVE_READER_H_


#include <vector>
#include <deque>

#include <bmx/ByteArray.h>
#include <bmx/wave/WaveIO.h>
#include <bmx/wave/WaveBEXT.h>
#include <bmx/wave/WaveTrackReader.h>



namespace bmx
{


class WaveReader
{
public:
    static WaveReader* Open(WaveIO *input, bool take_ownership);

public:
    ~WaveReader();

    void SetReadLimits();
    void SetReadLimits(int64_t start_position, int64_t duration, bool seek_start_position);
    int64_t GetReadStartPosition() const { return mReadStartPosition; }
    int64_t GetReadDuration() const      { return mReadDuration; }

public:
    uint32_t Read(uint32_t num_samples);
    void Seek(int64_t position);

    int64_t GetPosition() const { return mPosition; }
    int64_t GetDuration() const { return mSampleCount; }

public:
    Rational GetSamplingRate() const      { return mSamplingRate; }
    uint16_t GetChannelCount() const      { return mChannelCount; }
    uint16_t GetQuantizationBits() const  { return mQuantizationBits; }
    uint16_t GetChannelBlockAlign() const { return mChannelBlockAlign; }
    uint16_t GetBlockAlign() const        { return mBlockAlign; }
    WaveBEXT* GetBEXT() const             { return mBEXT; }

    uint32_t GetNumTracks() const         { return (uint32_t)mTracks.size(); }
    WaveTrackReader* GetTrack(uint32_t track_index) const;

    bool IsEnabled() const;

private:
    WaveReader(WaveIO *input, bool take_ownership, bool is_rf64, uint32_t riff_size);

    void ReadChunks(bool is_rf64, uint32_t riff_size);

private:
    WaveIO *mInput;
    bool mOwnInput;

    Rational mSamplingRate;
    uint16_t mChannelCount;
    uint16_t mQuantizationBits;
    uint16_t mChannelBlockAlign;
    uint16_t mBlockAlign;
    WaveBEXT *mBEXT;
    int64_t mSampleCount;

    std::vector<WaveTrackReader*> mTracks;

    int64_t mReadStartPosition;
    int64_t mReadDuration;

    int64_t mDataStartFilePosition;
    int64_t mPosition;
    ByteArray mReadBuffer;
};


};



#endif

