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

#include <map>

#include <bmx/mxf_helper/MXFMCALabelHelper.h>
#include <bmx/BMXTypes.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


bool bmx::check_mca_labels(const vector<MCALabelSubDescriptor*> &mca_labels)
{
    map<UUID, pair<bool, bool> > soundfield_group_ids;
    map<UUID, pair<bool, bool> > group_of_sf_group_ids;
    size_t i;
    for (i = 0; i < mca_labels.size(); i++) {
        const MCALabelSubDescriptor *desc = mca_labels[i];

        const AudioChannelLabelSubDescriptor *c_desc             = dynamic_cast<const AudioChannelLabelSubDescriptor*>(desc);
        const SoundfieldGroupLabelSubDescriptor *g_desc          = dynamic_cast<const SoundfieldGroupLabelSubDescriptor*>(desc);
        const GroupOfSoundfieldGroupsLabelSubDescriptor *gg_desc = dynamic_cast<const GroupOfSoundfieldGroupsLabelSubDescriptor*>(desc);
        if (c_desc) {
            if (c_desc->haveSoundfieldGroupLinkID()) {
                UUID id = c_desc->getSoundfieldGroupLinkID();
                if (soundfield_group_ids.count(id))
                    soundfield_group_ids[id].second = true;
                else
                    soundfield_group_ids[id] = make_pair(false, true);
            }
        } else if (g_desc) {
            UUID id = g_desc->getMCALinkID();
            if (soundfield_group_ids.count(id))
                soundfield_group_ids[id].first = true;
            else
                soundfield_group_ids[id] = make_pair(true, false);
            if (g_desc->haveGroupOfSoundfieldGroupsLinkID()) {
                vector<UUID> ids = g_desc->getGroupOfSoundfieldGroupsLinkID();
                size_t l;
                for (l = 0; l < ids.size(); l++) {
                    const UUID &gg_id = ids[l];
                    if (group_of_sf_group_ids.count(gg_id))
                        group_of_sf_group_ids[gg_id].second = true;
                    else
                        group_of_sf_group_ids[gg_id] = make_pair(false, true);
                }
            }
        } else if (gg_desc) {
            UUID id = gg_desc->getMCALinkID();
            if (group_of_sf_group_ids.count(id))
                group_of_sf_group_ids[id].first = true;
            else
                group_of_sf_group_ids[id] = make_pair(true, false);
        }
    }

    map<UUID, pair<bool, bool> >::const_iterator iter;
    for (iter = soundfield_group_ids.begin(); iter != soundfield_group_ids.end(); iter++) {
        if (!iter->second.first) {
            log_error("Soundfield group descriptor with id '%s' is missing\n",
                       get_uuid_string(iter->first).c_str());
            return false;
        }
        if (iter->second.first && !iter->second.second) {
            log_warn("Soundfield group descriptor with id '%s' is not referenced\n",
                     get_uuid_string(iter->first).c_str());
        }
    }
    for (iter = group_of_sf_group_ids.begin(); iter != group_of_sf_group_ids.end(); iter++) {
        if (!iter->second.first) {
            log_error("Group of soundfield group descriptor with id '%s' is missing",
                      get_uuid_string(iter->first).c_str());
            return false;
        }
        if (iter->second.first && !iter->second.second) {
            log_warn("Group of soundfield group descriptor with id '%s' is not referenced\n",
                     get_uuid_string(iter->first).c_str());
        }
    }

    return true;
}
