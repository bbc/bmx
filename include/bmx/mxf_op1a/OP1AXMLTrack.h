/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#ifndef BMX_OP1A_XML_TRACK_H_
#define BMX_OP1A_XML_TRACK_H_

#include <libMXF++/MXF.h>

#include <bmx/writer_helper/XMLWriterHelper.h>


namespace bmx
{


class OP1AFile;

class OP1AXMLTrack
{
public:
    OP1AXMLTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint32_t stream_id);
    ~OP1AXMLTrack();

    void SetSource(const std::string &filename);
    void SetSource(const unsigned char *data, uint32_t size, bool copy);

    void SetSchemeId(UL id);                        // id may be a half-swapped UUID
    void SetSchemeId(const std::string &name);      // creates a version 3 UUID using the name
    void SetLanguageCode(const std::string &code);
    void SetNamespace(const std::string &nmspace);
    void SetTextEncoding(TextEncoding encoding);
    void SetByteOrder(ByteOrder byte_order);

public:
    void AddHeaderMetadata(mxfpp::HeaderMetadata *header_metadata, mxfpp::MaterialPackage *material_package);

    bool RequireStreamPartition() const { return mRequireStreamPartition; }
    uint32_t GetStreamId() const        { return mStreamId; }
    void WriteStreamXMLData(mxfpp::File *mxf_file);

private:
    OP1AFile *mOP1AFile;
    uint32_t mTrackIndex;
    uint32_t mTrackId;
    uint32_t mStreamId;
    BMX_OPT_PROP_DECL(UL, mSchemeId);
    BMX_OPT_PROP_DECL(std::string, mLanguageCode);
    BMX_OPT_PROP_DECL(std::string, mNamespace);
    BMX_OPT_PROP_DECL(TextEncoding, mTextEncoding);
    BMX_OPT_PROP_DECL(ByteOrder, mByteOrder);
    XMLWriterHelper mXMLWriterHelper;
    std::string mFilename;
    const unsigned char *mData;
    unsigned char *mDataBuffer;
    int64_t mDataSize;
    bool mRequireStreamPartition;
};


};


#endif
