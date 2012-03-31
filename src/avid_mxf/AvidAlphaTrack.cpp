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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>

#include <bmx/avid_mxf/AvidAlphaTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



AvidAlphaTrack::AvidAlphaTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidPictureTrack(clip, track_index, essence_type, file)
{
    mRGBADescriptorHelper = dynamic_cast<UncRGBAMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mRGBADescriptorHelper);

    mEssenceElementKey = MXF_EE_K(AvidUncRGBA);
    mTrackNumber = MXF_AVID_UNC_RGBA_PICT_TRACK_NUM;

    mInputHeight = 0;
    mInputSampleSize = 0;
    mImageEndOffset = 0;
    mPaddingSize = 0;
    mSkipSize = 0;
}

AvidAlphaTrack::~AvidAlphaTrack()
{
}

void AvidAlphaTrack::SetInputHeight(uint32_t height)
{
    mInputHeight = height;
}

uint32_t AvidAlphaTrack::GetInputSampleSize()
{
    return mRGBADescriptorHelper->GetSampleSize(mInputHeight);
}

void AvidAlphaTrack::PrepareWrite()
{
    AvidTrack::PrepareWrite();

    mInputSampleSize = mRGBADescriptorHelper->GetSampleSize(mInputHeight);
    mImageEndOffset = mRGBADescriptorHelper->GetImageEndOffset();

    if (mInputSampleSize > mSampleSize)
        mSkipSize = mInputSampleSize - mSampleSize;
    else if (mInputSampleSize < mSampleSize)
        mPaddingSize = mSampleSize - mInputSampleSize;
}

void AvidAlphaTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(data && size && num_samples);

    // if multiple samples are passed in then they must all be the same size
    BMX_CHECK(mInputSampleSize * num_samples == size);

    const unsigned char *sample_data = data;
    const uint32_t sample_input_size = mSampleSize - mPaddingSize;
    uint32_t i;
    for (i = 0; i < num_samples; i++) {
        if (mPaddingSize > 0) {
            mMXFFile->writeZeros(mPaddingSize);
            mContainerSize += mPaddingSize;
        }

        BMX_CHECK(mMXFFile->write(sample_data + mSkipSize, sample_input_size) == sample_input_size);
        mContainerSize += sample_input_size;
        sample_data += mSkipSize + sample_input_size;

        if (mImageEndOffset > 0) {
            mMXFFile->writeZeros(mImageEndOffset);
            mContainerSize += mImageEndOffset;
        }

        mContainerDuration++;
    }
}

