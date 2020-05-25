/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#include <bmx/mxf_helper/AVCMXFDescriptorHelper.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    mxfUL pc_label;
    EssenceType essence_type;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(AVC_BASELINE),             AVC_BASELINE},
    {MXF_CMDEF_L(AVC_CONSTRAINED_BASELINE), AVC_CONSTRAINED_BASELINE},
    {MXF_CMDEF_L(AVC_MAIN),                 AVC_MAIN},
    {MXF_CMDEF_L(AVC_EXTENDED),             AVC_EXTENDED},
    {MXF_CMDEF_L(AVC_HIGH),                 AVC_HIGH},
    {MXF_CMDEF_L(AVC_HIGH_10),              AVC_HIGH_10},
    {MXF_CMDEF_L(AVC_HIGH_422),             AVC_HIGH_422},
    {MXF_CMDEF_L(AVC_HIGH_444),             AVC_HIGH_444},
    {MXF_CMDEF_L(AVC_HIGH_10_INTRA),        AVC_HIGH_10_INTRA},
    {MXF_CMDEF_L(AVC_HIGH_422_INTRA),       AVC_HIGH_422_INTRA},
    {MXF_CMDEF_L(AVC_HIGH_444_INTRA),       AVC_HIGH_444_INTRA},
    {MXF_CMDEF_L(AVC_CAVLC_444_INTRA),      AVC_CAVLC_444_INTRA},
};



EssenceType AVCMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_is_avc_ec(&ec_label, 0) &&
        !mxf_is_avc_ec(&ec_label, 1) &&
        !mxf_is_avc_ec(&alternative_ec_label, 0) &&
        !mxf_is_avc_ec(&alternative_ec_label, 1) &&
        !IsNullAvidECUL(ec_label, alternative_ec_label))
    {
        return UNKNOWN_ESSENCE_TYPE;
    }

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    if (!pic_descriptor || !pic_descriptor->havePictureEssenceCoding())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool AVCMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }

    return false;
}

void AVCMXFDescriptorHelper::MapColorPrimaries(uint8_t avc_value, PictureMXFDescriptorHelper *pict_helper)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(pict_helper->GetFileDescriptor());
    BMX_ASSERT(cdci_descriptor);

    switch (avc_value) {
        case 1:
            cdci_descriptor->setColorPrimaries(ITU709_COLOR_PRIM);
            break;
        case 4:
        case 5:
            cdci_descriptor->setColorPrimaries(ITU470_PAL_COLOR_PRIM);
            break;
        case 6:
        case 7:
            cdci_descriptor->setColorPrimaries(SMPTE170M_COLOR_PRIM);
            break;
        case 9:
            cdci_descriptor->setColorPrimaries(ITU2020_COLOR_PRIM);
            break;
        case 10:
            cdci_descriptor->setColorPrimaries(SMPTE_DCDM_COLOR_PRIM);
            break;
        default:
            break;
    }
}

void AVCMXFDescriptorHelper::MapTransferCharacteristic(uint8_t avc_value, PictureMXFDescriptorHelper *pict_helper)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(pict_helper->GetFileDescriptor());
    BMX_ASSERT(cdci_descriptor);

    switch (avc_value) {
        case 1:
        case 6:
            cdci_descriptor->setCaptureGamma(ITUR_BT709_TRANSFER_CH);
            break;
        case 4:
        case 5:
            cdci_descriptor->setCaptureGamma(ITUR_BT470_TRANSFER_CH);
            break;
        case 7:
            cdci_descriptor->setCaptureGamma(SMPTE240M_TRANSFER_CH);
            break;
        case 8:
            cdci_descriptor->setCaptureGamma(LINEAR_TRANSFER_CH);
            break;
        case 11:
            cdci_descriptor->setCaptureGamma(IEC6196624_XVYCC_TRANSFER_CH);
            break;
        case 12:
            cdci_descriptor->setCaptureGamma(ITU1361_TRANSFER_CH);
            break;
        case 14:
        case 15:
            cdci_descriptor->setCaptureGamma(ITU2020_TRANSFER_CH);
            break;
        case 16:
            cdci_descriptor->setCaptureGamma(SMPTE_ST2084_TRANSFER_CH);
            break;
        case 17:
            cdci_descriptor->setCaptureGamma(SMPTE_DCDM_TRANSFER_CH);
            break;
        case 18:
            cdci_descriptor->setCaptureGamma(HLG_OETF_TRANSFER_CH);
            break;
        default:
            break;
    }
}

