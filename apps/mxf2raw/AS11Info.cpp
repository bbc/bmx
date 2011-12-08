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

#include <AS11Info.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/as11/UKDPPDMS.h>
#include <bmx/as11/AS11CoreFramework.h>
#include <bmx/as11/AS11SegmentationFramework.h>
#include <bmx/as11/UKDPPFramework.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



static const char* CAPTIONS_TYPE_STR[] =
{
    "Hard Of Hearing",
    "Translation",
};

static const char* AUDIO_TRACK_LAYOUT_STR[] =
{
    "EBU R 48: 1a",
    "EBU R 48: 1b",
    "EBU R 48: 1c",
    "EBU R 48: 2a",
    "EBU R 48: 2b",
    "EBU R 48: 2c",
    "EBU R 48: 3a",
    "EBU R 48: 3b",
    "EBU R 48: 4a",
    "EBU R 48: 4b",
    "EBU R 48: 4c",
    "EBU R 48: 5a",
    "EBU R 48: 5b",
    "EBU R 48: 6a",
    "EBU R 48: 6b",
    "EBU R 48: 7a",
    "EBU R 48: 7b",
    "EBU R 48: 8a",
    "EBU R 48: 8b",
    "EBU R 48: 8c",
    "EBU R 48: 9a",
    "EBU R 48: 9b",
    "EBU R 48: 10a",
    "EBU R 48: 11a",
    "EBU R 48: 11b",
    "EBU R 48: 11c",
    "EBU R 123: 2a",
    "EBU R 123: 4a",
    "EBU R 123: 4b",
    "EBU R 123: 4c",
    "EBU R 123: 8a",
    "EBU R 123: 8b",
    "EBU R 123: 8c",
    "EBU R 123: 8d",
    "EBU R 123: 8e",
    "EBU R 123: 8f",
    "EBU R 123: 8g",
    "EBU R 123: 8h",
    "EBU R 123: 8i",
    "EBU R 123: 12a",
    "EBU R 123: 12b",
    "EBU R 123: 12c",
    "EBU R 123: 12d",
    "EBU R 123: 12e",
    "EBU R 123: 12f",
    "EBU R 123: 12g",
    "EBU R 123: 12h",
    "EBU R 123: 16a",
    "EBU R 123: 16b",
    "EBU R 123: 16c",
    "EBU R 123: 16d",
    "EBU R 123: 16e",
    "EBU R 123: 16f",
};

static const char* FPA_PASS_STR[] =
{
    "Yes",
    "No",
    "Not Tested",
};

static const char* SIGNING_PRESENT_STR[] =
{
    "Yes",
    "No",
    "Signer Only",
};

static const char* T3D_TYPE_STR[] =
{
    "Side By Side",
    "Dual",
    "Left Eye Only",
    "Right Eye Only",
};

static const char* AUDIO_LOUDNESS_STANDARD_STR[] =
{
    "None",
    "EBU R 128",
};

static const char* AUDIO_DESCRIPTION_TYPE_STR[] =
{
    "Control Data Narration",
    "A/D Mix",
};

static const char* SIGN_LANGUAGE_STR[] =
{
    "British Sign Language",
    "Makaton",
};




static const char* get_enum_string(uint8_t enum_value, const char **enum_strings, size_t enum_strings_size)
{
    if (enum_value >= enum_strings_size)
        return "unknown";
    else
        return enum_strings[enum_value];
}

static const char* get_audio_track_layout_string(uint8_t audio_track_layout)
{
    if (audio_track_layout == 255)
        return "undefined";
    else
        return get_enum_string(audio_track_layout, AUDIO_TRACK_LAYOUT_STR, ARRAY_SIZE(AUDIO_TRACK_LAYOUT_STR));
}

static const char* get_bool_string(bool value)
{
    if (value)
        return "true";
    else
        return "false";
}

static const char* get_picture_ratio_string(mxfRational ratio)
{
    static char buf[32];
    sprintf(buf, "%d:%d", ratio.numerator, ratio.denominator);
    return buf;
}

static char* get_date_string(mxfTimestamp timestamp)
{
    static char buf[64];
    sprintf(buf, "%d-%02u-%02u", timestamp.year, timestamp.month, timestamp.day);
    return buf;
}


