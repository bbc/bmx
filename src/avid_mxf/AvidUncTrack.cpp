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

#include <bmx/avid_mxf/AvidUncTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



AvidUncTrack::AvidUncTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidPictureTrack(clip, track_index, essence_type, file)
{
    mUncDescriptorHelper = dynamic_cast<UncCDCIMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mUncDescriptorHelper);

    if (essence_type == AVID_10BIT_UNC_SD ||
        essence_type == AVID_10BIT_UNC_HD_1080I ||
        essence_type == AVID_10BIT_UNC_HD_1080P ||
        essence_type == AVID_10BIT_UNC_HD_720P)
    {
        mIsAvid10Bit = true;
        mEssenceElementKey = MXF_EE_K(AvidUnc10BitClipWrapped);
        mTrackNumber = MXF_AVID_UNC_10BIT_PICT_TRACK_NUM;
        mUncDescriptorHelper->SetComponentDepth(10);
    }
    else
    {
        mIsAvid10Bit = false;
        mEssenceElementKey = MXF_EE_K(UncClipWrapped);
        mTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
        mUncDescriptorHelper->SetComponentDepth(8);
    }

    mInputHeight = 0;
    mInputSampleSize = 0;
    mImageStartOffset = 0;
    mPadding = 0;
    mPaddingSize = 0;
    mSkipSize = 0;
    mMSBSampleSize = 0;
    mLSBSampleSize = 0;
    mLSBPaddingSize = 0;
    mLSBSkipSize = 0;
}

AvidUncTrack::~AvidUncTrack()
{
    delete [] mPadding;
}

void AvidUncTrack::SetInputHeight(uint32_t height)
{
    mInputHeight = height;
}

uint32_t AvidUncTrack::GetInputSampleSize()
{
    return mUncDescriptorHelper->GetSampleSize(mInputHeight);
}

void AvidUncTrack::PrepareWrite()
{
    AvidPictureTrack::PrepareWrite();

    mInputSampleSize = mUncDescriptorHelper->GetSampleSize(mInputHeight);
    mImageStartOffset = mUncDescriptorHelper->GetImageStartOffset();

    if (mInputSampleSize > mSampleSize)
        mSkipSize = mInputSampleSize - mSampleSize;
    else if (mInputSampleSize < mSampleSize)
        mPaddingSize = mSampleSize - mInputSampleSize;

    if (mIsAvid10Bit) {
        mLSBSkipSize     = mSkipSize / 5;
        mSkipSize       -= mLSBSkipSize;
        mLSBPaddingSize  = mPaddingSize / 5;
        mPaddingSize    -= mLSBPaddingSize;
        mLSBSampleSize   = mSampleSize / 5;
        mMSBSampleSize   = mSampleSize - mLSBSampleSize;
    }

    if (mPaddingSize > 0) {
        mPadding = new unsigned char[mPaddingSize];
        uint32_t i;
        for (i = 0; i < mPaddingSize; i += 4) {
            mPadding[i    ] = 0x80;
            mPadding[i + 1] = 0x10;
            mPadding[i + 2] = 0x80;
            mPadding[i + 3] = 0x10;
        }
    }
}

void AvidUncTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(data && size && num_samples);

    // if multiple samples are passed in then they must all be the same size
    BMX_CHECK(mInputSampleSize * num_samples == size);

    if (mIsAvid10Bit) {
        const unsigned char *sample_data = data;
        const uint32_t lsb_input_size = mLSBSampleSize - mLSBPaddingSize;
        const uint32_t msb_input_size = mMSBSampleSize - mPaddingSize;
        uint32_t i;
        for (i = 0; i < num_samples; i++) {
            if (mImageStartOffset + mLSBPaddingSize > 0) {
                mMXFFile->writeZeros(mImageStartOffset + mLSBPaddingSize);
                mContainerSize += mImageStartOffset + mLSBPaddingSize;
            }

            BMX_CHECK(mMXFFile->write(sample_data + mLSBSkipSize, lsb_input_size) == lsb_input_size);
            mContainerSize += lsb_input_size;
            sample_data += mLSBSkipSize + lsb_input_size;

            if (mPaddingSize > 0) {
                BMX_CHECK(mMXFFile->write(mPadding, mPaddingSize) == mPaddingSize);
                mContainerSize += mPaddingSize;
            }

            BMX_CHECK(mMXFFile->write(sample_data + mSkipSize, msb_input_size) == msb_input_size);
            mContainerSize += msb_input_size;
            sample_data += mSkipSize + msb_input_size;

            mContainerDuration++;
        }
    } else {
        const unsigned char *sample_data = data;
        const uint32_t sample_input_size = mSampleSize - mPaddingSize;
        uint32_t i;
        for (i = 0; i < num_samples; i++) {
            if (mImageStartOffset > 0) {
                mMXFFile->writeZeros(mImageStartOffset);
                mContainerSize += mImageStartOffset;
            }

            if (mPaddingSize > 0) {
                BMX_CHECK(mMXFFile->write(mPadding, mPaddingSize) == mPaddingSize);
                mContainerSize += mPaddingSize;
            }

            BMX_CHECK(mMXFFile->write(sample_data + mSkipSize, sample_input_size) == sample_input_size);
            mContainerSize += sample_input_size;
            sample_data += mSkipSize + sample_input_size;

            mContainerDuration++;
        }
    }
}

