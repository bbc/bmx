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

#include <bmx/mxf_op1a/OP1AMPEG2LGTrack.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey VIDEO_ELEMENT_KEY = MXF_MPEG_PICT_EE_K(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);



OP1AMPEG2LGTrack::OP1AMPEG2LGTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                                   mxfRational frame_rate, EssenceType essence_type)
: OP1APictureTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    mTrackNumber = MXF_MPEG_PICT_TRACK_NUM(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

OP1AMPEG2LGTrack::~OP1AMPEG2LGTrack()
{
}

void OP1AMPEG2LGTrack::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterPictureTrackElement(mTrackIndex, mEssenceElementKey, false);
    mIndexTable->RegisterPictureTrackElement(mTrackIndex, false, true);
}

void OP1AMPEG2LGTrack::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_CHECK(num_samples == 1);
    BMX_CHECK(data && size);

    mWriterHelper.ProcessFrame(data, size);


    // update previous index entry if temporal offset now known

    if (mWriterHelper.HavePrevTemporalOffset()) {
        if (mWriterHelper.GetPrevTemporalOffset() <= mWriterHelper.GetFramePosition()) {
            mIndexTable->UpdateIndexEntry(mTrackIndex,
                                          mWriterHelper.GetFramePosition() - mWriterHelper.GetPrevTemporalOffset(),
                                          mWriterHelper.GetPrevTemporalOffset());
        } else {
            log_warn("Invalid MPEG temporal reference - failed to set MXF temporal offset in index entry before start\n");
        }
    }


    // write frame and add index entry

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
    mIndexTable->AddIndexEntry(mTrackIndex, mWriterHelper.GetFramePosition(), mWriterHelper.GetTemporalOffset(),
                               mWriterHelper.GetKeyFrameOffset(), mWriterHelper.GetFlags(),
                               mWriterHelper.HaveGOPHeader());
}

void OP1AMPEG2LGTrack::CompleteWrite()
{
    if (!mWriterHelper.CheckTemporalOffsetsComplete(mOP1AFile->mOutputEndOffset))
        log_warn("Incomplete MPEG-2 temporal offset data in index table\n");

    // update the file descriptor with info extracted from the essence data
    MPEGVideoDescriptor *mpeg_descriptor = dynamic_cast<MPEGVideoDescriptor*>(mDescriptorHelper->GetFileDescriptor());
    BMX_ASSERT(mpeg_descriptor);
    mpeg_descriptor->setSingleSequence(mWriterHelper.GetSingleSequence());
    mpeg_descriptor->setConstantBFrames(mWriterHelper.GetConstantBFrames());
    mpeg_descriptor->setLowDelay(mWriterHelper.GetLowDelay());
    mpeg_descriptor->setClosedGOP(mWriterHelper.GetClosedGOP());
    mpeg_descriptor->setIdenticalGOP(mWriterHelper.GetIdenticalGOP());
    if (mWriterHelper.GetMaxGOP() > 0)
        mpeg_descriptor->setMaxGOP(mWriterHelper.GetMaxGOP());
    if (mWriterHelper.GetBPictureCount() > 0)
        mpeg_descriptor->setBPictureCount(mWriterHelper.GetBPictureCount());
    if (mWriterHelper.GetBitRate() > 0)
        mpeg_descriptor->setBitRate(mWriterHelper.GetBitRate());
}

