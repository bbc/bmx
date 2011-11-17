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

#ifndef __IM_MXF_DESCRIPTOR_HELPER_H__
#define __IM_MXF_DESCRIPTOR_HELPER_H__


#include <libMXF++/MXF.h>



namespace im
{


class MXFDescriptorHelper
{
public:
    typedef enum
    {
        // unknown
        UNKNOWN_ESSENCE = 0,
        // generic
        PICTURE_ESSENCE,
        SOUND_ESSENCE,
        // D10 video
        D10_30,
        D10_40,
        D10_50,
        // DV video
        IEC_DV25,
        DVBASED_DV25,
        DV50,
        DV100_1080I,
        DV100_720P,
        // AVC-Intra video
        AVCI100_1080I,
        AVCI100_1080P,
        AVCI100_720P,
        AVCI50_1080I,
        AVCI50_1080P,
        AVCI50_720P,
        // Uncompressed video
        UNC_SD,
        UNC_HD_1080I,
        UNC_HD_1080P,
        UNC_HD_720P,
        AVID_10BIT_UNC_SD,
        AVID_10BIT_UNC_HD_1080I,
        AVID_10BIT_UNC_HD_1080P,
        AVID_10BIT_UNC_HD_720P,
        // MPEG-2 Long GOP video
        MPEG2LG_422P_HL,
        MPEG2LG_MP_HL,
        MPEG2LG_MP_H14,
        // VC-3
        VC3_1080P_1235,
        VC3_1080P_1237,
        VC3_1080P_1238,
        VC3_1080I_1241,
        VC3_1080I_1242,
        VC3_1080I_1243,
        VC3_720P_1250,
        VC3_720P_1251,
        VC3_720P_1252,
        VC3_1080P_1253,
        // Avid MJPEG
        MJPEG_2_1,
        MJPEG_3_1,
        MJPEG_10_1,
        MJPEG_20_1,
        MJPEG_4_1M,
        MJPEG_10_1M,
        MJPEG_15_1S,
        // WAVE PCM audio
        WAVE_PCM
    } EssenceType;

    typedef enum
    {
        SMPTE_377_2004_FLAVOUR,
        SMPTE_377_1_FLAVOUR,
        AVID_FLAVOUR,
    } DescriptorFlavour;

public:
    static EssenceType IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);
    static MXFDescriptorHelper* Create(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);
    static MXFDescriptorHelper* Create(EssenceType essence_type);
    static std::string EssenceTypeToString(EssenceType essence_type);

public:
    MXFDescriptorHelper();
    virtual ~MXFDescriptorHelper();

    virtual bool IsPicture() const { return false; }
    virtual bool IsSound() const { return false; }

public:
    // initialize from existing descriptor
    virtual void Initialize(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);

public:
    // configure and create new descriptor
    virtual void SetEssenceType(EssenceType essence_type);  // default depends on sub-class
    virtual void SetSampleRate(mxfRational sample_rate);    // default 25/1
    virtual void SetFrameWrapped(bool frame_wrapped);       // default true; clip wrapped if false
    virtual void SetFlavour(DescriptorFlavour flavour);     // default SMPTE_377_2004_FLAVOUR

    virtual mxfpp::FileDescriptor* CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata) = 0;
    virtual void UpdateFileDescriptor();

public:
    EssenceType GetEssenceType() const { return mEssenceType; }
    mxfRational GetSampleRate() const { return mSampleRate; }
    bool IsFrameWrapped() const { return mFrameWrapped; }
    mxfpp::FileDescriptor* GetFileDescriptor() const { return mFileDescriptor; }

    mxfUL GetEssenceContainerUL() const;

    virtual uint32_t GetSampleSize() = 0;

protected:
    virtual mxfUL ChooseEssenceContainerUL() const = 0;

protected:
    EssenceType mEssenceType;
    mxfRational mSampleRate;
    bool mFrameWrapped;
    DescriptorFlavour mFlavour;

    mxfpp::FileDescriptor *mFileDescriptor;
};


};



#endif

