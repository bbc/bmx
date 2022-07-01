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
#include <cstring>
#include <cerrno>
#include <algorithm>

#include <bmx/apps/AS11Helper.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/as11/UKDPPDMS.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/apps/PropertyFileParser.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const PropertyInfo AS11_CORE_PROPERTY_INFO[] =
{
    {"SeriesTitle",                 MXF_ITEM_K(AS11CoreFramework, AS11SeriesTitle),             127},
    {"ProgrammeTitle",              MXF_ITEM_K(AS11CoreFramework, AS11ProgrammeTitle),          127},
    {"EpisodeTitleNumber",          MXF_ITEM_K(AS11CoreFramework, AS11EpisodeTitleNumber),      127},
    {"ShimName",                    MXF_ITEM_K(AS11CoreFramework, AS11ShimName),                127},
    {"ShimVersion",                 MXF_ITEM_K(AS11CoreFramework, AS11ShimVersion),             0},
    {"AudioTrackLayout",            MXF_ITEM_K(AS11CoreFramework, AS11AudioTrackLayout),        0},
    {"PrimaryAudioLanguage",        MXF_ITEM_K(AS11CoreFramework, AS11PrimaryAudioLanguage),    0},
    {"ClosedCaptionsPresent",       MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsPresent),   0},
    {"ClosedCaptionsType",          MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsType),      0},
    {"ClosedCaptionsLanguage",      MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsLanguage),  0},

    {0, g_Null_Key, 0},
};

static const PropertyInfo AS11_SEGMENTATION_PROPERTY_INFO[] =
{
    {"PartNumber",                  MXF_ITEM_K(AS11SegmentationFramework,   AS11PartNumber),    0},
    {"PartTotal",                   MXF_ITEM_K(AS11SegmentationFramework,   AS11PartTotal),     0},

    {0, g_Null_Key, 0},
};

