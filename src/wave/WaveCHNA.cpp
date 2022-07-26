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
#include <set>

#include <bmx/wave/WaveCHNA.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


WaveCHNA::WaveCHNA()
{
    mNumTracks = 0;
    mNumUIDs = 0;
}

WaveCHNA::~WaveCHNA()
{

}

void WaveCHNA::AppendAudioID(const AudioID &audio_id)
{
    // Recalculate num tracks and UIDs and check the new audio_id isn't a duplicate track_index + UID
    uint16_t num_uids = 0;
    set<uint16_t> track_indexes;
    size_t i;
    for (i = 0; i < mAudioIDs.size(); i++) {
        const AudioID &existing_audio_id = mAudioIDs[i];
        // Tracks indexes start from 1 and so a 0 is a null AudioID
        if (existing_audio_id.track_index == 0)
            continue;

        // Each audio track UID shall be unique
        if (memcmp(audio_id.uid, existing_audio_id.uid, sizeof(existing_audio_id.uid)) == 0) {
            log_warn("Ignoring audio ID that has a duplicate UID\n");
            return;
        }

        track_indexes.insert(existing_audio_id.track_index);
        num_uids++;
    }

    mAudioIDs.push_back(audio_id);
    if (audio_id.track_index != 0) {
        track_indexes.insert(audio_id.track_index);
        num_uids++;
    }

    mNumUIDs = num_uids;
    mNumTracks = (uint16_t)track_indexes.size();
}

uint32_t WaveCHNA::GetSize()
{
    return 4 + mAudioIDs.size() * 40;
}

vector<WaveCHNA::AudioID> WaveCHNA::GetAudioIDs(uint16_t track_index) const
{
    vector<AudioID> track_audio_ids;

    size_t i;
    for (i = 0; i < mAudioIDs.size(); i++) {
        const AudioID &audio_id = mAudioIDs[i];
        if (audio_id.track_index == track_index)
            track_audio_ids.push_back(audio_id);
    }

    return track_audio_ids;
}

void WaveCHNA::Write(WaveIO *output)
{
    uint32_t size = GetSize();

    output->WriteTag("chna");
    output->WriteSize(size);

    output->WriteUInt16(mNumTracks);
    output->WriteUInt16(mNumUIDs);

    size_t i;
    for (i = 0; i < mAudioIDs.size(); i++) {
        const AudioID &audio_id = mAudioIDs[i];

        output->WriteUInt16(audio_id.track_index);
        output->Write(audio_id.uid, sizeof(audio_id.uid));
        output->Write(audio_id.track_ref, sizeof(audio_id.track_ref));
        output->Write(audio_id.pack_ref, sizeof(audio_id.pack_ref));
        output->WriteUInt8(0);  // pad byte
    }
}

void WaveCHNA::Read(WaveIO *input, uint32_t size)
{
    mNumTracks = input->ReadUInt16();
    mNumUIDs = input->ReadUInt16();

    size_t num_audio_ids = (size - 4) / 40;
    size_t i;
    for (i = 0; i < num_audio_ids; i++) {
        AudioID audio_id;

        audio_id.track_index = input->ReadUInt16();
        input->Read(audio_id.uid, sizeof(audio_id.uid));
        input->Read(audio_id.track_ref, sizeof(audio_id.track_ref));
        input->Read(audio_id.pack_ref, sizeof(audio_id.pack_ref));
        input->ReadUInt8();  // pad byte

        mAudioIDs.push_back(audio_id);
    }
}
