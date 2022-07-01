/*
 * Copyright (C) 2020, British Broadcasting Corporation
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

#include <mxf/mxf.h>
#include <mxf/mxf_labels_and_keys.h>

#include <bmx/mxf_helper/JPEG2000MXFDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


EssenceType JPEG2000MXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (mxf_is_jpeg2000_ec(&ec_label) || mxf_is_jpeg2000_ec(&alternative_ec_label)) {
        if (dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor))
            return JPEG2000_CDCI;
        else if (dynamic_cast<RGBAEssenceDescriptor*>(file_descriptor))
            return JPEG2000_RGBA;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool JPEG2000MXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return essence_type == JPEG2000_CDCI || essence_type == JPEG2000_RGBA;
}

JPEG2000MXFDescriptorHelper::JPEG2000MXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceType = JPEG2000_CDCI;
    mJPEG2000SubDescriptor = 0;
}

JPEG2000MXFDescriptorHelper::~JPEG2000MXFDescriptorHelper()
{
}

void JPEG2000MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                             mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    if (dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor))
        mEssenceType = JPEG2000_CDCI;
    else
        mEssenceType = JPEG2000_RGBA;

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    if (file_descriptor->haveSubDescriptors()) {
        vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
        for (size_t i = 0; i < sub_descriptors.size(); i++) {
            mJPEG2000SubDescriptor = dynamic_cast<JPEG2000SubDescriptor*>(sub_descriptors[i]);
            if (mJPEG2000SubDescriptor)
                break;
        }
    }
}

void JPEG2000MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    BMX_CHECK(essence_type == JPEG2000_CDCI || essence_type == JPEG2000_RGBA);
    PictureMXFDescriptorHelper::SetEssenceType(essence_type);
}

FileDescriptor* JPEG2000MXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    if (mEssenceType == JPEG2000_CDCI)
        mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    else
        mFileDescriptor = new RGBAEssenceDescriptor(header_metadata);
    mJPEG2000SubDescriptor = new JPEG2000SubDescriptor(header_metadata);
    mFileDescriptor->appendSubDescriptors(mJPEG2000SubDescriptor);

    UpdateFileDescriptor();

    return mFileDescriptor;
}

void JPEG2000MXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    // Currently only support progressive video frame layout
    GenericPictureEssenceDescriptor *pict_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
    pict_descriptor->setFrameLayout(MXF_FULL_FRAME);

    if ((mFlavour & MXFDESC_IMF_FLAVOUR)) {
        BMX_ASSERT(pict_descriptor->getFrameLayout() == MXF_FULL_FRAME);
        pict_descriptor->setDisplayF2Offset(0);
    }
}

void JPEG2000MXFDescriptorHelper::UpdateFileDescriptor(FileDescriptor *file_desc_in)
{
#define SET_PROPERTY(name)                                                \
    if (comp_desc_in->have##name() && !comp_desc->have##name())     \
        comp_desc->set##name(comp_desc_in->get##name());

    GenericPictureEssenceDescriptor *comp_desc = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);
    GenericPictureEssenceDescriptor *comp_desc_in = dynamic_cast<GenericPictureEssenceDescriptor*>(file_desc_in);

    if (comp_desc_in->haveFrameLayout() && comp_desc_in->getFrameLayout() != MXF_FULL_FRAME)
        BMX_EXCEPTION(("Only progressive video frame layout is currently supported"));

    SET_PROPERTY(FrameLayout)
    SET_PROPERTY(SignalStandard)
    SET_PROPERTY(AspectRatio)
    SET_PROPERTY(ActiveFormatDescriptor)
    SET_PROPERTY(VideoLineMap)
    if ((mFlavour & MXFDESC_IMF_FLAVOUR)) {
        BMX_ASSERT(comp_desc->getFrameLayout() == MXF_FULL_FRAME);
        comp_desc->setDisplayF2Offset(0);
    } else {
        SET_PROPERTY(FieldDominance)
        SET_PROPERTY(DisplayF2Offset)
    }
    SET_PROPERTY(CaptureGamma)
    SET_PROPERTY(CodingEquations)
    SET_PROPERTY(ColorPrimaries)

    SET_PROPERTY(MasteringDisplayPrimaries)
    SET_PROPERTY(MasteringDisplayWhitePointChromaticity)
    SET_PROPERTY(MasteringDisplayMaximumLuminance)
    SET_PROPERTY(MasteringDisplayMinimumLuminance)

    SET_PROPERTY(ActiveWidth)
    SET_PROPERTY(ActiveHeight)
    SET_PROPERTY(ActiveXOffset)
    SET_PROPERTY(ActiveYOffset)

    SET_PROPERTY(AlternativeCenterCuts);

    if (dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor)) {
        CDCIEssenceDescriptor *comp_desc = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
        CDCIEssenceDescriptor *comp_desc_in = dynamic_cast<CDCIEssenceDescriptor*>(file_desc_in);

        if (!comp_desc->haveColorSiting() && comp_desc_in->haveColorSiting())
            SetColorSitingMod(comp_desc_in->getColorSiting());
        SET_PROPERTY(BlackRefLevel)
        SET_PROPERTY(WhiteReflevel)
        SET_PROPERTY(ColorRange)
    }

    if (dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor)) {
        RGBAEssenceDescriptor *comp_desc = dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor);
        RGBAEssenceDescriptor *comp_desc_in = dynamic_cast<RGBAEssenceDescriptor*>(file_desc_in);

        SET_PROPERTY(ComponentMaxRef)
        SET_PROPERTY(ComponentMinRef)
        SET_PROPERTY(ScanningDirection)
    }
}

void JPEG2000MXFDescriptorHelper::UpdateFileDescriptor(J2CEssenceParser *essence_parser)
{
    UpdateFileDescriptor();

    // Determine this first because it will validate assumptions further below for YUV components.
    mxfRGBALayout j2c_layout;
    if (!essence_parser->GuessRGBALayout(mEssenceType == JPEG2000_CDCI, mEssenceType == JPEG2000_RGBA, false, &j2c_layout))
        BMX_EXCEPTION(("Unsupported JPEG2000 component layout"));

    mJPEG2000SubDescriptor->setRsiz(essence_parser->GetRsiz());
    mJPEG2000SubDescriptor->setXsiz(essence_parser->GetXsiz());
    mJPEG2000SubDescriptor->setYsiz(essence_parser->GetYsiz());
    mJPEG2000SubDescriptor->setXOsiz(essence_parser->GetXOsiz());
    mJPEG2000SubDescriptor->setYOsiz(essence_parser->GetYOsiz());
    mJPEG2000SubDescriptor->setXTsiz(essence_parser->GetXTsiz());
    mJPEG2000SubDescriptor->setYTsiz(essence_parser->GetYTsiz());
    mJPEG2000SubDescriptor->setXTOsiz(essence_parser->GetXTOsiz());
    mJPEG2000SubDescriptor->setYTOsiz(essence_parser->GetYTOsiz());
    mJPEG2000SubDescriptor->setCsiz(essence_parser->GetCsiz());
    mJPEG2000SubDescriptor->setPictureComponentSizing(essence_parser->GetComponentSizings());
    mJPEG2000SubDescriptor->setJ2CLayout(j2c_layout);

    GenericPictureEssenceDescriptor *pict_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(mFileDescriptor);

    mxfUL pc_label;
    mxf_get_jpeg2000_coding_label(essence_parser->GetProfile(), essence_parser->GetMainLevel(), essence_parser->GetSubLevel(), &pc_label);
    pict_descriptor->setPictureEssenceCoding(pc_label);

    // The MXF stored grid are set to equal the JPEG 2000 tiled grid.
    // Note that the JPEG 2000 tiled grid can extend beyond the reference grid - see the rounding up below.
    int tiled_grid_size_x = essence_parser->GetXsiz() - essence_parser->GetXTOsiz();
    int tiled_grid_size_y = essence_parser->GetYsiz() - essence_parser->GetYTOsiz();
    int tile_size_x = essence_parser->GetXTsiz();
    int tile_size_y = essence_parser->GetYTsiz();
    int tiles_num_x = (tiled_grid_size_x + tile_size_x - 1) / tile_size_x;  // rounded up
    int tiles_num_y = (tiled_grid_size_y + tile_size_y - 1) / tile_size_y;  // rounded up
    pict_descriptor->setStoredWidth(tile_size_x * tiles_num_x);
    pict_descriptor->setStoredHeight(tile_size_y * tiles_num_y);

    // The MXF sampled grid is assumed to be the MXF default which is the stored grid.
    pict_descriptor->setSampledWidth(pict_descriptor->getStoredWidth());
    pict_descriptor->setSampledHeight(pict_descriptor->getStoredHeight());
    pict_descriptor->setSampledXOffset(0);
    pict_descriptor->setSampledYOffset(0);

    // The MXF display grid is set equal to the JPEG2000 reference grid dimension minus the image offsets.
    // Note that the MXF display offset is relative to the MXF sampled grid. The JPEG2000 spec. requires
    // 0 <= XTOsiz <= XOsiz and 0 <= YTOsiz <= YOsiz. This means the MXF display offsets,
    // which are relative to the MXF sampled grid, will be >= 0.
    pict_descriptor->setDisplayWidth(essence_parser->GetXsiz() - essence_parser->GetXOsiz());
    pict_descriptor->setDisplayHeight(essence_parser->GetYsiz() - essence_parser->GetYOsiz());
    pict_descriptor->setDisplayXOffset(essence_parser->GetXOsiz() - essence_parser->GetXTOsiz());
    pict_descriptor->setDisplayYOffset(essence_parser->GetYOsiz() - essence_parser->GetYTOsiz());

    if (!pict_descriptor->haveAspectRatio()) {
        Rational calc_aspect_ratio;
        calc_aspect_ratio.numerator = pict_descriptor->getDisplayWidth();
        calc_aspect_ratio.denominator = pict_descriptor->getDisplayHeight();
        pict_descriptor->setAspectRatio(reduce_rational(calc_aspect_ratio));
    }
    if (!pict_descriptor->haveVideoLineMap()) {
        if (pict_descriptor->getFrameLayout() == MXF_FULL_FRAME) {
            pict_descriptor->appendVideoLineMap(1);
            pict_descriptor->appendVideoLineMap(0);
        } else {
            pict_descriptor->appendVideoLineMap(1);
            pict_descriptor->appendVideoLineMap(pict_descriptor->getDisplayHeight() / 2 + 1);
        }
    }

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    if (cdci_descriptor) {
        cdci_descriptor->setPaddingBits(0);
        cdci_descriptor->setComponentDepth(essence_parser->GetCompBitDepth(0));
        cdci_descriptor->setHorizontalSubsampling(essence_parser->GetComponentSizings()[1].xr_siz);
        cdci_descriptor->setVerticalSubsampling(essence_parser->GetComponentSizings()[1].yr_siz);
    }

    RGBAEssenceDescriptor *rgba_descriptor = dynamic_cast<RGBAEssenceDescriptor*>(mFileDescriptor);
    if (rgba_descriptor) {
        // Include fill in the pixel layout
        mxfRGBALayout pixel_layout;
        if (!essence_parser->GuessRGBALayout(mEssenceType == JPEG2000_CDCI, mEssenceType == JPEG2000_RGBA, true, &pixel_layout))
            BMX_EXCEPTION(("Unsupported JPEG2000 component layout"));
        rgba_descriptor->setPixelLayout(pixel_layout);
    }
}

mxfUL JPEG2000MXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return MXF_EC_L(JPEG2000P1Wrapped);
}