static const PropertyInfo UK_DPP_PROPERTY_INFO[] =
{
    {"ProductionNumber",            MXF_ITEM_K(UKDPPFramework, UKDPPProductionNumber),          127},
    {"Synopsis",                    MXF_ITEM_K(UKDPPFramework, UKDPPSynopsis),                  250},
    {"Originator",                  MXF_ITEM_K(UKDPPFramework, UKDPPOriginator),                127},
    {"CopyrightYear",               MXF_ITEM_K(UKDPPFramework, UKDPPCopyrightYear),             0},
    {"OtherIdentifier",             MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifier),           127},
    {"OtherIdentifierType",         MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifierType),       127},
    {"Genre",                       MXF_ITEM_K(UKDPPFramework, UKDPPGenre),                     127},
    {"Distributor",                 MXF_ITEM_K(UKDPPFramework, UKDPPDistributor),               127},
    {"PictureRatio",                MXF_ITEM_K(UKDPPFramework, UKDPPPictureRatio),              0},
    {"ThreeD",                      MXF_ITEM_K(UKDPPFramework, UKDPP3D),                        0},
    {"3D",                          MXF_ITEM_K(UKDPPFramework, UKDPP3D),                        0},
    {"ThreeDType",                  MXF_ITEM_K(UKDPPFramework, UKDPP3DType),                    0},
    {"3DType",                      MXF_ITEM_K(UKDPPFramework, UKDPP3DType),                    0},
    {"ProductPlacement",            MXF_ITEM_K(UKDPPFramework, UKDPPProductPlacement),          0},
    {"PSEPass",                     MXF_ITEM_K(UKDPPFramework, UKDPPPSEPass),                   0},
    {"PSEManufacturer",             MXF_ITEM_K(UKDPPFramework, UKDPPPSEManufacturer),           127},
    {"PSEVersion",                  MXF_ITEM_K(UKDPPFramework, UKDPPPSEVersion),                127},
    {"VideoComments",               MXF_ITEM_K(UKDPPFramework, UKDPPVideoComments),             127},
    {"SecondaryAudioLanguage",      MXF_ITEM_K(UKDPPFramework, UKDPPSecondaryAudioLanguage),    0},
    {"TertiaryAudioLanguage",       MXF_ITEM_K(UKDPPFramework, UKDPPTertiaryAudioLanguage),     0},
    {"AudioLoudnessStandard",       MXF_ITEM_K(UKDPPFramework, UKDPPAudioLoudnessStandard),     0},
    {"AudioComments",               MXF_ITEM_K(UKDPPFramework, UKDPPAudioComments),             127},
    {"LineUpStart",                 MXF_ITEM_K(UKDPPFramework, UKDPPLineUpStart),               0},
    {"IdentClockStart",             MXF_ITEM_K(UKDPPFramework, UKDPPIdentClockStart),           0},
    {"TotalNumberOfParts",          MXF_ITEM_K(UKDPPFramework, UKDPPTotalNumberOfParts),        0},
    {"TotalProgrammeDuration",      MXF_ITEM_K(UKDPPFramework, UKDPPTotalProgrammeDuration),    0},
    {"AudioDescriptionPresent",     MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionPresent),   0},
    {"AudioDescriptionType",        MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionType),      0},
    {"OpenCaptionsPresent",         MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsPresent),       0},
    {"OpenCaptionsType",            MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsType),          0},
    {"OpenCaptionsLanguage",        MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsLanguage),      0},
    {"SigningPresent",              MXF_ITEM_K(UKDPPFramework, UKDPPSigningPresent),            0},
    {"SignLanguage",                MXF_ITEM_K(UKDPPFramework, UKDPPSignLanguage),              0},
    {"CompletionDate",              MXF_ITEM_K(UKDPPFramework, UKDPPCompletionDate),            0},
    {"TextlessElementsExist",       MXF_ITEM_K(UKDPPFramework, UKDPPTextlessElementsExist),     0},
    {"ProgrammeHasText",            MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeHasText),          0},
    {"ProgrammeTextLanguage",       MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeTextLanguage),     0},
    {"ContactEmail",                MXF_ITEM_K(UKDPPFramework, UKDPPContactEmail),              127},
    {"ContactTelephoneNumber",      MXF_ITEM_K(UKDPPFramework, UKDPPContactTelephoneNumber),    127},

    {0, g_Null_Key, 0},
};

static const FrameworkInfo AS11_FRAMEWORK_INFO[] =
{
    {"AS11CoreFramework",           MXF_SET_K(AS11CoreFramework),           AS11_CORE_PROPERTY_INFO},
    {"AS11SegmentationFramework",   MXF_SET_K(AS11SegmentationFramework),   AS11_SEGMENTATION_PROPERTY_INFO},
    {"UKDPPFramework",              MXF_SET_K(UKDPPFramework),              UK_DPP_PROPERTY_INFO},

    {0, g_Null_Key, 0},
};

typedef struct
{
    const char *name;
    AS11SpecificationId id;
} SpecIdMap;

static const SpecIdMap AS11_SPEC_ID_MAP[] =
{
    {"as11-x1",   AS11_X1_SPEC},
    {"as11-x2",   AS11_X2_SPEC},
    {"as11-x3",   AS11_X3_SPEC},
    {"as11-x4",   AS11_X4_SPEC},
    {"as11-x5",   AS11_X5_SPEC},
    {"as11-x6",   AS11_X6_SPEC},
    {"as11-x7",   AS11_X7_SPEC},
    {"as11-x8",   AS11_X8_SPEC},
    {"as11-x9",   AS11_X9_SPEC},
};


static string get_short_name(string name)
{
    if (name.compare(0, strlen("AS11"), "AS11") == 0)
        return name.substr(strlen("AS11"));
    else if (name.compare(0, strlen("UKDPP"), "UKDPP") == 0)
        return name.substr(strlen("UKDPP"));
    else
        return name;
}



bool AS11Helper::ParseXMLSchemeId(const string &scheme_id_str, UL *label)
{
    if (scheme_id_str == "as11")
        *label = AS11_DM_XML_Document;
    else
        return false;

    return true;
}

