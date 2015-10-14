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

#ifndef BMX_MXF_TEXT_OBJECT_H_
#define BMX_MXF_TEXT_OBJECT_H_

#include <stdio.h>

#include <bmx/BMXTypes.h>


namespace mxfpp
{
    class TextBasedObject;
};

namespace bmx
{

class MXFFileReader;

class MXFTextObject
{
public:
    MXFTextObject(MXFFileReader *file_reader, mxfpp::TextBasedObject *text_object,
                  mxfUMID package_uid, uint32_t track_id, uint16_t component_index);
    ~MXFTextObject();

    void Read(unsigned char **data, size_t *size);
    void Read(FILE *file);

    mxfUMID GetPackageUID() const      { return mPackageUID; }
    uint32_t GetTrackId() const        { return mTrackId; }
    uint16_t GetComponentIndex() const { return mComponentIndex; }

    mxfUL GetSchemeId() const;
    std::string GetMimeType() const;
    std::string GetLanguageCode() const;
    std::string GetTextDataDescription() const;
    TextEncoding GetEncoding() const;
    bool IsInGenericStream() const;

    // only valid after Read() and IsInGenericStream() true
    // it doesn't distinguish between BMX_BIG_ENDIAN or BMX_BYTE_ORIENTED
    ByteOrder GetGSByteOrder() const { return mGSByteOrder; }

    bool IsXML() const { return mIsXML; }

private:
    bool CheckIsXML();

    void ReadGenericStream(FILE *text_file_out, unsigned char **data_out, size_t *data_out_size);

private:
    MXFFileReader *mFileReader;
    mxfpp::TextBasedObject *mTextObject;
    mxfUMID mPackageUID;
    uint32_t mTrackId;
    uint16_t mComponentIndex;
    bool mIsXML;
    ByteOrder mGSByteOrder;
};


};


#endif
