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

#ifndef BMX_MXF_SEQUENCE_TRACK_READER_H_
#define BMX_MXF_SEQUENCE_TRACK_READER_H_


#include <bmx/mxf_reader/MXFTrackReader.h>



namespace bmx
{

class MXFSequenceReader;

class MXFSequenceTrackReader : public MXFTrackReader
{
public:
    friend class MXFSequenceReader;

public:
    MXFSequenceTrackReader(MXFSequenceReader *sequence_reader, size_t track_index);
    virtual ~MXFSequenceTrackReader();

    virtual void SetEmptyFrames(bool enable);

    bool IsCompatible(MXFTrackReader *segment) const;
    void AppendSegment(MXFTrackReader *segment);

    size_t GetNumSegments() const { return mTrackSegments.size(); }
    MXFTrackReader* GetSegment(size_t index);

    void UpdateReadLimits();

public:
    virtual void SetEnable(bool enable);
    virtual void SetFrameBuffer(FrameBuffer *frame_buffer, bool take_ownership);

public:
    virtual std::vector<size_t> GetFileIds(bool internal_ess_only) const;

    virtual void GetReadLimits(bool limit_to_available, int64_t *start_position, int64_t *duration) const;
    virtual void SetReadLimits();
    virtual void SetReadLimits(int64_t start_position, int64_t duration, bool seek_start_position);
    virtual int64_t GetReadStartPosition() const { return mReadStartPosition; }
    virtual int64_t GetReadDuration() const      { return mReadDuration; }

    virtual uint32_t Read(uint32_t num_samples, bool is_top = true);
    virtual bool ReadError() const               { return mReadError; }
    virtual std::string ReadErrorMessage() const { return mReadErrorMessage; }
    virtual void Seek(int64_t position);

    virtual int64_t GetPosition() const       { return mPosition; }
    virtual mxfRational GetEditRate() const   { return mEditRate; }
    virtual int64_t GetDuration() const       { return mDuration; }
    virtual int64_t GetOrigin() const         { return mOrigin; }

    virtual bool GetIndexEntry(MXFIndexEntryExt *entry, int64_t position = CURRENT_POSITION_VALUE) const;

    virtual int16_t GetPrecharge(int64_t position, bool limit_to_available) const;
    virtual int16_t GetRollout(int64_t position, bool limit_to_available) const;

    virtual MXFTrackInfo* GetTrackInfo() const                 { return mTrackInfo; }
    virtual mxfpp::FileDescriptor* GetFileDescriptor() const   { return mFileDescriptor; }
    virtual mxfpp::SourcePackage* GetFileSourcePackage() const;

    virtual size_t GetTrackIndex() const { return mTrackIndex; }

    virtual bool HaveAVCIHeader() const;
    virtual const unsigned char* GetAVCIHeader() const;

public:
    virtual bool IsEnabled() const        { return mIsEnabled; }
    virtual FrameBuffer* GetFrameBuffer() { return &mFrameBuffer; }

public:
    virtual MXFFrameBuffer* GetMXFFrameBuffer() { return &mFrameBuffer; }
    virtual void SetNextFramePosition(Rational edit_rate, int64_t position);

private:
    void GetSegmentPosition(int64_t position, MXFTrackReader **segment, int64_t *segment_position) const;

    void UpdatePosition(size_t segment_index);

private:
    MXFSequenceReader *mSequenceReader;
    size_t mTrackIndex;
    MXFTrackInfo *mTrackInfo;
    mxfpp::FileDescriptor *mFileDescriptor;
    mxfpp::SourcePackage *mFileSourcePackage;

    bool mEmptyFrames;
    bool mEmptyFramesSet;

    bool mIsEnabled;

    int64_t mReadStartPosition;
    int64_t mReadDuration;

    mxfRational mEditRate;
    int64_t mPosition;
    int64_t mDuration;
    int64_t mOrigin;

    bool mReadError;
    std::string mReadErrorMessage;

    std::vector<MXFTrackReader*> mTrackSegments;
    std::vector<int64_t> mSegmentOffsets;

    MXFFrameBuffer mFrameBuffer;
};


};



#endif