AS11Helper::AS11Helper()
{
    mNormaliseStrings = false;
    mFillerCompleteSegments = false;
    mSourceInfo = 0;
    mWriterHelper = 0;
    mAS11FrameworkHelper = 0;
    mUKDPPFrameworkHelper = 0;
    mHaveUKDPPTotalNumberOfParts = false;
    mHaveUKDPPTotalProgrammeDuration = false;
    mAS11SpecId = UNKNOWN_AS11_SPEC;
}

AS11Helper::~AS11Helper()
{
    delete mSourceInfo;
    delete mWriterHelper;
    delete mAS11FrameworkHelper;
    delete mUKDPPFrameworkHelper;
}

void AS11Helper::SetNormaliseStrings(bool enable)
{
    mNormaliseStrings = enable;
}

void AS11Helper::ReadSourceInfo(MXFFileReader *source_file)
{
    int64_t read_start_pos;
    int64_t read_duration;
    source_file->GetReadLimits(false, &read_start_pos, &read_duration);
    if (read_start_pos != source_file->GetReadStartPosition() ||
        read_duration  != source_file->GetReadDuration())
    {
        BMX_EXCEPTION(("Copying AS-11 descriptive metadata is currently only supported for complete file duration transfers"));
    }

    mSourceInfo = new AS11Info();
    mSourceInfo->Read(source_file->GetHeaderMetadata());

    mSourceStartTimecode = source_file->GetMaterialTimecode(0);

    if (mSourceInfo->core && mSourceInfo->core->haveItem(&MXF_ITEM_K(AS11CoreFramework, AS11ProgrammeTitle)))
        mSourceProgrammeTitle = mSourceInfo->core->GetProgrammeTitle();

    int64_t offset = 0;
    size_t i;
    for (i = 0; i < mSourceInfo->segmentation.size(); i++) {
        DMSegment *seg = dynamic_cast<DMSegment*>(mSourceInfo->segmentation[i]);
        if (seg) {
            AS11SegmentationFramework *framework = dynamic_cast<AS11SegmentationFramework*>(seg->getDMFramework());
            BMX_CHECK(framework);

            Timecode seg_start_tc = mSourceStartTimecode;
            seg_start_tc.AddOffset(offset, mSourceInfo->segmentation_rate);

            AS11TCSegment segment;
            segment.part_number  = framework->GetPartNumber();
            segment.part_total   = framework->GetPartTotal();
            segment.start        = seg_start_tc;
            segment.duration     = mSourceInfo->segmentation[i]->getDuration();
            mSegments.push_back(segment);
        }
        offset += mSourceInfo->segmentation[i]->getDuration();
    }

    mFillerCompleteSegments = true;
}

bool AS11Helper::SupportFrameworkType(const char *type_str)
{
    FrameworkType type;
    return ParseFrameworkType(type_str, &type);
}

bool AS11Helper::ParseFrameworkFile(const char *type_str, const char *filename)
{
    FrameworkType type;
    if (!ParseFrameworkType(type_str, &type)) {
        log_warn("Unknown AS-11 framework type '%s'\n", type_str);
        return false;
    }

    PropertyFileParser property_parser;
    if (!property_parser.Open(filename)) {
        fprintf(stderr, "Failed to open framework file '%s'\n", filename);
        return false;
    }

    string name;
    string value;
    while (property_parser.ParseNext(&name, &value)) {
        SetFrameworkProperty(type, name, value);
    }
    if (property_parser.HaveError()) {
        fprintf(stderr, "Failed to parse framework file '%s'\n", filename);
        return false;
    }

    return true;
}

