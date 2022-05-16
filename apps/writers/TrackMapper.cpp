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

#include "TrackMapper.h"

#include <string.h>

#include <map>

#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


TrackMapper::InputTrackInfo::InputTrackInfo()
{
    external_index = 0;
    data_def = MXF_UNKNOWN_DDEF;
    essence_type = UNKNOWN_ESSENCE_TYPE;
    bits_per_sample = 0;
    channel_count = 1;
}


TrackMapper::TrackChannelMap::TrackChannelMap()
{
    have_input = false;
    input_external_index = 0;
    input_index = 0;
    input_channel_index = 0;
    output_channel_index = 0;
}


TrackMapper::OutputTrackMap::OutputTrackMap()
{
    Reset();
}

void TrackMapper::OutputTrackMap::Reset()
{
    essence_type = UNKNOWN_ESSENCE_TYPE;
    data_def = MXF_UNKNOWN_DDEF;
    bits_per_sample = 0;
    channel_maps.clear();
}



TrackMapper::ParsedInput* TrackMapper::ParsedInput::Parse(ParsedDefinition *def, const string &ele_str)
{
    (void)def;

    unsigned int val;
    BMX_CHECK(sscanf(ele_str.c_str(), "%u", &val) == 1);

    ParsedInput *input = new ParsedInput();
    input->index = val;
    return input;
}


TrackMapper::ParsedInputRange* TrackMapper::ParsedInputRange::Parse(ParsedDefinition *def,
                                                                    const string &from_str,
                                                                    const string &to_str)
{
    (void)def;

    unsigned int val1, val2;
    BMX_CHECK(sscanf(from_str.c_str(), "%u", &val1) == 1 &&
              sscanf(to_str.c_str(), "%u", &val2) == 1);

    ParsedInputRange *input = new ParsedInputRange();
    input->from_index = val1;
    input->to_index   = val2;
    return input;
}


TrackMapper::ParsedSilenceChannels* TrackMapper::ParsedSilenceChannels::Parse(ParsedDefinition *def,
                                                                              const string &ele_str)
{
    (void)def;

    unsigned int val;
    BMX_CHECK(sscanf(ele_str.c_str(), "s%u", &val) == 1);

    ParsedSilenceChannels *silence_channels = new ParsedSilenceChannels();
    silence_channels->channel_count = val;
    return silence_channels;
}


TrackMapper::ParsedRemainderRange* TrackMapper::ParsedRemainderRange::Parse(ParsedDefinition *def,
                                                                            const string &ele_str)
{
    (void)def;

    BMX_CHECK(ele_str == "x");

    ParsedRemainderRange *rem_range = new ParsedRemainderRange();
    return rem_range;
}


TrackMapper::ParsedGroup* TrackMapper::ParsedGroup::Parse(ParsedDefinition *def, const string &group_str_in)
{
    ParsedGroup *group = 0;
    ParsedElement *element = 0;
    try
    {
        group = new ParsedGroup();

        string group_str = group_str_in;
        if (group_str[0] == 'm') {
            group->single_track = false;
            group_str = group_str.substr(1);
        } else {
            group->single_track = true;
        }

        BMX_CHECK(!group_str.empty());
        if (group_str[0] == 'x') {
            group->elements.push_back(ParsedRemainderRange::Parse(def, group_str));
        } else {
            vector<string> items = split_string(group_str, ',', false, true);
            size_t i;
            for (i = 0; i < items.size(); i++) {
                const string &item_str = items[i];

                vector<string> range = split_string(item_str, '-', false, true);
                BMX_CHECK(range.size() == 1 || range.size() == 2);
                if (range.size() == 1) {
                    const string &input_str = range[0];
                    if (input_str[0] == 's')
                        element = ParsedSilenceChannels::Parse(def, input_str);
                    else
                        element = ParsedInput::Parse(def, input_str);
                } else {
                    const string &from_str = range[0];
                    const string &to_str   = range[1];
                    element = ParsedInputRange::Parse(def, from_str, to_str);
                }

                group->elements.push_back(element);
                element = 0;
            }
        }

        return group;
    }
    catch (...)
    {
        delete group;
        delete element;
        throw;
    }
}

TrackMapper::ParsedGroup::~ParsedGroup()
{
    size_t i;
    for (i = 0; i < elements.size(); i++)
        delete elements[i];
}


