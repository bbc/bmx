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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS

#include <cstring>

#include <im/avid_mxf/AvidTrack.h>
#include <im/avid_mxf/AvidDVTrack.h>
#include <im/avid_mxf/AvidMPEG2LGTrack.h>
#include <im/avid_mxf/AvidMJPEGTrack.h>
#include <im/avid_mxf/AvidD10Track.h>
#include <im/avid_mxf/AvidAVCITrack.h>
#include <im/avid_mxf/AvidUncTrack.h>
#include <im/avid_mxf/AvidVC3Track.h>
#include <im/avid_mxf/AvidPCMTrack.h>
#include <im/avid_mxf/AvidClip.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace im;
using namespace mxfpp;


#define HEADER_PARTITION    0
#define BODY_PARTITION      1
#define FOOTER_PARTITION    2


static const uint64_t FIXED_BODY_PP_OFFSET  = 0x40020;
static const uint32_t AV_TRACK_ID           = 1;
static const char VIDEO_TRACK_NAME[]        = "V1";
static const char AUDIO_TRACK_NAME[]        = "A1";
static const uint8_t LLEN                   = 9;



typedef struct
{
    AvidEssenceType avid_essence_type;
    MXFDescriptorHelper::EssenceType mh_essence_type;
} EssenceTypeMap;

static const EssenceTypeMap ESSENCE_TYPE_MAP[] =
{
    {AVID_IEC_DV25,             MXFDescriptorHelper::IEC_DV25},
    {AVID_DVBASED_DV25,         MXFDescriptorHelper::DVBASED_DV25},
    {AVID_DV50,                 MXFDescriptorHelper::DV50},
    {AVID_DV100_1080I,          MXFDescriptorHelper::DV100_1080I},
    {AVID_DV100_720P,           MXFDescriptorHelper::DV100_720P},
    {AVID_MPEG2LG_422P_HL,      MXFDescriptorHelper::MPEG2LG_422P_HL},
    {AVID_MPEG2LG_MP_HL,        MXFDescriptorHelper::MPEG2LG_MP_HL},
    {AVID_MPEG2LG_MP_H14,       MXFDescriptorHelper::MPEG2LG_MP_H14},
    {AVID_MJPEG_2_1,            MXFDescriptorHelper::MJPEG_2_1},
    {AVID_MJPEG_3_1,            MXFDescriptorHelper::MJPEG_3_1},
    {AVID_MJPEG_10_1,           MXFDescriptorHelper::MJPEG_10_1},
    {AVID_MJPEG_20_1,           MXFDescriptorHelper::MJPEG_20_1},
    {AVID_MJPEG_4_1M,           MXFDescriptorHelper::MJPEG_4_1M},
    {AVID_MJPEG_10_1M,          MXFDescriptorHelper::MJPEG_10_1M},
    {AVID_MJPEG_15_1S,          MXFDescriptorHelper::MJPEG_15_1S},
    {AVID_D10_30,               MXFDescriptorHelper::D10_30},
    {AVID_D10_40,               MXFDescriptorHelper::D10_40},
    {AVID_D10_50,               MXFDescriptorHelper::D10_50},
    {AVID_AVCI100_1080I,        MXFDescriptorHelper::AVCI100_1080I},
    {AVID_AVCI100_1080P,        MXFDescriptorHelper::AVCI100_1080P},
    {AVID_AVCI100_720P,         MXFDescriptorHelper::AVCI100_720P},
    {AVID_AVCI50_1080I,         MXFDescriptorHelper::AVCI50_1080I},
    {AVID_AVCI50_1080P,         MXFDescriptorHelper::AVCI50_1080P},
    {AVID_AVCI50_720P,          MXFDescriptorHelper::AVCI50_720P},
    {AVID_VC3_1080P_1235,       MXFDescriptorHelper::VC3_1080P_1235},
    {AVID_VC3_1080P_1237,       MXFDescriptorHelper::VC3_1080P_1237},
    {AVID_VC3_1080P_1238,       MXFDescriptorHelper::VC3_1080P_1238},
    {AVID_VC3_1080I_1241,       MXFDescriptorHelper::VC3_1080I_1241},
    {AVID_VC3_1080I_1242,       MXFDescriptorHelper::VC3_1080I_1242},
    {AVID_VC3_1080I_1243,       MXFDescriptorHelper::VC3_1080I_1243},
    {AVID_VC3_720P_1250,        MXFDescriptorHelper::VC3_720P_1250},
    {AVID_VC3_720P_1251,        MXFDescriptorHelper::VC3_720P_1251},
    {AVID_VC3_720P_1252,        MXFDescriptorHelper::VC3_720P_1252},
    {AVID_VC3_1080P_1253,       MXFDescriptorHelper::VC3_1080P_1253},
    {AVID_UNC_SD,               MXFDescriptorHelper::UNC_SD},
    {AVID_UNC_HD_1080I,         MXFDescriptorHelper::UNC_HD_1080I},
    {AVID_UNC_HD_1080P,         MXFDescriptorHelper::UNC_HD_1080P},
    {AVID_UNC_HD_720P,          MXFDescriptorHelper::UNC_HD_720P},
    {AVID_10BIT_UNC_SD,         MXFDescriptorHelper::AVID_10BIT_UNC_SD},
    {AVID_10BIT_UNC_HD_1080I,   MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080I},
    {AVID_10BIT_UNC_HD_1080P,   MXFDescriptorHelper::AVID_10BIT_UNC_HD_1080P},
    {AVID_10BIT_UNC_HD_720P,    MXFDescriptorHelper::AVID_10BIT_UNC_HD_720P},
    {AVID_PCM,                  MXFDescriptorHelper::WAVE_PCM},
};