bool AS11Helper::ParseSegmentationFile(const char *filename, Rational frame_rate)
{
    mSegments.clear();
    mFillerCompleteSegments = false;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, bmx_strerror(errno).c_str());
        return false;
    }

    bool last_tc_xx = false;
    int line_num = 0;
    int c = '1';
    while (c != EOF) {
        // move file pointer past the newline characters
        while ((c = fgetc(file)) != EOF && (c == '\r' || c == '\n')) {
            if (c == '\n')
                line_num++;
        }

        // skip leading space
        while (c == ' ' || c == '\t')
            c = fgetc(file);

        // line
        string line_str;
        while (c != EOF && (c != '\r' && c != '\n')) {
            line_str += c;
            c = fgetc(file);
        }

        int part_num = 0, part_total = 0;
        int som_h = 0, som_m = 0, som_s = 0, som_f = 0;
        int eom_h = 0, eom_m = 0, eom_s = 0, eom_f = 0;
        int dur_h = 0, dur_m = 0, dur_s = 0, dur_f = 0;
        char som_dc = 0;
        char eom_dc = 0;
        char dur_dc = 0;
        char xc = 0;
        if (!line_str.empty()) {
            int line_type = 0;
            if (sscanf(line_str.c_str(), "%d/%d %d:%d:%d%c%d E%d:%d:%d%c%d",
                       &part_num, &part_total,
                       &som_h, &som_m, &som_s, &som_dc, &som_f,
                       &eom_h, &eom_m, &eom_s, &eom_dc, &eom_f) == 12)
            {
                line_type = 1;
            }
            else if (sscanf(line_str.c_str(), "%d/%d %d:%d:%d%c%d D%d:%d:%d%c%d",
                            &part_num, &part_total,
                            &som_h, &som_m, &som_s, &som_dc, &som_f,
                            &dur_h, &dur_m, &dur_s, &dur_dc, &dur_f) == 12 ||
                     sscanf(line_str.c_str(), "%d/%d %d:%d:%d%c%d %d:%d:%d%c%d",
                            &part_num, &part_total,
                            &som_h, &som_m, &som_s, &som_dc, &som_f,
                            &dur_h, &dur_m, &dur_s, &dur_dc, &dur_f) == 12)
            {
                line_type = 2;
            }
            else if (sscanf(line_str.c_str(), "%d/%d %d:%d:%d%c%d %c",
                            &part_num, &part_total,
                            &som_h, &som_m, &som_s, &som_dc, &som_f, &xc) == 8 &&
                     (xc == 'x' || xc == 'X'))
            {
                line_type = 3;
            }
            if (line_type == 0) {
                fprintf(stderr, "Failed to parse segment line %d\n", line_num);
                fclose(file);
                mSegments.clear();
                return false;
            }

            AS11TCSegment segment;
            segment.part_number  = part_num;
            segment.part_total   = part_total;
            segment.start.Init(frame_rate, (som_dc != ':'), som_h, som_m, som_s, som_f);
            segment.duration     = 0;
            if (line_type == 1) {
                Timecode end(frame_rate, (eom_dc != ':'), eom_h, eom_m, eom_s, eom_f);
                segment.duration = end.GetOffset() - segment.start.GetOffset();
                if (segment.duration <= 0) {
                    fprintf(stderr, "Invalid end timecode on line %d resulting in duration %" PRId64 "\n",
                            line_num, segment.duration);
                    fclose(file);
                    mSegments.clear();
                    return false;
                }
            } else if (line_type == 2) {
                Timecode end = segment.start;
                end.AddDuration((dur_dc != ':'), dur_h, dur_m, dur_s, dur_f);
                segment.duration = end.GetOffset() - segment.start.GetOffset();
            }
            mSegments.push_back(segment);

            if (line_type == 3) {
                last_tc_xx = true;
                break;
            }
        }

        if (c == '\n')
            line_num++;
    }

    mFillerCompleteSegments = !last_tc_xx;


    fclose(file);

    return true;
}

bool AS11Helper::SetFrameworkProperty(const char *type_str, const char *name, const char *value)
{
    FrameworkType type;
    if (!ParseFrameworkType(type_str, &type)) {
        log_warn("Unknown AS-11 framework type '%s'\n", type_str);
        return false;
    }

    SetFrameworkProperty(type, name, value);
    return true;
}

bool AS11Helper::ParseSpecificationId(const string &spec_id_str)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(AS11_SPEC_ID_MAP); i++) {
        if (spec_id_str == AS11_SPEC_ID_MAP[i].name) {
            mAS11SpecId = AS11_SPEC_ID_MAP[i].id;
            return true;
        }
    }

    return false;
}

