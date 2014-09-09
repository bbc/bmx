/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#include <bmx/mxf_op1a/OP1ADataTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



OP1ADataTrack::OP1ADataTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             mxfRational frame_rate, EssenceType essence_type)
: OP1ATrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    mDataDescriptorHelper = dynamic_cast<DataMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mDataDescriptorHelper);

    mPosition = 0;
    mConstantDataSize = 0;
    mMaxDataSize = 0;
}

OP1ADataTrack::~OP1ADataTrack()
{
}

void OP1ADataTrack::SetConstantDataSize(uint32_t size)
{
    mConstantDataSize = size;
}

void OP1ADataTrack::SetMaxDataSize(uint32_t size)
{
    mMaxDataSize = size;
}

void OP1ADataTrack::PrepareWrite(uint8_t track_count)
{
    BMX_ASSERT(track_count == 1);
    // track number and essence element key are already complete

    mCPManager->RegisterDataTrackElement(mTrackIndex, mEssenceElementKey, mConstantDataSize, mMaxDataSize);
    mIndexTable->RegisterDataTrackElement(mTrackIndex, (mConstantDataSize || mMaxDataSize));
}

void OP1ADataTrack::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_CHECK(num_samples == 1);
    BMX_CHECK(data && size);

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
    if (!mConstantDataSize && !mMaxDataSize)
        mIndexTable->AddIndexEntry(mTrackIndex, mPosition, 0, 0, 0, true);

    mPosition++;
}

