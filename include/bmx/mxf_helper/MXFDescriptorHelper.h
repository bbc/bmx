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

#ifndef BMX_MXF_DESCRIPTOR_HELPER_H_
#define BMX_MXF_DESCRIPTOR_HELPER_H_


#include <libMXF++/MXF.h>

#include <bmx/EssenceType.h>


#define MXFDESC_SMPTE_377_2004_FLAVOUR        0x0001
#define MXFDESC_SMPTE_377_1_FLAVOUR           0x0002
#define MXFDESC_AVID_FLAVOUR                  0x0004
#define MXFDESC_RDD9_FLAVOUR                  0x0008
#define MXFDESC_ARD_ZDF_HDF_PROFILE_FLAVOUR   0x0010



namespace bmx
{


class MXFDescriptorHelper
{
public:
    static EssenceType IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);
    static MXFDescriptorHelper* Create(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version,
                                       mxfUL alternative_ec_label);
    static MXFDescriptorHelper* Create(EssenceType essence_type);

protected:
   static bool CompareECULs(mxfUL ec_label_a, mxfUL alternative_ec_label_a, mxfUL ec_label_b);
   static bool IsNullAvidECUL(mxfUL ec_label, mxfUL alternative_ec_label);

public:
    MXFDescriptorHelper();
    virtual ~MXFDescriptorHelper();

    virtual bool IsPicture() const { return false; }
    virtual bool IsSound() const { return false; }
    virtual bool IsData() const { return false; }

public:
    // initialize from existing descriptor
    virtual void Initialize(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label);

public:
    // configure and create new descriptor
    virtual void SetEssenceType(EssenceType essence_type);  // default depends on sub-class
    virtual void SetSampleRate(mxfRational sample_rate);    // default 25/1
    virtual void SetFrameWrapped(bool frame_wrapped);       // default true; clip wrapped if false
    virtual void SetFlavour(int flavour);                   // default MXFDESC_SMPTE_377_2004_FLAVOUR

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
    int mFlavour;

    mxfpp::FileDescriptor *mFileDescriptor;
};


};



#endif

