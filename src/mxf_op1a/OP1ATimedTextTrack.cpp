/*
 * Copyright (C) 2018, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <bmx/mxf_op1a/OP1ATimedTextTrack.h>

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



OP1ATimedTextTrack::OP1ATimedTextTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                                       mxfRational frame_rate, EssenceType essence_type)
: OP1ATrack(file, track_index, track_id, track_type_number, frame_rate, essence_type)
{
    mTrackNumber = MXF_EE_TRACKNUM(TimedText);
    mEssenceElementKey = MXF_EE_K(TimedText);

    mTimedTextDescriptorHelper = dynamic_cast<TimedTextMXFDescriptorHelper*>(mDescriptorHelper);
    BMX_ASSERT(mTimedTextDescriptorHelper);

    mFileSourcePackage = 0;
    mBodySID = mOP1AFile->CreateStreamId();
    mIndexSID = mOP1AFile->CreateStreamId();
    mDuration = -1;
}

OP1ATimedTextTrack::~OP1ATimedTextTrack()
{
}

void OP1ATimedTextTrack::SetSource(const TimedTextManifest *manifest)
{
    mTTFilename = manifest->mTTFilename;

    TimedTextManifest modified_manifest = *manifest;
    vector<TimedTextAncillaryResource> &resources = modified_manifest.GetAncillaryResources();
    size_t i;
    for (i = 0; i < resources.size(); i++) {
        TimedTextAncillaryResource &resource = resources[i];
        resource.stream_id = mOP1AFile->CreateStreamId();
        mAncillaryResources.push_back(resource);
    }
    mTimedTextDescriptorHelper->SetManifest(&modified_manifest);
}

void OP1ATimedTextTrack::SetDuration(int64_t duration)
{
    mDuration = duration;
}

void OP1ATimedTextTrack::WriteIndexTable(File *mxf_file, Partition *index_partition)
{
    // this is a partial index table that "indexes" the first edit unit only in the clip wrapped container
    mxfUUID uuid;
    mxf_generate_uuid(&uuid);
    IndexTableSegment index_segment;
    index_segment.setInstanceUID(uuid);
    index_segment.setIndexEditRate(mEditRate);
    index_segment.setIndexStartPosition(0);
    index_segment.setIndexDuration(1);  // indexes first edit unit only and so != mDuration
    index_segment.setIndexSID(mIndexSID);
    index_segment.setBodySID(mBodySID);
    index_segment.setEditUnitByteCount(0);
    index_segment.appendIndexEntry(0, 0, 0, 0, vector<uint32_t>(), vector<mxfRational>());

    index_segment.write(mxf_file, index_partition, 0);
}

void OP1ATimedTextTrack::WriteEssenceContainer(File *mxf_file, Partition *ess_partition)
{
    WriteFileData(mxf_file, &mEssenceElementKey, mTTFilename);
    ess_partition->fillToKag(mxf_file);
}

uint32_t OP1ATimedTextTrack::GetAncillaryResourceStreamId(size_t index) const
{
    BMX_ASSERT(index < mAncillaryResources.size());

    return mAncillaryResources[index].stream_id;
}

void OP1ATimedTextTrack::WriteAncillaryResource(File *mxf_file, Partition *stream_partition, size_t index)
{
    BMX_ASSERT(index < mAncillaryResources.size());

    WriteFileData(mxf_file, &MXF_EE_K(TimedTextAnc), mAncillaryResources[index].filename);
    stream_partition->fillToKag(mxf_file);
}

void OP1ATimedTextTrack::AddHeaderMetadata(HeaderMetadata *header_metadata, MaterialPackage *material_package,
                                           SourcePackage *file_source_package)
{
    mFileSourcePackage = file_source_package;

    mxfUL data_def_ul;
    BMX_CHECK(mxf_get_ddef_label(mDataDef, &data_def_ul));

    // Preface - ContentStorage
    ContentStorage *content_storage = header_metadata->getPreface()->getContentStorage();

    // Preface - ContentStorage - EssenceContainerData
    EssenceContainerData *ess_container_data = new EssenceContainerData(header_metadata);
    content_storage->appendEssenceContainerData(ess_container_data);
    ess_container_data->setLinkedPackageUID(mFileSourcePackage->getPackageUID());
    ess_container_data->setIndexSID(mIndexSID);
    ess_container_data->setBodySID(mBodySID);

    // Preface - ContentStorage - MaterialPackage - Timeline Track
    Track *track = new Track(header_metadata);
    material_package->appendTracks(track);
    track->setTrackName(get_track_name(mDataDef, mOutputTrackNumber));
    track->setTrackID(mTrackId);
    track->setTrackNumber(0);
    track->setEditRate(mEditRate);
    track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
    Sequence *sequence = new Sequence(header_metadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(data_def_ul);
    sequence->setDuration(mDuration);

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
    SourceClip *source_clip = new SourceClip(header_metadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(data_def_ul);
    source_clip->setDuration(mDuration);
    source_clip->setStartPosition(0);
    source_clip->setSourcePackageID(mFileSourcePackage->getPackageUID());
    source_clip->setSourceTrackID(mTrackId);


    // Preface - ContentStorage - SourcePackage - Timeline Track
    track = new Track(header_metadata);
    mFileSourcePackage->appendTracks(track);
    track->setTrackName(get_track_name(mDataDef, mOutputTrackNumber));
    track->setTrackID(mTrackId);
    track->setTrackNumber(mTrackNumber);
    track->setEditRate(mEditRate);
    track->setOrigin(0);

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
    sequence = new Sequence(header_metadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(data_def_ul);
    sequence->setDuration(mDuration);

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
    source_clip = new SourceClip(header_metadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(data_def_ul);
    source_clip->setDuration(mDuration);
    source_clip->setStartPosition(0);
    source_clip->setSourceTrackID(0);
    source_clip->setSourcePackageID(g_Null_UMID);

    // Preface - ContentStorage - SourcePackage - FileDescriptor
    FileDescriptor *descriptor = mDescriptorHelper->CreateFileDescriptor(header_metadata);
    if (mFileSourcePackage->haveDescriptor()) {
        MultipleDescriptor *mult_descriptor = dynamic_cast<MultipleDescriptor*>(file_source_package->getDescriptor());
        BMX_ASSERT(mult_descriptor);
        mult_descriptor->appendSubDescriptorUIDs(descriptor);
    } else {
        mFileSourcePackage->setDescriptor(descriptor);
    }
    descriptor->setLinkedTrackID(mTrackId);
    if (mDuration >= 0)
        descriptor->setContainerDuration(mDuration);
}

void OP1ATimedTextTrack::WriteFileData(File *mxf_file, const mxfKey *key, const string &filename)
{
    FILE *file = 0;
    try
    {
        file = fopen(filename.c_str(), "rb");
        if (!file)
            BMX_EXCEPTION(("Failed to open file '%s': %s", filename.c_str(), bmx_strerror(errno).c_str()));

#if defined(_WIN32)
        struct _stati64 stat_buf;
        if (_fstati64(_fileno(file), &stat_buf) != 0)
#else
        struct stat stat_buf;
        if (fstat(fileno(file), &stat_buf) != 0)
#endif
        {
            BMX_EXCEPTION(("Failed to stat file size '%s'", filename.c_str()));
        }
        int64_t data_size = (int64_t)stat_buf.st_size;

        uint8_t llen = mxf_get_llen(mxf_file->getCFile(), data_size);
        if (llen < 4)
            llen = 4;
        mxf_file->writeFixedKL(key, llen, data_size);

        int read_errno = 0;
        int64_t total_written = 0;
        unsigned char buffer[4096];
        while (total_written <= data_size) {
            size_t num_read = fread(buffer, 1, sizeof(buffer), file);
            read_errno = errno;
            if (num_read > 0) {
                mxf_file->write(buffer, (uint32_t)num_read);
                total_written += num_read;
            }
            if (num_read < sizeof(buffer)) {
                break;
            }
        }
        if (total_written < data_size) {
            BMX_EXCEPTION(("Failed to read all %" PRId64 " bytes from file '%s': %s",
                           data_size, filename.c_str(), bmx_strerror(read_errno).c_str()));
        }
        if (total_written > data_size) {
            BMX_EXCEPTION(("File size is less than read size for file '%s'", filename.c_str()));
        }

        fclose(file);
        file = 0;
    }
    catch (...)
    {
        if (file) {
            fclose(file);
        }
        throw;
    }
}
