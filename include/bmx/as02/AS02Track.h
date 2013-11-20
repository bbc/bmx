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

#ifndef BMX_AS02_TRACK_H_
#define BMX_AS02_TRACK_H_


#include <bmx/as02/AS02Bundle.h>
#include <bmx/mxf_helper/MXFDescriptorHelper.h>
#include <bmx/Checksum.h>



namespace bmx
{


class AS02Clip;

class AS02Track
{
public:
    static bool IsSupported(EssenceType essence_type, mxfRational sample_rate);

    static AS02Track* OpenNew(AS02Clip *clip, mxfpp::File *file, std::string rel_uri, uint32_t track_index,
                              EssenceType essence_type);

public:
    virtual ~AS02Track();

    void SetFileSourcePackageUID(mxfUMID package_uid);
    void SetOutputTrackNumber(uint32_t track_number);
    void SetMICType(MICType type);
    void SetMICScope(MICScope scope);

    void SetLowerLevelSourcePackage(mxfpp::SourcePackage *package, uint32_t track_id, std::string uri);
    void SetLowerLevelSourcePackage(mxfUMID package_uid, uint32_t track_id);

    void SetOutputStartOffset(int64_t offset);
    void SetOutputEndOffset(int64_t offset);

public:
    virtual void PrepareWrite();
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples) = 0;
    void CompleteWrite();

    void UpdatePackageMetadata(mxfpp::GenericPackage *package);

public:
    uint32_t GetTrackIndex() const { return mTrackIndex; }
    bool IsOutputTrackNumberSet() const { return mOutputTrackNumberSet; }
    uint32_t GetOutputTrackNumber() const { return mOutputTrackNumber; }
    std::string GetRelativeURL() const { return mRelativeURL; }
    EssenceType GetEssenceType() const { return mEssenceType; }
    virtual mxfUL GetEssenceContainerUL() const;
    mxfpp::SourcePackage* GetFileSourcePackage() const { return mFileSourcePackage; }
    bool IsPicture() const { return mIsPicture; }
    mxfRational GetSampleRate() const;
    std::pair<mxfUMID, uint32_t> GetSourceReference() const;

    uint32_t GetSampleSize();

    bool HasValidDuration() const;
    virtual int64_t GetOutputDuration(bool clip_frame_rate) const;
    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

    virtual int64_t ConvertClipDuration(int64_t clip_duration) const;

protected:
    AS02Track(AS02Clip *clip, uint32_t track_index, EssenceType essence_type, mxfpp::File *mxf_file,
              std::string rel_uri);

    virtual bool HaveCBEIndexTable() { return mSampleSize > 0; }
    virtual void WriteCBEIndexTable(mxfpp::Partition *partition);
    virtual bool HaveVBEIndexEntries() { return true; }
    virtual void WriteVBEIndexTable(mxfpp::Partition *partition) { (void)partition; };
    virtual void PreSampleWriting() {}
    virtual void PostSampleWriting(mxfpp::Partition *partition) { (void)partition; }

protected:
    mxfRational& GetClipFrameRate() const;

    void WriteCBEIndexTable(mxfpp::Partition *partition, uint32_t edit_unit_size, mxfpp::IndexTableSegment *&mIndexSegment);

    void UpdateEssenceOnlyChecksum(const unsigned char *data, uint32_t size);

protected:
    bool mIsPicture;
    uint32_t mTrackNumber;
    uint32_t mIndexSID;
    uint32_t mBodySID;
    uint8_t mLLen;
    uint8_t mEssenceElementLLen;
    uint32_t mKAGSize;
    mxfKey mEssenceElementKey;

    EssenceType mEssenceType;
    MXFDescriptorHelper *mDescriptorHelper;

    uint32_t mSampleSize;

    mxfUMID mMaterialPackageUID;
    mxfUMID mFileSourcePackageUID;

    int64_t mContainerDuration;
    int64_t mContainerSize;
    int64_t mOutputStartOffset;
    int64_t mOutputEndOffset;

    mxfpp::File *mMXFFile;

private:
    void CreateHeaderMetadata();
    void CreateFile();

private:
    AS02ManifestFile *mManifestFile;
    AS02Clip *mClip;
    uint32_t mTrackIndex;
    uint32_t mOutputTrackNumber;
    bool mOutputTrackNumberSet;
    std::string mRelativeURL;

    mxfpp::DataModel *mDataModel;
    mxfpp::HeaderMetadata *mHeaderMetadata;
    int64_t mHeaderMetadataStartPos;
    int64_t mHeaderMetadataEndPos;
    mxfpp::IndexTableSegment *mCBEIndexSegment;

    mxfpp::MaterialPackage* mMaterialPackage;
    mxfpp::SourcePackage* mFileSourcePackage;

    bool mHaveLowerLevelSourcePackage;
    mxfpp::SourcePackage *mLowerLevelSourcePackage;
    mxfUMID mLowerLevelSourcePackageUID;
    uint32_t mLowerLevelTrackId;
    std::string mLowerLevelURI;

    Checksum mEssenceOnlyChecksum;
};


};



#endif

