/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#ifndef __BMX_AS11_CLIP_H__
#define __BMX_AS11_CLIP_H__


#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/d10_mxf/D10File.h>

#include <bmx/as11/AS11Track.h>
#include <bmx/as11/AS11DMS.h>
#include <bmx/as11/AS11CoreFramework.h>
#include <bmx/as11/UKDPPDMS.h>
#include <bmx/as11/UKDPPFramework.h>



namespace bmx
{


typedef struct
{
    int64_t start;
    int64_t duration;
    uint16_t part_number;
    uint16_t part_total;
} AS11PosSegment;

typedef struct
{
    Timecode start;
    int64_t duration;
    uint16_t part_number;
    uint16_t part_total;
} AS11TCSegment;


class AS11Clip
{
public:
    static AS11Clip* OpenNewOP1AClip(int flavour, std::string filename, Rational frame_rate);
    static AS11Clip* OpenNewD10Clip(int flavour, std::string filename, Rational frame_rate);

    static std::string AS11ClipTypeToString(AS11ClipType clip_type);

public:
    ~AS11Clip();

    void SetClipName(std::string name);                // default ""
    void SetStartTimecode(Timecode start_timecode);    // default 00:00:00:00 non-drop
    void SetPartitionInterval(int64_t frame_count);    // default 0 (single partition)
    void SetProductInfo(std::string company_name, std::string product_name, mxfProductVersion product_version,
                        std::string version, mxfUUID product_uid);

    void SetOutputStartOffset(int64_t offset);
    void SetOutputEndOffset(int64_t offset);

    AS11Track* CreateTrack(EssenceType essence_type);

public:
    void PrepareHeaderMetadata();
    void PrepareWrite();
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void CompleteWrite();

public:
    mxfpp::HeaderMetadata* GetHeaderMetadata() const;
    mxfpp::DataModel* GetDataModel() const;

    void InsertAS11CoreFramework(AS11CoreFramework *framework);
    void InsertUKDPPFramework(UKDPPFramework *framework);

    void InsertPosSegmentation(std::vector<AS11PosSegment> segments);
    void InsertTCSegmentation(std::vector<AS11TCSegment> segments);
    void CompleteSegmentation(bool with_filler);
    uint16_t GetTotalSegments();
    int64_t GetTotalSegmentDuration();

public:
    Rational GetFrameRate() const;

    int64_t GetDuration();

    int64_t GetInputDuration() const;

    uint32_t GetNumTracks() const { return (uint32_t)mTracks.size(); }
    AS11Track* GetTrack(uint32_t track_index);

public:
    AS11ClipType GetType() const { return mType; }
    OP1AFile* GetOP1AClip() const { return mOP1AClip; }
    D10File* GetD10Clip() const { return mD10Clip; }

private:
    AS11Clip(OP1AFile *clip);
    AS11Clip(D10File *clip);

    void AppendDMSLabel(mxfUL scheme_label);
    void InsertFramework(uint32_t track_id, std::string track_name, mxfpp::DMFramework *framework);

private:
    AS11ClipType mType;
    OP1AFile *mOP1AClip;
    D10File *mD10Clip;

    std::vector<AS11Track*> mTracks;

    mxfpp::Sequence *mSegmentationSequence;
};


};



#endif

