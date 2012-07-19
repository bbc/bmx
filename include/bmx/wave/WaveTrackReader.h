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

#ifndef BMX_WAVE_TRACK_READER_H_
#define BMX_WAVE_TRACK_READER_H_


#include <bmx/BMXTypes.h>
#include <bmx/frame/FrameBuffer.h>



namespace bmx
{

class WaveReader;

class WaveTrackReader
{
public:
    WaveTrackReader(WaveReader *reader, uint16_t channel_offset, uint16_t channel_count);
    ~WaveTrackReader();

    void SetEnable(bool enable);
    void SetFrameBuffer(FrameBuffer *frame_buffer, bool take_ownership);

public:
    uint32_t Read(uint32_t num_samples);
    void Seek(int64_t position);

    int64_t GetPosition();
    int64_t GetDuration() const;

public:
    Rational GetSamplingRate() const        { return mSamplingRate; }
    uint16_t GetChannelCount() const        { return mChannelCount; }
    uint16_t GetQuantizationBits() const    { return mQuantizationBits; }
    uint16_t GetChannelBlockAlign() const   { return mChannelBlockAlign; }
    uint16_t GetBlockAlign() const          { return mBlockAlign; }

    bool IsEnabled() const                  { return mIsEnabled; }

    FrameBuffer* GetFrameBuffer()           { return mFrameBuffer; }

private:
    WaveReader *mReader;
    uint16_t mChannelOffset;
    Rational mSamplingRate;
    uint16_t mChannelCount;
    uint16_t mQuantizationBits;
    uint16_t mChannelBlockAlign;
    uint16_t mBlockAlign;

    bool mIsEnabled;

    FrameBuffer *mFrameBuffer;
    bool mOwnFrameBuffer;
};


};



#endif

