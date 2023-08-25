/*
 * Copyright (C) 2015, British Broadcasting Corporation
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


const mxfKey VC2SubDescriptorBase::setKey = MXF_SET_K(VC2SubDescriptor);


VC2SubDescriptorBase::VC2SubDescriptorBase(HeaderMetadata *headerMetadata)
: SubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

VC2SubDescriptorBase::VC2SubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: SubDescriptor(headerMetadata, cMetadataSet)
{}

VC2SubDescriptorBase::~VC2SubDescriptorBase()
{}


uint8_t VC2SubDescriptorBase::getVC2MajorVersion() const
{
    return getUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2MajorVersion));
}

uint8_t VC2SubDescriptorBase::getVC2MinorVersion() const
{
    return getUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2MinorVersion));
}

uint8_t VC2SubDescriptorBase::getVC2Profile() const
{
    return getUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2Profile));
}

uint8_t VC2SubDescriptorBase::getVC2Level() const
{
    return getUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2Level));
}

bool VC2SubDescriptorBase::haveVC2WaveletFilters() const
{
    return haveItem(&MXF_ITEM_K(VC2SubDescriptor, VC2WaveletFilters));
}

std::vector<uint8_t> VC2SubDescriptorBase::getVC2WaveletFilters() const
{
    return getUInt8ArrayItem(&MXF_ITEM_K(VC2SubDescriptor, VC2WaveletFilters));
}

bool VC2SubDescriptorBase::haveVC2SequenceHeadersIdentical() const
{
    return haveItem(&MXF_ITEM_K(VC2SubDescriptor, VC2SequenceHeadersIdentical));
}

bool VC2SubDescriptorBase::getVC2SequenceHeadersIdentical() const
{
    return getBooleanItem(&MXF_ITEM_K(VC2SubDescriptor, VC2SequenceHeadersIdentical));
}

bool VC2SubDescriptorBase::haveVC2EditUnitsAreCompleteSequences() const
{
    return haveItem(&MXF_ITEM_K(VC2SubDescriptor, VC2EditUnitsAreCompleteSequences));
}

bool VC2SubDescriptorBase::getVC2EditUnitsAreCompleteSequences() const
{
    return getBooleanItem(&MXF_ITEM_K(VC2SubDescriptor, VC2EditUnitsAreCompleteSequences));
}

void VC2SubDescriptorBase::setVC2MajorVersion(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2MajorVersion), value);
}

void VC2SubDescriptorBase::setVC2MinorVersion(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2MinorVersion), value);
}

void VC2SubDescriptorBase::setVC2Profile(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2Profile), value);
}

void VC2SubDescriptorBase::setVC2Level(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(VC2SubDescriptor, VC2Level), value);
}

void VC2SubDescriptorBase::setVC2WaveletFilters(const std::vector<uint8_t> &value)
{
    setUInt8ArrayItem(&MXF_ITEM_K(VC2SubDescriptor, VC2WaveletFilters), value);
}

void VC2SubDescriptorBase::appendVC2WaveletFilters(uint8_t value)
{
    appendUInt8ArrayItem(&MXF_ITEM_K(VC2SubDescriptor, VC2WaveletFilters), value);
}

void VC2SubDescriptorBase::setVC2SequenceHeadersIdentical(bool value)
{
    setBooleanItem(&MXF_ITEM_K(VC2SubDescriptor, VC2SequenceHeadersIdentical), value);
}

void VC2SubDescriptorBase::setVC2EditUnitsAreCompleteSequences(bool value)
{
    setBooleanItem(&MXF_ITEM_K(VC2SubDescriptor, VC2EditUnitsAreCompleteSequences), value);
}
