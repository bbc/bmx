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

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey IdentificationBase::setKey = MXF_SET_K(Identification);


IdentificationBase::IdentificationBase(HeaderMetadata *headerMetadata)
: InterchangeObject(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

IdentificationBase::IdentificationBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: InterchangeObject(headerMetadata, cMetadataSet)
{}

IdentificationBase::~IdentificationBase()
{}


mxfUUID IdentificationBase::getThisGenerationUID() const
{
    return getUUIDItem(&MXF_ITEM_K(Identification, ThisGenerationUID));
}

std::string IdentificationBase::getCompanyName() const
{
    return getStringItem(&MXF_ITEM_K(Identification, CompanyName));
}

std::string IdentificationBase::getProductName() const
{
    return getStringItem(&MXF_ITEM_K(Identification, ProductName));
}

bool IdentificationBase::haveProductVersion() const
{
    return haveItem(&MXF_ITEM_K(Identification, ProductVersion));
}

mxfProductVersion IdentificationBase::getProductVersion() const
{
    return getProductVersionItem(&MXF_ITEM_K(Identification, ProductVersion));
}

std::string IdentificationBase::getVersionString() const
{
    return getStringItem(&MXF_ITEM_K(Identification, VersionString));
}

mxfUUID IdentificationBase::getProductUID() const
{
    return getUUIDItem(&MXF_ITEM_K(Identification, ProductUID));
}

mxfTimestamp IdentificationBase::getModificationDate() const
{
    return getTimestampItem(&MXF_ITEM_K(Identification, ModificationDate));
}

bool IdentificationBase::haveToolkitVersion() const
{
    return haveItem(&MXF_ITEM_K(Identification, ToolkitVersion));
}

mxfProductVersion IdentificationBase::getToolkitVersion() const
{
    return getProductVersionItem(&MXF_ITEM_K(Identification, ToolkitVersion));
}

bool IdentificationBase::havePlatform() const
{
    return haveItem(&MXF_ITEM_K(Identification, Platform));
}

std::string IdentificationBase::getPlatform() const
{
    return getStringItem(&MXF_ITEM_K(Identification, Platform));
}

void IdentificationBase::setThisGenerationUID(mxfUUID value)
{
    setUUIDItem(&MXF_ITEM_K(Identification, ThisGenerationUID), value);
}

void IdentificationBase::setCompanyName(std::string value)
{
    setStringItem(&MXF_ITEM_K(Identification, CompanyName), value);
}

void IdentificationBase::setProductName(std::string value)
{
    setStringItem(&MXF_ITEM_K(Identification, ProductName), value);
}

void IdentificationBase::setProductVersion(mxfProductVersion value)
{
    setProductVersionItem(&MXF_ITEM_K(Identification, ProductVersion), value);
}

void IdentificationBase::setVersionString(std::string value)
{
    setStringItem(&MXF_ITEM_K(Identification, VersionString), value);
}

void IdentificationBase::setProductUID(mxfUUID value)
{
    setUUIDItem(&MXF_ITEM_K(Identification, ProductUID), value);
}

void IdentificationBase::setModificationDate(mxfTimestamp value)
{
    setTimestampItem(&MXF_ITEM_K(Identification, ModificationDate), value);
}

void IdentificationBase::setToolkitVersion(mxfProductVersion value)
{
    setProductVersionItem(&MXF_ITEM_K(Identification, ToolkitVersion), value);
}

void IdentificationBase::setPlatform(std::string value)
{
    setStringItem(&MXF_ITEM_K(Identification, Platform), value);
}

