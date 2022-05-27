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

#include <bmx/apps/AppMCALabelHelper.h>

#include <string.h>
#include <stdio.h>

#include <fstream>

#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/mxf_op1a/OP1APCMTrack.h>
#include <bmx/rdd9_mxf/RDD9PCMTrack.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace mxfpp;
using namespace bmx;


// Multi-channel audio labels from D-Cinema, SMPTE ST 428-12:2013

#define DC_MCA_LABEL(type, octet11) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x03, 0x02, type, octet11, 0x00, 0x00, 0x00, 0x00}

#define DC_CHANNEL_LABEL(octet11)  DC_MCA_LABEL(0x01, octet11)
#define DC_SOUNDFIELD_GROUP_LABEL(octet11)  DC_MCA_LABEL(0x02, octet11)


static const MCALabelEntry DC_MCA_LABELS[] =
{
    // Channels
    {AUDIO_CHANNEL_LABEL, "chL", "Left", DC_CHANNEL_LABEL(0x01)},
    {AUDIO_CHANNEL_LABEL, "chR", "Right", DC_CHANNEL_LABEL(0x02)},
    {AUDIO_CHANNEL_LABEL, "chC", "Center", DC_CHANNEL_LABEL(0x03)},
    {AUDIO_CHANNEL_LABEL, "chLFE", "LFE", DC_CHANNEL_LABEL(0x04)},
    {AUDIO_CHANNEL_LABEL, "chLs", "Left Surround", DC_CHANNEL_LABEL(0x05)},
    {AUDIO_CHANNEL_LABEL, "chRs", "Right Surround", DC_CHANNEL_LABEL(0x06)},
    {AUDIO_CHANNEL_LABEL, "chLss", "Left Side Surround", DC_CHANNEL_LABEL(0x07)},
    {AUDIO_CHANNEL_LABEL, "chRss", "Right Side Surround", DC_CHANNEL_LABEL(0x08)},
    {AUDIO_CHANNEL_LABEL, "chLrs", "Left Rear Surround", DC_CHANNEL_LABEL(0x09)},
    {AUDIO_CHANNEL_LABEL, "chRrs", "Right Rear Surround", DC_CHANNEL_LABEL(0x0a)},
    {AUDIO_CHANNEL_LABEL, "chLc", "Left Center", DC_CHANNEL_LABEL(0x0b)},
    {AUDIO_CHANNEL_LABEL, "chRc", "Right Center", DC_CHANNEL_LABEL(0x0c)},
    {AUDIO_CHANNEL_LABEL, "chCs", "Center Surround", DC_CHANNEL_LABEL(0x0d)},
    {AUDIO_CHANNEL_LABEL, "chHI", "Hearing Impaired", DC_CHANNEL_LABEL(0x0e)},
    {AUDIO_CHANNEL_LABEL, "chVIN", "Visually Impaired Narrative", DC_CHANNEL_LABEL(0x0f)},

    // Soundfield Groups
    {SOUNDFIELD_GROUP_LABEL, "sg51", "5.1", DC_SOUNDFIELD_GROUP_LABEL(0x01)},
    {SOUNDFIELD_GROUP_LABEL, "sg71", "7.1DS", DC_SOUNDFIELD_GROUP_LABEL(0x02)},
    {SOUNDFIELD_GROUP_LABEL, "sgSDS", "7.1SDS", DC_SOUNDFIELD_GROUP_LABEL(0x03)},
    {SOUNDFIELD_GROUP_LABEL, "sg61", "6.1", DC_SOUNDFIELD_GROUP_LABEL(0x04)},
    {SOUNDFIELD_GROUP_LABEL, "sgM", "Monoaural", DC_SOUNDFIELD_GROUP_LABEL(0x05)},
};


// Multi-channel audio labels from IMF, SMPTE ST 2067-8:2013

#define IMF_MCA_LABEL(type, octet12, octet13) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x03, 0x02, type, 0x20, octet12, octet13, 0x00, 0x00}

