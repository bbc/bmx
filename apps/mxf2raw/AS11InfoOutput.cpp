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

#include "AS11InfoOutput.h"
#include <bmx/as11/AS11Info.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/as11/UKDPPDMS.h>
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
    "Control Data / Narration",
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

static const char* get_rational_string(mxfRational ratio)
{
    static char buf[32];
    sprintf(buf, "%d/%d", ratio.numerator, ratio.denominator);
    return buf;
}

static char* get_date_string(mxfTimestamp timestamp)
{
    static char buf[64];
    sprintf(buf, "%d-%02u-%02u", timestamp.year, timestamp.month, timestamp.day);
    return buf;
}

static void print_core_framework(AS11CoreFramework *cf)
{
    printf("  AS-11 Core Framework:\n");
    printf("      ShimName                  : %s\n", cf->GetShimName().c_str());
    printf("      SeriesTitle               : %s\n", cf->GetSeriesTitle().c_str());
    printf("      ProgrammeTitle            : %s\n", cf->GetProgrammeTitle().c_str());
    printf("      EpisodeTitleNumber        : %s\n", cf->GetEpisodeTitleNumber().c_str());
    printf("      AudioTrackLayout          : %u (%s)\n",
           cf->GetAudioTrackLayout(),
           get_audio_track_layout_string(cf->GetAudioTrackLayout()));
    printf("      PrimaryAudioLanguage      : %s\n", cf->GetPrimaryAudioLanguage().c_str());
    printf("      ClosedCaptionsPresent     : %s\n", get_bool_string(cf->GetClosedCaptionsPresent()));
    if (cf->HaveClosedCaptionsType()) {
        printf("      ClosedCaptionsType        : %u (%s)\n",
               cf->GetClosedCaptionsType(),
               get_enum_string(cf->GetClosedCaptionsType(), CAPTIONS_TYPE_STR, ARRAY_SIZE(CAPTIONS_TYPE_STR)));
    }
    if (cf->HaveClosedCaptionsLanguage())
        printf("      ClosedCaptionsLanguage    : %s\n", cf->GetClosedCaptionsLanguage().c_str());
}

static void print_uk_dpp_framework(UKDPPFramework *udf, Timecode start_timecode, Rational frame_rate)
{
    printf("  UK DPP Framework:\n");
    printf("      ProductionNumber          : %s\n", udf->GetProductionNumber().c_str());
    printf("      Synopsis                  : %s\n", udf->GetSynopsis().c_str());
    printf("      Originator                : %s\n", udf->GetOriginator().c_str());
    printf("      CopyrightYear             : %u\n", udf->GetCopyrightYear());
    if (udf->HaveOtherIdentifier())
        printf("      OtherIdentifier           : %s\n", udf->GetOtherIdentifier().c_str());
    if (udf->HaveOtherIdentifierType())
        printf("      OtherIdentifierType       : %s\n", udf->GetOtherIdentifierType().c_str());
    if (udf->HaveGenre())
        printf("      Genre                     : %s\n", udf->GetGenre().c_str());
    if (udf->HaveDistributor())
        printf("      Distributor               : %s\n", udf->GetDistributor().c_str());
    if (udf->HavePictureRatio())
        printf("      PictureRatio              : %s\n", get_rational_string(udf->GetPictureRatio()));
    printf("      ThreeD                    : %s\n", get_bool_string(udf->Get3D()));
    if (udf->Have3DType()) {
        printf("      ThreeDType                : %u (%s)\n",
           udf->Get3DType(),
           get_enum_string(udf->Get3DType(), T3D_TYPE_STR, ARRAY_SIZE(T3D_TYPE_STR)));
    }
    if (udf->HaveProductPlacement())
        printf("      ProductPlacement          : %s\n", get_bool_string(udf->GetProductPlacement()));
    if (udf->HaveFPAPass()) {
        printf("      FPAPass                   : %u (%s)\n",
               udf->GetFPAPass(),
               get_enum_string(udf->GetFPAPass(), FPA_PASS_STR, ARRAY_SIZE(FPA_PASS_STR)));
    }
    if (udf->HaveFPAManufacturer())
        printf("      FPAManufacturer           : %s\n", udf->GetFPAManufacturer().c_str());
    if (udf->HaveFPAVersion())
        printf("      FPAVersion                : %s\n", udf->GetFPAVersion().c_str());
    if (udf->HaveVideoComments())
        printf("      VideoComments             : %s\n", udf->GetVideoComments().c_str());
    printf("      SecondaryAudioLanguage    : %s\n", udf->GetSecondaryAudioLanguage().c_str());
    printf("      TertiaryAudioLanguage     : %s\n", udf->GetTertiaryAudioLanguage().c_str());
    printf("      AudioLoudnessStandard     : %u (%s)\n",
           udf->GetAudioLoudnessStandard(),
           get_enum_string(udf->GetAudioLoudnessStandard(), AUDIO_LOUDNESS_STANDARD_STR,
                           ARRAY_SIZE(AUDIO_LOUDNESS_STANDARD_STR)));
    if (udf->HaveAudioComments())
        printf("      AudioComments             : %s\n", udf->GetAudioComments().c_str());

    Timecode line_up_start = start_timecode;
    line_up_start.AddOffset(udf->GetLineUpStart(), frame_rate);
    printf("      LineUpStart               : %s (%"PRId64" offset)\n",
           get_timecode_string(line_up_start).c_str(),
           udf->GetLineUpStart());

    Timecode ident_clock_start = start_timecode;
    ident_clock_start.AddOffset(udf->GetIdentClockStart(), frame_rate);
    printf("      IdentClockStart           : %s (%"PRId64" offset)\n",
           get_timecode_string(ident_clock_start).c_str(),
           udf->GetIdentClockStart());

    printf("      TotalNumberOfParts        : %u\n", udf->GetTotalNumberOfParts());
    printf("      TotalProgrammeDuration    : %s (%"PRId64" frames)\n",
           get_duration_string(udf->GetTotalProgrammeDuration(), frame_rate).c_str(),
           udf->GetTotalProgrammeDuration());
    printf("      AudioDescriptionPresent   : %s\n", get_bool_string(udf->GetAudioDescriptionPresent()));
    if (udf->HaveAudioDescriptionType()) {
        printf("      AudioDescriptionType      : %u (%s)\n",
               udf->GetAudioDescriptionType(),
               get_enum_string(udf->GetAudioDescriptionType(), AUDIO_DESCRIPTION_TYPE_STR,
                               ARRAY_SIZE(AUDIO_DESCRIPTION_TYPE_STR)));
    }
    printf("      OpenCaptionsPresent       : %s\n", get_bool_string(udf->GetOpenCaptionsPresent()));
    if (udf->HaveOpenCaptionsType()) {
        printf("      OpenCaptionsType          : %u (%s)\n",
               udf->GetOpenCaptionsType(),
               get_enum_string(udf->GetOpenCaptionsType(), CAPTIONS_TYPE_STR, ARRAY_SIZE(CAPTIONS_TYPE_STR)));
    }
    if (udf->HaveOpenCaptionsLanguage())
        printf("      OpenCaptionsLanguage      : %s\n", udf->GetOpenCaptionsLanguage().c_str());
    printf("      SigningPresent            : %u (%s)\n",
               udf->GetSigningPresent(),
               get_enum_string(udf->GetSigningPresent(), SIGNING_PRESENT_STR, ARRAY_SIZE(SIGNING_PRESENT_STR)));
    if (udf->HaveSignLanguage()) {
        printf("      SignLanguage              : %u (%s)\n",
               udf->GetSignLanguage(),
               get_enum_string(udf->GetSignLanguage(), SIGN_LANGUAGE_STR, ARRAY_SIZE(SIGN_LANGUAGE_STR)));
    }
    printf("      CompletionDate            : %s\n", get_date_string(udf->GetCompletionDate()));
    if (udf->HaveTextlessElementsExist())
        printf("      TextlessElementsExist     : %s\n", get_bool_string(udf->GetTextlessElementsExist()));
    if (udf->HaveProgrammeHasText())
        printf("      ProgrammeHasText          : %s\n", get_bool_string(udf->GetProgrammeHasText()));
    if (udf->HaveProgrammeTextLanguage())
        printf("      ProgrammeTextLanguage     : %s\n", udf->GetProgrammeTextLanguage().c_str());
    printf("      ContactEmail              : %s\n", udf->GetContactEmail().c_str());
    printf("      ContactTelephoneNumber    : %s\n", udf->GetContactTelephoneNumber().c_str());
}

