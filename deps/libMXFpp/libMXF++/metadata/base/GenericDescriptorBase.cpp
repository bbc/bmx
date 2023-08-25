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


const mxfKey GenericDescriptorBase::setKey = MXF_SET_K(GenericDescriptor);


GenericDescriptorBase::GenericDescriptorBase(HeaderMetadata *headerMetadata)
: InterchangeObject(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

GenericDescriptorBase::GenericDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: InterchangeObject(headerMetadata, cMetadataSet)
{}

GenericDescriptorBase::~GenericDescriptorBase()
{}


bool GenericDescriptorBase::haveLocators() const
{
    return haveItem(&MXF_ITEM_K(GenericDescriptor, Locators));
}

std::vector<Locator*> GenericDescriptorBase::getLocators() const
{
    vector<Locator*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(GenericDescriptor, Locators)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<Locator*>(iter->get()) != 0);
        result.push_back(dynamic_cast<Locator*>(iter->get()));
    }
    return result;
}

bool GenericDescriptorBase::haveSubDescriptors() const
{
    return haveItem(&MXF_ITEM_K(GenericDescriptor, SubDescriptors));
}

std::vector<SubDescriptor*> GenericDescriptorBase::getSubDescriptors() const
{
    vector<SubDescriptor*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(GenericDescriptor, SubDescriptors)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<SubDescriptor*>(iter->get()) != 0);
        result.push_back(dynamic_cast<SubDescriptor*>(iter->get()));
    }
    return result;
}

void GenericDescriptorBase::setLocators(const std::vector<Locator*> &value)
{
    WrapObjectVectorIterator<Locator> iter(value);
    setStrongRefArrayItem(&MXF_ITEM_K(GenericDescriptor, Locators), &iter);
}

void GenericDescriptorBase::appendLocators(Locator *value)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(GenericDescriptor, Locators), value);
}

void GenericDescriptorBase::setSubDescriptors(const std::vector<SubDescriptor*> &value)
{
    WrapObjectVectorIterator<SubDescriptor> iter(value);
    setStrongRefArrayItem(&MXF_ITEM_K(GenericDescriptor, SubDescriptors), &iter);
}

void GenericDescriptorBase::appendSubDescriptors(SubDescriptor *value)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(GenericDescriptor, SubDescriptors), value);
}

