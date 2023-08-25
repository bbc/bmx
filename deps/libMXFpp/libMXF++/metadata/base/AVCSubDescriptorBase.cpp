/*
 * Copyright (C) 2013, British Broadcasting Corporation
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


const mxfKey AVCSubDescriptorBase::setKey = MXF_SET_K(AVCSubDescriptor);


AVCSubDescriptorBase::AVCSubDescriptorBase(HeaderMetadata *headerMetadata)
: SubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

AVCSubDescriptorBase::AVCSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: SubDescriptor(headerMetadata, cMetadataSet)
{}

AVCSubDescriptorBase::~AVCSubDescriptorBase()
{}


uint8_t AVCSubDescriptorBase::getAVCDecodingDelay() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCDecodingDelay));
}

bool AVCSubDescriptorBase::haveAVCConstantBPictureFlag() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCConstantBPictureFlag));
}

bool AVCSubDescriptorBase::getAVCConstantBPictureFlag() const
{
    return getBooleanItem(&MXF_ITEM_K(AVCSubDescriptor, AVCConstantBPictureFlag));
}

bool AVCSubDescriptorBase::haveAVCCodedContentKind() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCCodedContentKind));
}

uint8_t AVCSubDescriptorBase::getAVCCodedContentKind() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCCodedContentKind));
}

bool AVCSubDescriptorBase::haveAVCClosedGOPIndicator() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCClosedGOPIndicator));
}

bool AVCSubDescriptorBase::getAVCClosedGOPIndicator() const
{
    return getBooleanItem(&MXF_ITEM_K(AVCSubDescriptor, AVCClosedGOPIndicator));
}

bool AVCSubDescriptorBase::haveAVCIdenticalGOPIndicator() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCIdenticalGOPIndicator));
}

bool AVCSubDescriptorBase::getAVCIdenticalGOPIndicator() const
{
    return getBooleanItem(&MXF_ITEM_K(AVCSubDescriptor, AVCIdenticalGOPIndicator));
}

bool AVCSubDescriptorBase::haveAVCMaximumGOPSize() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumGOPSize));
}

uint16_t AVCSubDescriptorBase::getAVCMaximumGOPSize() const
{
    return getUInt16Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumGOPSize));
}

bool AVCSubDescriptorBase::haveAVCMaximumBPictureCount() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumBPictureCount));
}

uint16_t AVCSubDescriptorBase::getAVCMaximumBPictureCount() const
{
    return getUInt16Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumBPictureCount));
}

bool AVCSubDescriptorBase::haveAVCMaximumBitrate() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumBitrate));
}

uint32_t AVCSubDescriptorBase::getAVCMaximumBitrate() const
{
    return getUInt32Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumBitrate));
}

bool AVCSubDescriptorBase::haveAVCAverageBitrate() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCAverageBitrate));
}

uint32_t AVCSubDescriptorBase::getAVCAverageBitrate() const
{
    return getUInt32Item(&MXF_ITEM_K(AVCSubDescriptor, AVCAverageBitrate));
}

bool AVCSubDescriptorBase::haveAVCProfile() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCProfile));
}

uint8_t AVCSubDescriptorBase::getAVCProfile() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCProfile));
}

bool AVCSubDescriptorBase::haveAVCProfileConstraint() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCProfileConstraint));
}

uint8_t AVCSubDescriptorBase::getAVCProfileConstraint() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCProfileConstraint));
}

bool AVCSubDescriptorBase::haveAVCLevel() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCLevel));
}

uint8_t AVCSubDescriptorBase::getAVCLevel() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCLevel));
}

bool AVCSubDescriptorBase::haveAVCMaximumRefFrames() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumRefFrames));
}

uint8_t AVCSubDescriptorBase::getAVCMaximumRefFrames() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumRefFrames));
}

bool AVCSubDescriptorBase::haveAVCSequenceParameterSetFlag() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCSequenceParameterSetFlag));
}

uint8_t AVCSubDescriptorBase::getAVCSequenceParameterSetFlag() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCSequenceParameterSetFlag));
}

bool AVCSubDescriptorBase::haveAVCPictureParameterSetFlag() const
{
    return haveItem(&MXF_ITEM_K(AVCSubDescriptor, AVCPictureParameterSetFlag));
}

uint8_t AVCSubDescriptorBase::getAVCPictureParameterSetFlag() const
{
    return getUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCPictureParameterSetFlag));
}

void AVCSubDescriptorBase::setAVCDecodingDelay(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCDecodingDelay), value);
}

void AVCSubDescriptorBase::setAVCConstantBPictureFlag(bool value)
{
    setBooleanItem(&MXF_ITEM_K(AVCSubDescriptor, AVCConstantBPictureFlag), value);
}

void AVCSubDescriptorBase::setAVCCodedContentKind(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCCodedContentKind), value);
}

void AVCSubDescriptorBase::setAVCClosedGOPIndicator(bool value)
{
    setBooleanItem(&MXF_ITEM_K(AVCSubDescriptor, AVCClosedGOPIndicator), value);
}

void AVCSubDescriptorBase::setAVCIdenticalGOPIndicator(bool value)
{
    setBooleanItem(&MXF_ITEM_K(AVCSubDescriptor, AVCIdenticalGOPIndicator), value);
}

void AVCSubDescriptorBase::setAVCMaximumGOPSize(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumGOPSize), value);
}

void AVCSubDescriptorBase::setAVCMaximumBPictureCount(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumBPictureCount), value);
}

void AVCSubDescriptorBase::setAVCMaximumBitrate(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumBitrate), value);
}

void AVCSubDescriptorBase::setAVCAverageBitrate(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(AVCSubDescriptor, AVCAverageBitrate), value);
}

void AVCSubDescriptorBase::setAVCProfile(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCProfile), value);
}

void AVCSubDescriptorBase::setAVCProfileConstraint(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCProfileConstraint), value);
}

void AVCSubDescriptorBase::setAVCLevel(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCLevel), value);
}

void AVCSubDescriptorBase::setAVCMaximumRefFrames(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCMaximumRefFrames), value);
}

void AVCSubDescriptorBase::setAVCSequenceParameterSetFlag(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCSequenceParameterSetFlag), value);
}

void AVCSubDescriptorBase::setAVCPictureParameterSetFlag(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AVCSubDescriptor, AVCPictureParameterSetFlag), value);
}