#define IMF_CHANNEL_LABEL(octet12, octet13)  IMF_MCA_LABEL(0x01, octet12, octet13)
#define IMF_SOUNDFIELD_GROUP_LABEL(octet12)  IMF_MCA_LABEL(0x02, octet12, 0x00)
#define IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(octet12)  IMF_MCA_LABEL(0x03, octet12, 0x00)


static const MCALabelEntry IMF_MCA_LABELS[] =
{
    //  Channels
    {AUDIO_CHANNEL_LABEL, "chM1", "Mono One", IMF_CHANNEL_LABEL(0x01, 0x00)},
    {AUDIO_CHANNEL_LABEL, "chM2", "Mono Two", IMF_CHANNEL_LABEL(0x02, 0x00)},
    {AUDIO_CHANNEL_LABEL, "chLt", "Left Total", IMF_CHANNEL_LABEL(0x03, 0x00)},
    {AUDIO_CHANNEL_LABEL, "chRt", "Right Total", IMF_CHANNEL_LABEL(0x04, 0x00)},
    {AUDIO_CHANNEL_LABEL, "chLst", "Left Surround Total", IMF_CHANNEL_LABEL(0x05, 0x00)},
    {AUDIO_CHANNEL_LABEL, "chRst", "Right Surround Total", IMF_CHANNEL_LABEL(0x06, 0x00)},
    {AUDIO_CHANNEL_LABEL, "chS", "Surround", IMF_CHANNEL_LABEL(0x07, 0x00)},

    // Soundfield Groups
    {SOUNDFIELD_GROUP_LABEL, "sgST", "Standard Stereo", IMF_SOUNDFIELD_GROUP_LABEL(0x01)},
    {SOUNDFIELD_GROUP_LABEL, "sgDM", "Dual Mono", IMF_SOUNDFIELD_GROUP_LABEL(0x02)},
    {SOUNDFIELD_GROUP_LABEL, "sgDNS", "Discrete Numbered Sources", IMF_SOUNDFIELD_GROUP_LABEL(0x03)},
    {SOUNDFIELD_GROUP_LABEL, "sg30", "3.0", IMF_SOUNDFIELD_GROUP_LABEL(0x04)},
    {SOUNDFIELD_GROUP_LABEL, "sg40", "4.0", IMF_SOUNDFIELD_GROUP_LABEL(0x05)},
    {SOUNDFIELD_GROUP_LABEL, "sg50", "5.0", IMF_SOUNDFIELD_GROUP_LABEL(0x06)},
    {SOUNDFIELD_GROUP_LABEL, "sg60", "6.0", IMF_SOUNDFIELD_GROUP_LABEL(0x07)},
    {SOUNDFIELD_GROUP_LABEL, "sg70", "7.0DS", IMF_SOUNDFIELD_GROUP_LABEL(0x08)},
    {SOUNDFIELD_GROUP_LABEL, "sgLtRt", "Lt-Rt", IMF_SOUNDFIELD_GROUP_LABEL(0x09)},
    {SOUNDFIELD_GROUP_LABEL, "sg51EX", "5.1EX", IMF_SOUNDFIELD_GROUP_LABEL(0x0a)},
    {SOUNDFIELD_GROUP_LABEL, "sgHA", "Hearing Accessibility", IMF_SOUNDFIELD_GROUP_LABEL(0x0b)},
    {SOUNDFIELD_GROUP_LABEL, "sgVA", "Visual Accessibility", IMF_SOUNDFIELD_GROUP_LABEL(0x0c)},

    // Group of Soundfield Groups
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggMPg", "Main Program", IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x01)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggDVS", "Descriptive Video Service", IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x02)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggDcm", "Dialog Centric Mix", IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x03)},
};


// Multi-channel audio labels from AMWA AS-11

#define AS11_MCA_LABEL(type, octet14) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x08, 0x01, 0x01, type, octet14, 0x00}

