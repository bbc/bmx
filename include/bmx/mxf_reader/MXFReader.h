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

#ifndef __BMX_MXF_READER_H__
#define __BMX_MXF_READER_H__


#include <libMXF++/MXF.h>

#include <bmx/BMXTypes.h>
#include <bmx/mxf_reader/MXFTrackReader.h>
#include <bmx/mxf_reader/MXFIndexEntryExt.h>



namespace bmx
{


class MXFGroupReader;

class MXFReader
{
public:
    friend class MXFGroupReader;

public:
    MXFReader();
    virtual ~MXFReader();

public:
    virtual void SetReadLimits() = 0;
    virtual void SetReadLimits(int64_t start_position, int64_t duration, bool seek_start_position) = 0;
    virtual int64_t GetReadStartPosition() const = 0;
    virtual int64_t GetReadDuration() const = 0;

    virtual uint32_t Read(uint32_t num_samples, bool is_top = true) = 0;

    virtual void Seek(int64_t position) = 0;
    void ClearFrameBuffers(bool del_frames);

    virtual int64_t GetPosition() const = 0;

    virtual int16_t GetMaxPrecharge(int64_t position, bool limit_to_available) const = 0;
    virtual int16_t GetMaxRollout(int64_t position, bool limit_to_available) const = 0;

    mxfRational GetSampleRate() const { return mSampleRate; }
    int64_t GetDuration() const       { return mDuration; }

public:
    virtual bool HaveFixedLeadFillerOffset() const = 0;
    virtual int64_t GetFixedLeadFillerOffset() const = 0;

    bool HaveMaterialTimecode() const { return mMaterialStartTimecode != 0; }
    Timecode GetMaterialTimecode(int64_t position = CURRENT_POSITION_VALUE) const;
    bool HaveFileSourceTimecode() const { return mFileSourceStartTimecode != 0; }
    Timecode GetFileSourceTimecode(int64_t position = CURRENT_POSITION_VALUE) const;
    bool HavePhysicalSourceTimecode() const { return mPhysicalSourceStartTimecode != 0; }
    Timecode GetPhysicalSourceTimecode(int64_t position = CURRENT_POSITION_VALUE) const;

    bool HavePlayoutTimecode() const;
    Timecode GetPlayoutTimecode(int64_t position = CURRENT_POSITION_VALUE) const;
    bool HaveSourceTimecode() const;
    Timecode GetSourceTimecode(int64_t position = CURRENT_POSITION_VALUE) const;

    std::string GetMaterialPackageName() const       { return mMaterialPackageName; }
    mxfUMID GetMaterialPackageUID() const            { return mMaterialPackageUID; }
    std::string GetPhysicalSourcePackageName() const { return mPhysicalSourcePackageName; }

public:
    virtual size_t GetNumTrackReaders() const = 0;
    virtual MXFTrackReader* GetTrackReader(size_t track_index) const = 0;

    virtual bool IsEnabled() const = 0;

public:
    virtual void SetNextFramePosition(int64_t position) = 0;
    virtual void SetNextFrameTrackPositions() = 0;

protected:
    mxfRational mSampleRate;
    int64_t mDuration;

    Timecode *mMaterialStartTimecode;
    Timecode *mFileSourceStartTimecode;
    Timecode *mPhysicalSourceStartTimecode;

    std::string mMaterialPackageName;
    mxfUMID mMaterialPackageUID;
    std::string mPhysicalSourcePackageName;

private:
    Timecode CreateTimecode(const Timecode *start_timecode, int64_t position) const;
};


};



#endif