typedef struct
{
    AvidEssenceType essence_type;
    bool is_mpeg2lg_720p;
    mxfRational sample_rate[10];
} AvidSampleRateSupport;

static const AvidSampleRateSupport AVID_SAMPLE_RATE_SUPPORT[] =
{
    {AVID_IEC_DV25,           false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_DVBASED_DV25,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_DV50,               false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_DV100_1080I,        false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_DV100_720P,         false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_MPEG2LG_422P_HL,    false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MPEG2LG_422P_HL,    true,     {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_MPEG2LG_MP_HL,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MPEG2LG_MP_HL,      true,     {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_MPEG2LG_MP_H14,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_D10_30,             false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_D10_40,             false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_D10_50,             false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_AVCI100_1080I,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_AVCI100_1080P,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_AVCI100_720P,       false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_AVCI50_1080I,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_AVCI50_1080P,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_AVCI50_720P,        false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_MJPEG_2_1,          false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MJPEG_3_1,          false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MJPEG_10_1,         false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MJPEG_20_1,         false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MJPEG_4_1M,         false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MJPEG_10_1M,        false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_MJPEG_15_1S,        false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_1080P_1235,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_1080P_1237,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_1080P_1238,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_1080I_1241,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_1080I_1242,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_1080I_1243,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_720P_1250,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_720P_1251,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_720P_1252,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_VC3_1080P_1253,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_UNC_SD,             false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_UNC_HD_1080I,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_UNC_HD_1080P,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_UNC_HD_720P,        false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_10BIT_UNC_SD,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_10BIT_UNC_HD_1080I, false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_10BIT_UNC_HD_1080P, false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_10BIT_UNC_HD_720P,  false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_PCM,                false,    {{48000, 1}, {0, 0}}},
};



bool AvidTrack::IsSupported(AvidEssenceType essence_type, bool is_mpeg2lg_720p, mxfRational sample_rate)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(AVID_SAMPLE_RATE_SUPPORT); i++) {
        if (essence_type == AVID_SAMPLE_RATE_SUPPORT[i].essence_type &&
            is_mpeg2lg_720p == AVID_SAMPLE_RATE_SUPPORT[i].is_mpeg2lg_720p)
        {
            size_t j = 0;
            while (AVID_SAMPLE_RATE_SUPPORT[i].sample_rate[j].numerator) {
                if (sample_rate == AVID_SAMPLE_RATE_SUPPORT[i].sample_rate[j])
                    return true;
                j++;
            }
        }
    }

    return false;
}