#define AS11_CHANNEL_LABEL(octet14)  AS11_MCA_LABEL(0x01, octet14)
#define AS11_SOUNDFIELD_GROUP_LABEL(octet14)  AS11_MCA_LABEL(0x02, octet14)
#define AS11_GROUP_OF_SOUNDFIELD_GROUP_LABEL(octet14)  AS11_MCA_LABEL(0x03, octet14)


static const MCALabelEntry AS11_MCA_LABELS[] =
{
    //  Channels
    {AUDIO_CHANNEL_LABEL, "chADSSdc", "AD Studio Signal Data Channel", AS11_CHANNEL_LABEL(0x01)},

    // Soundfield Groups
    {SOUNDFIELD_GROUP_LABEL, "sgADSS", "AD Studio Signal", AS11_SOUNDFIELD_GROUP_LABEL(0x01)},

    // Group of Soundfield Groups
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggAPg", "Alternative Program", AS11_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x01)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggADPgMx", "Audio Description Program Mix", AS11_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x02)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggAD", "Audio Description", AS11_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x03)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggME", "Music and Effects", AS11_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x04)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "ggUnAu", "Unused Audio", AS11_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x05)},
};

static const MCALabelEntry AS11_OVERRIDE_MCA_LABELS[] =
{
    // The label below is named "Visually Impaired-Narrative" in DC/IMF, i.e. with a "-"
    {AUDIO_CHANNEL_LABEL, "chVIN", "Visually Impaired Narrative", DC_CHANNEL_LABEL(0x0f)}
};



AppMCALabelHelper::LabelLine::LabelLine()
{
    Reset();
}

void AppMCALabelHelper::LabelLine::Reset()
{
    label = 0;
    id.clear();
    channel_index = (uint32_t)(-1);
    language.clear();
    repeat = true;
}


void AppMCALabelHelper::SoundfieldGroup::Reset()
{
    sg_label_line.Reset();
    c_label_lines.clear();
    gosg_label_lines.clear();
}



AppMCALabelHelper::AppMCALabelHelper(bool is_as11)
{
    // Register D-Cinema, IMF and AS-11 labels
    IndexLabels(DC_MCA_LABELS, BMX_ARRAY_SIZE(DC_MCA_LABELS), true);
    IndexLabels(IMF_MCA_LABELS, BMX_ARRAY_SIZE(IMF_MCA_LABELS), true);
    IndexLabels(AS11_MCA_LABELS, BMX_ARRAY_SIZE(AS11_MCA_LABELS), true);
    if (is_as11)
        IndexLabels(AS11_OVERRIDE_MCA_LABELS, BMX_ARRAY_SIZE(AS11_OVERRIDE_MCA_LABELS), true);

    // Generate numbered source audio labels
    char buffer[32];
    uint8_t i;
    for (i = 1; i < 128; i++) {
        GeneratedMCALabelEntry *generated_entry = new GeneratedMCALabelEntry();

        bmx_snprintf(buffer, sizeof(buffer), "chNSC%03d", i);
        generated_entry->gen_tag_symbol = buffer;

        bmx_snprintf(buffer, sizeof(buffer), "Numbered Source Channel %03d", i);
        generated_entry->gen_tag_name = buffer;

        generated_entry->entry.type = AUDIO_CHANNEL_LABEL;
        generated_entry->entry.tag_symbol = generated_entry->gen_tag_symbol.c_str();
        generated_entry->entry.tag_name = generated_entry->gen_tag_name.c_str();
        generated_entry->entry.dict_id = IMF_CHANNEL_LABEL(0x08, i);

        IndexGeneratedLabel(generated_entry, true);
    }
}

AppMCALabelHelper::~AppMCALabelHelper()
{
    ClearTrackLabels();

    size_t i;
    for (i = 0; i < mGeneratedEntries.size(); i++)
        delete mGeneratedEntries[i];
}

