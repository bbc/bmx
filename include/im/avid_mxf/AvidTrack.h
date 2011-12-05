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

#ifndef __IM_AVID_TRACK_H__
#define __IM_AVID_TRACK_H__

#include <string>

#include <libMXF++/MXF.h>

#include <im/mxf_helper/MXFDescriptorHelper.h>



namespace im
{


typedef enum
{
    AVID_UNKNOWN_ESSENCE = 0,
    AVID_IEC_DV25,
    AVID_DVBASED_DV25,
    AVID_DV50,
    AVID_DV100_1080I,
    AVID_DV100_720P,
    AVID_MPEG2LG_422P_HL,
    AVID_MPEG2LG_MP_HL,
    AVID_MPEG2LG_MP_H14,
    AVID_MJPEG_2_1,
    AVID_MJPEG_3_1,
    AVID_MJPEG_10_1,
    AVID_MJPEG_20_1,
    AVID_MJPEG_4_1M,
    AVID_MJPEG_10_1M,
    AVID_MJPEG_15_1S,
    AVID_D10_30,
    AVID_D10_40,
    AVID_D10_50,
    AVID_AVCI100_1080I,
    AVID_AVCI100_1080P,
    AVID_AVCI100_720P,
    AVID_AVCI50_1080I,
    AVID_AVCI50_1080P,
    AVID_AVCI50_720P,
    AVID_VC3_1080P_1235,
    AVID_VC3_1080P_1237,
    AVID_VC3_1080P_1238,
    AVID_VC3_1080I_1241,
    AVID_VC3_1080I_1242,
    AVID_VC3_1080I_1243,
    AVID_VC3_720P_1250,
    AVID_VC3_720P_1251,
    AVID_VC3_720P_1252,
    AVID_VC3_1080P_1253,
    AVID_UNC_SD,
    AVID_UNC_HD_1080I,
    AVID_UNC_HD_1080P,
    AVID_UNC_HD_720P,
    AVID_10BIT_UNC_SD,
    AVID_10BIT_UNC_HD_1080I,
    AVID_10BIT_UNC_HD_1080P,
    AVID_10BIT_UNC_HD_720P,
    AVID_PCM,
} AvidEssenceType;


class AvidClip;

class AvidTrack
{
public:
    static bool IsSupported(AvidEssenceType essence_type, bool is_mpeg2lg_720p, mxfRational sample_rate);

    static MXFDescriptorHelper::EssenceType ConvertEssenceType(AvidEssenceType avid_essence_type);
    static AvidEssenceType ConvertEssenceType(MXFDescriptorHelper::EssenceType mh_essence_type);

    static AvidTrack* OpenNew(AvidClip *clip, std::string filename, uint32_t track_index, AvidEssenceType essence_type);

public:
    virtual ~AvidTrack();

    void SetOutputTrackNumber(uint32_t track_number);

    void SetFileSourcePackageUID(mxfUMID package_uid);
    void SetSourceRef(mxfUMID ref_package_uid, uint32_t ref_track_id);

    void SetOutputEndOffset(int64_t offset);

public:
    virtual void PrepareWrite();
    virtual void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);
    void CompleteWrite();

    virtual uint32_t GetSampleSize();

    mxfRational GetSampleRate() const;

    bool HasValidDuration() const;
    int64_t GetOutputDuration(bool clip_frame_rate) const;
    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

public:
    virtual bool IsPicture() const = 0;

    void SetMaterialTrackId(uint32_t track_id);
    uint32_t GetMaterialTrackId() const { return mMaterialTrackId; }

    uint32_t GetTrackIndex() const { return mTrackIndex; }
    std::pair<mxfUMID, uint32_t> GetSourceReference() const;
    mxfUL GetEssenceContainerUL() const;

    bool IsOutputTrackNumberSet() const   { return mOutputTrackNumberSet; }
    uint32_t GetOutputTrackNumber() const { return mOutputTrackNumber; }

    mxfpp::MaterialPackage* GetMaterialPackage() const { return mMaterialPackage; }
    mxfpp::SourcePackage* GetFileSourcePackage() const { return mFileSourcePackage; }
    mxfpp::SourcePackage* GetRefSourcePackage() const { return mRefSourcePackage; }
    mxfpp::AvidHeaderMetadata* GetHeaderMetadata() const { return mHeaderMetadata; }
    mxfpp::DataModel* GetDataModel() const { return mDataModel; }

protected:
    AvidTrack(AvidClip *clip, uint32_t track_index, AvidEssenceType essence_type, mxfpp::File *mxf_file);

    virtual uint32_t GetImageStartOffset() { return 0; }

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
    uint32_t mImageStartOffset;
    uint32_t mTrackNumber;
    mxfKey mEssenceElementKey;
    uint32_t mIndexSID;
    uint32_t mBodySID;

    AvidEssenceType mEssenceType;
    MXFDescriptorHelper *mDescriptorHelper;

    mxfUMID mFileSourcePackageUID;
    uint32_t mMaterialTrackId;
    uint32_t mOutputTrackNumber;
    bool mOutputTrackNumberSet;

    mxfpp::DataModel *mDataModel;
    mxfpp::AvidHeaderMetadata *mHeaderMetadata;
    int64_t mHeaderMetadataStartPos;
    int64_t mEssenceDataStartPos;
    mxfpp::IndexTableSegment *mCBEIndexSegment;

    mxfpp::MaterialPackage *mMaterialPackage;
    mxfpp::SourcePackage* mFileSourcePackage;
    mxfpp::SourcePackage* mRefSourcePackage;

    int64_t mContainerDuration;
    int64_t mContainerSize;
    int64_t mOutputEndOffset;

private:
    void CreateHeaderMetadata();
    void CreateFile();
};


};



#endif
