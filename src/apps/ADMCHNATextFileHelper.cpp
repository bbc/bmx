/*
 * Copyright (C) 2022, British Broadcasting Corporation
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

#include <string.h>
#include <stdio.h>

#include <bmx/apps/ADMCHNATextFileHelper.h>
#include <bmx/apps/PropertyFileParser.h>

#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


WaveCHNA* bmx::parse_chna_text_file(const string &filename)
{
    PropertyFileParser property_parser;
    if (!property_parser.Open(filename))
        BMX_EXCEPTION(("Failed to open chna text file '%s'", filename.c_str()));

    WaveCHNA *chna = new WaveCHNA();
    try {
        string name;
        string value;
        unsigned int uint_value;
        bool have_audio_id = false;
        WaveCHNA::AudioID audio_id = {0, {0}, {0}, {0}};
        while (property_parser.ParseNext(&name, &value)) {
            if (name == "track_index") {
                if (sscanf(value.c_str(), "%u", &uint_value) != 1)
                    BMX_EXCEPTION(("Failed to parse chna track_index"));

                if (have_audio_id)
                    chna->AppendAudioID(audio_id);

                memset(&audio_id, 0, sizeof(audio_id));
                audio_id.track_index = uint_value;
                have_audio_id = true;
            } else if (name == "uid") {
                if (!have_audio_id)
                    BMX_EXCEPTION(("Missing chna track_index to start audio ID"));

                if (audio_id.track_index != 0) {  // track_index == 0 means null/placeholder audio ID
                    value.resize(sizeof(audio_id.uid));
                    memcpy(audio_id.uid, value.c_str(), sizeof(audio_id.uid));
                }
            } else if (name == "track_ref") {
                if (!have_audio_id)
                    BMX_EXCEPTION(("Missing chna track_index to start audio ID"));

                if (audio_id.track_index != 0) {  // track_index == 0 means null/placeholder audio ID
                    value.resize(sizeof(audio_id.track_ref));
                    memcpy(audio_id.track_ref, value.c_str(), sizeof(audio_id.track_ref));
                }
            } else if (name == "pack_ref") {
                if (!have_audio_id)
                    BMX_EXCEPTION(("Missing chna track_index to start audio ID"));

                if (audio_id.track_index != 0) {  // track_index == 0 means null/placeholder audio ID
                    value.resize(sizeof(audio_id.pack_ref));
                    memcpy(audio_id.pack_ref, value.c_str(), sizeof(audio_id.pack_ref));
                }
            } else {
                log_warn("Ignoring unknown property '%s' in chna text file\n", name.c_str());
            }
        }

        if (have_audio_id)
            chna->AppendAudioID(audio_id);

        if (chna->GetNumTracks() == 0)
            BMX_EXCEPTION(("No audio IDs parsed"));

        if (chna->GetNumUIDs() == 0)
            BMX_EXCEPTION(("No non-null audio IDs parsed"));

    } catch (...) {
        delete chna;
        throw;
    }

    return chna;
}

void bmx::write_chna_text_file(const string &filename, WaveCHNA *chna)
{
    FILE *text_file = fopen(filename.c_str(), "wb");
    if (!text_file)
        BMX_EXCEPTION(("Failed to open chna text file '%s' for writing: %s", filename.c_str(), bmx_strerror(errno).c_str()));

    try {
        const vector<WaveCHNA::AudioID> &audio_ids = chna->GetAudioIDs();
        size_t i;
        for (i = 0; i < audio_ids.size(); i++) {
            const WaveCHNA::AudioID &audio_id = audio_ids[i];

            if (i > 0)
                fprintf(text_file, "\n");

            fprintf(text_file, "track_index: %u\n", audio_id.track_index);
            if (audio_id.track_index == 0)  // track_index == 0 means null/placeholder audio ID
                continue;

            string value;
            if (audio_id.uid[0] != 0) {
                value.assign((const char*)audio_id.uid, sizeof(audio_id.uid));
                fprintf(text_file, "uid: %s\n", value.c_str());
            }

            if (audio_id.track_ref[0] != 0) {
                value.assign((const char*)audio_id.track_ref, sizeof(audio_id.track_ref));
                fprintf(text_file, "track_ref: %s\n", value.c_str());
            }

            if (audio_id.pack_ref[0] != 0) {
                value.assign((const char*)audio_id.pack_ref, sizeof(audio_id.pack_ref));
                fprintf(text_file, "pack_ref: %s\n", value.c_str());
            }
        }

    } catch (...) {
        fclose(text_file);
        throw;
    }

    fclose(text_file);
}
