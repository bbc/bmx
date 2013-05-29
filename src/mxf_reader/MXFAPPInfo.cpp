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

#include <cstdlib>
#include <cstring>

#include <bmx/mxf_reader/MXFAPPInfo.h>
#include <mxf/mxf_app.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



void MXFAPPInfo::RegisterExtensions(DataModel *data_model)
{
    BMX_CHECK(mxf_app_load_extensions(data_model->getCDataModel()));
}

bool MXFAPPInfo::IsAPP(HeaderMetadata *header_metadata)
{
    return mxf_app_is_app_mxf(header_metadata->getCHeaderMetadata());
}

MXFAPPInfo::MXFAPPInfo()
{
    source_record = 0;
    lto_record = 0;
    pse_failures = 0;
    num_pse_failures = 0;
    vtr_errors = 0;
    num_vtr_errors = 0;
    digibeta_dropouts = 0;
    num_digibeta_dropouts = 0;
    timecode_breaks = 0;
    num_timecode_breaks = 0;
}

MXFAPPInfo::~MXFAPPInfo()
{
    ResetAll();
}

bool MXFAPPInfo::CheckIssues(mxfpp::HeaderMetadata *header_metadata)
{
    return mxf_app_check_issues(header_metadata->getCHeaderMetadata());

}

bool MXFAPPInfo::ReadAll(HeaderMetadata *header_metadata)
{
    ResetAll();

    if (!ReadInfo(header_metadata))
        return false;

    ReadPSEFailures(header_metadata);
    ReadVTRErrors(header_metadata);
    ReadDigiBetaDropouts(header_metadata);
    ReadTimecodeBreaks(header_metadata);

    return true;
}

bool MXFAPPInfo::ReadInfo(HeaderMetadata *header_metadata)
{
    ResetInfo();

    ArchiveMXFInfo info;
    if (!mxf_app_get_info(header_metadata->getCHeaderMetadata(), &info))
        return false;

    original_filename       = info.filename;
    if (info.haveSourceInfaxData) {
        source_record = new InfaxData;
        *source_record      = info.sourceInfaxData;
    }
    if (info.haveLTOInfaxData) {
        lto_record = new InfaxData;
        *lto_record         = info.ltoInfaxData;
    }
    vtr_error_count         = info.vtrErrorCount;
    pse_failure_count       = info.pseFailureCount;
    digibeta_dropout_count  = info.digibetaDropoutCount;
    timecode_break_count    = info.timecodeBreakCount;

    return true;
}

bool MXFAPPInfo::ReadPSEFailures(HeaderMetadata *header_metadata)
{
    ResetPSEFailures();

    return mxf_app_get_pse_failures(header_metadata->getCHeaderMetadata(), &pse_failures, &num_pse_failures);
}

bool MXFAPPInfo::ReadVTRErrors(HeaderMetadata *header_metadata)
{
    ResetVTRErrors();

    return mxf_app_get_vtr_errors(header_metadata->getCHeaderMetadata(), &vtr_errors, &num_vtr_errors);
}

bool MXFAPPInfo::ReadDigiBetaDropouts(HeaderMetadata *header_metadata)
{
    ResetDigiBetaDropouts();

    return mxf_app_get_digibeta_dropouts(header_metadata->getCHeaderMetadata(), &digibeta_dropouts, &num_digibeta_dropouts);
}

bool MXFAPPInfo::ReadTimecodeBreaks(HeaderMetadata *header_metadata)
{
    ResetTimecodeBreaks();

    return mxf_app_get_timecode_breaks(header_metadata->getCHeaderMetadata(), &timecode_breaks, &num_timecode_breaks);
}

void MXFAPPInfo::ResetAll()
{
    ResetInfo();
    ResetPSEFailures();
    ResetVTRErrors();
    ResetDigiBetaDropouts();
    ResetTimecodeBreaks();
}

void MXFAPPInfo::ResetInfo()
{
    original_filename.clear();
    delete source_record;
    source_record = 0;
    delete lto_record;
    lto_record = 0;
}

void MXFAPPInfo::ResetPSEFailures()
{
    free(pse_failures);
    pse_failures = 0;
    num_pse_failures = 0;
}

void MXFAPPInfo::ResetVTRErrors()
{
    free(vtr_errors);
    vtr_errors = 0;
    num_vtr_errors = 0;
}

void MXFAPPInfo::ResetDigiBetaDropouts()
{
    free(digibeta_dropouts);
    digibeta_dropouts = 0;
    num_digibeta_dropouts = 0;
}

void MXFAPPInfo::ResetTimecodeBreaks()
{
    free(timecode_breaks);
    timecode_breaks = 0;
    num_timecode_breaks = 0;
}

