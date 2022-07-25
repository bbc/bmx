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

#include <libMXF++/MXF.h>

#include <bmx/BMXTypes.h>


namespace bmx
{


typedef void (*property_setter_func)(mxfpp::MCALabelSubDescriptor *descriptor, const std::string &value);
typedef bool (*property_checker_func)(mxfpp::MCALabelSubDescriptor *descriptor, const std::string &value);

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

typedef struct
{
    MCALabelEntry entry;
    std::string gen_tag_symbol;
    std::string gen_tag_name;
} GeneratedMCALabelEntry;


// IMF SMPTE ST 2067-2 Channel Assignment Label

static const UL IMF_MCA_LABEL_FRAMEWORK =
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x02, 0x02, 0x10, 0x04, 0x01, 0x00, 0x00};


class ClipWriter;

class AppMCALabelHelper
{
public:
    static bool ParseAudioLayoutMode(const std::string &audio_mode_str, UL *label);

public:
    AppMCALabelHelper(bool is_as11);
    ~AppMCALabelHelper();

    bool IndexLabels(const MCALabelEntry *entries, size_t num_entries, bool override_duplicates);

    // Takes ownership of generated_entry if returns true
    bool IndexGeneratedLabel(GeneratedMCALabelEntry *generated_entry, bool override_duplicates);

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
        bool repeat;
        std::vector<std::pair<std::string, std::string> > string_properties;
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
    void SetLabelProperties(mxfpp::MCALabelSubDescriptor *descriptor, LabelLine *label_line);
    void CheckPropertiesEqual(mxfpp::MCALabelSubDescriptor *descriptor, LabelLine *label_line);

    void ParseTrackLines(const std::vector<std::string> &lines);
    LabelLine ParseLabelLine(const std::string &line);
    const MCALabelEntry* Find(const std::string &id_str);

    void ClearTrackLabels();

private:
    std::map<std::string, const MCALabelEntry*> mEntries;
    std::vector<GeneratedMCALabelEntry*> mGeneratedEntries;
    std::vector<TrackLabels*> mTrackLabels;
    std::map<uint32_t, TrackLabels*> mTrackLabelsByTrackIndex;
    std::map<std::string, property_setter_func> mMCAPropertySetterMap;
    std::map<std::string, property_checker_func> mMCAPropertyCheckerMap;
};


};


#endif