static vector<DMFramework*> get_static_frameworks(MaterialPackage *mp)
{
    vector<DMFramework*> frameworks;

    // expect to find Static DM Track -> Sequence -> DM Segment -> DM Framework

    vector<GenericTrack*> tracks = mp->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        StaticTrack *st = dynamic_cast<StaticTrack*>(tracks[i]);
        if (!st)
            continue;

        StructuralComponent *sc = st->getSequence();
        if (!sc || sc->getDataDefinition() != MXF_DDEF_L(DescriptiveMetadata))
            continue;

        Sequence *seq = dynamic_cast<Sequence*>(sc);
        DMSegment *seg = dynamic_cast<DMSegment*>(sc);
        if (!seq && !seg)
            continue;

        if (seq) {
            vector<StructuralComponent*> scs = seq->getStructuralComponents();
            if (scs.size() != 1)
                continue;

            seg = dynamic_cast<DMSegment*>(scs[0]);
            if (!seg)
                continue;
        }

        if (!seg->haveDMFramework())
            continue;

        frameworks.push_back(seg->getDMFramework());
    }

    return frameworks;
}

static vector<StructuralComponent*> get_segmentation(MaterialPackage *mp, Rational *edit_rate)
{
    // expect to find DM Track -> Sequence -> (Filler | DM Segment -> AS11 Segmentation Framework)+ 

    vector<GenericTrack*> tracks = mp->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *tt = dynamic_cast<Track*>(tracks[i]);
        if (!tt)
            continue;

        StructuralComponent *sc = tt->getSequence();
        if (!sc || sc->getDataDefinition() != MXF_DDEF_L(DescriptiveMetadata))
            continue;

        Sequence *seq = dynamic_cast<Sequence*>(sc);
        if (!seq)
            continue;

        vector<StructuralComponent*> scs = seq->getStructuralComponents();
        if (scs.empty())
            continue;

        size_t j;
        for (j = 0; j < scs.size(); j++) {
            DMSegment *seg = dynamic_cast<DMSegment*>(scs[j]);
            bool is_filler = (*scs[j]->getKey() == MXF_SET_K(Filler));
            if ((!seg && !is_filler) ||
                ((seg && !seg->haveDMFramework())) ||
                (seg && !dynamic_cast<AS11SegmentationFramework*>(seg->getDMFramework())))
            {
                break;
            }
        }
        if (j >= scs.size()) {
            *edit_rate = tt->getEditRate();
            return scs;
        }
    }

    return vector<StructuralComponent*>();
}


static void print_core_framework(AS11CoreFramework *cf)
{
    printf("  AS-11 Core Framework:\n");
    printf("      Shim Name                 : %s\n", cf->GetShimName().c_str());
    printf("      Series Title              : %s\n", cf->GetSeriesTitle().c_str());
    printf("      Programme Title           : %s\n", cf->GetProgrammeTitle().c_str());
    printf("      Episode Title Number      : %s\n", cf->GetEpisodeTitleNumber().c_str());
    printf("      Audio Track Layout        : %u (%s)\n",
           cf->GetAudioTrackLayout(),
           get_audio_track_layout_string(cf->GetAudioTrackLayout()));
    printf("      Primary Audio Language    : %s\n", cf->GetPrimaryAudioLanguage().c_str());
    printf("      Closed Captions Present   : %s\n", get_bool_string(cf->GetClosedCaptionsPresent()));
    if (cf->HaveClosedCaptionsType()) {
        printf("      Closed Captions Type      : %u (%s)\n",
               cf->GetClosedCaptionsType(),
               get_enum_string(cf->GetClosedCaptionsType(), CAPTIONS_TYPE_STR, ARRAY_SIZE(CAPTIONS_TYPE_STR)));
    }
    if (cf->HaveClosedCaptionsLanguage())
        printf("      Closed Captions Language  : %s\n", cf->GetClosedCaptionsLanguage().c_str());
}

