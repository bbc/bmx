/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <libMXF++/MXF.h>

#include <bmx/BMXTypes.h>
#include <bmx/mxf_reader/MXFTrackInfo.h>
#include <bmx/st436/ST436Element.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



MXFTrackInfo::MXFTrackInfo()
{
    data_def = MXF_UNKNOWN_DDEF;

    essence_type = UNKNOWN_ESSENCE_TYPE;
    essence_container_label = g_Null_UL;
    material_package_uid = g_Null_UMID;
    material_track_id = 0;
    material_track_number = 0;
    file_package_uid = g_Null_UMID;
    edit_rate = ZERO_RATIONAL;
    duration = 0;
    lead_filler_offset = 0;
    file_track_id = 0;
    file_track_number = 0;
};

bool MXFTrackInfo::IsCompatible(const MXFTrackInfo *right) const
{
    return essence_type == right->essence_type &&
           edit_rate    == right->edit_rate;
}

void MXFTrackInfo::Clone(MXFTrackInfo *clone) const
{
    clone->data_def                 = data_def;
    clone->essence_type             = essence_type;
    clone->essence_container_label  = essence_container_label;
    clone->material_package_uid     = material_package_uid;
    clone->material_track_id        = material_track_id;
    clone->material_track_number    = material_track_number;
    clone->file_package_uid         = file_package_uid;
    clone->edit_rate                = edit_rate;
    clone->duration                 = duration;
    clone->lead_filler_offset       = lead_filler_offset;
    clone->file_track_id            = file_track_id;
    clone->file_track_number        = file_track_number;
}




MXFPictureTrackInfo::MXFPictureTrackInfo()
: MXFTrackInfo()
{
    data_def = MXF_PICTURE_DDEF;

    is_cdci = true;
    picture_essence_coding_label = g_Null_UL;
    signal_standard = 0;
    stored_width = 0;
    stored_height = 0;
    display_width = 0;
    display_height = 0;
    display_x_offset = 0;
    display_y_offset = 0;
    aspect_ratio = ZERO_RATIONAL;
    frame_layout = 0xff;
    afd = 0;
    horiz_subsampling = 0;
    vert_subsampling = 1;
    component_depth = 0;
    color_siting = MXF_COLOR_SITING_UNKNOWN;
    have_avci_header = false;
};

bool MXFPictureTrackInfo::IsCompatible(const MXFTrackInfo *right) const
{
    const MXFPictureTrackInfo *picture_right = dynamic_cast<const MXFPictureTrackInfo*>(right);
    if (!picture_right)
        return false;

    return MXFTrackInfo::IsCompatible(right) &&
           is_cdci                       == picture_right->is_cdci &&
           picture_essence_coding_label  == picture_right->picture_essence_coding_label &&
           signal_standard               == picture_right->signal_standard &&
           stored_width                  == picture_right->stored_width &&
           stored_height                 == picture_right->stored_height &&
           display_width                 == picture_right->display_width &&
           display_height                == picture_right->display_height &&
           display_x_offset              == picture_right->display_x_offset &&
           display_y_offset              == picture_right->display_y_offset &&
           aspect_ratio                  == picture_right->aspect_ratio &&
           frame_layout                  == picture_right->frame_layout &&
           afd                           == picture_right->afd &&
           (!is_cdci ||
               (component_depth          == picture_right->component_depth &&
                horiz_subsampling        == picture_right->horiz_subsampling &&
                vert_subsampling         == picture_right->vert_subsampling &&
                color_siting             == picture_right->color_siting &&
                have_avci_header         == picture_right->have_avci_header));
}

MXFTrackInfo* MXFPictureTrackInfo::Clone() const
{
    MXFPictureTrackInfo *clone = new MXFPictureTrackInfo();

    MXFTrackInfo::Clone(clone);

    clone->is_cdci                       = is_cdci;
    clone->picture_essence_coding_label  = picture_essence_coding_label;
    clone->signal_standard               = signal_standard;
    clone->stored_width                  = stored_width;
    clone->stored_height                 = stored_height;
    clone->display_width                 = display_width;
    clone->display_height                = display_height;
    clone->display_x_offset              = display_x_offset;
    clone->display_y_offset              = display_y_offset;
    clone->aspect_ratio                  = aspect_ratio;
    clone->frame_layout                  = frame_layout;
    clone->afd                           = afd;
    clone->horiz_subsampling             = horiz_subsampling;
    clone->vert_subsampling              = vert_subsampling;
    clone->component_depth               = component_depth;
    clone->color_siting                  = color_siting;
    clone->have_avci_header              = have_avci_header;

    return clone;
}



MXFSoundTrackInfo::MXFSoundTrackInfo()
: MXFTrackInfo()
{
    data_def = MXF_SOUND_DDEF;

    sampling_rate = ZERO_RATIONAL;
    bits_per_sample = 0;
    block_align = 0;
    channel_count = 0;
    sequence_offset = 0;
    locked = false;
    locked_set = false;
    audio_ref_level = 0;
    audio_ref_level_set = false;
    dial_norm = 0;
    dial_norm_set = false;
    d10_aes3_valid_flags = 0;
}

bool MXFSoundTrackInfo::IsCompatible(const MXFTrackInfo *right) const
{
    const MXFSoundTrackInfo *sound_right = dynamic_cast<const MXFSoundTrackInfo*>(right);
    if (!sound_right)
        return false;

    return MXFTrackInfo::IsCompatible(right) &&
           sampling_rate        == sound_right->sampling_rate &&
           bits_per_sample      == sound_right->bits_per_sample &&
           block_align          == sound_right->block_align &&
           channel_count        == sound_right->channel_count &&
           d10_aes3_valid_flags == sound_right->d10_aes3_valid_flags &&
           sequence_offset      == sound_right->sequence_offset;
}

