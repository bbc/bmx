/*
 * Copyright (C) 2016, British Broadcasting Corporation
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

#include <bmx/mxf_reader/MXFMCALabelIndex.h>

#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace mxfpp;
using namespace bmx;


MXFMCALabelIndex::MXFMCALabelIndex()
{
}

MXFMCALabelIndex::~MXFMCALabelIndex()
{
}

void MXFMCALabelIndex::RegisterLabel(MCALabelSubDescriptor *label)
{
    SoundfieldGroupLabelSubDescriptor *sg_label = dynamic_cast<SoundfieldGroupLabelSubDescriptor*>(label);
    GroupOfSoundfieldGroupsLabelSubDescriptor *gosg_label = dynamic_cast<GroupOfSoundfieldGroupsLabelSubDescriptor*>(label);
    if (sg_label || gosg_label)
        mLabels[label->getMCALinkID()] = label;
}

void MXFMCALabelIndex::RegisterLabels(const MXFMCALabelIndex *from_index)
{
    map<mxfUUID, MCALabelSubDescriptor*>::const_iterator iter;
    for (iter = from_index->mLabels.begin(); iter != from_index->mLabels.end(); iter++)
        mLabels[iter->first] = iter->second;
}

void MXFMCALabelIndex::CheckReferences(MCALabelSubDescriptor *label)
{
    AudioChannelLabelSubDescriptor *c_label     = dynamic_cast<AudioChannelLabelSubDescriptor*>(label);
    SoundfieldGroupLabelSubDescriptor *sg_label = dynamic_cast<SoundfieldGroupLabelSubDescriptor*>(label);

    if (c_label) {
        if (c_label->haveSoundfieldGroupLinkID()) {
            MCALabelSubDescriptor *ref_label = FindLabel(c_label->getSoundfieldGroupLinkID());
            SoundfieldGroupLabelSubDescriptor *ref_sg_label = dynamic_cast<SoundfieldGroupLabelSubDescriptor*>(ref_label);
            if (!ref_sg_label)
                BMX_EXCEPTION(("Missing MCA soundfield group label '%s'", get_uuid_string(c_label->getSoundfieldGroupLinkID()).c_str()));
            CheckReferences(ref_sg_label);
        }
    } else if (sg_label) {
        if (sg_label->haveGroupOfSoundfieldGroupsLinkID()) {
            vector<mxfUUID> link_ids = sg_label->getGroupOfSoundfieldGroupsLinkID();
            size_t i;
            for (i = 0; i < link_ids.size(); i++) {
                MCALabelSubDescriptor *ref_label = FindLabel(link_ids[i]);
                GroupOfSoundfieldGroupsLabelSubDescriptor *ref_gosg_label = dynamic_cast<GroupOfSoundfieldGroupsLabelSubDescriptor*>(ref_label);
                if (!ref_gosg_label)
                    BMX_EXCEPTION(("Missing MCA group of soundfield groups label '%s'", get_uuid_string(link_ids[i]).c_str()));
            }
        }
    }
}

MCALabelSubDescriptor* MXFMCALabelIndex::FindLabel(mxfUUID link_id) const
{
    if (mLabels.count(link_id))
        return mLabels.at(link_id);
    else
        return 0;
}
