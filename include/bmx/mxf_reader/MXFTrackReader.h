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

#ifndef BMX_MXF_TRACK_READER_H_
#define BMX_MXF_TRACK_READER_H_

#include <vector>

#include <libMXF++/MXF.h>

#include <bmx/mxf_reader/MXFFrameBuffer.h>
#include <bmx/mxf_reader/MXFTrackInfo.h>
#include <bmx/mxf_reader/MXFIndexEntryExt.h>


#define CURRENT_POSITION_VALUE      (int64_t)(((uint64_t)1)<<63)



namespace bmx
{


class MXFTrackReader
{
public:
    virtual ~MXFTrackReader() {}

    virtual void SetEmptyFrames(bool enable) = 0;

    virtual void SetEnable(bool enable) = 0;

    virtual void SetFrameBuffer(FrameBuffer *frame_buffer, bool take_ownership) = 0;

    virtual void GetReadLimits(bool limit_to_available, int64_t *start_position, int64_t *duration) const = 0;
    virtual void SetReadLimits() = 0;
    virtual void SetReadLimits(int64_t start_position, int64_t duration, bool seek_start_position) = 0;

public:
    virtual std::vector<size_t> GetFileIds(bool internal_ess_only) const = 0;

    virtual int64_t GetReadStartPosition() const = 0;
    virtual int64_t GetReadDuration() const = 0;

    virtual uint32_t Read(uint32_t num_samples, bool is_top = true) = 0;
    virtual bool ReadError() const = 0;
    virtual std::string ReadErrorMessage() const = 0;
    virtual void Seek(int64_t position) = 0;

    virtual int64_t GetPosition() const = 0;
    virtual mxfRational GetEditRate() const = 0;
    virtual int64_t GetDuration() const = 0;
    virtual int64_t GetOrigin() const = 0;

    virtual bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position = CURRENT_POSITION_VALUE) const = 0;

    virtual int16_t GetPrecharge(int64_t position, bool limit_to_available) const = 0;
    virtual int16_t GetRollout(int64_t position, bool limit_to_available) const = 0;

    virtual MXFTrackInfo* GetTrackInfo() const = 0;
    virtual mxfpp::FileDescriptor* GetFileDescriptor() const = 0;
    virtual mxfpp::SourcePackage* GetFileSourcePackage() const = 0;

    virtual size_t GetTrackIndex() const = 0;

    virtual bool HaveAVCIHeader() const = 0;
    virtual const unsigned char* GetAVCIHeader() const = 0;

public:
    virtual bool IsEnabled() const = 0;
    virtual FrameBuffer* GetFrameBuffer() = 0;

public:
    virtual MXFFrameBuffer* GetMXFFrameBuffer() = 0;
    virtual void SetNextFramePosition(Rational edit_rate, int64_t position) = 0;
};


};



#endif

