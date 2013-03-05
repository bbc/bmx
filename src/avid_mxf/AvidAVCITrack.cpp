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

#define __STDC_FORMAT_MACROS

#include <bmx/avid_mxf/AvidAVCITrack.h>
#include <bmx/avid_mxf/AvidClip.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const mxfKey VIDEO_ELEMENT_KEY = MXF_MPEG_PICT_EE_K(0x01, MXF_MPEG_PICT_CLIP_WRAPPED_EE_TYPE, 0x01);



AvidAVCITrack::AvidAVCITrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidPictureTrack(clip, track_index, essence_type, file)
{
    mAVCIDescriptorHelper = dynamic_cast<AVCIMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mAVCIDescriptorHelper);

    mWriterHelper.SetMode(AVCI_NO_OR_ALL_FRAME_HEADER_MODE);

    mTrackNumber = MXF_MPEG_PICT_TRACK_NUM(0x01, MXF_MPEG_PICT_CLIP_WRAPPED_EE_TYPE, 0x01);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

AvidAVCITrack::~AvidAVCITrack()
{
}

void AvidAVCITrack::SetMode(AvidAVCIMode mode)
{
    switch (mode)
    {
        case AVID_AVCI_NO_OR_ALL_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_NO_OR_ALL_FRAME_HEADER_MODE);
            break;
        case AVID_AVCI_NO_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_NO_FRAME_HEADER_MODE);
            mAVCIDescriptorHelper->SetIncludeHeader(false);
            break;
        case AVID_AVCI_ALL_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_ALL_FRAME_HEADER_MODE);
            mAVCIDescriptorHelper->SetIncludeHeader(true);
            break;
    }
}

void AvidAVCITrack::SetHeader(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetHeader(data, size);
}

uint32_t AvidAVCITrack::GetSampleWithoutHeaderSize()
{
    return mAVCIDescriptorHelper->GetSampleWithoutHeaderSize();
}

void AvidAVCITrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(data && size && num_samples);

    // if multiple samples are passed in then they must all be the same size
    uint32_t sample_size = size / num_samples;
    BMX_CHECK(sample_size * num_samples == size);
    BMX_CHECK(sample_size == GetSampleSize() || sample_size == GetSampleWithoutHeaderSize());


    const unsigned char *sample_data = data;
    const CDataBuffer *data_array;
    uint32_t array_size;
    uint32_t write_sample_size;
    uint32_t i, j;
    for (i = 0; i < num_samples; i++) {
        write_sample_size = mWriterHelper.ProcessFrame(sample_data, sample_size, &data_array, &array_size);

        for (j = 0; j < array_size; j++)
            BMX_CHECK(mMXFFile->write(data_array[j].data, data_array[j].size) == data_array[j].size);

        if (mContainerDuration == 0)
            mSampleSize = write_sample_size;

        sample_data += sample_size;
        mContainerDuration++;
        mContainerSize += write_sample_size;
    }
}

void AvidAVCITrack::PostSampleWriting(Partition *partition)
{
    AvidPictureTrack::PostSampleWriting(partition);

    // reset sample size here because mSampleSize might have changed from GetSampleSize() when metadata was created
    mAVCIDescriptorHelper->SetAvidFrameSampleSize(mSampleSize);
}