bool AppMCALabelHelper::IndexLabels(const MCALabelEntry *entries, size_t num_entries, bool override_duplicates)
{
    size_t i;
    for (i = 0; i < num_entries; i++) {
        const MCALabelEntry &entry = entries[i];
        if (!override_duplicates && mEntries.count(entry.tag_symbol)) {
            log_error("Duplicate audio label entry tag symbol '%s'\n", entry.tag_symbol);
            return false;
        }
        mEntries[entry.tag_symbol] = &entry;
    }

    return true;
}

bool AppMCALabelHelper::IndexGeneratedLabel(GeneratedMCALabelEntry *generated_entry, bool override_duplicates)
{
    if (!override_duplicates && mEntries.count(generated_entry->entry.tag_symbol)) {
        log_error("Duplicate audio label entry tag symbol '%s'\n", generated_entry->entry.tag_symbol);
        return false;
    }

    mEntries[generated_entry->entry.tag_symbol] = &generated_entry->entry;
    mGeneratedEntries.push_back(generated_entry);

    return true;
}

bool AppMCALabelHelper::ParseTrackLabels(const string &filename)
{
    ClearTrackLabels();

    ifstream input(filename.c_str());
    if (!input.is_open()) {
        log_error("Failed to open track labels file\n");
        return false;
    }

    int line_number = 0;
    try
    {
        vector<string> track_lines;
        string line;
        while (std::getline(input, line)) {
            size_t comment_hash = line.rfind("#");
            if (comment_hash != string::npos)
                line = line.substr(0, comment_hash);
            line = trim_string(line);
            if (line.empty()) {
                if (!track_lines.empty() && comment_hash == string::npos) {
                    ParseTrackLines(track_lines);
                    track_lines.clear();
                }
            } else {
                track_lines.push_back(line);
            }
            line_number++;
        }
        if (!track_lines.empty())
            ParseTrackLines(track_lines);

        return true;
    }
    catch (const BMXException &ex)
    {
        log_error("%s\n", ex.what());
        log_error("Failed to parse track labels, in label group ending at line %d\n", line_number);
        return false;
    }
    catch (...)
    {
        log_error("Failed to parse track labels, in label group ending at line %d\n", line_number);
        return false;
    }
}

