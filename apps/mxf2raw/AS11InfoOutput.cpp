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

#include <string.h>

#include "AS11InfoOutput.h"
#include <bmx/as11/AS11Info.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/as11/UKDPPDMS.h>
#include <bmx/apps/AppTextInfoWriter.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define VERSION_TYPE_VAL(major, minor)  ((((major) & 0xff) << 8) | ((minor) & 0xff))



static const EnumInfo CAPTIONS_TYPE_EINFO[] =
{
    {0,     "Hard_Of_Hearing"},
    {1,     "Translation"},
    {0, 0}
};

static const EnumInfo AUDIO_TRACK_LAYOUT_EINFO[] =
{
    {0,     "EBU_R_48_1a"},
    {1,     "EBU_R_48_1b"},
    {2,     "EBU_R_48_1c"},
    {3,     "EBU_R_48_2a"},
    {4,     "EBU_R_48_2b"},
    {5,     "EBU_R_48_2c"},
    {6,     "EBU_R_48_3a"},
    {7,     "EBU_R_48_3b"},
    {8,     "EBU_R_48_4a"},
    {9,     "EBU_R_48_4b"},
    {10,    "EBU_R_48_4c"},
    {11,    "EBU_R_48_5a"},
    {12,    "EBU_R_48_5b"},
    {13,    "EBU_R_48_6a"},
    {14,    "EBU_R_48_6b"},
    {15,    "EBU_R_48_7a"},
    {16,    "EBU_R_48_7b"},
    {17,    "EBU_R_48_8a"},
    {18,    "EBU_R_48_8b"},
    {19,    "EBU_R_48_8c"},
    {20,    "EBU_R_48_9a"},
    {21,    "EBU_R_48_9b"},
    {22,    "EBU_R_48_10a"},
    {23,    "EBU_R_48_11a"},
    {24,    "EBU_R_48_11b"},
    {25,    "EBU_R_48_11c"},
    {26,    "EBU_R_123_2a"},
    {27,    "EBU_R_123_4a"},
    {28,    "EBU_R_123_4b"},
    {29,    "EBU_R_123_4c"},
    {30,    "EBU_R_123_8a"},
    {31,    "EBU_R_123_8b"},
    {32,    "EBU_R_123_8c"},
    {33,    "EBU_R_123_8d"},
    {34,    "EBU_R_123_8e"},
    {35,    "EBU_R_123_8f"},
    {36,    "EBU_R_123_8g"},
    {37,    "EBU_R_123_8h"},
    {38,    "EBU_R_123_8i"},
    {39,    "EBU_R_123_12a"},
    {40,    "EBU_R_123_12b"},
    {41,    "EBU_R_123_12c"},
    {42,    "EBU_R_123_12d"},
    {43,    "EBU_R_123_12e"},
    {44,    "EBU_R_123_12f"},
    {45,    "EBU_R_123_12g"},
    {46,    "EBU_R_123_12h"},
    {47,    "EBU_R_123_16a"},
    {48,    "EBU_R_123_16b"},
    {49,    "EBU_R_123_16c"},
    {50,    "EBU_R_123_16d"},
    {51,    "EBU_R_123_16e"},
    {52,    "EBU_R_123_16f"},
    {255,   "Undefined"},
    {0, 0}
};

static const EnumInfo PSE_PASS_EINFO[] =
{
    {0,     "Yes"},
    {1,     "No"},
    {2,     "Not_Tested"},
    {0, 0}
};

static const EnumInfo SIGNING_PRESENT_EINFO[] =
{
    {0,     "Yes"},
    {1,     "No"},
    {2,     "Signer_Only"},
    {0, 0}
};

static const EnumInfo T3D_TYPE_EINFO[] =
{
    {0,     "Side_By_Side"},
    {1,     "Dual"},
    {2,     "Left_Eye_Only"},
    {3,     "Right_Eye_Only"},
    {0, 0}
};

static const EnumInfo AUDIO_LOUDNESS_STANDARD_EINFO[] =
{
    {0,     "None"},
    {1,     "EBU_R_128"},
    {0, 0}
};

static const EnumInfo AUDIO_DESCRIPTION_TYPE_EINFO[] =
{
    {0,     "Control_Data_Narration"},
    {1,     "A_D_Mix"},
    {0, 0}
};

static const EnumInfo SIGN_LANGUAGE_EINFO[] =
{
    {0,     "British_Sign_Language"},
    {1,     "Makaton"},
    {0, 0}
};



