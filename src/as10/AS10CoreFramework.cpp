/*
 * Copyright (C) 2016, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * AS10 contribution : Andrei Klotchkivski
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

#include <bmx/as10/AS10CoreFramework.h>
#include <bmx/as10/AS10DMS.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


const mxfKey AS10CoreFramework::set_key = MXF_SET_K(AS10CoreFramework);


void AS10CoreFramework::RegisterObjectFactory(HeaderMetadata *header_metadata)
{
    header_metadata->registerObjectFactory(&set_key, new MetadataSetFactory<AS10CoreFramework>());
}

AS10CoreFramework::AS10CoreFramework(HeaderMetadata *header_metadata)
: DMFramework(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);
}

AS10CoreFramework::AS10CoreFramework(HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set)
: DMFramework(header_metadata, c_metadata_set)
{}

AS10CoreFramework::~AS10CoreFramework()
{}

string AS10CoreFramework::GetShimName()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10ShimName));
}

bool AS10CoreFramework::HaveType()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10Type));
}

string AS10CoreFramework::GetType()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10Type));
}

bool AS10CoreFramework::HaveMainTitle()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10MainTitle));
}

string AS10CoreFramework::GetMainTitle()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10MainTitle));
}

bool AS10CoreFramework::HaveSubTitle()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10SubTitle));
}

string AS10CoreFramework::GetSubTitle()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10SubTitle));
}

bool AS10CoreFramework::HaveTitleDescription()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10TitleDescription));
}

string AS10CoreFramework::GetTitleDescription()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10TitleDescription));
}

bool AS10CoreFramework::HaveOrganizationName()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10OrganizationName));
}

string AS10CoreFramework::GetOrganizationName()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10OrganizationName));
}

bool AS10CoreFramework::HavePersonName()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10PersonName));
}

string AS10CoreFramework::GetPersonName()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10PersonName));
}

bool AS10CoreFramework::HaveLocationDescription()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10LocationDescription));
}

string AS10CoreFramework::GetLocationDescription()
{
    return getStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10LocationDescription));
}

bool AS10CoreFramework::HaveSpanningNumber()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10SpanningNumber));
}

uint16_t AS10CoreFramework::GetSpanningNumber()
{
    return getUInt16Item(&MXF_ITEM_K(AS10CoreFramework, AS10SpanningNumber));
}

bool AS10CoreFramework::HaveCommonSpanningID()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10CommonSpanningID));
}

mxfUMID AS10CoreFramework::GetCommonSpanningID()
{
    return getUMIDItem(&MXF_ITEM_K(AS10CoreFramework, AS10CommonSpanningID));
}

bool AS10CoreFramework::HaveCumulativeDuration()
{
    return haveItem(&MXF_ITEM_K(AS10CoreFramework, AS10CumulativeDuration));
}

int64_t AS10CoreFramework::GetCumulativeDuration()
{
    return getPositionItem(&MXF_ITEM_K(AS10CoreFramework, AS10CumulativeDuration));
}

void AS10CoreFramework::SetShimName(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10ShimName), value);
}

void AS10CoreFramework::SetType(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10Type), value);
}

void AS10CoreFramework::SetMainTitle(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10MainTitle), value);
}

void AS10CoreFramework::SetSubTitle(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10SubTitle), value);
}

void AS10CoreFramework::SetTitleDescription(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10TitleDescription), value);
}

void AS10CoreFramework::SetOrganizationName(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10OrganizationName), value);
}

void AS10CoreFramework::SetPersonName(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10PersonName), value);
}

void AS10CoreFramework::SetSpanningNumber(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(AS10CoreFramework, AS10SpanningNumber), value);
}

void AS10CoreFramework::SetLocationDescription(string value)
{
    setStringItem(&MXF_ITEM_K(AS10CoreFramework, AS10LocationDescription), value);
}

void AS10CoreFramework::SetCommonSpanningID(mxfUMID value)
{
    setUMIDItem(&MXF_ITEM_K(AS10CoreFramework, AS10CommonSpanningID), value);
}

void AS10CoreFramework::SetCumulativeDuration(int64_t value)
{
    setPositionItem(&MXF_ITEM_K(AS10CoreFramework, AS10CumulativeDuration), value);
}