void AppMCALabelHelper::InsertTrackLabels(ClipWriter *clip)
{
    BMX_CHECK(clip->GetOP1AClip() || clip->GetRDD9Clip());

    vector<ClipWriterTrack*> pcm_tracks;
    size_t i;
    for (i = 0; i < clip->GetNumTracks(); i++) {
        ClipWriterTrack *clip_track = clip->GetTrack((uint32_t)i);
        OP1APCMTrack *op1a_pcm_track = dynamic_cast<OP1APCMTrack*>(clip_track->GetOP1ATrack());
        RDD9PCMTrack *rdd9_pcm_track = dynamic_cast<RDD9PCMTrack*>(clip_track->GetRDD9Track());
        if (op1a_pcm_track || rdd9_pcm_track)
            pcm_tracks.push_back(clip_track);
    }


    map<string, SoundfieldGroupLabelSubDescriptor*> package_sg_desc_byid;
    map<string, GroupOfSoundfieldGroupsLabelSubDescriptor*> package_gosg_desc_byid;

    for (i = 0; i < mTrackLabels.size(); i++) {
        TrackLabels *track_labels = mTrackLabels[i];
        if (track_labels->soundfield_groups.empty())
            continue;

        if (track_labels->track_index >= pcm_tracks.size())
            BMX_EXCEPTION(("PCM (only) output track index %u does not exist", track_labels->track_index));
        ClipWriterTrack *pcm_track = pcm_tracks[track_labels->track_index];

        map<string, SoundfieldGroupLabelSubDescriptor*> track_sg_desc_byid;
        map<string, GroupOfSoundfieldGroupsLabelSubDescriptor*> track_gosg_desc_byid;

        size_t g;
        for (g = 0; g < track_labels->soundfield_groups.size(); g++) {
            SoundfieldGroup &sg = track_labels->soundfield_groups[g];

            set<UUID> gosg_link_ids;
            size_t l;
            for (l = 0; l < sg.gosg_label_lines.size(); l++) {
                LabelLine &gosg_label_line = sg.gosg_label_lines[l];

                GroupOfSoundfieldGroupsLabelSubDescriptor *gosg_desc;
                if (!gosg_label_line.id.empty() && package_gosg_desc_byid.count(gosg_label_line.id)) {
                    // Check that the properties, if present, are the same as the GOSG label with the same id
                    gosg_desc = package_gosg_desc_byid[gosg_label_line.id];
                    if (gosg_desc->getMCALabelDictionaryID() != gosg_label_line.label->dict_id) {
                        BMX_EXCEPTION(("Different group of soundfield group labels are using the same label id '%s'",
                                       gosg_label_line.id.c_str()));
                    }
                    if (!gosg_label_line.language.empty() && // allow non-first occurrence to not repeat language
                        (!gosg_desc->haveRFC5646SpokenLanguage() ||
                            gosg_label_line.language != gosg_desc->getRFC5646SpokenLanguage()))
                    {
                        BMX_EXCEPTION(("Group of soundfield group labels with id '%s' are using different languages",
                                       gosg_label_line.id.c_str()));
                    }

                    if (gosg_label_line.repeat && !track_gosg_desc_byid.count(gosg_label_line.id)) {
                        gosg_desc = pcm_track->AddGroupOfSoundfieldGroupLabel(gosg_desc);
                        track_gosg_desc_byid[gosg_label_line.id] = gosg_desc;
                    }
                } else {
                    if (!gosg_label_line.repeat) {
                        log_warn("Ignoring 'repeat' false attribute on first occurance of group of soundfield group label with id '%s'\n",
                                 gosg_label_line.id.c_str());
                    }

                    gosg_desc = pcm_track->AddGroupOfSoundfieldGroupLabel();
                    gosg_desc->setMCALabelDictionaryID(gosg_label_line.label->dict_id);
                    gosg_desc->setMCATagSymbol(gosg_label_line.label->tag_symbol);
                    if (gosg_label_line.label->tag_name && gosg_label_line.label->tag_name[0])
                        gosg_desc->setMCATagName(gosg_label_line.label->tag_name);
                    if (!gosg_label_line.language.empty())
                        gosg_desc->setRFC5646SpokenLanguage(gosg_label_line.language);

                    if (!gosg_label_line.id.empty()) {
                        package_gosg_desc_byid[gosg_label_line.id] = gosg_desc;
                        track_gosg_desc_byid[gosg_label_line.id] = gosg_desc;
                    }
                }

                gosg_link_ids.insert(gosg_desc->getMCALinkID());
            }

            SoundfieldGroupLabelSubDescriptor *sg_desc = 0;
            if (sg.sg_label_line.label) {
                LabelLine &sg_label_line = sg.sg_label_line;

                if (!sg_label_line.id.empty() && package_sg_desc_byid.count(sg_label_line.id)) {
                    // Check that the properties, if present, are the same as the SG label with the same id
                    sg_desc = package_sg_desc_byid[sg_label_line.id];
                    if (sg_desc->getMCALabelDictionaryID() != sg_label_line.label->dict_id) {
                        BMX_EXCEPTION(("Different soundfield group labels are using the same label id '%s'",
                                       sg_label_line.id.c_str()));
                    }
                    if (!sg_label_line.language.empty() &&  // allow non-first occurrence to not repeat language
                        (!sg_desc->haveRFC5646SpokenLanguage() ||
                            sg_label_line.language != sg_desc->getRFC5646SpokenLanguage()))
                    {
                        BMX_EXCEPTION(("Soundfield group labels with id '%s' are using different languages",
                                       sg_label_line.id.c_str()));
                    }
                    if (!gosg_link_ids.empty()) {
                        // Check that the GOSG are the same
                        vector<UUID> first_link_ids = sg_desc->getGroupOfSoundfieldGroupsLinkID();
                        if (first_link_ids.size() != gosg_link_ids.size()) {
                            BMX_EXCEPTION(("Different link id counts for same soundfield group labels with id '%s'",
                                        sg_label_line.id.c_str()));
                        }
                        size_t y;
                        for (y = 0; y < first_link_ids.size(); y++) {
                            if (!gosg_link_ids.count(first_link_ids[y])) {
                                BMX_EXCEPTION(("Different link ids for same soundfield group labels with id '%s'",
                                            sg_label_line.id.c_str()));
                            }
                        }
                    }

                    if (sg_label_line.repeat && !track_sg_desc_byid.count(sg_label_line.id)) {
                        sg_desc = pcm_track->AddSoundfieldGroupLabel(sg_desc);
                        track_sg_desc_byid[sg_label_line.id] = sg_desc;
                    }
                } else {
                    if (!sg_label_line.repeat) {
                        log_warn("Ignoring 'repeat' false attribute on first occurance of soundfield group label with id '%s'\n",
                                 sg_label_line.id.c_str());
                    }

                    sg_desc = pcm_track->AddSoundfieldGroupLabel();
                    sg_desc->setMCALabelDictionaryID(sg_label_line.label->dict_id);
                    sg_desc->setMCATagSymbol(sg_label_line.label->tag_symbol);
                    if (sg_label_line.label->tag_name && sg_label_line.label->tag_name[0])
                        sg_desc->setMCATagName(sg_label_line.label->tag_name);
                    if (!sg_label_line.language.empty())
                        sg_desc->setRFC5646SpokenLanguage(sg_label_line.language);

                    set<UUID>::const_iterator iter;
                    for (iter = gosg_link_ids.begin(); iter != gosg_link_ids.end(); iter++)
                        sg_desc->appendGroupOfSoundfieldGroupsLinkID(*iter);

                    if (!sg_label_line.id.empty()) {
                        package_sg_desc_byid[sg_label_line.id] = sg_desc;
                        track_sg_desc_byid[sg_label_line.id] = sg_desc;
                    }
                }
            }

            size_t c;
            for (c = 0; c < sg.c_label_lines.size(); c++) {
                LabelLine &c_label_line = sg.c_label_lines[c];

                if (c_label_line.channel_index != (uint32_t)(-1)) {
                    if (c_label_line.channel_index >= pcm_track->GetChannelCount()) {
                        BMX_EXCEPTION(("Channel label channel index %u >= track channel count %u",
                                       c_label_line.channel_index, pcm_track->GetChannelCount()));
                    }
                } else if (pcm_track->GetChannelCount() != 1) {
                    BMX_EXCEPTION(("Missing channel label channel index in track with %u channels",
                                   pcm_track->GetChannelCount()));
                }

                AudioChannelLabelSubDescriptor *a_desc = pcm_track->AddAudioChannelLabel();
                if (c_label_line.channel_index != (uint32_t)(-1))
                    a_desc->setMCAChannelID(c_label_line.channel_index + 1); // +1 because MCAChannelID starts from 1
                a_desc->setMCALabelDictionaryID(c_label_line.label->dict_id);
                a_desc->setMCATagSymbol(c_label_line.label->tag_symbol);
                if (c_label_line.label->tag_name && c_label_line.label->tag_name[0])
                    a_desc->setMCATagName(c_label_line.label->tag_name);
                if (!c_label_line.language.empty())
                    a_desc->setRFC5646SpokenLanguage(c_label_line.language);

                if (sg_desc)
                    a_desc->setSoundfieldGroupLinkID(sg_desc->getMCALinkID());
            }
        }
    }
}

