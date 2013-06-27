/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#include <bmx/as11/AS11CoreFramework.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



const mxfKey AS11CoreFramework::set_key = MXF_SET_K(AS11CoreFramework);


void AS11CoreFramework::RegisterObjectFactory(HeaderMetadata *header_metadata)
{
    header_metadata->registerObjectFactory(&set_key, new MetadataSetFactory<AS11CoreFramework>());
}

AS11CoreFramework::AS11CoreFramework(HeaderMetadata *header_metadata)
: DMFramework(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);
}

AS11CoreFramework::AS11CoreFramework(HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set)
: DMFramework(header_metadata, c_metadata_set)
{}

AS11CoreFramework::~AS11CoreFramework()
{}

string AS11CoreFramework::GetSeriesTitle()
{
    return getStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11SeriesTitle));
}

string AS11CoreFramework::GetProgrammeTitle()
{
    return getStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11ProgrammeTitle));
}

string AS11CoreFramework::GetEpisodeTitleNumber()
{
    return getStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11EpisodeTitleNumber));
}

string AS11CoreFramework::GetShimName()
{
    return getStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11ShimName));
}

mxfVersionType AS11CoreFramework::GetShimVersion()
{
    return getVersionTypeItem(&MXF_ITEM_K(AS11CoreFramework, AS11ShimVersion));
}

uint8_t AS11CoreFramework::GetAudioTrackLayout()
{
    return getUInt8Item(&MXF_ITEM_K(AS11CoreFramework, AS11AudioTrackLayout));
}

string AS11CoreFramework::GetPrimaryAudioLanguage()
{
    return getStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11PrimaryAudioLanguage));
}

bool AS11CoreFramework::GetClosedCaptionsPresent()
{
    return getBooleanItem(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsPresent));
}

bool AS11CoreFramework::HaveClosedCaptionsType()
{
    return haveItem(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsType));
}

uint8_t AS11CoreFramework::GetClosedCaptionsType()
{
    return getUInt8Item(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsType));
}

bool AS11CoreFramework::HaveClosedCaptionsLanguage()
{
    return haveItem(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsLanguage));
}

string AS11CoreFramework::GetClosedCaptionsLanguage()
{
    return getStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsLanguage));
}

void AS11CoreFramework::SetSeriesTitle(string value)
{
    setStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11SeriesTitle), value);
}

void AS11CoreFramework::SetProgrammeTitle(string value)
{
    setStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11ProgrammeTitle), value);
}

void AS11CoreFramework::SetEpisodeTitleNumber(string value)
{
    setStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11EpisodeTitleNumber), value);
}

void AS11CoreFramework::SetShimName(string value)
{
    setStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11ShimName), value);
}

void AS11CoreFramework::SetShimVersion(mxfVersionType value)
{
    setVersionTypeItem(&MXF_ITEM_K(AS11CoreFramework, AS11ShimVersion), value);
}

void AS11CoreFramework::SetAudioTrackLayout(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AS11CoreFramework, AS11AudioTrackLayout), value);
}

void AS11CoreFramework::SetPrimaryAudioLanguage(string value)
{
    setStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11PrimaryAudioLanguage), value);
}

void AS11CoreFramework::SetClosedCaptionsPresent(bool value)
{
    setBooleanItem(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsPresent), value);
}

void AS11CoreFramework::SetClosedCaptionsType(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsType), value);
}

void AS11CoreFramework::SetClosedCaptionsLanguage(string value)
{
    setStringItem(&MXF_ITEM_K(AS11CoreFramework, AS11ClosedCaptionsLanguage), value);
}