static void write_core_framework(AppInfoWriter *info_writer, AS11CoreFramework *cf, mxfVersionType shim_version)
{
    AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);
    if (text_writer)
        text_writer->PushItemValueIndent(strlen("closed_captions_present "));

    info_writer->WriteStringItem("shim_name", cf->GetShimName());
    if (shim_version > VERSION_TYPE_VAL(1, 0) || cf->haveItem(&MXF_ITEM_K(AS11CoreFramework, AS11ShimVersion)))
        info_writer->WriteVersionTypeItem("shim_version", cf->GetShimVersion());
    info_writer->WriteStringItem("series_title", cf->GetSeriesTitle());
    info_writer->WriteStringItem("programme_title", cf->GetProgrammeTitle());
    info_writer->WriteStringItem("episode_title_number", cf->GetEpisodeTitleNumber());
    info_writer->WriteEnumItem("audio_track_layout", AUDIO_TRACK_LAYOUT_EINFO, cf->GetAudioTrackLayout());
    info_writer->WriteStringItem("primary_audio_language", cf->GetPrimaryAudioLanguage());
    info_writer->WriteBoolItem("closed_captions_present", cf->GetClosedCaptionsPresent());
    if (cf->HaveClosedCaptionsType())
        info_writer->WriteEnumItem("closed_captions_type", CAPTIONS_TYPE_EINFO, cf->GetClosedCaptionsType());
    if (cf->HaveClosedCaptionsLanguage())
        info_writer->WriteStringItem("closed_captions_language", cf->GetClosedCaptionsLanguage());

    if (text_writer)
        text_writer->PopItemValueIndent();
}

static void write_uk_dpp_framework(AppInfoWriter *info_writer, UKDPPFramework *udf, Timecode start_timecode,
                                   Rational frame_rate, mxfVersionType shim_version)
{
    AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);
    if (text_writer)
        text_writer->PushItemValueIndent(strlen("audio_description_present "));

    info_writer->WriteStringItem("production_number", udf->GetProductionNumber());
    info_writer->WriteStringItem("synopsis", udf->GetSynopsis());
    info_writer->WriteStringItem("originator", udf->GetOriginator());
    info_writer->WriteIntegerItem("copyright_year", udf->GetCopyrightYear());
    if (udf->HaveOtherIdentifier())
        info_writer->WriteStringItem("other_identifier", udf->GetOtherIdentifier());
    if (udf->HaveOtherIdentifierType())
        info_writer->WriteStringItem("other_identifier_type", udf->GetOtherIdentifierType());
    if (udf->HaveGenre())
        info_writer->WriteStringItem("genre", udf->GetGenre());
    if (udf->HaveDistributor())
        info_writer->WriteStringItem("distributor", udf->GetDistributor());
    if (udf->HavePictureRatio())
        info_writer->WriteRationalItem("picture_ratio", udf->GetPictureRatio());
    info_writer->WriteBoolItem("three_d", udf->Get3D());
    if (udf->Have3DType())
        info_writer->WriteEnumItem("three_d_type", T3D_TYPE_EINFO, udf->Get3DType());
    if (udf->HaveProductPlacement())
        info_writer->WriteBoolItem("product_placement", udf->GetProductPlacement());
    if (shim_version > VERSION_TYPE_VAL(1, 0) || udf->haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEPass)))
        info_writer->WriteEnumItem("pse_pass", PSE_PASS_EINFO, udf->GetPSEPass());
    if (udf->HavePSEManufacturer())
        info_writer->WriteStringItem("pse_manufacturer", udf->GetPSEManufacturer());
    if (udf->HavePSEVersion())
        info_writer->WriteStringItem("pse_version", udf->GetPSEVersion());
    if (udf->HaveVideoComments())
        info_writer->WriteStringItem("video_comments", udf->GetVideoComments());
    info_writer->WriteStringItem("secondary_audio_language", udf->GetSecondaryAudioLanguage());
    info_writer->WriteStringItem("tertiary_audio_language", udf->GetTertiaryAudioLanguage());
    info_writer->WriteEnumItem("audio_loudness_standard", AUDIO_LOUDNESS_STANDARD_EINFO,
                               udf->GetAudioLoudnessStandard());
    if (udf->HaveAudioComments())
        info_writer->WriteStringItem("audio_comments", udf->GetAudioComments());

    Timecode line_up_start = start_timecode;
    line_up_start.AddOffset(udf->GetLineUpStart(), frame_rate);
    info_writer->WriteTimecodeItem("line_up_start", line_up_start);

    Timecode ident_clock_start = start_timecode;
    ident_clock_start.AddOffset(udf->GetIdentClockStart(), frame_rate);
    info_writer->WriteTimecodeItem("ident_clock_start", ident_clock_start);

    info_writer->WriteIntegerItem("total_number_of_parts", udf->GetTotalNumberOfParts());
    info_writer->WriteDurationItem("total_programme_duration", udf->GetTotalProgrammeDuration(), frame_rate);
    info_writer->WriteBoolItem("audio_description_present", udf->GetAudioDescriptionPresent());
    if (udf->HaveAudioDescriptionType()) {
        info_writer->WriteEnumItem("audio_description_type", AUDIO_DESCRIPTION_TYPE_EINFO,
                                   udf->GetAudioDescriptionType());
    }
    info_writer->WriteBoolItem("open_captions_present", udf->GetOpenCaptionsPresent());
    if (udf->HaveOpenCaptionsType())
        info_writer->WriteEnumItem("open_captions_type", CAPTIONS_TYPE_EINFO, udf->GetOpenCaptionsType());
    if (udf->HaveOpenCaptionsLanguage())
        info_writer->WriteStringItem("open_captions_language", udf->GetOpenCaptionsLanguage());
    info_writer->WriteEnumItem("signing_present", SIGNING_PRESENT_EINFO, udf->GetSigningPresent());
    if (udf->HaveSignLanguage())
        info_writer->WriteEnumItem("sign_language", SIGN_LANGUAGE_EINFO, udf->GetSignLanguage());
    info_writer->WriteDateOnlyItem("completion_date", udf->GetCompletionDate());
    if (udf->HaveTextlessElementsExist())
        info_writer->WriteBoolItem("textless_elements_exist", udf->GetTextlessElementsExist());
    if (udf->HaveProgrammeHasText())
        info_writer->WriteBoolItem("programme_has_text", udf->GetProgrammeHasText());
    if (udf->HaveProgrammeTextLanguage())
        info_writer->WriteStringItem("programme_text_language", udf->GetProgrammeTextLanguage());
    info_writer->WriteStringItem("contact_email", udf->GetContactEmail());
    info_writer->WriteStringItem("contact_telephone_number", udf->GetContactTelephoneNumber());

    if (text_writer)
        text_writer->PopItemValueIndent();
}

