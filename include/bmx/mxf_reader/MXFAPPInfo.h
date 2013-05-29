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

#ifndef BMX_MXF_APP_INFO_H_
#define BMX_MXF_APP_INFO_H_


#include <libMXF++/MXF.h>
#include <mxf/mxf_app_types.h>


namespace bmx
{


class MXFAPPInfo
{
public:
    static void RegisterExtensions(mxfpp::DataModel *data_model);
    static bool IsAPP(mxfpp::HeaderMetadata *header_metadata);

public:
    MXFAPPInfo();
    ~MXFAPPInfo();

    bool CheckIssues(mxfpp::HeaderMetadata *header_metadata);

    bool ReadAll(mxfpp::HeaderMetadata *header_metadata);

    bool ReadInfo(mxfpp::HeaderMetadata *header_metadata);
    bool ReadPSEFailures(mxfpp::HeaderMetadata *headerMetadata);
    bool ReadVTRErrors(mxfpp::HeaderMetadata *headerMetadata);
    bool ReadDigiBetaDropouts(mxfpp::HeaderMetadata *headerMetadata);
    bool ReadTimecodeBreaks(mxfpp::HeaderMetadata *headerMetadata);

private:
    void ResetAll();
    void ResetInfo();
    void ResetPSEFailures();
    void ResetVTRErrors();
    void ResetDigiBetaDropouts();
    void ResetTimecodeBreaks();

public:
    std::string original_filename;
    InfaxData *source_record;
    InfaxData *lto_record;
    uint32_t vtr_error_count;
    uint32_t pse_failure_count;
    uint32_t digibeta_dropout_count;
    uint32_t timecode_break_count;

    PSEFailure *pse_failures;
    size_t num_pse_failures;
    VTRErrorAtPos *vtr_errors;
    size_t num_vtr_errors;
    DigiBetaDropout *digibeta_dropouts;
    size_t num_digibeta_dropouts;
    TimecodeBreak *timecode_breaks;
    size_t num_timecode_breaks;
};


};



#endif

