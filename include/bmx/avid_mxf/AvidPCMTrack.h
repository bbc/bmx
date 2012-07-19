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

#ifndef BMX_AVID_PCM_TRACK_H_
#define BMX_AVID_PCM_TRACK_H_

#include <bmx/avid_mxf/AvidTrack.h>
#include <bmx/mxf_helper/WaveMXFDescriptorHelper.h>



namespace bmx
{


class AvidPCMTrack : public AvidTrack
{
public:
    AvidPCMTrack(AvidClip *clip, uint32_t track_index, EssenceType essence_type, mxfpp::File *file);
    virtual ~AvidPCMTrack();

    void SetSamplingRate(mxfRational sampling_rate);    // default 48000/1
    void SetQuantizationBits(uint32_t bits);            // default 16
    void SetChannelCount(uint32_t count);               // default 1
    void SetLocked(bool locked);                        // default not set
    void SetAudioRefLevel(int8_t level);                // default not set
    void SetDialNorm(int8_t dial_norm);                 // default not set
    void SetSequenceOffset(uint8_t offset);             // default not set

public:
    bool IsPicture() const { return false; }

    const std::vector<uint32_t>& GetSampleSequence() const { return mSampleSequence; }
    uint8_t GetSequenceOffset() const { return mWaveDescriptorHelper->GetSequenceOffset(); }
    std::vector<uint32_t> GetShiftedSampleSequence() const;

public:
    virtual int64_t GetOutputDuration(bool clip_frame_rate) const;

private:
    void SetSampleSequence();

private:
    WaveMXFDescriptorHelper *mWaveDescriptorHelper;
    std::vector<uint32_t> mSampleSequence;
};


};



#endif

