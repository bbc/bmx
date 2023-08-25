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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <cstdlib>

#include <memory>

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

#include <libMXF++/extensions/TaggedValue.h>

using namespace std;
using namespace mxfpp;



const mxfKey TaggedValue::set_key = MXF_SET_K(TaggedValue);

void TaggedValue::registerObjectFactory(HeaderMetadata *header_metadata)
{
    header_metadata->registerObjectFactory(&set_key, new MetadataSetFactory<TaggedValue>());
}



TaggedValue::TaggedValue(HeaderMetadata *header_metadata)
: InterchangeObject(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);
}

TaggedValue::TaggedValue(HeaderMetadata *header_metadata, string name)
: InterchangeObject(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);

    setName(name);
}

TaggedValue::TaggedValue(HeaderMetadata *header_metadata, string name, string value)
: InterchangeObject(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);

    setName(name);
    setValue(value);
}

TaggedValue::TaggedValue(HeaderMetadata *header_metadata, string name, int32_t value)
: InterchangeObject(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);

    setName(name);
    setValue(value);
}

TaggedValue::TaggedValue(HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set)
: InterchangeObject(header_metadata, c_metadata_set)
{}

TaggedValue::~TaggedValue()
{}

string TaggedValue::getName()
{
    return getStringItem(&MXF_ITEM_K(TaggedValue, Name));
}

TaggedValue::ValueType TaggedValue::getValueType()
{
    if (mxf_avid_is_string_tagged_value(_cMetadataSet))
        return STRING_VALUE_TYPE;
    else if (mxf_avid_is_int32_tagged_value(_cMetadataSet))
        return INT32_VALUE_TYPE;
    else
        return UNKNOWN_VALUE_TYPE;
}

string TaggedValue::getStringValue()
{
    mxfUTF16Char *utf16_value = 0;
    char *utf8_value = 0;
    size_t utf8_size;
    string result;

    try {
        MXFPP_CHECK(mxf_avid_get_string_tagged_value(_cMetadataSet, &utf16_value));

        utf8_size = mxf_utf16_to_utf8(0, utf16_value, 0);
        MXFPP_CHECK(utf8_size != (size_t)(-1));
        utf8_size += 1;
        utf8_value = new char[utf8_size];
        mxf_utf16_to_utf8(utf8_value, utf16_value, utf8_size);

        result = utf8_value;

        delete [] utf16_value;
        utf16_value = 0;
        delete [] utf8_value;
        utf8_value = 0;

        return result;

    } catch (...) {
        delete [] utf16_value;
        delete [] utf8_value;
        throw;
    }
}

int32_t TaggedValue::getInt32Value()
{
    int32_t int32_value;
    MXFPP_CHECK(mxf_avid_get_int32_tagged_value(_cMetadataSet, &int32_value));
    return int32_value;
}

vector<TaggedValue*> TaggedValue::getAvidAttributes()
{
    vector<TaggedValue*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(TaggedValue, TaggedValueAttributeList)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<TaggedValue*>(iter->get()) != 0);
        result.push_back(dynamic_cast<TaggedValue*>(iter->get()));
    }
    return result;
}

void TaggedValue::setName(string value)
{
    setStringItem(&MXF_ITEM_K(TaggedValue, Name), value);
}

void TaggedValue::setValue(string value)
{
    mxfUTF16Char *utf16Val = 0;
    size_t utf16ValSize;
    try
    {
        utf16ValSize = mxf_utf8_to_utf16(NULL, value.c_str(), 0);
        MXFPP_CHECK(utf16ValSize != (size_t)(-1));
        utf16ValSize += 1;
        utf16Val = new mxfUTF16Char[utf16ValSize];
        mxf_utf8_to_utf16(utf16Val, value.c_str(), utf16ValSize);

        MXFPP_CHECK(mxf_avid_set_indirect_string_item(_cMetadataSet, &MXF_ITEM_K(TaggedValue, Value), utf16Val));

        delete [] utf16Val;
    }
    catch (...)
    {
        delete [] utf16Val;
        throw;
    }
}

void TaggedValue::setValue(int32_t value)
{
    MXFPP_CHECK(mxf_avid_set_indirect_int32_item(_cMetadataSet, &MXF_ITEM_K(TaggedValue, Value), value));
}

TaggedValue* TaggedValue::appendAvidAttribute(string name, string value)
{
    return appendAvidAttribute(new TaggedValue(_headerMetadata, name, value));
}

TaggedValue* TaggedValue::appendAvidAttribute(string name, int32_t value)
{
    return appendAvidAttribute(new TaggedValue(_headerMetadata, name, value));
}

TaggedValue* TaggedValue::appendAvidAttribute(TaggedValue *taggedValue)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(TaggedValue, TaggedValueAttributeList), taggedValue);
    return taggedValue;
}

