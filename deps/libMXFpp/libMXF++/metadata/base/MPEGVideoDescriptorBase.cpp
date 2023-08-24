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


const mxfKey MPEGVideoDescriptorBase::setKey = MXF_SET_K(MPEGVideoDescriptor);


MPEGVideoDescriptorBase::MPEGVideoDescriptorBase(HeaderMetadata *headerMetadata)
: CDCIEssenceDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

MPEGVideoDescriptorBase::MPEGVideoDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: CDCIEssenceDescriptor(headerMetadata, cMetadataSet)
{}

MPEGVideoDescriptorBase::~MPEGVideoDescriptorBase()
{}


bool MPEGVideoDescriptorBase::haveSingleSequence() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, SingleSequence));
}

bool MPEGVideoDescriptorBase::getSingleSequence() const
{
    return getBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, SingleSequence));
}

bool MPEGVideoDescriptorBase::haveConstantBFrames() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, ConstantBFrames));
}

bool MPEGVideoDescriptorBase::getConstantBFrames() const
{
    return getBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, ConstantBFrames));
}

bool MPEGVideoDescriptorBase::haveCodedContentType() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, CodedContentType));
}

uint8_t MPEGVideoDescriptorBase::getCodedContentType() const
{
    return getUInt8Item(&MXF_ITEM_K(MPEGVideoDescriptor, CodedContentType));
}

bool MPEGVideoDescriptorBase::haveLowDelay() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, LowDelay));
}

bool MPEGVideoDescriptorBase::getLowDelay() const
{
    return getBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, LowDelay));
}

bool MPEGVideoDescriptorBase::haveClosedGOP() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, ClosedGOP));
}

bool MPEGVideoDescriptorBase::getClosedGOP() const
{
    return getBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, ClosedGOP));
}

bool MPEGVideoDescriptorBase::haveIdenticalGOP() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, IdenticalGOP));
}

bool MPEGVideoDescriptorBase::getIdenticalGOP() const
{
    return getBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, IdenticalGOP));
}

bool MPEGVideoDescriptorBase::haveMaxGOP() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, MaxGOP));
}

uint16_t MPEGVideoDescriptorBase::getMaxGOP() const
{
    return getUInt16Item(&MXF_ITEM_K(MPEGVideoDescriptor, MaxGOP));
}

bool MPEGVideoDescriptorBase::haveMaxBPictureCount() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, MaxBPictureCount));
}

uint16_t MPEGVideoDescriptorBase::getMaxBPictureCount() const
{
    return getUInt16Item(&MXF_ITEM_K(MPEGVideoDescriptor, MaxBPictureCount));
}

bool MPEGVideoDescriptorBase::haveBitRate() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, BitRate));
}

uint32_t MPEGVideoDescriptorBase::getBitRate() const
{
    return getUInt32Item(&MXF_ITEM_K(MPEGVideoDescriptor, BitRate));
}

bool MPEGVideoDescriptorBase::haveProfileAndLevel() const
{
    return haveItem(&MXF_ITEM_K(MPEGVideoDescriptor, ProfileAndLevel));
}

uint8_t MPEGVideoDescriptorBase::getProfileAndLevel() const
{
    return getUInt8Item(&MXF_ITEM_K(MPEGVideoDescriptor, ProfileAndLevel));
}

void MPEGVideoDescriptorBase::setSingleSequence(bool value)
{
    setBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, SingleSequence), value);
}

void MPEGVideoDescriptorBase::setConstantBFrames(bool value)
{
    setBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, ConstantBFrames), value);
}

void MPEGVideoDescriptorBase::setCodedContentType(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(MPEGVideoDescriptor, CodedContentType), value);
}

void MPEGVideoDescriptorBase::setLowDelay(bool value)
{
    setBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, LowDelay), value);
}

void MPEGVideoDescriptorBase::setClosedGOP(bool value)
{
    setBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, ClosedGOP), value);
}

void MPEGVideoDescriptorBase::setIdenticalGOP(bool value)
{
    setBooleanItem(&MXF_ITEM_K(MPEGVideoDescriptor, IdenticalGOP), value);
}

void MPEGVideoDescriptorBase::setMaxGOP(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(MPEGVideoDescriptor, MaxGOP), value);
}

void MPEGVideoDescriptorBase::setMaxBPictureCount(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(MPEGVideoDescriptor, MaxBPictureCount), value);
}

void MPEGVideoDescriptorBase::setBitRate(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(MPEGVideoDescriptor, BitRate), value);
}

void MPEGVideoDescriptorBase::setProfileAndLevel(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(MPEGVideoDescriptor, ProfileAndLevel), value);
}

