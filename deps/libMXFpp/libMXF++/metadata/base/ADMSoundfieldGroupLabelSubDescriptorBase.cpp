/*
 * Copyright (C) 2022, British Broadcasting Corporation
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


const mxfKey ADMSoundfieldGroupLabelSubDescriptorBase::setKey = MXF_SET_K(ADMSoundfieldGroupLabelSubDescriptor);


ADMSoundfieldGroupLabelSubDescriptorBase::ADMSoundfieldGroupLabelSubDescriptorBase(HeaderMetadata *headerMetadata)
: SoundfieldGroupLabelSubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

ADMSoundfieldGroupLabelSubDescriptorBase::ADMSoundfieldGroupLabelSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: SoundfieldGroupLabelSubDescriptor(headerMetadata, cMetadataSet)
{}

ADMSoundfieldGroupLabelSubDescriptorBase::~ADMSoundfieldGroupLabelSubDescriptorBase()
{}


uint32_t ADMSoundfieldGroupLabelSubDescriptorBase::getRIFFChunkStreamID_link2() const
{
    return getUInt32Item(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, RIFFChunkStreamID_link2));
}

bool ADMSoundfieldGroupLabelSubDescriptorBase::haveADMAudioProgrammeID_ST2131() const
{
    return haveItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioProgrammeID_ST2131));
}

string ADMSoundfieldGroupLabelSubDescriptorBase::getADMAudioProgrammeID_ST2131() const
{
    return getStringItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioProgrammeID_ST2131));
}

bool ADMSoundfieldGroupLabelSubDescriptorBase::haveADMAudioContentID_ST2131() const
{
    return haveItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioContentID_ST2131));
}

string ADMSoundfieldGroupLabelSubDescriptorBase::getADMAudioContentID_ST2131() const
{
    return getStringItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioContentID_ST2131));
}

bool ADMSoundfieldGroupLabelSubDescriptorBase::haveADMAudioObjectID_ST2131() const
{
    return haveItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioObjectID_ST2131));
}

string ADMSoundfieldGroupLabelSubDescriptorBase::getADMAudioObjectID_ST2131() const
{
    return getStringItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioObjectID_ST2131));
}

void ADMSoundfieldGroupLabelSubDescriptorBase::setRIFFChunkStreamID_link2(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, RIFFChunkStreamID_link2), value);
}

void ADMSoundfieldGroupLabelSubDescriptorBase::setADMAudioProgrammeID_ST2131(const string &value)
{
    setStringItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioProgrammeID_ST2131), value);
}

void ADMSoundfieldGroupLabelSubDescriptorBase::setADMAudioContentID_ST2131(const string &value)
{
    setStringItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioContentID_ST2131), value);
}

void ADMSoundfieldGroupLabelSubDescriptorBase::setADMAudioObjectID_ST2131(const string &value)
{
    setStringItem(&MXF_ITEM_K(ADMSoundfieldGroupLabelSubDescriptor, ADMAudioObjectID_ST2131), value);
}