bool AS11Helper::HaveProgrammeTitle() const
{
    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS11_CORE_FRAMEWORK_TYPE &&
            get_short_name(mFrameworkProperties[i].name) == "ProgrammeTitle")
        {
            return true;
        }
    }

    return !mSourceProgrammeTitle.empty();
}

string AS11Helper::GetProgrammeTitle() const
{
    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS11_CORE_FRAMEWORK_TYPE &&
            get_short_name(mFrameworkProperties[i].name) == "ProgrammeTitle")
        {
            return mFrameworkProperties[i].value;
        }
    }

    return mSourceProgrammeTitle;
}

bool AS11Helper::HaveAS11CoreFramework() const
{
    if ((mSourceInfo && mSourceInfo->core) || mAS11FrameworkHelper)
        return true;

    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS11_CORE_FRAMEWORK_TYPE)
            return true;
    }

    return false;
}

void AS11Helper::AddMetadata(ClipWriter *clip)
{
    if (clip->GetType() != CW_OP1A_CLIP_TYPE &&
        clip->GetType() != CW_D10_CLIP_TYPE &&
        clip->GetType() != CW_RDD9_CLIP_TYPE)
    {
        BMX_EXCEPTION(("AS-11 is not supported in clip type '%s'",
                       clip_type_to_string(clip->GetType(), NO_CLIP_SUB_TYPE)));
    }

    mWriterHelper = new AS11WriterHelper(clip);

    Timecode start_tc   = mWriterHelper->GetClip()->GetStartTimecode();
    Rational frame_rate = mWriterHelper->GetClip()->GetFrameRate();

    mWriterHelper->SetSpecificationId(mAS11SpecId);

    if (mSourceInfo) {
        if (mSourceInfo->core) {
            AS11CoreFramework::RegisterObjectFactory(clip->GetHeaderMetadata());
            AS11CoreFramework *core_copy =
                dynamic_cast<AS11CoreFramework*>(mSourceInfo->core->clone(clip->GetHeaderMetadata()));
            mAS11FrameworkHelper = new FrameworkHelper(core_copy, AS11_FRAMEWORK_INFO, start_tc, frame_rate);
            if (mNormaliseStrings) {
                mAS11FrameworkHelper->NormaliseStrings();
            }
        }
        if (mSourceInfo->ukdpp) {
            UKDPPFramework::RegisterObjectFactory(clip->GetHeaderMetadata());
            UKDPPFramework *ukdpp_copy =
                dynamic_cast<UKDPPFramework*>(mSourceInfo->ukdpp->clone(clip->GetHeaderMetadata()));
            mUKDPPFrameworkHelper = new FrameworkHelper(ukdpp_copy, AS11_FRAMEWORK_INFO, start_tc, frame_rate);
            if (mNormaliseStrings) {
                mUKDPPFrameworkHelper->NormaliseStrings();
            }
        }
    }

    size_t i;
    for (i = 0; i < mFrameworkProperties.size(); i++) {
        if (mFrameworkProperties[i].type == AS11_CORE_FRAMEWORK_TYPE) {
            if (!mAS11FrameworkHelper) {
                AS11CoreFramework *core_fw = new AS11CoreFramework(mWriterHelper->GetClip()->GetHeaderMetadata());
                mAS11FrameworkHelper = new FrameworkHelper(core_fw, AS11_FRAMEWORK_INFO, start_tc, frame_rate);
            }
            BMX_CHECK_M(mAS11FrameworkHelper->SetProperty(get_short_name(mFrameworkProperties[i].name), mFrameworkProperties[i].value),
                        ("Failed to set AS11CoreFramework property '%s' to '%s'",
                         mFrameworkProperties[i].name.c_str(), mFrameworkProperties[i].value.c_str()));
        } else {
            if (!mUKDPPFrameworkHelper) {
                UKDPPFramework *ukdpp_fw = new UKDPPFramework(mWriterHelper->GetClip()->GetHeaderMetadata());
                mUKDPPFrameworkHelper = new FrameworkHelper(ukdpp_fw, AS11_FRAMEWORK_INFO, start_tc, frame_rate);
            }
            BMX_CHECK_M(mUKDPPFrameworkHelper->SetProperty(get_short_name(mFrameworkProperties[i].name), mFrameworkProperties[i].value),
                        ("Failed to set UKDPPCoreFramework property '%s' to '%s'",
                         mFrameworkProperties[i].name.c_str(), mFrameworkProperties[i].value.c_str()));
        }
    }


    if (mAS11FrameworkHelper) {
        mWriterHelper->InsertAS11CoreFramework(dynamic_cast<AS11CoreFramework*>(mAS11FrameworkHelper->GetFramework()));
        BMX_CHECK_M(mAS11FrameworkHelper->GetFramework()->validate(true), ("AS11 Framework validation failed"));
    }

    if (!mSegments.empty())
        mWriterHelper->InsertTCSegmentation(mSegments);

    if (mUKDPPFrameworkHelper) {
        mWriterHelper->InsertUKDPPFramework(dynamic_cast<UKDPPFramework*>(mUKDPPFrameworkHelper->GetFramework()));

        // make sure UKDPPTotalNumberOfParts and UKDPPTotalProgrammeDuration are set (default 0) for validation
        UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(mUKDPPFrameworkHelper->GetFramework());
        if (dpp_framework->haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalNumberOfParts)))
            mHaveUKDPPTotalNumberOfParts = true;
        else
            dpp_framework->SetTotalNumberOfParts(0);
        if (dpp_framework->haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalProgrammeDuration)))
            mHaveUKDPPTotalProgrammeDuration = true;
        else
            dpp_framework->SetTotalProgrammeDuration(0);

        BMX_CHECK_M(dpp_framework->validate(true), ("UK DPP Framework validation failed"));
    }
}