void AppMCALabelHelper::ParseTrackLines(const vector<string> &lines)
{
    TrackLabels *track_labels = 0;
    try
    {
        BMX_ASSERT(!lines.empty());

        unsigned int track_index;
        if (sscanf(lines[0].c_str(), "%u", &track_index) != 1)
            throw BMXException("Failed to parse audio track index from '%s'", lines[0].c_str());
        if (mTrackLabelsByTrackIndex.count(track_index))
            throw BMXException("Duplicate audio track index %u", track_index);

        track_labels = new TrackLabels();
        track_labels->track_index = track_index;

        SoundfieldGroup sg;
        size_t i;
        for (i = 1; i < lines.size(); i++) {
            LabelLine label_line = ParseLabelLine(lines[i]);

            if (label_line.label->type == AUDIO_CHANNEL_LABEL) {
                if (!sg.gosg_label_lines.empty() || sg.sg_label_line.label) {
                    track_labels->soundfield_groups.push_back(sg);
                    sg.Reset();
                }
                sg.c_label_lines.push_back(label_line);
            } else if (label_line.label->type == SOUNDFIELD_GROUP_LABEL) {
                if (sg.c_label_lines.empty())
                    throw BMXException("No channels in soundfield group");
                if (sg.sg_label_line.label)
                    throw BMXException("Multiple soundfield group labels");
                sg.sg_label_line = label_line;
            } else { // GROUP_OF_SOUNDFIELD_GROUP_LABEL
                if (!sg.sg_label_line.label)
                    throw BMXException("No soundfield group label before group of soundfield groups label");
                sg.gosg_label_lines.push_back(label_line);
            }
        }
        if (!sg.gosg_label_lines.empty() || sg.sg_label_line.label || !sg.c_label_lines.empty()) {
            track_labels->soundfield_groups.push_back(sg);
            sg.Reset();
        }

        mTrackLabels.push_back(track_labels);
        mTrackLabelsByTrackIndex[track_index] = track_labels;
    }
    catch (...)
    {
        delete track_labels;
        throw;
    }
}

