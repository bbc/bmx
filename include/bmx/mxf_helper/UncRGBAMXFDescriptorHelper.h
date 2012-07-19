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

#ifndef BMX_UNC_RGBA_MXF_DESCRIPTOR_HELPER_H_
#define BMX_UNC_RGBA_MXF_DESCRIPTOR_HELPER_H_


#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>



namespace bmx
{


class UncRGBAMXFDescriptorHelper : public PictureMXFDescriptorHelper
{
public:
    static EssenceType IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);
    static bool IsSupported(EssenceType essence_type);

private:
    static size_t GetEssenceIndex(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label);

public:
    UncRGBAMXFDescriptorHelper();
    virtual ~UncRGBAMXFDescriptorHelper();

public:
    // initialize from existing descriptor
    virtual void Initialize(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label);

public:
    // configure and create new descriptor
    void SetStoredDimensions(uint32_t width, uint32_t height);
    void SetDisplayDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset);
    void SetSampledDimensions(uint32_t width, uint32_t height, int32_t x_offset, int32_t y_offset);
    void SetVideoLineMap(int32_t field1, int32_t field2);
    virtual void SetEssenceType(EssenceType essence_type);
    virtual void SetSampleRate(mxfRational sample_rate);

    virtual mxfpp::FileDescriptor* CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata);
    virtual void UpdateFileDescriptor();

public:
    virtual uint32_t GetImageAlignmentOffset();
    virtual uint32_t GetImageEndOffset();
    virtual uint32_t GetSampleSize();

    uint32_t GetSampleSize(uint32_t input_height);

protected:
    virtual mxfUL ChooseEssenceContainerUL() const;

private:
    bool UpdateEssenceIndex();
    void SetDefaultDimensions();

private:
    size_t mEssenceIndex;
    bool mStoredDimensionsSet;
    uint32_t mStoredWidth;
    uint32_t mStoredHeight;
    bool mDisplayDimensionsSet;
    uint32_t mDisplayWidth;
    uint32_t mDisplayHeight;
    int32_t mDisplayXOffset;
    int32_t mDisplayYOffset;
    bool mSampledDimensionsSet;
    uint32_t mSampledWidth;
    uint32_t mSampledHeight;
    int32_t mSampledXOffset;
    int32_t mSampledYOffset;
    int32_t mVideoLineMap[2];
    bool mVideoLineMapSet;
};


};



#endif

