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

#include <bmx/mxf_op1a/OP1AAVCITrack.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const mxfKey VIDEO_ELEMENT_KEY = MXF_MPEG_PICT_EE_K(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);



OP1AAVCITrack::OP1AAVCITrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             mxfRational frame_rate, EssenceType essence_type)
: OP1APictureTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    mAVCIDescriptorHelper = dynamic_cast<AVCIMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mAVCIDescriptorHelper);

    mTrackNumber = MXF_MPEG_PICT_TRACK_NUM(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;

    SetMode(OP1A_AVCI_ALL_FRAME_HEADER_MODE);
}

OP1AAVCITrack::~OP1AAVCITrack()
{
}

void OP1AAVCITrack::SetMode(OP1AAVCIMode mode)
{
    if ((mOP1AFile->GetFlavour() & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR) && mode != OP1A_AVCI_ALL_FRAME_HEADER_MODE)
        BMX_EXCEPTION(("ARD ZDF HDF flavour requires all AVC-I frames to contain SPS+PPS header data"));

    switch (mode)
    {
        case OP1A_AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE);
            break;
        case OP1A_AVCI_FIRST_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_FIRST_FRAME_HEADER_MODE);
            break;
        case OP1A_AVCI_ALL_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_ALL_FRAME_HEADER_MODE);
            break;
        case OP1A_AVCI_NO_OR_ALL_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_NO_OR_ALL_FRAME_HEADER_MODE);
            break;
        case OP1A_AVCI_NO_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_NO_FRAME_HEADER_MODE);
            break;
    }
}

void OP1AAVCITrack::SetHeader(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetHeader(data, size);
}

void OP1AAVCITrack::SetReplaceHeader(bool enable)
{
    mWriterHelper.SetReplaceHeader(enable);
}

uint32_t OP1AAVCITrack::GetSampleWithoutHeaderSize()
{
    return mAVCIDescriptorHelper->GetSampleWithoutHeaderSize();
}

void OP1AAVCITrack::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterAVCITrackElement(mTrackIndex, mEssenceElementKey,
                                         mAVCIDescriptorHelper->GetSampleSize(),
                                         mAVCIDescriptorHelper->GetSampleWithoutHeaderSize());
    mIndexTable->RegisterAVCITrackElement(mTrackIndex);
}

void OP1AAVCITrack::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_CHECK(data && size && num_samples);

    // if multiple samples are passed in then they must all be the same size
    uint32_t sample_size = size / num_samples;
    BMX_CHECK(sample_size * num_samples == size);
    BMX_CHECK(sample_size == GetSampleSize() || sample_size == GetSampleWithoutHeaderSize());

    const unsigned char *sample_data = data;
    const CDataBuffer *data_array;
    uint32_t array_size;
    uint32_t i;
    for (i = 0; i < num_samples; i++) {
        mWriterHelper.ProcessFrame(sample_data, sample_size, &data_array, &array_size);
        mCPManager->WriteSample(mTrackIndex, data_array, array_size);

        sample_data += sample_size;
    }
}

