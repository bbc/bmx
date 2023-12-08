/*
 * Copyright (C) 2023, British Broadcasting Corporation
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

#include <memory>

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey MGAAudioMetadataSubDescriptorBase::setKey = MXF_SET_K(MGAAudioMetadataSubDescriptor);


MGAAudioMetadataSubDescriptorBase::MGAAudioMetadataSubDescriptorBase(HeaderMetadata *headerMetadata)
: SubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

MGAAudioMetadataSubDescriptorBase::MGAAudioMetadataSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: SubDescriptor(headerMetadata, cMetadataSet)
{}

MGAAudioMetadataSubDescriptorBase::~MGAAudioMetadataSubDescriptorBase()
{}


mxfUUID MGAAudioMetadataSubDescriptorBase::getMGALinkID() const
{
    return getUUIDItem(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGALinkID));
}

uint8_t MGAAudioMetadataSubDescriptorBase::getMGAAudioMetadataIndex() const
{
    return getUInt8Item(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGAAudioMetadataIndex));
}

uint8_t MGAAudioMetadataSubDescriptorBase::getMGAAudioMetadataIdentifier() const
{
    return getUInt8Item(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGAAudioMetadataIdentifier));
}

vector<mxfUL> MGAAudioMetadataSubDescriptorBase::getMGAAudioMetadataPayloadULArray() const
{
    return getULArrayItem(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGAAudioMetadataPayloadULArray));
}

void MGAAudioMetadataSubDescriptorBase::setMGALinkID(mxfUUID value)
{
    setUUIDItem(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGALinkID), value);
}

void MGAAudioMetadataSubDescriptorBase::setMGAAudioMetadataIndex(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGAAudioMetadataIndex), value);
}

void MGAAudioMetadataSubDescriptorBase::setMGAAudioMetadataIdentifier(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGAAudioMetadataIdentifier), value);
}

void MGAAudioMetadataSubDescriptorBase::setMGAAudioMetadataPayloadULArray(const vector<mxfUL> &value)
{
    setULArrayItem(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGAAudioMetadataPayloadULArray), value);
}

void MGAAudioMetadataSubDescriptorBase::appendMGAAudioMetadataPayloadULArray(mxfUL value)
{
    appendULArrayItem(&MXF_ITEM_K(MGAAudioMetadataSubDescriptor, MGAAudioMetadataPayloadULArray), value);
}
