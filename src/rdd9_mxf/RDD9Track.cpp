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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS

#include <cstdio>

#include <bmx/rdd9_mxf/RDD9Track.h>
#include <bmx/rdd9_mxf/RDD9File.h>
#include <bmx/rdd9_mxf/RDD9ANCDataTrack.h>
#include <bmx/rdd9_mxf/RDD9MPEG2LGTrack.h>
#include <bmx/rdd9_mxf/RDD9PCMTrack.h>
#include <bmx/rdd9_mxf/RDD9VBIDataTrack.h>
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
    Rational sample_rate[10];
} RDD9SampleRateSupport;

static const RDD9SampleRateSupport RDD9_SAMPLE_RATE_SUPPORT[] =
{
    {MPEG2LG_422P_HL_1080I,    {                        {25, 1}, {30000, 1001},                         {0, 0}}},
    {MPEG2LG_422P_HL_1080P,    {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001},                         {0, 0}}},
    {MPEG2LG_422P_HL_720P,     {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MPEG2LG_MP_HL_1920_1080I, {                        {25, 1}, {30000, 1001},                         {0, 0}}},
    {MPEG2LG_MP_HL_1920_1080P, {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001},                         {0, 0}}},
    {MPEG2LG_MP_HL_1440_1080I, {                        {25, 1}, {30000, 1001},                         {0, 0}}},
    {MPEG2LG_MP_HL_1440_1080P, {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001},                         {0, 0}}},
    {MPEG2LG_MP_HL_720P,       {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001}, {50, 1}, {60000, 1001}, {0, 0}}},
    {MPEG2LG_MP_H14_1080I,     {                        {25, 1}, {30000, 1001},                         {0, 0}}},
    {MPEG2LG_MP_H14_1080P,     {{24000, 1001}, {24, 1}, {25, 1}, {30000, 1001},                         {0, 0}}},

    {WAVE_PCM,                 {{48000, 1}, {0, 0}}},
    {ANC_DATA,                 {{-1, -1}, {0, 0}}},
    {VBI_DATA,                 {{-1, -1}, {0, 0}}},
};



bool RDD9Track::IsSupported(EssenceType essence_type, Rational sample_rate)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(RDD9_SAMPLE_RATE_SUPPORT); i++) {
        if (essence_type == RDD9_SAMPLE_RATE_SUPPORT[i].essence_type) {
            if (RDD9_SAMPLE_RATE_SUPPORT[i].sample_rate[0].numerator < 0)
                return true;

            size_t j = 0;
            while (RDD9_SAMPLE_RATE_SUPPORT[i].sample_rate[j].numerator) {
                if (sample_rate == RDD9_SAMPLE_RATE_SUPPORT[i].sample_rate[j])
                    return true;
                j++;
            }
        }
    }

    return false;
}

RDD9Track* RDD9Track::Create(RDD9File *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                             Rational frame_rate, EssenceType essence_type)
{
    switch (essence_type)
    {
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
            return new RDD9MPEG2LGTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case WAVE_PCM:
            return new RDD9PCMTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case ANC_DATA:
            return new RDD9ANCDataTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        case VBI_DATA:
            return new RDD9VBIDataTrack(file, track_index, track_id, track_type_number, frame_rate, essence_type);
        default:
            BMX_ASSERT(false);
    }

    return 0;
}

RDD9Track::RDD9Track(RDD9File *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                     Rational frame_rate, EssenceType essence_type)
{
    mRDD9File = file;
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

    int descriptor_flavour = MXFDESC_RDD9_FLAVOUR;
    if ((file->mFlavour & RDD9_SMPTE_377_2004_FLAVOUR))
        descriptor_flavour |= MXFDESC_SMPTE_377_2004_FLAVOUR;
    else
        descriptor_flavour |= MXFDESC_SMPTE_377_1_FLAVOUR;
    if ((file->mFlavour & RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR))
        descriptor_flavour |= MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR;

    mEssenceType = essence_type;
    mDescriptorHelper = MXFDescriptorHelper::Create(essence_type);
    mDescriptorHelper->SetFlavour(descriptor_flavour);
    mDescriptorHelper->SetFrameWrapped(true);
    mDescriptorHelper->SetSampleRate(frame_rate);
}

RDD9Track::~RDD9Track()
{
    delete mDescriptorHelper;
}

void RDD9Track::SetOutputTrackNumber(uint32_t track_number)
{
    mOutputTrackNumber    = track_number;
    mOutputTrackNumberSet = true;
}

void RDD9Track::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    mRDD9File->WriteSamples(mTrackIndex, data, size, num_samples);
}

mxfUL RDD9Track::GetEssenceContainerUL() const
{
    return mDescriptorHelper->GetEssenceContainerUL();
}

uint32_t RDD9Track::GetSampleSize()
{
    return mDescriptorHelper->GetSampleSize();
}

int64_t RDD9Track::GetDuration() const
{
    return mRDD9File->GetDuration();
}

int64_t RDD9Track::GetContainerDuration() const
{
    return mRDD9File->GetContainerDuration();
}

void RDD9Track::AddHeaderMetadata(HeaderMetadata *header_metadata, MaterialPackage *material_package,
                                  SourcePackage *file_source_package)
{
    mxfUL data_def_ul;
    BMX_CHECK(mxf_get_ddef_label(mDataDef, &data_def_ul));

    int64_t material_track_duration = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_duration   = -1; // unknown - could be updated if not writing in a single pass
    int64_t source_track_origin     = 0;  // could be updated if not writing in a single pass


    // Preface - ContentStorage - MaterialPackage - Timeline Track
    Track *track = new Track(header_metadata);
    material_package->appendTracks(track);

    track->setTrackName(get_track_name(mDataDef, mOutputTrackNumber));
    track->setTrackID(mTrackId);
    // TODO: not sure whether setting TrackNumber in the MaterialPackage is a good idea for this MXF flavour
    //       track->setTrackNumber(mOutputTrackNumber);
    track->setTrackNumber(0);
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
    source_clip->setSourceTrackID(0);
    source_clip->setSourcePackageID(g_Null_UMID);

    // Preface - ContentStorage - SourcePackage - FileDescriptor
    FileDescriptor *descriptor = mDescriptorHelper->CreateFileDescriptor(header_metadata);
    BMX_ASSERT(file_source_package->haveDescriptor());
    MultipleDescriptor *mult_descriptor = dynamic_cast<MultipleDescriptor*>(file_source_package->getDescriptor());
    BMX_ASSERT(mult_descriptor);
    mult_descriptor->appendSubDescriptorUIDs(descriptor);
    descriptor->setLinkedTrackID(mTrackId);
}

void RDD9Track::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    BMX_ASSERT(data && size && num_samples);

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
}

void RDD9Track::CompleteEssenceKeyAndTrackNum(uint8_t track_count)
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