MXFTrackInfo* MXFSoundTrackInfo::Clone() const
{
    MXFSoundTrackInfo *clone = new MXFSoundTrackInfo();

    MXFTrackInfo::Clone(clone);

    clone->sampling_rate        = sampling_rate;
    clone->bits_per_sample      = bits_per_sample;
    clone->block_align          = block_align;
    clone->channel_count        = channel_count;
    clone->d10_aes3_valid_flags = d10_aes3_valid_flags;
    clone->sequence_offset      = sequence_offset;
    clone->locked               = locked;
    clone->locked_set           = locked_set;
    clone->audio_ref_level      = audio_ref_level;
    clone->audio_ref_level_set  = audio_ref_level_set;
    clone->dial_norm            = dial_norm;
    clone->dial_norm_set        = dial_norm_set;

    return clone;
}



VBIManifestElement::VBIManifestElement()
{
    line_number = 0;
    wrapping_type = 0;
    sample_coding = 0;
}

void VBIManifestElement::Parse(const ST436Line *line)
{
    BMX_CHECK(line->is_vbi);

    line_number   = line->line_number;
    wrapping_type = line->wrapping_type;
    sample_coding = line->payload_sample_coding;
}

bool VBIManifestElement::operator==(const VBIManifestElement &right) const
{
    return line_number   == right.line_number &&
           wrapping_type == right.wrapping_type &&
           sample_coding == right.sample_coding;
}



ANCManifestElement::ANCManifestElement()
{
    line_number = 0;
    wrapping_type = 0;
    sample_coding = 0;
    did = 0;
    sdid = 0;
}

void ANCManifestElement::Parse(const ST436Line *line)
{
    BMX_CHECK(!line->is_vbi);

    line_number   = line->line_number;
    wrapping_type = line->wrapping_type;
    sample_coding = line->payload_sample_coding;
    did           = 0;
    sdid          = 0;

    if (line->payload_sample_coding == ANC_8_BIT_COMP_LUMA ||
        line->payload_sample_coding == ANC_8_BIT_COMP_COLOR ||
        line->payload_sample_coding == ANC_8_BIT_COMP_LUMA_COLOR ||
        line->payload_sample_coding == ANC_8_BIT_COMP_LUMA_ERROR ||
        line->payload_sample_coding == ANC_8_BIT_COMP_COLOR_ERROR ||
        line->payload_sample_coding == ANC_8_BIT_COMP_LUMA_COLOR_ERROR)
    {
        if (line->payload_size > 0) {
            did = line->payload_data[0];
            if (did && line->payload_size > 1)
                sdid = line->payload_data[1];
        }
    }
    else if (line->payload_sample_coding == ANC_10_BIT_COMP_LUMA ||
             line->payload_sample_coding == ANC_10_BIT_COMP_COLOR ||
             line->payload_sample_coding == ANC_10_BIT_COMP_LUMA_COLOR)
    {
        // 8-bit ANC packet coding contains _lower-order_ 8 bits of 10-bit samples
        // the parity and inverted parity high-order bits are lost
        if (line->payload_size > 1) {
            did = ((line->payload_data[0] & 0x3f) << 2) |
                  ((line->payload_data[1] & 0xc0) >> 6);
            if (did && line->payload_size > 2) {
                sdid = ((line->payload_data[1] & 0x0f) << 4) |
                       ((line->payload_data[2] & 0xf0) >> 4);
            }
        }
    }
    else
    {
        log_debug("Unsupported sample coding %u for ANC data manifest extraction\n",
                  line->payload_sample_coding);
    }
}

bool ANCManifestElement::operator==(const ANCManifestElement &right) const
{
    return line_number   == right.line_number &&
           wrapping_type == right.wrapping_type &&
           sample_coding == right.sample_coding &&
           did           == right.did &&
           sdid          == right.sdid;
}



MXFDataTrackInfo::MXFDataTrackInfo()
: MXFTrackInfo()
{
    data_def = MXF_DATA_DDEF;
}

bool MXFDataTrackInfo::IsCompatible(const MXFTrackInfo *right) const
{
    const MXFDataTrackInfo *data_right = dynamic_cast<const MXFDataTrackInfo*>(right);
    if (!data_right)
        return false;

    return MXFTrackInfo::IsCompatible(right);
}

MXFTrackInfo* MXFDataTrackInfo::Clone() const
{
    MXFDataTrackInfo *clone = new MXFDataTrackInfo();

    MXFTrackInfo::Clone(clone);

    clone->vbi_manifest.assign(vbi_manifest.begin(), vbi_manifest.end());
    clone->anc_manifest.assign(anc_manifest.begin(), anc_manifest.end());

    return clone;
}

void MXFDataTrackInfo::AppendUniqueVBIElement(const VBIManifestElement &element)
{
    size_t i;
    for (i = 0; i < vbi_manifest.size(); i++) {
        if (vbi_manifest[i] == element)
            return;
    }

    vbi_manifest.push_back(element);
}

void MXFDataTrackInfo::AppendUniqueANCElement(const ANCManifestElement &element)
{
    size_t i;
    for (i = 0; i < anc_manifest.size(); i++) {
        if (anc_manifest[i] == element)
            return;
    }

    anc_manifest.push_back(element);
}