AppMCALabelHelper::LabelLine AppMCALabelHelper::ParseLabelLine(const string &line)
{
    LabelLine label_line;

    vector<string> elements = split_string(line, ',', false, true);
    BMX_ASSERT(!elements.empty());

    string label_str = elements[0];
    label_line.label = Find(label_str);
    if (!label_line.label)
        throw BMXException("Unknown audio label '%s'", label_str.c_str());

    size_t i;
    for (i = 1; i < elements.size(); i++) {
        vector<string> nv_pair = split_string(elements[i], '=', false, true);
        if (nv_pair.size() != 2)
            throw BMXException("Invalid audio label <name>=<value> attribute string '%s'", elements[i].c_str());
        string name  = nv_pair[0];
        string value = nv_pair[1];
        if (name == "id") {
            label_line.id = value;
        } else if (name == "chan") {
            unsigned int channel_index;
            if (sscanf(value.c_str(), "%u", &channel_index) != 1)
                throw BMXException("Failed to parse channel index from line '%s'", elements[i].c_str());
            label_line.channel_index = channel_index;
        } else if (name == "lang") {
            label_line.language = value;
        } else if (name == "repeat") {
            if (value == "0" || value == "false" || value == "no")
                label_line.repeat = false;
            else
                label_line.repeat = true;
        } else {
            log_warn("Ignoring unknown audio label attribute '%s'\n", elements[i].c_str());
        }
    }

    return label_line;
}

const MCALabelEntry* AppMCALabelHelper::Find(const string &id_str)
{
    if (mEntries.count(id_str))
        return mEntries[id_str];
    else
        return 0;
}

void AppMCALabelHelper::ClearTrackLabels()
{
    size_t i;
    for (i = 0; i < mTrackLabels.size(); i++)
        delete mTrackLabels[i];
    mTrackLabels.clear();
    mTrackLabelsByTrackIndex.clear();
}
