/*
 * Copyright (C) 2017, British Broadcasting Corporation
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

#include <bmx/mxf_helper/RDD36MXFDescriptorHelper.h>
#include <bmx/mxf_helper/AVCMXFDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


typedef struct
{
    // known values
    uint32_t width;
    uint32_t height;
    bool is_full_frame;
    mxfUL color_primaries;
    mxfUL transfer_characteristic;
    mxfUL matrix_coefficients;

    // guesses
    uint8_t signal_standard;
    Rational aspect_ratio;
    int32_t video_line_map[2];
    uint8_t color_siting;
} DefaultParameterMatch;

// TODO: add more defaults
// TODO: are the SD NTSC video line numbers correct?
static const DefaultParameterMatch DEFAULT_PARAM_MATCHES[] =
{
    {720, 480, false, SMPTE170M_COLOR_PRIM, ITUR_BT709_TRANSFER_CH, ITUR_BT601_CODING_EQ,
            MXF_SIGNAL_STANDARD_ITU601, {16, 9}, {23, 285}, MXF_COLOR_SITING_COSITING},
    {720, 576, false, ITUR_BT470_TRANSFER_CH, ITUR_BT709_TRANSFER_CH, ITUR_BT601_CODING_EQ,
            MXF_SIGNAL_STANDARD_ITU601, {16, 9}, {23, 336}, MXF_COLOR_SITING_COSITING},
    {1280, 720, true, ITU709_COLOR_PRIM, ITUR_BT709_TRANSFER_CH, ITUR_BT709_CODING_EQ,
            MXF_SIGNAL_STANDARD_SMPTE296M, {16, 9}, {26, 0}, MXF_COLOR_SITING_COSITING},
    {1920, 1080, false, ITU709_COLOR_PRIM, ITUR_BT709_TRANSFER_CH, ITUR_BT709_CODING_EQ,
            MXF_SIGNAL_STANDARD_SMPTE274M, {16, 9}, {21, 584}, MXF_COLOR_SITING_COSITING},
    {1920, 1080, true, ITU709_COLOR_PRIM, ITUR_BT709_TRANSFER_CH, ITUR_BT709_CODING_EQ,
            MXF_SIGNAL_STANDARD_SMPTE274M, {16, 9}, {42, 0}, MXF_COLOR_SITING_COSITING},
    {3840, 2160, false, ITU2020_COLOR_PRIM, ITU2020_TRANSFER_CH, ITU2020_NCL_CODING_EQ,
            MXF_SIGNAL_STANDARD_NONE, {16, 9}, {1, 0}, MXF_COLOR_SITING_COSITING},
};

typedef struct
{
    mxfUL pc_label;
    EssenceType essence_type;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_CMDEF_L(RDD36_422_PROXY),   RDD36_422_PROXY},
    {MXF_CMDEF_L(RDD36_422_LT),      RDD36_422_LT},
    {MXF_CMDEF_L(RDD36_422),         RDD36_422},
    {MXF_CMDEF_L(RDD36_422_HQ),      RDD36_422_HQ},
    {MXF_CMDEF_L(RDD36_4444),        RDD36_4444},
    {MXF_CMDEF_L(RDD36_4444_XQ),     RDD36_4444_XQ},
};


EssenceType RDD36MXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(RDD36FrameWrapped)) &&
        !mxf_equals_ul_mod_regver(&alternative_ec_label, &MXF_EC_L(RDD36FrameWrapped)))
    {
        return UNKNOWN_ESSENCE_TYPE;
    }

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor);
    if (!cdci_descriptor || !cdci_descriptor->havePictureEssenceCoding())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = cdci_descriptor->getPictureEssenceCoding();
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (mxf_equals_ul_mod_regver(&pc_label, &SUPPORTED_ESSENCE[i].pc_label))
            return SUPPORTED_ESSENCE[i].essence_type;
    }

    return UNKNOWN_ESSENCE_TYPE;
}

bool RDD36MXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (essence_type == SUPPORTED_ESSENCE[i].essence_type)
            return true;
    }
    return false;
}

RDD36MXFDescriptorHelper::RDD36MXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceType = RDD36_422;
    mGuessProps = true;
    BMX_OPT_PROP_DEFAULT(mIsOpaque, false);
    BMX_OPT_PROP_DEFAULT(mComponentDepth, 10);
}

RDD36MXFDescriptorHelper::~RDD36MXFDescriptorHelper()
{
}

void RDD36MXFDescriptorHelper::SetGuessProps(bool enable)
{
    mGuessProps = enable;
}

void RDD36MXFDescriptorHelper::SetIsOpaque(bool opaque)
{
    BMX_OPT_PROP_SET(mIsOpaque, opaque);
}

void RDD36MXFDescriptorHelper::SetComponentDepth(uint32_t depth)
{
    BMX_OPT_PROP_SET(mComponentDepth, depth);
}

void RDD36MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                          mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor);
    BMX_ASSERT(cdci_descriptor);

    if (!BMX_OPT_PROP_IS_SET(mIsOpaque) && cdci_descriptor->haveAlphaSampleDepth())
        BMX_OPT_PROP_SET(mIsOpaque, false);
    if (!BMX_OPT_PROP_IS_SET(mComponentDepth) && cdci_descriptor->haveComponentDepth())
        BMX_OPT_PROP_SET(mComponentDepth, cdci_descriptor->getComponentDepth());
}

void RDD36MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == essence_type)
            break;
    }
    BMX_CHECK(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));

    PictureMXFDescriptorHelper::SetEssenceType(essence_type);
}

FileDescriptor* RDD36MXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);

    UpdateFileDescriptor();

    return mFileDescriptor;
}

void RDD36MXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (SUPPORTED_ESSENCE[i].essence_type == mEssenceType) {
            cdci_descriptor->setPictureEssenceCoding(SUPPORTED_ESSENCE[i].pc_label);
            break;
        }
    }
    BMX_ASSERT(i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE));

    if (BMX_OPT_PROP_IS_SET(mComponentDepth))
        cdci_descriptor->setComponentDepth(mComponentDepth);
}

void RDD36MXFDescriptorHelper::UpdateFileDescriptor(FileDescriptor *file_desc_in)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    CDCIEssenceDescriptor *cdci_desc_in = dynamic_cast<CDCIEssenceDescriptor*>(file_desc_in);
    BMX_CHECK(cdci_desc_in);

    if (!BMX_OPT_PROP_IS_SET(mIsOpaque))
        BMX_OPT_PROP_SET(mIsOpaque, !cdci_descriptor->haveAlphaSampleDepth());

#define SET_PROPERTY(name)                                                \
    if (cdci_desc_in->have##name() && !cdci_descriptor->have##name())     \
        cdci_descriptor->set##name(cdci_desc_in->get##name());

    // these properties would otherwise have to be guessed from the RDD36 stream information
    // TODO: check file descriptor properties copied across don't contradict the RDD36 information
    SET_PROPERTY(SignalStandard)
    SET_PROPERTY(AspectRatio)
    SET_PROPERTY(ActiveFormatDescriptor)
    SET_PROPERTY(VideoLineMap)
    SET_PROPERTY(ComponentDepth)
    SET_PROPERTY(BlackRefLevel)
    SET_PROPERTY(WhiteReflevel)
    SET_PROPERTY(ColorRange)
    if (!cdci_descriptor->haveColorSiting() && cdci_desc_in->haveColorSiting())
        SetColorSitingMod(cdci_desc_in->getColorSiting());

    SET_PROPERTY(MasteringDisplayPrimaries)
    SET_PROPERTY(MasteringDisplayWhitePointChromaticity)
    SET_PROPERTY(MasteringDisplayMaximumLuminance)
    SET_PROPERTY(MasteringDisplayMinimumLuminance)

    SET_PROPERTY(ActiveWidth)
    SET_PROPERTY(ActiveHeight)
    SET_PROPERTY(ActiveXOffset)
    SET_PROPERTY(ActiveYOffset)

    SET_PROPERTY(AlternativeCenterCuts);
}

void RDD36MXFDescriptorHelper::UpdateFileDescriptor(RDD36EssenceParser *essence_parser)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    mxfUL pc_label = cdci_descriptor->getPictureEssenceCoding();
    if ((essence_parser->GetChromaFormat() == RDD36_CHROMA_422 &&
            !mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(RDD36_422_PROXY)) &&
            !mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(RDD36_422_LT)) &&
            !mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(RDD36_422)) &&
            !mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(RDD36_422_HQ))) ||
        (essence_parser->GetChromaFormat() == RDD36_CHROMA_444 &&
            !mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(RDD36_4444)) &&
            !mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(RDD36_4444_XQ))))
    {
        BMX_EXCEPTION(("RDD-36 chroma format does not match picture coding label"));
    }


    if (essence_parser->GetInterlaceMode() == RDD36_PROGRESSIVE_FRAME)
        cdci_descriptor->setFrameLayout(MXF_FULL_FRAME);
    else if (!cdci_descriptor->haveFrameLayout() || cdci_descriptor->getFrameLayout() != MXF_SEGMENTED_FRAME)
        cdci_descriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
    cdci_descriptor->setDisplayWidth(essence_parser->GetWidth());
    if (cdci_descriptor->getFrameLayout() == MXF_SEPARATE_FIELDS)
        cdci_descriptor->setDisplayHeight(essence_parser->GetHeight() / 2);
    else
        cdci_descriptor->setDisplayHeight(essence_parser->GetHeight());
    if (cdci_descriptor->getFrameLayout() == MXF_SEPARATE_FIELDS) {
        if (essence_parser->GetInterlaceMode() == RDD36_INTERLACED_BFF) {
            cdci_descriptor->setFieldDominance(2);
            cdci_descriptor->setDisplayF2Offset(-1);
        } else { // RDD36_INTERLACED_TFF
            cdci_descriptor->setFieldDominance(1);
            cdci_descriptor->setDisplayF2Offset(0);
        }
    } else if  ((mFlavour & MXFDESC_IMF_FLAVOUR)) {
        cdci_descriptor->setDisplayF2Offset(0);
    }

    // stored dimensions are a multiple of 16, the macro block size
    cdci_descriptor->setStoredWidth((cdci_descriptor->getDisplayWidth() + 15) / 16 * 16);
    cdci_descriptor->setStoredHeight((cdci_descriptor->getDisplayHeight() + 15) / 16 * 16);
    cdci_descriptor->setDisplayXOffset(0);
    cdci_descriptor->setDisplayYOffset(0);
    if (essence_parser->GetChromaFormat() == RDD36_CHROMA_422)
        cdci_descriptor->setHorizontalSubsampling(2);
    else // RDD36_CHROMA_444
        cdci_descriptor->setHorizontalSubsampling(1);
    cdci_descriptor->setVerticalSubsampling(1);
    if (essence_parser->GetChromaFormat() == RDD36_CHROMA_444 && !mIsOpaque) {
        if (essence_parser->GetAlphaChannelType() == RDD36_ALPHA_8BIT)
            cdci_descriptor->setAlphaSampleDepth(8);
        else if (essence_parser->GetAlphaChannelType() == RDD36_ALPHA_16BIT)
            cdci_descriptor->setAlphaSampleDepth(16);
    }
    if (!cdci_descriptor->haveComponentDepth())
        cdci_descriptor->setComponentDepth(mComponentDepth);
    uint8_t component_depth = cdci_descriptor->getComponentDepth();
    if (component_depth >= 8 &&
        !cdci_descriptor->haveBlackRefLevel() &&
        !cdci_descriptor->haveWhiteReflevel() &&
        !cdci_descriptor->haveColorRange())
    {
        cdci_descriptor->setBlackRefLevel(16 << (component_depth - 8));
        cdci_descriptor->setWhiteReflevel(235 << (component_depth - 8));
        cdci_descriptor->setColorRange((240 << (component_depth - 8)) - (16 << (component_depth - 8)) + 1);
    }
    if (!cdci_descriptor->haveColorPrimaries())
        AVCMXFDescriptorHelper::MapColorPrimaries(essence_parser->GetColorPrimaries(), this);
    if (!cdci_descriptor->haveCaptureGamma())
        AVCMXFDescriptorHelper::MapTransferCharacteristic(essence_parser->GetTransferCharacteristic(), this);
    if (!cdci_descriptor->haveCodingEquations())
        AVCMXFDescriptorHelper::MapMatrixCoefficients(essence_parser->GetMatrixCoefficients(), this);
    if (!cdci_descriptor->haveAspectRatio()) {
        if (essence_parser->GetAspectRatioCode() == RDD36_ASPECT_RATIO_4X3)
            cdci_descriptor->setAspectRatio(ASPECT_RATIO_4_3);
        else if (essence_parser->GetAspectRatioCode() == RDD36_ASPECT_RATIO_16X9)
            cdci_descriptor->setAspectRatio(ASPECT_RATIO_16_9);
    }

    if (mGuessProps) {
        uint32_t width = essence_parser->GetWidth();
        uint32_t height = essence_parser->GetHeight();
        bool is_full_frame = (cdci_descriptor->getFrameLayout() == MXF_FULL_FRAME);
        mxfUL color_primaries = g_Null_UL;
        mxfUL transfer_characteristic = g_Null_UL;
        mxfUL matrix_coefficients = g_Null_UL;
        if (cdci_descriptor->haveColorPrimaries())
            color_primaries = cdci_descriptor->getColorPrimaries();
        if (cdci_descriptor->haveCaptureGamma())
            transfer_characteristic = cdci_descriptor->getCaptureGamma();
        if (cdci_descriptor->haveCodingEquations())
            matrix_coefficients = cdci_descriptor->getCodingEquations();

        size_t i;
        for (i = 0; i < BMX_ARRAY_SIZE(DEFAULT_PARAM_MATCHES); i++) {
            if (DEFAULT_PARAM_MATCHES[i].width == width &&
                DEFAULT_PARAM_MATCHES[i].height == height &&
                DEFAULT_PARAM_MATCHES[i].is_full_frame == is_full_frame &&
                (DEFAULT_PARAM_MATCHES[i].color_primaries == color_primaries ||
                    color_primaries == g_Null_UL) &&
                (DEFAULT_PARAM_MATCHES[i].transfer_characteristic == transfer_characteristic ||
                    transfer_characteristic == g_Null_UL) &&
                (DEFAULT_PARAM_MATCHES[i].matrix_coefficients == matrix_coefficients ||
                    matrix_coefficients == g_Null_UL))
            {
                if (!cdci_descriptor->haveSignalStandard())
                    cdci_descriptor->setSignalStandard(DEFAULT_PARAM_MATCHES[i].signal_standard);
                if (!cdci_descriptor->haveAspectRatio())
                    cdci_descriptor->setAspectRatio(DEFAULT_PARAM_MATCHES[i].aspect_ratio);
                if (!cdci_descriptor->haveVideoLineMap()) {
                    cdci_descriptor->appendVideoLineMap(DEFAULT_PARAM_MATCHES[i].video_line_map[0]);
                    cdci_descriptor->appendVideoLineMap(DEFAULT_PARAM_MATCHES[i].video_line_map[1]);
                }
                if (!cdci_descriptor->haveColorSiting())
                    SetColorSitingMod(DEFAULT_PARAM_MATCHES[i].color_siting);
                break;
            }
        }
    }

    if (!cdci_descriptor->haveAspectRatio()) {
        Rational calc_aspect_ratio;
        calc_aspect_ratio.numerator   = essence_parser->GetWidth();
        calc_aspect_ratio.denominator = essence_parser->GetHeight();
        cdci_descriptor->setAspectRatio(reduce_rational(calc_aspect_ratio));
    }
    if (!cdci_descriptor->haveVideoLineMap()) {
        if (cdci_descriptor->getFrameLayout() == MXF_FULL_FRAME) {
            cdci_descriptor->appendVideoLineMap(1);
            cdci_descriptor->appendVideoLineMap(0);
        } else {
            cdci_descriptor->appendVideoLineMap(1);
            cdci_descriptor->appendVideoLineMap(essence_parser->GetHeight() / 2 + 1);
        }
    }
}

mxfUL RDD36MXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return MXF_EC_L(RDD36FrameWrapped);
}
