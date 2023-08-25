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


const mxfKey GenericSoundEssenceDescriptorBase::setKey = MXF_SET_K(GenericSoundEssenceDescriptor);


GenericSoundEssenceDescriptorBase::GenericSoundEssenceDescriptorBase(HeaderMetadata *headerMetadata)
: FileDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

GenericSoundEssenceDescriptorBase::GenericSoundEssenceDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: FileDescriptor(headerMetadata, cMetadataSet)
{}

GenericSoundEssenceDescriptorBase::~GenericSoundEssenceDescriptorBase()
{}


bool GenericSoundEssenceDescriptorBase::haveAudioSamplingRate() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate));
}

mxfRational GenericSoundEssenceDescriptorBase::getAudioSamplingRate() const
{
    return getRationalItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate));
}

bool GenericSoundEssenceDescriptorBase::haveLocked() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, Locked));
}

bool GenericSoundEssenceDescriptorBase::getLocked() const
{
    return getBooleanItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, Locked));
}

bool GenericSoundEssenceDescriptorBase::haveAudioRefLevel() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioRefLevel));
}

int8_t GenericSoundEssenceDescriptorBase::getAudioRefLevel() const
{
    return getInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioRefLevel));
}

bool GenericSoundEssenceDescriptorBase::haveElectroSpatialFormulation() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ElectroSpatialFormulation));
}

uint8_t GenericSoundEssenceDescriptorBase::getElectroSpatialFormulation() const
{
    return getUInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ElectroSpatialFormulation));
}

bool GenericSoundEssenceDescriptorBase::haveChannelCount() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount));
}

uint32_t GenericSoundEssenceDescriptorBase::getChannelCount() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount));
}

bool GenericSoundEssenceDescriptorBase::haveQuantizationBits() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits));
}

uint32_t GenericSoundEssenceDescriptorBase::getQuantizationBits() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits));
}

bool GenericSoundEssenceDescriptorBase::haveDialNorm() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, DialNorm));
}

int8_t GenericSoundEssenceDescriptorBase::getDialNorm() const
{
    return getInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, DialNorm));
}

bool GenericSoundEssenceDescriptorBase::haveSoundEssenceCompression() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, SoundEssenceCompression));
}

mxfUL GenericSoundEssenceDescriptorBase::getSoundEssenceCompression() const
{
    return getULItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, SoundEssenceCompression));
}

bool GenericSoundEssenceDescriptorBase::haveReferenceImageEditRate() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ReferenceImageEditRate));
}

mxfRational GenericSoundEssenceDescriptorBase::getReferenceImageEditRate() const
{
    return getRationalItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ReferenceImageEditRate));
}

bool GenericSoundEssenceDescriptorBase::haveReferenceAudioAlignmentLevel() const
{
    return haveItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ReferenceAudioAlignmentLevel));
}

int8_t GenericSoundEssenceDescriptorBase::getReferenceAudioAlignmentLevel() const
{
    return getInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ReferenceAudioAlignmentLevel));
}

void GenericSoundEssenceDescriptorBase::setAudioSamplingRate(mxfRational value)
{
    setRationalItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioSamplingRate), value);
}

void GenericSoundEssenceDescriptorBase::setLocked(bool value)
{
    setBooleanItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, Locked), value);
}

void GenericSoundEssenceDescriptorBase::setAudioRefLevel(int8_t value)
{
    setInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, AudioRefLevel), value);
}

void GenericSoundEssenceDescriptorBase::setElectroSpatialFormulation(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ElectroSpatialFormulation), value);
}

void GenericSoundEssenceDescriptorBase::setChannelCount(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ChannelCount), value);
}

void GenericSoundEssenceDescriptorBase::setQuantizationBits(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, QuantizationBits), value);
}

void GenericSoundEssenceDescriptorBase::setDialNorm(int8_t value)
{
    setInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, DialNorm), value);
}

void GenericSoundEssenceDescriptorBase::setSoundEssenceCompression(mxfUL value)
{
    setULItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, SoundEssenceCompression), value);
}

void GenericSoundEssenceDescriptorBase::setReferenceImageEditRate(mxfRational value)
{
    setRationalItem(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ReferenceImageEditRate), value);
}

void GenericSoundEssenceDescriptorBase::setReferenceAudioAlignmentLevel(int8_t value)
{
    setInt8Item(&MXF_ITEM_K(GenericSoundEssenceDescriptor, ReferenceAudioAlignmentLevel), value);
}
