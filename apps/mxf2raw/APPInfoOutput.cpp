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
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



static char* get_date_string(mxfTimestamp timestamp)
{
    static char buf[64];
    bmx_snprintf(buf, sizeof(buf), "%d-%02u-%02u", timestamp.year, timestamp.month, timestamp.day);
    return buf;
}

static char* get_timecode_string(ArchiveTimecode &timecode)
{
    static char buf[64];
    if (timecode.hour == INVALID_TIMECODE_HOUR) {
        bmx_snprintf(buf, sizeof(buf), "<unknown>");
    } else {
        bmx_snprintf(buf, sizeof(buf), "%02u:%02u:%02u%c%02u", timecode.hour, timecode.min, timecode.sec,
                     (timecode.dropFrame ? ';' : ':'), timecode.frame);
    }
    return buf;
}

static void print_infax_data(const InfaxData *data)
{
    Rational sec_rate = {1, 1};

    printf("      Format            : %s\n", data->format);
    printf("      ProgrammeTitle    : %s\n", data->progTitle);
    printf("      EpisodeTitle      : %s\n", data->epTitle);
    printf("      TransmissionDate  : %s\n", get_date_string(data->txDate));
    printf("      MagazinePrefix    : %s\n", data->magPrefix);
    printf("      ProgrammeNumber   : %s\n", data->progNo);
    printf("      ProductionCode    : %s\n", data->prodCode);
    printf("      SpoolStatus       : %s\n", data->spoolStatus);
    printf("      StockDate         : %s\n", get_date_string(data->stockDate));
    printf("      SpoolDescriptor   : %s\n", data->spoolDesc);
    printf("      Memo              : %s\n", data->memo);
    printf("      Duration          : %s\n", get_duration_string(data->duration, sec_rate).c_str());
    printf("      SpoolNumber       : %s\n", data->spoolNo);
    printf("      AccessionNumber   : %s\n", data->accNo);
    printf("      CatalogueDetail   : %s\n", data->catDetail);
    printf("      ItemNumber        : %u\n", data->itemNo);
}



