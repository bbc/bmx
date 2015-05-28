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

#ifndef BMX_WAVE_WRITER_H_
#define BMX_WAVE_WRITER_H_


#include <vector>
#include <deque>

#include <bmx/ByteArray.h>
#include <bmx/wave/WaveIO.h>
#include <bmx/wave/WaveBEXT.h>
#include <bmx/wave/WaveTrackWriter.h>



namespace bmx
{


class WaveWriter
{
public:
    friend class WaveTrackWriter;

public:
    WaveWriter(WaveIO *output, bool take_ownership);
    ~WaveWriter();

    void SetStartTimecode(Timecode start_timecode);     // sets time_reference in bext
    void SetSampleCount(int64_t count);                 // metadata and sizes are calculated and set
    WaveBEXT* GetBroadcastAudioExtension() { return mBEXT; }

public:
    WaveTrackWriter* CreateTrack();

public:
    void PrepareWrite();
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void CompleteWrite();

public:
    Rational GetSamplingRate() const    { return mSamplingRate; }
    int64_t GetDuration() const         { return mSampleCount; }

    uint32_t GetNumTracks() const       { return (uint32_t)mTracks.size(); }
    WaveTrackWriter* GetTrack(uint32_t track_index);

private:
    void SetSamplingRate(Rational sampling_rate);
    void SetQuantizationBits(uint16_t bits);

private:
    WaveIO *mOutput;
    bool mOwnOutput;
    Timecode mStartTimecode;
    bool mStartTimecodeSet;
    int64_t mSetSampleCount;
    WaveBEXT *mBEXT;

    Rational mSamplingRate;
    bool mSamplingRateSet;
    uint16_t mQuantizationBits;
    bool mQuantizationBitsSet;
    uint16_t mChannelCount;
    uint16_t mChannelBlockAlign;
    uint16_t mBlockAlign;

    std::vector<WaveTrackWriter*> mTracks;

    typedef struct
    {
        ByteArray data;
        int64_t start_sample_count;
        uint32_t num_samples;
        uint16_t channel_count;
    } BufferSegment;

    std::deque<BufferSegment*> mBufferSegments;
    uint32_t mBufferSize;
    int64_t mSampleCount;

    int64_t mJunkChunkFilePosition;
    int64_t mBEXTFilePosition;
    int64_t mDataChunkFilePosition;
    int64_t mFactChunkFilePosition;

    bool mUseRF64;
    int64_t mSetSize;
    int64_t mSetDataSize;
};


};



#endif

