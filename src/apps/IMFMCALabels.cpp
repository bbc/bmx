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

#define __STDC_FORMAT_MACROS

#include <bmx/apps/IMFMCALabels.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



// Multi-channel audio labels from SMPTE ST 428-12:2013

// Note that symbols are defined in ST 428-12 and ST 2067-8 to have a minimum of 1 and
// maximum of 6 characters whilst SMPTE ST 377-4:2021 requires minimum of 2 characters
// and maximum of 6, where the start is a alpha character.

#define DC_MCA_LABEL(type, octet11) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x03, 0x02, type, octet11, 0x00, 0x00, 0x00, 0x00}

#define DC_CHANNEL_LABEL(octet11)  DC_MCA_LABEL(0x01, octet11)
#define DC_SOUNDFIELD_GROUP_LABEL(octet11)  DC_MCA_LABEL(0x02, octet11)


static const MCALabelEntry DC_MCA_LABELS[] =
{
    // Channels
    {AUDIO_CHANNEL_LABEL, "L", "Left", DC_CHANNEL_LABEL(0x01)},
    {AUDIO_CHANNEL_LABEL, "R", "Right", DC_CHANNEL_LABEL(0x02)},
    {AUDIO_CHANNEL_LABEL, "C", "Center", DC_CHANNEL_LABEL(0x03)},
    {AUDIO_CHANNEL_LABEL, "LFE", "LFE", DC_CHANNEL_LABEL(0x04)},
    {AUDIO_CHANNEL_LABEL, "Ls", "Left Surround", DC_CHANNEL_LABEL(0x05)},
    {AUDIO_CHANNEL_LABEL, "Rs", "Right Surround", DC_CHANNEL_LABEL(0x06)},
    {AUDIO_CHANNEL_LABEL, "Lss", "Left Side Surround", DC_CHANNEL_LABEL(0x07)},
    {AUDIO_CHANNEL_LABEL, "Rss", "Right Side Surround", DC_CHANNEL_LABEL(0x08)},
    {AUDIO_CHANNEL_LABEL, "Lrs", "Left Rear Surround", DC_CHANNEL_LABEL(0x09)},
    {AUDIO_CHANNEL_LABEL, "Rrs", "Right Rear Surround", DC_CHANNEL_LABEL(0x0a)},
    {AUDIO_CHANNEL_LABEL, "Lc", "Left Center", DC_CHANNEL_LABEL(0x0b)},
    {AUDIO_CHANNEL_LABEL, "Rc", "Right Center", DC_CHANNEL_LABEL(0x0c)},
    {AUDIO_CHANNEL_LABEL, "Cs", "Center Surround", DC_CHANNEL_LABEL(0x0d)},
    {AUDIO_CHANNEL_LABEL, "HI", "Hearing Impaired", DC_CHANNEL_LABEL(0x0e)},
    {AUDIO_CHANNEL_LABEL, "VIN", "Visually Impaired-Narrative", DC_CHANNEL_LABEL(0x0f)},

    // Soundfield Groups
    {SOUNDFIELD_GROUP_LABEL, "51", "5.1", DC_SOUNDFIELD_GROUP_LABEL(0x01)},
    {SOUNDFIELD_GROUP_LABEL, "71", "7.1DS", DC_SOUNDFIELD_GROUP_LABEL(0x02)},
    {SOUNDFIELD_GROUP_LABEL, "SDS", "7.1SDS", DC_SOUNDFIELD_GROUP_LABEL(0x03)},
    {SOUNDFIELD_GROUP_LABEL, "61", "6.1", DC_SOUNDFIELD_GROUP_LABEL(0x04)},
    {SOUNDFIELD_GROUP_LABEL, "M", "Monoaural", DC_SOUNDFIELD_GROUP_LABEL(0x05)},
};


// Multi-channel audio labels from SMPTE ST 2067-8:2013

#define IMF_MCA_LABEL(type, octet12, octet13) \
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x03, 0x02, type, 0x20, octet12, octet13, 0x00, 0x00}

#define IMF_CHANNEL_LABEL(octet12, octet13)  IMF_MCA_LABEL(0x01, octet12, octet13)
#define IMF_SOUNDFIELD_GROUP_LABEL(octet12)  IMF_MCA_LABEL(0x02, octet12, 0x00)
#define IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(octet12)  IMF_MCA_LABEL(0x03, octet12, 0x00)


