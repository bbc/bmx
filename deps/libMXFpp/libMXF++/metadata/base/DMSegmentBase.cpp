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

#include <memory>

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey DMSegmentBase::setKey = MXF_SET_K(DMSegment);


DMSegmentBase::DMSegmentBase(HeaderMetadata *headerMetadata)
: StructuralComponent(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

DMSegmentBase::DMSegmentBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: StructuralComponent(headerMetadata, cMetadataSet)
{}

DMSegmentBase::~DMSegmentBase()
{}


int64_t DMSegmentBase::getEventStartPosition() const
{
    return getPositionItem(&MXF_ITEM_K(DMSegment, EventStartPosition));
}

bool DMSegmentBase::haveEventComment() const
{
    return haveItem(&MXF_ITEM_K(DMSegment, EventComment));
}

std::string DMSegmentBase::getEventComment() const
{
    return getStringItem(&MXF_ITEM_K(DMSegment, EventComment));
}

bool DMSegmentBase::haveTrackIDs() const
{
    return haveItem(&MXF_ITEM_K(DMSegment, TrackIDs));
}

std::vector<uint32_t> DMSegmentBase::getTrackIDs() const
{
    return getUInt32ArrayItem(&MXF_ITEM_K(DMSegment, TrackIDs));
}

bool DMSegmentBase::haveDMFramework() const
{
    return haveItem(&MXF_ITEM_K(DMSegment, DMFramework));
}

DMFramework* DMSegmentBase::getDMFramework() const
{
    unique_ptr<MetadataSet> obj(getStrongRefItem(&MXF_ITEM_K(DMSegment, DMFramework)));
    MXFPP_CHECK(dynamic_cast<DMFramework*>(obj.get()) != 0);
    return dynamic_cast<DMFramework*>(obj.release());
}

void DMSegmentBase::setEventStartPosition(int64_t value)
{
    setPositionItem(&MXF_ITEM_K(DMSegment, EventStartPosition), value);
}

void DMSegmentBase::setEventComment(std::string value)
{
    setStringItem(&MXF_ITEM_K(DMSegment, EventComment), value);
}

void DMSegmentBase::setTrackIDs(const std::vector<uint32_t> &value)
{
    setUInt32ArrayItem(&MXF_ITEM_K(DMSegment, TrackIDs), value);
}

void DMSegmentBase::appendTrackIDs(uint32_t value)
{
    appendUInt32ArrayItem(&MXF_ITEM_K(DMSegment, TrackIDs), value);
}

void DMSegmentBase::setDMFramework(DMFramework *value)
{
    setStrongRefItem(&MXF_ITEM_K(DMSegment, DMFramework), value);
}

