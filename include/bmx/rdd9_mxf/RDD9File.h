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

#ifndef BMX_RDD9_FILE_H_
#define BMX_RDD9_FILE_H_

#include <set>
#include <map>
#include <vector>

#include <bmx/rdd9_mxf/RDD9Track.h>
#include <bmx/rdd9_mxf/RDD9MPEG2LGTrack.h>
#include <bmx/rdd9_mxf/RDD9PCMTrack.h>
#include <bmx/BMXTypes.h>
#include <bmx/MXFChecksumFile.h>


#define RDD9_SMPTE_377_2004_FLAVOUR          0x0001
#define RDD9_SINGLE_PASS_WRITE_FLAVOUR       0x0002
#define RDD9_SINGLE_PASS_MD5_WRITE_FLAVOUR   0x0006
#define RDD9_NO_BODY_PART_UPDATE_FLAVOUR     0x0008
#define RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR     0x0010



namespace bmx
{


class RDD9File
{
public:
    friend class RDD9Track;
    friend class RDD9MPEG2LGTrack;
    friend class RDD9PCMTrack;

public:
    RDD9File(int flavour, mxfpp::File *mxf_file, Rational frame_rate);
    virtual ~RDD9File();

    void SetClipName(std::string name);                                 // default ""
    void SetStartTimecode(Timecode start_timecode);                     // default 00:00:00:00, non-drop frame
    void SetHaveInputUserTimecode(bool enable);                         // default false (generated)
    void SetProductInfo(std::string company_name, std::string product_name, mxfProductVersion product_version,
                        std::string version, mxfUUID product_uid);
    void SetCreationDate(mxfTimestamp creation_date);                   // default generated ('now')
    void SetGenerationUID(mxfUUID generation_uid);                      // default generated
    void SetMaterialPackageUID(mxfUMID package_uid);                    // default generated
    void SetFileSourcePackageUID(mxfUMID package_uid);                  // default generated
    void ReserveHeaderMetadataSpace(uint32_t min_bytes);                // default 8192
    void SetPartitionInterval(int64_t frame_count);                     // default 10sec

public:
    void SetOutputStartOffset(int64_t offset);
    void SetOutputEndOffset(int64_t offset);

    RDD9Track* CreateTrack(EssenceType essence_type);

public:
    void PrepareHeaderMetadata();
    void PrepareWrite();
    void WriteUserTimecode(Timecode user_timecode);
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void CompleteWrite();

public:
    mxfpp::HeaderMetadata* GetHeaderMetadata() const { return mHeaderMetadata; }
    mxfpp::DataModel* GetDataModel() const           { return mDataModel; }

    Rational GetFrameRate() const       { return mEditRate; }
    Timecode GetStartTimecode() const   { return mStartTimecode; }
    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

    uint32_t GetNumTracks() const { return (uint32_t)mTracks.size(); }
    RDD9Track* GetTrack(uint32_t track_index);

    std::string GetMD5DigestStr() const { return mMD5DigestStr; }

    int GetFlavour() const { return mFlavour; }

private:
    RDD9IndexTable* GetIndexTable() const                       { return mIndexTable; }
    RDD9ContentPackageManager* GetContentPackageManager() const { return mCPManager; }

    void CreateHeaderMetadata();
    void CreateFile();

    void UpdatePackageMetadata();
    void UpdateTrackMetadata(mxfpp::GenericPackage *package, int64_t origin, int64_t duration);

    void WriteContentPackages(bool final_write);

private:
    int mFlavour;
    mxfpp::File *mMXFFile;

    std::string mClipName;
    Rational mFrameRate;
    Rational mEditRate;
    Timecode mStartTimecode;
    std::string mCompanyName;
    std::string mProductName;
    mxfProductVersion mProductVersion;
    std::string mVersionString;
    mxfUUID mProductUID;
    uint32_t mReserveMinBytes;
    mxfTimestamp mCreationDate;
    mxfUUID mGenerationUID;
    mxfUMID mMaterialPackageUID;
    mxfUMID mFileSourcePackageUID;

    int64_t mOutputStartOffset;
    int64_t mOutputEndOffset;

    bool mFirstWrite;

    int64_t mPartitionInterval;
    int64_t mPartitionFrameCount;

    std::vector<RDD9Track*> mTracks;
    std::map<uint32_t, RDD9Track*> mTrackMap;
    std::map<MXFDataDefEnum, uint8_t> mTrackCounts;
    bool mHaveANCTrack;
    bool mHaveVBITrack;

    std::set<mxfUL> mEssenceContainerULs;

    mxfpp::DataModel *mDataModel;
    mxfpp::HeaderMetadata *mHeaderMetadata;
    int64_t mHeaderMetadataEndPos;

    mxfpp::MaterialPackage *mMaterialPackage;
    mxfpp::SourcePackage *mFileSourcePackage;

    RDD9IndexTable *mIndexTable;
    RDD9ContentPackageManager *mCPManager;

    MXFChecksumFile *mMXFChecksumFile;
    std::string mMD5DigestStr;
};


};



#endif