APPInfoOutput::APPInfoOutput()
{
    mFileReader = 0;
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

void APPInfoOutput::ExtractInfo(int event_mask)
{
    if (!mInfo.ReadInfo(mFileReader->GetHeaderMetadata())) {
        log_warn("No APP info present in file\n");
        return;
    }

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
            log_warn("Failed to read frame from track %zu at position %"PRId64"\n",
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

void APPInfoOutput::PrintInfo()
{
    printf("APP Information:\n");

    printf("  Original filename  : %s\n", mInfo.original_filename.c_str());
    printf("  Event counts       :\n");
    printf("      VTR errors         : %u\n", mInfo.vtr_error_count);
    printf("      PSE failures       : %u\n", mInfo.pse_failure_count);
    printf("      Digibeta dropouts  : %u\n", mInfo.digibeta_dropout_count);
    printf("      Timecode breaks    : %u\n", mInfo.timecode_break_count);
    printf("  Source Infax data  :\n");
    if (mInfo.source_record)
        print_infax_data(mInfo.source_record);
    printf("  LTO Infax data     :\n");
    if (mInfo.lto_record)
        print_infax_data(mInfo.lto_record);

    printf("\n");
}

void APPInfoOutput::PrintEvents()
{
    Rational frame_rate = mFileReader->GetEditRate();
    size_t i;

    if (mInfo.num_digibeta_dropouts > 0) {
        printf("APP Digibeta dropouts:\n");
        printf("  Count     : %"PRIszt"\n", mInfo.num_digibeta_dropouts);
        printf("  Dropouts  :\n");
        printf("    %10s:%14s%14s%14s%10s\n", "num", "frame", "vitc", "ltc", "strength");
        for (i = 0; i < mInfo.num_digibeta_dropouts; i++) {
            printf("    %10"PRIszt":%14s%14s%14s%10d\n",
                   i,
                   get_duration_string(mInfo.digibeta_dropouts[i].position, frame_rate).c_str(),
                   get_timecode_string(mDigiBetaDropoutTimecodes[i].vitc),
                   get_timecode_string(mDigiBetaDropoutTimecodes[i].ltc),
                   mInfo.digibeta_dropouts[i].strength);
        }
    }
    if (mInfo.num_pse_failures > 0) {
        printf("APP PSE failures:\n");
        printf("  Count     : %"PRIszt"\n", mInfo.num_pse_failures);
        printf("  Failures  :\n");
        printf("    %10s:%14s%14s%14s%9s%10s%10s%6s\n",
               "num", "frame", "vitc", "ltc", "red", "spatial", "lumin", "ext");
        for (i = 0; i < mInfo.num_pse_failures; i++) {
            printf("    %10"PRIszt":%14s%14s%14s",
                   i,
                   get_duration_string(mInfo.pse_failures[i].position, frame_rate).c_str(),
                   get_timecode_string(mPSEFailureTimecodes[i].vitc),
                   get_timecode_string(mPSEFailureTimecodes[i].ltc));
            if (mInfo.pse_failures[i].redFlash > 0)
                printf("%9.1f", mInfo.pse_failures[i].redFlash / 1000.0);
            else
                printf("%9.1f", 0.0);
            if (mInfo.pse_failures[i].spatialPattern > 0)
                printf("%10.1f", mInfo.pse_failures[i].spatialPattern / 1000.0);
            else
                printf("%10.1f", 0.0);
            if (mInfo.pse_failures[i].luminanceFlash > 0)
                printf("%10.1f", mInfo.pse_failures[i].luminanceFlash / 1000.0);
            else
                printf("%10.1f", 0.0);
            if (mInfo.pse_failures[i].extendedFailure == 0)
                printf("%6s\n", "F");
            else
                printf("%6s\n", "T");
        }
    }
    if (mInfo.num_timecode_breaks > 0) {
        printf("APP timecode breaks:\n");
        printf("  Count     : %"PRIszt"\n", mInfo.num_timecode_breaks);
        printf("  Breaks    :\n");
        printf("    %10s:%14s%14s%14s%8s\n", "num", "frame", "vitc", "ltc", "type");
        for (i = 0; i < mInfo.num_timecode_breaks; i++) {
            printf("    %10"PRIszt":%14s%14s%14s",
                   i,
                   get_duration_string(mInfo.timecode_breaks[i].position, frame_rate).c_str(),
                   get_timecode_string(mTimecodeBreakTimecodes[i].vitc),
                   get_timecode_string(mTimecodeBreakTimecodes[i].ltc));
            switch (mInfo.timecode_breaks[i].timecodeType)
            {
                case TIMECODE_BREAK_VITC:
                    printf("    VITC\n");
                    break;
                case TIMECODE_BREAK_LTC:
                    printf("     LTC\n");
                    break;
                default:
                    printf("  0x%04x\n", mInfo.timecode_breaks[i].timecodeType);
                    break;
            }
        }
    }
    if (mInfo.num_vtr_errors > 0) {
        printf("APP VTR errors:\n");
        printf("  Count     : %"PRIszt"\n", mInfo.num_vtr_errors);
        printf("  Errors    :\n");
        printf("    %10s:%14s%14s%14s%7s   %s\n", "num", "frame", "vitc", "ltc", "code", "description");
        for (i = 0; i < mInfo.num_vtr_errors; i++) {
            printf("    %10"PRIszt":%14s%14s%14s",
                   i,
                   get_duration_string(mInfo.vtr_errors[i].position, frame_rate).c_str(),
                   get_timecode_string(mVTRErrorTimecodes[i].vitc),
                   get_timecode_string(mVTRErrorTimecodes[i].ltc));
            printf("%5s%02x", "0x", mInfo.vtr_errors[i].errorCode);
            if (mInfo.vtr_errors[i].errorCode == 0x00) {
                printf("   No error\n");
            } else {
                if ((mInfo.vtr_errors[i].errorCode & 0x07) == 0x01)
                    printf("   Video almost good, ");
                else if ((mInfo.vtr_errors[i].errorCode & 0x07) == 0x02)
                    printf("   Video state cannot be determined, ");
                else if ((mInfo.vtr_errors[i].errorCode & 0x07) == 0x03)
                    printf("   Video state unclear, ");
                else if ((mInfo.vtr_errors[i].errorCode & 0x07) == 0x04)
                    printf("   Video no good, ");
                else
                    printf("   Video good, ");

                if ((mInfo.vtr_errors[i].errorCode & 0x70) == 0x10)
                    printf("Audio almost good\n");
                else if ((mInfo.vtr_errors[i].errorCode & 0x70) == 0x20)
                    printf("Audio state cannot be determined\n");
                else if ((mInfo.vtr_errors[i].errorCode & 0x70) == 0x30)
                    printf("Audio state unclear\n");
                else if ((mInfo.vtr_errors[i].errorCode & 0x70) == 0x40)
                    printf("Audio no good\n");
                else
                    printf("Audio good\n");
            }
        }
    }
}