void AVCMXFDescriptorHelper::MapMatrixCoefficients(uint8_t avc_value, PictureMXFDescriptorHelper *pict_helper)
{
    switch (avc_value) {
        case 0:
            pict_helper->SetCodingEquationsMod(GBR_CODING_EQ);
            break;
        case 1:
            pict_helper->SetCodingEquationsMod(ITUR_BT709_CODING_EQ);
            break;
        case 5:
        case 6:
            pict_helper->SetCodingEquationsMod(ITUR_BT601_CODING_EQ);
            break;
        case 7:
            pict_helper->SetCodingEquationsMod(SMPTE_240M_CODING_EQ);
            break;
        case 8:
            pict_helper->SetCodingEquationsMod(Y_CG_CO_CODING_EQ);
            break;
        case 9:
            pict_helper->SetCodingEquationsMod(ITU2020_NCL_CODING_EQ);
            break;
        default:
            break;
    }
}

AVCMXFDescriptorHelper::AVCMXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceIndex = 0;
    mEssenceType = SUPPORTED_ESSENCE[0].essence_type;
    mAVCSubDescriptor = 0;
}

AVCMXFDescriptorHelper::~AVCMXFDescriptorHelper()
{
}

void AVCMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                        mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mFrameWrapped = (mxf_is_avc_ec(&ec_label, 1) || mxf_is_avc_ec(&alternative_ec_label, 1));

    GenericPictureEssenceDescriptor *pic_descriptor = dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    mxfUL pc_label = pic_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label))
        {
            mEssenceIndex = i;
            mEssenceType = SUPPORTED_ESSENCE[i].essence_type;
            break;
        }
    }

    if (file_descriptor->haveSubDescriptors()) {
        vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
        for (i = 0; i < sub_descriptors.size(); i++) {
            mAVCSubDescriptor = dynamic_cast<AVCSubDescriptor*>(sub_descriptors[i]);
            if (mAVCSubDescriptor)
                break;
        }
    }
}

void AVCMXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);

    UpdateEssenceIndex();
}

FileDescriptor* AVCMXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    UpdateEssenceIndex();

    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    mAVCSubDescriptor = new AVCSubDescriptor(header_metadata);
    mFileDescriptor->appendSubDescriptors(mAVCSubDescriptor);

    UpdateFileDescriptor();

    return mFileDescriptor;
}

void AVCMXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[mEssenceIndex].pc_label);
}

void AVCMXFDescriptorHelper::UpdateFileDescriptor(FileDescriptor *file_desc_in)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    CDCIEssenceDescriptor *cdci_desc_in = dynamic_cast<CDCIEssenceDescriptor*>(file_desc_in);
    BMX_CHECK(cdci_desc_in);

#define SET_PROPERTY(name)                                                \
    if (cdci_desc_in->have##name() && !cdci_descriptor->have##name())     \
        cdci_descriptor->set##name(cdci_desc_in->get##name());

    // TODO: move these to PictureMXFDescriptorHelper once this update is supported across all formats

    SET_PROPERTY(SignalStandard)
    SET_PROPERTY(FrameLayout)
    SET_PROPERTY(AspectRatio)
    SET_PROPERTY(ActiveFormatDescriptor)
    SET_PROPERTY(VideoLineMap)
    SET_PROPERTY(FieldDominance)
    SET_PROPERTY(CaptureGamma)
    SET_PROPERTY(CodingEquations)
    SET_PROPERTY(ColorPrimaries)
    if (!cdci_descriptor->haveColorSiting() && cdci_desc_in->haveColorSiting())
        SetColorSitingMod(cdci_desc_in->getColorSiting());
    SET_PROPERTY(BlackRefLevel)
    SET_PROPERTY(WhiteReflevel)
    SET_PROPERTY(ColorRange)
}

