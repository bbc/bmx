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
    sprintf(buf, "%d-%02u-%02u", timestamp.year, timestamp.month, timestamp.day);
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



void bmx::app_register_extensions(MXFFileReader *file_reader)
{
    MXFAPPInfo::RegisterExtensions(file_reader->GetDataModel());
}

void bmx::app_print_info(MXFFileReader *file_reader)
{
    MXFAPPInfo info;
    if (!info.ReadInfo(file_reader->GetHeaderMetadata())) {
        log_warn("No APP info present in file\n");
        return;
    }


    printf("APP Information:\n");

    printf("  Original filename  : %s\n", info.original_filename.c_str());
    printf("  Event counts       :\n");
    printf("      VTR errors         : %u\n", info.vtr_error_count);
    printf("      PSE failures       : %u\n", info.pse_failure_count);
    printf("      Digibeta dropouts  : %u\n", info.digibeta_dropout_count);
    printf("      Timecode breaks    : %u\n", info.timecode_break_count);
    printf("  Source Infax data  :\n");
    if (info.source_record)
        print_infax_data(info.source_record);
    printf("  LTO Infax data     :\n");
    if (info.lto_record)
        print_infax_data(info.lto_record);

    printf("\n");
}

void bmx::app_print_events(MXFFileReader *file_reader, int event_mask_in)
{
    MXFAPPInfo info;
    int event_mask = event_mask_in;
    size_t i;
    Rational frame_rate = file_reader->GetSampleRate();

    if ((event_mask & DIGIBETA_DROPOUT_MASK) && !info.ReadDigiBetaDropouts(file_reader->GetHeaderMetadata())) {
        log_warn("Failed to read APP Digibeta dropout events\n");
        event_mask &= ~DIGIBETA_DROPOUT_MASK;
    }
    if ((event_mask & PSE_FAILURE_MASK) && !info.ReadPSEFailures(file_reader->GetHeaderMetadata())) {
        log_warn("Failed to read APP PSE failure events\n");
        event_mask &= ~PSE_FAILURE_MASK;
    }
    if ((event_mask & TIMECODE_BREAK_MASK) && !info.ReadTimecodeBreaks(file_reader->GetHeaderMetadata())) {
        log_warn("Failed to read APP timecode break events\n");
        event_mask &= ~TIMECODE_BREAK_MASK;
    }
    if ((event_mask & VTR_ERROR_MASK) && !info.ReadVTRErrors(file_reader->GetHeaderMetadata())) {
        log_warn("Failed to read APP VTR error events\n");
        event_mask &= ~VTR_ERROR_MASK;
    }


    if (event_mask & DIGIBETA_DROPOUT_MASK) {
        printf("APP Digibeta dropouts:\n");
        printf("  Count     : %"PRIszt"\n", info.num_digibeta_dropouts);
        printf("  Dropouts  :\n");
        printf("    %10s:%14s %10s\n", "num", "frame", "strength");
        for (i = 0; i < info.num_digibeta_dropouts; i++) {
            printf("    %10"PRIszt":%14s %10d\n",
                   i, get_duration_string(info.digibeta_dropouts[i].position, frame_rate).c_str(),
                   info.digibeta_dropouts[i].strength);
        }
    }
    if (event_mask & PSE_FAILURE_MASK) {
        printf("APP PSE failures:\n");
        printf("  Count     : %"PRIszt"\n", info.num_pse_failures);
        printf("  Failures  :\n");
        printf("    %10s:%14s%10s%10s%10s%10s\n",
               "num", "frame", "red", "spatial", "lumin", "ext");
        for (i = 0; i < info.num_pse_failures; i++) {
            printf("    %10"PRIszt":%14s",
                   i, get_duration_string(info.pse_failures[i].position, frame_rate).c_str());
            if (info.pse_failures[i].redFlash > 0)
                printf("%10.1f", info.pse_failures[i].redFlash / 1000.0);
            else
                printf("%10.1f", 0.0);
            if (info.pse_failures[i].spatialPattern > 0)
                printf("%10.1f", info.pse_failures[i].spatialPattern / 1000.0);
            else
                printf("%10.1f", 0.0);
            if (info.pse_failures[i].luminanceFlash > 0)
                printf("%10.1f", info.pse_failures[i].luminanceFlash / 1000.0);
            else
                printf("%10.1f", 0.0);
            if (info.pse_failures[i].extendedFailure == 0)
                printf("%10s\n", "F");
            else
                printf("%10s\n", "T");
        }
    }
    if (event_mask & TIMECODE_BREAK_MASK) {
        printf("APP timecode breaks:\n");
        printf("  Count     : %"PRIszt"\n", info.num_timecode_breaks);
        printf("  Breaks    :\n");
        printf("    %10s:%14s %10s\n",
               "num", "frame", "type");
        for (i = 0; i < info.num_timecode_breaks; i++) {
            printf("    %10"PRIszt":%14s",
                   i, get_duration_string(info.timecode_breaks[i].position, frame_rate).c_str());
            switch (info.timecode_breaks[i].timecodeType)
            {
                case TIMECODE_BREAK_VITC:
                    printf("       VITC\n");
                    break;
                case TIMECODE_BREAK_LTC:
                    printf("        LTC\n");
                    break;
                default:
                    printf("     0x%04x\n", info.timecode_breaks[i].timecodeType);
                    break;
            }
        }
    }
    if (event_mask & VTR_ERROR_MASK) {
        printf("APP VTR errors:\n");
        printf("  Count     : %"PRIszt"\n", info.num_vtr_errors);
        printf("  Errors    :\n");
        printf("    %10s:%14s %10s    %s\n", "num", "frame", "code", "description");
        for (i = 0; i < info.num_vtr_errors; i++) {
            printf("    %10"PRIszt":%14s",
                   i, get_duration_string(info.vtr_errors[i].position, frame_rate).c_str());
            printf(" %8s%02x", "0x", info.vtr_errors[i].errorCode);
            if (info.vtr_errors[i].errorCode == 0x00) {
                printf("    No error\n");
            } else {
                if ((info.vtr_errors[i].errorCode & 0x07) == 0x01)
                    printf("    Video almost good, ");
                else if ((info.vtr_errors[i].errorCode & 0x07) == 0x02)
                    printf("    Video state cannot be determined, ");
                else if ((info.vtr_errors[i].errorCode & 0x07) == 0x03)
                    printf("    Video state unclear, ");
                else if ((info.vtr_errors[i].errorCode & 0x07) == 0x04)
                    printf("    Video no good, ");
                else
                    printf("    Video good, ");

                if ((info.vtr_errors[i].errorCode & 0x70) == 0x10)
                    printf("Audio almost good\n");
                else if ((info.vtr_errors[i].errorCode & 0x70) == 0x20)
                    printf("Audio state cannot be determined\n");
                else if ((info.vtr_errors[i].errorCode & 0x70) == 0x30)
                    printf("Audio state unclear\n");
                else if ((info.vtr_errors[i].errorCode & 0x70) == 0x40)
                    printf("Audio no good\n");
                else
                    printf("Audio good\n");
            }
        }
    }
}

