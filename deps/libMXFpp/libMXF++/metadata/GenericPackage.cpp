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
#include <libMXF++/extensions/TaggedValue.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace mxfpp;



GenericPackage::GenericPackage(HeaderMetadata *headerMetadata)
: GenericPackageBase(headerMetadata)
{}

GenericPackage::GenericPackage(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: GenericPackageBase(headerMetadata, cMetadataSet)
{}

GenericPackage::~GenericPackage()
{}

vector<TaggedValue*> GenericPackage::getAvidAttributes()
{
    vector<TaggedValue*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(GenericPackage, MobAttributeList)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<TaggedValue*>(iter->get()) != 0);
        result.push_back(dynamic_cast<TaggedValue*>(iter->get()));
    }

    return result;
}

vector<TaggedValue*> GenericPackage::getAvidUserComments()
{
    vector<TaggedValue*> result;
    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(GenericPackage, UserComments)));
    while (iter->next())
    {
        MXFPP_CHECK(dynamic_cast<TaggedValue*>(iter->get()) != 0);
        result.push_back(dynamic_cast<TaggedValue*>(iter->get()));
    }

    return result;
}

TaggedValue* GenericPackage::appendAvidAttribute(string name, string value)
{
    return appendAvidAttribute(new TaggedValue(_headerMetadata, name, value));
}

TaggedValue* GenericPackage::appendAvidAttribute(string name, int32_t value)
{
    return appendAvidAttribute(new TaggedValue(_headerMetadata, name, value));
}

TaggedValue* GenericPackage::appendAvidAttribute(TaggedValue *taggedValue)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(GenericPackage, MobAttributeList), taggedValue);
    return taggedValue;
}

TaggedValue* GenericPackage::appendAvidUserComment(string name, string value)
{
    return appendAvidUserComment(new TaggedValue(_headerMetadata, name, value));
}

TaggedValue* GenericPackage::appendAvidUserComment(TaggedValue *taggedValue)
{
    appendStrongRefArrayItem(&MXF_ITEM_K(GenericPackage, UserComments), taggedValue);
    return taggedValue;
}

GenericTrack* GenericPackage::findTrack(uint32_t trackId) const
{
    vector<GenericTrack*> tracks = getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        if (tracks[i]->haveTrackID() && tracks[i]->getTrackID() == trackId)
            return tracks[i];
    }

    return 0;
}

