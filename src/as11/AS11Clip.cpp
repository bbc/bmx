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

#include <im/as11/AS11Clip.h>
#include <im/as11/AS11SegmentationFramework.h>
#include <im/MXFUtils.h>
#include <im/Utils.h>
#include <im/Version.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;


typedef struct
{
    AS11ClipType type;
    const char *str;
} AS11ClipTypeStringMap;

static const AS11ClipTypeStringMap AS11_CLIP_TYPE_STRING_MAP[] =
{
    {AS11_UNKNOWN_CLIP_TYPE,  "unknown"},
    {AS11_OP1A_CLIP_TYPE,     "MXF OP-1A"},
    {AS11_D10_CLIP_TYPE,      "D10"},
};



static int64_t get_offset(uint16_t to_tc_base, uint16_t from_tc_base, int64_t from_offset)
{
    return convert_position(from_offset, to_tc_base, from_tc_base, ROUND_AUTO);
}



AS11Clip* AS11Clip::OpenNewOP1AClip(string filename, Rational frame_rate)
{
    return new AS11Clip(OP1AFile::OpenNew(OP1A_DEFAULT_FLAVOUR, filename, frame_rate));
}

AS11Clip* AS11Clip::OpenNewD10Clip(string filename, Rational frame_rate)
{
    return new AS11Clip(D10File::OpenNew(filename, frame_rate));
}

string AS11Clip::AS11ClipTypeToString(AS11ClipType clip_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(AS11_CLIP_TYPE_STRING_MAP); i++) {
        if (clip_type == AS11_CLIP_TYPE_STRING_MAP[i].type)
            return AS11_CLIP_TYPE_STRING_MAP[i].str;
    }

    IM_ASSERT(false);
    return "";
}

AS11Clip::AS11Clip(OP1AFile *clip)
{
    mType = AS11_OP1A_CLIP_TYPE;
    mOP1AClip = clip;
    mD10Clip = 0;
    mSegmentationSequence = 0;

    mOP1AClip->SetProductInfo(get_im_company_name(), get_im_library_name(), get_im_mxf_product_version(),
                              get_im_version_string(), get_im_product_uid());
}

AS11Clip::AS11Clip(D10File *clip)
{
    mType = AS11_D10_CLIP_TYPE;
    mOP1AClip = 0;
    mD10Clip = clip;
    mSegmentationSequence = 0;

    mD10Clip->SetProductInfo(get_im_company_name(), get_im_library_name(), get_im_mxf_product_version(),
                             get_im_version_string(), get_im_product_uid());
}

AS11Clip::~AS11Clip()
{
    delete mOP1AClip;
    delete mD10Clip;

    size_t i;
    for (i = 0; i < mTracks.size(); i++)
        delete mTracks[i];
}

void AS11Clip::SetClipName(string name)
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->SetClipName(name);
            break;
        case AS11_D10_CLIP_TYPE:
            mD10Clip->SetClipName(name);
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Clip::SetStartTimecode(Timecode start_timecode)
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->SetStartTimecode(start_timecode);
            break;
        case AS11_D10_CLIP_TYPE:
            mD10Clip->SetStartTimecode(start_timecode);
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Clip::SetPartitionInterval(int64_t frame_count)
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->SetPartitionInterval(frame_count);
            break;
        case AS11_D10_CLIP_TYPE:
            log_warn("Setting partition interval not supported in D10 clip\n");
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Clip::SetOutputStartOffset(int64_t offset)
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->SetOutputStartOffset(offset);
            break;
        case AS11_D10_CLIP_TYPE:
            log_warn("Setting output start offset not supported in D10 clip\n");
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Clip::SetOutputEndOffset(int64_t offset)
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->SetOutputEndOffset(offset);
            break;
        case AS11_D10_CLIP_TYPE:
            log_warn("Setting output end offset not supported in D10 clip\n");
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

AS11Track* AS11Clip::CreateTrack(AS11EssenceType essence_type)
{
    AS11Track *track = 0;
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            track = new AS11Track(essence_type, mOP1AClip->CreateTrack(
                (OP1AEssenceType)AS11Track::ConvertEssenceType(mType, essence_type)));
            break;
        case AS11_D10_CLIP_TYPE:
            track = new AS11Track(essence_type, mD10Clip->CreateTrack(
                (D10EssenceType)AS11Track::ConvertEssenceType(mType, essence_type)));
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    mTracks.push_back(track);
    return track;
}

void AS11Clip::PrepareHeaderMetadata()
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->PrepareHeaderMetadata();
            break;
        case AS11_D10_CLIP_TYPE:
            mD10Clip->PrepareHeaderMetadata();
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Clip::PrepareWrite()
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->PrepareWrite();
            break;
        case AS11_D10_CLIP_TYPE:
            mD10Clip->PrepareWrite();
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Clip::WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    AS11Track *track = GetTrack(track_index);
    track->WriteSamples(data, size, num_samples);
}

