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

#include <bmx/wave/WaveTrackReader.h>
#include <bmx/wave/WaveReader.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



WaveTrackReader::WaveTrackReader(WaveReader *reader, uint16_t channel_offset, uint16_t channel_count)
{
    mReader = reader;
    mChannelOffset = channel_offset;
    mChannelCount = channel_count;
    mSamplingRate = reader->GetSamplingRate();
    mQuantizationBits = reader->GetQuantizationBits();
    mChannelBlockAlign = reader->GetChannelBlockAlign();
    mBlockAlign = mChannelBlockAlign * mChannelCount;
    mIsEnabled = true;
    mFrameBuffer = new DefaultFrameBuffer();
    mOwnFrameBuffer = true;
}

WaveTrackReader::~WaveTrackReader()
{
    if (mOwnFrameBuffer)
        delete mFrameBuffer;
}

void WaveTrackReader::SetEnable(bool enable)
{
    mIsEnabled = enable;
}

void WaveTrackReader::SetFrameBuffer(FrameBuffer *frame_buffer, bool take_ownership)
{
    if (mOwnFrameBuffer)
        delete mFrameBuffer;

    mFrameBuffer = frame_buffer;
    mOwnFrameBuffer = take_ownership;
}

uint32_t WaveTrackReader::Read(uint32_t num_samples)
{
    return mReader->Read(num_samples);
}

void WaveTrackReader::Seek(int64_t position)
{
    mReader->Seek(position);
}

int64_t WaveTrackReader::GetPosition()
{
    return mReader->GetPosition();
}

int64_t WaveTrackReader::GetDuration() const
{
    return mReader->GetDuration();
}

