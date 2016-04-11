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

#ifndef BMX_APP_MCA_LABEL_HELPER_H_
#define BMX_APP_MCA_LABEL_HELPER_H_

#include <string>
#include <vector>
#include <map>

#include <bmx/BMXTypes.h>


namespace bmx
{


typedef enum
{
    AUDIO_CHANNEL_LABEL,
    SOUNDFIELD_GROUP_LABEL,
    GROUP_OF_SOUNDFIELD_GROUP_LABEL,
} MCALabelType;

typedef struct
{
    MCALabelType type;
    const char *tag_symbol;
    const char *tag_name; // optional
    UL dict_id;
} MCALabelEntry;


class ClipWriter;

class AppMCALabelHelper
{
public:
    AppMCALabelHelper();
    ~AppMCALabelHelper();

    bool IndexLabels(const MCALabelEntry *entries, size_t num_entries);

public:
    bool ParseTrackLabels(const std::string &filename);

    void InsertTrackLabels(ClipWriter *clip);

private:
    class LabelLine
    {
    public:
        LabelLine();

        void Reset();

        const MCALabelEntry *label;
        std::string id;
        uint32_t channel_index; // starts from 0. Note that the descriptor ChannelID starts from 1!
        std::string language;
        bool repeat;
    };

    class SoundfieldGroup
    {
    public:
        void Reset();

        LabelLine sg_label_line; // can be 'null', indicating that the channels are not assigned to a soundfield group
        std::vector<LabelLine> c_label_lines;
        std::vector<LabelLine> gosg_label_lines;
    };

    class TrackLabels
    {
    public:
        uint32_t track_index;
        std::vector<SoundfieldGroup> soundfield_groups;
    };

private:
    void ParseTrackLines(const std::vector<std::string> &lines);
    LabelLine ParseLabelLine(const std::string &line);
    const MCALabelEntry* Find(const std::string &id_str);

    void ClearTrackLabels();

private:
    std::map<std::string, const MCALabelEntry*> mEntries;
    std::vector<TrackLabels*> mTrackLabels;
    std::map<uint32_t, TrackLabels*> mTrackLabelsByTrackIndex;
};


};


#endif