TrackMapper::ParsedDefinition* TrackMapper::ParsedDefinition::Parse(const string &def_str)
{
    BMX_CHECK(!def_str.empty());

    ParsedDefinition *def = 0;
    try
    {
        def = new ParsedDefinition();

        vector<string> groups = split_string(def_str, ';', false, true);
        size_t g;
        for (g = 0; g < groups.size(); g++)
            def->groups.push_back(ParsedGroup::Parse(def, groups[g]));

        return def;
    }
    catch (...)
    {
        delete def;
        throw;
    }
}

TrackMapper::ParsedDefinition::~ParsedDefinition()
{
    size_t i;
    for (i = 0; i < groups.size(); i++)
        delete groups[i];
}



void TrackMapper::DumpOutputTrackMap(FILE *file, const vector<InputTrackInfo> &input_tracks,
                                     const vector<OutputTrackMap> &track_maps)
{
    map<uint32_t, uint32_t> input_base_channels;
    uint32_t total_channels = 0;
    size_t i;
    for (i = 0; i < input_tracks.size(); i++) {
        const InputTrackInfo &input_track = input_tracks[i];
        if (input_track.essence_type == WAVE_PCM) {
            input_base_channels[(uint32_t)i] = total_channels;
            total_channels += input_track.channel_count;
        }
    }

    for (i = 0; i < track_maps.size(); i++) {
        const OutputTrackMap &track_map = track_maps[i];
        fprintf(file, "Track %" PRIszt " (%s)\n", i, essence_type_to_string(track_map.essence_type));
        size_t c;
        for (c = 0; c < track_map.channel_maps.size(); c++) {
            const TrackChannelMap &channel_map = track_map.channel_maps[c];
            if (track_map.essence_type == WAVE_PCM) {
                uint32_t input_channel_index = input_base_channels[channel_map.input_index] +
                                                    channel_map.input_channel_index;
                if (channel_map.have_input)
                    fprintf(file, "  %u <- %u\n", channel_map.output_channel_index, input_channel_index);
                else
                    fprintf(file, "  %u <- <silence>\n", channel_map.output_channel_index);
            } else {
                fprintf(file, "  %u <- %u\n", channel_map.output_channel_index, channel_map.input_index);
            }
        }
    }
}

bool TrackMapper::IsMonoOutputTrackMap(const vector<OutputTrackMap> &track_maps)
{
    size_t i;
    for (i = 0; i < track_maps.size(); i++) {
        if (track_maps[i].channel_maps.size() != 1)
            return false;
    }

    return true;
}

TrackMapper::TrackMapper()
{
    mMapType = MONO_AUDIO_MAP;
    mDefinition = 0;
}

TrackMapper::~TrackMapper()
{
    ClearDefinition();
}