void AS11Helper::Complete()
{
    BMX_ASSERT(mWriterHelper);

    if (!mSegments.empty())
        mWriterHelper->CompleteSegmentation(mFillerCompleteSegments);

    if (mUKDPPFrameworkHelper) {
        // calculate or check total number of parts and programme duration
        UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(mUKDPPFrameworkHelper->GetFramework());
        if (mHaveUKDPPTotalNumberOfParts) {
            BMX_CHECK_M(mWriterHelper->GetTotalSegments() == dpp_framework->GetTotalNumberOfParts(),
                        ("UKDPPTotalNumberOfParts value %u does not equal actual total part count %u",
                         dpp_framework->GetTotalNumberOfParts(), mWriterHelper->GetTotalSegments()));
        } else {
            dpp_framework->SetTotalNumberOfParts(mWriterHelper->GetTotalSegments());
        }
        if (mHaveUKDPPTotalProgrammeDuration) {
            BMX_CHECK_M(dpp_framework->GetTotalProgrammeDuration() >= mWriterHelper->GetTotalSegmentDuration(),
                        ("UKDPPTotalProgrammeDuration value %" PRId64 " is less than duration of parts in this "
                         "file %" PRId64,
                         dpp_framework->GetTotalProgrammeDuration(), mWriterHelper->GetTotalSegmentDuration()));
        } else {
            dpp_framework->SetTotalProgrammeDuration(mWriterHelper->GetTotalSegmentDuration());
        }
    }
}

bool AS11Helper::ParseFrameworkType(const char *type_str, FrameworkType *type) const
{
    if (strcmp(type_str, "as11") == 0) {
        *type = AS11_CORE_FRAMEWORK_TYPE;
        return true;
    } else if (strcmp(type_str, "dpp") == 0) {
        *type = DPP_FRAMEWORK_TYPE;
        return true;
    } else {
        return false;
    }
}

void AS11Helper::SetFrameworkProperty(FrameworkType type, std::string name, std::string value)
{
    FrameworkProperty framework_property;
    framework_property.type  = type;
    framework_property.name  = name;
    framework_property.value = value;

    mFrameworkProperties.push_back(framework_property);
}