MXFDescriptorHelper::EssenceType AvidTrack::ConvertEssenceType(AvidEssenceType avid_essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (ESSENCE_TYPE_MAP[i].avid_essence_type == avid_essence_type)
            return ESSENCE_TYPE_MAP[i].mh_essence_type;
    }

    return MXFDescriptorHelper::UNKNOWN_ESSENCE;
}

AvidEssenceType AvidTrack::ConvertEssenceType(MXFDescriptorHelper::EssenceType mh_essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (ESSENCE_TYPE_MAP[i].mh_essence_type == mh_essence_type)
            return ESSENCE_TYPE_MAP[i].avid_essence_type;
    }

    return AVID_UNKNOWN_ESSENCE;
}

AvidTrack* AvidTrack::OpenNew(AvidClip *clip, string filename, uint32_t track_index, AvidEssenceType essence_type)
{
    File *file = File::openNew(filename);

    switch (essence_type)
    {
        case AVID_IEC_DV25:
        case AVID_DVBASED_DV25:
        case AVID_DV50:
        case AVID_DV100_1080I:
        case AVID_DV100_720P:
            return new AvidDVTrack(clip, track_index, essence_type, file);
        case AVID_MPEG2LG_422P_HL:
        case AVID_MPEG2LG_MP_HL:
        case AVID_MPEG2LG_MP_H14:
            return new AvidMPEG2LGTrack(clip, track_index, essence_type, file);
        case AVID_MJPEG_2_1:
        case AVID_MJPEG_3_1:
        case AVID_MJPEG_10_1:
        case AVID_MJPEG_20_1:
        case AVID_MJPEG_4_1M:
        case AVID_MJPEG_10_1M:
        case AVID_MJPEG_15_1S:
            return new AvidMJPEGTrack(clip, track_index, essence_type, file);
        case AVID_D10_30:
        case AVID_D10_40:
        case AVID_D10_50:
            return new AvidD10Track(clip, track_index, essence_type, file);
        case AVID_AVCI100_1080I:
        case AVID_AVCI100_1080P:
        case AVID_AVCI100_720P:
        case AVID_AVCI50_1080I:
        case AVID_AVCI50_1080P:
        case AVID_AVCI50_720P:
            return new AvidAVCITrack(clip, track_index, essence_type, file);
        case AVID_VC3_1080P_1235:
        case AVID_VC3_1080P_1237:
        case AVID_VC3_1080P_1238:
        case AVID_VC3_1080I_1241:
        case AVID_VC3_1080I_1242:
        case AVID_VC3_1080I_1243:
        case AVID_VC3_720P_1250:
        case AVID_VC3_720P_1251:
        case AVID_VC3_720P_1252:
        case AVID_VC3_1080P_1253:
            return new AvidVC3Track(clip, track_index, essence_type, file);
        case AVID_UNC_SD:
        case AVID_UNC_HD_1080I:
        case AVID_UNC_HD_1080P:
        case AVID_UNC_HD_720P:
        case AVID_10BIT_UNC_SD:
        case AVID_10BIT_UNC_HD_1080I:
        case AVID_10BIT_UNC_HD_1080P:
        case AVID_10BIT_UNC_HD_720P:
            return new AvidUncTrack(clip, track_index, essence_type, file);
        case AVID_PCM:
            return new AvidPCMTrack(clip, track_index, essence_type, file);
        case AVID_UNKNOWN_ESSENCE:
            IM_ASSERT(false);
    }

    return 0;
}

