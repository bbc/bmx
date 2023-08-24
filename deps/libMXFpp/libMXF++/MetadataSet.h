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

#ifndef MXFPP_METADATA_SET_H_
#define MXFPP_METADATA_SET_H_

#include <map>
#include <vector>



namespace mxfpp
{


typedef struct
{
    uint8_t *data;
    uint16_t length;
} ByteArray;


class MetadataSet;

class ObjectIterator
{
public:
    virtual ~ObjectIterator() {};

    virtual bool next() = 0;
    virtual MetadataSet* get() = 0;
    virtual uint32_t size() = 0;
};

template <typename Obj>
class WrapObjectVectorIterator : public ObjectIterator
{
public:
    WrapObjectVectorIterator(const std::vector<Obj*> &value)
    : _value(value), _size(0), _pos(((uint32_t)-1))
    {
        _size = (uint32_t)value.size();
    }

    virtual ~WrapObjectVectorIterator() {}


    virtual bool next()
    {
        if (_pos < _size)
        {
            _pos++;
        }
        else if (_pos == ((uint32_t)-1))
        {
            _pos = 0;
        }
        return _pos < _size;
    }

    virtual MetadataSet* get()
    {
        if (_pos == ((uint32_t)-1))
        {
            throw MXFException("Must call next() before calling get() in object iterator");
        }
        else if (_pos >= _size)
        {
            throw MXFException("Object iterator has no more objects to return");
        }
        return _value.at(_pos);
    }

    virtual uint32_t size()
    {
        return _size;
    }

private:
    const std::vector<Obj*>& _value;
    uint32_t _size;
    uint32_t _pos;
};


class HeaderMetadata;

class MetadataSet
{
public:
    friend class HeaderMetadata;

public:
    MetadataSet(const MetadataSet &set);
    virtual ~MetadataSet();

    std::vector<MXFMetadataItem*> getItems() const;

    bool haveItem(const mxfKey *itemKey) const;

    ByteArray getRawBytesItem(const mxfKey *itemKey) const;

    uint8_t getUInt8Item(const mxfKey *itemKey) const;
    uint16_t getUInt16Item(const mxfKey *itemKey) const;
    uint32_t getUInt32Item(const mxfKey *itemKey) const;
    uint64_t getUInt64Item(const mxfKey *itemKey) const;
    int8_t getInt8Item(const mxfKey *itemKey) const;
    int16_t getInt16Item(const mxfKey *itemKey) const;
    int32_t getInt32Item(const mxfKey *itemKey) const;
    int64_t getInt64Item(const mxfKey *itemKey) const;
    mxfVersionType getVersionTypeItem(const mxfKey *itemKey) const;
    mxfUUID getUUIDItem(const mxfKey *itemKey) const;
    mxfUL getULItem(const mxfKey *itemKey) const;
    mxfAUID getAUIDItem(const mxfKey *itemKey) const;
    mxfUMID getUMIDItem(const mxfKey *itemKey) const;
    mxfTimestamp getTimestampItem(const mxfKey *itemKey) const;
    int64_t getLengthItem(const mxfKey *itemKey) const;
    mxfRational getRationalItem(const mxfKey *itemKey) const;
    int64_t getPositionItem(const mxfKey *itemKey) const;
    bool getBooleanItem(const mxfKey *itemKey) const;
    mxfProductVersion getProductVersionItem(const mxfKey *itemKey) const;
    mxfRGBALayout getRGBALayoutItem(const mxfKey *itemKey) const;
    mxfJ2KExtendedCapabilities getJ2KExtendedCapabilitiesItem(const mxfKey *itemKey) const;
    mxfThreeColorPrimaries getThreeColorPrimariesItem(const mxfKey *itemKey) const;
    mxfColorPrimary getColorPrimaryItem(const mxfKey *itemKey) const;
    std::string getStringItem(const mxfKey *itemKey) const;
    std::string getUTF8StringItem(const mxfKey *itemKey) const;
    std::string getISO7StringItem(const mxfKey *itemKey) const;
    MetadataSet* getStrongRefItem(const mxfKey *itemKey) const;
    MetadataSet* getStrongRefItemLight(const mxfKey *itemKey) const;
    MetadataSet* getWeakRefItem(const mxfKey *itemKey) const;
    MetadataSet* getWeakRefItemLight(const mxfKey *itemKey) const;