bool TrackMapper::ParseMapDef(const string &def_str)
{
    ClearDefinition();

    string str = trim_string(def_str);
    if (str == "mono")
        mMapType = MONO_AUDIO_MAP;
    else if (str == "stereo")
        mMapType = STEREO_AUDIO_MAP;
    else if (str == "singlemca")
        mMapType = SINGLE_MC_AUDIO_MAP;
    else
        mMapType = INPUT_DEFINITION_MAP;
    if (mMapType != INPUT_DEFINITION_MAP)
        return true;

    try
    {
        mDefinition = ParsedDefinition::Parse(def_str);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void TrackMapper::SetMapType(MapType type)
{
    mMapType = type;
    if (type != INPUT_DEFINITION_MAP)
        ClearDefinition();
}

vector<TrackMapper::OutputTrackMap> TrackMapper::MapTracks(const vector<InputTrackInfo> &input_tracks,
                                                           vector<InputTrackInfo> *unused_input_tracks)
{
    switch (mMapType)
    {
        case INPUT_DEFINITION_MAP:
            return DefinitionMapTracks(input_tracks, unused_input_tracks);
        case MONO_AUDIO_MAP:
            unused_input_tracks->clear();
            return MonoAudioMapTracks(input_tracks);
        case STEREO_AUDIO_MAP:
            unused_input_tracks->clear();
            return StereoAudioMapTracks(input_tracks);
        case SINGLE_MC_AUDIO_MAP:
            unused_input_tracks->clear();
            return SingleMCAudioMapTracks(input_tracks);
    }

    return vector<TrackMapper::OutputTrackMap>(); // for the compiler
}

vector<TrackMapper::OutputTrackMap> TrackMapper::DefinitionMapTracks(const vector<InputTrackInfo> &input_tracks,
                                                                     vector<InputTrackInfo> *unused_input_tracks)
{
    vector<TrackMapper::OutputTrackMap> output;

    if (!mDefinition || mDefinition->groups.empty()) {
        unused_input_tracks->insert(unused_input_tracks->begin(), input_tracks.begin(), input_tracks.end());
        return output;
    }

    set<size_t> used_input_tracks;
    vector<pair<uint32_t, uint32_t> > input_channels;
    set<size_t> unused_channels;
    size_t i;
    for (i = 0; i < input_tracks.size(); i++) {
        const InputTrackInfo &input_track = input_tracks[i];
        if (input_track.essence_type == WAVE_PCM) {
            size_t c;
            for (c = 0; c < input_track.channel_count; c++) {
                unused_channels.insert(input_channels.size());
                input_channels.push_back(make_pair((uint32_t)i, (uint32_t)c));
            }
        }
    }

    size_t g;
    for (g = 0; g < mDefinition->groups.size(); g++) {
        ParsedGroup *group = mDefinition->groups[g];
        OutputTrackMap track_map;
        size_t e;
        for (e = 0; e < group->elements.size(); e++) {
            ParsedElement *element = group->elements[e];
            ParsedInput *input                      = dynamic_cast<ParsedInput*>(element);
            ParsedSilenceChannels *silence_channels = dynamic_cast<ParsedSilenceChannels*>(element);
            ParsedInputRange *input_range           = dynamic_cast<ParsedInputRange*>(element);
            ParsedRemainderRange *rem_range         = dynamic_cast<ParsedRemainderRange*>(element);

            if (input) {
                if (input->index < input_channels.size()) {
                    uint32_t input_track_index    = input_channels[input->index].first;
                    uint32_t input_channel_index  = input_channels[input->index].second;
                    const InputTrackInfo &input_track = input_tracks[input_track_index];

                    track_map.essence_type    = input_track.essence_type;
                    track_map.data_def        = input_track.data_def;
                    track_map.bits_per_sample = input_track.bits_per_sample;

                    TrackChannelMap channel_map;
                    channel_map.have_input           = true;
                    channel_map.input_external_index = input_track.external_index;
                    channel_map.input_index          = input_track_index;
                    channel_map.input_channel_index  = input_channel_index;
                    channel_map.output_channel_index = (uint32_t)track_map.channel_maps.size();
                    track_map.channel_maps.push_back(channel_map);

                    if (!group->single_track) {
                        output.push_back(track_map);
                        track_map.Reset();
                    }

                    unused_channels.erase(input->index);
                    used_input_tracks.insert(input_track_index);
                }
            } else if (silence_channels) {
                size_t r;
                for (r = 0; r < silence_channels->channel_count; r++) {
                    if (track_map.channel_maps.empty()) {
                        track_map.essence_type    = WAVE_PCM;
                        track_map.data_def        = MXF_SOUND_DDEF;
                        track_map.bits_per_sample = 16; // could be updated if there are other channels
                    }

                    TrackChannelMap channel_map;
                    channel_map.have_input           = false;
                    channel_map.output_channel_index = (uint32_t)track_map.channel_maps.size();
                    track_map.channel_maps.push_back(channel_map);

                    if (!group->single_track) {
                        output.push_back(track_map);
                        track_map.Reset();
                    }
                }
            } else if (input_range) {
                size_t r;
                for (r = input_range->from_index; r <= input_range->to_index && r < input_channels.size(); r++) {
                    uint32_t input_track_index    = input_channels[r].first;
                    uint32_t input_channel_index  = input_channels[r].second;
                    const InputTrackInfo &input_track = input_tracks[input_track_index];

                    track_map.essence_type    = input_track.essence_type;
                    track_map.data_def        = input_track.data_def;
                    track_map.bits_per_sample = input_track.bits_per_sample;

                    TrackChannelMap channel_map;
                    channel_map.have_input           = true;
                    channel_map.input_external_index = input_track.external_index;
                    channel_map.input_index          = input_track_index;
                    channel_map.input_channel_index  = input_channel_index;
                    channel_map.output_channel_index = (uint32_t)track_map.channel_maps.size();
                    track_map.channel_maps.push_back(channel_map);

                    if (!group->single_track) {
                        output.push_back(track_map);
                        track_map.Reset();
                    }

                    unused_channels.erase(r);
                    used_input_tracks.insert(input_track_index);
                }
            } else if (rem_range) {
                set<size_t>::iterator iter;
                for (iter = unused_channels.begin(); iter != unused_channels.end(); iter++) {
                    uint32_t input_track_index    = input_channels[*iter].first;
                    uint32_t input_channel_index  = input_channels[*iter].second;
                    const InputTrackInfo &input_track = input_tracks[input_track_index];

                    track_map.essence_type    = input_track.essence_type;
                    track_map.data_def        = input_track.data_def;
                    track_map.bits_per_sample = input_track.bits_per_sample;

                    TrackChannelMap channel_map;
                    channel_map.have_input           = true;
                    channel_map.input_external_index = input_track.external_index;
                    channel_map.input_index          = input_track_index;
                    channel_map.input_channel_index  = input_channel_index;
                    channel_map.output_channel_index = (uint32_t)track_map.channel_maps.size();
                    track_map.channel_maps.push_back(channel_map);

                    if (!group->single_track) {
                        output.push_back(track_map);
                        track_map.Reset();
                    }

                    used_input_tracks.insert(input_track_index);
                }
                unused_channels.clear();
            }
        }
        if (group->single_track && !track_map.channel_maps.empty())
            output.push_back(track_map);
    }

    BMX_CHECK(CheckCompatibleAudio(input_tracks, output));

    for (i = 0; i < input_tracks.size(); i++) {
        if (!used_input_tracks.count(i))
            unused_input_tracks->push_back(input_tracks[i]);
    }

    return output;
}

vector<TrackMapper::OutputTrackMap> TrackMapper::MonoAudioMapTracks(const vector<InputTrackInfo> &input_tracks)
{
    vector<OutputTrackMap> output;

    size_t i;
    for (i = 0; i < input_tracks.size(); i++) {
        const InputTrackInfo &input_track = input_tracks[i];
        if (input_track.essence_type == WAVE_PCM) {
            uint32_t c;
            for (c = 0; c < input_track.channel_count; c++) {
                OutputTrackMap track_map;
                track_map.essence_type    = input_track.essence_type;
                track_map.data_def        = input_track.data_def;
                track_map.bits_per_sample = input_track.bits_per_sample;

                TrackChannelMap channel_map;
                channel_map.have_input           = true;
                channel_map.input_external_index = input_track.external_index;
                channel_map.input_index          = (uint32_t)i;
                channel_map.input_channel_index  = c;
                channel_map.output_channel_index = 0;
                track_map.channel_maps.push_back(channel_map);

                output.push_back(track_map);
            }
        } else {
            OutputTrackMap track_map;
            track_map.essence_type    = input_track.essence_type;
            track_map.data_def        = input_track.data_def;
            track_map.bits_per_sample = input_track.bits_per_sample;

            TrackChannelMap channel_map;
            channel_map.have_input           = true;
            channel_map.input_external_index = input_track.external_index;
            channel_map.input_index          = (uint32_t)i;
            channel_map.input_channel_index  = 0;
            channel_map.output_channel_index = 0;
            track_map.channel_maps.push_back(channel_map);

            output.push_back(track_map);
        }
    }

    BMX_CHECK(CheckCompatibleAudio(input_tracks, output));

    return output;
}

vector<TrackMapper::OutputTrackMap> TrackMapper::StereoAudioMapTracks(const vector<InputTrackInfo> &input_tracks)
{
    vector<OutputTrackMap> output;

    size_t mono_track_index = 0;
    bool have_mono_channel = false;
    size_t i;
    for (i = 0; i < input_tracks.size(); i++) {
        const InputTrackInfo &input_track = input_tracks[i];
        if (input_track.essence_type == WAVE_PCM) {
            uint32_t c = 0;
            if (have_mono_channel && c < input_track.channel_count) {
                TrackChannelMap channel_map;
                channel_map.have_input           = true;
                channel_map.input_external_index = input_track.external_index;
                channel_map.input_index          = (uint32_t)i;
                channel_map.input_channel_index  = c;
                channel_map.output_channel_index = 1;
                output[mono_track_index].channel_maps.push_back(channel_map);
                have_mono_channel = false;
                c++;
            }
            while (c < input_track.channel_count) {
                OutputTrackMap track_map;
                track_map.essence_type    = input_track.essence_type;
                track_map.data_def        = input_track.data_def;
                track_map.bits_per_sample = input_track.bits_per_sample;

                TrackChannelMap channel_map;
                channel_map.have_input           = true;
                channel_map.input_external_index = input_track.external_index;
                channel_map.input_index          = (uint32_t)i;
                channel_map.input_channel_index  = c;
                channel_map.output_channel_index = 0;
                track_map.channel_maps.push_back(channel_map);
                c++;
                if (c < input_track.channel_count) {
                    TrackChannelMap channel_map;
                    channel_map.have_input           = true;
                    channel_map.input_external_index = input_track.external_index;
                    channel_map.input_index          = (uint32_t)i;
                    channel_map.input_channel_index  = c;
                    channel_map.output_channel_index = 1;
                    track_map.channel_maps.push_back(channel_map);
                    c++;
                } else {
                    have_mono_channel = true;
                    mono_track_index = output.size();
                }

                output.push_back(track_map);
            }
        } else {
            OutputTrackMap track_map;
            track_map.essence_type    = input_track.essence_type;
            track_map.data_def        = input_track.data_def;
            track_map.bits_per_sample = input_track.bits_per_sample;

            TrackChannelMap channel_map;
            channel_map.have_input           = true;
            channel_map.input_external_index = input_track.external_index;
            channel_map.input_index          = (uint32_t)i;
            channel_map.input_channel_index  = 0;
            channel_map.output_channel_index = 0;
            track_map.channel_maps.push_back(channel_map);

            output.push_back(track_map);
        }
    }

    // add a silence channel if there is a mono channel remaining
    if (have_mono_channel) {
        TrackChannelMap channel_map;
        channel_map.have_input           = false;
        channel_map.output_channel_index = 1;
        output[mono_track_index].channel_maps.push_back(channel_map);
        have_mono_channel = false;
    }

    BMX_CHECK(CheckCompatibleAudio(input_tracks, output));

    return output;
}

vector<TrackMapper::OutputTrackMap> TrackMapper::SingleMCAudioMapTracks(const vector<InputTrackInfo> &input_tracks)
{
    vector<OutputTrackMap> output;

    size_t pcm_output_index = 0;
    bool have_pcm_output = false;
    size_t i;
    for (i = 0; i < input_tracks.size(); i++) {
        const InputTrackInfo &input_track = input_tracks[i];
        if (input_track.essence_type == WAVE_PCM) {
            if (!have_pcm_output) {
                OutputTrackMap track_map;
                track_map.essence_type    = input_track.essence_type;
                track_map.data_def        = input_track.data_def;
                track_map.bits_per_sample = input_track.bits_per_sample;
                output.push_back(track_map);

                have_pcm_output = true;
                pcm_output_index = output.size() - 1;
            }
            OutputTrackMap &track_map = output[pcm_output_index];

            uint32_t c;
            for (c = 0; c < input_track.channel_count; c++) {
                TrackChannelMap channel_map;
                channel_map.have_input           = true;
                channel_map.input_external_index = input_track.external_index;
                channel_map.input_index          = (uint32_t)i;
                channel_map.input_channel_index  = c;
                channel_map.output_channel_index = (uint32_t)track_map.channel_maps.size();
                track_map.channel_maps.push_back(channel_map);
            }
        } else {
            OutputTrackMap track_map;
            track_map.essence_type    = input_track.essence_type;
            track_map.data_def        = input_track.data_def;
            track_map.bits_per_sample = input_track.bits_per_sample;

            TrackChannelMap channel_map;
            channel_map.have_input           = true;
            channel_map.input_external_index = input_track.external_index;
            channel_map.input_index          = (uint32_t)i;
            channel_map.input_channel_index  = 0;
            channel_map.output_channel_index = 0;
            track_map.channel_maps.push_back(channel_map);

            output.push_back(track_map);
        }
    }

    BMX_CHECK(CheckCompatibleAudio(input_tracks, output));

    return output;
}

bool TrackMapper::CheckCompatibleAudio(const vector<InputTrackInfo> &input_tracks, vector<OutputTrackMap> &output)
{
    size_t i;
    for (i = 0; i < output.size(); i++) {
        OutputTrackMap &track_map = output[i];
        if (track_map.essence_type == WAVE_PCM) {
            size_t c;
            for (c = 0; c < track_map.channel_maps.size(); c++) {
                TrackChannelMap &channel_map = track_map.channel_maps[c];
                if (channel_map.have_input) {
                    const InputTrackInfo &input_track = input_tracks[channel_map.input_index];
                    if (input_track.bits_per_sample != track_map.bits_per_sample) {
                        log_error("PCM bits per sample is not the same for all channels in output track\n");
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void TrackMapper::ClearDefinition()
{
    delete mDefinition;
    mDefinition = 0;
}
