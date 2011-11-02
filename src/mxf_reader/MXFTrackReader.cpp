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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <im/mxf_reader/MXFTrackReader.h>
#include <im/mxf_reader/MXFFileReader.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



MXFTrackReader::MXFTrackReader(MXFFileReader *file_reader, MXFTrackInfo *track_info, FileDescriptor *file_descriptor,
                               SourcePackage *file_source_package)
{
    mFileReader = file_reader;
    mTrackInfo = track_info;
    mFileDescriptor = file_descriptor;
    mFileSourcePackage = file_source_package;

    mIsEnabled = true;
    mFrameBuffer = new MXFDefaultFrameBuffer();
    mOwnFrameBuffer = true;
}

MXFTrackReader::~MXFTrackReader()
{
    delete mTrackInfo;
    if (mOwnFrameBuffer)
        delete mFrameBuffer;
}

void MXFTrackReader::SetEnable(bool enable)
{
    mIsEnabled = enable;
}

void MXFTrackReader::SetFrameBuffer(MXFFrameBuffer *frame_buffer, bool take_ownership)
{
    if (mOwnFrameBuffer)
        delete mFrameBuffer;

    mFrameBuffer = frame_buffer;
    mOwnFrameBuffer = take_ownership;
}

void MXFTrackReader::SetReadLimits()
{
    mFileReader->SetReadLimits();
}

void MXFTrackReader::SetReadLimits(int64_t start_position, int64_t end_position, bool seek_to_start)
{
    mFileReader->SetReadLimits(start_position, end_position, seek_to_start);
}

int64_t MXFTrackReader::GetReadStartPosition()
{
    return mFileReader->GetReadStartPosition();
}

int64_t MXFTrackReader::GetReadEndPosition()
{
    return mFileReader->GetReadEndPosition();
}

int64_t MXFTrackReader::GetReadDuration()
{
    return mFileReader->GetReadDuration();
}

uint32_t MXFTrackReader::Read(uint32_t num_samples, int64_t frame_position)
{
    return mFileReader->Read(num_samples, frame_position);
}

void MXFTrackReader::Seek(int64_t position)
{
    mFileReader->Seek(position);
}

int64_t MXFTrackReader::GetPosition()
{
    return mFileReader->GetPosition();
}

mxfRational MXFTrackReader::GetSampleRate()
{
    return mFileReader->GetSampleRate();
}

int64_t MXFTrackReader::GetDuration()
{
    return mFileReader->GetDuration();
}

bool MXFTrackReader::GetIndexEntry(MXFIndexEntryExt *entry, int64_t position)
{
    return mFileReader->GetInternalIndexEntry(entry, position);
}

int16_t MXFTrackReader::GetPrecharge(int64_t position, bool limit_to_available)
{
    return mFileReader->GetInternalPrecharge(position, limit_to_available);
}

int16_t MXFTrackReader::GetRollout(int64_t position, bool limit_to_available)
{
    return mFileReader->GetInternalRollout(position, limit_to_available);
}

void MXFTrackReader::Reset(int64_t position)
{
    mFrameBuffer->ResetFrame(position);
}

