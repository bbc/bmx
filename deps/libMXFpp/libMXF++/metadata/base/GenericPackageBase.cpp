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

#include <memory>

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey GenericPackageBase::setKey = MXF_SET_K(GenericPackage);


GenericPackageBase::GenericPackageBase(HeaderMetadata *headerMetadata)
: InterchangeObject(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

GenericPackageBase::GenericPackageBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: InterchangeObject(headerMetadata, cMetadataSet)
{}

GenericPackageBase::~GenericPackageBase()
{}


mxfUMID GenericPackageBase::getPackageUID() const
{
    return getUMIDItem(&MXF_ITEM_K(GenericPackage, PackageUID));
}

bool GenericPackageBase::haveName() const
{
    return haveItem(&MXF_ITEM_K(GenericPackage, Name));
}

std::string GenericPackageBase::getName() const
{
    return getStringItem(&MXF_ITEM_K(GenericPackage, Name));
}

mxfTimestamp GenericPackageBase::getPackageCreationDate() const
{
    return getTimestampItem(&MXF_ITEM_K(GenericPackage, PackageCreationDate));
}

mxfTimestamp GenericPackageBase::getPackageModifiedDate() const
{
    return getTimestampItem(&MXF_ITEM_K(GenericPackage, PackageModifiedDate));
}

std::vector<GenericTrack*> GenericPackageBase::getTracks() const
{
    vector<GenericTrack*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(GenericPackage, Tracks)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<GenericTrack*>(iter->get()) != 0);
        result.push_back(dynamic_cast<GenericTrack*>(iter->get()));
    }
    return result;
}

void GenericPackageBase::setPackageUID(mxfUMID value)
{
    setUMIDItem(&MXF_ITEM_K(GenericPackage, PackageUID), value);
}

void GenericPackageBase::setName(std::string value)
{
    setStringItem(&MXF_ITEM_K(GenericPackage, Name), value);
}

void GenericPackageBase::setPackageCreationDate(mxfTimestamp value)
{
    setTimestampItem(&MXF_ITEM_K(GenericPackage, PackageCreationDate), value);
}

void GenericPackageBase::setPackageModifiedDate(mxfTimestamp value)
{
    setTimestampItem(&MXF_ITEM_K(GenericPackage, PackageModifiedDate), value);
}

void GenericPackageBase::setTracks(const std::vector<GenericTrack*> &value)
{
    WrapObjectVectorIterator<GenericTrack> iter(value);
    setStrongRefArrayItem(&MXF_ITEM_K(GenericPackage, Tracks), &iter);
}

void GenericPackageBase::appendTracks(GenericTrack *value)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(GenericPackage, Tracks), value);
}

