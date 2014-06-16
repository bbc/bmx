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

#define __STDC_FORMAT_MACROS

#include <libMXF++/MXF.h>

#include <bmx/as11/AS11Info.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/as11/UKDPPDMS.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



void AS11Info::RegisterExtensions(DataModel *data_model)
{
    AS11DMS::RegisterExtensions(data_model);
    UKDPPDMS::RegisterExtensions(data_model);
}

AS11Info::AS11Info()
{
    Reset();
}

AS11Info::~AS11Info()
{
}

bool AS11Info::Read(HeaderMetadata *header_metadata)
{
    Reset();

    MaterialPackage *mp = header_metadata->getPreface()->findMaterialPackage();
    if (!mp) {
        log_warn("No material package found\n");
        return false;
    }
    vector<GenericTrack*> tracks = mp->getTracks();

    AS11CoreFramework::RegisterObjectFactory(header_metadata);
    UKDPPFramework::RegisterObjectFactory(header_metadata);
    AS11SegmentationFramework::RegisterObjectFactory(header_metadata);

    GetStaticFrameworks(tracks);
    GetSegmentation(tracks);

    return core != 0 || ukdpp != 0 || !segmentation.empty();
}

void AS11Info::Reset()
{
    core = 0;
    ukdpp = 0;
    segmentation.clear();
    segmentation_rate = ZERO_RATIONAL;
}

void AS11Info::GetStaticFrameworks(vector<GenericTrack*> &tracks)
{
    // expect to find Static DM Track -> Sequence -> DM Segment -> DM Framework
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        StaticTrack *st = dynamic_cast<StaticTrack*>(tracks[i]);
        if (!st)
            continue;

        StructuralComponent *sc = st->getSequence();
        if (!sc || sc->getDataDefinition() != MXF_DDEF_L(DescriptiveMetadata))
            continue;

        Sequence *seq = dynamic_cast<Sequence*>(sc);
        DMSegment *seg = dynamic_cast<DMSegment*>(sc);
        if (!seq && !seg)
            continue;

        if (seq) {
            vector<StructuralComponent*> scs = seq->getStructuralComponents();
            if (scs.size() != 1)
                continue;

            seg = dynamic_cast<DMSegment*>(scs[0]);
            if (!seg)
                continue;
        }

        if (!seg->haveDMFramework())
            continue;

        DMFramework *framework = seg->getDMFrameworkLight();
        if (framework) {
            AS11CoreFramework *core_maybe = dynamic_cast<AS11CoreFramework*>(framework);
            UKDPPFramework *ukdpp_maybe = dynamic_cast<UKDPPFramework*>(framework);
            if (core_maybe)
                core = core_maybe;
            else if (ukdpp_maybe)
                ukdpp = ukdpp_maybe;
        }
    }
}

void AS11Info::GetSegmentation(vector<GenericTrack*> &tracks)
{
    // expect to find DM Track -> Sequence -> (Filler | DM Segment -> AS11 Segmentation Framework)+
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *tt = dynamic_cast<Track*>(tracks[i]);
        if (!tt)
            continue;

        StructuralComponent *sc = tt->getSequence();
        if (!sc || sc->getDataDefinition() != MXF_DDEF_L(DescriptiveMetadata))
            continue;

        Sequence *seq = dynamic_cast<Sequence*>(sc);
        if (!seq)
            continue;

        segmentation = seq->getStructuralComponents();
        if (segmentation.empty())
            continue;

        size_t j;
        for (j = 0; j < segmentation.size(); j++) {
            DMSegment *seg = dynamic_cast<DMSegment*>(segmentation[j]);
            if ((!seg && *segmentation[j]->getKey() != MXF_SET_K(Filler)) ||
                (seg && !seg->haveDMFramework()) ||
                (seg && !dynamic_cast<AS11SegmentationFramework*>(seg->getDMFrameworkLight())))
            {
                break;
            }
        }
        if (j < segmentation.size()) {
            segmentation.clear();
            continue;
        }

        segmentation_rate = normalize_rate(tt->getEditRate());
        break;
    }
}

