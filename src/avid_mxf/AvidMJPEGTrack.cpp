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

#include <cstring>

#include <bmx/avid_mxf/AvidMJPEGTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define INDEX_ENTRY_SIZE            11
#define INDEX_ENTRIES_INCREMENT     250
#define NULL_TEMPORAL_OFFSET        127



AvidMJPEGTrack::AvidMJPEGTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *file)
: AvidPictureTrack(clip, track_index, essence_type, file)
{
    mMJPEGDescriptorHelper = dynamic_cast<MJPEGMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mMJPEGDescriptorHelper);

    mTrackNumber = MXF_AVID_MJPEG_PICT_TRACK_NUM;
    mEssenceElementKey = MXF_EE_K(AvidMJPEGClipWrapped);

    mIndexSegment.SetAllocBlockSize(INDEX_ENTRIES_INCREMENT * INDEX_ENTRY_SIZE);
}

AvidMJPEGTrack::~AvidMJPEGTrack()
{
}

bool AvidMJPEGTrack::IsSingleField() const
{
    return mMJPEGDescriptorHelper->IsSingleField();
}

void AvidMJPEGTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(num_samples == 1);
    BMX_CHECK(size > 0);

    BMX_CHECK(mMXFFile->write(data, size) == size);

    unsigned char entry[INDEX_ENTRY_SIZE];
    mxf_set_int8(0, &entry[0]);
    mxf_set_int8(0, &entry[1]);
    mxf_set_uint8(0x80, &entry[2]); // key frame
    mxf_set_int64(mContainerSize, &entry[3]);
    mIndexSegment.Append(entry, sizeof(entry));

    mContainerDuration++;
    mContainerSize += size;
}

void AvidMJPEGTrack::WriteVBEIndexTable(Partition *partition)
{
    partition->markIndexStart(mMXFFile);

    IndexTableSegment segment;
    segment.setIndexEditRate(mDescriptorHelper->GetSampleRate());
    segment.setIndexSID(mIndexSID);
    segment.setBodySID(mBodySID);
    segment.setEditUnitByteCount(0);

    uint32_t num_index_entries = mIndexSegment.GetSize() / INDEX_ENTRY_SIZE;
    BMX_ASSERT(num_index_entries >= 1);
    int64_t index_duration = num_index_entries - 1;

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

void AvidMJPEGTrack::PostSampleWriting(mxfpp::Partition *partition)
{
    // append final index entry providing the total size / end of last frame
    unsigned char entry[INDEX_ENTRY_SIZE];
    mxf_set_int8(0, &entry[0]);
    mxf_set_int8(0, &entry[1]);
    mxf_set_uint8(0x80, &entry[2]);
    mxf_set_int64(mContainerSize, &entry[3]);
    mIndexSegment.Append(entry, sizeof(entry));

    AvidPictureTrack::PostSampleWriting(partition);
}

