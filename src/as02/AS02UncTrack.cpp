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

#define __STDC_LIMIT_MACROS

#include <bmx/as02/AS02UncTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const mxfKey VIDEO_ELEMENT_KEY = MXF_UNC_EE_K(0x01, MXF_UNC_FRAME_WRAPPED_EE_TYPE, 0x00);



AS02UncTrack::AS02UncTrack(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, File *file,
                           string rel_uri)
: AS02PictureTrack(clip, track_index, essence_type, file, rel_uri)
{
    mUncDescriptorHelper = dynamic_cast<UncCDCIMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mUncDescriptorHelper);

    mUncDescriptorHelper->SetComponentDepth(8);
    mInputHeight = 0;
    mInputSampleSize = 0;
    mSkipSize = 0;

    mTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

AS02UncTrack::~AS02UncTrack()
{
}

void AS02UncTrack::SetComponentDepth(uint32_t depth)
{
    mUncDescriptorHelper->SetComponentDepth(depth);
}

void AS02UncTrack::SetInputHeight(uint32_t height)
{
    mInputHeight = height;
}

void AS02UncTrack::PrepareWrite()
{
    uint32_t sample_size = mUncDescriptorHelper->GetSampleSize();
    if (mInputHeight != 0)
        mInputSampleSize = mUncDescriptorHelper->GetSampleSize(mInputHeight);
    else
        mInputSampleSize = sample_size;

    BMX_CHECK_M(mInputSampleSize >= sample_size,
                ("Insufficient input height %u for uncompressed track", mInputHeight));
    mSkipSize = mInputSampleSize - sample_size;

    if (sample_size > (UINT32_MAX >> 8)) // > max length for llen 4
        mEssenceElementLLen = 8;
    else
        mEssenceElementLLen = 4;

    AS02PictureTrack::PrepareWrite();
}

void AS02UncTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_CHECK(data && size && num_samples);

    // if multiple samples are passed in then they must all be the same size
    BMX_CHECK(mInputSampleSize * num_samples == size);

    const unsigned char *sample_data = data;
    uint32_t i;
    for (i = 0; i < num_samples; i++) {
        AS02PictureTrack::WriteSamples(sample_data + mSkipSize, mInputSampleSize - mSkipSize, 1);
        sample_data += mInputSampleSize;
    }
}

