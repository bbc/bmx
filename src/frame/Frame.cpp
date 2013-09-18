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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bmx/frame/Frame.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



FrameMetadata::FrameMetadata(const char *id)
{
    mId = id;
}

FrameMetadata::~FrameMetadata()
{
}



Frame::Frame()
{
    edit_rate = ZERO_RATIONAL;
    position = NULL_FRAME_POSITION;
    track_edit_rate = ZERO_RATIONAL;
    track_position = NULL_FRAME_POSITION;
    ec_position = NULL_FRAME_POSITION;
    request_num_samples = 0;
    first_sample_offset = 0;
    num_samples = 0;
    temporal_reordering = false;
    temporal_offset = 0;
    key_frame_offset = 0;
    flags = 0;
    cp_file_position = 0;
    file_position = 0;
    kl_size = 0;
    file_id = (size_t)(-1);
    element_key = g_Null_Key;
}

Frame::~Frame()
{
    map<string, vector<FrameMetadata*> >::const_iterator iter;
    for (iter = mMetadata.begin(); iter != mMetadata.end(); iter++) {
        size_t i;
        for (i = 0; i < iter->second.size(); i++)
            delete iter->second[i];
    }
}

const vector<FrameMetadata*>* Frame::GetMetadata(std::string id) const
{
    map<string, vector<FrameMetadata*> >::const_iterator result = mMetadata.find(id);
    if (result == mMetadata.end())
        return 0;

    return &result->second;
}

void Frame::InsertMetadata(FrameMetadata *metadata)
{
    mMetadata[metadata->GetId()].push_back(metadata);
}



DefaultFrame::DefaultFrame()
: Frame()
{
}

DefaultFrame::~DefaultFrame()
{
}

uint32_t DefaultFrame::GetSize() const
{
    return mData.GetSize();
}

const unsigned char* DefaultFrame::GetBytes() const
{
    return mData.GetBytes();
}

void DefaultFrame::Grow(uint32_t min_size)
{
    mData.Grow(min_size);
}

uint32_t DefaultFrame::GetSizeAvailable() const
{
    return mData.GetSizeAvailable();
}

unsigned char* DefaultFrame::GetBytesAvailable() const
{
    return mData.GetBytesAvailable();
}

void DefaultFrame::SetSize(uint32_t size)
{
    mData.SetSize(size);
}

void DefaultFrame::IncrementSize(uint32_t inc)
{
    mData.IncrementSize(inc);
}



Frame* DefaultFrameFactory::CreateFrame()
{
    return new DefaultFrame();
}

