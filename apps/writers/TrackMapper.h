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


#ifndef BMX_TRACK_MAPPER_H_
#define BMX_TRACK_MAPPER_H_

#include <inttypes.h>
#include <stdio.h>

#include <vector>
#include <set>
#include <string>

#include <bmx/EssenceType.h>


namespace bmx
{


class TrackMapper
{
public:
    typedef enum
    {
        INPUT_DEFINITION_MAP,
        MONO_AUDIO_MAP,
        STEREO_AUDIO_MAP,
        SINGLE_MC_AUDIO_MAP,
    } MapType;

    class InputTrackInfo
    {
    public:
        InputTrackInfo();

        uint32_t external_index;
        int data_def;
        EssenceType essence_type;
        uint32_t bits_per_sample;
        uint32_t channel_count;
    };

    class TrackChannelMap
    {
    public:
        TrackChannelMap();

        bool have_input;
        uint32_t input_external_index;
        uint32_t input_index;
        uint32_t input_channel_index;
        uint32_t output_channel_index;
    };

    class OutputTrackMap
    {
    public:
        OutputTrackMap();

        void Reset();

        EssenceType essence_type;
        int data_def;
        uint32_t bits_per_sample;
        std::vector<TrackChannelMap> channel_maps;
    };

public:
    static void DumpOutputTrackMap(FILE *file, const std::vector<InputTrackInfo> &input_tracks,
                                   const std::vector<OutputTrackMap> &track_maps);
    static bool IsMonoOutputTrackMap(const std::vector<OutputTrackMap> &track_maps);

public:
    TrackMapper();
    ~TrackMapper();

    bool ParseMapDef(const std::string &def_str);
    void SetMapType(MapType type);

    std::vector<OutputTrackMap> MapTracks(const std::vector<InputTrackInfo> &input_tracks,
                                          std::vector<InputTrackInfo> *unused_input_tracks);

    MapType GetMapType() const { return mMapType; }

private:
    std::vector<OutputTrackMap> DefinitionMapTracks(const std::vector<InputTrackInfo> &input_tracks,
                                                    std::vector<InputTrackInfo> *unused_input_tracks);
    std::vector<OutputTrackMap> MonoAudioMapTracks(const std::vector<InputTrackInfo> &input_tracks);
    std::vector<OutputTrackMap> StereoAudioMapTracks(const std::vector<InputTrackInfo> &input_tracks);
    std::vector<OutputTrackMap> SingleMCAudioMapTracks(const std::vector<InputTrackInfo> &input_tracks);

    bool CheckCompatibleAudio(const std::vector<InputTrackInfo> &input_tracks, std::vector<OutputTrackMap> &output);

    void ClearDefinition();

private:
    class ParsedDefinition;

    class ParsedElement
    {
    public:
        virtual ~ParsedElement() {};
    };

    class ParsedInput : public ParsedElement
    {
    public:
        static ParsedInput* Parse(ParsedDefinition *def, const std::string &ele_str);

    public:
        virtual ~ParsedInput() {};

        uint32_t index;
    };

    class ParsedSilenceChannels : public ParsedElement
    {
    public:
        static ParsedSilenceChannels* Parse(ParsedDefinition *def, const std::string &ele_str);

    public:
        virtual ~ParsedSilenceChannels() {};

        uint32_t channel_count;
    };

    class ParsedInputRange : public ParsedElement
    {
    public:
        static ParsedInputRange* Parse(ParsedDefinition *def, const std::string &from_str, const std::string &to_str);

    public:
        virtual ~ParsedInputRange() {};

        uint32_t from_index;
        uint32_t to_index;
    };

    class ParsedRemainderRange : public ParsedElement
    {
    public:
        static ParsedRemainderRange* Parse(ParsedDefinition *def, const std::string &ele_str);

    public:
        virtual ~ParsedRemainderRange() {};
    };

    class ParsedGroup
    {
    public:
        static ParsedGroup* Parse(ParsedDefinition *def, const std::string &group_str);

    public:
        virtual ~ParsedGroup();

        bool single_track;
        std::vector<ParsedElement*> elements;
    };

    class ParsedDefinition
    {
    public:
        static ParsedDefinition* Parse(const std::string &def_str);

    public:
        virtual ~ParsedDefinition();

        std::vector<ParsedGroup*> groups;
    };

private:
    MapType mMapType;
    ParsedDefinition *mDefinition;
};


};


#endif