void AS11Clip::CompleteWrite()
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1AClip->CompleteWrite();
            break;
        case AS11_D10_CLIP_TYPE:
            mD10Clip->CompleteWrite();
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

HeaderMetadata* AS11Clip::GetHeaderMetadata() const
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            return mOP1AClip->GetHeaderMetadata();
        case AS11_D10_CLIP_TYPE:
            return mD10Clip->GetHeaderMetadata();
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

DataModel* AS11Clip::GetDataModel() const
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            return mOP1AClip->GetDataModel();
        case AS11_D10_CLIP_TYPE:
            return mD10Clip->GetDataModel();
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

void AS11Clip::InsertAS11CoreFramework(AS11CoreFramework *framework)
{
    AppendDMSLabel(MXF_DM_L(AS11CoreDescriptiveScheme));
    InsertFramework(3001, "AS_11_Core", framework);
}

void AS11Clip::InsertUKDPPFramework(UKDPPFramework *framework)
{
    AppendDMSLabel(MXF_DM_L(UKDPPDescriptiveScheme));
    InsertFramework(3101, "UK_DPP", framework);
}

void AS11Clip::InsertPosSegmentation(vector<AS11PosSegment> segments)
{
    IM_ASSERT(!mSegmentationSequence);

    AppendDMSLabel(MXF_DM_L(AS11SegmentationDescriptiveScheme));

    HeaderMetadata *header_metadata = GetHeaderMetadata();
    IM_ASSERT(header_metadata);

    MaterialPackage *material_package = header_metadata->getPreface()->findMaterialPackage();
    IM_ASSERT(material_package);

    // Preface - ContentStorage - Package - DM Track
    Track *dm_track = new Track(header_metadata);
    material_package->appendTracks(dm_track);
    dm_track->setTrackName("AS_11_Segmentation");
    dm_track->setTrackID(3002);
    dm_track->setTrackNumber(0);
    dm_track->setEditRate(GetFrameRate());
    dm_track->setOrigin(0);

    // Preface - ContentStorage - Package - DM Track - Sequence
    mSegmentationSequence = new Sequence(header_metadata);
    dm_track->setSequence(mSegmentationSequence);
    mSegmentationSequence->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));
    // duration is set below

    int64_t next_start = 0;
    size_t i;
    for (i = 0; i < segments.size(); i++) {
        IM_CHECK_M(segments[i].start >= next_start,
                   ("AS11 segment starts (%"PRId64") before end of last segment (%"PRId64")",
                    segments[i].start, next_start - 1));

        if (segments[i].start > next_start) {
            // Preface - ContentStorage - Package - DM Track - Sequence - Filler
            StructuralComponent *component = dynamic_cast<StructuralComponent*>(
                header_metadata->createAndWrap(&MXF_SET_K(Filler)));
            mSegmentationSequence->appendStructuralComponents(component);
            component->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));
            component->setDuration(segments[i].start - next_start);
        }

        // Preface - ContentStorage - Package - DM Track - Sequence - DMSegment
        DMSegment *dm_segment = new DMSegment(header_metadata);
        mSegmentationSequence->appendStructuralComponents(dm_segment);
        dm_segment->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));
        dm_segment->setDuration(segments[i].duration);

        // Preface - ContentStorage - Package - DM Track - Sequence - DMSegment - DMFramework
        AS11SegmentationFramework *framework = new AS11SegmentationFramework(header_metadata);
        framework->SetPartNumber(segments[i].part_number);
        framework->SetPartTotal(segments[i].part_total);
        dm_segment->setDMFramework(framework);

        next_start = segments[i].start + segments[i].duration;
    }

    mSegmentationSequence->setDuration(next_start);
}

void AS11Clip::InsertTCSegmentation(vector<AS11TCSegment> segments)
{
    vector<AS11PosSegment> pos_segments;

    Timecode start_timecode;
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            start_timecode = mOP1AClip->GetStartTimecode();
            break;
        case AS11_D10_CLIP_TYPE:
            start_timecode = mD10Clip->GetStartTimecode();
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    uint16_t to_tc_base = get_rounded_tc_base(GetFrameRate());
    int64_t start_offset = get_offset(to_tc_base, start_timecode.GetRoundedTCBase(), start_timecode.GetOffset());
    int64_t offset;
    size_t i;
    for (i = 0; i < segments.size(); i++) {
        offset = get_offset(to_tc_base, segments[i].start.GetRoundedTCBase(), segments[i].start.GetOffset());
        if (offset < start_offset) {
            // assume passed midnight
            offset = get_offset(to_tc_base, segments[i].start.GetRoundedTCBase(),
                                segments[i].start.GetOffset() + segments[i].start.GetMaxOffset());
        }

        AS11PosSegment pos_segment;
        pos_segment.start = offset - start_offset;
        pos_segment.duration = segments[i].duration;
        pos_segment.part_number = segments[i].part_number;
        pos_segment.part_total = segments[i].part_total;
        pos_segments.push_back(pos_segment);
    }

    InsertPosSegmentation(pos_segments);
}

