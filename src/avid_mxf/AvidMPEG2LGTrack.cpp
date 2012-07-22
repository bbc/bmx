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

#include <bmx/avid_mxf/AvidMPEG2LGTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define INDEX_ENTRY_SIZE            11
#define INDEX_ENTRIES_INCREMENT     250
#define NULL_TEMPORAL_OFFSET        127



AvidMPEG2LGTrack::AvidMPEG2LGTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidPictureTrack(clip, track_index, essence_type, file)
{
    mWriterHelper.SetFlavour(MPEG2LGWriterHelper::AVID_FLAVOUR);

    mTrackNumber = MXF_AVID_MPEG_PICT_TRACK_NUM;
    mEssenceElementKey = MXF_EE_K(AvidMPEGClipWrapped);

    mIndexSegment.SetAllocBlockSize(INDEX_ENTRIES_INCREMENT * INDEX_ENTRY_SIZE);
}

AvidMPEG2LGTrack::~AvidMPEG2LGTrack()
{
}

void AvidMPEG2LGTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(num_samples == 1);
    BMX_CHECK(data && size);

    mWriterHelper.ProcessFrame(data, size);


    // update previous index entry pair if temporal offset now known

    if (mWriterHelper.HavePrevTemporalOffset()) {
        // mIndexSegments only hold whole GOPs and so the earlier entry should be in the current segment
        BMX_CHECK((size_t)(mWriterHelper.GetPrevTemporalOffset() * INDEX_ENTRY_SIZE) <= mIndexSegment.GetSize());

        uint32_t segment_offset = mIndexSegment.GetSize();
        segment_offset -= 2 * mWriterHelper.GetPrevTemporalOffset() * INDEX_ENTRY_SIZE;
        mxf_set_int8(mWriterHelper.GetPrevTemporalOffset() * 2, &mIndexSegment.GetBytes()[segment_offset]);
        mxf_set_int8(mWriterHelper.GetPrevTemporalOffset() * 2, &mIndexSegment.GetBytes()[segment_offset] + INDEX_ENTRY_SIZE);
    }


    // write frame

    BMX_CHECK(mMXFFile->write(data, size) == size);


    // add index entry pair
    // Avid has 2 entries, presumably because it is separate fields
    // Note that the Avid flags have bits 0-3 to to zero. These are reserved in 377 but defined in S381 (MPEG mapping)

    unsigned char entry[INDEX_ENTRY_SIZE];
    mxf_set_int8(mWriterHelper.GetTemporalOffset() * 2, &entry[0]);  // temporal offset
    mxf_set_int8(mWriterHelper.GetKeyFrameOffset() * 2, &entry[1]);  // key frame offset
    mxf_set_uint8(mWriterHelper.GetFlags() & 0xf0,      &entry[2]);  // flags
    mxf_set_int64(mContainerSize,                       &entry[3]);  // stream offset

    mIndexSegment.Append(entry, sizeof(entry));
    mIndexSegment.Append(entry, sizeof(entry));


    mContainerDuration++;
    mContainerSize += size;
}

void AvidMPEG2LGTrack::WriteVBEIndexTable(Partition *partition)
{
    partition->markIndexStart(mMXFFile);

    IndexTableSegment segment;
    segment.setIndexEditRate(mDescriptorHelper->GetSampleRate());
    segment.setIndexSID(mIndexSID);
    segment.setBodySID(mBodySID);
    segment.setEditUnitByteCount(0);

    uint32_t num_index_entries = mIndexSegment.GetSize() / INDEX_ENTRY_SIZE;
    BMX_ASSERT(num_index_entries >= 1);
    int64_t index_duration = (num_index_entries - 1) / 2;

    mxfUUID uuid;
    mxf_generate_uuid(&uuid);

    segment.setInstanceUID(uuid);
    segment.setIndexStartPosition(0);
    segment.setIndexDuration(index_duration);
    segment.writeHeader(mMXFFile, 0, num_index_entries);
    // Avid ignores the 16-bit llen and uses the number of index entries (uint32) instead
    segment.writeAvidIndexEntryArrayHeader(mMXFFile, 0, 0, num_index_entries);
    mMXFFile->write(mIndexSegment.GetBytes(), mIndexSegment.GetSize());

    partition->fillToKag(mMXFFile);
    partition->markIndexEnd(mMXFFile);
}

void AvidMPEG2LGTrack::PostSampleWriting(mxfpp::Partition *partition)
{
    if (!mWriterHelper.CheckTemporalOffsetsComplete(0))
        log_warn("Incomplete MPEG-2 temporal offset data in index table\n");

    // append final (single) index entry providing the total size / end of last frame
    unsigned char entry[INDEX_ENTRY_SIZE];
    mxf_set_int8(0,                 &entry[0]);
    mxf_set_int8(0,                 &entry[1]);
    mxf_set_uint8(0xc0,             &entry[2]);
    mxf_set_int64(mContainerSize,   &entry[3]);
    mIndexSegment.Append(entry, sizeof(entry));


    // Note: no need to update the file descriptor with MPEG info extracted from the essence data because
    // Avid uses the CDCIDescriptor


    AvidPictureTrack::PostSampleWriting(partition);
}

