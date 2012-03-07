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

#include <cstring>

#include <bmx/wave/WaveTrackWriter.h>
#include <bmx/wave/WaveWriter.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



WaveTrackWriter::WaveTrackWriter(WaveWriter *writer, uint32_t track_index)
{
    mWriter = writer;
    mTrackIndex = track_index;
    mChannelCount = 1;
    mSampleCount = 0;
}

WaveTrackWriter::~WaveTrackWriter()
{
}

void WaveTrackWriter::SetSamplingRate(Rational sampling_rate)
{
    mWriter->SetSamplingRate(sampling_rate);
}

void WaveTrackWriter::SetQuantizationBits(uint16_t bits)
{
    mWriter->SetQuantizationBits(bits);
}

void WaveTrackWriter::SetChannelCount(uint16_t count)
{
    BMX_CHECK(count > 0);
    mChannelCount = count;
}

void WaveTrackWriter::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    mWriter->WriteSamples(mTrackIndex, data, size, num_samples);
}

uint32_t WaveTrackWriter::GetSampleSize() const
{
    return mWriter->mChannelBlockAlign * mChannelCount;
}

Rational WaveTrackWriter::GetSamplingRate() const
{
    return mWriter->GetSamplingRate();
}

int64_t WaveTrackWriter::GetDuration() const
{
    return mWriter->GetDuration();
}