void AS11Clip::CompleteSegmentation(bool with_filler)
{
    if (!mSegmentationSequence)
        return;

    if (mSegmentationSequence->getDuration() >= GetDuration()) {
        if (mSegmentationSequence->getDuration() > GetDuration()) {
            log_warn("AS-11 segmentation duration (%"PRId64") exceeds package duration (%"PRId64")\n",
                     mSegmentationSequence->getDuration(), GetDuration());
        }
        return;
    }

    HeaderMetadata *header_metadata = GetHeaderMetadata();
    IM_ASSERT(header_metadata);

    vector<StructuralComponent*> components = mSegmentationSequence->getStructuralComponents();

    if (with_filler || components.empty()) {
        // Preface - ContentStorage - Package - DM Track - Sequence - Filler
        StructuralComponent *component = dynamic_cast<StructuralComponent*>(
            header_metadata->createAndWrap(&MXF_SET_K(Filler)));
        mSegmentationSequence->appendStructuralComponents(component);
        component->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));
        component->setDuration(GetDuration() - mSegmentationSequence->getDuration());
    } else {
        components.back()->setDuration(GetDuration() - mSegmentationSequence->getDuration());
    }

    mSegmentationSequence->setDuration(GetDuration());
}

uint16_t AS11Clip::GetTotalSegments()
{
    if (!mSegmentationSequence)
        return 0;

    uint16_t total_segments = 0;
    vector<StructuralComponent*> components = mSegmentationSequence->getStructuralComponents();
    size_t i;
    for (i = 0; i < components.size(); i++) {
        if (*components[i]->getKey() != MXF_SET_K(Filler))
            total_segments++;
    }

    return total_segments;
}

int64_t AS11Clip::GetTotalSegmentDuration()
{
    if (!mSegmentationSequence)
        return 0;

    int64_t total_duration = 0;
    vector<StructuralComponent*> components = mSegmentationSequence->getStructuralComponents();
    size_t i;
    for (i = 0; i < components.size(); i++) {
        if (components[i]->haveDuration() && *components[i]->getKey() != MXF_SET_K(Filler))
            total_duration += components[i]->getDuration();
    }

    return total_duration;
}

Rational AS11Clip::GetFrameRate() const
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            return mOP1AClip->GetFrameRate();
        case AS11_D10_CLIP_TYPE:
            return mD10Clip->GetFrameRate();
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return ZERO_RATIONAL;
}

int64_t AS11Clip::GetDuration()
{
    switch (mType)
    {
        case AS11_OP1A_CLIP_TYPE:
            return mOP1AClip->GetDuration();
        case AS11_D10_CLIP_TYPE:
            return mD10Clip->GetDuration();
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

AS11Track* AS11Clip::GetTrack(uint32_t track_index)
{
    IM_CHECK(track_index < mTracks.size());
    return mTracks[track_index];
}

void AS11Clip::AppendDMSLabel(mxfUL scheme_label)
{
    HeaderMetadata *header_metadata = GetHeaderMetadata();
    IM_ASSERT(header_metadata);

    vector<mxfUL> dm_schemes = header_metadata->getPreface()->getDMSchemes();
    size_t i;
    for (i = 0; i < dm_schemes.size(); i++) {
        if (mxf_equals_ul(&dm_schemes[i], &scheme_label))
            break;
    }
    if (i >= dm_schemes.size())
        header_metadata->getPreface()->appendDMSchemes(scheme_label);
}

void AS11Clip::InsertFramework(uint32_t track_id, string track_name, DMFramework *framework)
{
    HeaderMetadata *header_metadata = GetHeaderMetadata();
    IM_ASSERT(header_metadata);

    MaterialPackage *material_package = header_metadata->getPreface()->findMaterialPackage();
    IM_ASSERT(material_package);

    // Preface - ContentStorage - Package - DM Track
    StaticTrack *dm_track = new StaticTrack(header_metadata);
    material_package->appendTracks(dm_track);
    dm_track->setTrackName(track_name);
    dm_track->setTrackID(track_id);
    dm_track->setTrackNumber(0);

    // Preface - ContentStorage - Package - DM Track - Sequence
    Sequence *sequence = new Sequence(header_metadata);
    dm_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));

    // Preface - ContentStorage - Package - DM Track - Sequence - DMSegment
    DMSegment *dm_segment = new DMSegment(header_metadata);
    sequence->appendStructuralComponents(dm_segment);
    dm_segment->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));

    // move the framework set after the dm degment set
    mxf_remove_set(header_metadata->getCHeaderMetadata(), framework->getCMetadataSet());
    IM_CHECK(mxf_add_set(header_metadata->getCHeaderMetadata(), framework->getCMetadataSet()));

    // Preface - ContentStorage - Package - DM Track - Sequence - DMSegment - DMFramework
    dm_segment->setDMFramework(framework);
}

