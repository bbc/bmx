/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#include <bmx/mxf_op1a/OP1AVC2Track.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const mxfKey VIDEO_ELEMENT_KEY = MXF_VC2_EE_K(0x01, MXF_VC2_FRAME_WRAPPED_EE_TYPE, 0x00);



OP1AVC2Track::OP1AVC2Track(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                           mxfRational frame_rate)
: OP1APictureTrack(file, track_index, track_id, track_type_number, frame_rate, VC2)
{
    mVC2DescriptorHelper = dynamic_cast<VC2MXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mVC2DescriptorHelper);

    mTrackNumber = MXF_VC2_TRACK_NUM(0x01, MXF_VC2_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
    mWriterHelper.SetDescriptorHelper(dynamic_cast<VC2MXFDescriptorHelper*>(mDescriptorHelper));

    SetModeFlags(VC2_PICTURE_ONLY | VC2_COMPLETE_SEQUENCES);
}

OP1AVC2Track::~OP1AVC2Track()
{
}

void OP1AVC2Track::SetModeFlags(int flags)
{
    mWriterHelper.SetModeFlags(flags);
}

void OP1AVC2Track::SetSequenceHeader(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetSequenceHeader(data, size);
}

void OP1AVC2Track::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterPictureTrackElement(mTrackIndex, mEssenceElementKey, false);
    mIndexTable->RegisterPictureTrackElement(mTrackIndex, false, false);
}

void OP1AVC2Track::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_CHECK(num_samples == 1);
    BMX_CHECK(data && size);

    const CDataBuffer *data_array;
    uint32_t array_size;
    mWriterHelper.ProcessFrame(data, size, &data_array, &array_size);

    mCPManager->WriteSample(mTrackIndex, data_array, array_size);
    mIndexTable->AddIndexEntry(mTrackIndex, mWriterHelper.GetProcessFrameCount() - 1, 0, 0, 0, true, false);
}

void OP1AVC2Track::CompleteWrite()
{
    mWriterHelper.CompleteProcess();
}
