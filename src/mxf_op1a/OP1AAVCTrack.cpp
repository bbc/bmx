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

#include <bmx/mxf_op1a/OP1AAVCTrack.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey VIDEO_ELEMENT_KEY = MXF_MPEG_PICT_EE_K(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);



OP1AAVCTrack::OP1AAVCTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                           mxfRational frame_rate, EssenceType essence_type)
: OP1APictureTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    mTrackNumber = MXF_MPEG_PICT_TRACK_NUM(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
    mWriterHelper.SetDescriptorHelper(dynamic_cast<AVCMXFDescriptorHelper*>(mDescriptorHelper));

    log_warn("AVC support is work-in-progress\n");
}

OP1AAVCTrack::~OP1AAVCTrack()
{
}

void OP1AAVCTrack::SetHeader(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetHeader(data, size);
}

void OP1AAVCTrack::SetSPS(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetSPS(data, size);
}

void OP1AAVCTrack::SetPPS(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetPPS(data, size);
}

void OP1AAVCTrack::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterPictureTrackElement(mTrackIndex, mEssenceElementKey, false);
    mIndexTable->RegisterPictureTrackElement(mTrackIndex, false, true);
}

void OP1AAVCTrack::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_CHECK(num_samples == 1);
    BMX_CHECK(data && size);

    mWriterHelper.ProcessFrame(data, size);


    // update previous index entries and get the current index

    bool require_update = true;
    int64_t position = -1;
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    MPEGFrameType frame_type;
    while (mWriterHelper.TakeCompleteIndexEntry(&position, &temporal_offset, &key_frame_offset, &flags, &frame_type)) {
        if (position == mWriterHelper.GetFramePosition()) {
            require_update = false;
            break;
        }
        mIndexTable->UpdateIndexEntry(mTrackIndex, position, temporal_offset, key_frame_offset, flags);
    }
    if (require_update)
        mWriterHelper.GetIncompleteIndexEntry(&position, &temporal_offset, &key_frame_offset, &flags, &frame_type);


    // write frame and add index entry

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
    mIndexTable->AddIndexEntry(mTrackIndex, position, temporal_offset, key_frame_offset, flags,
                               frame_type == I_FRAME, require_update);
}

void OP1AAVCTrack::CompleteWrite()
{
    mWriterHelper.CompleteProcess();

    int64_t position;
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    MPEGFrameType frame_type;
    while (mWriterHelper.TakeCompleteIndexEntry(&position, &temporal_offset, &key_frame_offset, &flags, &frame_type))
        mIndexTable->UpdateIndexEntry(mTrackIndex, position, temporal_offset, key_frame_offset, flags);
}
