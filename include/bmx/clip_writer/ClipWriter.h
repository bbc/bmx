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

#ifndef __BMX_CLIP_WRITER_H__
#define __BMX_CLIP_WRITER_H__


#include <bmx/as02/AS02Clip.h>
#include <bmx/as11/AS11Clip.h>
#include <bmx/mxf_op1a/OP1AFile.h>
#include <bmx/avid_mxf/AvidClip.h>
#include <bmx/d10_mxf/D10File.h>

#include <bmx/clip_writer/ClipWriterTrack.h>



namespace bmx
{


class ClipWriter
{
public:
    static ClipWriter* OpenNewAS02Clip(std::string bundle_directory, bool create_bundle_dir, Rational frame_rate);
    static ClipWriter* OpenNewAS11OP1AClip(int flavour, std::string filename, Rational frame_rate);
    static ClipWriter* OpenNewAS11D10Clip(std::string filename, Rational frame_rate);
    static ClipWriter* OpenNewOP1AClip(int flavour, std::string filename, Rational frame_rate);
    static ClipWriter* OpenNewAvidClip(Rational frame_rate, std::string filename_prefix = "");
    static ClipWriter* OpenNewD10Clip(std::string filename, Rational frame_rate);

    static std::string ClipWriterTypeToString(ClipWriterType clip_type);

public:
    ~ClipWriter();

    void SetClipName(std::string name);                             // default ""
    void SetStartTimecode(Timecode start_timecode);                 // default 00:00:00:00 non-drop
    void SetProductInfo(std::string company_name, std::string product_name, mxfProductVersion product_version,
                        std::string version, mxfUUID product_uid);

public:
    ClipWriterTrack* CreateTrack(EssenceType essence_type, std::string track_filename = "");

public:
    void PrepareWrite();
    void WriteSamples(uint32_t track_index, const unsigned char *data, uint32_t size, uint32_t num_samples);
    void CompleteWrite();

public:
    Rational GetFrameRate() const;

    int64_t GetDuration();

    uint32_t GetNumTracks() const { return mTracks.size(); }
    ClipWriterTrack* GetTrack(uint32_t track_index);

public:
    ClipWriterType GetType() const { return mType; }

    AS02Clip* GetAS02Clip() const { return mAS02Clip; }
    AS11Clip* GetAS11Clip() const { return mAS11Clip; }
    OP1AFile* GetOP1AClip() const { return mOP1AClip; }
    AvidClip* GetAvidClip() const { return mAvidClip; }
    D10File* GetD10Clip()   const { return mD10Clip; }

private:
    ClipWriter(AS02Bundle *bundle, AS02Clip *clip);
    ClipWriter(AS11Clip *clip);
    ClipWriter(OP1AFile *clip);
    ClipWriter(AvidClip *clip);
    ClipWriter(D10File *clip);

private:
    ClipWriterType mType;
    AS02Bundle *mAS02Bundle;
    AS02Clip *mAS02Clip;
    AS11Clip *mAS11Clip;
    OP1AFile *mOP1AClip;
    AvidClip *mAvidClip;
    D10File *mD10Clip;

    std::vector<ClipWriterTrack*> mTracks;
};


};



#endif