AvidTrack::AvidTrack(AvidClip *clip, uint32_t track_index, AvidEssenceType essence_type, File *mxf_file)
{
    mClip = clip;
    mTrackIndex = track_index;
    mMXFFile = mxf_file;
    mSourceRefPackageUID = g_Null_UMID;
    mSourceRefTrackId = 0;
    mSampleSize = 0;
    mImageStartOffset = 0;
    mTrackNumber = 0;
    memset(&mEssenceElementKey, 0, sizeof(mEssenceElementKey));
    mBodySID = 1;
    mIndexSID = 2;
    mxf_generate_aafsdk_umid(&mFileSourcePackageUID);
    mMaterialTrackId = 0;
    mOutputTrackNumber = 0;
    mOutputTrackNumberSet = false;
    mDataModel = 0;
    mHeaderMetadata = 0;
    mHeaderMetadataStartPos = 0;
    mEssenceDataStartPos = 0;
    mCBEIndexSegment = 0;
    mMaterialPackage = 0;
    mFileSourcePackage = 0;
    mRefSourcePackage = 0;
    mContainerDuration = 0;
    mContainerSize = 0;
    mOutputEndOffset = 0;
    memset(&mEssenceContainerUL, 0, sizeof(mEssenceContainerUL));

    mEssenceType = essence_type;
    mDescriptorHelper = MXFDescriptorHelper::Create(ConvertEssenceType(essence_type));
    mDescriptorHelper->SetFlavour(MXFDescriptorHelper::AVID_FLAVOUR);
}

AvidTrack::~AvidTrack()
{
    delete mDescriptorHelper;
    delete mMXFFile;
    delete mDataModel;
    delete mHeaderMetadata;
    delete mCBEIndexSegment;
}

void AvidTrack::SetOutputTrackNumber(uint32_t track_number)
{
    mOutputTrackNumber = track_number;
    mOutputTrackNumberSet = true;
}

void AvidTrack::SetFileSourcePackageUID(mxfUMID package_uid)
{
    mFileSourcePackageUID = package_uid;
}

void AvidTrack::SetSourceRef(mxfUMID ref_package_uid, uint32_t ref_track_id)
{
    mSourceRefPackageUID = ref_package_uid;
    mSourceRefTrackId = ref_track_id;
}

void AvidTrack::SetOutputEndOffset(int64_t offset)
{
    IM_CHECK(offset <= 0);
    mOutputEndOffset = offset;
}

void AvidTrack::PrepareWrite()
{
    mSampleSize = GetSampleSize();
    mImageStartOffset = GetImageStartOffset();

    CreateHeaderMetadata();
    CreateFile();
}

void AvidTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    IM_ASSERT(mMXFFile);
    IM_CHECK(mSampleSize > 0);
    IM_CHECK(size > 0 && num_samples > 0);
    IM_CHECK(size == num_samples * mSampleSize);

    if (mImageStartOffset > 0) {
        mMXFFile->writeZeros(mImageStartOffset);
        mContainerSize += mImageStartOffset;
    }

    IM_CHECK(mMXFFile->write(data, size) == size);
    mContainerSize += size;

    mContainerDuration += num_samples;
}

void AvidTrack::CompleteWrite()
{
    IM_ASSERT(mMXFFile);


    // complete writing of samples

    PostSampleWriting(&mMXFFile->getPartition(BODY_PARTITION));

    int64_t file_pos = mMXFFile->tell();
    mMXFFile->seek(mEssenceDataStartPos, SEEK_SET);
    mMXFFile->writeFixedKL(&mEssenceElementKey, LLEN, file_pos - (mEssenceDataStartPos + mxfKey_extlen + LLEN));
    mMXFFile->seek(file_pos, SEEK_SET);
    mMXFFile->getPartition(BODY_PARTITION).fillToKag(mMXFFile);
    

    // write the footer partition pack and header metadata

    file_pos = mMXFFile->tell();
    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(OpenIncomplete, Footer));
    footer_partition.setKagSize(0x100);
    footer_partition.setIndexSID(mIndexSID);
    footer_partition.setBodySID(0);
    // sample files have index table positioned 199 bytes after start of partition
    // 57 = 0x20 + 16 + 9
    IM_ASSERT(mMXFFile->getMinLLen() == 9);
    IM_CHECK(mxf_write_partition(mMXFFile->getCFile(), footer_partition.getCPartition()));
    mMXFFile->fillToPosition(file_pos + footer_partition.getKagSize() - 57);


    // write the index table

    if (HaveCBEIndexTable())
        WriteCBEIndexTable(&footer_partition);
    else
        WriteVBEIndexTable(&footer_partition);


    // write the RIP

    mMXFFile->writeRIP();


    // update container duration (track durations were updated in AvidClip::CompleteWrite)

    FileDescriptor *file_descriptor = dynamic_cast<FileDescriptor*>(mFileSourcePackage->getDescriptor());
    file_descriptor->setContainerDuration(mContainerDuration);



    // re-write the header metadata in the header partition

    mMXFFile->seek(mHeaderMetadataStartPos, SEEK_SET);
    PositionFillerWriter position_filler_writer(FIXED_BODY_PP_OFFSET);
    mHeaderMetadata->write(mMXFFile, &mMXFFile->getPartition(HEADER_PARTITION), &position_filler_writer);


    // update and re-write the partition packs

    mMXFFile->getPartition(HEADER_PARTITION).setKey(&MXF_PP_K(ClosedComplete, Header));
    mMXFFile->getPartition(BODY_PARTITION).setKey(&MXF_PP_K(ClosedComplete, Body));
    mMXFFile->getPartition(FOOTER_PARTITION).setKey(&MXF_PP_K(ClosedComplete, Footer));
    mMXFFile->updatePartitions();


    // done with the file
    delete mMXFFile;
    mMXFFile = 0;
}

