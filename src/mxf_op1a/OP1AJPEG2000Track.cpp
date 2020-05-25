/*
 * Copyright (C) 2020, British Broadcasting Corporation
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

#include <bmx/mxf_op1a/OP1AJPEG2000Track.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/essence_parser/J2CEssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey VIDEO_ELEMENT_KEY = MXF_JPEG2000_EE_K(0x01, MXF_JPEG2000_NOT_CLIP_WRAPPED_EE_TYPE, 0x00);


OP1AJPEG2000Track::OP1AJPEG2000Track(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                                     mxfRational frame_rate, EssenceType essence_type)
: OP1APictureTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    mWriterHelper.SetDescriptorHelper(dynamic_cast<JPEG2000MXFDescriptorHelper*>(mDescriptorHelper));

    mTrackNumber = MXF_JPEG2000_TRACK_NUM(0x01, MXF_JPEG2000_NOT_CLIP_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

OP1AJPEG2000Track::~OP1AJPEG2000Track()
{
}

void OP1AJPEG2000Track::PrepareWrite(uint8_t track_count)
{
    CompleteEssenceKeyAndTrackNum(track_count);

    mCPManager->RegisterPictureTrackElement(mTrackIndex, mEssenceElementKey, false);
    mIndexTable->RegisterPictureTrackElement(mTrackIndex, false, false);
}

void OP1AJPEG2000Track::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    mWriterHelper.ProcessFrame(data, size);

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
    mIndexTable->AddIndexEntry(mTrackIndex, mWriterHelper.GetFramePosition(), 0, 0, 0x80, true, false);
}

void OP1AJPEG2000Track::CompleteWrite()
{
    mWriterHelper.CompleteProcess();

    OP1APictureTrack::CompleteWrite();
}
