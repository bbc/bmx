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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <string.h>
#include <errno.h>
#include <limits.h>

#include <libMXF++/MXF.h>

#include <bmx/mxf_reader/MXFTextObject.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


MXFTextObject::MXFTextObject(MXFFileReader *file_reader, TextBasedObject *text_object,
                             mxfUMID package_uid, uint32_t track_id, uint16_t component_index)
{
    mFileReader = file_reader;
    mTextObject = text_object;
    mPackageUID = package_uid;
    mTrackId = track_id;
    mComponentIndex = component_index;
    mIsXML = CheckIsXML();
    mGSByteOrder = UNKNOWN_BYTE_ORDER;
}

MXFTextObject::~MXFTextObject()
{
}

void MXFTextObject::Read(unsigned char **data, size_t *size)
{
    UTF8TextBasedSet *utf8_text        = dynamic_cast<UTF8TextBasedSet*>(mTextObject);
    UTF16TextBasedSet *utf16_text      = dynamic_cast<UTF16TextBasedSet*>(mTextObject);
    GenericStreamTextBasedSet *gs_text = dynamic_cast<GenericStreamTextBasedSet*>(mTextObject);

    if (utf8_text || utf16_text) {
        mxfpp::ByteArray byte_array;
        if (utf8_text)
            byte_array = utf8_text->getUTF8TextData();
        else
            byte_array = utf16_text->getUTF16TextData();
        if (byte_array.length > 0) {
            *data = new unsigned char[byte_array.length];
            memcpy(*data, byte_array.data, byte_array.length);
            *size = byte_array.length;
        } else {
            *data = 0;
            *size = 0;
        }
    } else if (gs_text) {
        ReadGenericStream(0, data, size);
    } else {
        log_warn("Can't read unknown text object type\n");
        *data = 0;
        *size = 0;
    }
}

void MXFTextObject::Read(FILE *file)
{
    UTF8TextBasedSet *utf8_text        = dynamic_cast<UTF8TextBasedSet*>(mTextObject);
    UTF16TextBasedSet *utf16_text      = dynamic_cast<UTF16TextBasedSet*>(mTextObject);
    GenericStreamTextBasedSet *gs_text = dynamic_cast<GenericStreamTextBasedSet*>(mTextObject);

    if (utf8_text || utf16_text) {
        mxfpp::ByteArray byte_array;
        if (utf8_text)
            byte_array = utf8_text->getUTF8TextData();
        else
            byte_array = utf16_text->getUTF16TextData();
        if (byte_array.length > 0) {
            size_t num_write = fwrite(byte_array.data, 1, byte_array.length, file);
            if (num_write != byte_array.length)
                BMX_EXCEPTION(("Failed to write text object to file: %s", bmx_strerror(errno).c_str()));
        }
    } else if (gs_text) {
        ReadGenericStream(file, 0, 0);
    } else {
        log_warn("Can't read unknown text object type\n");
    }
}

mxfUL MXFTextObject::GetSchemeId() const
{
    return mTextObject->getTextBasedMetadataPayloadSchemaID();
}

string MXFTextObject::GetMimeType() const
{
    return mTextObject->getTextMIMEMediaType();
}

string MXFTextObject::GetLanguageCode() const
{
    return mTextObject->getRFC5646TextLanguageCode();
}

string MXFTextObject::GetTextDataDescription() const
{
    if (mTextObject->haveTextDataDescription())
        return mTextObject->getTextDataDescription();
    else
        return "";
}

TextEncoding MXFTextObject::GetEncoding() const
{
    if (dynamic_cast<UTF8TextBasedSet*>(mTextObject))
        return UTF8;
    else if (dynamic_cast<UTF16TextBasedSet*>(mTextObject))
        return UTF16;
    else
        return UNKNOWN_TEXT_ENCODING;
}

bool MXFTextObject::IsInGenericStream() const
{
    return dynamic_cast<GenericStreamTextBasedSet*>(mTextObject) != 0;
}

bool MXFTextObject::CheckIsXML()
{
    // check for text/xml, application/xml and application/...+xml

    string mime_type = mTextObject->getTextMIMEMediaType();
    size_t sep_pos = mime_type.find('/');
    if (sep_pos == string::npos)
        return false;

    string type = trim_string(mime_type.substr(0, sep_pos));
    if (type != "text" && type != "application")
        return false;

    string subtype = trim_string(mime_type.substr(sep_pos + 1));
    if (subtype.compare(0, 3, "xml") == 0)
        return true;
    else if (type == "text")
        return false;
    // note: content-type syntax appears to disallow '.' and so allowed chars based on IANA registrations instead
    size_t i;
    for (i = 0; i < subtype.size(); i++) {
        char &c = subtype[i];
        if (!(c >= 'a' && c <= 'z') &&
            !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') &&
            !(c == '.' || c == '-'))
        {
            break;
        }
    }
    if (i > 0 && i < subtype.size() && subtype.compare(i, 4, "+xml") == 0)
        return true;
    else
        return false;
}

