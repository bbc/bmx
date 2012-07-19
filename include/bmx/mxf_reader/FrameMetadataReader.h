/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_FRAME_METADATA_READER_H_
#define BMX_FRAME_METADATA_READER_H_


#include <bmx/mxf_reader/MXFFrameMetadata.h>



namespace mxfpp
{
    class File;
};


namespace bmx
{

class MXFFileReader;


class FrameMetadataChildReader
{
public:
    virtual ~FrameMetadataChildReader() {}

    virtual void Reset() = 0;
    virtual bool ProcessFrameMetadata(const mxfKey *key, uint64_t len) = 0;
    virtual void InsertFrameMetadata(Frame *frame, uint32_t track_number) = 0;
};


class SystemScheme1Reader : public FrameMetadataChildReader
{
public:
    SystemScheme1Reader(mxfpp::File *file, Rational frame_rate, bool is_bbc_preservation_file);
    virtual ~SystemScheme1Reader();

    virtual void Reset();
    virtual bool ProcessFrameMetadata(const mxfKey *key, uint64_t len);
    virtual void InsertFrameMetadata(Frame *frame, uint32_t track_number);

private:
    mxfpp::File *mFile;
    Rational mFrameRate;
    bool mIsBBCPreservationFile;

    SS1TimecodeArray *mTimecodeArray;

    std::vector<uint32_t> mCRC32s;
    std::vector<uint32_t> mTrackNumbers;
};


class SDTICPSystemMetadataReader : public FrameMetadataChildReader
{
public:
    SDTICPSystemMetadataReader(mxfpp::File *file);
    virtual ~SDTICPSystemMetadataReader();

    virtual void Reset();
    virtual bool ProcessFrameMetadata(const mxfKey *key, uint64_t len);
    virtual void InsertFrameMetadata(Frame *frame, uint32_t track_number);

private:
    mxfpp::File *mFile;
    SDTICPSystemMetadata *mMetadata;
};


class SDTICPPackageMetadataReader : public FrameMetadataChildReader
{
public:
    SDTICPPackageMetadataReader(mxfpp::File *file);
    virtual ~SDTICPPackageMetadataReader();

    virtual void Reset();
    virtual bool ProcessFrameMetadata(const mxfKey *key, uint64_t len);
    virtual void InsertFrameMetadata(Frame *frame, uint32_t track_number);

private:
    mxfpp::File *mFile;
    SDTICPPackageMetadata *mMetadata;
};


class FrameMetadataReader
{
public:
    FrameMetadataReader(MXFFileReader *file_reader);
    ~FrameMetadataReader();

    void Reset();
    bool ProcessFrameMetadata(const mxfKey *key, uint64_t len);
    void InsertFrameMetadata(Frame *frame, uint32_t track_number);

private:
    std::vector<FrameMetadataChildReader*> mReaders;
};


};



#endif

