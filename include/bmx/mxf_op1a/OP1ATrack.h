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

#ifndef BMX_OP1A_TRACK_H_
#define BMX_OP1A_TRACK_H_

#include <libMXF++/MXF.h>

#include <bmx/mxf_helper/MXFDescriptorHelper.h>
#include <bmx/mxf_op1a/OP1AContentPackage.h>
#include <bmx/mxf_op1a/OP1AIndexTable.h>



namespace bmx
{


class OP1AFile;

class OP1ATrack
{
public:
    friend class OP1AFile;

public:
    static bool IsSupported(EssenceType essence_type, mxfRational sample_rate);

    static OP1ATrack* Create(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             mxfRational frame_rate, EssenceType essence_type);

public:
    virtual ~OP1ATrack();

    void SetOutputTrackNumber(uint32_t track_number);

    void SetLowerLevelSourcePackage(mxfpp::SourcePackage *package, uint32_t track_id, std::string uri);
    void SetLowerLevelSourcePackage(mxfUMID package_uid, uint32_t track_id);

public:
    void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

public:
    uint32_t GetTrackIndex() const { return mTrackIndex; }

    MXFDataDefEnum GetDataDef() const { return mDataDef; }
    mxfUL GetEssenceContainerUL() const;

    uint32_t GetSampleSize();

    mxfRational GetEditRate() const { return mEditRate; }

    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

protected:
    OP1ATrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
              mxfRational frame_rate, EssenceType essence_type);

    void AddHeaderMetadata(mxfpp::HeaderMetadata *header_metadata, mxfpp::MaterialPackage *material_package,
                           mxfpp::SourcePackage *file_source_package);

protected:
    virtual void PrepareWrite(uint8_t track_count) = 0;
    virtual void WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples);
    virtual void WriteSampleInt(const CDataBuffer *data_array, uint32_t array_size);
    virtual void CompleteWrite() {}

    void CompleteEssenceKeyAndTrackNum(uint8_t track_count);

    bool IsOutputTrackNumberSet() const { return mOutputTrackNumberSet; }
    uint32_t GetOutputTrackNumber() const { return mOutputTrackNumber; }

protected:
    OP1AFile *mOP1AFile;
    OP1AContentPackageManager *mCPManager;
    OP1AIndexTable *mIndexTable;
    uint32_t mTrackIndex;
    uint32_t mTrackId;
    uint32_t mOutputTrackNumber;
    bool mOutputTrackNumberSet;
    MXFDataDefEnum mDataDef;
    mxfRational mFrameRate;
    mxfRational mEditRate;
    uint32_t mTrackNumber;
    mxfKey mEssenceElementKey;
    uint8_t mTrackTypeNumber;

    EssenceType mEssenceType;
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

