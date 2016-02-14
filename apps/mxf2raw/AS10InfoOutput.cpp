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

#include "AS10InfoOutput.h"
#include <bmx/as10/AS10Info.h>
#include <bmx/as10/AS10DMS.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define VERSION_TYPE_VAL(major, minor)  ((((major) & 0xff) << 8) | ((minor) & 0xff))


static void write_core_framework(AppInfoWriter *info_writer, AS10CoreFramework *cf)
{
    info_writer->WriteStringItem("ShimName", cf->GetShimName());
	if (cf->HaveProperty("Type"))
      info_writer->WriteStringItem("Type", cf->GetType());
	if (cf->HaveProperty("MainTitle"))
      info_writer->WriteStringItem("MainTitle", cf->GetMainTitle());
	if (cf->HaveProperty("SubTitle"))
	  info_writer->WriteStringItem("SubTitle", cf->GetSubTitle());
	if (cf->HaveProperty("TitleDescription"))
      info_writer->WriteStringItem("TitleDescription", cf->GetTitleDescription());
	if (cf->HaveProperty("PersonName"))
	  info_writer->WriteStringItem("PersonName", cf->GetPersonName());
	if (cf->HaveProperty("OrganizationName"))
	  info_writer->WriteStringItem("OrganizationName", cf->GetOrganizationName());
	if (cf->HaveProperty("LocationDescription"))
	  info_writer->WriteStringItem("LocationDescription", cf->GetLocationDescription());
	if (cf->HaveProperty("CommonSpanningID"))
	{
		mxfUMID zeroId;
		memset(&zeroId, 0, sizeof(mxfUMID));
		mxfUMID csID = cf->GetCommonSpanningID();
		if (memcmp(&csID, &zeroId, sizeof(mxfUMID)) == 0)
			info_writer->WriteStringItem("CommonSpanningID", "urn:smpte:umid:0");
		else
			info_writer->WriteUMIDItem("CommonSpanningID", cf->GetCommonSpanningID());
	}
	if (cf->HaveProperty("SpanningNumber"))
	  info_writer->WriteIntegerItem("SpanningNumber", cf->GetSpanningNumber());
	if (cf->HaveProperty("CumulativeDuration"))
      info_writer->WriteIntegerItem("CumulativeDuration", cf->GetCumulativeDuration());
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

    Rational frame_rate = file_reader->GetEditRate();
    Timecode start_timecode = file_reader->GetMaterialTimecode(0);
    BMX_CHECK(file_reader->GetFixedLeadFillerOffset() == 0);

    info_writer->StartSection("as10");

    if (info.core) 
	{
        info_writer->StartSection("core");
        write_core_framework(info_writer, info.core);
        info_writer->EndSection();
    }
 
    info_writer->EndSection();
}

