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

#ifndef BMX_MXF_FRAME_BUFFER_H_
#define BMX_MXF_FRAME_BUFFER_H_

#include <bmx/frame/FrameBuffer.h>



namespace bmx
{


class MXFFrameBuffer : public FrameBuffer
{
public:
    MXFFrameBuffer();
    virtual ~MXFFrameBuffer();

    void SetEmptyFrames(bool enable);

    void SetTargetBuffer(FrameBuffer *target_buffer, bool take_ownership);

    void SetNextFramePosition(Rational edit_rate, int64_t position);
    void SetNextFrameTrackPosition(Rational edit_rate, int64_t position);

    void SetTemporaryBuffer(bool enable);

public:
    virtual void SetFrameFactory(FrameFactory *frame_factory, bool take_ownership);
    virtual void StartRead();
    virtual void CompleteRead();
    virtual void AbortRead();
    virtual Frame* CreateFrame();
    virtual void PushFrame(Frame *frame);
    virtual void PopFrame(bool del_frame);
    virtual Frame* GetLastFrame(bool pop);
    virtual size_t GetNumFrames() const;
    virtual void Clear(bool del_frames);

private:
    bool mEmptyFrames;
    FrameBuffer *mTargetBuffer;
    bool mOwnTargetBuffer;
    Rational mNextFrameEditRate;
    int64_t mNextFramePosition;
    Rational mNextFrameTrackEditRate;
    int64_t mNextFrameTrackPosition;
    DefaultFrameBuffer mTemporaryBuffer;
    bool mUseTemporaryBuffer;
};


};



#endif