    std::vector<uint8_t> getUInt8ArrayItem(const mxfKey *itemKey) const;
    std::vector<uint16_t> getUInt16ArrayItem(const mxfKey *itemKey) const;
    std::vector<uint32_t> getUInt32ArrayItem(const mxfKey *itemKey) const;
    std::vector<uint64_t> getUInt64ArrayItem(const mxfKey *itemKey) const;
    std::vector<int8_t> getInt8ArrayItem(const mxfKey *itemKey) const;
    std::vector<int16_t> getInt16ArrayItem(const mxfKey *itemKey) const;
    std::vector<int32_t> getInt32ArrayItem(const mxfKey *itemKey) const;
    std::vector<int64_t> getInt64ArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfVersionType> getVersionTypeArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfUUID> getUUIDArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfUL> getULArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfAUID> getAUIDArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfUMID> getUMIDArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfTimestamp> getTimestampArrayItem(const mxfKey *itemKey) const;
    std::vector<int64_t> getLengthArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfRational> getRationalArrayItem(const mxfKey *itemKey) const;
    std::vector<int64_t> getPositionArrayItem(const mxfKey *itemKey) const;
    std::vector<bool> getBooleanArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfProductVersion> getProductVersionArrayItem(const mxfKey *itemKey) const;
    std::vector<mxfJ2KComponentSizing> getJ2KComponentSizingArrayItem(const mxfKey *itemKey) const;

    ObjectIterator* getStrongRefArrayItem(const mxfKey *itemKey) const;
    ObjectIterator* getWeakRefArrayItem(const mxfKey *itemKey) const;


    void setRawBytesItem(const mxfKey *itemKey, ByteArray value);

    void setUInt8Item(const mxfKey *itemKey, uint8_t value);
    void setUInt16Item(const mxfKey *itemKey, uint16_t value);
    void setUInt32Item(const mxfKey *itemKey, uint32_t value);
    void setUInt64Item(const mxfKey *itemKey, uint64_t value);
    void setInt8Item(const mxfKey *itemKey, int8_t value);
    void setInt16Item(const mxfKey *itemKey, int16_t value);
    void setInt32Item(const mxfKey *itemKey, int32_t value);
    void setInt64Item(const mxfKey *itemKey, int64_t value);
    void setVersionTypeItem(const mxfKey *itemKey, mxfVersionType value);
    void setUUIDItem(const mxfKey *itemKey, mxfUUID value);
    void setULItem(const mxfKey *itemKey, mxfUL value);
    void setAUIDItem(const mxfKey *itemKey, mxfAUID value);
    void setUMIDItem(const mxfKey *itemKey, mxfUMID value);
    void setTimestampItem(const mxfKey *itemKey, mxfTimestamp value);
    void setLengthItem(const mxfKey *itemKey, int64_t value);
    void setRationalItem(const mxfKey *itemKey, mxfRational value);
    void setPositionItem(const mxfKey *itemKey, int64_t value);
    void setBooleanItem(const mxfKey *itemKey, bool value);
    void setProductVersionItem(const mxfKey *itemKey, mxfProductVersion value);
    void setRGBALayoutItem(const mxfKey *itemKey, mxfRGBALayout value);
    void setJ2KExtendedCapabilitiesItem(const mxfKey *itemKey, mxfJ2KExtendedCapabilities value);
    void setThreeColorPrimariesItem(const mxfKey *itemKey, mxfThreeColorPrimaries value) const;
    void setColorPrimaryItem(const mxfKey *itemKey, mxfColorPrimary value) const;
    void setStringItem(const mxfKey *itemKey, std::string value);
    void setFixedSizeStringItem(const mxfKey *itemKey, std::string value, uint16_t size);
    void setUTF8StringItem(const mxfKey *itemKey, std::string value);
    void setISO7StringItem(const mxfKey *itemKey, std::string value);
    void setStrongRefItem(const mxfKey *itemKey, MetadataSet *value);
    void setWeakRefItem(const mxfKey *itemKey, MetadataSet *value);