uint32_t AvidTrack::GetSampleSize()
{
    return mDescriptorHelper->GetSampleSize();
}

mxfRational AvidTrack::GetSampleRate() const
{
    return mDescriptorHelper->GetSampleRate();
}

bool AvidTrack::HasValidDuration() const
{
    return mContainerDuration + mOutputEndOffset >= 0;
}

int64_t AvidTrack::GetOutputDuration(bool clip_frame_rate) const
{
    if (mContainerDuration + mOutputEndOffset <= 0)
        return 0;

    if (clip_frame_rate) {
        return convert_duration(GetSampleRate(), mContainerDuration + mOutputEndOffset,
                                mClip->mClipFrameRate, ROUND_AUTO);
    }

    return mContainerDuration + mOutputEndOffset;
}

int64_t AvidTrack::GetDuration() const
{
    if (mContainerDuration + mOutputEndOffset <= 0)
        return 0;

    return mContainerDuration + mOutputEndOffset;
}

int64_t AvidTrack::GetContainerDuration() const
{
    return mContainerDuration;
}

void AvidTrack::SetMaterialTrackId(uint32_t track_id)
{
    mMaterialTrackId = track_id;
}

mxfUL AvidTrack::GetEssenceContainerUL() const
{
    return mEssenceContainerUL;
}

pair<mxfUMID, uint32_t> AvidTrack::GetSourceReference() const
{
    pair<mxfUMID, uint32_t> source_ref;
    source_ref.first = mFileSourcePackageUID;
    source_ref.second = AV_TRACK_ID;

    return source_ref;
}

void AvidTrack::WriteCBEIndexTable(Partition *partition)
{
    IM_ASSERT(mSampleSize > 0);
    IM_ASSERT(!mCBEIndexSegment);

    mxfUUID uuid;
    mxf_generate_uuid(&uuid);

    mCBEIndexSegment = new IndexTableSegment();
    mCBEIndexSegment->setInstanceUID(uuid);
    mCBEIndexSegment->setIndexEditRate(GetSampleRate());
    mCBEIndexSegment->setIndexSID(mIndexSID);
    mCBEIndexSegment->setBodySID(mBodySID);
    mCBEIndexSegment->setEditUnitByteCount(mImageStartOffset + mSampleSize);
    mCBEIndexSegment->setIndexDuration(mContainerDuration);

    KAGFillerWriter kag_filler_writer(partition);
    mCBEIndexSegment->write(mMXFFile, partition, &kag_filler_writer);
}

