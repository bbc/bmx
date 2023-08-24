/*
 * MXF labels, keys, track numbers, etc.
 *
 * Copyright (C) 2006, British Broadcasting Corporation
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

#include <stdlib.h>
#include <string.h>

#include <mxf/mxf.h>



mxfKey g_KLVFill_key = /* g_LegacyKLVFill_key */
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x02, 0x10, 0x01, 0x00, 0x00, 0x00};



void mxf_set_generalized_op_label(mxfUL *label, int item_complexity, int package_complexity, int qualifier)
{
    static const mxfUL base_label = MXF_GEN_OP_L(0x00, 0x00, 0x00);

    memcpy(label, &base_label, sizeof(*label));
    label->octet12 = item_complexity;
    label->octet13 = package_complexity;
    label->octet14 = qualifier;
}

int mxf_is_op_atom(const mxfUL *label)
{
    static const mxfUL opAtomPrefix = MXF_ATOM_OP_L(0);

    /* ignoring octet7, the registry version byte */
    return memcmp(label,          &opAtomPrefix,        7) == 0 &&
           memcmp(&label->octet8, &opAtomPrefix.octet8, 5) == 0;
}

int mxf_is_op_1a(const mxfUL *label)
{
    static const mxfUL op1APrefix = MXF_GEN_OP_L(0x01, 0x01, 0x00);

    /* ignoring octet7, the registry version byte */
    return memcmp(label,          &op1APrefix,        7) == 0 &&
           memcmp(&label->octet8, &op1APrefix.octet8, 5) == 0;
}

int mxf_is_op_1b(const mxfUL *label)
{
    static const mxfUL op1BPrefix = MXF_GEN_OP_L(0x01, 0x02, 0x00);

    /* ignoring octet7, the registry version byte */
    return memcmp(label,          &op1BPrefix,        7) == 0 &&
           memcmp(&label->octet8, &op1BPrefix.octet8, 5) == 0;
}



int mxf_is_picture(const mxfUL *label)
{
    return memcmp(label, &MXF_DDEF_L(Picture), sizeof(mxfUL)) == 0 ||
           memcmp(label, &MXF_DDEF_L(LegacyPicture), sizeof(mxfUL)) == 0;
}

int mxf_is_sound(const mxfUL *label)
{
    return memcmp(label, &MXF_DDEF_L(Sound), sizeof(mxfUL)) == 0 ||
           memcmp(label, &MXF_DDEF_L(LegacySound), sizeof(mxfUL)) == 0;
}

int mxf_is_timecode(const mxfUL *label)
{
    return memcmp(label, &MXF_DDEF_L(Timecode), sizeof(mxfUL)) == 0 ||
           memcmp(label, &MXF_DDEF_L(LegacyTimecode), sizeof(mxfUL)) == 0;
}

int mxf_is_data(const mxfUL *label)
{
    return memcmp(label, &MXF_DDEF_L(Data), sizeof(mxfUL)) == 0;
}

int mxf_is_descriptive_metadata(const mxfUL *label)
{
    return memcmp(label, &MXF_DDEF_L(DescriptiveMetadata), sizeof(mxfUL)) == 0;
}

MXFDataDefEnum mxf_get_ddef_enum(const mxfUL *label)
{
    if (mxf_is_picture(label))
        return MXF_PICTURE_DDEF;
    else if (mxf_is_sound(label))
        return MXF_SOUND_DDEF;
    else if (mxf_is_timecode(label))
        return MXF_TIMECODE_DDEF;
    else if (mxf_is_data(label))
        return MXF_DATA_DDEF;
    else if (mxf_is_descriptive_metadata(label))
        return MXF_DM_DDEF;
    else
        return MXF_UNKNOWN_DDEF;
}

int mxf_get_ddef_label(MXFDataDefEnum data_def, mxfUL *label)
{
    switch (data_def)
    {
        case MXF_PICTURE_DDEF:
            memcpy(label, &MXF_DDEF_L(Picture), sizeof(*label));
            return 1;
        case MXF_SOUND_DDEF:
            memcpy(label, &MXF_DDEF_L(Sound), sizeof(*label));
            return 1;
        case MXF_TIMECODE_DDEF:
            memcpy(label, &MXF_DDEF_L(Timecode), sizeof(*label));
            return 1;
        case MXF_DATA_DDEF:
            memcpy(label, &MXF_DDEF_L(Data), sizeof(*label));
            return 1;
        case MXF_DM_DDEF:
            memcpy(label, &MXF_DDEF_L(DescriptiveMetadata), sizeof(*label));
            return 1;
        case MXF_UNKNOWN_DDEF:
            break;
    }

    return 0;
}