void MXFTextObject::ReadGenericStream(FILE *text_file_out, unsigned char **data_out, size_t *data_out_size)
{
    mxfpp::File *mxf_file = mFileReader->mFile;
    int64_t original_file_pos = mxf_file->tell();
    try
    {
        GenericStreamTextBasedSet *gs_text = dynamic_cast<GenericStreamTextBasedSet*>(mTextObject);

        uint64_t body_size = 0;
        ByteArray buffer;
        mxfKey key;
        uint8_t llen;
        uint64_t len;
        const vector<Partition*> &partitions = mFileReader->mFile->getPartitions();
        size_t i;
        for (i = 0; i < partitions.size(); i++) {
            if (partitions[i]->getBodySID() != gs_text->getGenericStreamSID())
                continue;

            if (partitions[i]->getBodyOffset() > body_size) {
                log_warn("Ignoring potential missing text object generic stream data; "
                         "partition pack's BodyOffset 0x%" PRIx64 " > expected offset 0x" PRIx64 "\n",
                         partitions[i]->getBodyOffset(), body_size);
                continue;
            } else if (partitions[i]->getBodyOffset() < body_size) {
                log_warn("Ignoring overlapping or repeated text object generic stream data; "
                         "partition pack's BodyOffset 0x%" PRIx64 " < expected offset 0x" PRIx64 "\n",
                         partitions[i]->getBodyOffset(), body_size);
                continue;
            }

            mxf_file->seek(partitions[i]->getThisPartition(), SEEK_SET);
            mxf_file->readKL(&key, &llen, &len);
            mxf_file->skip(len);

            bool have_gs_key = false;
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
                } else if (mxf_is_gs_data_element(&key)) {
                    if (key != MXF_EE_K(RP2057_LE) &&
                        key != MXF_EE_K(RP2057_BE) && // == MXF_EE_K(RP2057_BYTES)
                        key != MXF_EE_K(RP2057_ENDIAN_UNK))
                    {
                        BMX_EXCEPTION(("Generic stream essence element key is not a RP 2057 key"));
                    }
                    if (body_size == 0 || mGSByteOrder != UNKNOWN_BYTE_ORDER) {
                        ByteOrder byte_order = UNKNOWN_BYTE_ORDER;
                        if (key == MXF_EE_K(RP2057_LE))
                            byte_order = BMX_LITTLE_ENDIAN;
                        else if (key == MXF_EE_K(RP2057_BE)) // == MXF_EE_K(RP2057_BYTES)
                            byte_order = BMX_BIG_ENDIAN;
                        if (body_size == 0)
                            mGSByteOrder = byte_order;
                        else if (mGSByteOrder != byte_order)
                            mGSByteOrder = UNKNOWN_BYTE_ORDER;
                    }

                    if (data_out) {
                        if (len > UINT32_MAX || (uint64_t)buffer.GetAllocatedSize() + len > UINT32_MAX)
                            BMX_EXCEPTION(("Text object in generic stream exceeds maximum supported in-memory size"));
                        buffer.Grow((uint32_t)len);
                    } else {
                        buffer.Allocate(8192);
                    }
                    uint64_t rem_len = len;
                    while (rem_len > 0) {
                        uint32_t count = 8192;
                        if (count > rem_len)
                            count = (uint32_t)rem_len;
                        uint32_t num_read = mxf_file->read(buffer.GetBytesAvailable(), count);
                        if (num_read != count)
                            BMX_EXCEPTION(("Failed to read text object data from generic stream"));
                        if (text_file_out) {
                            size_t num_write = fwrite(buffer.GetBytesAvailable(), 1, num_read, text_file_out);
                            if (num_write != num_read) {
                                BMX_EXCEPTION(("Failed to write text object to file: %s",
                                               bmx_strerror(errno).c_str()));
                            }
                        } else {
                            buffer.IncrementSize(num_read);
                        }
                        rem_len -= num_read;
                    }

                    have_gs_key = true;
                    body_size += mxfKey_extlen + llen + len;
                } else {
                    mxf_file->skip(len);
                    if (have_gs_key)
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
