/*
 * Copyright (C) 2008, British Broadcasting Corporation
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

#include <cstdlib>

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace mxfpp;



class ReferencedObjectIterator : public ObjectIterator
{
public:
    ReferencedObjectIterator(HeaderMetadata *headerMetadata, const MetadataSet *metadataSet, const mxfKey *itemKey)
    : _headerMetadata(headerMetadata), _itemKey(*itemKey), _value(0), _length(0), _haveNext(true),
      _referencedMetadataSet(0)
    {
        mxf_initialise_sets_iter(headerMetadata->getCHeaderMetadata(), &_setsIter);
        MXFPP_CHECK(mxf_initialise_array_item_iterator(metadataSet->getCMetadataSet(), itemKey, &_arrayIter));
    }

    virtual ~ReferencedObjectIterator() {}


    virtual bool next()
    {
        ::MXFMetadataSet *cSet;

        _referencedMetadataSet = 0;
        while (_haveNext)
        {
            _haveNext = mxf_next_array_item_element(&_arrayIter, &_value, &_length) != 0;
            if (_haveNext)
            {
                MXFPP_CHECK(_length == mxfUUID_extlen);
                if (!mxf_get_strongref_s(_headerMetadata->getCHeaderMetadata(), &_setsIter, _value, &cSet))
                {
                    char keyStr[KEY_STR_SIZE];
                    mxf_sprint_key(keyStr, &_itemKey);
                    mxf_log_warn("Failed to de-reference strong reference array element (item key = %s)\n", keyStr);
                    continue;
                }

                _referencedMetadataSet = _headerMetadata->wrap(cSet);
                break;
            }
        }

        return _haveNext;
    }

    virtual MetadataSet* get()
    {
        if (_referencedMetadataSet == 0)
        {
            throw MXFException("Object reference iterator has no more objects to return");
        }
        return _referencedMetadataSet;
    }

    virtual uint32_t size()
    {
        return _arrayIter.numElements;
    }


private:
    HeaderMetadata* _headerMetadata;
    mxfKey _itemKey;
    uint8_t* _value;
    uint32_t _length;
    ::MXFArrayItemIterator _arrayIter;
    ::MXFListIterator _setsIter;
    bool _haveNext;
    MetadataSet* _referencedMetadataSet;
};



MetadataSet::MetadataSet(const MetadataSet &set)
: _headerMetadata(set._headerMetadata), _cMetadataSet(set._cMetadataSet)
{}

MetadataSet::MetadataSet(HeaderMetadata *headerMetadata, ::MXFMetadataSet *metadataSet)
: _headerMetadata(headerMetadata), _cMetadataSet(metadataSet)
{}

MetadataSet::~MetadataSet()
{
    if (_cMetadataSet != 0 && _cMetadataSet->headerMetadata == 0)
    {
        mxf_free_set(&_cMetadataSet);
    }

    if (_headerMetadata != 0)
    {
        _headerMetadata->remove(this);
    }
}

vector<MXFMetadataItem*> MetadataSet::getItems() const
{
    vector<MXFMetadataItem*> items;
    MXFListIterator item_iter;
    mxf_initialise_list_iter(&item_iter, &_cMetadataSet->items);
    while (mxf_next_list_iter_element(&item_iter))
        items.push_back((MXFMetadataItem*)mxf_get_iter_element(&item_iter));

    return items;
}

bool MetadataSet::haveItem(const mxfKey *itemKey) const
{
    return mxf_have_item(_cMetadataSet, itemKey) != 0;
}

ByteArray MetadataSet::getRawBytesItem(const mxfKey *itemKey) const
{
    MXFMetadataItem *item;
    ByteArray byteArray;
    MXFPP_CHECK(mxf_get_item(_cMetadataSet, itemKey, &item));
    byteArray.data = item->value;
    byteArray.length = item->length;
    return byteArray;
}

uint8_t MetadataSet::getUInt8Item(const mxfKey *itemKey) const
{
    uint8_t result;
    MXFPP_CHECK(mxf_get_uint8_item(_cMetadataSet, itemKey, &result));
    return result;
}

uint16_t MetadataSet::getUInt16Item(const mxfKey *itemKey) const
{
    uint16_t result;
    MXFPP_CHECK(mxf_get_uint16_item(_cMetadataSet, itemKey, &result));
    return result;
}

uint32_t MetadataSet::getUInt32Item(const mxfKey *itemKey) const
{
    uint32_t result;
    MXFPP_CHECK(mxf_get_uint32_item(_cMetadataSet, itemKey, &result));
    return result;
}

uint64_t MetadataSet::getUInt64Item(const mxfKey *itemKey) const
{
    uint64_t result;
    MXFPP_CHECK(mxf_get_uint64_item(_cMetadataSet, itemKey, &result));
    return result;
}

int8_t MetadataSet::getInt8Item(const mxfKey *itemKey) const
{
    int8_t result;
    MXFPP_CHECK(mxf_get_int8_item(_cMetadataSet, itemKey, &result));
    return result;
}

int16_t MetadataSet::getInt16Item(const mxfKey *itemKey) const
{
    int16_t result;
    MXFPP_CHECK(mxf_get_int16_item(_cMetadataSet, itemKey, &result));
    return result;
}

int32_t MetadataSet::getInt32Item(const mxfKey *itemKey) const
{
    int32_t result;
    MXFPP_CHECK(mxf_get_int32_item(_cMetadataSet, itemKey, &result));
    return result;
}

int64_t MetadataSet::getInt64Item(const mxfKey *itemKey) const
{
    int64_t result;
    MXFPP_CHECK(mxf_get_int64_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfVersionType MetadataSet::getVersionTypeItem(const mxfKey *itemKey) const
{
    mxfVersionType result;
    MXFPP_CHECK(mxf_get_version_type_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfUUID MetadataSet::getUUIDItem(const mxfKey *itemKey) const
{
    mxfUUID result;
    MXFPP_CHECK(mxf_get_uuid_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfUL MetadataSet::getULItem(const mxfKey *itemKey) const
{
    mxfUL result;
    MXFPP_CHECK(mxf_get_ul_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfAUID MetadataSet::getAUIDItem(const mxfKey *itemKey) const
{
    mxfAUID result;
    MXFPP_CHECK(mxf_get_auid_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfUMID MetadataSet::getUMIDItem(const mxfKey *itemKey) const
{
    mxfUMID result;
    MXFPP_CHECK(mxf_get_umid_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfTimestamp MetadataSet::getTimestampItem(const mxfKey *itemKey) const
{
    mxfTimestamp result;
    MXFPP_CHECK(mxf_get_timestamp_item(_cMetadataSet, itemKey, &result));
    return result;
}

int64_t MetadataSet::getLengthItem(const mxfKey *itemKey) const
{
    int64_t result;
    MXFPP_CHECK(mxf_get_length_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfRational MetadataSet::getRationalItem(const mxfKey *itemKey) const
{
    mxfRational result;
    MXFPP_CHECK(mxf_get_rational_item(_cMetadataSet, itemKey, &result));
    return result;
}

int64_t MetadataSet::getPositionItem(const mxfKey *itemKey) const
{
    int64_t result;
    MXFPP_CHECK(mxf_get_position_item(_cMetadataSet, itemKey, &result));
    return result;
}

bool MetadataSet::getBooleanItem(const mxfKey *itemKey) const
{
    mxfBoolean result;
    MXFPP_CHECK(mxf_get_boolean_item(_cMetadataSet, itemKey, &result));
    return result != 0;
}

mxfProductVersion MetadataSet::getProductVersionItem(const mxfKey *itemKey) const
{
    mxfProductVersion result;
    MXFMetadataItem *item;
    MXFPP_CHECK(mxf_get_item(_cMetadataSet, itemKey, &item));
    if (item->length == mxfProductVersion_extlen - 1)
    {
        mxf_avid_get_product_version(item->value, &result);
    }
    else
    {
        MXFPP_CHECK(item->length == mxfProductVersion_extlen);
        mxf_get_product_version(item->value, &result);
    }
    return result;
}

mxfRGBALayout MetadataSet::getRGBALayoutItem(const mxfKey *itemKey) const
{
    mxfRGBALayout result;
    MXFPP_CHECK(mxf_get_rgba_layout_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfJ2KExtendedCapabilities MetadataSet::getJ2KExtendedCapabilitiesItem(const mxfKey *itemKey) const
{
    mxfJ2KExtendedCapabilities result;
    MXFPP_CHECK(mxf_get_j2k_ext_capabilities_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfThreeColorPrimaries MetadataSet::getThreeColorPrimariesItem(const mxfKey *itemKey) const
{
    mxfThreeColorPrimaries result;
    MXFPP_CHECK(mxf_get_three_color_primaries_item(_cMetadataSet, itemKey, &result));
    return result;
}

mxfColorPrimary MetadataSet::getColorPrimaryItem(const mxfKey *itemKey) const
{
    mxfColorPrimary result;
    MXFPP_CHECK(mxf_get_color_primary_item(_cMetadataSet, itemKey, &result));
    return result;
}

string MetadataSet::getStringItem(const mxfKey *itemKey) const
{
    string result;
    mxfUTF16Char *utf16Result = 0;
    char *utf8Result = 0;
    try
    {
        uint16_t utf16Size;
        MXFPP_CHECK(mxf_get_utf16string_item_size(_cMetadataSet, itemKey, &utf16Size));
        utf16Result = new mxfUTF16Char[utf16Size];
        MXFPP_CHECK(mxf_get_utf16string_item(_cMetadataSet, itemKey, utf16Result));

        size_t utf8Size = mxf_utf16_to_utf8(0, utf16Result, 0);
        MXFPP_CHECK(utf8Size != (size_t)(-1));
        utf8Size += 1;
        utf8Result = new char[utf8Size];
        mxf_utf16_to_utf8(utf8Result, utf16Result, utf8Size);
        result = utf8Result;
        delete [] utf16Result;
        utf16Result = 0;
        delete [] utf8Result;
        utf8Result = 0;
        return result;
    }
    catch (...)
    {
        delete [] utf16Result;
        delete [] utf8Result;
        throw;
    }
}

string MetadataSet::getUTF8StringItem(const mxfKey *itemKey) const
{
    string result;
    char *utf8Result = 0;
    try
    {
        uint16_t utf8Size;
        MXFPP_CHECK(mxf_get_utf8string_item_size(_cMetadataSet, itemKey, &utf8Size));
        utf8Result = new char[utf8Size];
        MXFPP_CHECK(mxf_get_utf8string_item(_cMetadataSet, itemKey, utf8Result));
        result = utf8Result;
        delete [] utf8Result;
        utf8Result = 0;
        return result;
    }
    catch (...)
    {
        delete [] utf8Result;
        throw;
    }
}

string MetadataSet::getISO7StringItem(const mxfKey *itemKey) const
{
    string result;
    char *iso7Result = 0;
    try
    {
        uint16_t iso7Size;
        MXFPP_CHECK(mxf_get_iso7string_item_size(_cMetadataSet, itemKey, &iso7Size));
        iso7Result = new char[iso7Size];
        MXFPP_CHECK(mxf_get_iso7string_item(_cMetadataSet, itemKey, iso7Result));
        result = iso7Result;
        delete [] iso7Result;
        iso7Result = 0;
        return result;
    }
    catch (...)
    {
        delete [] iso7Result;
        throw;
    }
}

MetadataSet* MetadataSet::getStrongRefItem(const mxfKey *itemKey) const
{
    ::MXFMetadataSet *cSet;
    if (!mxf_get_strongref_item(_cMetadataSet, itemKey, &cSet))
    {
        char keyStr[KEY_STR_SIZE];
        mxf_sprint_key(keyStr, itemKey);
        mxf_log_warn("Failed to de-reference strong reference property (item key = %s)\n", keyStr);
        return 0;
    }
    return _headerMetadata->wrap(cSet);
}

MetadataSet* MetadataSet::getStrongRefItemLight(const mxfKey *itemKey) const
{
    ::MXFMetadataSet *cSet;
    if (!mxf_get_strongref_item_light(_cMetadataSet, itemKey, &cSet))
    {
        return 0;
    }
    return _headerMetadata->wrap(cSet);
}

MetadataSet* MetadataSet::getWeakRefItem(const mxfKey *itemKey) const
{
    ::MXFMetadataSet *cSet;
    if (!mxf_get_weakref_item(_cMetadataSet, itemKey, &cSet))
    {
        char keyStr[KEY_STR_SIZE];
        mxf_sprint_key(keyStr, itemKey);
        mxf_log_warn("Failed to de-reference weak reference property (item key = %s)\n", keyStr);
        return 0;
    }
    return _headerMetadata->wrap(cSet);
}

MetadataSet* MetadataSet::getWeakRefItemLight(const mxfKey *itemKey) const
{
    ::MXFMetadataSet *cSet;
    if (!mxf_get_weakref_item_light(_cMetadataSet, itemKey, &cSet))
    {
        return 0;
    }
    return _headerMetadata->wrap(cSet);
}

vector<uint8_t> MetadataSet::getUInt8ArrayItem(const mxfKey *itemKey) const
{
    vector<uint8_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    uint8_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 1);
        mxf_get_uint8(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<uint16_t> MetadataSet::getUInt16ArrayItem(const mxfKey *itemKey) const
{
    vector<uint16_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    uint16_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 2);
        mxf_get_uint16(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<uint32_t> MetadataSet::getUInt32ArrayItem(const mxfKey *itemKey) const
{
    vector<uint32_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    uint32_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 4);
        mxf_get_uint32(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<uint64_t> MetadataSet::getUInt64ArrayItem(const mxfKey *itemKey) const
{
    vector<uint64_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    uint64_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 8);
        mxf_get_uint64(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<int8_t> MetadataSet::getInt8ArrayItem(const mxfKey *itemKey) const
{
    vector<int8_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    int8_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 1);
        mxf_get_int8(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<int16_t> MetadataSet::getInt16ArrayItem(const mxfKey *itemKey) const
{
    vector<int16_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    int16_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 2);
        mxf_get_int16(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<int32_t> MetadataSet::getInt32ArrayItem(const mxfKey *itemKey) const
{
    vector<int32_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    int32_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 4);
        mxf_get_int32(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<int64_t> MetadataSet::getInt64ArrayItem(const mxfKey *itemKey) const
{
    vector<int64_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    int64_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 8);
        mxf_get_int64(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfVersionType> MetadataSet::getVersionTypeArrayItem(const mxfKey *itemKey) const
{
    vector<mxfVersionType> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfVersionType value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfVersionType_extlen);
        mxf_get_version_type(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfUUID> MetadataSet::getUUIDArrayItem(const mxfKey *itemKey) const
{
    vector<mxfUUID> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfUUID value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfUUID_extlen);
        mxf_get_uuid(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfUL> MetadataSet::getULArrayItem(const mxfKey *itemKey) const
{
    vector<mxfUL> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfUL value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfUL_extlen);
        mxf_get_ul(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfAUID> MetadataSet::getAUIDArrayItem(const mxfKey *itemKey) const
{
    vector<mxfAUID> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfAUID value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfAUID_extlen);
        mxf_get_auid(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfUMID> MetadataSet::getUMIDArrayItem(const mxfKey *itemKey) const
{
    vector<mxfUMID> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfUMID value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfUMID_extlen);
        mxf_get_umid(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfTimestamp> MetadataSet::getTimestampArrayItem(const mxfKey *itemKey) const
{
    vector<mxfTimestamp> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfTimestamp value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfTimestamp_extlen);
        mxf_get_timestamp(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<int64_t> MetadataSet::getLengthArrayItem(const mxfKey *itemKey) const
{
    vector<int64_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    int64_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 8);
        mxf_get_int64(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfRational> MetadataSet::getRationalArrayItem(const mxfKey *itemKey) const
{
    vector<mxfRational> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfRational value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfRational_extlen);
        mxf_get_rational(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<int64_t> MetadataSet::getPositionArrayItem(const mxfKey *itemKey) const
{
    vector<int64_t> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    int64_t value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == 8);
        mxf_get_int64(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<bool> MetadataSet::getBooleanArrayItem(const mxfKey *itemKey) const
{
    vector<bool> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfBoolean value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfBoolean_extlen);
        mxf_get_boolean(element, &value);
        result.push_back(value != 0);
    }
    return result;
}

vector<mxfProductVersion> MetadataSet::getProductVersionArrayItem(const mxfKey *itemKey) const
{
    vector<mxfProductVersion> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfProductVersion value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfProductVersion_extlen);
        mxf_get_product_version(element, &value);
        result.push_back(value);
    }
    return result;
}

vector<mxfJ2KComponentSizing> MetadataSet::getJ2KComponentSizingArrayItem(const mxfKey *itemKey) const
{
    vector<mxfJ2KComponentSizing> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfJ2KComponentSizing value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength))
    {
        MXFPP_CHECK(elementLength == mxfJ2KComponentSizing_extlen);
        mxf_get_j2k_component_sizing(element, &value);
        result.push_back(value);
    }
    return result;
}

ObjectIterator* MetadataSet::getStrongRefArrayItem(const mxfKey *itemKey) const
{
    return new ReferencedObjectIterator(_headerMetadata, this, itemKey);
}

ObjectIterator* MetadataSet::getWeakRefArrayItem(const mxfKey *itemKey) const
{
    return new ReferencedObjectIterator(_headerMetadata, this, itemKey);
}


void MetadataSet::setRawBytesItem(const mxfKey *itemKey, ByteArray value)
{
    MXFPP_CHECK(mxf_set_item(_cMetadataSet, itemKey, value.data, value.length));
}

void MetadataSet::setUInt8Item(const mxfKey *itemKey, uint8_t value)
{
    MXFPP_CHECK(mxf_set_uint8_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setUInt16Item(const mxfKey *itemKey, uint16_t value)
{
    MXFPP_CHECK(mxf_set_uint16_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setUInt32Item(const mxfKey *itemKey, uint32_t value)
{
    MXFPP_CHECK(mxf_set_uint32_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setUInt64Item(const mxfKey *itemKey, uint64_t value)
{
    MXFPP_CHECK(mxf_set_uint64_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setInt8Item(const mxfKey *itemKey, int8_t value)
{
    MXFPP_CHECK(mxf_set_int8_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setInt16Item(const mxfKey *itemKey, int16_t value)
{
    MXFPP_CHECK(mxf_set_int16_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setInt32Item(const mxfKey *itemKey, int32_t value)
{
    MXFPP_CHECK(mxf_set_int32_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setInt64Item(const mxfKey *itemKey, int64_t value)
{
    MXFPP_CHECK(mxf_set_int64_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setVersionTypeItem(const mxfKey *itemKey, mxfVersionType value)
{
    MXFPP_CHECK(mxf_set_version_type_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setUUIDItem(const mxfKey *itemKey, mxfUUID value)
{
    MXFPP_CHECK(mxf_set_uuid_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setULItem(const mxfKey *itemKey, mxfUL value)
{
    MXFPP_CHECK(mxf_set_ul_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setAUIDItem(const mxfKey *itemKey, mxfAUID value)
{
    MXFPP_CHECK(mxf_set_auid_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setUMIDItem(const mxfKey *itemKey, mxfUMID value)
{
    MXFPP_CHECK(mxf_set_umid_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setTimestampItem(const mxfKey *itemKey, mxfTimestamp value)
{
    MXFPP_CHECK(mxf_set_timestamp_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setLengthItem(const mxfKey *itemKey, int64_t value)
{
    MXFPP_CHECK(mxf_set_length_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setRationalItem(const mxfKey *itemKey, mxfRational value)
{
    MXFPP_CHECK(mxf_set_rational_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setPositionItem(const mxfKey *itemKey, int64_t value)
{
    MXFPP_CHECK(mxf_set_position_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setBooleanItem(const mxfKey *itemKey, bool value)
{
    MXFPP_CHECK(mxf_set_boolean_item(_cMetadataSet, itemKey, value));
}

void MetadataSet::setProductVersionItem(const mxfKey *itemKey, mxfProductVersion value)
{
    MXFPP_CHECK(mxf_set_product_version_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setRGBALayoutItem(const mxfKey *itemKey, mxfRGBALayout value)
{
    MXFPP_CHECK(mxf_set_rgba_layout_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setJ2KExtendedCapabilitiesItem(const mxfKey *itemKey, mxfJ2KExtendedCapabilities value)
{
    MXFPP_CHECK(mxf_set_j2k_ext_capabilities_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setThreeColorPrimariesItem(const mxfKey *itemKey, mxfThreeColorPrimaries value) const
{
    MXFPP_CHECK(mxf_set_three_color_primaries_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setColorPrimaryItem(const mxfKey *itemKey, mxfColorPrimary value) const
{
    MXFPP_CHECK(mxf_set_color_primary_item(_cMetadataSet, itemKey, &value));
}

void MetadataSet::setStringItem(const mxfKey *itemKey, string value)
{
    mxfUTF16Char *utf16Val = 0;
    size_t utf16ValSize;
    try
    {
        utf16ValSize = mxf_utf8_to_utf16(NULL, value.c_str(), 0);
        MXFPP_CHECK(utf16ValSize != (size_t)(-1));
        utf16ValSize += 1;
        utf16Val = new wchar_t[utf16ValSize];
        mxf_utf8_to_utf16(utf16Val, value.c_str(), utf16ValSize);

        MXFPP_CHECK(mxf_set_utf16string_item(_cMetadataSet, itemKey, utf16Val));

        delete [] utf16Val;
        utf16Val = 0;
    }
    catch (...)
    {
        delete [] utf16Val;
        throw;
    }
}

void MetadataSet::setFixedSizeStringItem(const mxfKey *itemKey, string value, uint16_t size)
{
    mxfUTF16Char *utf16Val = 0;
    size_t utf16ValSize;
    try
    {
        utf16ValSize = mxf_utf8_to_utf16(NULL, value.c_str(), 0);
        MXFPP_CHECK(utf16ValSize != (size_t)(-1));
        utf16ValSize += 1;
        utf16Val = new wchar_t[utf16ValSize];
        mxf_utf8_to_utf16(utf16Val, value.c_str(), utf16ValSize);

        MXFPP_CHECK(mxf_set_fixed_size_utf16string_item(_cMetadataSet, itemKey, utf16Val, size));

        delete [] utf16Val;
        utf16Val = 0;
    }
    catch (...)
    {
        delete [] utf16Val;
        throw;
    }
}

void MetadataSet::setUTF8StringItem(const mxfKey *itemKey, string value)
{
    MXFPP_CHECK(mxf_set_utf8string_item(_cMetadataSet, itemKey, value.c_str()));
}

void MetadataSet::setISO7StringItem(const mxfKey *itemKey, string value)
{
    MXFPP_CHECK(mxf_set_iso7string_item(_cMetadataSet, itemKey, value.c_str()));
}

void MetadataSet::setStrongRefItem(const mxfKey *itemKey, MetadataSet *value)
{
    MXFPP_CHECK(value->getCMetadataSet() != 0);
    MXFPP_CHECK(mxf_set_strongref_item(_cMetadataSet, itemKey, value->getCMetadataSet()));
}

void MetadataSet::setWeakRefItem(const mxfKey *itemKey, MetadataSet *value)
{
    MXFPP_CHECK(value->getCMetadataSet() != 0);
    MXFPP_CHECK(mxf_set_weakref_item(_cMetadataSet, itemKey, value->getCMetadataSet()));
}

void MetadataSet::setUInt8ArrayItem(const mxfKey *itemKey, const vector<uint8_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 1, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_uint8(value.at(i), data);
        data += 1;
    }
}

void MetadataSet::setUInt16ArrayItem(const mxfKey *itemKey, const vector<uint16_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 2, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_uint16(value.at(i), data);
        data += 2;
    }
}

void MetadataSet::setUInt32ArrayItem(const mxfKey *itemKey, const vector<uint32_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 4, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_uint32(value.at(i), data);
        data += 4;
    }
}

void MetadataSet::setUInt64ArrayItem(const mxfKey *itemKey, const vector<uint64_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 8, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_uint64(value.at(i), data);
        data += 8;
    }
}

void MetadataSet::setInt8ArrayItem(const mxfKey *itemKey, const vector<int8_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 1, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_int8(value.at(i), data);
        data += 1;
    }
}

void MetadataSet::setInt16ArrayItem(const mxfKey *itemKey, const vector<int16_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 2, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_int16(value.at(i), data);
        data += 2;
    }
}

void MetadataSet::setInt32ArrayItem(const mxfKey *itemKey, const vector<int32_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 4, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_int32(value.at(i), data);
        data += 4;
    }
}

void MetadataSet::setInt64ArrayItem(const mxfKey *itemKey, const vector<int64_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 8, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_int64(value.at(i), data);
        data += 8;
    }
}

void MetadataSet::setVersionTypeArrayItem(const mxfKey *itemKey, const vector<mxfVersionType> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfVersionType_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_version_type(value.at(i), data);
        data += mxfVersionType_extlen;
    }
}

void MetadataSet::setUUIDArrayItem(const mxfKey *itemKey, const vector<mxfUUID> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfUUID_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_uuid(&value.at(i), data);
        data += mxfUUID_extlen;
    }
}

void MetadataSet::setULArrayItem(const mxfKey *itemKey, const vector<mxfUL> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfUL_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_ul(&value.at(i), data);
        data += mxfUL_extlen;
    }
}

void MetadataSet::setAUIDArrayItem(const mxfKey *itemKey, const vector<mxfAUID> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfAUID_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_auid(&value.at(i), data);
        data += mxfAUID_extlen;
    }
}

void MetadataSet::setUMIDArrayItem(const mxfKey *itemKey, const vector<mxfUMID> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfUMID_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_umid(&value.at(i), data);
        data += mxfUMID_extlen;
    }
}

void MetadataSet::setTimestampArrayItem(const mxfKey *itemKey, const vector<mxfTimestamp> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfTimestamp_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_timestamp(&value.at(i), data);
        data += mxfTimestamp_extlen;
    }
}

void MetadataSet::setLengthArrayItem(const mxfKey *itemKey, const vector<int64_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 8, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_int64(value.at(i), data);
        data += 8;
    }
}

void MetadataSet::setRationalArrayItem(const mxfKey *itemKey, const vector<mxfRational> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfRational_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_rational(&value.at(i), data);
        data += mxfRational_extlen;
    }
}

void MetadataSet::setPositionArrayItem(const mxfKey *itemKey, const vector<int64_t> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, 8, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_int64(value.at(i), data);
        data += 8;
    }
}

void MetadataSet::setBooleanArrayItem(const mxfKey *itemKey, const vector<bool> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfBoolean_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_boolean(value.at(i), data);
        data += mxfBoolean_extlen;
    }
}

void MetadataSet::setProductVersionArrayItem(const mxfKey *itemKey, const vector<mxfProductVersion> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfProductVersion_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_product_version(&value.at(i), data);
        data += mxfProductVersion_extlen;
    }
}

void MetadataSet::setJ2KComponentSizingArrayItem(const mxfKey *itemKey, const vector<mxfJ2KComponentSizing> &value)
{
    size_t i;
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfJ2KComponentSizing_extlen, (uint32_t)value.size(), &data));
    for (i = 0; i < value.size(); i++)
    {
        mxf_set_j2k_component_sizing(&value.at(i), data);
        data += mxfJ2KComponentSizing_extlen;
    }
}

void MetadataSet::setStrongRefArrayItem(const mxfKey *itemKey, ObjectIterator *iter)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfUUID_extlen, iter->size(), &data));
    while (iter->next())
    {
        MXFPP_CHECK(iter->get()->getCMetadataSet() != 0);
        mxf_set_strongref(iter->get()->getCMetadataSet(), data);
        data += mxfUUID_extlen;
    }
}

void MetadataSet::setWeakRefArrayItem(const mxfKey *itemKey, ObjectIterator *iter)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfUUID_extlen, iter->size(), &data));
    while (iter->next())
    {
        MXFPP_CHECK(iter->get()->getCMetadataSet() != 0);
        mxf_set_strongref(iter->get()->getCMetadataSet(), data);
        data += mxfUUID_extlen;
    }
}

void MetadataSet::appendUInt8ArrayItem(const mxfKey *itemKey, uint8_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 1, 1, &data));
    mxf_set_uint8(value, data);
}

void MetadataSet::appendUInt16ArrayItem(const mxfKey *itemKey, uint16_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 2, 1, &data));
    mxf_set_uint16(value, data);
}

void MetadataSet::appendUInt32ArrayItem(const mxfKey *itemKey, uint32_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 4, 1, &data));
    mxf_set_uint32(value, data);
}

void MetadataSet::appendUInt64ArrayItem(const mxfKey *itemKey, uint64_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 8, 1, &data));
    mxf_set_uint64(value, data);
}

void MetadataSet::appendInt8ArrayItem(const mxfKey *itemKey, int8_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 1, 1, &data));
    mxf_set_int8(value, data);
}

void MetadataSet::appendInt16ArrayItem(const mxfKey *itemKey, int16_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 2, 1, &data));
    mxf_set_int16(value, data);
}

void MetadataSet::appendInt32ArrayItem(const mxfKey *itemKey, int32_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 4, 1, &data));
    mxf_set_int32(value, data);
}

void MetadataSet::appendInt64ArrayItem(const mxfKey *itemKey, int64_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 8, 1, &data));
    mxf_set_int64(value, data);
}

void MetadataSet::appendVersionTypeArrayItem(const mxfKey *itemKey, mxfVersionType value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfVersionType_extlen, 1, &data));
    mxf_set_version_type(value, data);
}

void MetadataSet::appendUUIDArrayItem(const mxfKey *itemKey, mxfUUID value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfUUID_extlen, 1, &data));
    mxf_set_uuid(&value, data);
}

void MetadataSet::appendULArrayItem(const mxfKey *itemKey, mxfUL value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfUL_extlen, 1, &data));
    mxf_set_ul(&value, data);
}

void MetadataSet::appendAUIDArrayItem(const mxfKey *itemKey, mxfAUID value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfAUID_extlen, 1, &data));
    mxf_set_auid(&value, data);
}

void MetadataSet::appendUMIDArrayItem(const mxfKey *itemKey, mxfUMID value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfUMID_extlen, 1, &data));
    mxf_set_umid(&value, data);
}

void MetadataSet::appendTimestampArrayItem(const mxfKey *itemKey, mxfTimestamp value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfTimestamp_extlen, 1, &data));
    mxf_set_timestamp(&value, data);
}

void MetadataSet::appendLengthArrayItem(const mxfKey *itemKey, int64_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 8, 1, &data));
    mxf_set_int64(value, data);
}

void MetadataSet::appendRationalArrayItem(const mxfKey *itemKey, mxfRational value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfRational_extlen, 1, &data));
    mxf_set_rational(&value, data);
}

void MetadataSet::appendPositionArrayItem(const mxfKey *itemKey, int64_t value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, 8, 1, &data));
    mxf_set_int64(value, data);
}

void MetadataSet::appendBooleanArrayItem(const mxfKey *itemKey, bool value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfBoolean_extlen, 1, &data));
    mxf_set_boolean(value, data);
}

void MetadataSet::appendProductVersionArrayItem(const mxfKey *itemKey, mxfProductVersion value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfProductVersion_extlen, 1, &data));
    mxf_set_product_version(&value, data);
}

void MetadataSet::appendJ2KComponentSizingArrayItem(const mxfKey *itemKey, mxfJ2KComponentSizing value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfJ2KComponentSizing_extlen, 1, &data));
    mxf_set_j2k_component_sizing(&value, data);
}

void MetadataSet::appendStrongRefArrayItem(const mxfKey *itemKey, MetadataSet *value)
{
    MXFPP_CHECK(mxf_add_array_item_strongref(_cMetadataSet, itemKey, value->getCMetadataSet()));
}

void MetadataSet::appendWeakRefArrayItem(const mxfKey *itemKey, MetadataSet *value)
{
    MXFPP_CHECK(mxf_add_array_item_weakref(_cMetadataSet, itemKey, value->getCMetadataSet()));
}


void MetadataSet::removeItem(const mxfKey *itemKey)
{
    MXFMetadataItem *item;
    MXFPP_CHECK(mxf_remove_item(_cMetadataSet, itemKey, &item));
    mxf_free_item(&item);
}


MetadataSet* MetadataSet::clone(HeaderMetadata *toHeaderMetadata)
{
    ::MXFMetadataSet *toMetadataSet;
    MXFPP_CHECK(mxf_clone_set(_cMetadataSet, toHeaderMetadata->getCHeaderMetadata(), &toMetadataSet));
    return toHeaderMetadata->wrap(toMetadataSet);
}


bool MetadataSet::validate(bool logErrors)
{
    return mxf_validate_set(_cMetadataSet, logErrors);
}


void MetadataSet::setAvidRGBColor(const mxfKey *itemKey, uint16_t red, uint16_t green, uint16_t blue)
{
    RGBColor color;
    color.red = red;
    color.green = green;
    color.blue = blue;
    MXFPP_CHECK(mxf_avid_set_rgb_color_item(_cMetadataSet, itemKey, &color));
}

void MetadataSet::setAvidProductVersion(const mxfKey *itemKey, mxfProductVersion value)
{
    MXFPP_CHECK(mxf_avid_set_product_version_item(_cMetadataSet, itemKey, &value));
}

