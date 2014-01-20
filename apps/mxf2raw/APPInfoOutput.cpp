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
#define __STDC_LIMIT_MACROS

#include <cstdio>
#include <cstring>

#include "APPInfoOutput.h"
#include <bmx/mxf_reader/MXFAPPInfo.h>
#include <bmx/apps/AppTextInfoWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const EnumInfo VTR_STATUS_EINFO[] =
{
    {0,      "Good"},
    {1,      "Almost_Good"},
    {2,      "State_Cannot_Be_Determined"},
    {3,      "State_Unclear"},
    {4,      "No_Good"},
    {5,      "Good"},
    {6,      "Good"},
    {7,      "Good"},
    {0, 0}
};



static string get_archive_tc_string(const ArchiveTimecode &timecode)
{
    BMX_ASSERT(timecode.hour != INVALID_TIMECODE_HOUR);

    char buf[64];
    bmx_snprintf(buf, sizeof(buf), "%02u:%02u:%02u%c%02u",
                 timecode.hour, timecode.min, timecode.sec, (timecode.dropFrame ? ';' : ':'), timecode.frame);
    return buf;
}

static void write_infax_data(AppInfoWriter *info_writer, const InfaxData *data)
{
    AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);
    const Rational second_rate = {1, 1};

#define WRITE_STRING_ITEM(var, name)                            \
    if (data->var && data->var[0])                              \
        info_writer->WriteStringItem(name, data->var);

#define WRITE_TIMESTAMP_ITEM(var, name)                         \
    if (data->var.year || data->var.month || data->var.day)     \
        info_writer->WriteDateOnlyItem(name, data->var);

    if (text_writer)
        text_writer->PushItemValueIndent(strlen("transmission_date "));

    WRITE_STRING_ITEM(format,               "format")
    WRITE_STRING_ITEM(progTitle,            "programme_title")
    WRITE_STRING_ITEM(epTitle,              "episode_title")
    WRITE_TIMESTAMP_ITEM(txDate,            "transmission_date")
    WRITE_STRING_ITEM(magPrefix,            "magazine_prefix")
    WRITE_STRING_ITEM(progNo,               "programme_number")
    WRITE_STRING_ITEM(prodCode,             "production_code")
    WRITE_STRING_ITEM(spoolStatus,          "spool_status")
    WRITE_TIMESTAMP_ITEM(stockDate,         "stock_date")
    WRITE_STRING_ITEM(spoolDesc,            "spool_descriptor")
    WRITE_STRING_ITEM(memo,                 "memo")
    if (data->duration != 0)
        info_writer->WriteDurationItem(     "duration", data->duration, second_rate); // Infax duration is in seconds
    WRITE_STRING_ITEM(spoolNo,              "spoolNumber")
    WRITE_STRING_ITEM(accNo,                "accession_number")
    WRITE_STRING_ITEM(catDetail,            "catalogue_detail")
    info_writer->WriteIntegerItem(          "item_number", data->itemNo);

    if (text_writer)
        text_writer->PopItemValueIndent();
}



APPInfoOutput::APPInfoOutput()
{
    mFileReader = 0;
    mHaveInfo = false;
    mPSEFailureTCIndex = 0;
    mVTRErrorTCIndex = 0;
    mDigiBetaDropoutTCIndex = 0;
    mTimecodeBreakTCIndex = 0;
}

APPInfoOutput::~APPInfoOutput()
{
}

void APPInfoOutput::RegisterExtensions(MXFFileReader *file_reader)
{
    mFileReader = file_reader;
    MXFAPPInfo::RegisterExtensions(file_reader->GetDataModel());
}

bool APPInfoOutput::CheckIssues()
{
    return mInfo.CheckIssues(mFileReader->GetHeaderMetadata());
}

