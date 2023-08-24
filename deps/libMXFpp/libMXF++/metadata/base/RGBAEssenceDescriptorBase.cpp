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


const mxfKey RGBAEssenceDescriptorBase::setKey = MXF_SET_K(RGBAEssenceDescriptor);


RGBAEssenceDescriptorBase::RGBAEssenceDescriptorBase(HeaderMetadata *headerMetadata)
: GenericPictureEssenceDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

RGBAEssenceDescriptorBase::RGBAEssenceDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericPictureEssenceDescriptor(headerMetadata, cMetadataSet)
{}

RGBAEssenceDescriptorBase::~RGBAEssenceDescriptorBase()
{}


bool RGBAEssenceDescriptorBase::haveComponentMaxRef() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, ComponentMaxRef));
}

uint32_t RGBAEssenceDescriptorBase::getComponentMaxRef() const
{
    return getUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, ComponentMaxRef));
}

bool RGBAEssenceDescriptorBase::haveComponentMinRef() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, ComponentMinRef));
}

uint32_t RGBAEssenceDescriptorBase::getComponentMinRef() const
{
    return getUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, ComponentMinRef));
}

bool RGBAEssenceDescriptorBase::haveAlphaMaxRef() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, AlphaMaxRef));
}

uint32_t RGBAEssenceDescriptorBase::getAlphaMaxRef() const
{
    return getUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, AlphaMaxRef));
}

bool RGBAEssenceDescriptorBase::haveAlphaMinRef() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, AlphaMinRef));
}

uint32_t RGBAEssenceDescriptorBase::getAlphaMinRef() const
{
    return getUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, AlphaMinRef));
}

bool RGBAEssenceDescriptorBase::haveScanningDirection() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, ScanningDirection));
}

uint8_t RGBAEssenceDescriptorBase::getScanningDirection() const
{
    return getUInt8Item(&MXF_ITEM_K(RGBAEssenceDescriptor, ScanningDirection));
}

bool RGBAEssenceDescriptorBase::havePixelLayout() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, PixelLayout));
}

mxfRGBALayout RGBAEssenceDescriptorBase::getPixelLayout() const
{
    return getRGBALayoutItem(&MXF_ITEM_K(RGBAEssenceDescriptor, PixelLayout));
}

bool RGBAEssenceDescriptorBase::havePalette() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, Palette));
}

ByteArray RGBAEssenceDescriptorBase::getPalette() const
{
    return getRawBytesItem(&MXF_ITEM_K(RGBAEssenceDescriptor, Palette));
}

bool RGBAEssenceDescriptorBase::havePaletteLayout() const
{
    return haveItem(&MXF_ITEM_K(RGBAEssenceDescriptor, PaletteLayout));
}

mxfRGBALayout RGBAEssenceDescriptorBase::getPaletteLayout() const
{
    return getRGBALayoutItem(&MXF_ITEM_K(RGBAEssenceDescriptor, PaletteLayout));
}

void RGBAEssenceDescriptorBase::setComponentMaxRef(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, ComponentMaxRef), value);
}

void RGBAEssenceDescriptorBase::setComponentMinRef(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, ComponentMinRef), value);
}

void RGBAEssenceDescriptorBase::setAlphaMaxRef(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, AlphaMaxRef), value);
}

void RGBAEssenceDescriptorBase::setAlphaMinRef(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(RGBAEssenceDescriptor, AlphaMinRef), value);
}

void RGBAEssenceDescriptorBase::setScanningDirection(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(RGBAEssenceDescriptor, ScanningDirection), value);
}

void RGBAEssenceDescriptorBase::setPixelLayout(mxfRGBALayout value)
{
    setRGBALayoutItem(&MXF_ITEM_K(RGBAEssenceDescriptor, PixelLayout), value);
}

void RGBAEssenceDescriptorBase::setPalette(ByteArray value)
{
    setRawBytesItem(&MXF_ITEM_K(RGBAEssenceDescriptor, Palette), value);
}

void RGBAEssenceDescriptorBase::setPaletteLayout(mxfRGBALayout value)
{
    setRGBALayoutItem(&MXF_ITEM_K(RGBAEssenceDescriptor, PaletteLayout), value);
}

