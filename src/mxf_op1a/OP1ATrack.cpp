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

#include <cstdio>

#include <im/mxf_op1a/OP1ATrack.h>
#include <im/mxf_op1a/OP1AFile.h>
#include <im/mxf_op1a/OP1ADVTrack.h>
#include <im/mxf_op1a/OP1AD10Track.h>
#include <im/mxf_op1a/OP1AAVCITrack.h>
#include <im/mxf_op1a/OP1AUncTrack.h>
#include <im/mxf_op1a/OP1AMPEG2LGTrack.h>
#include <im/mxf_op1a/OP1AVC3Track.h>
#include <im/mxf_op1a/OP1APCMTrack.h>
#include <im/MXFUtils.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    OP1AEssenceType op1a_essence_type;
    MXFDescriptorHelper::EssenceType mh_essence_type;
} EssenceTypeMap;

static const EssenceTypeMap ESSENCE_TYPE_MAP[] =
{
    {OP1A_IEC_DV25,         MXFDescriptorHelper::IEC_DV25},
    {OP1A_DVBASED_DV25,     MXFDescriptorHelper::DVBASED_DV25},
    {OP1A_DV50,             MXFDescriptorHelper::DV50},
    {OP1A_DV100_1080I,      MXFDescriptorHelper::DV100_1080I},
    {OP1A_DV100_720P,       MXFDescriptorHelper::DV100_720P},
    {OP1A_D10_30,           MXFDescriptorHelper::D10_30},
    {OP1A_D10_40,           MXFDescriptorHelper::D10_40},
    {OP1A_D10_50,           MXFDescriptorHelper::D10_50},
    {OP1A_AVCI100_1080I,    MXFDescriptorHelper::AVCI100_1080I},
    {OP1A_AVCI100_1080P,    MXFDescriptorHelper::AVCI100_1080P},
    {OP1A_AVCI100_720P,     MXFDescriptorHelper::AVCI100_720P},
    {OP1A_AVCI50_1080I,     MXFDescriptorHelper::AVCI50_1080I},
    {OP1A_AVCI50_1080P,     MXFDescriptorHelper::AVCI50_1080P},
    {OP1A_AVCI50_720P,      MXFDescriptorHelper::AVCI50_720P},
    {OP1A_UNC_SD,           MXFDescriptorHelper::UNC_SD},
    {OP1A_UNC_HD_1080I,     MXFDescriptorHelper::UNC_HD_1080I},
    {OP1A_UNC_HD_1080P,     MXFDescriptorHelper::UNC_HD_1080P},
    {OP1A_UNC_HD_720P,      MXFDescriptorHelper::UNC_HD_720P},
    {OP1A_MPEG2LG_422P_HL,  MXFDescriptorHelper::MPEG2LG_422P_HL},
    {OP1A_MPEG2LG_MP_HL,    MXFDescriptorHelper::MPEG2LG_MP_HL},
    {OP1A_MPEG2LG_MP_H14,   MXFDescriptorHelper::MPEG2LG_MP_H14},
    {OP1A_VC3_1080P_1235,   MXFDescriptorHelper::VC3_1080P_1235},
    {OP1A_VC3_1080P_1237,   MXFDescriptorHelper::VC3_1080P_1237},
    {OP1A_VC3_1080P_1238,   MXFDescriptorHelper::VC3_1080P_1238},
    {OP1A_VC3_1080I_1241,   MXFDescriptorHelper::VC3_1080I_1241},
    {OP1A_VC3_1080I_1242,   MXFDescriptorHelper::VC3_1080I_1242},
    {OP1A_VC3_1080I_1243,   MXFDescriptorHelper::VC3_1080I_1243},
    {OP1A_VC3_720P_1250,    MXFDescriptorHelper::VC3_720P_1250},
    {OP1A_VC3_720P_1251,    MXFDescriptorHelper::VC3_720P_1251},
    {OP1A_VC3_720P_1252,    MXFDescriptorHelper::VC3_720P_1252},
    {OP1A_VC3_1080P_1253,   MXFDescriptorHelper::VC3_1080P_1253},
    {OP1A_PCM,              MXFDescriptorHelper::WAVE_PCM},
};