void APPInfoOutput::ExtractInfo(int event_mask)
{
    if (!mInfo.ReadInfo(mFileReader->GetHeaderMetadata())) {
        log_warn("No APP info present in file\n");
        mHaveInfo = false;
        return;
    }
    mHaveInfo = true;

    if ((event_mask & DIGIBETA_DROPOUT_MASK) && !mInfo.ReadDigiBetaDropouts(mFileReader->GetHeaderMetadata()))
        log_warn("Failed to read APP Digibeta dropout events\n");

    if ((event_mask & PSE_FAILURE_MASK) && !mInfo.ReadPSEFailures(mFileReader->GetHeaderMetadata()))
        log_warn("Failed to read APP PSE failure events\n");

    if ((event_mask & TIMECODE_BREAK_MASK) && !mInfo.ReadTimecodeBreaks(mFileReader->GetHeaderMetadata()))
        log_warn("Failed to read APP timecode break events\n");

    if ((event_mask & VTR_ERROR_MASK) && !mInfo.ReadVTRErrors(mFileReader->GetHeaderMetadata()))
        log_warn("Failed to read APP VTR error events\n");


    APPTimecodeInfo tc_info;
    memset(&tc_info, 0, sizeof(tc_info));
    tc_info.vitc.hour = INVALID_TIMECODE_HOUR;
    tc_info.ltc.hour  = INVALID_TIMECODE_HOUR;

    mPSEFailureTimecodes.assign(mInfo.num_pse_failures, tc_info);
    mVTRErrorTimecodes.assign(mInfo.num_vtr_errors, tc_info);
    mDigiBetaDropoutTimecodes.assign(mInfo.num_digibeta_dropouts, tc_info);
    mTimecodeBreakTimecodes.assign(mInfo.num_timecode_breaks, tc_info);
}

void APPInfoOutput::AddEventTimecodes(int64_t position, Timecode vitc, Timecode ltc)
{
#define COPY_EVENT_TC(tt)                                   \
    tc_info->tt.hour       = (uint8_t)tt.GetHour();         \
    tc_info->tt.min        = (uint8_t)tt.GetMin();          \
    tc_info->tt.sec        = (uint8_t)tt.GetSec();          \
    tc_info->tt.frame      = (uint8_t)tt.GetFrame();        \
    tc_info->tt.dropFrame  = tt.IsDropFrame();

#define ADD_EVENT_TC(index, array, info)            \
    while (index < array.size() &&                  \
           info[index].position == position)        \
    {                                               \
        tc_info = &array[index];                    \
        COPY_EVENT_TC(vitc)                         \
        COPY_EVENT_TC(ltc)                          \
        index++;                                    \
    }

    APPTimecodeInfo *tc_info;
    ADD_EVENT_TC(mPSEFailureTCIndex,       mPSEFailureTimecodes,       mInfo.pse_failures)
    ADD_EVENT_TC(mVTRErrorTCIndex,         mVTRErrorTimecodes,         mInfo.vtr_errors)
    ADD_EVENT_TC(mDigiBetaDropoutTCIndex,  mDigiBetaDropoutTimecodes,  mInfo.digibeta_dropouts)
    ADD_EVENT_TC(mTimecodeBreakTCIndex,    mTimecodeBreakTimecodes,    mInfo.timecode_breaks)
}

void APPInfoOutput::CompleteEventTimecodes()
{
    size_t enabled_track_index = 0;
    size_t i;
    for (i = 0; i < mFileReader->GetNumTrackReaders(); i++) {
        if (mFileReader->GetTrackReader(i)->IsEnabled()) {
            enabled_track_index = i;
            break;
        }
    }
    if (i >= mFileReader->GetNumTrackReaders()) {
        log_warn("No supported tracks to allow event timecodes to be filled in\n");
        return;
    }

#define UPDATE_TC_INDEX(index, array)                               \
    while (index < array.size() &&                                  \
           array[index].vitc.hour != INVALID_TIMECODE_HOUR)         \
    {                                                               \
        index++;                                                    \
    }

#define MIN_READ_POS(index, array, info)                            \
    if (index < array.size() && info[index].position < position)    \
        position = info[index].position;                            \

    mPSEFailureTCIndex = 0;
    mVTRErrorTCIndex = 0;
    mDigiBetaDropoutTCIndex = 0;
    mTimecodeBreakTCIndex = 0;

    int64_t position = 0;
    Frame *frame = 0;
    while (true) {
        UPDATE_TC_INDEX(mPSEFailureTCIndex,       mPSEFailureTimecodes)
        UPDATE_TC_INDEX(mVTRErrorTCIndex,         mVTRErrorTimecodes)
        UPDATE_TC_INDEX(mDigiBetaDropoutTCIndex,  mDigiBetaDropoutTimecodes)
        UPDATE_TC_INDEX(mTimecodeBreakTCIndex,    mTimecodeBreakTimecodes)

        position = INT64_MAX;
        MIN_READ_POS(mPSEFailureTCIndex,       mPSEFailureTimecodes,       mInfo.pse_failures)
        MIN_READ_POS(mVTRErrorTCIndex,         mVTRErrorTimecodes,         mInfo.vtr_errors)
        MIN_READ_POS(mDigiBetaDropoutTCIndex,  mDigiBetaDropoutTimecodes,  mInfo.digibeta_dropouts)
        MIN_READ_POS(mTimecodeBreakTCIndex,    mTimecodeBreakTimecodes,    mInfo.timecode_breaks)
        if (position == INT64_MAX)
            break;

        mFileReader->Seek(position);
        if (mFileReader->Read(1) != 1) {
            log_warn("Failed to read frame at position %"PRId64"\n", position);
            break;
        }

        frame = mFileReader->GetTrackReader(enabled_track_index)->GetFrameBuffer()->GetLastFrame(true);
        for (i = 0; i < mFileReader->GetNumTrackReaders(); i++)
            mFileReader->GetTrackReader(i)->GetFrameBuffer()->Clear(true);

        if (!frame || frame->IsEmpty()) {
            log_warn("Failed to read frame from track %"PRIszt" at position %"PRId64"\n",
                     enabled_track_index, position);
            break;
        }

        const vector<FrameMetadata*> *metadata = frame->GetMetadata(SYSTEM_SCHEME_1_FMETA_ID);
        if (!metadata) {
            log_warn("System Scheme 1 metadata not present in frame\n");
            break;
        }

        for (i = 0; i < metadata->size(); i++) {
            const SystemScheme1Metadata *ss1_meta =
                dynamic_cast<const SystemScheme1Metadata*>((*metadata)[i]);

            if (ss1_meta->GetType() == SystemScheme1Metadata::TIMECODE_ARRAY) {
                const SS1TimecodeArray *tc_array = dynamic_cast<const SS1TimecodeArray*>(ss1_meta);
                AddEventTimecodes(position, tc_array->GetVITC(), tc_array->GetLTC());
                break;
            }
        }
        if (i >= metadata->size()) {
            log_warn("System Scheme 1 timecode array metadata not present in frame\n");
            break;
        }

        delete frame;
        frame = 0;
    }

    delete frame;
}

