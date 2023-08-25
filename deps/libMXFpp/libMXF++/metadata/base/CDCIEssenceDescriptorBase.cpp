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


const mxfKey CDCIEssenceDescriptorBase::setKey = MXF_SET_K(CDCIEssenceDescriptor);


CDCIEssenceDescriptorBase::CDCIEssenceDescriptorBase(HeaderMetadata *headerMetadata)
: GenericPictureEssenceDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

CDCIEssenceDescriptorBase::CDCIEssenceDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericPictureEssenceDescriptor(headerMetadata, cMetadataSet)
{}

CDCIEssenceDescriptorBase::~CDCIEssenceDescriptorBase()
{}


bool CDCIEssenceDescriptorBase::haveComponentDepth() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth));
}

uint32_t CDCIEssenceDescriptorBase::getComponentDepth() const
{
    return getUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth));
}

bool CDCIEssenceDescriptorBase::haveHorizontalSubsampling() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling));
}

uint32_t CDCIEssenceDescriptorBase::getHorizontalSubsampling() const
{
    return getUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling));
}

bool CDCIEssenceDescriptorBase::haveVerticalSubsampling() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling));
}

uint32_t CDCIEssenceDescriptorBase::getVerticalSubsampling() const
{
    return getUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling));
}

bool CDCIEssenceDescriptorBase::haveColorSiting() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, ColorSiting));
}

uint8_t CDCIEssenceDescriptorBase::getColorSiting() const
{
    return getUInt8Item(&MXF_ITEM_K(CDCIEssenceDescriptor, ColorSiting));
}

bool CDCIEssenceDescriptorBase::haveReversedByteOrder() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, ReversedByteOrder));
}

bool CDCIEssenceDescriptorBase::getReversedByteOrder() const
{
    return getBooleanItem(&MXF_ITEM_K(CDCIEssenceDescriptor, ReversedByteOrder));
}

bool CDCIEssenceDescriptorBase::havePaddingBits() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, PaddingBits));
}

int16_t CDCIEssenceDescriptorBase::getPaddingBits() const
{
    return getInt16Item(&MXF_ITEM_K(CDCIEssenceDescriptor, PaddingBits));
}

bool CDCIEssenceDescriptorBase::haveAlphaSampleDepth() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, AlphaSampleDepth));
}

uint32_t CDCIEssenceDescriptorBase::getAlphaSampleDepth() const
{
    return getUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, AlphaSampleDepth));
}

bool CDCIEssenceDescriptorBase::haveBlackRefLevel() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, BlackRefLevel));
}

uint32_t CDCIEssenceDescriptorBase::getBlackRefLevel() const
{
    return getUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, BlackRefLevel));
}

bool CDCIEssenceDescriptorBase::haveWhiteReflevel() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, WhiteReflevel));
}

uint32_t CDCIEssenceDescriptorBase::getWhiteReflevel() const
{
    return getUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, WhiteReflevel));
}

bool CDCIEssenceDescriptorBase::haveColorRange() const
{
    return haveItem(&MXF_ITEM_K(CDCIEssenceDescriptor, ColorRange));
}

uint32_t CDCIEssenceDescriptorBase::getColorRange() const
{
    return getUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, ColorRange));
}

void CDCIEssenceDescriptorBase::setComponentDepth(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, ComponentDepth), value);
}

void CDCIEssenceDescriptorBase::setHorizontalSubsampling(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, HorizontalSubsampling), value);
}

void CDCIEssenceDescriptorBase::setVerticalSubsampling(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, VerticalSubsampling), value);
}

void CDCIEssenceDescriptorBase::setColorSiting(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(CDCIEssenceDescriptor, ColorSiting), value);
}

void CDCIEssenceDescriptorBase::setReversedByteOrder(bool value)
{
    setBooleanItem(&MXF_ITEM_K(CDCIEssenceDescriptor, ReversedByteOrder), value);
}

void CDCIEssenceDescriptorBase::setPaddingBits(int16_t value)
{
    setInt16Item(&MXF_ITEM_K(CDCIEssenceDescriptor, PaddingBits), value);
}

void CDCIEssenceDescriptorBase::setAlphaSampleDepth(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, AlphaSampleDepth), value);
}

void CDCIEssenceDescriptorBase::setBlackRefLevel(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, BlackRefLevel), value);
}

void CDCIEssenceDescriptorBase::setWhiteReflevel(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, WhiteReflevel), value);
}

void CDCIEssenceDescriptorBase::setColorRange(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(CDCIEssenceDescriptor, ColorRange), value);
}

