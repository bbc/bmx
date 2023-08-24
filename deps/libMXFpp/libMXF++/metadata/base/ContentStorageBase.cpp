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


const mxfKey ContentStorageBase::setKey = MXF_SET_K(ContentStorage);


ContentStorageBase::ContentStorageBase(HeaderMetadata *headerMetadata)
: InterchangeObject(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

ContentStorageBase::ContentStorageBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: InterchangeObject(headerMetadata, cMetadataSet)
{}

ContentStorageBase::~ContentStorageBase()
{}


std::vector<GenericPackage*> ContentStorageBase::getPackages() const
{
    vector<GenericPackage*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(ContentStorage, Packages)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<GenericPackage*>(iter->get()) != 0);
        result.push_back(dynamic_cast<GenericPackage*>(iter->get()));
    }
    return result;
}

bool ContentStorageBase::haveEssenceContainerData() const
{
    return haveItem(&MXF_ITEM_K(ContentStorage, EssenceContainerData));
}

std::vector<EssenceContainerData*> ContentStorageBase::getEssenceContainerData() const
{
    vector<EssenceContainerData*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(ContentStorage, EssenceContainerData)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<EssenceContainerData*>(iter->get()) != 0);
        result.push_back(dynamic_cast<EssenceContainerData*>(iter->get()));
    }
    return result;
}

void ContentStorageBase::setPackages(const std::vector<GenericPackage*> &value)
{
    WrapObjectVectorIterator<GenericPackage> iter(value);
    setStrongRefArrayItem(&MXF_ITEM_K(ContentStorage, Packages), &iter);
}

void ContentStorageBase::appendPackages(GenericPackage *value)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(ContentStorage, Packages), value);
}

void ContentStorageBase::setEssenceContainerData(const std::vector<EssenceContainerData*> &value)
{
    WrapObjectVectorIterator<EssenceContainerData> iter(value);
    setStrongRefArrayItem(&MXF_ITEM_K(ContentStorage, EssenceContainerData), &iter);
}

void ContentStorageBase::appendEssenceContainerData(EssenceContainerData *value)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(ContentStorage, EssenceContainerData), value);
}