static void print_uk_dpp_framework(UKDPPFramework *udf, Rational frame_rate)
{
    printf("  UK DPP Framework:\n");
    printf("      Production Number         : %s\n", udf->GetProductionNumber().c_str());
    printf("      Synopsis                  : %s\n", udf->GetSynopsis().c_str());
    printf("      Originator                : %s\n", udf->GetOriginator().c_str());
    printf("      Copyright Year            : %u\n", udf->GetCopyrightYear());
    if (udf->HaveOtherIdentifier())
        printf("      Other Identifier          : %s\n", udf->GetOtherIdentifier().c_str());
    if (udf->HaveOtherIdentifierType())
        printf("      Other Identifier Type     : %s\n", udf->GetOtherIdentifierType().c_str());
    if (udf->HaveGenre())
        printf("      Genre                     : %s\n", udf->GetGenre().c_str());
    if (udf->HaveDistributor())
        printf("      Distributor               : %s\n", udf->GetDistributor().c_str());
    if (udf->HavePictureRatio())
        printf("      Picture Ratio             : %s\n", get_picture_ratio_string(udf->GetPictureRatio()));
    printf("      3D                        : %s\n", get_bool_string(udf->Get3D()));
    if (udf->Have3DType()) {
        printf("      3D Type                   : %u (%s)\n",
           udf->Get3DType(),
           get_enum_string(udf->Get3DType(), T3D_TYPE_STR, ARRAY_SIZE(T3D_TYPE_STR)));
    }
    if (udf->HaveProductPlacement())
        printf("      Product Placement         : %s\n", get_bool_string(udf->GetProductPlacement()));
    if (udf->HaveFPAPass()) {
        printf("      FPA Pass                  : %u (%s)\n",
               udf->GetFPAPass(),
               get_enum_string(udf->GetFPAPass(), FPA_PASS_STR, ARRAY_SIZE(FPA_PASS_STR)));
    }
    if (udf->HaveFPAManufacturer())
        printf("      FPA Manufacturer          : %s\n", udf->GetFPAManufacturer().c_str());
    if (udf->HaveFPAVersion())
        printf("      FPA Version               : %s\n", udf->GetFPAVersion().c_str());
    if (udf->HaveVideoComments())
        printf("      Video Comments            : %s\n", udf->GetVideoComments().c_str());
    printf("      Secondary Audio Language  : %s\n", udf->GetSecondaryAudioLanguage().c_str());
    printf("      Tertiary Audio Language   : %s\n", udf->GetTertiaryAudioLanguage().c_str());
    printf("      Audio Loudness Standard   : %u (%s)\n",
           udf->GetAudioLoudnessStandard(),
           get_enum_string(udf->GetAudioLoudnessStandard(), AUDIO_LOUDNESS_STANDARD_STR,
                           ARRAY_SIZE(AUDIO_LOUDNESS_STANDARD_STR)));
    if (udf->HaveAudioComments())
        printf("      Audio Comments            : %s\n", udf->GetAudioComments().c_str());
    printf("      Line Up Start             : %"PRId64" (%s)\n",
           udf->GetLineUpStart(),
           get_generic_duration_string_2(udf->GetLineUpStart(), frame_rate).c_str());
    printf("      Ident Clock Start         : %"PRId64" (%s)\n",
           udf->GetIdentClockStart(),
           get_generic_duration_string_2(udf->GetIdentClockStart(), frame_rate).c_str());
    printf("      Total Number Of Parts     : %u\n", udf->GetTotalNumberOfParts());
    printf("      Total Programme Duration  : %"PRId64" (%s)\n",
           udf->GetTotalProgrammeDuration(),
           get_generic_duration_string_2(udf->GetTotalProgrammeDuration(), frame_rate).c_str());
    printf("      Audio Description Present : %s\n", get_bool_string(udf->GetAudioDescriptionPresent()));
    if (udf->HaveAudioDescriptionType()) {
        printf("      Audio Description Type    : %u (%s)\n",
               udf->GetAudioDescriptionType(),
               get_enum_string(udf->GetAudioDescriptionType(), AUDIO_DESCRIPTION_TYPE_STR,
                               ARRAY_SIZE(AUDIO_DESCRIPTION_TYPE_STR)));
    }
    printf("      Open Captions Present     : %s\n", get_bool_string(udf->GetOpenCaptionsPresent()));
    if (udf->HaveOpenCaptionsType()) {
        printf("      Open Captions Type        : %u (%s)\n",
               udf->GetOpenCaptionsType(),
               get_enum_string(udf->GetOpenCaptionsType(), CAPTIONS_TYPE_STR, ARRAY_SIZE(CAPTIONS_TYPE_STR)));
    }
    if (udf->HaveOpenCaptionsLanguage())
        printf("      Open Captions Language    : %s\n", udf->GetOpenCaptionsLanguage().c_str());
    printf("      Signing Present           : %u (%s)\n",
               udf->GetSigningPresent(),
               get_enum_string(udf->GetSigningPresent(), SIGNING_PRESENT_STR, ARRAY_SIZE(SIGNING_PRESENT_STR)));
    if (udf->HaveSignLanguage()) {
        printf("      Sign Language             : %u (%s)\n",
               udf->GetSignLanguage(),
               get_enum_string(udf->GetSignLanguage(), SIGN_LANGUAGE_STR, ARRAY_SIZE(SIGN_LANGUAGE_STR)));
    }
    printf("      Completion Date           : %s\n", get_date_string(udf->GetCompletionDate()));
    if (udf->HaveTextlessElementsExist())
        printf("      Textless Elements Exist   : %s\n", get_bool_string(udf->GetTextlessElementsExist()));
    if (udf->HaveProgrammeHasText())
        printf("      Programme Has Text        : %s\n", get_bool_string(udf->GetProgrammeHasText()));
    if (udf->HaveProgrammeTextLanguage())
        printf("      Programme Text Language   : %s\n", udf->GetProgrammeTextLanguage().c_str());
    printf("      Contact Email             : %s\n", udf->GetContactEmail().c_str());
    printf("      Contact Telephone No      : %s\n", udf->GetContactTelephoneNo().c_str());
}

