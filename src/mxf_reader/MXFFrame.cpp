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

#include <im/mxf_reader/MXFFrame.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;



MXFFrame::MXFFrame()
{
    MXFFrame::Reset();
    mTemporalReordering = false;
    mTemporalReorderingWasSet = false;
}

MXFFrame::~MXFFrame()
{
}

void MXFFrame::Reset()
{
    mPosition = (int64_t)((uint64_t)1<<63);
    mFirstSampleOffset = 0;
    mNumSamples = 0;
    mTemporalOffset = 0;
    mKeyFrameOffset = 0;
    mFlags = 0;
    mCPFilePosition = 0;
    mFilePosition = 0;
}

void MXFFrame::SetPosition(int64_t position)
{
    mPosition = position;
}

void MXFFrame::SetFirstSampleOffset(uint32_t offset)
{
    mFirstSampleOffset = offset;
}

void MXFFrame::SetNumSamples(uint32_t num_samples)
{
    mNumSamples = num_samples;
}

void MXFFrame::IncNumSamples(uint32_t num_samples)
{
    mNumSamples += num_samples;
}

void MXFFrame::SetTemporalReordering(bool reorder)
{
    mTemporalReordering = reorder;
    mTemporalReorderingWasSet = true;
}

void MXFFrame::SetTemporalOffset(int8_t offset)
{
    mTemporalOffset = offset;
}

void MXFFrame::SetKeyFrameOffset(int8_t offset)
{
    mKeyFrameOffset = offset;
}

void MXFFrame::SetFlags(uint8_t flags)
{
    mFlags = flags;
}

void MXFFrame::SetCPFilePosition(int64_t position)
{
    mCPFilePosition = position;
}

void MXFFrame::SetFilePosition(int64_t position)
{
    mFilePosition = position;
}

void MXFFrame::CopyMetadataTo(MXFFrame *to_frame) const
{
    to_frame->mPosition = mPosition;
    to_frame->mFirstSampleOffset = mFirstSampleOffset;
    to_frame->mNumSamples = mNumSamples;
    to_frame->mTemporalReorderingWasSet = mTemporalReorderingWasSet;
    to_frame->mTemporalReordering = mTemporalReordering;
    to_frame->mTemporalOffset = mTemporalOffset;
    to_frame->mKeyFrameOffset = mKeyFrameOffset;
    to_frame->mFlags = mFlags;
    to_frame->mCPFilePosition = mCPFilePosition;
    to_frame->mFilePosition = mFilePosition;
}



MXFDefaultFrame::MXFDefaultFrame()
: MXFFrame()
{
}

MXFDefaultFrame::~MXFDefaultFrame()
{
}

uint32_t MXFDefaultFrame::GetSize() const
{
    return mData.GetSize();
}

const unsigned char* MXFDefaultFrame::GetBytes() const
{
    return mData.GetBytes();
}

void MXFDefaultFrame::Grow(uint32_t min_size)
{
    mData.Grow(min_size);
}

uint32_t MXFDefaultFrame::GetSizeAvailable() const
{
    return mData.GetSizeAvailable();
}

unsigned char* MXFDefaultFrame::GetBytesAvailable() const
{
    return mData.GetBytesAvailable();
}

void MXFDefaultFrame::SetSize(uint32_t size)
{
    mData.SetSize(size);
}

void MXFDefaultFrame::IncrementSize(uint32_t inc)
{
    mData.IncrementSize(inc);
}

void MXFDefaultFrame::Reset()
{
    MXFFrame::Reset();
    mData.SetSize(0);
}

