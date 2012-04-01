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

#include <bmx/as02/AS02D10Track.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const mxfKey VIDEO_ELEMENT_KEY = MXF_D10_PICTURE_EE_K(0x00);



AS02D10Track::AS02D10Track(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, File *file,
                           string rel_uri)
: AS02PictureTrack(clip, track_index, essence_type, file, rel_uri)
{
    mD10DescriptorHelper = dynamic_cast<D10MXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mD10DescriptorHelper);

    mInputSampleSize = 0;
    mRemoveExcessPadding = false;

    mTrackNumber = MXF_D10_PICTURE_TRACK_NUM(0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

AS02D10Track::~AS02D10Track()
{
}

void AS02D10Track::SetSampleSize(uint32_t size, bool remove_excess_padding)
{
    mInputSampleSize = size;
    mRemoveExcessPadding = remove_excess_padding;
}

void AS02D10Track::PrepareWrite()
{
    uint32_t max_sample_size = mD10DescriptorHelper->GetMaxSampleSize();
    uint32_t sample_size = mInputSampleSize;
    if (mInputSampleSize == 0) {
        mInputSampleSize = max_sample_size;
        sample_size = max_sample_size;
    } else if (mInputSampleSize > max_sample_size) {
        if (mRemoveExcessPadding) {
            log_info("Removing %u excess padding bytes from D-10 frame\n", mInputSampleSize - max_sample_size);
            sample_size = max_sample_size;
        } else {
            log_warn("Input D-10 sample size %u is larger than maximum sample size %u\n",
                     mInputSampleSize, max_sample_size);
        }
    }
    mD10DescriptorHelper->SetSampleSize(sample_size);

    AS02PictureTrack::PrepareWrite();
}

void AS02D10Track::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(size > 0 && num_samples > 0);
    BMX_CHECK(size >= num_samples * mInputSampleSize);

    if (mInputSampleSize > mSampleSize) {
        const unsigned char *sample_data = data;
        uint32_t i;
        for (i = 0; i < num_samples; i++) {
            BMX_CHECK_M(check_excess_d10_padding(sample_data, mInputSampleSize, mSampleSize),
                        ("Failed to remove D-10 frame padding bytes; found non-zero bytes"));

            AS02PictureTrack::WriteSamples(sample_data, mSampleSize, 1);
            sample_data += mInputSampleSize;
        }
    } else {
        AS02PictureTrack::WriteSamples(data, size, num_samples);
    }
}

