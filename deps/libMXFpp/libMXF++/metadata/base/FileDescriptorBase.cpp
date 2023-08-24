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


const mxfKey FileDescriptorBase::setKey = MXF_SET_K(FileDescriptor);


FileDescriptorBase::FileDescriptorBase(HeaderMetadata *headerMetadata)
: GenericDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

FileDescriptorBase::FileDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericDescriptor(headerMetadata, cMetadataSet)
{}

FileDescriptorBase::~FileDescriptorBase()
{}


bool FileDescriptorBase::haveLinkedTrackID() const
{
    return haveItem(&MXF_ITEM_K(FileDescriptor, LinkedTrackID));
}

uint32_t FileDescriptorBase::getLinkedTrackID() const
{
    return getUInt32Item(&MXF_ITEM_K(FileDescriptor, LinkedTrackID));
}

mxfRational FileDescriptorBase::getSampleRate() const
{
    return getRationalItem(&MXF_ITEM_K(FileDescriptor, SampleRate));
}

bool FileDescriptorBase::haveContainerDuration() const
{
    return haveItem(&MXF_ITEM_K(FileDescriptor, ContainerDuration));
}

int64_t FileDescriptorBase::getContainerDuration() const
{
    return getLengthItem(&MXF_ITEM_K(FileDescriptor, ContainerDuration));
}

mxfUL FileDescriptorBase::getEssenceContainer() const
{
    return getULItem(&MXF_ITEM_K(FileDescriptor, EssenceContainer));
}

bool FileDescriptorBase::haveCodec() const
{
    return haveItem(&MXF_ITEM_K(FileDescriptor, Codec));
}

mxfUL FileDescriptorBase::getCodec() const
{
    return getULItem(&MXF_ITEM_K(FileDescriptor, Codec));
}

void FileDescriptorBase::setLinkedTrackID(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(FileDescriptor, LinkedTrackID), value);
}

void FileDescriptorBase::setSampleRate(mxfRational value)
{
    setRationalItem(&MXF_ITEM_K(FileDescriptor, SampleRate), value);
}

void FileDescriptorBase::setContainerDuration(int64_t value)
{
    setLengthItem(&MXF_ITEM_K(FileDescriptor, ContainerDuration), value);
}

void FileDescriptorBase::setEssenceContainer(mxfUL value)
{
    setULItem(&MXF_ITEM_K(FileDescriptor, EssenceContainer), value);
}

void FileDescriptorBase::setCodec(mxfUL value)
{
    setULItem(&MXF_ITEM_K(FileDescriptor, Codec), value);
}

