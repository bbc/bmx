/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#include <bmx/frame/FrameBuffer.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;

#define NULL_READ_START_INDEX   (size_t)(-1)



DefaultFrameBuffer::DefaultFrameBuffer()
{
    mFrameFactory = new DefaultFrameFactory();
    mOwnFrameFactory = true;
    mStartReadIndex = NULL_READ_START_INDEX;
}

DefaultFrameBuffer::DefaultFrameBuffer(FrameFactory *frame_factory, bool take_ownership)
{
    mFrameFactory = frame_factory;
    mOwnFrameFactory = take_ownership;
    mStartReadIndex = NULL_READ_START_INDEX;
}

DefaultFrameBuffer::~DefaultFrameBuffer()
{
    if (mOwnFrameFactory)
        delete mFrameFactory;

    Clear(true);
}

void DefaultFrameBuffer::SetFrameFactory(FrameFactory *frame_factory, bool take_ownership)
{
    if (mOwnFrameFactory)
        delete mFrameFactory;

    mFrameFactory = frame_factory;
    mOwnFrameFactory = take_ownership;
}

void DefaultFrameBuffer::StartRead()
{
    mStartReadIndex = mFrames.size();
}

void DefaultFrameBuffer::CompleteRead()
{
    mStartReadIndex = NULL_READ_START_INDEX;
}

void DefaultFrameBuffer::AbortRead()
{
    if (mStartReadIndex != NULL_READ_START_INDEX && mStartReadIndex < mFrames.size()) {
        size_t num_pops = mFrames.size() - mStartReadIndex;
        size_t i;
        for (i = 0; i < num_pops; i++)
            PopFrame(true);
    }

    mStartReadIndex = NULL_READ_START_INDEX;
}

Frame* DefaultFrameBuffer::CreateFrame()
{
    return mFrameFactory->CreateFrame();
}

void DefaultFrameBuffer::PushFrame(Frame *frame)
{
    mFrames.push_back(frame);
}

void DefaultFrameBuffer::PopFrame(bool del_frame)
{
    if (del_frame && mFrames.front())
        delete mFrames.front();

    mFrames.pop_front();
}

Frame* DefaultFrameBuffer::GetLastFrame(bool pop)
{
    if (mFrames.empty())
        return 0;

    Frame *frame = mFrames.front();
    if (pop)
        mFrames.pop_front();

    return frame;
}

size_t DefaultFrameBuffer::GetNumFrames() const
{
    return mFrames.size();
}

void DefaultFrameBuffer::Clear(bool del_frames)
{
    if (del_frames) {
        size_t i;
        for (i = 0; i < mFrames.size(); i++)
            delete mFrames[i];
    }

    mFrames.clear();
    mStartReadIndex = NULL_READ_START_INDEX;
}

