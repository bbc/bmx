/*
 * Copyright (C) 2010  British Broadcasting Corporation.
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

#ifndef __IM_MXF_FILE_TRACK_READER_H__
#define __IM_MXF_FILE_TRACK_READER_H__


#include <im/mxf_reader/MXFTrackReader.h>



namespace im
{

class MXFFileReader;

class MXFFileTrackReader : public MXFTrackReader
{
public:
    MXFFileTrackReader(MXFFileReader *file_reader, MXFTrackInfo *track_info, mxfpp::FileDescriptor *file_descriptor,
                       mxfpp::SourcePackage *file_source_package);
    virtual ~MXFFileTrackReader();

    virtual void SetEnable(bool enable);
    virtual void SetFrameBuffer(MXFFrameBuffer *frame_buffer, bool take_ownership);

public:
    virtual void SetReadLimits();
    virtual void SetReadLimits(int64_t start_position, int64_t end_position, bool seek_start_position);
    virtual int64_t GetReadStartPosition();
    virtual int64_t GetReadEndPosition();
    virtual int64_t GetReadDuration();

    virtual uint32_t Read(uint32_t num_samples, int64_t frame_position = CURRENT_POSITION_VALUE);
    virtual void Seek(int64_t position);

    virtual int64_t GetPosition();
    virtual mxfRational GetSampleRate();
    virtual int64_t GetDuration();

    virtual bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position = CURRENT_POSITION_VALUE);

    virtual int16_t GetPrecharge(int64_t position, bool limit_to_available);
    virtual int16_t GetRollout(int64_t position, bool limit_to_available);

    virtual MXFTrackInfo* GetTrackInfo() const { return mTrackInfo; }
    virtual mxfpp::FileDescriptor* GetFileDescriptor() const { return mFileDescriptor; }
    virtual mxfpp::SourcePackage* GetFileSourcePackage() const { return mFileSourcePackage; }

public:
    virtual bool IsEnabled() const { return mIsEnabled; }
    virtual MXFFrameBuffer* GetFrameBuffer() { return mFrameBuffer; }
    virtual MXFFrame* GetFrame(int64_t position) { return mFrameBuffer->GetFrame(position); }

    virtual void Reset(int64_t position);

private:
    MXFFileReader *mFileReader;
    MXFTrackInfo *mTrackInfo;
    mxfpp::FileDescriptor *mFileDescriptor;
    mxfpp::SourcePackage *mFileSourcePackage;

    bool mIsEnabled;

    MXFFrameBuffer *mFrameBuffer;
    bool mOwnFrameBuffer;
};


};



#endif

