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

#include <bmx/as02/AS02MPEG2LGTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define INDEX_ENTRY_SIZE            11
// MAX_INDEX_SEGMENT_SIZE <
//      (65535 [2-byte max len]
//        - (80 [segment header] + 12 [delta entry array header] + 6 [delta entry] + 22 [index entry array header]))
#define MAX_INDEX_SEGMENT_SIZE      65000
#define INDEX_ENTRIES_INCREMENT     250

#define MAX_GOP_SIZE_GUESS          30



static const mxfKey VIDEO_ELEMENT_KEY = MXF_MPEG_PICT_EE_K(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);



AS02MPEG2LGTrack::AS02MPEG2LGTrack(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, File *file,
                                   string rel_uri)
: AS02PictureTrack(clip, track_index, essence_type, file, rel_uri)
{
    mTrackNumber = MXF_MPEG_PICT_TRACK_NUM(0x01, MXF_MPEG_PICT_FRAME_WRAPPED_EE_TYPE, 0x00);
    mEssenceElementKey = VIDEO_ELEMENT_KEY;

    mIndexStartPosition = 0;
}

AS02MPEG2LGTrack::~AS02MPEG2LGTrack()
{
    size_t i;
    for (i = 0; i < mIndexSegments.size(); i++)
        delete mIndexSegments[i];
}

void AS02MPEG2LGTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(num_samples == 1);
    BMX_CHECK(data && size);

    mWriterHelper.ProcessFrame(data, size);


    // update previous index entry if temporal offset now known

    if (mWriterHelper.HavePrevTemporalOffset()) {
        BMX_ASSERT(!mIndexSegments.empty());
        uint16_t segment_offset = mWriterHelper.GetPrevTemporalOffset() * INDEX_ENTRY_SIZE;
        size_t segment_index = mIndexSegments.size() - 1;
        while (segment_index > 0 && segment_offset > mIndexSegments[segment_index]->GetSize()) {
            segment_offset -= mIndexSegments[segment_index]->GetSize();
            segment_index--;
        }

        if (segment_offset <= mIndexSegments[segment_index]->GetSize()) {
            uint32_t forward_segment_offset = mIndexSegments[segment_index]->GetSize() - segment_offset;
            mxf_set_int8(mWriterHelper.GetPrevTemporalOffset(),
                         &mIndexSegments[segment_index]->GetBytes()[forward_segment_offset]);
        } else {
            log_warn("Invalid MPEG temporal reference - failed to set MXF temporal offset in index entry before start\n");
        }
    }


    // write frame

    HandlePartitionInterval(mWriterHelper.HaveGOPHeader());

    mMXFFile->writeFixedKL(&mEssenceElementKey, mEssenceElementLLen, size);
    BMX_CHECK(mMXFFile->write(data, size) == size);

    UpdateEssenceOnlyChecksum(data, size);


    // add index entry

    if (mIndexSegments.empty() ||
        mIndexSegments.back()->GetSize() >= MAX_INDEX_SEGMENT_SIZE ||
        (mIndexSegments.back()->GetSize() >= (MAX_INDEX_SEGMENT_SIZE - MAX_GOP_SIZE_GUESS) &&
            mWriterHelper.HaveGOPHeader()))
    {
        mIndexSegments.push_back(new ByteArray());
        mIndexSegments.back()->SetAllocBlockSize(INDEX_ENTRIES_INCREMENT * INDEX_ENTRY_SIZE);
    }

    unsigned char entry[INDEX_ENTRY_SIZE];
    mxf_set_int8(mWriterHelper.GetTemporalOffset(), &entry[0]);  // temporal offset
    mxf_set_int8(mWriterHelper.GetKeyFrameOffset(), &entry[1]);  // key frame offset
    mxf_set_uint8(mWriterHelper.GetFlags(),         &entry[2]);  // flags
    mxf_set_int64(mContainerSize,                   &entry[3]);  // stream offset

    mIndexSegments.back()->Append(entry, sizeof(entry));


    mContainerDuration++;
    mContainerSize += mxfKey_extlen + mEssenceElementLLen + size;
}

void AS02MPEG2LGTrack::WriteVBEIndexTable(Partition *partition)
{
    if (mIndexSegments.empty())
        return;

    partition->markIndexStart(mMXFFile);

    IndexTableSegment segment;
    segment.setIndexEditRate(mDescriptorHelper->GetSampleRate());
    segment.setIndexSID(mIndexSID);
    segment.setBodySID(mBodySID);
    segment.setEditUnitByteCount(0);

    int64_t index_start_position = mIndexStartPosition;
    uint32_t index_duration;
    mxfUUID uuid;
    size_t i;
    for (i = 0; i < mIndexSegments.size(); i++) {
        index_duration = mIndexSegments[i]->GetSize() / INDEX_ENTRY_SIZE;

        mxf_generate_uuid(&uuid);
        segment.setInstanceUID(uuid);
        segment.setIndexStartPosition(index_start_position);
        segment.setIndexDuration(index_duration);

        segment.writeHeader(mMXFFile, 1, index_duration);
        segment.writeDeltaEntryArrayHeader(mMXFFile, 1);
        segment.writeDeltaEntry(mMXFFile, -1, 0, 0);
        segment.writeIndexEntryArrayHeader(mMXFFile, 0, 0, index_duration);
        mMXFFile->write(mIndexSegments[i]->GetBytes(), mIndexSegments[i]->GetSize());

        delete mIndexSegments[i];

        index_start_position += index_duration;
    }
    mIndexSegments.clear();

    mIndexStartPosition = index_start_position;

    partition->fillToKag(mMXFFile);
    partition->markIndexEnd(mMXFFile);
}

void AS02MPEG2LGTrack::PostSampleWriting(mxfpp::Partition *partition)
{
    (void)partition;

    if (!mWriterHelper.CheckTemporalOffsetsComplete(mOutputEndOffset))
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