void AVCMXFDescriptorHelper::UpdateFileDescriptor(AVCEssenceParser *essence_parser)
{
    UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    cdci_descriptor->setStoredWidth(essence_parser->GetStoredWidth());
    cdci_descriptor->setStoredHeight(essence_parser->GetStoredHeight());
    cdci_descriptor->setDisplayWidth(essence_parser->GetDisplayWidth());
    cdci_descriptor->setDisplayHeight(essence_parser->GetDisplayHeight());
    cdci_descriptor->setDisplayXOffset(essence_parser->GetDisplayXOffset());
    cdci_descriptor->setDisplayYOffset(essence_parser->GetDisplayYOffset());
    cdci_descriptor->setSampledWidth(essence_parser->GetStoredWidth());
    cdci_descriptor->setSampledHeight(essence_parser->GetStoredHeight());
    cdci_descriptor->setSampledXOffset(0);
    cdci_descriptor->setSampledYOffset(0);
    cdci_descriptor->setImageStartOffset(0);
    cdci_descriptor->setPaddingBits(0);

    // TODO: Display/StoredF2Offset
    // TODO: need to parse the pic_timing SEI to set this
    //if (!cdci_descriptor->haveFieldDominance()) ...

    cdci_descriptor->setComponentDepth(essence_parser->GetComponentDepth());

    // TODO: check existing chroma siting doesn't contradict chroma format
    switch (essence_parser->GetChromaFormat())
    {
        case 1: // 4:2:0
            cdci_descriptor->setHorizontalSubsampling(2);
            cdci_descriptor->setVerticalSubsampling(2);
            // TODO: assuming chroma sample locations are identical on both fields
            if (!cdci_descriptor->haveColorSiting()) {
                switch (essence_parser->GetChromaLocation())
                {
                    case 0:
                        SetColorSitingMod(MXF_COLOR_SITING_VERT_MIDPOINT);
                        break;
                    case 1:
                        SetColorSitingMod(MXF_COLOR_SITING_QUINCUNX);
                        break;
                    case 2:
                        SetColorSitingMod(MXF_COLOR_SITING_COSITING);
                        break;
                    case 3:
                        SetColorSitingMod(MXF_COLOR_SITING_HORIZ_MIDPOINT);
                        break;
                    default:
                        break;
                }
            }
            break;
        case 2: // 4:2:2
            cdci_descriptor->setHorizontalSubsampling(2);
            cdci_descriptor->setVerticalSubsampling(1);
            if (!cdci_descriptor->haveColorSiting())
                SetColorSitingMod(MXF_COLOR_SITING_COSITING);
            break;
        case 3: // 4:4:4
            cdci_descriptor->setHorizontalSubsampling(1);
            cdci_descriptor->setVerticalSubsampling(1);
            if (!cdci_descriptor->haveColorSiting())
                SetColorSitingMod(MXF_COLOR_SITING_COSITING);
            break;
        case 0: // Monochrome
        default:
            break;
    }

    // TODO: extract BlackRefLevel, WhiteRefLevel, ColorRange if possible
    // TODO: check levels and range don't component depth (and possibly full range flag?)

    if (!cdci_descriptor->haveSignalStandard()) {
        switch (essence_parser->GetVideoFormat()) {
            case 1:
            case 2:
                cdci_descriptor->setSignalStandard(MXF_SIGNAL_STANDARD_ITU601);
                break;
            default:
                break;
        }
    }
    if (!cdci_descriptor->haveColorPrimaries())
        MapColorPrimaries(essence_parser->GetColorPrimaries(), this);
    if (!cdci_descriptor->haveCaptureGamma())
        MapTransferCharacteristic(essence_parser->GetTransferCharacteristics(), this);
    if (!cdci_descriptor->haveCodingEquations())
        MapMatrixCoefficients(essence_parser->GetMatrixCoefficients(), this);

    if (!cdci_descriptor->haveAspectRatio() && essence_parser->GetSampleAspectRatio().numerator > 0) {
        // TODO: this calculation will not always be correct because the "clean area" is unknown
        Rational sample_aspect_ratio = essence_parser->GetSampleAspectRatio();
        Rational calc_aspect_ratio;
        calc_aspect_ratio.numerator   = (int32_t)(sample_aspect_ratio.numerator   * essence_parser->GetDisplayWidth());
        calc_aspect_ratio.denominator = (int32_t)(sample_aspect_ratio.denominator * essence_parser->GetDisplayHeight());
        cdci_descriptor->setAspectRatio(reduce_rational(calc_aspect_ratio));
    }

    BMX_ASSERT(mAVCSubDescriptor);

    mAVCSubDescriptor->setAVCProfile(essence_parser->GetProfile());
    mAVCSubDescriptor->setAVCProfileConstraint(essence_parser->GetProfileConstraint());
    mAVCSubDescriptor->setAVCLevel(essence_parser->GetLevel());
}

mxfUL AVCMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if (mFrameWrapped)
        return MXF_EC_L(AVCFrameWrapped);
    else
        return MXF_EC_L(AVCClipWrapped);
}

void AVCMXFDescriptorHelper::UpdateEssenceIndex()
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType) {
            mEssenceIndex = i;
            break;
        }
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));
}