static void print_segmentation_framework(DMSegment *seg, Rational frame_rate)
{
    AS11SegmentationFramework *framework = dynamic_cast<AS11SegmentationFramework*>(seg->getDMFramework());

    printf("      Part:\n");
    printf("          Duration    : %"PRId64" (%s)\n",
           seg->getDuration(),
           get_generic_duration_string_2(seg->getDuration(), frame_rate).c_str());
    printf("          Part Number : %u\n", framework->GetPartNumber());
    printf("          Part Total  : %u\n", framework->GetPartTotal());
}



void bmx::as11_register_extensions(MXFFileReader *file_reader)
{
    AS11DMS::RegisterExtensions(file_reader->GetDataModel());
    UKDPPDMS::RegisterExtensions(file_reader->GetDataModel());
}

void bmx::as11_print_info(MXFFileReader *file_reader)
{
    HeaderMetadata *header_metadata = file_reader->GetHeaderMetadata();

    AS11CoreFramework::RegisterObjectFactory(header_metadata);
    AS11SegmentationFramework::RegisterObjectFactory(header_metadata);
    UKDPPFramework::RegisterObjectFactory(header_metadata);

    MaterialPackage *mp = file_reader->GetHeaderMetadata()->getPreface()->findMaterialPackage();
    if (!mp) {
        log_warn("No material package found\n");
        return;
    }

    Rational frame_rate = file_reader->GetSampleRate();

    vector<DMFramework*> static_frameworks = get_static_frameworks(mp);

    Rational segmentation_rate;
    vector<StructuralComponent*> segmentation = get_segmentation(mp, &segmentation_rate);

    if (!segmentation.empty() && segmentation_rate != frame_rate) {
        log_warn("AS-11 segmentation track edit rate %d/%d != frame rate %d/%d\n",
                 segmentation_rate.numerator, segmentation_rate.denominator,
                 frame_rate.numerator, frame_rate.denominator);
    }


    printf("AS-11 Information:\n");

    size_t i;
    for (i = 0; i < static_frameworks.size(); i++) {
        AS11CoreFramework *cf = dynamic_cast<AS11CoreFramework*>(static_frameworks[i]);
        UKDPPFramework *udf = dynamic_cast<UKDPPFramework*>(static_frameworks[i]);

        if (cf)
            print_core_framework(cf);
        else if (udf)
            print_uk_dpp_framework(udf, frame_rate);
    }

    if (!segmentation.empty()) {
        printf("  Segmentation:\n");

        for (i = 0; i < segmentation.size(); i++) {
            DMSegment *seg = dynamic_cast<DMSegment*>(segmentation[i]);
            if (seg) {
                print_segmentation_framework(seg, segmentation_rate);
            } else {
                printf("      Filler:\n");
                printf("          Duration    : %"PRId64" (%s)\n",
                       segmentation[i]->getDuration(),
                       get_generic_duration_string_2(segmentation[i]->getDuration(), segmentation_rate).c_str());
            }
        }
    }

    printf("\n");
}