void APPInfoOutput::WriteInfo(AppInfoWriter *info_writer, bool include_events)
{
    if (!mHaveInfo)
        return;

    AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);

    info_writer->RegisterCCName("bbc_archive",      "BBCArchive");
    info_writer->RegisterCCName("lto_infax",        "LTOInfax");
    info_writer->RegisterCCName("pse_failures",     "PSEFailures");
    info_writer->RegisterCCName("pse_failure",      "PSEFailure");
    info_writer->RegisterCCName("vtr_error",        "VTRError");
    info_writer->RegisterCCName("vtr_errors",       "VTRErrors");

    info_writer->StartSection("bbc_archive");

    info_writer->WriteStringItem("original_filename", mInfo.original_filename);

    if (text_writer)
        text_writer->PushItemValueIndent(strlen("digibeta_dropouts "));
    info_writer->StartSection("event_counts");
    info_writer->WriteIntegerItem("vtr_errors", mInfo.vtr_error_count);
    info_writer->WriteIntegerItem("pse_failures", mInfo.pse_failure_count);
    info_writer->WriteIntegerItem("digibeta_dropouts", mInfo.digibeta_dropout_count);
    info_writer->WriteIntegerItem("timecode_breaks", mInfo.timecode_break_count);
    info_writer->EndSection();
    if (text_writer)
        text_writer->PopItemValueIndent();

    if (mInfo.source_record) {
        info_writer->StartSection("source_infax");
        write_infax_data(info_writer, mInfo.source_record);
        info_writer->EndSection();
    }
    if (mInfo.lto_record) {
        info_writer->StartSection("lto_infax");
        write_infax_data(info_writer, mInfo.lto_record);
        info_writer->EndSection();
    }

    if (include_events) {
        info_writer->StartSection("events");
        PrintEvents(info_writer);
        info_writer->EndSection();
    }

    info_writer->EndSection();
}

void APPInfoOutput::PrintEvents(AppInfoWriter *info_writer)
{
    Rational frame_rate = mFileReader->GetEditRate();
    size_t i;

    AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);

