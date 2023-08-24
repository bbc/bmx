/*
 * Copyright (C) 2009, British Broadcasting Corporation
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

#ifndef __TAGGED_VALUE_H__
#define __TAGGED_VALUE_H__


#include <libMXF++/metadata/InterchangeObject.h>



namespace mxfpp
{


class TaggedValue : public mxfpp::InterchangeObject
{
public:
    typedef enum
    {
        UNKNOWN_VALUE_TYPE,
        STRING_VALUE_TYPE,
        INT32_VALUE_TYPE,
    } ValueType;

public:
    static void registerObjectFactory(mxfpp::HeaderMetadata *header_metadata);

public:
    static const mxfKey set_key;

public:
    friend class mxfpp::MetadataSetFactory<TaggedValue>;

public:
    TaggedValue(mxfpp::HeaderMetadata *header_metadata);
    TaggedValue(mxfpp::HeaderMetadata *header_metadata, std::string name);
    TaggedValue(mxfpp::HeaderMetadata *header_metadata, std::string name, std::string value);
    TaggedValue(mxfpp::HeaderMetadata *header_metadata, std::string name, int32_t value);
    virtual ~TaggedValue();


    // getters
    std::string getName();
    ValueType getValueType();
    std::string getStringValue();
    int32_t getInt32Value();
    std::vector<TaggedValue*> getAvidAttributes();


    // setters
    void setName(std::string value);
    void setValue(std::string value);
    void setValue(int32_t value);
    TaggedValue* appendAvidAttribute(std::string name, std::string value);
    TaggedValue* appendAvidAttribute(std::string name, int32_t value);
    TaggedValue* appendAvidAttribute(TaggedValue *value);

protected:
    TaggedValue(mxfpp::HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set);
};


};



#endif
