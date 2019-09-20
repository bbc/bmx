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

#ifndef BMX_OP1A_TIMED_TEXT_TRACK_H_
#define BMX_OP1A_TIMED_TEXT_TRACK_H_

#include <bmx/mxf_op1a/OP1ATrack.h>
#include <bmx/mxf_helper/TimedTextMXFDescriptorHelper.h>
#include <bmx/mxf_helper/TimedTextManifest.h>
#include <bmx/mxf_helper/TimedTextMXFResourceProvider.h>


namespace bmx
{


class OP1ATimedTextTrack : public OP1ATrack
{
public:
    OP1ATimedTextTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                       mxfRational frame_rate, EssenceType essence_type);
    virtual ~OP1ATimedTextTrack();

    void SetSource(const TimedTextManifest *manifest);
    void SetResourceProvider(TimedTextMXFResourceProvider *provider);

    void SetBodySID(uint32_t id);
    void SetIndexSID(uint32_t id);
    void SetDuration(int64_t duration);

    void WriteIndexTable(mxfpp::File *mxf_file, mxfpp::Partition *index_partition);
    void WriteEssenceContainer(mxfpp::File *mxf_file, mxfpp::Partition *ess_partition);

    size_t GetNumAncillaryResources() const { return mAncillaryResources.size(); }
    uint32_t GetAncillaryResourceStreamId(size_t index) const;
    void WriteAncillaryResource(mxfpp::File *mxf_file, mxfpp::Partition *stream_partition, size_t index);

    void UpdateTrackMetadata(int64_t duration);

public:
    mxfpp::SourcePackage* GetFileSourcePackage() const { return mFileSourcePackage; }

    uint32_t GetBodySID() const  { return mBodySID; }
    uint32_t GetIndexSID() const { return mIndexSID; }
    int64_t GetDuration() const  { return mDuration; }

    int64_t GetStart() const { return mTTStart; }

protected:
    virtual void AddHeaderMetadata(mxfpp::HeaderMetadata *header_metadata, mxfpp::MaterialPackage *material_package,
                                   mxfpp::SourcePackage *file_source_package);
    virtual void PrepareWrite(uint8_t track_count) { (void)track_count; }

private:
    void WriteFileData(mxfpp::File *mxf_file, const mxfKey *key, const std::string &filename);
    void WriteResourceProviderData(mxfpp::File *mxf_file, const mxfKey *key, int64_t data_size);

private:
    TimedTextMXFDescriptorHelper *mTimedTextDescriptorHelper;
    uint32_t mBodySID;
    uint32_t mIndexSID;
    int64_t mDuration;
    int64_t mTTStart;
    mxfpp::SourcePackage *mFileSourcePackage;
    mxfpp::Track *mMPTrack;
    mxfpp::Track *mFPTrack;
    std::string mTTFilename;
    std::vector<TimedTextAncillaryResource> mAncillaryResources;
    std::vector<uint32_t> mInputAncStreamIds;
    TimedTextMXFResourceProvider *mResourceProvider;
};


};


#endif
