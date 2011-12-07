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

#include <bmx/as11/AS11SegmentationFramework.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



const mxfKey AS11SegmentationFramework::set_key = MXF_SET_K(AS11SegmentationFramework);


void AS11SegmentationFramework::RegisterObjectFactory(HeaderMetadata *header_metadata)
{
    header_metadata->registerObjectFactory(&set_key, new MetadataSetFactory<AS11SegmentationFramework>());
}

AS11SegmentationFramework::AS11SegmentationFramework(HeaderMetadata *header_metadata)
: DMFramework(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);
}

AS11SegmentationFramework::AS11SegmentationFramework(HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set)
: DMFramework(header_metadata, c_metadata_set)
{}

AS11SegmentationFramework::~AS11SegmentationFramework()
{}

uint16_t AS11SegmentationFramework::GetPartNumber()
{
    return getUInt16Item(&MXF_ITEM_K(AS11SegmentationFramework, AS11PartNumber));
}

uint16_t AS11SegmentationFramework::GetPartTotal()
{
    return getUInt16Item(&MXF_ITEM_K(AS11SegmentationFramework, AS11PartTotal));
}

void AS11SegmentationFramework::SetPartNumber(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(AS11SegmentationFramework, AS11PartNumber), value);
}

void AS11SegmentationFramework::SetPartTotal(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(AS11SegmentationFramework, AS11PartTotal), value);
}