static void print_segmentation_framework(DMSegment *seg, Timecode start_timecode, int64_t offset, Rational edit_rate)
{
    AS11SegmentationFramework *framework = dynamic_cast<AS11SegmentationFramework*>(seg->getDMFramework());

    char part_str[12];
    sprintf(part_str, "%u/%u", framework->GetPartNumber(), framework->GetPartTotal());

    Timecode som = start_timecode;
    som.AddOffset(offset, edit_rate);

    printf("      %-15s%-13s%s\n", part_str, get_timecode_string(som).c_str(),
           get_duration_string(seg->getDuration(), edit_rate).c_str());
}



void bmx::as11_register_extensions(MXFFileReader *file_reader)
{
    AS11Info::RegisterExtensions(file_reader->GetDataModel());
}

void bmx::as11_print_info(MXFFileReader *file_reader)
{
    AS11Info info;
    if (!info.Read(file_reader->GetHeaderMetadata())) {
        log_warn("No AS-11 info present in file\n");
        return;
    }

    Rational frame_rate = file_reader->GetSampleRate();
    Timecode start_timecode = file_reader->GetMaterialTimecode(0);
    BMX_CHECK(file_reader->GetFixedLeadFillerOffset() == 0);

    if (!info.segmentation.empty() && info.segmentation_rate != frame_rate) {
        log_warn("AS-11 segmentation track edit rate %d/%d != frame rate %d/%d\n",
                 info.segmentation_rate.numerator, info.segmentation_rate.denominator,
                 frame_rate.numerator, frame_rate.denominator);
    }


    printf("AS-11 Information:\n");

    if (info.core)
        print_core_framework(info.core);
    if (info.ukdpp)
        print_uk_dpp_framework(info.ukdpp, start_timecode, frame_rate);

    if (!info.segmentation.empty()) {
        printf("  Segmentation:\n");
        printf("      PartNo/Total   SOM          Duration\n");

        int64_t offset = 0;
        size_t i;
        for (i = 0; i < info.segmentation.size(); i++) {
            DMSegment *seg = dynamic_cast<DMSegment*>(info.segmentation[i]);
            if (seg)
                print_segmentation_framework(seg, start_timecode, offset, info.segmentation_rate);
            offset += info.segmentation[i]->getDuration();
        }
    }

    printf("\n");
}