static void write_segmentation_framework(AppInfoWriter *info_writer, DMSegment *seg, Timecode start_timecode,
                                         int64_t offset, Rational edit_rate)
{
    AS11SegmentationFramework *framework = dynamic_cast<AS11SegmentationFramework*>(seg->getDMFramework());

    Timecode som = start_timecode;
    som.AddOffset(offset, edit_rate);

    AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);
    if (text_writer)
        text_writer->PushItemValueIndent(strlen("part_number "));

    info_writer->WriteIntegerItem("part_number", framework->GetPartNumber());
    info_writer->WriteIntegerItem("part_total", framework->GetPartTotal());
    info_writer->WriteTimecodeItem("som", som);
    info_writer->WriteDurationItem("duration", seg->getDuration(), edit_rate);

    if (text_writer)
        text_writer->PopItemValueIndent();
}



void bmx::as11_register_extensions(MXFFileReader *file_reader)
{
    AS11Info::RegisterExtensions(file_reader->GetDataModel());
}

void bmx::as11_write_info(AppInfoWriter *info_writer, MXFFileReader *file_reader)
{
    info_writer->RegisterCCName("as11",     "AS11");
    info_writer->RegisterCCName("uk_dpp",   "UKDPP");

    AS11Info info;
    if (!info.Read(file_reader->GetHeaderMetadata())) {
        log_warn("No AS-11 info present in file\n");
        return;
    }

    Rational frame_rate = file_reader->GetEditRate();
    Timecode start_timecode = file_reader->GetMaterialTimecode(0);
    BMX_CHECK(file_reader->GetFixedLeadFillerOffset() == 0);

    if (!info.segmentation.empty() && info.segmentation_rate != frame_rate) {
        log_warn("AS-11 segmentation track edit rate %d/%d != frame rate %d/%d\n",
                 info.segmentation_rate.numerator, info.segmentation_rate.denominator,
                 frame_rate.numerator, frame_rate.denominator);
    }


    info_writer->StartSection("as11");

    mxfVersionType shim_version = VERSION_TYPE_VAL(1, 0);
    if (info.core) {
        if (info.core->haveItem(&MXF_ITEM_K(AS11CoreFramework, AS11ShimVersion)))
            shim_version = info.core->GetShimVersion();
        info_writer->StartSection("core");
        write_core_framework(info_writer, info.core, shim_version);
        info_writer->EndSection();
    }
    if (info.ukdpp) {
        info_writer->StartSection("uk_dpp");
        write_uk_dpp_framework(info_writer, info.ukdpp, start_timecode, frame_rate, shim_version);
        info_writer->EndSection();
    }

    size_t seg_count = 0;
    size_t i;
    for (i = 0; i < info.segmentation.size(); i++) {
        if (dynamic_cast<DMSegment*>(info.segmentation[i]))
            seg_count++;
    }
    if (seg_count > 0) {
        info_writer->StartArrayItem("segmentation", seg_count);
        int64_t offset = 0;
        size_t index = 0;
        for (i = 0; i < info.segmentation.size(); i++) {
            DMSegment *seg = dynamic_cast<DMSegment*>(info.segmentation[i]);
            if (seg) {
                info_writer->StartArrayElement("segment", index);
                write_segmentation_framework(info_writer, seg, start_timecode, offset, info.segmentation_rate);
                info_writer->EndArrayElement();
                index++;
            }
            offset += info.segmentation[i]->getDuration();
        }
        info_writer->EndArrayItem();
    }

    info_writer->EndSection();
}

