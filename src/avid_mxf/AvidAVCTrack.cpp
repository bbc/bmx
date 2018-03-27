/*
 * Copyright (C) 2018, British Broadcasting Corporation
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

#include <bmx/avid_mxf/AvidAVCTrack.h>
#include <bmx/avid_mxf/AvidClip.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey VIDEO_ELEMENT_KEY = MXF_MPEG_PICT_EE_K(0x01, MXF_MPEG_PICT_CLIP_WRAPPED_EE_TYPE, 0x01);



AvidAVCTrack::AvidAVCTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidPictureTrack(clip, track_index, essence_type, file)
, mIndexTable(mIndexSID, mBodySID, mDescriptorHelper->GetSampleRate())
{
    mTrackNumber = MXF_MPEG_PICT_TRACK_NUM(0x01, MXF_MPEG_PICT_CLIP_WRAPPED_EE_TYPE, 0x01);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
    mWriterHelper.SetDescriptorHelper(dynamic_cast<AVCMXFDescriptorHelper*>(mDescriptorHelper));

    log_warn("AVC support is work-in-progress\n");
}

AvidAVCTrack::~AvidAVCTrack()
{
}

void AvidAVCTrack::SetHeader(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetHeader(data, size);
}

void AvidAVCTrack::SetSPS(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetSPS(data, size);
}

void AvidAVCTrack::SetPPS(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetPPS(data, size);
}

void AvidAVCTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
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

        mIndexTable.UpdateIndexEntry(position, temporal_offset, key_frame_offset, flags);
    }
    if (require_update) {
        mWriterHelper.GetIncompleteIndexEntry(&position, &temporal_offset, &key_frame_offset, &flags, &frame_type);
    }


    // write frame

    BMX_CHECK(mMXFFile->write(data, size) == size);


    // add index entry

    mIndexTable.AddIndexEntry(position, temporal_offset, key_frame_offset, flags,
            mContainerSize, frame_type == I_FRAME, require_update);

    mContainerDuration++;
    mContainerSize += size;
}

void AvidAVCTrack::WriteVBEIndexTable(Partition *partition)
{
    mIndexTable.WriteVBEIndexTable(mMXFFile, partition);
}

void AvidAVCTrack::PostSampleWriting(Partition *partition)
{
    mWriterHelper.CompleteProcess();

    // @TODO
    //if (!mWriterHelper.CheckTemporalOffsetsComplete(0))
    //    log_warn("Incomplete AVC temporal offset data in index table\n");

    int64_t position;
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
    MPEGFrameType frame_type;
    while (mWriterHelper.TakeCompleteIndexEntry(&position, &temporal_offset, &key_frame_offset, &flags, &frame_type))
        mIndexTable.UpdateIndexEntry(position, temporal_offset, key_frame_offset, flags);

    // append final index entry providing the total size / end of last frame
    mIndexTable.AddIndexEntry(mWriterHelper.GetFramePosition() + 1, 0, 0, 0x80, mContainerSize, true, false);


    AvidPictureTrack::PostSampleWriting(partition);
}
