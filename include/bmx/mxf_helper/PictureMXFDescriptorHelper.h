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
#include <bmx/BMXTypes.h>



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
    void SetAspectRatio(mxfRational aspect_ratio);
    void SetAFD(uint8_t afd);
    void SetSignalStandard(MXFSignalStandard signal_standard);
    void SetFrameLayout(MXFFrameLayout frame_layout);
    void SetFieldDominance(uint8_t field_num);
    void SetTransferCharacteristic(mxfUL label);
    void SetCodingEquations(mxfUL label);
    void SetColorPrimaries(mxfUL label);
    void SetColorSiting(MXFColorSiting color_siting);
    void SetBlackRefLevel(uint32_t level);
    void SetWhiteRefLevel(uint32_t level);
    void SetColorRange(uint32_t range);
    void SetComponentMaxRef(uint32_t ref);
    void SetComponentMinRef(uint32_t ref);
    void SetScanningDirection(uint8_t direction);
    void SetMasteringDisplayPrimaries(mxfThreeColorPrimaries primaries);
    void SetMasteringDisplayWhitePointChromaticity(mxfColorPrimary chroma);
    void SetMasteringDisplayMaximumLuminance(uint32_t max_lum);
    void SetMasteringDisplayMinimumLuminance(uint32_t min_lum);
    void SetActiveWidth(uint32_t width);
    void SetActiveHeight(uint32_t height);
    void SetActiveXOffset(uint32_t offset);
    void SetActiveYOffset(uint32_t offset);
    void SetDisplayF2Offset(int32_t offset);
    void SetAlternativeCenterCuts(std::vector<mxfUL> &cuts);

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


    // value depends on flavour
    void SetCodingEquationsMod(mxfUL label);
    void SetColorSitingMod(uint8_t color_siting);

protected:
    virtual mxfUL ChooseEssenceContainerUL() const;

protected:
    int32_t mAvidResolutionId;
    BMX_OPT_PROP_DECL(mxfRational, mAspectRatio);
    BMX_OPT_PROP_DECL(uint8_t, mAFD);
    BMX_OPT_PROP_DECL(uint32_t, mImageAlignmentOffset);
    BMX_OPT_PROP_DECL(uint32_t, mImageStartOffset);
    BMX_OPT_PROP_DECL(uint32_t, mImageEndOffset);
    BMX_OPT_PROP_DECL(MXFSignalStandard, mSignalStandard);
    BMX_OPT_PROP_DECL(MXFFrameLayout, mFrameLayout);
    BMX_OPT_PROP_DECL(uint8_t, mFieldDominance);
    BMX_OPT_PROP_DECL(mxfUL, mTransferCh);
    BMX_OPT_PROP_DECL(mxfUL, mCodingEquations);
    BMX_OPT_PROP_DECL(mxfUL, mColorPrimaries);
    BMX_OPT_PROP_DECL(MXFColorSiting, mColorSiting);
    BMX_OPT_PROP_DECL(uint32_t, mBlackRefLevel);
    BMX_OPT_PROP_DECL(uint32_t, mWhiteRefLevel);
    BMX_OPT_PROP_DECL(uint32_t, mColorRange);
    BMX_OPT_PROP_DECL(uint32_t, mComponentMaxRef);
    BMX_OPT_PROP_DECL(uint32_t, mComponentMinRef);
    BMX_OPT_PROP_DECL(uint8_t, mScanningDirection);
    BMX_OPT_PROP_DECL(mxfThreeColorPrimaries, mMasteringDisplayPrimaries);
    BMX_OPT_PROP_DECL(mxfColorPrimary, mMasteringDisplayWhitePointChromaticity);
    BMX_OPT_PROP_DECL(uint32_t, mMasteringDisplayMaximumLuminance);
    BMX_OPT_PROP_DECL(uint32_t, mMasteringDisplayMinimumLuminance);
    BMX_OPT_PROP_DECL(uint32_t, mActiveWidth);
    BMX_OPT_PROP_DECL(uint32_t, mActiveHeight);
    BMX_OPT_PROP_DECL(uint32_t, mActiveXOffset);
    BMX_OPT_PROP_DECL(uint32_t, mActiveYOffset);
    BMX_OPT_PROP_DECL(int32_t, mDisplayF2Offset);
    BMX_OPT_PROP_DECL(std::vector<mxfUL>, mAlternativeCenterCuts);
};


};



#endif

