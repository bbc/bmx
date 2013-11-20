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

#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



AS02PictureTrack::AS02PictureTrack(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, File *file,
                                   string rel_uri)
: AS02Track(clip, track_index, essence_type, file, rel_uri)
{
    mPictureDescriptorHelper = dynamic_cast<PictureMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mPictureDescriptorHelper);

    mPartitionInterval = 0;
    mPartitionFrameCount = 0;

    mPictureDescriptorHelper->SetFrameWrapped(true);
    mPictureDescriptorHelper->SetSampleRate(GetClipFrameRate());
    mPictureDescriptorHelper->SetAspectRatio(ASPECT_RATIO_16_9);

    mIsPicture = true;
}

AS02PictureTrack::~AS02PictureTrack()
{
}

void AS02PictureTrack::SetAspectRatio(mxfRational aspect_ratio)
{
    mPictureDescriptorHelper->SetAspectRatio(aspect_ratio);
}

void AS02PictureTrack::SetAFD(uint8_t afd)
{
    mPictureDescriptorHelper->SetAFD(afd);
}

void AS02PictureTrack::SetPartitionInterval(int64_t frame_count)
{
    BMX_CHECK(frame_count >= 0);
    mPartitionInterval = frame_count;
}

void AS02PictureTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(mSampleSize > 0);
    BMX_CHECK(size > 0 && num_samples > 0);
    BMX_CHECK(size >= num_samples * mSampleSize);

    CDataBuffer data_array[1];
    uint32_t i;
    for (i = 0; i < num_samples; i++) {
        data_array[0].data = (unsigned char*)&data[i * mSampleSize];
        data_array[0].size = mSampleSize;
        WriteSample(data_array, 1);
    }
}

void AS02PictureTrack::WriteSample(const CDataBuffer *data_array, uint32_t array_size)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(data_array && array_size > 0);

    HandlePartitionInterval(true);

    mMXFFile->writeFixedKL(&mEssenceElementKey, mEssenceElementLLen, dba_get_total_size(data_array, array_size));
    mContainerSize += mxfKey_extlen + mEssenceElementLLen;

    uint32_t i;
    for (i = 0; i < array_size; i++) {
        BMX_CHECK(mMXFFile->write(data_array[i].data, data_array[i].size) == data_array[i].size);
        mContainerSize += data_array[i].size;

        UpdateEssenceOnlyChecksum(data_array[i].data, data_array[i].size);
    }

    mContainerDuration++;
}

void AS02PictureTrack::HandlePartitionInterval(bool can_start_partition)
{
    if (mPartitionInterval == 0)
        return;

    if (mPartitionFrameCount >= mPartitionInterval && can_start_partition) {
        if (!HaveCBEIndexTable()) {
            // VBE index table partition

            mMXFFile->openMemoryFile(8192);

            Partition &index_partition = mMXFFile->createPartition();
            index_partition.setKey(&MXF_PP_K(OpenComplete, Body));
            index_partition.setIndexSID(mIndexSID);
            index_partition.setBodySID(0);
            index_partition.write(mMXFFile);

            WriteVBEIndexTable(&index_partition);

            mMXFFile->updatePartitions();
            mMXFFile->closeMemoryFile();
        }

        // start a new essence data partition
        Partition &ess_partition = mMXFFile->createPartition();
        ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
        ess_partition.setIndexSID(0);
        ess_partition.setBodySID(mBodySID);
        ess_partition.setBodyOffset(mContainerSize);
        ess_partition.write(mMXFFile);

        mPartitionFrameCount = 0;
    }

    mPartitionFrameCount++;
}