#define WRITE_POSITION_INFO(event, timecodes)                                               \
    info_writer->WritePositionItem("position", event.position, frame_rate);                 \
    if (timecodes.vitc.hour != INVALID_TIMECODE_HOUR)                                       \
        info_writer->WriteStringItem("vitc", get_archive_tc_string(timecodes.vitc));        \
    if (timecodes.ltc.hour != INVALID_TIMECODE_HOUR)                                        \
        info_writer->WriteStringItem("ltc", get_archive_tc_string(timecodes.ltc));

    if (mInfo.num_digibeta_dropouts > 0) {
        if (text_writer)
            text_writer->PushItemValueIndent(strlen("position "));

        info_writer->StartArrayItem("digibeta_dropouts", mInfo.num_digibeta_dropouts);
        for (i = 0; i < mInfo.num_digibeta_dropouts; i++) {
            info_writer->StartArrayElement("digibeta_dropout", i);
            WRITE_POSITION_INFO(mInfo.digibeta_dropouts[i], mDigiBetaDropoutTimecodes[i])
            info_writer->WriteIntegerItem("strength", mInfo.digibeta_dropouts[i].strength);
            info_writer->EndArrayElement();
        }
        info_writer->EndArrayItem();

        if (text_writer)
            text_writer->PopItemValueIndent();
    }

    if (mInfo.num_pse_failures > 0) {
        if (text_writer)
            text_writer->PushItemValueIndent(strlen("extended_failure "));

        info_writer->StartArrayItem("pse_failures", mInfo.num_pse_failures);
        for (i = 0; i < mInfo.num_pse_failures; i++) {
            info_writer->StartArrayElement("pse_failure", i);
            WRITE_POSITION_INFO(mInfo.pse_failures[i], mPSEFailureTimecodes[i])
            if (mInfo.pse_failures[i].redFlash > 0)
                info_writer->WriteFormatItem("red_flash", "%f", mInfo.pse_failures[i].redFlash / 1000.0);
            if (mInfo.pse_failures[i].spatialPattern > 0)
                info_writer->WriteFormatItem("spatial_pattern", "%f", mInfo.pse_failures[i].spatialPattern / 1000.0);
            if (mInfo.pse_failures[i].luminanceFlash > 0)
                info_writer->WriteFormatItem("luminance_flash", "%f", mInfo.pse_failures[i].luminanceFlash / 1000.0);
            if (mInfo.pse_failures[i].extendedFailure)
                info_writer->WriteBoolItem("extended_failure", mInfo.pse_failures[i].extendedFailure != 0);
            info_writer->EndArrayElement();
        }
        info_writer->EndArrayItem();

        if (text_writer)
            text_writer->PopItemValueIndent();
    }

    if (mInfo.num_timecode_breaks > 0) {
        size_t tc_count = 0;
        for (i = 0; i < mInfo.num_timecode_breaks; i++) {
            if ((mInfo.timecode_breaks[i].timecodeType & TIMECODE_BREAK_VITC))
                tc_count++;
            if ((mInfo.timecode_breaks[i].timecodeType & TIMECODE_BREAK_LTC))
                tc_count++;
        }

        if (text_writer)
            text_writer->PushItemValueIndent(strlen("position "));

        info_writer->StartArrayItem("timecode_breaks", tc_count);
        size_t index = 0;
        for (i = 0; i < mInfo.num_timecode_breaks; i++) {
            if ((mInfo.timecode_breaks[i].timecodeType & TIMECODE_BREAK_VITC)) {
                info_writer->StartArrayElement("timecode_break", index);
                WRITE_POSITION_INFO(mInfo.timecode_breaks[i], mTimecodeBreakTimecodes[i])
                info_writer->WriteStringItem("type", "VITC");
                info_writer->EndArrayElement();
                index++;
            }
            if ((mInfo.timecode_breaks[i].timecodeType & TIMECODE_BREAK_LTC)) {
                info_writer->StartArrayElement("timecode_break", index);
                WRITE_POSITION_INFO(mInfo.timecode_breaks[i], mTimecodeBreakTimecodes[i])
                info_writer->WriteStringItem("type", "LTC");
                info_writer->EndArrayElement();
                index++;
            }
        }
        info_writer->EndArrayItem();

        if (text_writer)
            text_writer->PopItemValueIndent();
    }

    if (mInfo.num_vtr_errors > 0) {
        if (text_writer)
            text_writer->PushItemValueIndent(strlen("audio_status "));

        info_writer->StartArrayItem("vtr_errors", mInfo.num_vtr_errors);
        for (i = 0; i < mInfo.num_vtr_errors; i++) {
            info_writer->StartArrayElement("vtr_error", i);
            WRITE_POSITION_INFO(mInfo.vtr_errors[i], mVTRErrorTimecodes[i])
            info_writer->WriteIntegerItem("error_code", mInfo.vtr_errors[i].errorCode, true);
            info_writer->WriteEnumItem("video_status", VTR_STATUS_EINFO, ( mInfo.vtr_errors[i].errorCode       & 0x07));
            info_writer->WriteEnumItem("audio_status", VTR_STATUS_EINFO, ((mInfo.vtr_errors[i].errorCode >> 4) & 0x07));
            info_writer->EndArrayElement();
        }
        info_writer->EndArrayItem();

        if (text_writer)
            text_writer->PopItemValueIndent();
    }
}

