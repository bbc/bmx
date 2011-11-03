/*
 * Copyright (C) 2011  British Broadcasting Corporation.
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifndef __IM_MXF_TRACK_READER_H__
#define __IM_MXF_TRACK_READER_H__


#include <libMXF++/MXF.h>

#include <im/mxf_reader/MXFFrameBuffer.h>
#include <im/mxf_reader/MXFTrackInfo.h>
#include <im/mxf_reader/MXFIndexEntryExt.h>


#define CURRENT_POSITION_VALUE      (int64_t)(((uint64_t)1)<<63)



namespace im
{


class MXFTrackReader
{
public:
    virtual ~MXFTrackReader() {}

    virtual void SetEnable(bool enable) = 0;

    virtual void SetFrameBuffer(MXFFrameBuffer *frame_buffer, bool take_ownership) = 0;

    virtual void SetReadLimits() = 0;
    virtual void SetReadLimits(int64_t start_position, int64_t end_position, bool seek_start_position) = 0;

public:
    virtual int64_t GetReadStartPosition() const = 0;
    virtual int64_t GetReadEndPosition() const = 0;
    virtual int64_t GetReadDuration() const = 0;

    virtual uint32_t Read(uint32_t num_samples, int64_t frame_position = CURRENT_POSITION_VALUE) = 0;
    virtual void Seek(int64_t position) = 0;

    virtual int64_t GetPosition() const = 0;
    virtual mxfRational GetSampleRate() const = 0;
    virtual int64_t GetDuration() const = 0;

    virtual bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position = CURRENT_POSITION_VALUE) const = 0;

    virtual int16_t GetPrecharge(int64_t position, bool limit_to_available) const = 0;
    virtual int16_t GetRollout(int64_t position, bool limit_to_available) const = 0;

    virtual MXFTrackInfo* GetTrackInfo() const = 0;
    virtual mxfpp::FileDescriptor* GetFileDescriptor() const = 0;
    virtual mxfpp::SourcePackage* GetFileSourcePackage() const = 0;

public:
    virtual bool IsEnabled() const = 0;

    virtual MXFFrameBuffer* GetFrameBuffer() = 0;
    virtual MXFFrame* GetFrame(int64_t position) = 0;

    virtual void Reset(int64_t position) = 0;
};


};



#endif

