/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey AES3AudioDescriptorBase::setKey = MXF_SET_K(AES3AudioDescriptor);


AES3AudioDescriptorBase::AES3AudioDescriptorBase(HeaderMetadata *headerMetadata)
: WaveAudioDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

AES3AudioDescriptorBase::AES3AudioDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: WaveAudioDescriptor(headerMetadata, cMetadataSet)
{}

AES3AudioDescriptorBase::~AES3AudioDescriptorBase()
{}


bool AES3AudioDescriptorBase::haveEmphasis() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, Emphasis));
}

uint8_t AES3AudioDescriptorBase::getEmphasis() const
{
    return getUInt8Item(&MXF_ITEM_K(AES3AudioDescriptor, Emphasis));
}

bool AES3AudioDescriptorBase::haveBlockStartOffset() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, BlockStartOffset));
}

uint16_t AES3AudioDescriptorBase::getBlockStartOffset() const
{
    return getUInt16Item(&MXF_ITEM_K(AES3AudioDescriptor, BlockStartOffset));
}

bool AES3AudioDescriptorBase::haveAuxiliaryBitsMode() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, AuxiliaryBitsMode));
}

uint8_t AES3AudioDescriptorBase::getAuxiliaryBitsMode() const
{
    return getUInt8Item(&MXF_ITEM_K(AES3AudioDescriptor, AuxiliaryBitsMode));
}

bool AES3AudioDescriptorBase::haveChannelStatusMode() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, ChannelStatusMode));
}

vector<uint8_t> AES3AudioDescriptorBase::getChannelStatusMode() const
{
    return getUInt8ArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, ChannelStatusMode));
}

bool AES3AudioDescriptorBase::haveFixedChannelStatusData() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedChannelStatusData));
}

vector<mxfAES3FixedData> AES3AudioDescriptorBase::getFixedChannelStatusData() const
{
    return getAES3FixedDataArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedChannelStatusData));
}

bool AES3AudioDescriptorBase::haveUserDataMode() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, UserDataMode));
}

vector<uint8_t> AES3AudioDescriptorBase::getUserDataMode() const
{
    return getUInt8ArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, UserDataMode));
}

bool AES3AudioDescriptorBase::haveFixedUserData() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedUserData));
}

vector<mxfAES3FixedData> AES3AudioDescriptorBase::getFixedUserData() const
{
    return getAES3FixedDataArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedUserData));
}

bool AES3AudioDescriptorBase::haveLinkedTimecodeTrackID() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, LinkedTimecodeTrackID));
}

uint32_t AES3AudioDescriptorBase::getLinkedTimecodeTrackID() const
{
    return getUInt32Item(&MXF_ITEM_K(AES3AudioDescriptor, LinkedTimecodeTrackID));
}

bool AES3AudioDescriptorBase::haveSMPTE377MDataStreamNumber() const
{
    return haveItem(&MXF_ITEM_K(AES3AudioDescriptor, SMPTE377MDataStreamNumber));
}

uint8_t AES3AudioDescriptorBase::getSMPTE377MDataStreamNumber() const
{
    return getUInt8Item(&MXF_ITEM_K(AES3AudioDescriptor, SMPTE377MDataStreamNumber));
}

void AES3AudioDescriptorBase::setEmphasis(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AES3AudioDescriptor, Emphasis), value);
}

void AES3AudioDescriptorBase::setBlockStartOffset(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(AES3AudioDescriptor, BlockStartOffset), value);
}

void AES3AudioDescriptorBase::setAuxiliaryBitsMode(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AES3AudioDescriptor, AuxiliaryBitsMode), value);
}

void AES3AudioDescriptorBase::setChannelStatusMode(const vector<uint8_t> &value)
{
    setUInt8ArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, ChannelStatusMode), value);
}

void AES3AudioDescriptorBase::appendChannelStatusMode(uint8_t value)
{
    appendUInt8ArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, ChannelStatusMode), value);
}

void AES3AudioDescriptorBase::setFixedChannelStatusData(const vector<mxfAES3FixedData> &value)
{
    setAES3FixedArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedChannelStatusData), value);
}

void AES3AudioDescriptorBase::appendFixedChannelStatusData(const mxfAES3FixedData &value)
{
    appendAES3FixedArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedChannelStatusData), value);
}

void AES3AudioDescriptorBase::setUserDataMode(const vector<uint8_t> &value)
{
    setUInt8ArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, UserDataMode), value);
}

void AES3AudioDescriptorBase::appendUserDataMode(uint8_t value)
{
    appendUInt8ArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, UserDataMode), value);
}

void AES3AudioDescriptorBase::setFixedUserData(const vector<mxfAES3FixedData> &value)
{
    setAES3FixedArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedUserData), value);
}

void AES3AudioDescriptorBase::appendFixedUserData(const mxfAES3FixedData &value)
{
    appendAES3FixedArrayItem(&MXF_ITEM_K(AES3AudioDescriptor, FixedUserData), value);
}

void AES3AudioDescriptorBase::setLinkedTimecodeTrackID(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(AES3AudioDescriptor, LinkedTimecodeTrackID), value);
}

void AES3AudioDescriptorBase::setSMPTE377MDataStreamNumber(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AES3AudioDescriptor, SMPTE377MDataStreamNumber), value);
}

vector<mxfAES3FixedData> AES3AudioDescriptorBase::getAES3FixedDataArrayItem(const mxfKey *itemKey) const
{
    vector<mxfAES3FixedData> result;
    ::MXFArrayItemIterator iter;
    uint8_t *element;
    uint32_t elementLength;
    mxfAES3FixedData value;

    MXFPP_CHECK(mxf_initialise_array_item_iterator(_cMetadataSet, itemKey, &iter));
    while (mxf_next_array_item_element(&iter, &element, &elementLength)) {
        MXFPP_CHECK(elementLength == mxfAES3FixedData_extlen);
        mxf_get_aes3_fixed_data(element, &value);
        result.push_back(value);
    }

    return result;
}

void AES3AudioDescriptorBase::setAES3FixedArrayItem(const mxfKey *itemKey, const vector<mxfAES3FixedData> &value)
{
    uint32_t arraySize = (uint32_t)value.size();
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_alloc_array_item_elements(_cMetadataSet, itemKey, mxfAES3FixedData_extlen, arraySize, &data));

    uint32_t i;
    for (i = 0; i < arraySize; i++) {
        mxf_set_aes3_fixed_data(&value[i], data);
        data += mxfAES3FixedData_extlen;
    }
}

void AES3AudioDescriptorBase::appendAES3FixedArrayItem(const mxfKey *itemKey, const mxfAES3FixedData &value)
{
    uint8_t *data = 0;
    MXFPP_CHECK(mxf_grow_array_item(_cMetadataSet, itemKey, mxfAES3FixedData_extlen, 1, &data));
    mxf_set_aes3_fixed_data(&value, data);
}

