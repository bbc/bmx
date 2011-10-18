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

#ifndef __IM_MXF_TRACK_READER_H__
#define __IM_MXF_TRACK_READER_H__


#include <libMXF++/MXF.h>

#include <im/mxf_reader/MXFFrameBuffer.h>
#include <im/mxf_reader/MXFTrackInfo.h>
#include <im/mxf_reader/MXFIndexEntryExt.h>


#define CURRENT_POSITION_VALUE      (int64_t)(((uint64_t)1)<<63)



namespace im
{

class MXFReader;

class MXFTrackReader
{
public:
    MXFTrackReader(MXFReader *file_reader, MXFTrackInfo *track_info, mxfpp::FileDescriptor *file_descriptor,
                   mxfpp::SourcePackage *file_source_package);
    ~MXFTrackReader();

    void SetEnable(bool enable);
    void SetFrameBuffer(MXFFrameBuffer *frame_buffer, bool take_ownership);

    MXFReader* GetFileReader() { return mFileReader; }

public:
    void SetReadLimits();
    void SetReadLimits(int64_t start_position, int64_t end_position, bool seek_start_position);
    int64_t GetReadStartPosition();
    int64_t GetReadEndPosition();
    int64_t GetReadDuration();

    uint32_t Read(uint32_t num_samples, int64_t frame_position = CURRENT_POSITION_VALUE);
    void Seek(int64_t position);

    int64_t GetPosition();
    mxfRational GetSampleRate();
    int64_t GetDuration();

    bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position = CURRENT_POSITION_VALUE);

    int16_t GetPrecharge(int64_t position, bool limit_to_available);
    int16_t GetRollout(int64_t position, bool limit_to_available);

    MXFTrackInfo* GetTrackInfo() const { return mTrackInfo; }
    mxfpp::FileDescriptor* GetFileDescriptor() const { return mFileDescriptor; }
    mxfpp::SourcePackage* GetFileSourcePackage() const { return mFileSourcePackage; }

public:
    bool IsEnabled() const { return mIsEnabled; }
    MXFFrameBuffer* GetFrameBuffer() { return mFrameBuffer; }
    MXFFrame* GetFrame(int64_t position) { return mFrameBuffer->GetFrame(position); }

    void Reset(int64_t position);

private:
    MXFReader *mFileReader;
    MXFTrackInfo *mTrackInfo;
    mxfpp::FileDescriptor *mFileDescriptor;
    mxfpp::SourcePackage *mFileSourcePackage;

    bool mIsEnabled;

    MXFFrameBuffer *mFrameBuffer;
    bool mOwnFrameBuffer;
};


};



#endif

