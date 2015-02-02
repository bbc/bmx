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

#ifndef BMX_OP1A_FILE_H_
#define BMX_OP1A_FILE_H_

#include <vector>
#include <set>
#include <map>

#include <libMXF++/MXF.h>

#include <bmx/mxf_op1a/OP1ATrack.h>
#include <bmx/mxf_op1a/OP1AMPEG2LGTrack.h>
#include <bmx/BMXTypes.h>
#include <bmx/MXFChecksumFile.h>


#define OP1A_DEFAULT_FLAVOUR                0x0000
#define OP1A_MIN_PARTITIONS_FLAVOUR         0x0001      // header and footer partition only
#define OP1A_512_KAG_FLAVOUR                0x0002
#define OP1A_377_2004_FLAVOUR               0x0004
#define OP1A_SINGLE_PASS_WRITE_FLAVOUR      0x0008
#define OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR  0x0018
#define OP1A_NO_BODY_PART_UPDATE_FLAVOUR    0x0020
#define OP1A_BODY_PARTITIONS_FLAVOUR        0x0040      // separate body partitions; index tables not in separate partition
#define OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR    0x0080
#define OP1A_ZERO_MP_TRACK_NUMBER_FLAVOUR   0x0100      // set Material Package Track Number to 0



namespace bmx
{


class OP1AFile
{
public:
    friend class OP1ATrack;
    friend class OP1AMPEG2LGTrack;

public:
    OP1AFile(int flavour, mxfpp::File *mxf_file, mxfRational frame_rate);
    virtual ~OP1AFile();

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
    void SetPartitionInterval(int64_t frame_count);                     // default 0 (single partition)
    void SetInputDuration(int64_t duration);                            // single pass flavours only
    void SetClipWrapped(bool enable);                                   // default false (frame wrapped)
    void SetAddSystemItem(bool enable);                                 // default false, no system item

public:
    void SetOutputStartOffset(int64_t offset);
    void SetOutputEndOffset(int64_t offset);

    OP1ATrack* CreateTrack(EssenceType essence_type);

public:
    void PrepareHeaderMetadata();
    void PrepareWrite();
    void WriteUserTimecode(Timecode user_timecode);
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void CompleteWrite();

public:
    mxfpp::HeaderMetadata* GetHeaderMetadata() const { return mHeaderMetadata; }
    mxfpp::DataModel* GetDataModel() const { return mDataModel; }

    bool IsFrameWrapped() const { return mFrameWrapped; }

    mxfRational GetFrameRate() const { return mEditRate; }

    Timecode GetStartTimecode() const { return mStartTimecode; }

    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

    int64_t GetInputDuration() const { return mInputDuration; }

    uint32_t GetNumTracks() const { return (uint32_t)mTracks.size(); }
    OP1ATrack* GetTrack(uint32_t track_index);

    std::string GetMD5DigestStr() const { return mMD5DigestStr; }

    int GetFlavour() const { return mFlavour; }

private:
    OP1AIndexTable* GetIndexTable() const { return mIndexTable; }
    OP1AContentPackageManager* GetContentPackageManager() const { return mCPManager; }

    void CreateHeaderMetadata();
    void CreateFile();

    void UpdatePackageMetadata();
    void UpdateTrackMetadata(mxfpp::GenericPackage *package, int64_t origin, int64_t duration);

    void WriteContentPackages(bool end_of_samples);

    void SetPartitionsFooterOffset();

private:
    int mFlavour;
    mxfpp::File *mMXFFile;

    std::string mClipName;
    mxfRational mFrameRate;
    mxfRational mEditRate;
    Timecode mStartTimecode;
    std::string mCompanyName;
    std::string mProductName;
    mxfProductVersion mProductVersion;
    std::string mVersionString;
    mxfUUID mProductUID;
    uint32_t mReserveMinBytes;
    int64_t mInputDuration;
    mxfTimestamp mCreationDate;
    mxfUUID mGenerationUID;
    mxfUMID mMaterialPackageUID;
    mxfUMID mFileSourcePackageUID;
    bool mFrameWrapped;

    int64_t mOutputStartOffset;
    int64_t mOutputEndOffset;

    std::vector<OP1ATrack*> mTracks;
    std::map<uint32_t, OP1ATrack*> mTrackMap;
    std::map<MXFDataDefEnum, uint8_t> mTrackCounts;
    bool mHaveANCTrack;
    bool mHaveVBITrack;

    mxfpp::DataModel *mDataModel;
    mxfpp::HeaderMetadata *mHeaderMetadata;
    int64_t mHeaderMetadataEndPos;

    std::set<mxfUL> mEssenceContainerULs;

    mxfpp::MaterialPackage *mMaterialPackage;
    mxfpp::SourcePackage *mFileSourcePackage;

    OP1AIndexTable *mIndexTable;
    OP1AContentPackageManager *mCPManager;

    bool mFirstWrite;

    int64_t mPartitionInterval;
    int64_t mPartitionFrameCount;

    uint32_t mKAGSize;
    uint32_t mEssencePartitionKAGSize;

    bool mSupportCompleteSinglePass;
    int64_t mFooterPartitionOffset;

    MXFChecksumFile *mMXFChecksumFile;
    std::string mMD5DigestStr;
};


};



#endif

