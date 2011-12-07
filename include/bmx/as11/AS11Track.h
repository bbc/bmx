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

#ifndef __BMX_AS11_TRACK_H__
#define __BMX_AS11_TRACK_H__


#include <bmx/mxf_op1a/OP1ATrack.h>
#include <bmx/mxf_op1a/OP1AAVCITrack.h>
#include <bmx/d10_mxf/D10Track.h>



namespace bmx
{


typedef enum
{
    AS11_UNKNOWN_CLIP_TYPE = 0,
    AS11_OP1A_CLIP_TYPE,
    AS11_D10_CLIP_TYPE,
} AS11ClipType;


class AS11Track
{
public:
    static bool IsSupported(AS11ClipType clip_type, EssenceType essence_type, Rational sample_rate);

public:
    AS11Track(EssenceType essence_type, OP1ATrack *track);
    AS11Track(EssenceType essence_type, D10Track *track);
    virtual ~AS11Track();

public:
    // General properties
    void SetOutputTrackNumber(uint32_t track_number);

    // Picture properties
    void SetAspectRatio(Rational aspect_ratio);             // default 16/9
    void SetComponentDepth(uint32_t depth);                 // default 8; alternative is 10
    void SetSampleSize(uint32_t size);                      // D10 sample size
    void SetAVCIMode(OP1AAVCIMode mode);                    // default OP1A_AVCI_ALL_FRAME_HEADER_MODE
    void SetAVCIHeader(const unsigned char *data, uint32_t size);
    void SetAFD(uint8_t afd);                               // default not set

    // Sound properties
    void SetSamplingRate(Rational sampling_rate);   // default 48000/1
    void SetQuantizationBits(uint32_t bits);        // default 16
    void SetChannelCount(uint32_t count);           // default 1
    void SetLocked(bool locked);                    // default not set
    void SetAudioRefLevel(int8_t level);            // default not set
    void SetDialNorm(int8_t dial_norm);             // default not set
    void SetSequenceOffset(uint8_t offset);         // default 0 for D10 or not set for OP1A

public:
    // Sound properties
    std::vector<uint32_t> GetSampleSequence() const;
    uint8_t GetSequenceOffset() const;
    std::vector<uint32_t> GetShiftedSampleSequence() const;

public:
    void WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples);

public:
    bool IsPicture() const;

    uint32_t GetSampleSize();
    uint32_t GetAVCISampleWithoutHeaderSize();

public:
    AS11ClipType GetClipType() const   { return mClipType; }
    EssenceType GetEssenceType() const { return mEssenceType; }

    OP1ATrack* GetOP1ATrack() const { return mOP1ATrack; }
    D10Track* GetD10Track() const   { return mD10Track; }

private:
    AS11ClipType mClipType;
    EssenceType mEssenceType;
    OP1ATrack *mOP1ATrack;
    D10Track *mD10Track;
};


};



#endif

