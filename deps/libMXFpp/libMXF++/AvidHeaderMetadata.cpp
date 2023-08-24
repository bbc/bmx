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

#include <mxf/mxf_avid.h>

using namespace std;
using namespace mxfpp;



AvidHeaderMetadata::AvidHeaderMetadata(DataModel *dataModel)
: HeaderMetadata(dataModel)
{
    MXFPP_CHECK(mxf_avid_load_extensions(dataModel->getCDataModel()));
    dataModel->finalise();
}

AvidHeaderMetadata::~AvidHeaderMetadata()
{
}

void AvidHeaderMetadata::createDefaultMetaDictionary()
{
    ::MXFMetadataSet *metaDictSet;
    MXFPP_CHECK(mxf_avid_create_default_metadictionary(getCHeaderMetadata(), &metaDictSet));
}

void AvidHeaderMetadata::createDefaultDictionary(Preface *preface)
{
    ::MXFMetadataSet *dictSet;
    MXFPP_CHECK(mxf_avid_create_default_dictionary(getCHeaderMetadata(), &dictSet));

    MXFPP_CHECK(mxf_set_strongref_item(preface->getCMetadataSet(), &MXF_ITEM_K(Preface, Dictionary), dictSet));
}

void AvidHeaderMetadata::read(File *file, Partition *partition, const mxfKey *key, uint8_t llen, uint64_t len)
{
    MXFPP_CHECK(mxf_avid_read_filtered_header_metadata(file->getCFile(), 0, getCHeaderMetadata(),
                                                       partition->getCPartition()->headerByteCount, key, llen, len));
}

void AvidHeaderMetadata::write(File *file, Partition *partition, FillerWriter *filler)
{
    partition->markHeaderStart(file);

    MXFPP_CHECK(mxf_avid_write_header_metadata(file->getCFile(), getCHeaderMetadata(), partition->getCPartition()));
    if (filler)
    {
        filler->write(file);
    }
    else
    {
        partition->fillToKag(file);
    }

    partition->markHeaderEnd(file);
}