static const MCALabelEntry IMF_MCA_LABELS[] =
{
    //  Channels
    {AUDIO_CHANNEL_LABEL, "M1", "Mono One", IMF_CHANNEL_LABEL(0x01, 0x00)},
    {AUDIO_CHANNEL_LABEL, "M2", "Mono Two", IMF_CHANNEL_LABEL(0x02, 0x00)},
    {AUDIO_CHANNEL_LABEL, "Lt", "Left Total", IMF_CHANNEL_LABEL(0x03, 0x00)},
    {AUDIO_CHANNEL_LABEL, "Rt", "Right Total", IMF_CHANNEL_LABEL(0x04, 0x00)},
    {AUDIO_CHANNEL_LABEL, "Lst", "Left Surround Total", IMF_CHANNEL_LABEL(0x05, 0x00)},
    {AUDIO_CHANNEL_LABEL, "Rst", "Right Surround Total", IMF_CHANNEL_LABEL(0x06, 0x00)},
    {AUDIO_CHANNEL_LABEL, "S", "Surround", IMF_CHANNEL_LABEL(0x07, 0x00)},

    // Soundfield Groups
    {SOUNDFIELD_GROUP_LABEL, "ST", "Standard Stereo", IMF_SOUNDFIELD_GROUP_LABEL(0x01)},
    {SOUNDFIELD_GROUP_LABEL, "DM", "Dual Mono", IMF_SOUNDFIELD_GROUP_LABEL(0x02)},
    {SOUNDFIELD_GROUP_LABEL, "DNS", "Discrete Numbered Sources", IMF_SOUNDFIELD_GROUP_LABEL(0x03)},
    {SOUNDFIELD_GROUP_LABEL, "30", "3.0", IMF_SOUNDFIELD_GROUP_LABEL(0x04)},
    {SOUNDFIELD_GROUP_LABEL, "40", "4.0", IMF_SOUNDFIELD_GROUP_LABEL(0x05)},
    {SOUNDFIELD_GROUP_LABEL, "50", "5.0", IMF_SOUNDFIELD_GROUP_LABEL(0x06)},
    {SOUNDFIELD_GROUP_LABEL, "60", "6.0", IMF_SOUNDFIELD_GROUP_LABEL(0x07)},
    {SOUNDFIELD_GROUP_LABEL, "70", "7.0DS", IMF_SOUNDFIELD_GROUP_LABEL(0x08)},
    {SOUNDFIELD_GROUP_LABEL, "LtRt", "Lt-Rt", IMF_SOUNDFIELD_GROUP_LABEL(0x09)},
    {SOUNDFIELD_GROUP_LABEL, "51EX", "5.1EX", IMF_SOUNDFIELD_GROUP_LABEL(0x0a)},
    {SOUNDFIELD_GROUP_LABEL, "HA", "Hearing Accessibility", IMF_SOUNDFIELD_GROUP_LABEL(0x0b)},
    {SOUNDFIELD_GROUP_LABEL, "VA", "Visual Accessibility", IMF_SOUNDFIELD_GROUP_LABEL(0x0c)},

    // Group of Soundfield Groups
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "MPg", "Main Program", IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x01)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "DVS", "Descriptive Video Service", IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x02)},
    {GROUP_OF_SOUNDFIELD_GROUP_LABEL, "Dcm", "Dialog Centric Mix", IMF_GROUP_OF_SOUNDFIELD_GROUP_LABEL(0x03)},
};


bool bmx::index_imf_mca_labels(AppMCALabelHelper *labels_helper)
{
    labels_helper->IndexLabels(DC_MCA_LABELS, BMX_ARRAY_SIZE(DC_MCA_LABELS), true);
    labels_helper->IndexLabels(IMF_MCA_LABELS, BMX_ARRAY_SIZE(IMF_MCA_LABELS), true);

    // Generate numbered source audio labels
    char buffer[32];
    uint8_t i;
    for (i = 1; i < 128; i++) {
        GeneratedMCALabelEntry *generated_entry = new GeneratedMCALabelEntry();

        bmx_snprintf(buffer, sizeof(buffer), "NSC%03d", i);
        generated_entry->gen_tag_symbol = buffer;

        bmx_snprintf(buffer, sizeof(buffer), "Numbered Source Channel %03d", i);
        generated_entry->gen_tag_name = buffer;

        generated_entry->entry.type = AUDIO_CHANNEL_LABEL;
        generated_entry->entry.tag_symbol = generated_entry->gen_tag_symbol.c_str();
        generated_entry->entry.tag_name = generated_entry->gen_tag_name.c_str();
        generated_entry->entry.dict_id = IMF_CHANNEL_LABEL(0x08, i);

        labels_helper->IndexGeneratedLabel(generated_entry, true);
    }

    return true;
}