    void setUInt8ArrayItem(const mxfKey *itemKey, const std::vector<uint8_t> &value);
    void setUInt16ArrayItem(const mxfKey *itemKey, const std::vector<uint16_t> &value);
    void setUInt32ArrayItem(const mxfKey *itemKey, const std::vector<uint32_t> &value);
    void setUInt64ArrayItem(const mxfKey *itemKey, const std::vector<uint64_t> &value);
    void setInt8ArrayItem(const mxfKey *itemKey, const std::vector<int8_t> &value);
    void setInt16ArrayItem(const mxfKey *itemKey, const std::vector<int16_t> &value);
    void setInt32ArrayItem(const mxfKey *itemKey, const std::vector<int32_t> &value);
    void setInt64ArrayItem(const mxfKey *itemKey, const std::vector<int64_t> &value);
    void setVersionTypeArrayItem(const mxfKey *itemKey, const std::vector<mxfVersionType> &value);
    void setUUIDArrayItem(const mxfKey *itemKey, const std::vector<mxfUUID> &value);
    void setULArrayItem(const mxfKey *itemKey, const std::vector<mxfUL> &value);
    void setAUIDArrayItem(const mxfKey *itemKey, const std::vector<mxfAUID> &value);
    void setUMIDArrayItem(const mxfKey *itemKey, const std::vector<mxfUMID> &value);
    void setTimestampArrayItem(const mxfKey *itemKey, const std::vector<mxfTimestamp> &value);
    void setLengthArrayItem(const mxfKey *itemKey, const std::vector<int64_t> &value);
    void setRationalArrayItem(const mxfKey *itemKey, const std::vector<mxfRational> &value);
    void setPositionArrayItem(const mxfKey *itemKey, const std::vector<int64_t> &value);
    void setBooleanArrayItem(const mxfKey *itemKey, const std::vector<bool> &value);
    void setProductVersionArrayItem(const mxfKey *itemKey, const std::vector<mxfProductVersion> &value);
    void setJ2KComponentSizingArrayItem(const mxfKey *itemKey, const std::vector<mxfJ2KComponentSizing> &value);

    void setStrongRefArrayItem(const mxfKey *itemKey, ObjectIterator *iter);
    void setWeakRefArrayItem(const mxfKey *itemKey, ObjectIterator *iter);

    void appendUInt8ArrayItem(const mxfKey *itemKey, uint8_t value);
    void appendUInt16ArrayItem(const mxfKey *itemKey, uint16_t value);
    void appendUInt32ArrayItem(const mxfKey *itemKey, uint32_t value);
    void appendUInt64ArrayItem(const mxfKey *itemKey, uint64_t value);
    void appendInt8ArrayItem(const mxfKey *itemKey, int8_t value);
    void appendInt16ArrayItem(const mxfKey *itemKey, int16_t value);
    void appendInt32ArrayItem(const mxfKey *itemKey, int32_t value);
    void appendInt64ArrayItem(const mxfKey *itemKey, int64_t value);
    void appendVersionTypeArrayItem(const mxfKey *itemKey, mxfVersionType value);
    void appendUUIDArrayItem(const mxfKey *itemKey, mxfUUID value);
    void appendULArrayItem(const mxfKey *itemKey, mxfUL value);
    void appendAUIDArrayItem(const mxfKey *itemKey, mxfAUID value);
    void appendUMIDArrayItem(const mxfKey *itemKey, mxfUMID value);
    void appendTimestampArrayItem(const mxfKey *itemKey, mxfTimestamp value);
    void appendLengthArrayItem(const mxfKey *itemKey, int64_t value);
    void appendRationalArrayItem(const mxfKey *itemKey, mxfRational value);
    void appendPositionArrayItem(const mxfKey *itemKey, int64_t value);
    void appendBooleanArrayItem(const mxfKey *itemKey, bool value);
    void appendProductVersionArrayItem(const mxfKey *itemKey, mxfProductVersion value);
    void appendJ2KComponentSizingArrayItem(const mxfKey *itemKey, mxfJ2KComponentSizing value);

    void appendStrongRefArrayItem(const mxfKey *itemKey, MetadataSet *value);
    void appendWeakRefArrayItem(const mxfKey *itemKey, MetadataSet *value);


    void removeItem(const mxfKey *itemKey);


    MetadataSet* clone(HeaderMetadata *toHeaderMetadata);

    bool validate(bool logErrors);

    // Avid extensions
    void setAvidRGBColor(const mxfKey *itemKey, uint16_t red, uint16_t green, uint16_t blue);
    void setAvidProductVersion(const mxfKey *itemKey, mxfProductVersion value);


    HeaderMetadata* getHeaderMetadata() const { return _headerMetadata; }

    ::MXFMetadataSet* getCMetadataSet() const { return _cMetadataSet; }
    const mxfKey* getKey() const { return &_cMetadataSet->key; }

protected:
    MetadataSet(HeaderMetadata *headerMetadata, ::MXFMetadataSet *metadataSet);

    HeaderMetadata* _headerMetadata;
    ::MXFMetadataSet* _cMetadataSet;
};


class AbsMetadataSetFactory
{
public:
    virtual ~AbsMetadataSetFactory() {};

    virtual MetadataSet* create(HeaderMetadata *headerMetadata, ::MXFMetadataSet *metadataSet) = 0;
};

template<class MetadataSetType>
class MetadataSetFactory : public AbsMetadataSetFactory
{
public:
    virtual MetadataSet* create(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
    {
        return new MetadataSetType(headerMetadata, cMetadataSet);
    }
};


};



#endif

