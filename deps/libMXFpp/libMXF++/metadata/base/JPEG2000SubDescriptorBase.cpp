/*
 * Copyright (C) 2020, British Broadcasting Corporation
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


const mxfKey JPEG2000SubDescriptorBase::setKey = MXF_SET_K(JPEG2000SubDescriptor);


JPEG2000SubDescriptorBase::JPEG2000SubDescriptorBase(HeaderMetadata *headerMetadata)
: SubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

JPEG2000SubDescriptorBase::JPEG2000SubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: SubDescriptor(headerMetadata, cMetadataSet)
{}

JPEG2000SubDescriptorBase::~JPEG2000SubDescriptorBase()
{}


uint16_t JPEG2000SubDescriptorBase::getRsiz() const
{
    return getUInt16Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Rsiz));
}

uint32_t JPEG2000SubDescriptorBase::getXsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Xsiz));
}

uint32_t JPEG2000SubDescriptorBase::getYsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Ysiz));
}

uint32_t JPEG2000SubDescriptorBase::getXOsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, XOsiz));
}

uint32_t JPEG2000SubDescriptorBase::getYOsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, YOsiz));
}

uint32_t JPEG2000SubDescriptorBase::getXTsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, XTsiz));
}

uint32_t JPEG2000SubDescriptorBase::getYTsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, YTsiz));
}

uint32_t JPEG2000SubDescriptorBase::getXTOsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, XTOsiz));
}

uint32_t JPEG2000SubDescriptorBase::getYTOsiz() const
{
    return getUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, YTOsiz));
}

uint16_t JPEG2000SubDescriptorBase::getCsiz() const
{
    return getUInt16Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Csiz));
}

vector<mxfJ2KComponentSizing> JPEG2000SubDescriptorBase::getPictureComponentSizing() const
{
    return getJ2KComponentSizingArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, PictureComponentSizing));
}

bool JPEG2000SubDescriptorBase::haveCodingStyleDefault() const
{
    return haveItem(&MXF_ITEM_K(JPEG2000SubDescriptor, CodingStyleDefault));
}

ByteArray JPEG2000SubDescriptorBase::getCodingStyleDefault() const
{
    return getRawBytesItem(&MXF_ITEM_K(JPEG2000SubDescriptor, CodingStyleDefault));
}

bool JPEG2000SubDescriptorBase::haveQuantizationDefault() const
{
    return haveItem(&MXF_ITEM_K(JPEG2000SubDescriptor, QuantizationDefault));
}

ByteArray JPEG2000SubDescriptorBase::getQuantizationDefault() const
{
    return getRawBytesItem(&MXF_ITEM_K(JPEG2000SubDescriptor, QuantizationDefault));
}

bool JPEG2000SubDescriptorBase::haveJ2CLayout() const
{
    return haveItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2CLayout));
}

mxfRGBALayout JPEG2000SubDescriptorBase::getJ2CLayout() const
{
    return getRGBALayoutItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2CLayout));
}

bool JPEG2000SubDescriptorBase::haveJ2KExtendedCapabilities() const
{
    return haveItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KExtendedCapabilities));
}

mxfJ2KExtendedCapabilities JPEG2000SubDescriptorBase::getJ2KExtendedCapabilities() const
{
    return getJ2KExtendedCapabilitiesItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KExtendedCapabilities));
}

bool JPEG2000SubDescriptorBase::haveJ2KProfile() const
{
    return haveItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KProfile));
}

vector<uint16_t> JPEG2000SubDescriptorBase::getJ2KProfile() const
{
    return getUInt16ArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KProfile));
}

bool JPEG2000SubDescriptorBase::haveJ2KCorrespondingProfile() const
{
    return haveItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KCorrespondingProfile));
}

vector<uint16_t> JPEG2000SubDescriptorBase::getJ2KCorrespondingProfile() const
{
    return getUInt16ArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KCorrespondingProfile));
}

void JPEG2000SubDescriptorBase::setRsiz(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Rsiz), value);
}

void JPEG2000SubDescriptorBase::setXsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Xsiz), value);
}

void JPEG2000SubDescriptorBase::setYsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Ysiz), value);
}

void JPEG2000SubDescriptorBase::setXOsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, XOsiz), value);
}

void JPEG2000SubDescriptorBase::setYOsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, YOsiz), value);
}

void JPEG2000SubDescriptorBase::setXTsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, XTsiz), value);
}

void JPEG2000SubDescriptorBase::setYTsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, YTsiz), value);
}

void JPEG2000SubDescriptorBase::setXTOsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, XTOsiz), value);
}

void JPEG2000SubDescriptorBase::setYTOsiz(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(JPEG2000SubDescriptor, YTOsiz), value);
}

void JPEG2000SubDescriptorBase::setCsiz(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(JPEG2000SubDescriptor, Csiz), value);
}

void JPEG2000SubDescriptorBase::setPictureComponentSizing(const vector<mxfJ2KComponentSizing> &value)
{
    setJ2KComponentSizingArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, PictureComponentSizing), value);
}

void JPEG2000SubDescriptorBase::appendPictureComponentSizing(mxfJ2KComponentSizing value)
{
    appendJ2KComponentSizingArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, PictureComponentSizing), value);
}

void JPEG2000SubDescriptorBase::setCodingStyleDefault(ByteArray value)
{
    setRawBytesItem(&MXF_ITEM_K(JPEG2000SubDescriptor, CodingStyleDefault), value);
}

void JPEG2000SubDescriptorBase::setQuantizationDefault(ByteArray value)
{
    setRawBytesItem(&MXF_ITEM_K(JPEG2000SubDescriptor, QuantizationDefault), value);
}

void JPEG2000SubDescriptorBase::setJ2CLayout(mxfRGBALayout value)
{
    setRGBALayoutItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2CLayout), value);
}

void JPEG2000SubDescriptorBase::setJ2KExtendedCapabilities(mxfJ2KExtendedCapabilities value)
{
    setJ2KExtendedCapabilitiesItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KExtendedCapabilities), value);
}

void JPEG2000SubDescriptorBase::setJ2KProfile(const vector<uint16_t> &value)
{
    setUInt16ArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KProfile), value);
}

void JPEG2000SubDescriptorBase::appendJ2KProfile(uint16_t value)
{
    appendUInt16ArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KProfile), value);
}

void JPEG2000SubDescriptorBase::setJ2KCorrespondingProfile(const vector<uint16_t> &value)
{
    setUInt16ArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KCorrespondingProfile), value);
}

void JPEG2000SubDescriptorBase::appendJ2KCorrespondingProfile(uint16_t value)
{
    appendUInt16ArrayItem(&MXF_ITEM_K(JPEG2000SubDescriptor, J2KCorrespondingProfile), value);
}
