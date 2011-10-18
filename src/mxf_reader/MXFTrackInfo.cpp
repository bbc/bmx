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

#include <cstring>

#include <libMXF++/MXF.h>

#include <im/mxf_reader/MXFTrackInfo.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



MXFTrackInfo::MXFTrackInfo()
{
    is_picture = false;
    is_sound = false;

    essence_type = MXFDescriptorHelper::UNKNOWN_ESSENCE;
    memset(&essence_container_label, 0, sizeof(essence_container_label));
    memset(&material_package_uid, 0, sizeof(material_package_uid));
    material_track_id = 0;
    material_track_number = 0;
    memset(&file_package_uid, 0, sizeof(file_package_uid));
    memset(&edit_rate, 0, sizeof(edit_rate));
    duration = 0;
    lead_filler_offset = 0;
    file_track_id = 0;
    file_track_number = 0;
};

bool MXFTrackInfo::IsD10Audio() const
{
    return is_sound &&
           (mxf_equals_ul_mod_regver(&essence_container_label, &MXF_EC_L(D10_30_625_50_defined_template)) ||
            mxf_equals_ul_mod_regver(&essence_container_label, &MXF_EC_L(D10_30_525_60_defined_template)) ||
            mxf_equals_ul_mod_regver(&essence_container_label, &MXF_EC_L(D10_40_625_50_defined_template)) ||
            mxf_equals_ul_mod_regver(&essence_container_label, &MXF_EC_L(D10_40_525_60_defined_template)) ||
            mxf_equals_ul_mod_regver(&essence_container_label, &MXF_EC_L(D10_50_625_50_defined_template)) ||
            mxf_equals_ul_mod_regver(&essence_container_label, &MXF_EC_L(D10_50_525_60_defined_template)));
}

bool MXFTrackInfo::IsCompatible(const MXFTrackInfo *right) const
{
    return essence_type == right->essence_type &&
           edit_rate    == right->edit_rate;
}

void MXFTrackInfo::Clone(MXFTrackInfo *clone) const
{
    clone->is_picture               = is_picture;
    clone->is_sound                 = is_sound;
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
    is_picture = true;

    memset(&picture_essence_coding_label, 0, sizeof(picture_essence_coding_label));
    signal_standard = 0;
    stored_width = 0;
    stored_height = 0;
    display_width = 0;
    display_height = 0;
    display_x_offset = 0;
    display_y_offset = 0;
    horiz_subsampling = 0;
    vert_subsampling = 1;
    component_depth = 0;
    memset(&aspect_ratio, 0, sizeof(aspect_ratio));
    frame_layout = 0xff;
    color_siting = MXF_COLOR_SITING_UNKNOWN;
    afd = 0;
};

bool MXFPictureTrackInfo::IsCompatible(const MXFTrackInfo *right) const
{
    const MXFPictureTrackInfo *picture_right = dynamic_cast<const MXFPictureTrackInfo*>(right);
    if (!picture_right)
        return false;

    return MXFTrackInfo::IsCompatible(right) &&
           picture_essence_coding_label  == picture_right->picture_essence_coding_label &&
           signal_standard               == picture_right->signal_standard &&
           stored_width                  == picture_right->stored_width &&
           stored_height                 == picture_right->stored_height &&
           display_width                 == picture_right->display_width &&
           display_height                == picture_right->display_height &&
           display_x_offset              == picture_right->display_x_offset &&
           display_y_offset              == picture_right->display_y_offset &&
           horiz_subsampling             == picture_right->horiz_subsampling &&
           vert_subsampling              == picture_right->vert_subsampling &&
           component_depth               == picture_right->component_depth &&
           aspect_ratio                  == picture_right->aspect_ratio &&
           frame_layout                  == picture_right->frame_layout &&
           color_siting                  == picture_right->color_siting &&
           afd                           == picture_right->afd;
}

MXFTrackInfo* MXFPictureTrackInfo::Clone() const
{
    MXFPictureTrackInfo *clone = new MXFPictureTrackInfo();

    MXFTrackInfo::Clone(clone);

    clone->picture_essence_coding_label  = picture_essence_coding_label;
    clone->signal_standard               = signal_standard;
    clone->stored_width                  = stored_width;
    clone->stored_height                 = stored_height;
    clone->display_width                 = display_width;
    clone->display_height                = display_height;
    clone->display_x_offset              = display_x_offset;
    clone->display_y_offset              = display_y_offset;
    clone->horiz_subsampling             = horiz_subsampling;
    clone->vert_subsampling              = vert_subsampling;
    clone->component_depth               = component_depth;
    clone->aspect_ratio                  = aspect_ratio;
    clone->frame_layout                  = frame_layout;
    clone->color_siting                  = color_siting;
    clone->afd                           = afd;

    return clone;
}



MXFSoundTrackInfo::MXFSoundTrackInfo()
: MXFTrackInfo()
{
    is_sound = true;

    memset(&sampling_rate, 0, sizeof(sampling_rate));
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
    clone->sequence_offset      = sequence_offset;
    clone->locked               = locked;
    clone->locked_set           = locked_set;
    clone->audio_ref_level      = audio_ref_level;
    clone->audio_ref_level_set  = audio_ref_level_set;
    clone->dial_norm            = dial_norm;
    clone->dial_norm_set        = dial_norm_set;

    return clone;
}

