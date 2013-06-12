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

#ifndef BMX_AVID_TRACK_H_
#define BMX_AVID_TRACK_H_

#include <string>

#include <libMXF++/MXF.h>
#include <libMXF++/extensions/TaggedValue.h>

#include <bmx/mxf_helper/MXFDescriptorHelper.h>



namespace bmx
{


class AvidClip;

class AvidTrack
{
public:
    static bool IsSupported(EssenceType essence_type, mxfRational sample_rate);

    static AvidTrack* OpenNew(AvidClip *clip, mxfpp::File *file, uint32_t track_index, EssenceType essence_type);

public:
    virtual ~AvidTrack();

    void SetOutputTrackNumber(uint32_t track_number);

    void SetFileSourcePackageUID(mxfUMID package_uid);
    void SetSourceRef(mxfUMID ref_package_uid, uint32_t ref_track_id);

    virtual bool SupportOutputStartOffset() { return false; }
    void SetOutputStartOffset(int64_t offset);

public:
    virtual void PrepareWrite();
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);
    void CompleteWrite();

    virtual uint32_t GetSampleSize();

    mxfRational GetSampleRate() const;

    virtual int64_t GetOutputDuration(bool clip_frame_rate) const;
    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

    int64_t GetFilePosition() const;

public:
    virtual bool IsPicture() const = 0;

    mxfUMID GetFileSourcePackageUID() const { return mFileSourcePackageUID; }

    void SetMaterialTrackId(uint32_t track_id);
    uint32_t GetMaterialTrackId() const { return mMaterialTrackId; }

    uint32_t GetTrackIndex() const { return mTrackIndex; }
    std::pair<mxfUMID, uint32_t> GetSourceReference() const;
    mxfUL GetEssenceContainerUL() const;

    bool IsOutputTrackNumberSet() const   { return mOutputTrackNumberSet; }
    uint32_t GetOutputTrackNumber() const { return mOutputTrackNumber; }

    int64_t GetOutputStartOffset() const { return mOutputStartOffset; }

    mxfpp::MaterialPackage* GetMaterialPackage() const { return mMaterialPackage; }
    mxfpp::SourcePackage* GetFileSourcePackage() const { return mFileSourcePackage; }
    mxfpp::SourcePackage* GetRefSourcePackage() const { return mRefSourcePackage; }
    mxfpp::AvidHeaderMetadata* GetHeaderMetadata() const { return mHeaderMetadata; }
    mxfpp::DataModel* GetDataModel() const { return mDataModel; }

    void SetPhysicalSourceStartTimecode();

protected:
    AvidTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, mxfpp::File *mxf_file);

    virtual uint32_t GetEditUnitSize() const { return mSampleSize; }

    virtual bool HaveCBEIndexTable() { return mSampleSize > 0; }
    virtual void WriteCBEIndexTable(mxfpp::Partition *partition);
    virtual void WriteVBEIndexTable(mxfpp::Partition *partition) { (void)partition; };
    virtual void PreSampleWriting() {};
    virtual void PostSampleWriting(mxfpp::Partition *partition) { (void)partition; };

    void WriteCBEIndexTable(mxfpp::Partition *partition, uint32_t edit_unit_size, mxfpp::IndexTableSegment *&mIndexSegment);

protected:
    AvidClip *mClip;
    uint32_t mTrackIndex;
    mxfpp::File *mMXFFile;

    mxfUMID mSourceRefPackageUID;
    uint32_t mSourceRefTrackId;

    mxfUL mEssenceContainerUL;
    uint32_t mSampleSize;
    uint32_t mTrackNumber;
    mxfKey mEssenceElementKey;
    uint32_t mIndexSID;
    uint32_t mBodySID;

    EssenceType mEssenceType;
    MXFDescriptorHelper *mDescriptorHelper;

    mxfUMID mFileSourcePackageUID;
    uint32_t mMaterialTrackId;
    uint32_t mOutputTrackNumber;
    bool mOutputTrackNumberSet;

    mxfpp::DataModel *mDataModel;
    mxfpp::AvidHeaderMetadata *mHeaderMetadata;
    int64_t mEssenceDataStartPos;
    mxfpp::IndexTableSegment *mCBEIndexSegment;

    mxfpp::MaterialPackage *mMaterialPackage;
    mxfpp::SourcePackage* mFileSourcePackage;
    mxfpp::SourcePackage* mRefSourcePackage;

    mxfpp::TaggedValue *mOMMMobCompleteTaggedValue;
    mxfpp::TaggedValue *mEWCFileMobTaggedValue;

    int64_t mContainerDuration;
    int64_t mContainerSize;
    int64_t mOutputStartOffset;

private:
    void CreateHeaderMetadata();
    void CreateFile();

    mxfpp::TimecodeComponent* GetTimecodeComponent(mxfpp::GenericPackage *package);
};


};



#endif
