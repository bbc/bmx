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

#ifndef BMX_MXF_TRACK_INFO_
#define BMX_MXF_TRACK_INFO_


#include <vector>

#include <bmx/BMXTypes.h>
#include <bmx/EssenceType.h>



namespace bmx
{


class MXFTrackInfo
{
public:
    MXFTrackInfo();
    virtual ~MXFTrackInfo() {}

    virtual bool IsCompatible(const MXFTrackInfo *right) const;

    virtual MXFTrackInfo* Clone() const = 0;

public:
    MXFDataDefEnum data_def;

    EssenceType essence_type;
    mxfUL essence_container_label;
    mxfUMID material_package_uid;
    uint32_t material_track_id;
    uint32_t material_track_number;
    mxfUMID file_package_uid;
    mxfRational edit_rate;
    int64_t duration;
    int64_t lead_filler_offset;
    uint32_t file_track_id;
    uint32_t file_track_number;

protected:
    void Clone(MXFTrackInfo *clone) const;
};


class MXFPictureTrackInfo : public MXFTrackInfo
{
public:
    MXFPictureTrackInfo();
    virtual ~MXFPictureTrackInfo() {}

    virtual bool IsCompatible(const MXFTrackInfo *right) const;

    virtual MXFTrackInfo* Clone() const;

public:
    bool is_cdci;

    mxfUL picture_essence_coding_label;
    uint8_t signal_standard;
    uint32_t stored_width;
    uint32_t stored_height;
    uint32_t display_width;
    uint32_t display_height;
    int32_t display_x_offset;
    int32_t display_y_offset;
    mxfRational aspect_ratio;
    uint8_t frame_layout;
    uint8_t afd;

    // CDCI only
    uint32_t component_depth;
    uint32_t horiz_subsampling;
    uint32_t vert_subsampling;
    uint8_t color_siting;
    bool have_avci_header;
};


class MXFSoundTrackInfo : public MXFTrackInfo
{
public:
    MXFSoundTrackInfo();
    virtual ~MXFSoundTrackInfo() {}

    virtual bool IsCompatible(const MXFTrackInfo *right) const;

    virtual MXFTrackInfo* Clone() const;

public:
    mxfRational sampling_rate;
    uint32_t bits_per_sample;
    uint16_t block_align;
    uint32_t channel_count;
    uint8_t d10_aes3_valid_flags;
    uint8_t sequence_offset;
    bool locked;
    bool locked_set;
    int8_t audio_ref_level;
    bool audio_ref_level_set;
    int8_t dial_norm;
    bool dial_norm_set;
};


class ST436Line;

class VBIManifestElement
{
public:
    VBIManifestElement();

    void Parse(const ST436Line *line);

    bool operator==(const VBIManifestElement &right) const;

public:
    uint16_t line_number;
    uint8_t wrapping_type;
    uint8_t sample_coding;
};

class ANCManifestElement
{
public:
    ANCManifestElement();

    void Parse(const ST436Line *line);

    bool operator==(const ANCManifestElement &right) const;

public:
    uint16_t line_number;
    uint8_t wrapping_type;
    uint8_t sample_coding;
    uint8_t did;
    uint8_t sdid;
};

class MXFDataTrackInfo : public MXFTrackInfo
{
public:
    MXFDataTrackInfo();
    virtual ~MXFDataTrackInfo() {}

    virtual bool IsCompatible(const MXFTrackInfo *right) const;

    virtual MXFTrackInfo* Clone() const;

    void AppendUniqueVBIElement(const VBIManifestElement &element);
    void AppendUniqueANCElement(const ANCManifestElement &element);

public:
    std::vector<VBIManifestElement> vbi_manifest;
    std::vector<ANCManifestElement> anc_manifest;
};


};



#endif

