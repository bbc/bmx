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

#ifndef BMX_PICTURE_MXF_DESCRIPTOR_HELPER_H_
#define BMX_PICTURE_MXF_DESCRIPTOR_HELPER_H_


#include <bmx/mxf_helper/MXFDescriptorHelper.h>



namespace bmx
{


class PictureMXFDescriptorHelper : public MXFDescriptorHelper
{
public:
    static EssenceType IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);
    static PictureMXFDescriptorHelper* Create(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version,
                                              mxfUL alternative_ec_label);

    static bool IsSupported(EssenceType essence_type);
    static MXFDescriptorHelper* Create(EssenceType essence_type);

public:
    PictureMXFDescriptorHelper();
    virtual ~PictureMXFDescriptorHelper();

public:
    virtual bool IsPicture() const { return true; }

public:
    // initialize from existing descriptor
    virtual void Initialize(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label);

public:
    // configure and create new descriptor
    void SetAspectRatio(mxfRational aspect_ratio);  // default 16/9
    void SetAFD(uint8_t afd);                       // default not set

    virtual mxfpp::FileDescriptor* CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata);
    virtual void UpdateFileDescriptor();

public:
    virtual uint32_t GetImageAlignmentOffset()  { return mImageAlignmentOffset; }
    virtual uint32_t GetImageStartOffset()      { return mImageStartOffset; }
    virtual uint32_t GetImageEndOffset()        { return mImageEndOffset; }
    virtual uint32_t GetEditUnitSize()          { return GetImageStartOffset() +
                                                         GetSampleSize() +
                                                         GetImageEndOffset(); }
    virtual uint32_t GetSampleSize()            { return 0; }

public:
    // Avid extensions

    bool HaveAvidResolutionID();
    int32_t GetAvidResolutionID();
    void SetAvidResolutionID(int32_t resolution_id);

    bool HaveAvidFrameSampleSize();
    int32_t GetAvidFrameSampleSize();
    void SetAvidFrameSampleSize(int32_t size);

    bool HaveAvidImageSize();
    int32_t GetAvidImageSize();
    void SetAvidImageSize(int32_t size);

    bool HaveAvidFirstFrameOffset();
    int32_t GetAvidFirstFrameOffset();
    void SetFirstFrameOffset(int32_t offset);

protected:
    virtual mxfUL ChooseEssenceContainerUL() const;

    void SetCodingEquations(mxfUL label);
    void SetColorSiting(uint8_t color_siting);

protected:
    mxfRational mAspectRatio;
    uint8_t mAFD;
    int32_t mAvidResolutionId;
    uint32_t mImageAlignmentOffset;
    bool mImageAlignmentOffsetSet;
    uint32_t mImageStartOffset;
    bool mImageStartOffsetSet;
    uint32_t mImageEndOffset;
    bool mImageEndOffsetSet;
};


};



#endif

