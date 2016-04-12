/*
 * Copyright (C) 2016, British Broadcasting Corporation
 * All Rights Reserved.
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

#define __STDC_FORMAT_MACROS

#include <bmx/as10/AS10WriterHelper.h>
#include <bmx/as10/AS10DMS.h>
#include <bmx/as10/AS10Info.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const char *AS10_CORE_TRACK_NAME = "AS_10_Core";


AS10WriterHelper::AS10WriterHelper(ClipWriter *clip)
{
    mClip = clip;

    AS10Info::RegisterExtensions(clip->GetHeaderMetadata());

    UniqueIdHelper *track_id_helper = clip->GetTrackIdHelper();
    BMX_ASSERT(track_id_helper);
    track_id_helper->SetId(AS10_CORE_TRACK_NAME, 6001);
}

AS10WriterHelper::~AS10WriterHelper()
{
}

void AS10WriterHelper::InsertAS10CoreFramework(AS10CoreFramework *framework)
{
    AppendDMSLabel(MXF_DM_L(AS10CoreDescriptiveScheme));
    InsertFramework(mClip->GetTrackIdHelper()->GetId(AS10_CORE_TRACK_NAME), AS10_CORE_TRACK_NAME, framework);
}

void AS10WriterHelper::AppendDMSLabel(mxfUL scheme_label)
{
    HeaderMetadata *header_metadata = mClip->GetHeaderMetadata();
    BMX_ASSERT(header_metadata);

    vector<mxfUL> dm_schemes = header_metadata->getPreface()->getDMSchemes();
    size_t i;
    for (i = 0; i < dm_schemes.size(); i++) {
        if (mxf_equals_ul_mod_regver(&dm_schemes[i], &scheme_label))
            break;
    }
    if (i >= dm_schemes.size())
        header_metadata->getPreface()->appendDMSchemes(scheme_label);
}

void AS10WriterHelper::InsertFramework(uint32_t track_id, string track_name, DMFramework *framework)
{
    HeaderMetadata *header_metadata = mClip->GetHeaderMetadata();
    BMX_ASSERT(header_metadata);

    MaterialPackage *material_package = header_metadata->getPreface()->findMaterialPackage();
    BMX_ASSERT(material_package);

    // Preface - ContentStorage - Package - DM Track
    StaticTrack *dm_track = new StaticTrack(header_metadata);
    material_package->appendTracks(dm_track);
    dm_track->setTrackName(track_name);
    dm_track->setTrackID(track_id);
    dm_track->setTrackNumber(0);

    // Preface - ContentStorage - Package - DM Track - Sequence
    Sequence *sequence = new Sequence(header_metadata);
    dm_track->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));

    // Preface - ContentStorage - Package - DM Track - Sequence - DMSegment
    DMSegment *dm_segment = new DMSegment(header_metadata);
    sequence->appendStructuralComponents(dm_segment);
    dm_segment->setDataDefinition(MXF_DDEF_L(DescriptiveMetadata));

    // move the framework set after the dm degment set
    mxf_remove_set(header_metadata->getCHeaderMetadata(), framework->getCMetadataSet());
    BMX_CHECK(mxf_add_set(header_metadata->getCHeaderMetadata(), framework->getCMetadataSet()));

    // Preface - ContentStorage - Package - DM Track - Sequence - DMSegment - DMFramework
    dm_segment->setDMFramework(framework);
}

