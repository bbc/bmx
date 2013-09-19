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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>

#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>
#include <bmx/mxf_helper/D10MXFDescriptorHelper.h>
#include <bmx/mxf_helper/DVMXFDescriptorHelper.h>
#include <bmx/mxf_helper/AVCIntraUncsMXFDescriptorHelper.h>
#include <bmx/mxf_helper/AVCIMXFDescriptorHelper.h>
#include <bmx/mxf_helper/UncCDCIMXFDescriptorHelper.h>
#include <bmx/mxf_helper/UncRGBAMXFDescriptorHelper.h>
#include <bmx/mxf_helper/MPEG2LGMXFDescriptorHelper.h>
#include <bmx/mxf_helper/VC3MXFDescriptorHelper.h>
#include <bmx/mxf_helper/MJPEGMXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/BMXTypes.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



EssenceType PictureMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    GenericPictureEssenceDescriptor *picture_descriptor =
        dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!picture_descriptor)
        return UNKNOWN_ESSENCE_TYPE;

    EssenceType essence_type = D10MXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = DVMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = AVCIntraUncsMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = AVCIMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = UncCDCIMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = UncRGBAMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = MPEG2LGMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = VC3MXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = MJPEGMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;

    return PICTURE_ESSENCE;
}

PictureMXFDescriptorHelper* PictureMXFDescriptorHelper::Create(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                                               mxfUL alternative_ec_label)
{
    PictureMXFDescriptorHelper *helper;
    if (D10MXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new D10MXFDescriptorHelper();
    else if (DVMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new DVMXFDescriptorHelper();
    else if (AVCIntraUncsMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new AVCIntraUncsMXFDescriptorHelper();
    else if (AVCIMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new AVCIMXFDescriptorHelper();
    else if (UncCDCIMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new UncCDCIMXFDescriptorHelper();
    else if (UncRGBAMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new UncRGBAMXFDescriptorHelper();
    else if (MPEG2LGMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new MPEG2LGMXFDescriptorHelper();
    else if (VC3MXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new VC3MXFDescriptorHelper();
    else if (MJPEGMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        helper = new MJPEGMXFDescriptorHelper();
    else
        helper = new PictureMXFDescriptorHelper();

    helper->Initialize(file_descriptor, mxf_version, alternative_ec_label);

    return helper;
}

bool PictureMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return D10MXFDescriptorHelper::IsSupported(essence_type) ||
           DVMXFDescriptorHelper::IsSupported(essence_type) ||
           AVCIntraUncsMXFDescriptorHelper::IsSupported(essence_type) ||
           AVCIMXFDescriptorHelper::IsSupported(essence_type) ||
           UncCDCIMXFDescriptorHelper::IsSupported(essence_type) ||
           UncRGBAMXFDescriptorHelper::IsSupported(essence_type) ||
           MPEG2LGMXFDescriptorHelper::IsSupported(essence_type) ||
           VC3MXFDescriptorHelper::IsSupported(essence_type) ||
           MJPEGMXFDescriptorHelper::IsSupported(essence_type);
}

MXFDescriptorHelper* PictureMXFDescriptorHelper::Create(EssenceType essence_type)
{
    BMX_ASSERT(IsSupported(essence_type));

    PictureMXFDescriptorHelper *helper;
    if (D10MXFDescriptorHelper::IsSupported(essence_type))
        helper = new D10MXFDescriptorHelper();
    else if (DVMXFDescriptorHelper::IsSupported(essence_type))
        helper = new DVMXFDescriptorHelper();
    else if (AVCIntraUncsMXFDescriptorHelper::IsSupported(essence_type))
        helper = new AVCIntraUncsMXFDescriptorHelper();
    else if (AVCIMXFDescriptorHelper::IsSupported(essence_type))
        helper = new AVCIMXFDescriptorHelper();
    else if (UncCDCIMXFDescriptorHelper::IsSupported(essence_type))
        helper = new UncCDCIMXFDescriptorHelper();
    else if (UncRGBAMXFDescriptorHelper::IsSupported(essence_type))
        helper = new UncRGBAMXFDescriptorHelper();
    else if (MPEG2LGMXFDescriptorHelper::IsSupported(essence_type))
        helper = new MPEG2LGMXFDescriptorHelper();
    else if (VC3MXFDescriptorHelper::IsSupported(essence_type))
        helper = new VC3MXFDescriptorHelper();
    else if (MJPEGMXFDescriptorHelper::IsSupported(essence_type))
        helper = new MJPEGMXFDescriptorHelper();
    else
        helper = new PictureMXFDescriptorHelper();

    helper->SetEssenceType(essence_type);

    return helper;
}

PictureMXFDescriptorHelper::PictureMXFDescriptorHelper()
: MXFDescriptorHelper()
{
    mEssenceType = PICTURE_ESSENCE;
    mAspectRatio = ASPECT_RATIO_16_9;
    mAFD = 0;
    mAvidResolutionId = 0;
    mImageAlignmentOffset = 1;
    mImageAlignmentOffsetSet = false;
    mImageStartOffset = 0;
    mImageStartOffsetSet = false;
    mImageEndOffset = 0;
    mImageEndOffsetSet = false;
}

PictureMXFDescriptorHelper::~PictureMXFDescriptorHelper()
{
}

void PictureMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                            mxfUL alternative_ec_label)
{
    MXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    GenericPictureEssenceDescriptor *picture_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    BMX_ASSERT(picture_descriptor);

    if (picture_descriptor->haveActiveFormatDescriptor()) {
        decode_afd(picture_descriptor->getActiveFormatDescriptor(), mxf_version, &mAFD, &mAspectRatio);
    } else {
        mAFD = 0;
        mAspectRatio = ZERO_RATIONAL;
    }

    if (picture_descriptor->haveAspectRatio())
        mAspectRatio = picture_descriptor->getAspectRatio();

    if (picture_descriptor->haveImageAlignmentOffset())
        mImageAlignmentOffset = picture_descriptor->getImageAlignmentOffset();
    else
        mImageAlignmentOffset = 1;
    mImageAlignmentOffsetSet = true;

    if (picture_descriptor->haveImageStartOffset())
        mImageStartOffset = picture_descriptor->getImageStartOffset();
    else
        mImageStartOffset = 0;
    mImageStartOffsetSet = true;

    if (picture_descriptor->haveImageEndOffset())
        mImageEndOffset = picture_descriptor->getImageEndOffset();
    else
        mImageEndOffset = 0;
    mImageEndOffsetSet = true;
}

void PictureMXFDescriptorHelper::SetAspectRatio(mxfRational aspect_ratio)
{
    BMX_ASSERT(!mFileDescriptor);

    mAspectRatio = aspect_ratio;
}

void PictureMXFDescriptorHelper::SetAFD(uint8_t afd)
{
    mAFD = afd;
}

FileDescriptor* PictureMXFDescriptorHelper::CreateFileDescriptor(HeaderMetadata *header_metadata)
{
    (void)header_metadata;

    // implemented by child classes only
    BMX_ASSERT(false);
    return 0;
}

void PictureMXFDescriptorHelper::UpdateFileDescriptor()
{
    MXFDescriptorHelper::UpdateFileDescriptor();

    GenericPictureEssenceDescriptor *picture_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(picture_descriptor);

    picture_descriptor->setAspectRatio(mAspectRatio);
    if (mAFD)
        picture_descriptor->setActiveFormatDescriptor(encode_afd(mAFD, mAspectRatio));

    if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        if (GetImageAlignmentOffset() > 1)
            picture_descriptor->setImageAlignmentOffset(GetImageAlignmentOffset());
        if (GetImageStartOffset() > 0)
            picture_descriptor->setImageStartOffset(GetImageStartOffset());
        // not setting ImageEndOffset because Avid alpha channel sample files did not have it set
        SetAvidFrameSampleSize(GetEditUnitSize());
        if (mAvidResolutionId != 0)
            SetAvidResolutionID(mAvidResolutionId);
    }
}

bool PictureMXFDescriptorHelper::HaveAvidResolutionID()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID));
}

int32_t PictureMXFDescriptorHelper::GetAvidResolutionID()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID));
}

