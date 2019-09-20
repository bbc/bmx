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

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <bmx/mxf_reader/MXFTimedTextTrackReader.h>

#include <string.h>
#include <errno.h>
#include <limits.h>

#include <bmx/mxf_reader/MXFFileReader.h>
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
    mxfpp::File *mxf_file = GetFileReader()->mFile;
    int64_t original_file_pos = mxf_file->tell();
    try
    {
        uint64_t body_size = 0;
        ByteArray buffer;
        mxfKey key;
        uint8_t llen;
        uint64_t len;
        const vector<Partition*> &partitions = mxf_file->getPartitions();
        size_t i;
        for (i = 0; i < partitions.size(); i++) {
            if (partitions[i]->getBodySID() != stream_id)
                continue;

            if (partitions[i]->getBodyOffset() > body_size) {
                log_warn("Ignoring potential missing stream data; "
                         "partition pack's BodyOffset 0x%" PRIx64 " > expected offset 0x" PRIx64 "\n",
                         partitions[i]->getBodyOffset(), body_size);
                continue;
            } else if (partitions[i]->getBodyOffset() < body_size) {
                log_warn("Ignoring overlapping or repeated stream data; "
                         "partition pack's BodyOffset 0x%" PRIx64 " < expected offset 0x" PRIx64 "\n",
                         partitions[i]->getBodyOffset(), body_size);
                continue;
            }

            mxf_file->seek(partitions[i]->getThisPartition(), SEEK_SET);
            mxf_file->readKL(&key, &llen, &len);
            mxf_file->skip(len);

            bool have_stream_key = false;
            while (!mxf_file->eof())
            {
                mxf_file->readNextNonFillerKL(&key, &llen, &len);

                if (mxf_is_partition_pack(&key)) {
                    break;
                } else if (mxf_is_header_metadata(&key)) {
                    if (partitions[i]->getHeaderByteCount() > mxfKey_extlen + llen + len)
                        mxf_file->skip(partitions[i]->getHeaderByteCount() - (mxfKey_extlen + llen));
                    else
                        mxf_file->skip(len);
                } else if (mxf_is_index_table_segment(&key)) {
                    if (partitions[i]->getIndexByteCount() > mxfKey_extlen + llen + len)
                        mxf_file->skip(partitions[i]->getIndexByteCount() - (mxfKey_extlen + llen));
                    else
                        mxf_file->skip(len);
                } else if (mxf_equals_key_mod_regver(&key, stream_key)) {
                    if (data_out) {
                        if (len > UINT32_MAX || (uint64_t)buffer.GetAllocatedSize() + len > UINT32_MAX)
                            BMX_EXCEPTION(("Stream data size exceeds maximum supported in-memory size"));
                        buffer.Grow((uint32_t)len);
                    } else if (file_out) {
                        buffer.Allocate(8192);
                    }
                    if (data_out || file_out) {
                        uint64_t rem_len = len;
                        while (rem_len > 0) {
                            uint32_t count = 8192;
                            if (count > rem_len)
                                count = (uint32_t)rem_len;
                            uint32_t num_read = mxf_file->read(buffer.GetBytesAvailable(), count);
                            if (num_read != count)
                                BMX_EXCEPTION(("Failed to read text object data from generic stream"));
                            if (file_out) {
                                size_t num_write = fwrite(buffer.GetBytesAvailable(), 1, num_read, file_out);
                                if (num_write != num_read) {
                                    BMX_EXCEPTION(("Failed to write text object to file: %s",
                                                   bmx_strerror(errno).c_str()));
                                }
                            } else if (data_out) {
                                buffer.IncrementSize(num_read);
                            }
                            rem_len -= num_read;
                        }
                    } else {
                        if (len > 0) {
                            ranges_out->push_back(make_pair(mxf_file->tell(), len));
                            mxf_file->skip(len);
                        }
                    }

                    have_stream_key = true;
                    body_size += mxfKey_extlen + llen + len;
                } else {
                    mxf_file->skip(len);
                    if (have_stream_key)
                        body_size += mxfKey_extlen + llen + len;
                }
            }
        }

        if (data_out) {
            if (buffer.GetSize() > 0) {
                *data_out      = buffer.GetBytes();
                *data_out_size = buffer.GetSize();
                buffer.TakeBytes();
            } else {
                *data_out      = 0;
                *data_out_size = 0;
            }
        }

        mxf_file->seek(original_file_pos, SEEK_SET);
    }
    catch (...)
    {
        mxf_file->seek(original_file_pos, SEEK_SET);
        throw;
    }
}
