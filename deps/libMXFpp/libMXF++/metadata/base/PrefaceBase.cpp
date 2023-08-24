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


const mxfKey PrefaceBase::setKey = MXF_SET_K(Preface);


PrefaceBase::PrefaceBase(HeaderMetadata *headerMetadata)
: InterchangeObject(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

PrefaceBase::PrefaceBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: InterchangeObject(headerMetadata, cMetadataSet)
{}

PrefaceBase::~PrefaceBase()
{}


mxfTimestamp PrefaceBase::getLastModifiedDate() const
{
    return getTimestampItem(&MXF_ITEM_K(Preface, LastModifiedDate));
}

uint16_t PrefaceBase::getVersion() const
{
    return getVersionTypeItem(&MXF_ITEM_K(Preface, Version));
}

bool PrefaceBase::haveObjectModelVersion() const
{
    return haveItem(&MXF_ITEM_K(Preface, ObjectModelVersion));
}

uint32_t PrefaceBase::getObjectModelVersion() const
{
    return getUInt32Item(&MXF_ITEM_K(Preface, ObjectModelVersion));
}

bool PrefaceBase::havePrimaryPackage() const
{
    return haveItem(&MXF_ITEM_K(Preface, PrimaryPackage));
}

GenericPackage* PrefaceBase::getPrimaryPackage() const
{
    unique_ptr<MetadataSet> obj(getWeakRefItem(&MXF_ITEM_K(Preface, PrimaryPackage)));
    MXFPP_CHECK(dynamic_cast<GenericPackage*>(obj.get()) != 0);
    return dynamic_cast<GenericPackage*>(obj.release());
}

std::vector<Identification*> PrefaceBase::getIdentifications() const
{
    vector<Identification*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(Preface, Identifications)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<Identification*>(iter->get()) != 0);
        result.push_back(dynamic_cast<Identification*>(iter->get()));
    }
    return result;
}

ContentStorage* PrefaceBase::getContentStorage() const
{
    unique_ptr<MetadataSet> obj(getStrongRefItem(&MXF_ITEM_K(Preface, ContentStorage)));
    MXFPP_CHECK(dynamic_cast<ContentStorage*>(obj.get()) != 0);
    return dynamic_cast<ContentStorage*>(obj.release());
}

mxfUL PrefaceBase::getOperationalPattern() const
{
    return getULItem(&MXF_ITEM_K(Preface, OperationalPattern));
}

std::vector<mxfUL> PrefaceBase::getEssenceContainers() const
{
    return getULArrayItem(&MXF_ITEM_K(Preface, EssenceContainers));
}

std::vector<mxfUL> PrefaceBase::getDMSchemes() const
{
    return getULArrayItem(&MXF_ITEM_K(Preface, DMSchemes));
}

bool PrefaceBase::haveIsRIPPresent() const
{
    return haveItem(&MXF_ITEM_K(Preface, IsRIPPresent));
}

bool PrefaceBase::getIsRIPPresent() const
{
    return getBooleanItem(&MXF_ITEM_K(Preface, IsRIPPresent));
}

void PrefaceBase::setLastModifiedDate(mxfTimestamp value)
{
    setTimestampItem(&MXF_ITEM_K(Preface, LastModifiedDate), value);
}

void PrefaceBase::setVersion(uint16_t value)
{
    setVersionTypeItem(&MXF_ITEM_K(Preface, Version), value);
}

void PrefaceBase::setObjectModelVersion(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(Preface, ObjectModelVersion), value);
}

void PrefaceBase::setPrimaryPackage(GenericPackage *value)
{
    setWeakRefItem(&MXF_ITEM_K(Preface, PrimaryPackage), value);
}

void PrefaceBase::setIdentifications(const std::vector<Identification*> &value)
{
    WrapObjectVectorIterator<Identification> iter(value);
    setStrongRefArrayItem(&MXF_ITEM_K(Preface, Identifications), &iter);
}

void PrefaceBase::appendIdentifications(Identification *value)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(Preface, Identifications), value);
}

void PrefaceBase::setContentStorage(ContentStorage *value)
{
    setStrongRefItem(&MXF_ITEM_K(Preface, ContentStorage), value);
}

void PrefaceBase::setOperationalPattern(mxfUL value)
{
    setULItem(&MXF_ITEM_K(Preface, OperationalPattern), value);
}

void PrefaceBase::setEssenceContainers(const std::vector<mxfUL> &value)
{
    setULArrayItem(&MXF_ITEM_K(Preface, EssenceContainers), value);
}

void PrefaceBase::appendEssenceContainers(mxfUL value)
{
    appendULArrayItem(&MXF_ITEM_K(Preface, EssenceContainers), value);
}

void PrefaceBase::setDMSchemes(const std::vector<mxfUL> &value)
{
    setULArrayItem(&MXF_ITEM_K(Preface, DMSchemes), value);
}

void PrefaceBase::appendDMSchemes(mxfUL value)
{
    appendULArrayItem(&MXF_ITEM_K(Preface, DMSchemes), value);
}

void PrefaceBase::setIsRIPPresent(bool value)
{
    setBooleanItem(&MXF_ITEM_K(Preface, IsRIPPresent), value);
}