void PictureMXFDescriptorHelper::SetAvidResolutionID(int32_t resolution_id)
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), resolution_id);
}

bool PictureMXFDescriptorHelper::HaveAvidFrameSampleSize()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize));
}

int32_t PictureMXFDescriptorHelper::GetAvidFrameSampleSize()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize));
}

void PictureMXFDescriptorHelper::SetAvidFrameSampleSize(int32_t size)
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), size);
}

bool PictureMXFDescriptorHelper::HaveAvidImageSize()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize));
}

int32_t PictureMXFDescriptorHelper::GetAvidImageSize()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize));
}

void PictureMXFDescriptorHelper::SetAvidImageSize(int32_t size)
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), size);
}

bool PictureMXFDescriptorHelper::HaveAvidFirstFrameOffset()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FirstFrameOffset));
}

int32_t PictureMXFDescriptorHelper::GetAvidFirstFrameOffset()
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FirstFrameOffset));
}

void PictureMXFDescriptorHelper::SetFirstFrameOffset(int32_t offset)
{
    BMX_ASSERT(mFileDescriptor);
    return mFileDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FirstFrameOffset), offset);
}

mxfUL PictureMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    // implemented by child classes only
    BMX_ASSERT(false);
    return g_Null_UL;
}

void PictureMXFDescriptorHelper::SetCodingEquations(mxfUL label)
{
    GenericPictureEssenceDescriptor *picture_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(picture_descriptor);

    mxfAUID auid;
    if ((mFlavour & MXFDESC_AVID_FLAVOUR)) {
        // this label is half-swapped in Avid files
        // Note that 377-1 defines an AUID with a UL stored as-is and an UUID half swapped. An IDAU has the
        // opposite, swapping the UL. The "AUID" (aafUID_t) type defined in the AAF SDK is therefore an IDAU! The
        // mxfAUID type and mxf_avid_set_auid function follow the AAF SDK naming
        mxf_avid_set_auid(&label, &auid);
        picture_descriptor->setCodingEquations(auid);
    } else {
        picture_descriptor->setCodingEquations(label);
    }
}

void PictureMXFDescriptorHelper::SetColorSiting(uint8_t color_siting)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    if ((mFlavour & MXFDESC_SMPTE_377_2004_FLAVOUR)) {
        if (color_siting > MXF_COLOR_SITING_REC601)
            cdci_descriptor->setColorSiting(MXF_COLOR_SITING_UNKNOWN);
        else
            cdci_descriptor->setColorSiting(color_siting);
    } else {
        // MXF_COLOR_SITING_VERT_MIDPOINT is 2nd of 2 additional color siting enums added in SMPTE 377-1
        if (color_siting > MXF_COLOR_SITING_VERT_MIDPOINT)
            cdci_descriptor->setColorSiting(MXF_COLOR_SITING_UNKNOWN);
        else
            cdci_descriptor->setColorSiting(color_siting);
    }
}

