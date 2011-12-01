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

#ifndef __IM_MXF_FRAME_BUFFER_H__
#define __IM_MXF_FRAME_BUFFER_H__

#include <im/frame/FrameBuffer.h>



namespace im
{


class MXFFrameBuffer : public FrameBuffer
{
public:
    MXFFrameBuffer();
    virtual ~MXFFrameBuffer();

    void SetTargetBuffer(FrameBuffer *target_buffer, bool take_ownership);

    void SetNextFramePosition(int64_t position);
    void SetNextFrameTrackPosition(int64_t position);

public:
    virtual void SetFrameFactory(FrameFactory *frame_factory, bool take_ownership);
    virtual Frame* CreateFrame();
    virtual void PushFrame(Frame *frame);
    virtual void PopFrame(bool del_frame);
    virtual Frame* GetLastFrame(bool pop);
    virtual size_t GetNumFrames() const;
    virtual void Clear(bool del_frames);

private:
    FrameBuffer *mTargetBuffer;
    bool mOwnTargetBuffer;
    int64_t mNextFramePosition;
    int64_t mNextFrameTrackPosition;
};


};



#endif