typedef struct
{
    OP1AEssenceType essence_type;
    bool is_mpeg2lg_720p;
    mxfRational sample_rate[10];
} OP1ASampleRateSupport;

static const OP1ASampleRateSupport OP1A_SAMPLE_RATE_SUPPORT[] =
{
    {OP1A_IEC_DV25,           false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_DVBASED_DV25,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_DV50,               false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_DV100_1080I,        false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_DV100_720P,         false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_D10_30,             false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_D10_40,             false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_D10_50,             false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_AVCI100_1080I,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_AVCI100_1080P,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_AVCI100_720P,       false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_AVCI50_1080I,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_AVCI50_1080P,       false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_AVCI50_720P,        false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_UNC_SD,             false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_UNC_HD_1080I,       false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_UNC_HD_1080P,       false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_UNC_HD_720P,        false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_MPEG2LG_422P_HL,    false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_MPEG2LG_422P_HL,    true,     {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_MPEG2LG_MP_HL,      false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_MPEG2LG_MP_HL,      true,     {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_MPEG2LG_MP_H14,     false,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {OP1A_VC3_1080P_1235,     false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_1080P_1237,     false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_1080P_1238,     false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_1080I_1241,     false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_1080I_1242,     false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_1080I_1243,     false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_720P_1250,      false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_720P_1251,      false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_720P_1252,      false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_VC3_1080P_1253,     false,    {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {OP1A_PCM,                false,    {{48000, 1}, {0, 0}}},
};



bool OP1ATrack::IsSupported(OP1AEssenceType essence_type, bool is_mpeg2lg_720p, mxfRational sample_rate)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(OP1A_SAMPLE_RATE_SUPPORT); i++) {
        if (essence_type == OP1A_SAMPLE_RATE_SUPPORT[i].essence_type &&
            is_mpeg2lg_720p == OP1A_SAMPLE_RATE_SUPPORT[i].is_mpeg2lg_720p)
        {
            size_t j = 0;
            while (OP1A_SAMPLE_RATE_SUPPORT[i].sample_rate[j].numerator) {
                if (sample_rate == OP1A_SAMPLE_RATE_SUPPORT[i].sample_rate[j])
                    return true;
                j++;
            }
        }
    }

    return false;
}

MXFDescriptorHelper::EssenceType OP1ATrack::ConvertEssenceType(OP1AEssenceType op1a_essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (ESSENCE_TYPE_MAP[i].op1a_essence_type == op1a_essence_type)
            return ESSENCE_TYPE_MAP[i].mh_essence_type;
    }

    return MXFDescriptorHelper::UNKNOWN_ESSENCE;
}

OP1AEssenceType OP1ATrack::ConvertEssenceType(MXFDescriptorHelper::EssenceType mh_essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (ESSENCE_TYPE_MAP[i].mh_essence_type == mh_essence_type)
            return ESSENCE_TYPE_MAP[i].op1a_essence_type;
    }

    return OP1A_UNKNOWN_ESSENCE;
}

OP1ATrack* OP1ATrack::Create(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             mxfRational frame_rate, OP1AEssenceType essence_type)
{
    switch (essence_type)
    {
        case OP1A_IEC_DV25:
        case OP1A_DVBASED_DV25:
        case OP1A_DV50:
        case OP1A_DV100_1080I:
        case OP1A_DV100_720P:
            return new OP1ADVTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case OP1A_D10_30:
        case OP1A_D10_40:
        case OP1A_D10_50:
            return new OP1AD10Track(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case OP1A_AVCI100_1080I:
        case OP1A_AVCI100_1080P:
        case OP1A_AVCI100_720P:
        case OP1A_AVCI50_1080I:
        case OP1A_AVCI50_1080P:
        case OP1A_AVCI50_720P:
            return new OP1AAVCITrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case OP1A_UNC_SD:
        case OP1A_UNC_HD_1080I:
        case OP1A_UNC_HD_1080P:
        case OP1A_UNC_HD_720P:
            return new OP1AUncTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case OP1A_MPEG2LG_422P_HL:
        case OP1A_MPEG2LG_MP_HL:
        case OP1A_MPEG2LG_MP_H14:
            return new OP1AMPEG2LGTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case OP1A_VC3_1080P_1235:
        case OP1A_VC3_1080P_1237:
        case OP1A_VC3_1080P_1238:
        case OP1A_VC3_1080I_1241:
        case OP1A_VC3_1080I_1242:
        case OP1A_VC3_1080I_1243:
        case OP1A_VC3_720P_1250:
        case OP1A_VC3_720P_1251:
        case OP1A_VC3_720P_1252:
        case OP1A_VC3_1080P_1253:
            return new OP1AVC3Track(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case OP1A_PCM:
            return new OP1APCMTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case OP1A_UNKNOWN_ESSENCE:
            IM_ASSERT(false);
    }

    return 0;
}

OP1ATrack::OP1ATrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                     mxfRational frame_rate, OP1AEssenceType essence_type)
{
    mOP1AFile = file;
    mCPManager = file->GetContentPackageManager();
    mIndexTable = file->GetIndexTable();
    mTrackIndex = track_index;
    mTrackId = track_id;
    mOutputTrackNumber = 0;
    mOutputTrackNumberSet = false;
    mTrackTypeNumber = track_type_number;
    mIsPicture = true;
    mFrameRate = frame_rate;
    mTrackNumber = 0;
    mHaveLowerLevelSourcePackage = false;
    mLowerLevelSourcePackage = 0;
    mLowerLevelSourcePackageUID = g_Null_UMID;
    mLowerLevelTrackId = 0;

    mEssenceType = essence_type;
    mDescriptorHelper = MXFDescriptorHelper::Create(ConvertEssenceType(essence_type));
    mDescriptorHelper->SetFlavour(MXFDescriptorHelper::SMPTE_377_1_FLAVOUR);
    mDescriptorHelper->SetFrameWrapped(true);
    mDescriptorHelper->SetSampleRate(frame_rate);
}

OP1ATrack::~OP1ATrack()
{
    delete mDescriptorHelper;
}

void OP1ATrack::SetOutputTrackNumber(uint32_t track_number)
{
    mOutputTrackNumber = track_number;
    mOutputTrackNumberSet = true;
}

void OP1ATrack::SetLowerLevelSourcePackage(SourcePackage *package, uint32_t track_id, string uri)
{
    IM_CHECK(!mHaveLowerLevelSourcePackage);

#if 0   // TODO: allow dark strong referenced sets to be cloned without resulting in an error
    mLowerLevelSourcePackage = dynamic_cast<SourcePackage*>(package->clone(mHeaderMetadata));
#else
    mLowerLevelSourcePackage = 0;
#endif
    mLowerLevelSourcePackageUID = package->getPackageUID();
    mLowerLevelTrackId = track_id;
    mLowerLevelURI = uri;

    mHaveLowerLevelSourcePackage = true;
}

void OP1ATrack::SetLowerLevelSourcePackage(mxfUMID package_uid, uint32_t track_id)
{
    IM_CHECK(!mHaveLowerLevelSourcePackage);

    mLowerLevelSourcePackageUID = package_uid;
    mLowerLevelTrackId = track_id;

    mHaveLowerLevelSourcePackage = true;
}

void OP1ATrack::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    mOP1AFile->WriteSamples(mTrackIndex, data, size, num_samples);
}

mxfUL OP1ATrack::GetEssenceContainerUL() const
{
    return mDescriptorHelper->GetEssenceContainerUL();
}

uint32_t OP1ATrack::GetSampleSize()
{
    return mDescriptorHelper->GetSampleSize();
}

void OP1ATrack::AddHeaderMetadata(HeaderMetadata *header_metadata, MaterialPackage *material_package,
                                  SourcePackage *file_source_package)
{
    mxfUL data_def = (mIsPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));

    // Preface - ContentStorage - MaterialPackage - Timeline Track
    Track *track = new Track(header_metadata);
    material_package->appendTracks(track);
    track->setTrackName(get_track_name(mIsPicture, mOutputTrackNumber));
    track->setTrackID(mTrackId);
    track->setTrackNumber(mOutputTrackNumber);
    track->setEditRate(mFrameRate);
    track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
    Sequence *sequence = new Sequence(header_metadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(data_def);
    sequence->setDuration(-1); // updated when writing completed

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
    SourceClip *source_clip = new SourceClip(header_metadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(data_def);
    source_clip->setDuration(-1); // updated when writing completed
    source_clip->setStartPosition(0);
    source_clip->setSourcePackageID(file_source_package->getPackageUID());
    source_clip->setSourceTrackID(mTrackId);


    // Preface - ContentStorage - SourcePackage - Timeline Track
    track = new Track(header_metadata);
    file_source_package->appendTracks(track);
    track->setTrackName(get_track_name(mIsPicture, mOutputTrackNumber));
    track->setTrackID(mTrackId);
    track->setTrackNumber(mTrackNumber);
    track->setEditRate(mFrameRate);
    track->setOrigin(0); // could be updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
    sequence = new Sequence(header_metadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(data_def);
    sequence->setDuration(-1); // updated when writing completed

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
    source_clip = new SourceClip(header_metadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(data_def);
    source_clip->setDuration(-1); // updated when writing completed
    source_clip->setStartPosition(0);
    if (mHaveLowerLevelSourcePackage) {
        source_clip->setSourcePackageID(mLowerLevelSourcePackageUID);
        source_clip->setSourceTrackID(mLowerLevelTrackId);
    } else {
        source_clip->setSourceTrackID(0);
        source_clip->setSourcePackageID(g_Null_UMID);
    }

    // Preface - ContentStorage - SourcePackage - FileDescriptor
    FileDescriptor *descriptor = mDescriptorHelper->CreateFileDescriptor(header_metadata);
    if (file_source_package->haveDescriptor()) {
        MultipleDescriptor *mult_descriptor = dynamic_cast<MultipleDescriptor*>(file_source_package->getDescriptor());
        IM_ASSERT(mult_descriptor);
        mult_descriptor->appendSubDescriptorUIDs(descriptor);
    } else {
        file_source_package->setDescriptor(descriptor);
    }
    descriptor->setLinkedTrackID(mTrackId);
    descriptor->setContainerDuration(-1);  // updated when writing completed

    // Preface - ContentStorage - (lower-level) SourcePackage
    if (mLowerLevelSourcePackage) {
        header_metadata->getPreface()->getContentStorage()->appendPackages(mLowerLevelSourcePackage);
        if (!mLowerLevelURI.empty()) {
            NetworkLocator *network_locator = new NetworkLocator(header_metadata);
            mLowerLevelSourcePackage->getDescriptor()->appendLocators(network_locator);
            network_locator->setURLString(mLowerLevelURI);
        }
    }
}

void OP1ATrack::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    IM_ASSERT(data && size && num_samples);

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
}

void OP1ATrack::CompleteEssenceKeyAndTrackNum(uint8_t track_count)
{
    mxf_complete_essence_element_key(&mEssenceElementKey,
                                     track_count,
                                     mxf_get_essence_element_type(mTrackNumber),
                                     mTrackTypeNumber);
    mxf_complete_essence_element_track_num(&mTrackNumber,
                                           track_count,
                                           mxf_get_essence_element_type(mTrackNumber),
                                           mTrackTypeNumber);
}

