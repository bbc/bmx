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

#ifndef __IM_MXF_CLIP_READER_H__
#define __IM_MXF_CLIP_READER_H__


#include <libMXF++/MXF.h>

#include <im/IMTypes.h>
#include <im/mxf_reader/MXFTrackReader.h>
#include <im/mxf_reader/MXFIndexEntryExt.h>



namespace im
{


class MXFReader;

class MXFClipReader
{
public:
    friend class MXFReader;

public:
    MXFClipReader();
    virtual ~MXFClipReader();

public:
    virtual void SetReadLimits() = 0;
    virtual void SetReadLimits(int64_t start_position, int64_t end_position, bool seek_start_position) = 0;
    virtual int64_t GetReadStartPosition() = 0;
    virtual int64_t GetReadEndPosition() = 0;
    virtual int64_t GetReadDuration() = 0;

    virtual uint32_t Read(uint32_t num_samples, int64_t frame_position = CURRENT_POSITION_VALUE) = 0;
    virtual void Seek(int64_t position) = 0;

    virtual int64_t GetPosition() = 0;

    virtual int16_t GetMaxPrecharge(int64_t position, bool limit_to_available) = 0;
    virtual int16_t GetMaxRollout(int64_t position, bool limit_to_available) = 0;

    mxfRational GetSampleRate() { return mSampleRate; }
    int64_t GetDuration() { return mDuration; }

public:
    virtual bool HaveFixedLeadFillerOffset() = 0;
    virtual int64_t GetFixedLeadFillerOffset() = 0;

    bool HaveMaterialTimecode() const { return mMaterialStartTimecode != 0; }
    Timecode GetMaterialTimecode(int64_t position = CURRENT_POSITION_VALUE);
    bool HaveFileSourceTimecode() const { return mFileSourceStartTimecode != 0; }
    Timecode GetFileSourceTimecode(int64_t position = CURRENT_POSITION_VALUE);
    bool HavePhysicalSourceTimecode() const { return mPhysicalSourceStartTimecode != 0; }
    Timecode GetPhysicalSourceTimecode(int64_t position = CURRENT_POSITION_VALUE);

    bool HavePlayoutTimecode() const;
    Timecode GetPlayoutTimecode(int64_t position = CURRENT_POSITION_VALUE);
    bool HaveSourceTimecode() const;
    Timecode GetSourceTimecode(int64_t position = CURRENT_POSITION_VALUE);

public:
    virtual size_t GetNumTrackReaders() const = 0;
    virtual MXFTrackReader* GetTrackReader(size_t track_index) const = 0;

    virtual bool IsEnabled() const = 0;

protected:
    mxfRational mSampleRate;
    int64_t mDuration;

    Timecode *mMaterialStartTimecode;
    Timecode *mFileSourceStartTimecode;
    Timecode *mPhysicalSourceStartTimecode;

private:
    Timecode CreateTimecode(const Timecode *start_timecode, int64_t position);
};


};



#endif

