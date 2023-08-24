/*
 * Copyright (C) 2016, British Broadcasting Corporation
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


const mxfKey MCALabelSubDescriptorBase::setKey = MXF_SET_K(MCALabelSubDescriptor);


MCALabelSubDescriptorBase::MCALabelSubDescriptorBase(HeaderMetadata *headerMetadata)
: SubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

MCALabelSubDescriptorBase::MCALabelSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: SubDescriptor(headerMetadata, cMetadataSet)
{}

MCALabelSubDescriptorBase::~MCALabelSubDescriptorBase()
{}


bool MCALabelSubDescriptorBase::haveMCAChannelID() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAChannelID));
}

uint32_t MCALabelSubDescriptorBase::getMCAChannelID() const
{
    return getUInt32Item(&MXF_ITEM_K(MCALabelSubDescriptor, MCAChannelID));
}

mxfUL MCALabelSubDescriptorBase::getMCALabelDictionaryID() const
{
    return getULItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCALabelDictionaryID));
}

std::string MCALabelSubDescriptorBase::getMCATagSymbol() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATagSymbol));
}

bool MCALabelSubDescriptorBase::haveMCATagName() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATagName));
}

std::string MCALabelSubDescriptorBase::getMCATagName() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATagName));
}

mxfUUID MCALabelSubDescriptorBase::getMCALinkID() const
{
    return getUUIDItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCALinkID));
}

bool MCALabelSubDescriptorBase::haveMCAPartitionKind() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAPartitionKind));
}

std::string MCALabelSubDescriptorBase::getMCAPartitionKind() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAPartitionKind));
}

bool MCALabelSubDescriptorBase::haveMCAPartitionNumber() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAPartitionNumber));
}

std::string MCALabelSubDescriptorBase::getMCAPartitionNumber() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAPartitionNumber));
}

bool MCALabelSubDescriptorBase::haveMCATitle() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitle));
}

std::string MCALabelSubDescriptorBase::getMCATitle() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitle));
}

bool MCALabelSubDescriptorBase::haveMCATitleVersion() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitleVersion));
}

std::string MCALabelSubDescriptorBase::getMCATitleVersion() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitleVersion));
}

bool MCALabelSubDescriptorBase::haveMCATitleSubVersion() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitleSubVersion));
}

std::string MCALabelSubDescriptorBase::getMCATitleSubVersion() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitleSubVersion));
}

bool MCALabelSubDescriptorBase::haveMCAEpisode() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAEpisode));
}

std::string MCALabelSubDescriptorBase::getMCAEpisode() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAEpisode));
}

bool MCALabelSubDescriptorBase::haveRFC5646SpokenLanguage() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, RFC5646SpokenLanguage));
}

std::string MCALabelSubDescriptorBase::getRFC5646SpokenLanguage() const
{
    return getISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, RFC5646SpokenLanguage));
}

bool MCALabelSubDescriptorBase::haveMCAAudioContentKind() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAudioContentKind));
}

std::string MCALabelSubDescriptorBase::getMCAAudioContentKind() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAudioContentKind));
}

bool MCALabelSubDescriptorBase::haveMCAAudioElementKind() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAudioElementKind));
}

std::string MCALabelSubDescriptorBase::getMCAAudioElementKind() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAudioElementKind));
}

bool MCALabelSubDescriptorBase::haveMCAContent() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContent));
}

std::string MCALabelSubDescriptorBase::getMCAContent() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContent));
}

bool MCALabelSubDescriptorBase::haveMCAUseClass() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAUseClass));
}

std::string MCALabelSubDescriptorBase::getMCAUseClass() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAUseClass));
}

bool MCALabelSubDescriptorBase::haveMCAContentSubtype() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContentSubtype));
}

std::string MCALabelSubDescriptorBase::getMCAContentSubtype() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContentSubtype));
}

bool MCALabelSubDescriptorBase::haveMCAContentDifferentiator() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContentDifferentiator));
}

std::string MCALabelSubDescriptorBase::getMCAContentDifferentiator() const
{
    return getStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContentDifferentiator));
}

bool MCALabelSubDescriptorBase::haveMCASpokenLanguageAttribute() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCASpokenLanguageAttribute));
}

std::string MCALabelSubDescriptorBase::getMCASpokenLanguageAttribute() const
{
    return getISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCASpokenLanguageAttribute));
}

bool MCALabelSubDescriptorBase::haveRFC5646AdditionalSpokenLanguages() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, RFC5646AdditionalSpokenLanguages));
}

std::string MCALabelSubDescriptorBase::getRFC5646AdditionalSpokenLanguages() const
{
    return getISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, RFC5646AdditionalSpokenLanguages));
}

bool MCALabelSubDescriptorBase::haveMCAAdditionalLanguageAttributes() const
{
    return haveItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAdditionalLanguageAttributes));
}

std::string MCALabelSubDescriptorBase::getMCAAdditionalLanguageAttributes() const
{
    return getISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAdditionalLanguageAttributes));
}


void MCALabelSubDescriptorBase::setMCAChannelID(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(MCALabelSubDescriptor, MCAChannelID), value);
}

void MCALabelSubDescriptorBase::setMCALabelDictionaryID(mxfUL value)
{
    setULItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCALabelDictionaryID), value);
}

void MCALabelSubDescriptorBase::setMCATagSymbol(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATagSymbol), value);
}

void MCALabelSubDescriptorBase::setMCATagName(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATagName), value);
}

void MCALabelSubDescriptorBase::setMCALinkID(mxfUUID value)
{
    setUUIDItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCALinkID), value);
}

void MCALabelSubDescriptorBase::setMCAPartitionKind(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAPartitionKind), value);
}

void MCALabelSubDescriptorBase::setMCAPartitionNumber(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAPartitionNumber), value);
}

void MCALabelSubDescriptorBase::setMCATitle(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitle), value);
}

void MCALabelSubDescriptorBase::setMCATitleVersion(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitleVersion), value);
}

void MCALabelSubDescriptorBase::setMCATitleSubVersion(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCATitleSubVersion), value);
}

void MCALabelSubDescriptorBase::setMCAEpisode(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAEpisode), value);
}

void MCALabelSubDescriptorBase::setRFC5646SpokenLanguage(std::string value)
{
    setISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, RFC5646SpokenLanguage), value);
}

void MCALabelSubDescriptorBase::setMCAAudioContentKind(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAudioContentKind), value);
}

void MCALabelSubDescriptorBase::setMCAAudioElementKind(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAudioElementKind), value);
}

void MCALabelSubDescriptorBase::setMCAContent(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContent), value);
}

void MCALabelSubDescriptorBase::setMCAUseClass(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAUseClass), value);
}

void MCALabelSubDescriptorBase::setMCAContentSubtype(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContentSubtype), value);
}

void MCALabelSubDescriptorBase::setMCAContentDifferentiator(std::string value)
{
    setStringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAContentDifferentiator), value);
}

void MCALabelSubDescriptorBase::setMCASpokenLanguageAttribute(std::string value)
{
    setISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCASpokenLanguageAttribute), value);
}

void MCALabelSubDescriptorBase::setRFC5646AdditionalSpokenLanguages(std::string value)
{
    setISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, RFC5646AdditionalSpokenLanguages), value);
}

void MCALabelSubDescriptorBase::setMCAAdditionalLanguageAttributes(std::string value)
{
    setISO7StringItem(&MXF_ITEM_K(MCALabelSubDescriptor, MCAAdditionalLanguageAttributes), value);
}
