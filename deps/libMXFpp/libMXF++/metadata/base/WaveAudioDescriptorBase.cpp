/*
 * Copyright (C) 2012, British Broadcasting Corporation
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


const mxfKey WaveAudioDescriptorBase::setKey = MXF_SET_K(WaveAudioDescriptor);


WaveAudioDescriptorBase::WaveAudioDescriptorBase(HeaderMetadata *headerMetadata)
: GenericSoundEssenceDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

WaveAudioDescriptorBase::WaveAudioDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericSoundEssenceDescriptor(headerMetadata, cMetadataSet)
{}

WaveAudioDescriptorBase::~WaveAudioDescriptorBase()
{}


uint16_t WaveAudioDescriptorBase::getBlockAlign() const
{
    return getUInt16Item(&MXF_ITEM_K(WaveAudioDescriptor, BlockAlign));
}

bool WaveAudioDescriptorBase::haveSequenceOffset() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, SequenceOffset));
}

uint8_t WaveAudioDescriptorBase::getSequenceOffset() const
{
    return getUInt8Item(&MXF_ITEM_K(WaveAudioDescriptor, SequenceOffset));
}

uint32_t WaveAudioDescriptorBase::getAvgBps() const
{
    return getUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, AvgBps));
}

bool WaveAudioDescriptorBase::haveChannelAssignment() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, ChannelAssignment));
}

mxfUL WaveAudioDescriptorBase::getChannelAssignment() const
{
    return getULItem(&MXF_ITEM_K(WaveAudioDescriptor, ChannelAssignment));
}

bool WaveAudioDescriptorBase::havePeakEnvelopeVersion() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeVersion));
}

uint32_t WaveAudioDescriptorBase::getPeakEnvelopeVersion() const
{
    return getUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeVersion));
}

bool WaveAudioDescriptorBase::havePeakEnvelopeFormat() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeFormat));
}

uint32_t WaveAudioDescriptorBase::getPeakEnvelopeFormat() const
{
    return getUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeFormat));
}

bool WaveAudioDescriptorBase::havePointsPerPeakValue() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PointsPerPeakValue));
}

uint32_t WaveAudioDescriptorBase::getPointsPerPeakValue() const
{
    return getUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PointsPerPeakValue));
}

bool WaveAudioDescriptorBase::havePeakEnvelopeBlockSize() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeBlockSize));
}

uint32_t WaveAudioDescriptorBase::getPeakEnvelopeBlockSize() const
{
    return getUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeBlockSize));
}

bool WaveAudioDescriptorBase::havePeakChannels() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakChannels));
}

uint32_t WaveAudioDescriptorBase::getPeakChannels() const
{
    return getUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakChannels));
}

bool WaveAudioDescriptorBase::havePeakFrames() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakFrames));
}

uint32_t WaveAudioDescriptorBase::getPeakFrames() const
{
    return getUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakFrames));
}

bool WaveAudioDescriptorBase::havePeakOfPeaksPosition() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakOfPeaksPosition));
}

int64_t WaveAudioDescriptorBase::getPeakOfPeaksPosition() const
{
    return getPositionItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakOfPeaksPosition));
}

bool WaveAudioDescriptorBase::havePeakEnvelopeTimestamp() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeTimestamp));
}

mxfTimestamp WaveAudioDescriptorBase::getPeakEnvelopeTimestamp() const
{
    return getTimestampItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeTimestamp));
}

bool WaveAudioDescriptorBase::havePeakEnvelopeData() const
{
    return haveItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeData));
}

ByteArray WaveAudioDescriptorBase::getPeakEnvelopeData() const
{
    return getRawBytesItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeData));
}

void WaveAudioDescriptorBase::setBlockAlign(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(WaveAudioDescriptor, BlockAlign), value);
}

void WaveAudioDescriptorBase::setSequenceOffset(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(WaveAudioDescriptor, SequenceOffset), value);
}

void WaveAudioDescriptorBase::setAvgBps(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, AvgBps), value);
}

void WaveAudioDescriptorBase::setChannelAssignment(mxfUL value)
{
    setULItem(&MXF_ITEM_K(WaveAudioDescriptor, ChannelAssignment), value);
}

void WaveAudioDescriptorBase::setPeakEnvelopeVersion(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeVersion), value);
}

void WaveAudioDescriptorBase::setPeakEnvelopeFormat(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeFormat), value);
}

void WaveAudioDescriptorBase::setPointsPerPeakValue(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PointsPerPeakValue), value);
}

void WaveAudioDescriptorBase::setPeakEnvelopeBlockSize(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeBlockSize), value);
}

void WaveAudioDescriptorBase::setPeakChannels(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakChannels), value);
}

void WaveAudioDescriptorBase::setPeakFrames(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(WaveAudioDescriptor, PeakFrames), value);
}

void WaveAudioDescriptorBase::setPeakOfPeaksPosition(int64_t value)
{
    setPositionItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakOfPeaksPosition), value);
}

void WaveAudioDescriptorBase::setPeakEnvelopeTimestamp(mxfTimestamp value)
{
    setTimestampItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeTimestamp), value);
}

void WaveAudioDescriptorBase::setPeakEnvelopeData(ByteArray value)
{
    setRawBytesItem(&MXF_ITEM_K(WaveAudioDescriptor, PeakEnvelopeData), value);
}

