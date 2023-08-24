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

#ifndef MXF_APP_H_
#define MXF_APP_H_


#include <mxf/mxf_app_types.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct
{
    mxfTimestamp creationDate;
    char filename[256];
    InfaxData sourceInfaxData;
    int haveSourceInfaxData;
    InfaxData ltoInfaxData;
    int haveLTOInfaxData;
    uint32_t vtrErrorCount;
    uint32_t pseFailureCount;
    uint32_t digibetaDropoutCount;
    uint32_t timecodeBreakCount;
} ArchiveMXFInfo;


#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;

#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

#include <mxf/mxf_app_extensions_data_model.h>


int mxf_app_load_extensions(MXFDataModel *dataModel);

int mxf_app_is_app_mxf(MXFHeaderMetadata *headerMetadata);
int mxf_app_check_issues(MXFHeaderMetadata *headerMetadata);
int mxf_app_get_info(MXFHeaderMetadata *headerMetadata, ArchiveMXFInfo *info);
int mxf_app_get_pse_failures(MXFHeaderMetadata *headerMetadata, PSEFailure **failures, size_t *numFailures);
int mxf_app_get_vtr_errors(MXFHeaderMetadata *headerMetadata, VTRErrorAtPos **errors, size_t *numErrors);
int mxf_app_get_digibeta_dropouts(MXFHeaderMetadata *headerMetadata, DigiBetaDropout **digibetaDropouts,
                                  size_t *numDigiBetaDropouts);
int mxf_app_get_timecode_breaks(MXFHeaderMetadata *headerMetadata, TimecodeBreak **timecodeBreaks,
                                size_t *numTimecodeBreaks);


/* returns 1 if footer headermetadata was read, return 2 if none is present (*headerMetadata is NULL) */
int mxf_app_read_footer_metadata(const char *filename, MXFDataModel *dataModel, MXFHeaderMetadata **headerMetadata);

int mxf_app_is_metadata_only(const char *filename);



#ifdef __cplusplus
}
#endif


#endif

