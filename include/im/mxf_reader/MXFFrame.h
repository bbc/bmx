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

#ifndef __IM_MXF_FRAME_H__
#define __IM_MXF_FRAME_H__


#include <im/ByteArray.h>



namespace im
{


class MXFFrame
{
public:
    MXFFrame();
    virtual ~MXFFrame();

public:
    // read

    bool IsEmpty() const { return mNumSamples == 0; }

    virtual uint32_t GetSize() const = 0;
    virtual const unsigned char* GetBytes() const = 0;

    int64_t GetPosition() const { return mPosition; }
    uint32_t GetFirstSampleOffset() const { return mFirstSampleOffset; }
    uint32_t GetNumSamples() const { return mNumSamples; }

    bool GetTemporalReordering() const { return mTemporalReordering; }
    int8_t GetTemporalOffset() const { return mTemporalOffset; }
    int8_t GetKeyFrameOffset() const { return mKeyFrameOffset; }
    uint8_t GetFlags() const { return mFlags; }

    int64_t GetCPFilePosition() const { return mCPFilePosition; }
    int64_t GetFilePosition() const { return mFilePosition; }

public:
    // write

    virtual void Grow(uint32_t min_size) = 0;
    virtual uint32_t GetSizeAvailable() const = 0;
    virtual unsigned char* GetBytesAvailable() const = 0;
    virtual void SetSize(uint32_t size) = 0;
    virtual void IncrementSize(uint32_t inc) = 0;
    virtual void Complete() = 0;

    virtual void Reset();

    void SetPosition(int64_t position);
    void SetFirstSampleOffset(uint32_t offset);
    void SetNumSamples(uint32_t num_samples);
    void IncNumSamples(uint32_t num_samples);

    bool TemporalReorderingWasSet() { return mTemporalReorderingWasSet; }
    void SetTemporalReordering(bool reorder);
    void SetTemporalOffset(int8_t offset);
    void SetKeyFrameOffset(int8_t offset);
    void SetFlags(uint8_t flags);

    void SetCPFilePosition(int64_t position);
    void SetFilePosition(int64_t position);

    void CopyMetadataTo(MXFFrame *to_frame) const;

private:
    int64_t mPosition;
    uint32_t mFirstSampleOffset;
    uint32_t mNumSamples;

    bool mTemporalReordering;
    bool mTemporalReorderingWasSet;
    int8_t mTemporalOffset;
    int8_t mKeyFrameOffset;
    uint8_t mFlags;

    int64_t mCPFilePosition;
    int64_t mFilePosition;
};



class MXFDefaultFrame : public MXFFrame
{
public:
    MXFDefaultFrame();
    virtual ~MXFDefaultFrame();

public:
    virtual uint32_t GetSize() const;
    virtual const unsigned char* GetBytes() const;

public:
    virtual void Grow(uint32_t min_size);
    virtual uint32_t GetSizeAvailable() const;
    virtual unsigned char* GetBytesAvailable() const;
    virtual void SetSize(uint32_t size);
    virtual void IncrementSize(uint32_t inc);
    virtual void Complete() {};

    virtual void Reset();

protected:
    ByteArray mData;
};


};



#endif

