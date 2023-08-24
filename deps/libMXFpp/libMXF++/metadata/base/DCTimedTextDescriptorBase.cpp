/*
 * Copyright (C) 2018, British Broadcasting Corporation
 * All Rights Reserved.
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


const mxfKey DCTimedTextDescriptorBase::setKey = MXF_SET_K(DCTimedTextDescriptor);


DCTimedTextDescriptorBase::DCTimedTextDescriptorBase(HeaderMetadata *headerMetadata)
: GenericDataEssenceDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

DCTimedTextDescriptorBase::DCTimedTextDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericDataEssenceDescriptor(headerMetadata, cMetadataSet)
{}

DCTimedTextDescriptorBase::~DCTimedTextDescriptorBase()
{}


bool DCTimedTextDescriptorBase::haveResourceID() const
{
    return haveItem(&MXF_ITEM_K(DCTimedTextDescriptor, ResourceID));
}

mxfUUID DCTimedTextDescriptorBase::getResourceID() const
{
    return getUUIDItem(&MXF_ITEM_K(DCTimedTextDescriptor, ResourceID));
}

std::string DCTimedTextDescriptorBase::getNamespaceURI() const
{
    return getStringItem(&MXF_ITEM_K(DCTimedTextDescriptor, NamespaceURI));
}

bool DCTimedTextDescriptorBase::haveRFC5646LanguageTagList() const
{
    return haveItem(&MXF_ITEM_K(DCTimedTextDescriptor, RFC5646LanguageTagList));
}

std::string DCTimedTextDescriptorBase::getRFC5646LanguageTagList() const
{
    return getStringItem(&MXF_ITEM_K(DCTimedTextDescriptor, RFC5646LanguageTagList));
}

std::string DCTimedTextDescriptorBase::getUCSEncoding() const
{
    return getStringItem(&MXF_ITEM_K(DCTimedTextDescriptor, UCSEncoding));
}

void DCTimedTextDescriptorBase::setResourceID(mxfUUID value)
{
    setUUIDItem(&MXF_ITEM_K(DCTimedTextDescriptor, ResourceID), value);
}

void DCTimedTextDescriptorBase::setNamespaceURI(std::string value)
{
    setStringItem(&MXF_ITEM_K(DCTimedTextDescriptor, NamespaceURI), value);
}

void DCTimedTextDescriptorBase::setRFC5646LanguageTagList(std::string value)
{
    setStringItem(&MXF_ITEM_K(DCTimedTextDescriptor, RFC5646LanguageTagList), value);
}

void DCTimedTextDescriptorBase::setUCSEncoding(std::string value)
{
    setStringItem(&MXF_ITEM_K(DCTimedTextDescriptor, UCSEncoding), value);
}
