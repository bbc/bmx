/*
 * Copyright (C) 2018, British Broadcasting Corporation
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

#include <bmx/mxf_reader/MXFTimedTextTrackReader.h>

#include <string.h>

#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/GenericStreamReader.h>
#include <bmx/BMXException.h>
#include <bmx/Utils.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



MXFTimedTextTrackReader::MXFTimedTextTrackReader(MXFFileReader *file_reader, size_t track_index,
                                                 MXFTrackInfo *track_info, FileDescriptor *file_descriptor,
                                                 SourcePackage *file_source_package)
: MXFFileTrackReader(file_reader, track_index, track_info, file_descriptor, file_source_package)
{
    BMX_ASSERT(track_info->essence_type == TIMED_TEXT);
    mBodySID = 0;
}

MXFTimedTextTrackReader::~MXFTimedTextTrackReader()
{
}

int64_t MXFTimedTextTrackReader::GetOrigin() const
{
    // MXFFileReader would throw a MXF_RESULT_NOT_SUPPORTED if origin != 0
    return 0;
}

int16_t MXFTimedTextTrackReader::GetPrecharge(int64_t position, bool limit_to_available) const
{
    (void)position;
    (void)limit_to_available;
    return 0;
}

int64_t MXFTimedTextTrackReader::GetAvailablePrecharge(int64_t position) const
{
    if (position < 0)
        return 0;
    else
        return - position;
}

int16_t MXFTimedTextTrackReader::GetRollout(int64_t position, bool limit_to_available) const
{
    (void)position;
    (void)limit_to_available;
    return 0;
}

int64_t MXFTimedTextTrackReader::GetAvailableRollout(int64_t position) const
{
    if (position >= GetDuration())
        return 0;
    else
        return GetDuration() - 1 - position;
}

void MXFTimedTextTrackReader::SetBodySID(uint32_t body_sid)
{
    mBodySID = body_sid;
}

TimedTextManifest* MXFTimedTextTrackReader::GetManifest()
{
    return dynamic_cast<MXFDataTrackInfo*>(mTrackInfo)->timed_text_manifest;
}

void MXFTimedTextTrackReader::ReadTimedText(FILE *file_out, unsigned char **data_out, size_t *size_out)
{
    BMX_ASSERT(mBodySID != 0);

    ReadStream(mBodySID, &MXF_EE_K(TimedText), file_out, data_out, size_out, 0);
}

void MXFTimedTextTrackReader::ReadAncillaryResourceById(mxfUUID resource_id, FILE *file_out,
                                                        unsigned char **data_out, size_t *size_out)
{
    uint32_t stream_id = 0;
    vector<TimedTextAncillaryResource> &anc_resources = GetManifest()->GetAncillaryResources();
    size_t i;
    for (i = 0; i < anc_resources.size(); i++) {
        if (anc_resources[i].resource_id == resource_id) {
            stream_id = anc_resources[i].stream_id;
            break;
        }
    }
    if (stream_id == 0) {
        BMX_EXCEPTION(("Unknown timed text ancillary resource id"));
    }

    ReadStream(stream_id, &MXF_EE_K(TimedTextAnc), file_out, data_out, size_out, 0);
}

void MXFTimedTextTrackReader::ReadAncillaryResourceByStreamId(uint32_t stream_id, FILE *file_out,
                                                              unsigned char **data_out, size_t *size_out)
{
    ReadStream(stream_id, &MXF_EE_K(TimedTextAnc), file_out, data_out, size_out, 0);
}

TimedTextMXFResourceProvider* MXFTimedTextTrackReader::CreateResourceProvider()
{
    mxfpp::File *file = 0;
    TimedTextMXFResourceProvider *provider = 0;
    try {
        file = GetFileReader()->GetFileFactory()->OpenRead(GetFileReader()->GetFilename());
        provider = new TimedTextMXFResourceProvider(file);

        vector<pair<int64_t, int64_t> > ranges;
        ReadStream(mBodySID, &MXF_EE_K(TimedText), 0, 0, 0, &ranges);
        provider->AddTimedTextResource(ranges);

        vector<TimedTextAncillaryResource> &anc_resources = GetManifest()->GetAncillaryResources();
        size_t i;
        for (i = 0; i < anc_resources.size(); i++) {
            ranges.clear();
            ReadStream(anc_resources[i].stream_id, &MXF_EE_K(TimedTextAnc), 0, 0, 0, &ranges);
            provider->AddAncillaryResource(anc_resources[i].stream_id, ranges);
        }
    } catch (...) {
      if (provider) {
          delete provider;
      } else {
          delete file;
      }
      throw;
    }

    return provider;
}

void MXFTimedTextTrackReader::ReadStream(uint32_t stream_id, const mxfKey *stream_key,
                                         FILE *file_out,
                                         unsigned char **data_out, size_t *data_out_size,
                                         std::vector<std::pair<int64_t, int64_t> > *ranges_out)
{
    vector<mxfKey> expected_stream_keys;
    expected_stream_keys.push_back(*stream_key);

    GenericStreamReader stream_reader(GetFileReader()->mFile, stream_id, expected_stream_keys);
    if (file_out)
        stream_reader.Read(file_out);
    else if (data_out && data_out_size)
        stream_reader.Read(data_out, data_out_size);
    else if (ranges_out)
        *ranges_out = stream_reader.GetFileOffsetRanges();
}
