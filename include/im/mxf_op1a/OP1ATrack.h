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

#ifndef __IM_OP1A_TRACK_H__
#define __IM_OP1A_TRACK_H__


#include <libMXF++/MXF.h>

#include <im/mxf_helper/MXFDescriptorHelper.h>
#include <im/mxf_op1a/OP1AContentPackage.h>
#include <im/mxf_op1a/OP1AIndexTable.h>



namespace im
{


typedef enum
{
    OP1A_UNKNOWN_ESSENCE = 0,
    OP1A_IEC_DV25,
    OP1A_DVBASED_DV25,
    OP1A_DV50,
    OP1A_DV100_1080I,
    OP1A_DV100_720P,
    OP1A_D10_30,
    OP1A_D10_40,
    OP1A_D10_50,
    OP1A_AVCI100_1080I,
    OP1A_AVCI100_1080P,
    OP1A_AVCI100_720P,
    OP1A_AVCI50_1080I,
    OP1A_AVCI50_1080P,
    OP1A_AVCI50_720P,
    OP1A_UNC_SD,
    OP1A_UNC_HD_1080I,
    OP1A_UNC_HD_1080P,
    OP1A_UNC_HD_720P,
    OP1A_MPEG2LG_422P_HL,
    OP1A_MPEG2LG_MP_HL,
    OP1A_MPEG2LG_MP_H14,
    OP1A_VC3_1080P_1235,
    OP1A_VC3_1080P_1237,
    OP1A_VC3_1080P_1238,
    OP1A_VC3_1080I_1241,
    OP1A_VC3_1080I_1242,
    OP1A_VC3_1080I_1243,
    OP1A_VC3_720P_1250,
    OP1A_VC3_720P_1251,
    OP1A_VC3_720P_1252,
    OP1A_VC3_1080P_1253,
    OP1A_PCM
} OP1AEssenceType;


class OP1AFile;

class OP1ATrack
{
public:
    friend class OP1AFile;

public:
    static bool IsSupported(OP1AEssenceType essence_type, bool is_mpeg2lg_720p, mxfRational sample_rate);

    static MXFDescriptorHelper::EssenceType ConvertEssenceType(OP1AEssenceType op1a_essence_type);
    static OP1AEssenceType ConvertEssenceType(MXFDescriptorHelper::EssenceType mh_essence_type);

    static OP1ATrack* Create(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             mxfRational frame_rate, OP1AEssenceType essence_type);

public:
    virtual ~OP1ATrack();

    void SetMaterialTrackNumber(uint32_t track_number);

    void SetLowerLevelSourcePackage(mxfpp::SourcePackage *package, uint32_t track_id, std::string uri);
    void SetLowerLevelSourcePackage(mxfUMID package_uid, uint32_t track_id);

public:
    void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

public:
    uint32_t GetTrackIndex() const { return mTrackIndex; }

    bool IsPicture() const { return mIsPicture; }
    mxfUL GetEssenceContainerUL() const;

    uint32_t GetSampleSize();

protected:
    OP1ATrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
              mxfRational frame_rate, OP1AEssenceType essence_type);

    void AddHeaderMetadata(mxfpp::HeaderMetadata *header_metadata, mxfpp::MaterialPackage *material_package,
                           mxfpp::SourcePackage *file_source_package);

protected:
    virtual void PrepareWrite(uint8_t picture_track_count, uint8_t sound_track_count) = 0;
    virtual void WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples);
    virtual void CompleteWrite() {}

    void CompleteEssenceKeyAndTrackNum(uint8_t track_count);

    bool IsMaterialTrackNumberSet() const { return mMaterialTrackNumberSet; }
    uint32_t GetMaterialTrackNumber() const { return mMaterialTrackNumber; }

protected:
    OP1AFile *mOP1AFile;
    OP1AContentPackageManager *mCPManager;
    OP1AIndexTable *mIndexTable;
    uint32_t mTrackIndex;
    uint32_t mTrackId;
    uint32_t mMaterialTrackNumber;
    bool mMaterialTrackNumberSet;
    bool mIsPicture;
    mxfRational mFrameRate;
    uint32_t mTrackNumber;
    mxfKey mEssenceElementKey;
    uint8_t mTrackTypeNumber;

    OP1AEssenceType mEssenceType;
    MXFDescriptorHelper *mDescriptorHelper;

private:
    bool mHaveLowerLevelSourcePackage;
    mxfpp::SourcePackage *mLowerLevelSourcePackage;
    mxfUMID mLowerLevelSourcePackageUID;
    uint32_t mLowerLevelTrackId;
    std::string mLowerLevelURI;
};


};



#endif

