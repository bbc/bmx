/*
 * Copyright (C) 2022, British Broadcasting Corporation
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

#include <string.h>
#include <vector>

#include <bmx/mxf_helper/ADMCHNAMXFDescriptorHelper.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


WaveCHNA* bmx::convert_adm_chna_descriptor_to_chunk(const ADM_CHNASubDescriptor *descriptor)
{
    WaveCHNA *chunk = new WaveCHNA();

    vector<ADMChannelMapping*> channel_mappings = descriptor->getADMChannelMappingsArray();
    size_t i;
    for (i = 0; i < channel_mappings.size(); i++) {
        ADMChannelMapping *mapping = channel_mappings[i];

        WaveCHNA::AudioID audio_id;
        audio_id.track_index = mapping->getLocalChannelID();

        string uid_str = mapping->getADMAudioTrackUID();
        uid_str.resize(sizeof(audio_id.uid));
        memcpy(audio_id.uid, uid_str.c_str(), sizeof(audio_id.uid));

        string track_ref_str = mapping->getADMAudioTrackChannelFormatID();
        track_ref_str.resize(sizeof(audio_id.track_ref));
        memcpy(audio_id.track_ref, track_ref_str.c_str(), sizeof(audio_id.track_ref));

        if (mapping->haveADMAudioPackFormatID()) {
            string pack_ref_str = mapping->getADMAudioPackFormatID();
            pack_ref_str.resize(sizeof(audio_id.pack_ref));
            memcpy(audio_id.pack_ref, pack_ref_str.c_str(), sizeof(audio_id.pack_ref));
        } else {
            memset(audio_id.pack_ref, 0, sizeof(audio_id.pack_ref));
        }

        chunk->AppendAudioID(audio_id);
    }

    return chunk;
}

ADM_CHNASubDescriptor* bmx::convert_chunk_to_adm_chna_descriptor(HeaderMetadata *header_metadata, const WaveCHNA *chunk)
{
    ADM_CHNASubDescriptor *descriptor = new ADM_CHNASubDescriptor(header_metadata);
    descriptor->setNumLocalChannels(chunk->GetNumTracks());
    descriptor->setNumADMAudioTrackUIDs(chunk->GetNumUIDs());

    const vector<WaveCHNA::AudioID> &audio_ids = chunk->GetAudioIDs();
    size_t i;
    for (i = 0; i < audio_ids.size(); i++) {
        const WaveCHNA::AudioID &audio_id = audio_ids[i];

        // The descriptor shall have no "null" or "unused" mappings
        if (audio_id.track_index == 0)
            continue;

        ADMChannelMapping *mapping = new ADMChannelMapping(header_metadata);
        mapping->setLocalChannelID(audio_id.track_index);
        mapping->setADMAudioTrackUID(string((const char*)audio_id.uid, sizeof(audio_id.uid)));
        mapping->setADMAudioTrackChannelFormatID(string((const char*)audio_id.track_ref, sizeof(audio_id.track_ref)));

        string pack_ref_str = string((const char*)audio_id.pack_ref, sizeof(audio_id.pack_ref));
        if (!pack_ref_str.empty())
            mapping->setADMAudioPackFormatID(pack_ref_str);

        descriptor->appendADMChannelMappingsArray(mapping);
    }

    return descriptor;
}
