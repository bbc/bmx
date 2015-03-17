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

#include <bmx/mxf_op1a/OP1ATrack.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/mxf_op1a/OP1AANCDataTrack.h>
#include <bmx/mxf_op1a/OP1ADVTrack.h>
#include <bmx/mxf_op1a/OP1AD10Track.h>
#include <bmx/mxf_op1a/OP1AAVCITrack.h>
#include <bmx/mxf_op1a/OP1AUncTrack.h>
#include <bmx/mxf_op1a/OP1AMPEG2LGTrack.h>
#include <bmx/mxf_op1a/OP1AVC3Track.h>
#include <bmx/mxf_op1a/OP1APCMTrack.h>
#include <bmx/mxf_op1a/OP1AVBIDataTrack.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    EssenceType essence_type;
    mxfRational sample_rate[10];
} OP1ASampleRateSupport;

static const OP1ASampleRateSupport OP1A_SAMPLE_RATE_SUPPORT[] =
{
    {IEC_DV25,                 {{25, 1}, {30000, 1001}, {0, 0}}},
    {DVBASED_DV25,             {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV50,                     {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV100_1080I,              {{25, 1}, {30000, 1001}, {0, 0}}},
    {DV100_720P,               {{24000, 1001}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
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
    {UNC_SD,                   {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {UNC_HD_1080I,             {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {UNC_HD_1080P,             {{25, 1}, {30000, 1001}, {30, 1}, {50, 1}, {60000, 1001}, {60, 1}, {0, 0}}},
    {UNC_HD_720P,              {{25, 1}, {30000, 1001}, {30, 1}, {50, 1}, {60000, 1001}, {60, 1}, {0, 0}}},
    {UNC_UHD_3840,             {{-1, -1}, {0, 0}}},
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
    {VC3_1080P_1235,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080P_1237,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080P_1238,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080I_1241,           {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080I_1242,           {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080I_1243,           {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080I_1244,           {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_720P_1250,            {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_720P_1251,            {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_720P_1252,            {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080P_1253,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_720P_1258,            {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080P_1259,           {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {VC3_1080I_1260,           {{25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {WAVE_PCM,                 {{48000, 1}, {0, 0}}},
    {ANC_DATA,                 {{-1, -1}, {0, 0}}},
    {VBI_DATA,                 {{-1, -1}, {0, 0}}},
};



bool OP1ATrack::IsSupported(EssenceType essence_type, mxfRational sample_rate)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(OP1A_SAMPLE_RATE_SUPPORT); i++) {
        if (essence_type == OP1A_SAMPLE_RATE_SUPPORT[i].essence_type) {
            if (OP1A_SAMPLE_RATE_SUPPORT[i].sample_rate[0].numerator < 0)
                return true;

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

OP1ATrack* OP1ATrack::Create(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             mxfRational frame_rate, EssenceType essence_type)
{
    switch (essence_type)
    {
        case IEC_DV25:
        case DVBASED_DV25:
        case DV50:
        case DV100_1080I:
        case DV100_720P:
            return new OP1ADVTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case D10_30:
        case D10_40:
        case D10_50:
            return new OP1AD10Track(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case AVCI200_1080I:
        case AVCI200_1080P:
        case AVCI200_720P:
        case AVCI100_1080I:
        case AVCI100_1080P:
        case AVCI100_720P:
        case AVCI50_1080I:
        case AVCI50_1080P:
        case AVCI50_720P:
            return new OP1AAVCITrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case UNC_SD:
        case UNC_HD_1080I:
        case UNC_HD_1080P:
        case UNC_HD_720P:
        case UNC_UHD_3840:
            return new OP1AUncTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
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
            return new OP1AMPEG2LGTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
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
            return new OP1AVC3Track(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case WAVE_PCM:
            return new OP1APCMTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case ANC_DATA:
            return new OP1AANCDataTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case VBI_DATA:
            return new OP1AVBIDataTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        default:
            BMX_ASSERT(false);
    }

    return 0;
}

OP1ATrack::OP1ATrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                     mxfRational frame_rate, EssenceType essence_type)
{
    mOP1AFile = file;
    mCPManager = file->GetContentPackageManager();
    mIndexTable = file->GetIndexTable();
    mTrackIndex = track_index;
    mTrackId = track_id;
    mOutputTrackNumber = 0;
    mOutputTrackNumberSet = false;
    mTrackTypeNumber = track_type_number;
    mDataDef = convert_essence_type_to_data_def(essence_type);
    mFrameRate = frame_rate;
    mEditRate = frame_rate;
    mTrackNumber = 0;
    mHaveLowerLevelSourcePackage = false;
    mLowerLevelSourcePackage = 0;
    mLowerLevelSourcePackageUID = g_Null_UMID;
    mLowerLevelTrackId = 0;

    int descriptor_flavour = 0;
    if ((file->mFlavour & OP1A_377_2004_FLAVOUR))
        descriptor_flavour |= MXFDESC_SMPTE_377_2004_FLAVOUR;
    else
        descriptor_flavour |= MXFDESC_SMPTE_377_1_FLAVOUR;
    if ((file->mFlavour & OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        descriptor_flavour |= MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR;

    mEssenceType = essence_type;
    mDescriptorHelper = MXFDescriptorHelper::Create(essence_type);
    mDescriptorHelper->SetFlavour(descriptor_flavour);
    mDescriptorHelper->SetFrameWrapped(file->IsFrameWrapped());
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
    BMX_CHECK(!mHaveLowerLevelSourcePackage);

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
    BMX_CHECK(!mHaveLowerLevelSourcePackage);

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

int64_t OP1ATrack::GetDuration() const
{
    return mOP1AFile->GetDuration();
}

int64_t OP1ATrack::GetContainerDuration() const
{
    return mOP1AFile->GetContainerDuration();
}

void OP1ATrack::AddHeaderMetadata(HeaderMetadata *header_metadata, MaterialPackage *material_package,
                                  SourcePackage *file_source_package)
{
    mxfUL data_def_ul;
    BMX_CHECK(mxf_get_ddef_label(mDataDef, &data_def_ul));

    int64_t material_track_duration = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_duration = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_origin = 0; // could be updated if not writing in a single pass
    if (mOP1AFile->mSupportCompleteSinglePass) {
        material_track_duration = mOP1AFile->mInputDuration - mOP1AFile->mOutputStartOffset +
                                  mOP1AFile->mOutputEndOffset;
        if (material_track_duration < 0)
            material_track_duration = 0;
        source_track_duration = mOP1AFile->mInputDuration + mOP1AFile->mOutputEndOffset;
        source_track_origin = mOP1AFile->mOutputStartOffset;
    }

    // Preface - ContentStorage - MaterialPackage - Timeline Track
    Track *track = new Track(header_metadata);
    material_package->appendTracks(track);
    track->setTrackName(get_track_name(mDataDef, mOutputTrackNumber));
    track->setTrackID(mTrackId);
    if ((mOP1AFile->GetFlavour() & OP1A_ZERO_MP_TRACK_NUMBER_FLAVOUR))
        track->setTrackNumber(0);
    else
        track->setTrackNumber(mOutputTrackNumber);
    track->setEditRate(mEditRate);
    track->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
    Sequence *sequence = new Sequence(header_metadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(data_def_ul);
    sequence->setDuration(material_track_duration);

    // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
    SourceClip *source_clip = new SourceClip(header_metadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(data_def_ul);
    source_clip->setDuration(material_track_duration);
    source_clip->setStartPosition(0);
    source_clip->setSourcePackageID(file_source_package->getPackageUID());
    source_clip->setSourceTrackID(mTrackId);


    // Preface - ContentStorage - SourcePackage - Timeline Track
    track = new Track(header_metadata);
    file_source_package->appendTracks(track);
    track->setTrackName(get_track_name(mDataDef, mOutputTrackNumber));
    track->setTrackID(mTrackId);
    track->setTrackNumber(mTrackNumber);
    track->setEditRate(mEditRate);
    track->setOrigin(source_track_origin);

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
    sequence = new Sequence(header_metadata);
    track->setSequence(sequence);
    sequence->setDataDefinition(data_def_ul);
    sequence->setDuration(source_track_duration);

    // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
    source_clip = new SourceClip(header_metadata);
    sequence->appendStructuralComponents(source_clip);
    source_clip->setDataDefinition(data_def_ul);
    source_clip->setDuration(source_track_duration);
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
        BMX_ASSERT(mult_descriptor);
        mult_descriptor->appendSubDescriptorUIDs(descriptor);
    } else {
        file_source_package->setDescriptor(descriptor);
    }
    descriptor->setLinkedTrackID(mTrackId);
    if (mOP1AFile->mSupportCompleteSinglePass)
        descriptor->setContainerDuration(mOP1AFile->mInputDuration);

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
    BMX_ASSERT(data && size && num_samples);

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
}

void OP1ATrack::WriteSampleInt(const CDataBuffer *data_array, uint32_t array_size)
{
    BMX_ASSERT(data_array && array_size);

    mCPManager->WriteSample(mTrackIndex, data_array, array_size);
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

