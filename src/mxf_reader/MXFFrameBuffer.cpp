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

#include <bmx/mxf_reader/MXFFrameBuffer.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



MXFFrameBuffer::MXFFrameBuffer()
{
    mTargetBuffer = 0;
    mOwnTargetBuffer = false;
    mNextFramePosition = NULL_FRAME_POSITION;
    mNextFrameTrackPosition = NULL_FRAME_POSITION;
    mUseTemporaryBuffer = false;
}

MXFFrameBuffer::~MXFFrameBuffer()
{
    if (mOwnTargetBuffer)
        delete mTargetBuffer;
    mTemporaryBuffer.Clear(true);
}

void MXFFrameBuffer::SetTargetBuffer(FrameBuffer *target_buffer, bool take_ownership)
{
    if (mOwnTargetBuffer)
        delete mTargetBuffer;

    mTargetBuffer = target_buffer;
    mOwnTargetBuffer = take_ownership;
}

void MXFFrameBuffer::SetNextFramePosition(int64_t position)
{
    mNextFramePosition = position;

    if (!mUseTemporaryBuffer) {
        MXFFrameBuffer *target_mxf_buffer = dynamic_cast<MXFFrameBuffer*>(mTargetBuffer);
        if (target_mxf_buffer)
            target_mxf_buffer->SetNextFramePosition(position);
    }
}

void MXFFrameBuffer::SetNextFrameTrackPosition(int64_t position)
{
    mNextFrameTrackPosition = position;

    if (!mUseTemporaryBuffer) {
        MXFFrameBuffer *target_mxf_buffer = dynamic_cast<MXFFrameBuffer*>(mTargetBuffer);
        if (target_mxf_buffer)
            target_mxf_buffer->SetNextFrameTrackPosition(position);
    }
}

void MXFFrameBuffer::SetTemporaryBuffer(bool enable)
{
    mTemporaryBuffer.Clear(true);
    mUseTemporaryBuffer = enable;
}

void MXFFrameBuffer::SetFrameFactory(FrameFactory *frame_factory, bool take_ownership)
{
    mTargetBuffer->SetFrameFactory(frame_factory, take_ownership);
}

Frame* MXFFrameBuffer::CreateFrame()
{
    if (mUseTemporaryBuffer)
        return mTemporaryBuffer.CreateFrame();
    else
        return mTargetBuffer->CreateFrame();
}

void MXFFrameBuffer::PushFrame(Frame *frame)
{
    frame->position = mNextFramePosition;
    frame->track_position = mNextFrameTrackPosition;

    if (mUseTemporaryBuffer)
        mTemporaryBuffer.PushFrame(frame);
    else
        mTargetBuffer->PushFrame(frame);
}

void MXFFrameBuffer::PopFrame(bool del_frame)
{
    if (mUseTemporaryBuffer)
        mTemporaryBuffer.PopFrame(del_frame);
    else
        mTargetBuffer->PopFrame(del_frame);
}

Frame* MXFFrameBuffer::GetLastFrame(bool pop)
{
    if (mUseTemporaryBuffer)
        return mTemporaryBuffer.GetLastFrame(pop);
    else
        return mTargetBuffer->GetLastFrame(pop);
}

size_t MXFFrameBuffer::GetNumFrames() const
{
    if (mUseTemporaryBuffer)
        return mTemporaryBuffer.GetNumFrames();
    else
        return mTargetBuffer->GetNumFrames();
}

void MXFFrameBuffer::Clear(bool del_frames)
{
    if (mUseTemporaryBuffer)
        mTemporaryBuffer.Clear(del_frames);
    else
        mTargetBuffer->Clear(del_frames);
}

