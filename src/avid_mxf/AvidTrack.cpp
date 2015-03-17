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
#include <cstdio>

#include <bmx/avid_mxf/AvidTrack.h>
#include <bmx/avid_mxf/AvidDVTrack.h>
#include <bmx/avid_mxf/AvidMPEG2LGTrack.h>
#include <bmx/avid_mxf/AvidMJPEGTrack.h>
#include <bmx/avid_mxf/AvidD10Track.h>
#include <bmx/avid_mxf/AvidAVCITrack.h>
#include <bmx/avid_mxf/AvidUncTrack.h>
#include <bmx/avid_mxf/AvidVC3Track.h>
#include <bmx/avid_mxf/AvidPCMTrack.h>
#include <bmx/avid_mxf/AvidAlphaTrack.h>
#include <bmx/avid_mxf/AvidClip.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
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
    EssenceType essence_type;
    mxfRational sample_rate[10];
} AvidSampleRateSupport;

static const AvidSampleRateSupport AVID_SAMPLE_RATE_SUPPORT[] =
{
    {IEC_DV25,                 {{25, 1}, {30000, 1001}, {0, 0}}},
    {DVBASED_DV25,             {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV50,                     {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV100_1080I,              {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV100_720P,               {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MPEG2LG_422P_HL_1080I,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_422P_HL_1080P,    {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_422P_HL_720P,     {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1920_1080I, {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1920_1080P, {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1440_1080I, {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1440_1080P, {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_720P,       {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MPEG2LG_MP_H14_1080I,     {{25, 1}, {30000, 1001}, {0, 0}}},
    {MPEG2LG_MP_H14_1080P,     {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {D10_30,                   {{25, 1}, {30000, 1001}, {0, 0}}},
    {D10_40,                   {{25, 1}, {30000, 1001}, {0, 0}}},
    {D10_50,                   {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI200_1080I,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI200_1080P,            {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI200_720P,             {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI100_1080I,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI100_1080P,            {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI100_720P,             {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI50_1080I,             {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVCI50_1080P,             {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVCI50_720P,              {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MJPEG_2_1,                {{25, 1}, {30000, 1001}, {0, 0}}},
    {MJPEG_3_1,                {{25, 1}, {30000, 1001}, {0, 0}}},
    {MJPEG_10_1,               {{25, 1}, {30000, 1001}, {0, 0}}},
    {MJPEG_20_1,               {{25, 1}, {30000, 1001}, {0, 0}}},
    {MJPEG_4_1M,               {{25, 1}, {30000, 1001}, {0, 0}}},
    {MJPEG_10_1M,              {{25, 1}, {30000, 1001}, {0, 0}}},
    {MJPEG_15_1S,              {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080P_1235,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080P_1237,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080P_1238,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080I_1241,           {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080I_1242,           {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080I_1243,           {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080I_1244,           {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_720P_1250,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_720P_1251,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_720P_1252,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080P_1253,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_720P_1258,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080P_1259,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {0, 0}}},
    {VC3_1080I_1260,           {{25, 1}, {30000, 1001}, {0, 0}}},
    {UNC_SD,                   {{25, 1}, {30000, 1001}, {0, 0}}},
    {UNC_HD_1080I,             {{25, 1}, {30000, 1001}, {0, 0}}},
    {UNC_HD_1080P,             {{25, 1}, {30000, 1001}, {30, 1}, {50, 1}, {60000, 1001}, {60, 1}, {0, 0}}},
    {UNC_HD_720P,              {{25, 1}, {30000, 1001}, {30, 1}, {50, 1}, {60000, 1001}, {60, 1}, {0, 0}}},
    {AVID_10BIT_UNC_SD,        {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_10BIT_UNC_HD_1080I,  {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_10BIT_UNC_HD_1080P,  {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_10BIT_UNC_HD_720P,   {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_ALPHA_SD,            {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_ALPHA_HD_1080I,      {{25, 1}, {30000, 1001}, {0, 0}}},
    {AVID_ALPHA_HD_1080P,      {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {AVID_ALPHA_HD_720P,       {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {WAVE_PCM,                 {{48000, 1}, {0, 0}}},
};



bool AvidTrack::IsSupported(EssenceType essence_type, mxfRational sample_rate)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(AVID_SAMPLE_RATE_SUPPORT); i++) {
        if (essence_type == AVID_SAMPLE_RATE_SUPPORT[i].essence_type) {
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

AvidTrack* AvidTrack::OpenNew(AvidClip *clip, File *file, uint32_t track_index, EssenceType essence_type)
{
    switch (essence_type)
    {
        case IEC_DV25:
        case DVBASED_DV25:
        case DV50:
        case DV100_1080I:
        case DV100_720P:
            return new AvidDVTrack(clip, track_index, essence_type, file);
        case MPEG2LG_422P_HL_1080I:
        case MPEG2LG_422P_HL_1080P:
        case MPEG2LG_422P_HL_720P:
        case MPEG2LG_MP_HL_1920_1080I:
        case MPEG2LG_MP_HL_1920_1080P:
        case MPEG2LG_MP_HL_1440_1080I:
        case MPEG2LG_MP_HL_1440_1080P:
        case MPEG2LG_MP_HL_720P:
        case MPEG2LG_MP_H14_1080I:
        case MPEG2LG_MP_H14_1080P:
            if (clip->mFlavour & AVID_GROWING_FILE_FLAVOUR)
                BMX_EXCEPTION(("Avid growing file is not supported for format '%s'", essence_type_to_string(essence_type)));
            return new AvidMPEG2LGTrack(clip, track_index, essence_type, file);
        case MJPEG_2_1:
        case MJPEG_3_1:
        case MJPEG_10_1:
        case MJPEG_20_1:
        case MJPEG_4_1M:
        case MJPEG_10_1M:
        case MJPEG_15_1S:
            if (clip->mFlavour & AVID_GROWING_FILE_FLAVOUR)
                BMX_EXCEPTION(("Avid growing file is not supported for format '%s'", essence_type_to_string(essence_type)));
            return new AvidMJPEGTrack(clip, track_index, essence_type, file);
        case D10_30:
        case D10_40:
        case D10_50:
            return new AvidD10Track(clip, track_index, essence_type, file);
        case AVCI200_1080I:
        case AVCI200_1080P:
        case AVCI200_720P:
        case AVCI100_1080I:
        case AVCI100_1080P:
        case AVCI100_720P:
        case AVCI50_1080I:
        case AVCI50_1080P:
        case AVCI50_720P:
            return new AvidAVCITrack(clip, track_index, essence_type, file);
        case VC3_1080P_1235:
        case VC3_1080P_1237:
        case VC3_1080P_1238:
        case VC3_1080I_1241:
        case VC3_1080I_1242:
        case VC3_1080I_1243:
        case VC3_1080I_1244:
        case VC3_720P_1250:
        case VC3_720P_1251:
        case VC3_720P_1252:
        case VC3_1080P_1253:
        case VC3_720P_1258:
        case VC3_1080P_1259:
        case VC3_1080I_1260:
            return new AvidVC3Track(clip, track_index, essence_type, file);
        case UNC_SD:
        case UNC_HD_1080I:
        case UNC_HD_1080P:
        case UNC_HD_720P:
        case AVID_10BIT_UNC_SD:
        case AVID_10BIT_UNC_HD_1080I:
        case AVID_10BIT_UNC_HD_1080P:
        case AVID_10BIT_UNC_HD_720P:
            return new AvidUncTrack(clip, track_index, essence_type, file);
        case AVID_ALPHA_SD:
        case AVID_ALPHA_HD_1080I:
        case AVID_ALPHA_HD_1080P:
        case AVID_ALPHA_HD_720P:
            return new AvidAlphaTrack(clip, track_index, essence_type, file);
        case WAVE_PCM:
            return new AvidPCMTrack(clip, track_index, essence_type, file);
        default:
            BMX_ASSERT(false);
    }

    return 0;
}

AvidTrack::AvidTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, File *mxf_file)
{
    mClip = clip;
    mTrackIndex = track_index;
    mMXFFile = mxf_file;
    mSourceRefPackageUID = g_Null_UMID;
    mSourceRefTrackId = 0;
    mSampleSize = 0;
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
    mEssenceDataStartPos = 0;
    mCBEIndexSegment = 0;
    mMaterialPackage = 0;
    mFileSourcePackage = 0;
    mRefSourcePackage = 0;
    mOMMMobCompleteTaggedValue = 0;
    mEWCFileMobTaggedValue = 0;
    mContainerDuration = 0;
    mContainerSize = 0;
    mOutputStartOffset = 0;
    memset(&mEssenceContainerUL, 0, sizeof(mEssenceContainerUL));

    mEssenceType = essence_type;
    mDescriptorHelper = MXFDescriptorHelper::Create(essence_type);
    mDescriptorHelper->SetFlavour(MXFDESC_SMPTE_377_2004_FLAVOUR | MXFDESC_AVID_FLAVOUR);
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

void AvidTrack::SetOutputStartOffset(int64_t offset)
{
    BMX_CHECK(offset == 0 || SupportOutputStartOffset());
    BMX_CHECK(offset >= 0);

    mOutputStartOffset = offset;
}

void AvidTrack::PrepareWrite()
{
    mSampleSize = GetSampleSize();

    CreateHeaderMetadata();
    CreateFile();
}

void AvidTrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(mMXFFile);
    BMX_CHECK(mSampleSize > 0);
    BMX_CHECK(size > 0 && num_samples > 0);
    BMX_CHECK(size >= num_samples * mSampleSize);

    uint32_t write_size = num_samples * mSampleSize;
    BMX_CHECK(mMXFFile->write(data, write_size) == write_size);
    mContainerSize += write_size;
    mContainerDuration += num_samples;
}

void AvidTrack::CompleteWrite()
{
    BMX_ASSERT(mMXFFile);


    // complete writing of samples

    PostSampleWriting(&mMXFFile->getPartition(BODY_PARTITION));

    int64_t file_pos = mMXFFile->tell();
    mMXFFile->seek(mEssenceDataStartPos, SEEK_SET);
    mMXFFile->writeFixedKL(&mEssenceElementKey, LLEN, file_pos - (mEssenceDataStartPos + mxfKey_extlen + LLEN));
    mMXFFile->seek(file_pos, SEEK_SET);
    mMXFFile->getPartition(BODY_PARTITION).fillToKag(mMXFFile);


    // write the footer partition and RIP to memory first

    mMXFFile->openMemoryFile(8192);


    // write the footer partition pack

    file_pos = mMXFFile->tell();
    Partition &footer_partition = mMXFFile->createPartition();
    footer_partition.setKey(&MXF_PP_K(ClosedComplete, Footer));
    footer_partition.setKagSize(0x100);
    footer_partition.setIndexSID(mIndexSID);
    footer_partition.setBodySID(0);
    // sample files have index table positioned 199 bytes after start of partition
    // 57 = 0x20 + 16 + 9
    BMX_ASSERT(mMXFFile->getMinLLen() == 9);
    BMX_CHECK(mxf_write_partition(mMXFFile->getCFile(), footer_partition.getCPartition()));
    mMXFFile->fillToPosition(file_pos + footer_partition.getKagSize() - 57);


    // write the index table

    if (HaveCBEIndexTable())
        WriteCBEIndexTable(&footer_partition);
    else
        WriteVBEIndexTable(&footer_partition);


    // write the RIP

    mMXFFile->writeRIP();


    // update the footer partition pack and flush memory writes to file

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();


    // update container duration (track durations were updated in AvidClip::CompleteWrite)

    FileDescriptor *file_descriptor = dynamic_cast<FileDescriptor*>(mFileSourcePackage->getDescriptor());
    file_descriptor->setContainerDuration(mContainerDuration);


    // update growing file attributes

    if (mOMMMobCompleteTaggedValue)
        mOMMMobCompleteTaggedValue->setValue(1);
    if (mEWCFileMobTaggedValue)
        mEWCFileMobTaggedValue->setValue(0);


    // re-write header partition to memory first

    mMXFFile->seek(0, SEEK_SET);
    mMXFFile->openMemoryFile(8192);
    mMXFFile->setMemoryPartitionIndexes(0, 0); // overwriting and updating from header pp


    // update and re-write the header partition pack

    Partition &header_partition = mMXFFile->getPartition(HEADER_PARTITION);
    header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
    header_partition.setFooterPartition(footer_partition.getThisPartition());
    header_partition.write(mMXFFile);


    // re-write the header metadata in the header partition

    PositionFillerWriter pos_filler_writer(FIXED_BODY_PP_OFFSET);
    mHeaderMetadata->write(mMXFFile, &header_partition, &pos_filler_writer);


    // update header partition pack and flush memory writes to file

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();


    // update body partition and re-write the partition packs

    if (mClip->mFlavour & AVID_GROWING_FILE_FLAVOUR)
        mMXFFile->getPartition(BODY_PARTITION).setKey(&MXF_PP_K(OpenComplete, Body));
    mMXFFile->updateBodyPartitions(&MXF_PP_K(ClosedComplete, Body));


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

int64_t AvidTrack::GetOutputDuration(bool clip_frame_rate) const
{
    BMX_ASSERT(!clip_frame_rate || mClip->mClipFrameRate == GetSampleRate());

    if (mContainerDuration < mOutputStartOffset)
        return 0;

    return mContainerDuration - mOutputStartOffset;
}

int64_t AvidTrack::GetDuration() const
{
    return mContainerDuration;
}

int64_t AvidTrack::GetContainerDuration() const
{
    return mContainerDuration;
}

int64_t AvidTrack::GetFilePosition() const
{
    return mMXFFile->tell();
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
    BMX_ASSERT(!mCBEIndexSegment);

    mxfUUID uuid;
    mxf_generate_uuid(&uuid);

    mCBEIndexSegment = new IndexTableSegment();
    mCBEIndexSegment->setInstanceUID(uuid);
    mCBEIndexSegment->setIndexEditRate(GetSampleRate());
    mCBEIndexSegment->setIndexSID(mIndexSID);
    mCBEIndexSegment->setBodySID(mBodySID);
    mCBEIndexSegment->setEditUnitByteCount(GetEditUnitSize());
    mCBEIndexSegment->setIndexDuration(mContainerDuration);

    mCBEIndexSegment->write(mMXFFile, partition, 0);
}

void AvidTrack::CreateHeaderMetadata()
{
    BMX_ASSERT(!mHeaderMetadata);

    // initial duration is -1 or GrowingDuration if flavour is growing file
    int64_t track_growing_duration = -1;
    if (mClip->mGrowingDuration >= 0) {
        track_growing_duration = convert_duration(mClip->GetFrameRate(), mClip->mGrowingDuration, GetSampleRate(),
                                                  ROUND_AUTO);
    }

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
    preface->setVersion(MXF_PREFACE_VER(1, 1)); // AAF SDK version
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
    if (mClip->mFlavour & AVID_GROWING_FILE_FLAVOUR) {
        TaggedValue *bundle_tv = mMaterialPackage->appendAvidAttribute("_OMM_BUNDLE", "__AttributeList");
        mOMMMobCompleteTaggedValue = bundle_tv->appendAvidAttribute("_OMM_MOB_COMPLETE", 0);
    }

    // Preface - ContentStorage - SourcePackage
    mFileSourcePackage = new SourcePackage(mHeaderMetadata);
    content_storage->appendPackages(mFileSourcePackage);
    mFileSourcePackage->setPackageUID(mFileSourcePackageUID);
    mFileSourcePackage->setPackageCreationDate(mClip->mCreationDate);
    mFileSourcePackage->setPackageModifiedDate(mClip->mCreationDate);
    if (!mClip->mProjectName.empty())
        mFileSourcePackage->appendAvidAttribute("_PJ", mClip->mProjectName);
    if (mClip->mFlavour & AVID_GROWING_FILE_FLAVOUR)
        mEWCFileMobTaggedValue = mFileSourcePackage->appendAvidAttribute("_EWC_FILE_MOB", 1);

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
    sequence->setDuration(track_growing_duration); // updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
    SourceClip *source_clip = new SourceClip(mHeaderMetadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(IsPicture() ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
    source_clip->setDuration(track_growing_duration); // updated when writing completed
    source_clip->setStartPosition(0); // could be updated by AvidClip
    source_clip->setSourceTrackID(mSourceRefTrackId);
    source_clip->setSourcePackageID(mSourceRefPackageUID);

    // Preface - ContentStorage - SourcePackage - FileDescriptor
    FileDescriptor *descriptor = mDescriptorHelper->CreateFileDescriptor(mHeaderMetadata);
    mFileSourcePackage->setDescriptor(descriptor);
    if (track_growing_duration >= 0)
        descriptor->setContainerDuration(track_growing_duration);
    descriptor->setLinkedTrackID(AV_TRACK_ID);
    // replace essence container label with generic AAF-KLV (aka MXF) label
    descriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));

    // Preface - ContentStorage - EssenceContainerData
    EssenceContainerData *ess_container_data = new EssenceContainerData(mHeaderMetadata);
    content_storage->appendEssenceContainerData(ess_container_data);
    ess_container_data->setLinkedPackageUID(mFileSourcePackageUID);
    ess_container_data->setIndexSID(mIndexSID);
    ess_container_data->setBodySID(mBodySID);

    // add referenced physical source package
    if (mClip->mPhysicalSourcePackage &&
        mClip->mPhysicalSourcePackage->getPackageUID() == mSourceRefPackageUID)
    {
        mRefSourcePackage = dynamic_cast<SourcePackage*>(mClip->mPhysicalSourcePackage->clone(mHeaderMetadata));
        content_storage->appendPackages(mRefSourcePackage);
        SetPhysicalSourceStartTimecode();
    }

    // Preface - Identification
    Identification *ident = new Identification(mHeaderMetadata);
    preface->appendIdentifications(ident);
    ident->initialise(mClip->mCompanyName, mClip->mProductName, mClip->mVersionString, mClip->mProductUID);
    if (mClip->mProductVersion.major != 0 || mClip->mProductVersion.minor != 0 || mClip->mProductVersion.patch != 0 ||
        mClip->mProductVersion.build != 0 || mClip->mProductVersion.release != 0)
    {
        ident->setProductVersion(mClip->mProductVersion);
    }
    ident->setModificationDate(mClip->mCreationDate);
    ident->setThisGenerationUID(mClip->mGenerationUID);
    // ProductVersion type in AAF/Avid differs from MXF. The last record member (release aka type) is a uint8
    ident->setAvidProductVersion(&MXF_ITEM_K(Identification, ToolkitVersion), mHeaderMetadata->getToolkitVersion());
}

void AvidTrack::CreateFile()
{
    BMX_ASSERT(mHeaderMetadata);


    // set the minimum llen
    mMXFFile->setMinLLen(LLEN);


    // write header partition and essence partition pack to memory first

    mMXFFile->openMemoryFile(8192);


    // write the header partition pack and header metadata

    Partition &header_partition = mMXFFile->createPartition();
    if (mClip->mFlavour & AVID_GROWING_FILE_FLAVOUR)
        header_partition.setKey(&MXF_PP_K(ClosedComplete, Header));
    else
        header_partition.setKey(&MXF_PP_K(OpenIncomplete, Header));
    header_partition.setIndexSID(0);
    header_partition.setBodySID(0);
    header_partition.setKagSize(0x100);
    header_partition.setOperationalPattern(&MXF_OP_L(atom, NTracks_NSourceClips));
    header_partition.addEssenceContainer(&mEssenceContainerUL);
    header_partition.write(mMXFFile);

    PositionFillerWriter position_filler_writer(FIXED_BODY_PP_OFFSET);
    mHeaderMetadata->write(mMXFFile, &header_partition, &position_filler_writer);


    // write the essence data partition pack

    int64_t file_pos = mMXFFile->tell();
    Partition &ess_partition = mMXFFile->createPartition();
    if (mClip->mFlavour & AVID_GROWING_FILE_FLAVOUR)
        ess_partition.setKey(&MXF_PP_K(ClosedIncomplete, Footer));
    else
        ess_partition.setKey(&MXF_PP_K(OpenComplete, Body));
    ess_partition.setKagSize(IsPicture() ? 0x20000 : 0x1000);
    ess_partition.setIndexSID(0);
    ess_partition.setBodySID(mBodySID);
    // place the first byte of the essence data at the KAG boundary taking into account that the body
    // partition offset (FIXED_BODY_PP_OFFSET) is 0x20 bytes (why?) beyond where it would be expected to be.
    // 57 = 0x20 + 16 + 9
    BMX_ASSERT(mMXFFile->getMinLLen() == 9);
    BMX_CHECK(mxf_write_partition(mMXFFile->getCFile(), ess_partition.getCPartition()));
    mMXFFile->fillToPosition(file_pos + ess_partition.getKagSize() - 57);


    // essence data start

    mEssenceDataStartPos = mMXFFile->tell(); // need this position when we re-write the key
    mMXFFile->writeFixedKL(&mEssenceElementKey, LLEN, 0);


    // update partitions and flush memory

    mMXFFile->updatePartitions();
    mMXFFile->closeMemoryFile();


    PreSampleWriting();
}

void AvidTrack::SetPhysicalSourceStartTimecode()
{
    if (!mRefSourcePackage ||
        !mRefSourcePackage->haveDescriptor() ||
        !mDataModel->isSubclassOf(mRefSourcePackage->getDescriptor(), &MXF_SET_K(PhysicalDescriptor)))
    {
        return;
    }

    // get physical package start timecode
    TimecodeComponent *phys_tc_component = GetTimecodeComponent(mRefSourcePackage);
    if (!phys_tc_component)
        return;
    Timecode phys_start_timecode(phys_tc_component->getRoundedTimecodeBase(),
                                 phys_tc_component->getDropFrame(),
                                 phys_tc_component->getStartTimecode());

    // convert to a offset at clip frame rate
    uint16_t rounded_clip_tc_base = get_rounded_tc_base(mClip->mClipFrameRate);
    int64_t phys_tc_start_offset = convert_position(phys_start_timecode.GetOffset(),
                                                    rounded_clip_tc_base,
                                                    phys_start_timecode.GetRoundedTCBase(),
                                                    ROUND_AUTO);
    int64_t clip_tc_start_offset = convert_position(mClip->mStartTimecode.GetOffset(),
                                                    rounded_clip_tc_base,
                                                    mClip->mStartTimecode.GetRoundedTCBase(),
                                                    ROUND_AUTO);
    int64_t start_position = clip_tc_start_offset - phys_tc_start_offset;
    if (start_position < 0) {
        // physical source's start timecode was > start timecode
        log_warn("Not setting start timecode in file source package because start position was negative\n");
        return;
    }

    // set the start position
    vector<GenericTrack*> tracks = mFileSourcePackage->getTracks();
    size_t j;
    for (j = 0; j < tracks.size(); j++) {
        Track *track = dynamic_cast<Track*>(tracks[j]);
        if (!track)
            continue;

        StructuralComponent *track_sequence = track->getSequence();
        mxfUL data_def = track_sequence->getDataDefinition();
        if (!mxf_is_picture(&data_def) && !mxf_is_sound(&data_def))
            continue;

        Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
        BMX_ASSERT(sequence);
        vector<StructuralComponent*> components = sequence->getStructuralComponents();
        BMX_ASSERT(components.size() == 1);
        SourceClip *source_clip = dynamic_cast<SourceClip*>(components[0]);
        BMX_ASSERT(source_clip);

        source_clip->setStartPosition(convert_position(mClip->mClipFrameRate, start_position, track->getEditRate(),
                                                       ROUND_AUTO));
        break;
    }

    // set physical source timecode component's drop frame flag
    phys_tc_component->setDropFrame(mClip->mStartTimecode.IsDropFrame());
}

TimecodeComponent* AvidTrack::GetTimecodeComponent(GenericPackage *package)
{
    // find the timecode component in this package
    TimecodeComponent *tc_component = 0;
    vector<GenericTrack*> tracks = package->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *track = dynamic_cast<Track*>(tracks[i]);
        if (!track)
            continue;

        StructuralComponent *track_sequence = track->getSequence();
        mxfUL data_def = track_sequence->getDataDefinition();
        if (!mxf_is_timecode(&data_def))
            continue;

        Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
        tc_component = dynamic_cast<TimecodeComponent*>(track_sequence);
        if (sequence) {
            vector<StructuralComponent*> components = sequence->getStructuralComponents();
            size_t j;
            for (j = 0; j < components.size(); j++) {
                tc_component = dynamic_cast<TimecodeComponent*>(components[j]);
                if (tc_component)
                    break;
            }
        }
        if (tc_component)
            break;
    }

    return tc_component;
}