void AvidTrack::CreateHeaderMetadata()
{
    IM_ASSERT(!mHeaderMetadata);

    // descriptor's essence container label will be replaced by the generic AAF-KLV (aka MXF) label
    mEssenceContainerUL = mDescriptorHelper->GetEssenceContainerUL();


    // create the Avid header metadata
    mDataModel = new DataModel();
    mHeaderMetadata = new AvidHeaderMetadata(mDataModel);


    // Meta-dictionary
    mHeaderMetadata->createDefaultMetaDictionary();

    // Preface
    Preface *preface = new Preface(mHeaderMetadata);
    preface->setInt16Item(&MXF_ITEM_K(Preface, ByteOrder), 0x4949); // little-endian
    preface->setUInt32Item(&MXF_ITEM_K(Preface, ObjectModelVersion), 1);
    preface->setVersion(0x0101); // AAF SDK version 1.1
    preface->setLastModifiedDate(mClip->mCreationDate);
    preface->setOperationalPattern(MXF_OP_L(atom, NTracks_NSourceClips));
    preface->appendEssenceContainers(mEssenceContainerUL);
    if (!mClip->mProjectName.empty())
        preface->setStringItem(&MXF_ITEM_K(Preface, ProjectName), mClip->mProjectName);
    preface->setRationalItem(&MXF_ITEM_K(Preface, ProjectEditRate), mClip->mClipFrameRate);
    preface->setUMIDItem(&MXF_ITEM_K(Preface, MasterMobID), mClip->mMaterialPackageUID);
    preface->setUMIDItem(&MXF_ITEM_K(Preface, EssenceFileMobID), mFileSourcePackageUID);
    preface->appendDMSchemes(MXF_DM_L(LegacyDMS1));

    // Preface - Dictionary
    mHeaderMetadata->createDefaultDictionary(preface);

    // Preface - ContentStorage
    ContentStorage *content_storage = new ContentStorage(mHeaderMetadata);
    preface->setContentStorage(content_storage);

    // preface - ContentStorage - MaterialPackage
    mMaterialPackage = dynamic_cast<MaterialPackage*>(mClip->mMaterialPackage->clone(mHeaderMetadata));
    content_storage->appendPackages(mMaterialPackage);

    // Preface - ContentStorage - SourcePackage
    mFileSourcePackage = new SourcePackage(mHeaderMetadata);
    content_storage->appendPackages(mFileSourcePackage);
    mFileSourcePackage->setPackageUID(mFileSourcePackageUID);
    mFileSourcePackage->setPackageCreationDate(mClip->mCreationDate);
    mFileSourcePackage->setPackageModifiedDate(mClip->mCreationDate);
    if (!mClip->mProjectName.empty())
        mFileSourcePackage->attachAvidAttribute("_PJ", mClip->mProjectName);

    // Preface - ContentStorage - SourcePackage - Timeline Track
    Track *track = new Track(mHeaderMetadata);
    mFileSourcePackage->appendTracks(track);
    track->setTrackID(AV_TRACK_ID);
    track->setTrackName(IsPicture() ? VIDEO_TRACK_NAME : AUDIO_TRACK_NAME);
    track->setTrackNumber(mTrackNumber);
    track->setEditRate(GetSampleRate());
    track->setOrigin(0);

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
    Sequence *sequence = new Sequence(mHeaderMetadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(IsPicture() ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
    sequence->setDuration(-1); // updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
    SourceClip *source_clip = new SourceClip(mHeaderMetadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(IsPicture() ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
    source_clip->setDuration(-1); // updated when writing completed
    source_clip->setStartPosition(0); // could be updated by AvidClip
    source_clip->setSourceTrackID(mSourceRefTrackId);
    source_clip->setSourcePackageID(mSourceRefPackageUID);

    // Preface - ContentStorage - SourcePackage - FileDescriptor
    FileDescriptor *descriptor = mDescriptorHelper->CreateFileDescriptor(mHeaderMetadata);
    mFileSourcePackage->setDescriptor(descriptor);
    descriptor->setLinkedTrackID(AV_TRACK_ID);
    descriptor->setContainerDuration(-1);  // updated when writing completed
    // replace essence container label with generic AAF-KLV (aka MXF) label
    descriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));

    // Preface - ContentStorage - EssenceContainerData
    EssenceContainerData *ess_container_data = new EssenceContainerData(mHeaderMetadata);
    content_storage->appendEssenceContainerData(ess_container_data);
    ess_container_data->setLinkedPackageUID(mFileSourcePackageUID);
    ess_container_data->setIndexSID(mIndexSID);
    ess_container_data->setBodySID(mBodySID);

    // add referenced tape or import source package
    size_t i;
    for (i = 0; i < mClip->mTapeSourcePackages.size(); i++) {
        if (mClip->mTapeSourcePackages[i]->getPackageUID() == mSourceRefPackageUID) {
            mRefSourcePackage = dynamic_cast<SourcePackage*>(mClip->mTapeSourcePackages[i]->clone(mHeaderMetadata));
            content_storage->appendPackages(mRefSourcePackage);
            break;
        }
    }
    if (!mRefSourcePackage) {
        for (i = 0; i < mClip->mImportSourcePackages.size(); i++) {
            if (mClip->mImportSourcePackages[i]->getPackageUID() == mSourceRefPackageUID) {
                mRefSourcePackage = dynamic_cast<SourcePackage*>(mClip->mImportSourcePackages[i]->clone(mHeaderMetadata));
                content_storage->appendPackages(mRefSourcePackage);
                break;
            }
        }
    }

    // Preface - Identification
    Identification *ident = new Identification(mHeaderMetadata);
    preface->appendIdentifications(ident);
    ident->initialise(mClip->mCompanyName, mClip->mProductName, mClip->mVersionString, mClip->mProductUID);
    ident->setProductVersion(mClip->mProductVersion);
    ident->setModificationDate(mClip->mCreationDate);
    ident->setThisGenerationUID(mClip->mGenerationUID);
    // ProductVersion type in AAF/Avid differs from MXF. The last record member (release aka type) is a uint8
    ident->setAvidProductVersion(&MXF_ITEM_K(Identification, ToolkitVersion), mHeaderMetadata->getToolkitVersion());
}

void AvidTrack::CreateFile()
{
    IM_ASSERT(mHeaderMetadata);


    // set the minimum llen
    mMXFFile->setMinLLen(LLEN);


    // write the header partition pack and header metadata

    Partition &header_partition = mMXFFile->createPartition();
    header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setIndexSID(0);
    header_partition.setBodySID(0);
    header_partition.setKagSize(0x100);
    header_partition.setOperationalPattern(&MXF_OP_L(atom, NTracks_NSourceClips));
    header_partition.addEssenceContainer(&mEssenceContainerUL);
    header_partition.write(mMXFFile);
    header_partition.fillToKag(mMXFFile);

    mHeaderMetadataStartPos = mMXFFile->tell(); // need this position when we re-write the header metadata
    PositionFillerWriter position_filler_writer(FIXED_BODY_PP_OFFSET);
    mHeaderMetadata->write(mMXFFile, &header_partition, &position_filler_writer);


    // write the essence data partition pack

    int64_t file_pos = mMXFFile->tell();
    Partition &ess_partition = mMXFFile->createPartition();
    ess_partition.setKey(&MXF_PP_K(OpenIncomplete, Body));
    ess_partition.setKagSize(IsPicture() ? 0x20000 : 0x1000);
    ess_partition.setIndexSID(0);
    ess_partition.setBodySID(mBodySID);
    // place the first byte of the essence data at the KAG boundary taking into account that the body
    // partition offset (FIXED_BODY_PP_OFFSET) is 0x20 bytes (why?) beyond where it would be expected to be.
    // 57 = 0x20 + 16 + 9
    IM_ASSERT(mMXFFile->getMinLLen() == 9);
    IM_CHECK(mxf_write_partition(mMXFFile->getCFile(), ess_partition.getCPartition()));
    mMXFFile->fillToPosition(file_pos + ess_partition.getKagSize() - 57);


    // essence data start

    mEssenceDataStartPos = mMXFFile->tell(); // need this position when we re-write the key
    mMXFFile->writeFixedKL(&mEssenceElementKey, LLEN, 0);


    PreSampleWriting();
}

