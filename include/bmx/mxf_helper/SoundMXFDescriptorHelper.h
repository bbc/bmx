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

#ifndef BMX_SOUND_MXF_DESCRIPTOR_HELPER_H_
#define BMX_SOUND_MXF_DESCRIPTOR_HELPER_H_


#include <bmx/mxf_helper/MXFDescriptorHelper.h>



namespace bmx
{


class SoundMXFDescriptorHelper : public MXFDescriptorHelper
{
public:
    static EssenceType IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);
    static SoundMXFDescriptorHelper* Create(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version,
                                            mxfUL alternative_ec_label);

    static bool IsSupported(EssenceType essence_type);
    static MXFDescriptorHelper* Create(EssenceType essence_type);

public:
    SoundMXFDescriptorHelper();
    virtual ~SoundMXFDescriptorHelper();

public:
    virtual bool IsSound() const { return true; }

public:
    // initialize from existing descriptor
    virtual void Initialize(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label);

public:
    // configure and create new descriptor
    void SetSamplingRate(mxfRational sampling_rate);    // default 48000/1
    void SetQuantizationBits(uint32_t bits);            // default 16
    void SetChannelCount(uint32_t count);               // default 1
    void SetLocked(bool locked);                        // default not set
    void SetAudioRefLevel(int8_t level);                // default not set
    void SetDialNorm(int8_t dial_norm);                 // default not set

    virtual mxfpp::FileDescriptor* CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata);
    virtual void UpdateFileDescriptor();

public:
    mxfRational GetSamplingRate() const     { return mSamplingRate; }
    uint32_t GetQuantizationBits() const    { return mQuantizationBits; }
    uint32_t GetChannelCount() const        { return mChannelCount; }
    bool HaveSetLocked() const              { return mLockedSet; }
    bool GetLocked() const                  { return mLocked; }
    bool HaveSetAudioRefLevel() const       { return mAudioRefLevelSet; }
    int8_t GetAudioRefLevel() const         { return mAudioRefLevel; }
    bool HaveSetDialNorm() const            { return mDialNormSet; }
    int8_t GetDialNorm() const              { return mDialNorm; }

    virtual uint32_t GetSampleSize();

protected:
    virtual mxfUL ChooseEssenceContainerUL() const;

protected:
    mxfRational mSamplingRate;
    uint32_t mQuantizationBits;
    uint32_t mChannelCount;
    bool mLocked;
    bool mLockedSet;
    int8_t mAudioRefLevel;
    bool mAudioRefLevelSet;
    int8_t mDialNorm;
    bool mDialNormSet;
};


};



#endif

