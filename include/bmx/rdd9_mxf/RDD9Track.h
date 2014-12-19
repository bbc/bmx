/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_RDD9_TRACK_H_
#define BMX_RDD9_TRACK_H_


#include <libMXF++/MXF.h>

#include <bmx/mxf_helper/MXFDescriptorHelper.h>
#include <bmx/rdd9_mxf/RDD9ContentPackage.h>
#include <bmx/rdd9_mxf/RDD9IndexTable.h>



namespace bmx
{


class RDD9File;

class RDD9Track
{
public:
    friend class RDD9File;

public:
    static bool IsSupported(EssenceType essence_type, Rational sample_rate);

    static RDD9Track* Create(RDD9File *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             Rational frame_rate, EssenceType essence_type);

public:
    virtual ~RDD9Track();

    void SetOutputTrackNumber(uint32_t track_number);

public:
    void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

public:
    uint32_t GetTrackIndex() const { return mTrackIndex; }

    MXFDataDefEnum GetDataDef() const { return mDataDef; }
    mxfUL GetEssenceContainerUL() const;

    uint32_t GetSampleSize();

    Rational GetEditRate() const { return mEditRate; }

    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

protected:
    RDD9Track(RDD9File *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
              Rational frame_rate, EssenceType essence_type);

    void AddHeaderMetadata(mxfpp::HeaderMetadata *header_metadata, mxfpp::MaterialPackage *material_package,
                           mxfpp::SourcePackage *file_source_package);

protected:
    virtual void PrepareWrite(uint8_t track_count) = 0;
    virtual void WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples);
    virtual void CompleteWrite() {}

    void CompleteEssenceKeyAndTrackNum(uint8_t track_count);

    bool IsOutputTrackNumberSet() const   { return mOutputTrackNumberSet; }
    uint32_t GetOutputTrackNumber() const { return mOutputTrackNumber; }

protected:
    RDD9File *mRDD9File;
    RDD9ContentPackageManager *mCPManager;
    RDD9IndexTable *mIndexTable;
    uint32_t mTrackIndex;
    uint32_t mTrackId;
    uint32_t mOutputTrackNumber;
    bool mOutputTrackNumberSet;
    MXFDataDefEnum mDataDef;
    Rational mFrameRate;
    Rational mEditRate;
    uint32_t mTrackNumber;
    mxfKey mEssenceElementKey;
    uint8_t mTrackTypeNumber;

    EssenceType mEssenceType;
    MXFDescriptorHelper *mDescriptorHelper;
};


};



#endif

