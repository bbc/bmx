/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_RDD9_PCM_TRACK_H_
#define BMX_RDD9_PCM_TRACK_H_

#include <bmx/rdd9_mxf/RDD9Track.h>
#include <bmx/mxf_helper/WaveMXFDescriptorHelper.h>



namespace bmx
{


class RDD9PCMTrack : public RDD9Track
{
public:
    RDD9PCMTrack(RDD9File *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                 Rational frame_rate, EssenceType essence_type);
    virtual ~RDD9PCMTrack();

    void SetSamplingRate(Rational sampling_rate);       // default 48000/1
    void SetQuantizationBits(uint32_t bits);            // default 16
    void SetChannelCount(uint32_t count);               // default and required 1
    void SetLocked(bool locked);                        // default not set
    void SetAudioRefLevel(int8_t level);                // default not set
    void SetDialNorm(int8_t dial_norm);                 // default not set
    void SetSequenceOffset(uint8_t offset);             // default not set
    void SetChannelAssignment(UL label);                // default not set

    mxfpp::AudioChannelLabelSubDescriptor* AddAudioChannelLabel(
        mxfpp::AudioChannelLabelSubDescriptor *copy_from = 0);
    mxfpp::SoundfieldGroupLabelSubDescriptor* AddSoundfieldGroupLabel(
        mxfpp::SoundfieldGroupLabelSubDescriptor *copy_from = 0);
    mxfpp::GroupOfSoundfieldGroupsLabelSubDescriptor* AddGroupOfSoundfieldGroupLabel(
        mxfpp::GroupOfSoundfieldGroupsLabelSubDescriptor *copy_from = 0);
    mxfpp::ADMSoundfieldGroupLabelSubDescriptor* AddADMSoundfieldGroupLabel(
        mxfpp::ADMSoundfieldGroupLabelSubDescriptor *copy_from = 0);

public:
    const std::vector<uint32_t>& GetSampleSequence() const  { return mSampleSequence; }
    uint8_t GetSequenceOffset() const                       { return mWaveDescriptorHelper->GetSequenceOffset(); }
    std::vector<uint32_t> GetShiftedSampleSequence() const;
    Rational GetSamplingRate() const                        { return mWaveDescriptorHelper->GetSamplingRate(); }
    uint32_t GetQuantizationBits() const                    { return mWaveDescriptorHelper->GetQuantizationBits(); }
    uint32_t GetChannelCount() const                        { return mWaveDescriptorHelper->GetChannelCount(); }

    const std::vector<mxfpp::MCALabelSubDescriptor*>& GetMCALabels() const { return mMCALabels; }

protected:
    virtual void AddHeaderMetadata(mxfpp::HeaderMetadata *header_metadata, mxfpp::MaterialPackage *material_package,
                                   mxfpp::SourcePackage *file_source_package);

    virtual void PrepareWrite(uint8_t track_count);
    virtual void CompleteWrite();

private:
    void SetSampleSequence();

private:
    WaveMXFDescriptorHelper *mWaveDescriptorHelper;
    std::vector<uint32_t> mSampleSequence;
    std::vector<mxfpp::MCALabelSubDescriptor*> mMCALabels;
};


};



#endif

