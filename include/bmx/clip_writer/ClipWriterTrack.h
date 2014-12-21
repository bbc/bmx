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

#ifndef BMX_CLIP_WRITER_TRACK_H_
#define BMX_CLIP_WRITER_TRACK_H_


#include <bmx/as02/AS02Track.h>
#include <bmx/mxf_op1a/OP1ATrack.h>
#include <bmx/avid_mxf/AvidTrack.h>
#include <bmx/d10_mxf/D10Track.h>
#include <bmx/rdd9_mxf/RDD9Track.h>
#include <bmx/wave/WaveTrackWriter.h>
#include <bmx/writer_helper/AVCIWriterHelper.h>



namespace bmx
{


typedef enum
{
    CW_UNKNOWN_CLIP_TYPE = 0,
    CW_AS02_CLIP_TYPE,
    CW_OP1A_CLIP_TYPE,
    CW_AVID_CLIP_TYPE,
    CW_D10_CLIP_TYPE,
    CW_RDD9_CLIP_TYPE,
    CW_WAVE_CLIP_TYPE,
} ClipWriterType;



class ClipWriterTrack
{
public:
    static bool IsSupported(ClipWriterType clip_type, EssenceType essence_type, Rational sample_rate);

public:
    ClipWriterTrack(EssenceType essence_type, AS02Track *track);
    ClipWriterTrack(EssenceType essence_type, OP1ATrack *track);
    ClipWriterTrack(EssenceType essence_type, AvidTrack *track);
    ClipWriterTrack(EssenceType essence_type, D10Track *track);
    ClipWriterTrack(EssenceType essence_type, RDD9Track *track);
    ClipWriterTrack(EssenceType essence_type, WaveTrackWriter *track);
    virtual ~ClipWriterTrack();

public:
    // General properties
    void SetOutputTrackNumber(uint32_t track_number);

    // Picture properties
    void SetAspectRatio(Rational aspect_ratio);                     // default 16/9
    void SetComponentDepth(uint32_t depth);                         // default 8; alternative is 10
    void SetAVCIMode(AVCIMode mode);                                // default depends on track type
    void SetAVCIHeader(const unsigned char *data, uint32_t size);
    void SetReplaceAVCIHeader(bool enable);                         // default false; requires SetAVCIHeader if true
    void SetAFD(uint8_t afd);                                       // default not set
    void SetInputHeight(uint32_t height);                           // uncompressed; default 0

    // Sound properties
    void SetAES3Mapping(bool enable);               // default BWF mapping
    void SetSamplingRate(Rational sampling_rate);   // default 48000/1
    void SetQuantizationBits(uint32_t bits);        // default 16
    void SetChannelCount(uint32_t count);           // default 1
    void SetLocked(bool locked);                    // default not set
    void SetAudioRefLevel(int8_t level);            // default not set
    void SetDialNorm(int8_t dial_norm);             // default not set
    void SetSequenceOffset(uint8_t offset);         // default D10 determined from input or not set

    // Data properties
    void SetConstantDataSize(uint32_t size);
    void SetMaxDataSize(uint32_t size);

public:
    void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

public:
    bool IsPicture() const;

    uint32_t GetSampleSize() const;
    uint32_t GetInputSampleSize() const;
    uint32_t GetAVCISampleWithoutHeaderSize() const;
    bool IsSingleField() const;

    std::vector<uint32_t> GetShiftedSampleSequence() const;

    int64_t GetDuration() const;
    int64_t GetContainerDuration() const;

public:
    ClipWriterType GetClipType() const { return mClipType; }
    EssenceType GetEssenceType() const { return mEssenceType; }

    AS02Track* GetAS02Track()       const { return mAS02Track; }
    OP1ATrack* GetOP1ATrack()       const { return mOP1ATrack; }
    AvidTrack* GetAvidTrack()       const { return mAvidTrack; }
    D10Track* GetD10Track()         const { return mD10Track; }
    RDD9Track* GetRDD9Track()       const { return mRDD9Track; }
    WaveTrackWriter* GetWaveTrack() const { return mWaveTrack; }

private:
    ClipWriterType mClipType;
    EssenceType mEssenceType;
    AS02Track *mAS02Track;
    OP1ATrack *mOP1ATrack;
    AvidTrack *mAvidTrack;
    D10Track *mD10Track;
    RDD9Track *mRDD9Track;
    WaveTrackWriter *mWaveTrack;
};


};



#endif

