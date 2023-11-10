/*
 * Copyright (C) 2023, Fraunhofer IIS
 * All Rights Reserved.
 *
 * Author: Nisha Bhaskar
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

const mxfKey JPEGXSSubDescriptorBase::setKey = MXF_SET_K(JPEGXSSubDescriptor);


JPEGXSSubDescriptorBase::JPEGXSSubDescriptorBase(HeaderMetadata *headerMetadata)
	: SubDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
	headerMetadata->add(this);
}

JPEGXSSubDescriptorBase::JPEGXSSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
	: SubDescriptor(headerMetadata, cMetadataSet)
{}

JPEGXSSubDescriptorBase::~JPEGXSSubDescriptorBase()
{}

// getters

uint16_t JPEGXSSubDescriptorBase::getJPEGXSPpih() const
{
	return getUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSPpih));
}

uint16_t JPEGXSSubDescriptorBase::getJPEGXSPlev() const
{
	return getUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSPlev));
}

uint16_t JPEGXSSubDescriptorBase::getJPEGXSWf() const
{
	return getUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSWf));
}

uint16_t JPEGXSSubDescriptorBase::getJPEGXSHf() const
{
	return getUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSHf));
}

uint8_t JPEGXSSubDescriptorBase::getJPEGXSNc() const
{
	return getUInt8Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSNc));
}

uint16_t JPEGXSSubDescriptorBase::getJPEGXSCw() const
{
	return getUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSCw));
}

uint16_t JPEGXSSubDescriptorBase::getJPEGXSHsl() const
{
	return getUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSHsl));
}

uint32_t JPEGXSSubDescriptorBase::getJPEGXSMaximumBitRate() const
{
	return getUInt32Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSMaximumBitRate));
}

std::vector<uint8_t> JPEGXSSubDescriptorBase::getJPEGXSComponentTable() const
{
	return getUInt8ArrayItem(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSComponentTable));
}


// setters


void JPEGXSSubDescriptorBase::setJPEGXSPpih(uint16_t value)
{
	setUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSPpih), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSPlev(uint16_t value)
{
	setUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSPlev), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSWf(uint16_t value)
{
	setUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSWf), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSHf(uint16_t value)
{
	setUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSHf), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSNc(uint8_t value)
{
	setUInt8Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSNc), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSCw(uint16_t value)
{
	setUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSCw), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSHsl(uint16_t value)
{
	setUInt16Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSHsl), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSMaximumBitRate(uint32_t value)
{
	setUInt32Item(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSMaximumBitRate), value);
}

void JPEGXSSubDescriptorBase::setJPEGXSComponentTable(std::vector<uint8_t> value)
{
	setUInt8JxsArray(&MXF_ITEM_K(JPEGXSSubDescriptor, JPEGXSComponentTable), value);
}

