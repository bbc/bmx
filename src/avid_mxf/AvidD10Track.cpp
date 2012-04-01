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

#include <bmx/avid_mxf/AvidD10Track.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const mxfKey VIDEO_ELEMENT_KEY = MXF_D10_PICTURE_EE_K(0x01);


AvidD10Track::AvidD10Track(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidPictureTrack(clip, track_index, essence_type, file)
{
    mD10DescriptorHelper = dynamic_cast<D10MXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mD10DescriptorHelper);

    mInputSampleSize = 0;
    mPaddingSize = 0;

    mTrackNumber = MXF_D10_PICTURE_TRACK_NUM(0x01);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

AvidD10Track::~AvidD10Track()
{
}

void AvidD10Track::SetSampleSize(uint32_t size)
{
    mInputSampleSize = size;
}

void AvidD10Track::PrepareWrite()
{
    uint32_t max_sample_size = mD10DescriptorHelper->GetSampleSize();
    if (mInputSampleSize == 0) {
        mInputSampleSize = max_sample_size;
    } else if (mInputSampleSize < max_sample_size) {
        mD10DescriptorHelper->SetSampleSize(max_sample_size);
        mPaddingSize = max_sample_size - mInputSampleSize;
    } else if (mInputSampleSize > max_sample_size) {
        log_warn("Input D-10 sample size %u is larger than 4k aligned Avid sample size %u\n",
                 mInputSampleSize, max_sample_size);
        mD10DescriptorHelper->SetSampleSize(mInputSampleSize);
    }

    AvidTrack::PrepareWrite();
}

void AvidD10Track::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(size > 0 && num_samples > 0);
    BMX_CHECK(size >= num_samples * mInputSampleSize);

    if (mPaddingSize > 0) {
        const unsigned char *sample_data = data;
        uint32_t i;
        for (i = 0; i < num_samples; i++) {
            BMX_CHECK(mMXFFile->write(sample_data, mInputSampleSize) == mInputSampleSize);
            mContainerSize += mInputSampleSize;
            sample_data += mInputSampleSize;

            mMXFFile->writeZeros(mPaddingSize);
            mContainerSize += mPaddingSize;

            mContainerDuration++;
        }
    } else {
        uint32_t write_size = num_samples * mInputSampleSize;
        BMX_CHECK(mMXFFile->write(data, write_size) == write_size);
        mContainerSize += write_size;
        mContainerDuration += num_samples;
    }
}

