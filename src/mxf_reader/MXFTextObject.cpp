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

#include <string.h>
#include <errno.h>

#include <libMXF++/MXF.h>

#include <bmx/mxf_reader/MXFTextObject.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/GenericStreamReader.h>
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

    mExpectedStreamKeys.push_back(MXF_EE_K(RP2057_LE));
    mExpectedStreamKeys.push_back(MXF_EE_K(RP2057_BE));  // == MXF_EE_K(RP2057_BYTES)
    mExpectedStreamKeys.push_back(MXF_EE_K(RP2057_ENDIAN_UNK));
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
    GenericStreamTextBasedSet *gs_text = dynamic_cast<GenericStreamTextBasedSet*>(mTextObject);

    GenericStreamReader stream_reader(mFileReader->mFile, gs_text->getGenericStreamSID(), mExpectedStreamKeys);
    if (text_file_out)
        stream_reader.Read(text_file_out);
    else
        stream_reader.Read(data_out, data_out_size);

    if (*stream_reader.GetStreamKey() == MXF_EE_K(RP2057_LE))
        mGSByteOrder = BMX_LITTLE_ENDIAN;
    else if (*stream_reader.GetStreamKey() == MXF_EE_K(RP2057_BE))
        mGSByteOrder = BMX_BIG_ENDIAN;
    else
        mGSByteOrder = UNKNOWN_BYTE_ORDER;
}
