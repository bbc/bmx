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

#ifndef BMX_FRAME_H_
#define BMX_FRAME_H_


#include <map>
#include <vector>
#include <string>

#include <bmx/BMXTypes.h>
#include <bmx/ByteArray.h>


#define NULL_FRAME_POSITION     (int64_t)(((uint64_t)1)<<63)



namespace bmx
{


class FrameMetadata
{
public:
    FrameMetadata(const char *id);
    virtual ~FrameMetadata();

    const char* GetId() const { return mId; }

protected:
    const char *mId;
};


class Frame
{
public:
    Frame();
    virtual ~Frame();

    virtual uint32_t GetSize() const = 0;
    virtual const unsigned char* GetBytes() const = 0;

    virtual void Grow(uint32_t min_size) = 0;
    virtual uint32_t GetSizeAvailable() const = 0;
    virtual unsigned char* GetBytesAvailable() const = 0;
    virtual void SetSize(uint32_t size) = 0;
    virtual void IncrementSize(uint32_t inc) = 0;

public:
    bool IsEmpty() const    { return num_samples == 0; }
    bool IsComplete() const { return num_samples == request_num_samples; }

    const std::map<std::string, std::vector<FrameMetadata*> >& GetMetadata() const { return mMetadata; }
    const std::vector<FrameMetadata*>* GetMetadata(std::string id) const;
    void InsertMetadata(FrameMetadata *metadata);

public:
    Rational edit_rate;
    int64_t position;

    Rational track_edit_rate;
    int64_t track_position;
    int64_t ec_position;

    uint32_t request_num_samples;
    uint32_t first_sample_offset;
    uint32_t num_samples;

    bool temporal_reordering;
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;

    int64_t cp_file_position;   // position of first KLV in content package
    int64_t file_position;      // frame wrapped: position of KLV; clip wrapped: position of sample data
    uint8_t kl_size;            // frame wrapped: size of KL; clip wrapped: 0
    size_t file_id;             // file index identifier for file containing sample data

    mxfKey element_key;

protected:
    std::map<std::string, std::vector<FrameMetadata*> > mMetadata;
};


class FrameFactory
{
public:
    virtual ~FrameFactory() {};

    virtual Frame* CreateFrame() = 0;
};


class DefaultFrame : public Frame
{
public:
    DefaultFrame();
    virtual ~DefaultFrame();

    virtual uint32_t GetSize() const;
    virtual const unsigned char* GetBytes() const;

    virtual void Grow(uint32_t min_size);
    virtual uint32_t GetSizeAvailable() const;
    virtual unsigned char* GetBytesAvailable() const;
    virtual void SetSize(uint32_t size);
    virtual void IncrementSize(uint32_t inc);

private:
    ByteArray mData;
};


class DefaultFrameFactory : public FrameFactory
{
public:
    virtual ~DefaultFrameFactory() {};

    virtual Frame* CreateFrame();
};


};



#endif

