/*
 * Copyright (C) 2016, British Broadcasting Corporation
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

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <bmx/d10_mxf/D10XMLTrack.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;

static const char *DEFAULT_SCHEME_ID_NAME = "Default_BMX_Payload_Scheme_ID";
static const char *XML_MIME_TYPE          = "application/xml";


D10XMLTrack::D10XMLTrack(D10File *file, uint32_t track_index, uint32_t track_id, uint32_t stream_id)
{
    mD10File = file;
    mTrackIndex = track_index;
    mTrackId = track_id;
    mStreamId = stream_id;
    BMX_OPT_PROP_DEFAULT(mSchemeId, g_Null_UL);
    BMX_OPT_PROP_DEFAULT(mLanguageCode, "");
    BMX_OPT_PROP_DEFAULT(mNamespace, "");
    BMX_OPT_PROP_DEFAULT(mTextEncoding, UNKNOWN_TEXT_ENCODING);
    BMX_OPT_PROP_DEFAULT(mByteOrder, UNKNOWN_BYTE_ORDER);
    mData = 0;
    mDataBuffer = 0;
    mDataSize = 0;
    mRequireStreamPartition = false;

    SetSchemeId(DEFAULT_SCHEME_ID_NAME);
    BMX_OPT_PROP_MARK(mSchemeId, false);
}

D10XMLTrack::~D10XMLTrack()
{
    delete [] mDataBuffer;
}

void D10XMLTrack::SetSource(const string &filename)
{
    mFilename = filename;
    mXMLWriterHelper.ExtractInfo(filename);

    if (mXMLWriterHelper.GetSize() < UINT16_MAX) {
        FILE *file = 0;
        try
        {
            file = fopen(mFilename.c_str(), "rb");
            if (!file)
                BMX_EXCEPTION(("Failed to open XML file '%s': %s", mFilename.c_str(), bmx_strerror(errno).c_str()));

            mDataBuffer = new unsigned char[(size_t)mXMLWriterHelper.GetSize()];
            size_t num_read = fread(mDataBuffer, 1, (size_t)mXMLWriterHelper.GetSize(), file);
            if (num_read < (size_t)mXMLWriterHelper.GetSize() && ferror(file)) {
                BMX_EXCEPTION(("Failed to read %" PRId64 " bytes from '%s': %s",
                               mXMLWriterHelper.GetSize(), mFilename.c_str(), bmx_strerror(errno).c_str()));
            }

            mData     = mDataBuffer;
            mDataSize = (uint32_t)num_read;

            fclose(file);
        }
        catch (...)
        {
            if (file)
                fclose(file);
            throw;
        }
    } else {
        mData     = 0;
        mDataSize = mXMLWriterHelper.GetSize();
    }
}

void D10XMLTrack::SetSource(const unsigned char *data, uint32_t size, bool copy)
{
    mXMLWriterHelper.ExtractInfo(data, size);

    if (copy) {
        mDataBuffer = new unsigned char[size];
        memcpy(mDataBuffer, data, size);
        mData = mDataBuffer;
    } else {
        mData = data;
    }
    mDataSize = size;
}

void D10XMLTrack::SetSchemeId(UL id)
{
    BMX_OPT_PROP_SET(mSchemeId, id);
}

void D10XMLTrack::SetSchemeId(const string &name)
{
    UUID uuid = create_uuid_from_name(name);
    mxf_swap_uid(&mSchemeId, (const mxfUID*)&uuid);
    BMX_OPT_PROP_MARK(mSchemeId, true);
}

void D10XMLTrack::SetLanguageCode(const string &code)
{
    BMX_OPT_PROP_SET(mLanguageCode, code);
}

void D10XMLTrack::SetNamespace(const string &nmspace)
{
    BMX_OPT_PROP_SET(mNamespace, nmspace);
}

void D10XMLTrack::SetTextEncoding(TextEncoding encoding)
{
    BMX_OPT_PROP_SET(mTextEncoding, encoding);
}

void D10XMLTrack::SetByteOrder(ByteOrder byte_order)
{
    BMX_OPT_PROP_SET(mByteOrder, byte_order);
}

void D10XMLTrack::AddHeaderMetadata(HeaderMetadata *header_metadata, MaterialPackage *material_package)
{
    if (!BMX_OPT_PROP_IS_SET(mLanguageCode))
        mLanguageCode = mXMLWriterHelper.GetLanguageCode();
    if (!BMX_OPT_PROP_IS_SET(mNamespace))
        mNamespace = mXMLWriterHelper.GetNamespace();
    if (!BMX_OPT_PROP_IS_SET(mTextEncoding))
        mTextEncoding = mXMLWriterHelper.GetTextEncoding();
    if (mTextEncoding == UTF8)
        mByteOrder = BMX_BYTE_ORIENTED;
    else if (!BMX_OPT_PROP_IS_SET(mByteOrder))
        mByteOrder = mXMLWriterHelper.GetByteOrder();

    if (mTextEncoding == UTF16)
        mDataSize = (mDataSize >> 1) << 1;

    // Preface - ContentStorage - Package - DM Static Track
    StaticTrack *dm_track = new StaticTrack(header_metadata);
    material_package->appendTracks(dm_track);
    dm_track->setTrackID(mTrackId);
    dm_track->setTrackNumber(0);

    // Preface - ContentStorage - Package - DM Static Track - Sequence
    Sequence *dm_sequence = new Sequence(header_metadata);
    dm_track->setSequence(dm_sequence);
    dm_sequence->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));

    // Preface - ContentStorage - Package - DM Static Track - Sequence - DMSegment
    DMSegment *dm_segment = new DMSegment(header_metadata);
    dm_sequence->appendStructuralComponents(dm_segment);
    dm_segment->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));

    // Preface - ContentStorage - Package - DM Static Track - Sequence - DMSegment - TextBasedDMFramework
    TextBasedDMFramework *dm_framework = new TextBasedDMFramework(header_metadata);
    dm_segment->setDMFramework(dm_framework);

    // Preface - ContentStorage - Package - DM Static Track - Sequence - DMSegment - TextBasedDMFramework - TextBasedObject
    TextBasedObject *xml_obj;
    if (mDataSize < UINT16_MAX && mXMLWriterHelper.GetTextEncoding() == UTF8) {
        UTF8TextBasedSet *utf8_xml = new UTF8TextBasedSet(header_metadata);
        xml_obj = utf8_xml;
    } else if (mDataSize < UINT16_MAX && mXMLWriterHelper.GetTextEncoding() == UTF16) {
        UTF16TextBasedSet *utf16_xml = new UTF16TextBasedSet(header_metadata);
        xml_obj = utf16_xml;
    } else {
        GenericStreamTextBasedSet *stream_xml = new GenericStreamTextBasedSet(header_metadata);
        xml_obj = stream_xml;
    }
    dm_framework->setTextBasedObject(xml_obj);
    xml_obj->setTextBasedMetadataPayloadSchemaID(mSchemeId);
    xml_obj->setTextMIMEMediaType(XML_MIME_TYPE);
    xml_obj->setRFC5646TextLanguageCode(mLanguageCode);
    if (!mNamespace.empty())
        xml_obj->setTextDataDescription(mNamespace);

    if (dynamic_cast<UTF8TextBasedSet*>(xml_obj)) {
        UTF8TextBasedSet *utf8_xml = dynamic_cast<UTF8TextBasedSet*>(xml_obj);
        mxfpp::ByteArray utf8_bytes;
        utf8_bytes.data   = (uint8_t*)mData;
        utf8_bytes.length = (uint16_t)mDataSize;
        utf8_xml->setUTF8TextData(utf8_bytes);
        mRequireStreamPartition = false;
    } else if (dynamic_cast<UTF16TextBasedSet*>(xml_obj)) {
        UTF16TextBasedSet *utf16_xml = dynamic_cast<UTF16TextBasedSet*>(xml_obj);
        mxfpp::ByteArray utf16_bytes;
        utf16_bytes.data   = (uint8_t*)mData;
        utf16_bytes.length = (uint16_t)mDataSize;
        utf16_xml->setUTF16TextData(utf16_bytes);
        mRequireStreamPartition = false;
    } else {
        GenericStreamTextBasedSet *stream_xml = dynamic_cast<GenericStreamTextBasedSet*>(xml_obj);
        stream_xml->setGenericStreamSID(mStreamId);
        mRequireStreamPartition = true;
    }
}

void D10XMLTrack::WriteStreamXMLData(File *mxf_file)
{
    uint8_t llen = mxf_get_llen(mxf_file->getCFile(), mDataSize);
    if (llen < 4)
        llen = 4;

    switch (mXMLWriterHelper.GetByteOrder())
    {
        case UNKNOWN_BYTE_ORDER:
            mxf_file->writeFixedKL(&MXF_EE_K(RP2057_ENDIAN_UNK), llen, mDataSize);
            break;
        case BMX_BYTE_ORIENTED:
            mxf_file->writeFixedKL(&MXF_EE_K(RP2057_BYTES), llen, mDataSize);
            break;
        case BMX_BIG_ENDIAN:
            mxf_file->writeFixedKL(&MXF_EE_K(RP2057_BE), llen, mDataSize);
            break;
        case BMX_LITTLE_ENDIAN:
            mxf_file->writeFixedKL(&MXF_EE_K(RP2057_LE), llen, mDataSize);
            break;
    }

    if (mData) {
        mxf_file->write(mData, (uint32_t)mDataSize);
    } else {
        FILE *file = 0;
        try
        {
            unsigned char buffer[8192];

            file = fopen(mFilename.c_str(), "rb");
            if (!file)
                BMX_EXCEPTION(("Failed to open XML file '%s': %s", mFilename.c_str(), bmx_strerror(errno).c_str()));

            int64_t rem_size = mDataSize;
            while (rem_size > 0) {
                size_t read_count = sizeof(buffer);
                if ((int64_t)read_count > rem_size)
                    read_count = (size_t)rem_size;
                size_t num_read = fread(buffer, 1, read_count, file);
                if (num_read < read_count) {
                    BMX_EXCEPTION(("Failed to read %" PRIszt " bytes from '%s': %s",
                                   read_count, mFilename.c_str(), bmx_strerror(errno).c_str()));
                }
                mxf_file->write(buffer, (uint32_t)num_read);
                rem_size -= num_read;
            }
            fclose(file);
        }
        catch (...)
        {
            if (file)
                fclose(file);
            throw;
        }
    }
}