void mxf_get_jpeg2000_coding_label(uint16_t profile, uint8_t main_level, uint8_t sub_level, mxfUL *label)
{
    uint8_t variant = 0x00;
    uint8_t constraints = 0x7f;
    if (profile == 0x00 && main_level == 255 && sub_level == 255) {
        variant = 0x01;
        constraints = 0x7f;
    } else if (profile >= 0x03 && profile < 0x05 && main_level == 255 && sub_level == 255) {
        variant = 0x01;
        constraints = (uint8_t)profile;
    } else if (profile == 0x10 && main_level > 0 && main_level < 8 && sub_level == 255) {
        variant = 0x01;
        constraints = profile + main_level;
    } else if (profile >= 0x04 && profile < 0x0a) {
        if (main_level == 0 && sub_level == 0) {
            constraints = 0x01;
        } else if (main_level > 0 && main_level < 4 && sub_level < 2) {
            constraints = main_level * 2 + sub_level;
        } else if (main_level >= 4 && main_level < 12 && sub_level < main_level - 1) {
            constraints = 0x08;
            for (uint8_t ml = 4; ml < main_level; ml++)
                constraints += ml - 1;
            constraints += sub_level;
        }

        if (constraints != 0x7f)
            variant = profile - 2;
    }

    if (variant == 0x00) {
        // Use the generic variant if the profile, main level or sub level are unknown
        variant = 0x01;
        constraints = 0x7f;
    }

    memcpy(label, &MXF_CMDEF_L(JPEG2000_UNDEFINED), sizeof(*label));
    label->octet14 = variant;
    label->octet15 = constraints;
}


int mxf_is_generic_container_label(const mxfUL *label)
{
    static const mxfUL gcLabel = MXF_GENERIC_CONTAINER_LABEL(0x00, 0x00, 0x00, 0x00);

    /* compare first 7 bytes, skip the registry version and compare another 5 bytes */
    return memcmp(label, &gcLabel, 7) == 0 &&
           memcmp(&label->octet8, &gcLabel.octet8, 5) == 0;
}



int mxf_is_avc_ec(const mxfUL *label, int frame_wrapped)
{
    return mxf_is_generic_container_label(label) &&
           label->octet13 == 0x10 &&                         /* AVC byte stream */
           (label->octet14 & 0xf0) == 0x60 &&                /* video stream */
           ((frame_wrapped && label->octet15 == 0x01) ||     /* frame wrapped or */
               (!frame_wrapped && label->octet15 == 0x02));  /*   clip wrapped */
}

int mxf_is_mpeg_video_ec(const mxfUL *label, int frame_wrapped)
{
    return mxf_is_generic_container_label(label) &&
           label->octet13 == 0x04 &&                         /* MPEG elementary stream */
           (label->octet14 & 0xf0) == 0x60 &&                /* video stream */
           ((frame_wrapped && label->octet15 == 0x01) ||     /* frame wrapped or */
               (!frame_wrapped && label->octet15 == 0x02));  /*   clip wrapped */
}

int mxf_is_jpeg2000_ec(const mxfUL *label)
{
    return mxf_is_generic_container_label(label) &&
           label->octet13 == 0x0c;
}


void mxf_complete_essence_element_key(mxfKey *key, uint8_t count, uint8_t type, uint8_t num)
{
    key->octet13 = count;
    key->octet14 = type;
    key->octet15 = num;
}

void mxf_complete_essence_element_track_num(uint32_t *trackNum, uint8_t count, uint8_t type, uint8_t num)
{
    *trackNum &= 0xFF000000;
    *trackNum |= ((uint32_t)count) << 16;
    *trackNum |= ((uint32_t)type)  << 8;
    *trackNum |=  (uint32_t)(num);
}



int mxf_is_gs_data_element(const mxfKey *key)
{
    static const mxfUL gsKey = MXF_GS_DATA_ELEMENT_KEY(0x00, 0x00);

    return memcmp(key, &gsKey, 11) == 0 &&
           memcmp(&key->octet13, &gsKey.octet13, 3) == 0;
}
