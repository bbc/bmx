/*
 * Copyright (C) 2016, British Broadcasting Corporation
 * All Rights Reserved.
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

#include "AS10InfoOutput.h"
#include <bmx/as10/AS10Info.h>
#include <bmx/as10/AS10DMS.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static void write_core_framework(AppInfoWriter *info_writer, AS10CoreFramework *cf, Rational frame_rate)
{
  info_writer->WriteStringItem("shim_name", cf->GetShimName());
  if (cf->HaveType())
      info_writer->WriteStringItem("type", cf->GetType());
  if (cf->HaveMainTitle())
      info_writer->WriteStringItem("main_title", cf->GetMainTitle());
  if (cf->HaveSubTitle())
      info_writer->WriteStringItem("sub_title", cf->GetSubTitle());
  if (cf->HaveTitleDescription())
      info_writer->WriteStringItem("title_description", cf->GetTitleDescription());
  if (cf->HavePersonName())
      info_writer->WriteStringItem("person_name", cf->GetPersonName());
  if (cf->HaveOrganizationName())
      info_writer->WriteStringItem("organization_name", cf->GetOrganizationName());
  if (cf->HaveLocationDescription())
      info_writer->WriteStringItem("location_description", cf->GetLocationDescription());
  if (cf->HaveCommonSpanningID())
      info_writer->WriteUMIDItem("common_spanning_id", cf->GetCommonSpanningID());
  if (cf->HaveSpanningNumber())
      info_writer->WriteIntegerItem("spanning_number", cf->GetSpanningNumber());
  if (cf->HaveCumulativeDuration())
      info_writer->WriteDurationItem("cumulative_duration", cf->GetCumulativeDuration(), frame_rate);
}


void bmx::as10_register_extensions(MXFFileReader *file_reader)
{
    AS10Info::RegisterExtensions(file_reader->GetHeaderMetadata());
}

void bmx::as10_write_info(AppInfoWriter *info_writer, MXFFileReader *file_reader)
{
    info_writer->RegisterCCName("as10", "AS10");

    AS10Info info;
    if (!info.Read(file_reader->GetHeaderMetadata())) {
        log_warn("No AS-10 info present in file\n");
        return;
    }

    info_writer->StartSection("as10");

    if (info.core) {
        info_writer->StartSection("core");
        write_core_framework(info_writer, info.core, file_reader->GetEditRate());
        info_writer->EndSection();
    }

    info_writer->EndSection();
}

