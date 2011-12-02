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

#include <im/d10_mxf/D10MPEGTrack.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



D10MPEGTrack::D10MPEGTrack(D10File *file, uint32_t track_index, mxfRational frame_rate, D10EssenceType essence_type)
: D10Track(file, track_index, frame_rate, essence_type)
{
    mD10DescriptorHelper = dynamic_cast<D10MXFDescriptorHelper*>(mDescriptorHelper);
    IM_ASSERT(mD10DescriptorHelper);

    mD10DescriptorHelper->SetAspectRatio(ASPECT_RATIO_16_9);
}

D10MPEGTrack::~D10MPEGTrack()
{
}

void D10MPEGTrack::SetOutputTrackNumber(uint32_t track_number)
{
    (void)track_number;

    log_warn("Ignoring attempt to set D10 MPEG output track number\n");
}

void D10MPEGTrack::SetAspectRatio(mxfRational aspect_ratio)
{
    mD10DescriptorHelper->SetAspectRatio(aspect_ratio);
}

void D10MPEGTrack::SetAFD(uint8_t afd)
{
    mD10DescriptorHelper->SetAFD(afd);
}

void D10MPEGTrack::SetSampleSize(uint32_t size)
{
    mD10DescriptorHelper->SetSampleSize(size);
}

FileDescriptor* D10MPEGTrack::CreateFileDescriptor(HeaderMetadata *header_metadata)
{
    return mD10DescriptorHelper->CreateFileDescriptor(header_metadata);
}

void D10MPEGTrack::PrepareWrite()
{
    mCPManager->RegisterMPEGTrackElement(mTrackIndex, mD10DescriptorHelper->GetSampleSize());
}

