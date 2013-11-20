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

#define __STDC_FORMAT_MACROS

#include <bmx/as02/AS02AVCITrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey VIDEO_ELEMENT_KEY = MXF_MPEG_PICT_EE_K(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);



AS02AVCITrack::AS02AVCITrack(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, File *file,
                             string rel_uri)
: AS02PictureTrack(clip, track_index, essence_type, file, rel_uri)
{
    mAVCIDescriptorHelper = dynamic_cast<AVCIMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mAVCIDescriptorHelper);

    mWriterHelper.SetMode(AVCI_ALL_FRAME_HEADER_MODE);
    mFirstFrameHeaderOnly = false;

    mIndexSegment1 = 0;
    mIndexSegment2 = 0;
    mEndOfIndexTablePosition = 0;

    mTrackNumber = MXF_MPEG_PICT_TRACK_NUM(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;
}

AS02AVCITrack::~AS02AVCITrack()
{
    delete mIndexSegment1;
    delete mIndexSegment2;
}

void AS02AVCITrack::SetMode(AS02AVCIMode mode)
{
    switch (mode)
    {
        case AS02_AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE);
            break;
        case AS02_AVCI_FIRST_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_FIRST_FRAME_HEADER_MODE);
            break;
        case AS02_AVCI_ALL_FRAME_HEADER_MODE:
            mWriterHelper.SetMode(AVCI_ALL_FRAME_HEADER_MODE);
            break;
    }
}

void AS02AVCITrack::SetHeader(const unsigned char *data, uint32_t size)
{
    mWriterHelper.SetHeader(data, size);
}

uint32_t AS02AVCITrack::GetSampleWithoutHeaderSize()
{
    return mAVCIDescriptorHelper->GetSampleWithoutHeaderSize();
}

void AS02AVCITrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(data && size && num_samples);

    // if multiple samples are passed in then they must all be the same size
    uint32_t sample_size = size / num_samples;
    BMX_CHECK(sample_size * num_samples == size);
    BMX_CHECK(sample_size == GetSampleSize() || sample_size == GetSampleWithoutHeaderSize());


    HandlePartitionInterval(true);

    const unsigned char *sample_data = data;
    const CDataBuffer *data_array;
    uint32_t array_size;
    uint32_t write_sample_size;
    uint32_t i, j;
    for (i = 0; i < num_samples; i++) {
        write_sample_size = mWriterHelper.ProcessFrame(sample_data, sample_size, &data_array, &array_size);

        mMXFFile->writeFixedKL(&mEssenceElementKey, mEssenceElementLLen, write_sample_size);
        for (j = 0; j < array_size; j++) {
            BMX_CHECK(mMXFFile->write(data_array[j].data, data_array[j].size) == data_array[j].size);
            UpdateEssenceOnlyChecksum(data_array[j].data, data_array[j].size);
        }

        if (mContainerDuration == 1 && write_sample_size == GetSampleWithoutHeaderSize())
            mFirstFrameHeaderOnly = true;

        sample_data += sample_size;
        mContainerDuration++;
        mContainerSize += mxfKey_extlen + mEssenceElementLLen + write_sample_size;
    }
}

void AS02AVCITrack::WriteCBEIndexTable(Partition *partition)
{
    if (!mIndexSegment1) {
        mxfUUID uuid;
        mxf_generate_uuid(&uuid);

        mIndexSegment1 = new IndexTableSegment();
        mIndexSegment1->setInstanceUID(uuid);
        mIndexSegment1->setIndexStartPosition(0);
        mIndexSegment1->setIndexEditRate(mDescriptorHelper->GetSampleRate());
        mIndexSegment1->setIndexDuration(0); // will be updated when writing is completed (2nd WriteIndexTable() call)
        mIndexSegment1->setIndexSID(mIndexSID);
        mIndexSegment1->setBodySID(mBodySID);
        mIndexSegment1->setEditUnitByteCount(mxfKey_extlen + mEssenceElementLLen + GetSampleSize());
    } else {
        if (mFirstFrameHeaderOnly) {
            // index table segment pair

            mIndexSegment1->setIndexDuration(1);

            mxfUUID uuid;
            mxf_generate_uuid(&uuid);

            mIndexSegment2 = new IndexTableSegment();
            mIndexSegment2->setInstanceUID(uuid);
            mIndexSegment2->setIndexStartPosition(1);
            mIndexSegment2->setIndexEditRate(mDescriptorHelper->GetSampleRate());
            mIndexSegment2->setIndexDuration(mContainerDuration - 1);
            mIndexSegment2->setIndexSID(mIndexSID);
            mIndexSegment2->setBodySID(mBodySID);
            mIndexSegment2->setEditUnitByteCount(mxfKey_extlen + mEssenceElementLLen + GetSampleWithoutHeaderSize());
        } else {
            // single index table segment

            mIndexSegment1->setIndexDuration(mContainerDuration);
        }
    }


    partition->markIndexStart(mMXFFile);

    int64_t file_pos = mMXFFile->tell();
    BMX_CHECK(mxf_write_index_table_segment(mMXFFile->getCFile(), mIndexSegment1->getCIndexTableSegment()));

    // rely on segment2 size == segment1 size and size remaining the same for both calls to WriteCBEIndexTable
    uint32_t segment_size = (uint32_t)(mMXFFile->tell() - file_pos);

    if (mIndexSegment2) {
        file_pos = mMXFFile->tell();
        BMX_CHECK(mxf_write_index_table_segment(mMXFFile->getCFile(), mIndexSegment2->getCIndexTableSegment()));
        BMX_ASSERT((uint32_t)(mMXFFile->tell() - file_pos) == segment_size);
    }

    if (mEndOfIndexTablePosition > 0) {
        mMXFFile->fillToPosition(mEndOfIndexTablePosition);
    } else {
        partition->allocateSpaceToKag(mMXFFile, segment_size);
        mEndOfIndexTablePosition = mMXFFile->tell();
    }

    partition->markIndexEnd(mMXFFile);
}

