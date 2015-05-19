/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#include <bmx/mxf_helper/VC2MXFDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


typedef struct
{
    VC2EssenceParser::VideoParameters params;
    uint8_t signal_standard;
    Rational aspect_ratio;
    int32_t video_line_map[2];
    uint8_t color_siting;
} DefaultParameterMatch;

// TODO: add more defaults
// TODO: are the SD NTSC video line numbers correct?
static const DefaultParameterMatch DEFAULT_PARAM_MATCHES[] =
{
    {{720, 480, 1, 1, 0, 0, 0, 10, 11, 0, 480, 0, 0, 0, 0, 0, 0, 1, 1, 0},
            MXF_SIGNAL_STANDARD_ITU601, {4, 3}, {23, 285}, MXF_COLOR_SITING_COSITING},
    {{720, 480, 1, 1, 0, 0, 0, 40, 33, 0, 480, 0, 0, 0, 0, 0, 0, 1, 1, 0},
            MXF_SIGNAL_STANDARD_ITU601, {16, 9}, {23, 285}, MXF_COLOR_SITING_COSITING},
    {{720, 576, 1, 1, 1, 0, 0, 12, 11, 0, 576, 0, 0, 0, 0, 0, 0, 2, 1, 0},
            MXF_SIGNAL_STANDARD_ITU601, {4, 3}, {23, 336}, MXF_COLOR_SITING_COSITING},
    {{720, 576, 1, 1, 1, 0, 0, 16, 11, 0, 576, 0, 0, 0, 0, 0, 0, 2, 1, 0},
            MXF_SIGNAL_STANDARD_ITU601, {16, 9}, {23, 336}, MXF_COLOR_SITING_COSITING},
    {{1280, 720, 1, 0, 1, 0, 0,  1, 1, 1280, 720, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            MXF_SIGNAL_STANDARD_SMPTE296M, {16, 9}, {26, 0}, MXF_COLOR_SITING_COSITING},
    {{1920, 1080, 1, 1, 1, 0, 0, 1, 1, 1920, 1080, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            MXF_SIGNAL_STANDARD_SMPTE274M, {16, 9}, {21, 584}, MXF_COLOR_SITING_COSITING},
    {{1920, 1080, 1, 0, 1, 0, 0, 1, 1, 1920, 1080, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            MXF_SIGNAL_STANDARD_SMPTE274M, {16, 9}, {42, 0}, MXF_COLOR_SITING_COSITING},
    {{3840, 2160, 1, 0, 1, 0, 0, 1, 1, 3840, 2160, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            MXF_SIGNAL_STANDARD_NONE, {16, 9}, {1, 0}, MXF_COLOR_SITING_COSITING},
};


EssenceType VC2MXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(VC2FrameWrapped)) &&
        !mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(VC2ClipWrapped)) &&
        !mxf_equals_ul_mod_regver(&alternative_ec_label, &MXF_EC_L(VC2FrameWrapped)) &&
        !mxf_equals_ul_mod_regver(&alternative_ec_label, &MXF_EC_L(VC2ClipWrapped)))
    {
        return UNKNOWN_ESSENCE_TYPE;
    }

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor);
    if (!cdci_descriptor || !cdci_descriptor->havePictureEssenceCoding() || !cdci_descriptor->haveSubDescriptors())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = cdci_descriptor->getPictureEssenceCoding();
    if (!mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(VC2)))
        return UNKNOWN_ESSENCE_TYPE;

    vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
    size_t i;
    for (i = 0; i < sub_descriptors.size(); i++) {
        if (dynamic_cast<VC2SubDescriptor*>(sub_descriptors[i]))
            break;
    }
    if (i >= sub_descriptors.size())
        return UNKNOWN_ESSENCE_TYPE;

    return VC2;
}

bool VC2MXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return essence_type == VC2;
}

VC2MXFDescriptorHelper::VC2MXFDescriptorHelper()
: PictureMXFDescriptorHelper()
{
    mEssenceType = VC2;
    mVC2SubDescriptor = 0;
    mGuessProps = true;
}

VC2MXFDescriptorHelper::~VC2MXFDescriptorHelper()
{
}

void VC2MXFDescriptorHelper::SetGuessProps(bool enable)
{
    mGuessProps = enable;
}

void VC2MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version,
                                        mxfUL alternative_ec_label)
{
    BMX_ASSERT(IsSupported(file_descriptor, alternative_ec_label));

    PictureMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    mFrameWrapped = mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(VC2FrameWrapped));
}

FileDescriptor* VC2MXFDescriptorHelper::CreateFileDescriptor(mxfpp::HeaderMetadata *header_metadata)
{
    mFileDescriptor = new CDCIEssenceDescriptor(header_metadata);
    mVC2SubDescriptor = new VC2SubDescriptor(header_metadata);
    mFileDescriptor->appendSubDescriptors(mVC2SubDescriptor);

    UpdateFileDescriptor();

    return mFileDescriptor;
}

void VC2MXFDescriptorHelper::UpdateFileDescriptor()
{
    PictureMXFDescriptorHelper::UpdateFileDescriptor();

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    cdci_descriptor->setPictureEssenceCoding(MXF_CMDEF_L(VC2));

    // set required properties in case the duration is 0
    mVC2SubDescriptor->setVC2MajorVersion(1);
    mVC2SubDescriptor->setVC2MinorVersion(0);
    mVC2SubDescriptor->setVC2Profile(1);
    mVC2SubDescriptor->setVC2Level(1);
}

void VC2MXFDescriptorHelper::UpdateFileDescriptor(FileDescriptor *file_desc_in)
{
    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    CDCIEssenceDescriptor *cdci_desc_in = dynamic_cast<CDCIEssenceDescriptor*>(file_desc_in);
    BMX_CHECK(cdci_descriptor);

#define SET_PROPERTY(name)                                                \
    if (cdci_desc_in->have##name() && !cdci_descriptor->have##name())     \
        cdci_descriptor->set##name(cdci_desc_in->get##name());

    // these properties would otherwise have to be guessed from the VC-2 stream information
    // TODO: check file descriptor properties copied across don't contradict the VC-2 information
    SET_PROPERTY(SignalStandard)
    SET_PROPERTY(AspectRatio)
    SET_PROPERTY(ActiveFormatDescriptor)
    SET_PROPERTY(VideoLineMap)
    if (!cdci_descriptor->haveColorSiting() && cdci_desc_in->haveColorSiting())
        SetColorSitingMod(cdci_desc_in->getColorSiting());
}

void VC2MXFDescriptorHelper::SetSequenceHeader(const VC2EssenceParser::SequenceHeader *sequence_header)
{
    BMX_ASSERT(mVC2SubDescriptor);
    mVC2SubDescriptor->setVC2MajorVersion(sequence_header->major_version);
    mVC2SubDescriptor->setVC2MinorVersion(sequence_header->minor_version);
    mVC2SubDescriptor->setVC2Profile(sequence_header->profile);
    mVC2SubDescriptor->setVC2Level(sequence_header->level);

    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(mFileDescriptor);
    BMX_ASSERT(cdci_descriptor);

    if (sequence_header->source_params.source_sampling == 0 &&
        sequence_header->picture_coding_mode == 0)
    {
        cdci_descriptor->setFrameLayout(MXF_FULL_FRAME);
    }
    else if (sequence_header->source_params.source_sampling == 1 &&
             sequence_header->picture_coding_mode == 0)
    {
        cdci_descriptor->setFrameLayout(MXF_MIXED_FIELDS);
    }
    else if (sequence_header->source_params.source_sampling == 0 &&
             sequence_header->picture_coding_mode == 1)
    {
        cdci_descriptor->setFrameLayout(MXF_SEGMENTED_FRAME);
    }
    else
    {
        cdci_descriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
    }
    cdci_descriptor->setStoredWidth((uint32_t)sequence_header->source_params.frame_width);
    if (sequence_header->picture_coding_mode == 0)
        cdci_descriptor->setStoredHeight((uint32_t)sequence_header->source_params.frame_height);
    else
        cdci_descriptor->setStoredHeight((uint32_t)(sequence_header->source_params.frame_height / 2));
    cdci_descriptor->setDisplayWidth(cdci_descriptor->getStoredWidth());
    cdci_descriptor->setDisplayHeight(cdci_descriptor->getStoredHeight());
    if (sequence_header->source_params.transfer_function == 0)
        cdci_descriptor->setCaptureGamma(ITUR_BT709_TRANSFER_CH);
    else if (sequence_header->source_params.transfer_function == 1)
        cdci_descriptor->setCaptureGamma(ITU1361_TRANSFER_CH);
    else if (sequence_header->source_params.transfer_function == 2)
        cdci_descriptor->setCaptureGamma(LINEAR_TRANSFER_CH);
    else
        cdci_descriptor->setCaptureGamma(SMPTE_DCDM_TRANSFER_CH);
    if (sequence_header->source_params.source_sampling == 1)
        cdci_descriptor->setFieldDominance((sequence_header->source_params.top_field_first ? 1 : 2));
    if (sequence_header->source_params.color_matrix == 0)
        cdci_descriptor->setCodingEquations(ITUR_BT709_CODING_EQ);
    else if (sequence_header->source_params.color_matrix == 1)
        cdci_descriptor->setCodingEquations(ITUR_BT601_CODING_EQ);
    else if (sequence_header->source_params.color_matrix == 2)
        cdci_descriptor->setCodingEquations(Y_CG_CO_CODING_EQ);
    else
        cdci_descriptor->setCodingEquations(GBR_CODING_EQ);
    if (sequence_header->source_params.color_primaries == 0)
        cdci_descriptor->setColorPrimaries(ITU709_COLOR_PRIM);
    else if (sequence_header->source_params.color_primaries == 1)
        cdci_descriptor->setColorPrimaries(SMPTE170M_COLOR_PRIM);
    else if (sequence_header->source_params.color_primaries == 2)
        cdci_descriptor->setColorPrimaries(ITU470_PAL_COLOR_PRIM);
    else
        cdci_descriptor->setColorPrimaries(SMPTE_DCDM_COLOR_PRIM);
    cdci_descriptor->setComponentDepth((uint32_t)sequence_header->coding_params.luma_depth);
    if (sequence_header->source_params.color_diff_format_index == 0)
        cdci_descriptor->setHorizontalSubsampling(1);
    else
        cdci_descriptor->setHorizontalSubsampling(2);
    if (sequence_header->source_params.color_diff_format_index == 2)
        cdci_descriptor->setVerticalSubsampling(2);
    else
        cdci_descriptor->setVerticalSubsampling(1);
    cdci_descriptor->setBlackRefLevel((uint32_t)sequence_header->source_params.luma_offset);
    cdci_descriptor->setWhiteReflevel((uint32_t)(sequence_header->source_params.luma_offset +
                                                    sequence_header->source_params.luma_excursion));
    cdci_descriptor->setColorRange((uint32_t)(sequence_header->source_params.color_diff_excursion + 1));

    if (mGuessProps) {
        const VC2EssenceParser::VideoParameters &video_params = sequence_header->source_params;
        size_t i;
        for (i = 0; i < BMX_ARRAY_SIZE(DEFAULT_PARAM_MATCHES); i++) {
            if (DEFAULT_PARAM_MATCHES[i].params.frame_width == video_params.frame_width &&
                DEFAULT_PARAM_MATCHES[i].params.frame_height == video_params.frame_height &&
                DEFAULT_PARAM_MATCHES[i].params.color_diff_format_index == video_params.color_diff_format_index &&
                DEFAULT_PARAM_MATCHES[i].params.source_sampling == video_params.source_sampling &&
                DEFAULT_PARAM_MATCHES[i].params.pixel_aspect_ratio_numer == video_params.pixel_aspect_ratio_numer &&
                DEFAULT_PARAM_MATCHES[i].params.pixel_aspect_ratio_denom == video_params.pixel_aspect_ratio_denom &&
                DEFAULT_PARAM_MATCHES[i].params.color_primaries == video_params.color_primaries &&
                DEFAULT_PARAM_MATCHES[i].params.color_matrix == video_params.color_matrix &&
                DEFAULT_PARAM_MATCHES[i].params.transfer_function == video_params.transfer_function)
            {
                cdci_descriptor->setSignalStandard(DEFAULT_PARAM_MATCHES[i].signal_standard);
                cdci_descriptor->setAspectRatio(DEFAULT_PARAM_MATCHES[i].aspect_ratio);
                if (cdci_descriptor->haveVideoLineMap())
                    cdci_descriptor->removeItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap));
                cdci_descriptor->appendVideoLineMap(DEFAULT_PARAM_MATCHES[i].video_line_map[0]);
                cdci_descriptor->appendVideoLineMap(DEFAULT_PARAM_MATCHES[i].video_line_map[1]);
                SetColorSitingMod(DEFAULT_PARAM_MATCHES[i].color_siting);
                break;
            }
        }
    }
    if (!cdci_descriptor->haveAspectRatio()) {
        Rational calc_aspect_ratio;
        calc_aspect_ratio.numerator   = (int32_t)(sequence_header->source_params.pixel_aspect_ratio_numer * sequence_header->source_params.clean_width);
        calc_aspect_ratio.denominator = (int32_t)(sequence_header->source_params.pixel_aspect_ratio_denom * sequence_header->source_params.clean_height);
        cdci_descriptor->setAspectRatio(reduce_rational(calc_aspect_ratio));
    }
    if (!cdci_descriptor->haveVideoLineMap()) {
        if (sequence_header->source_params.source_sampling == 0 &&
            sequence_header->picture_coding_mode == 0)
        {
            // MXF_FULL_FRAME
            cdci_descriptor->appendVideoLineMap(1);
            cdci_descriptor->appendVideoLineMap(0);
        }
        else
        {
            cdci_descriptor->appendVideoLineMap(1);
            cdci_descriptor->appendVideoLineMap((int32_t)((sequence_header->source_params.frame_height / 2) + 1));
        }
    }
}

void VC2MXFDescriptorHelper::SetWaveletFilters(const vector<uint8_t> &wavelet_filters)
{
    BMX_ASSERT(mVC2SubDescriptor);
    mVC2SubDescriptor->setVC2WaveletFilters(wavelet_filters);
}

void VC2MXFDescriptorHelper::SetIdenticalSequence(bool identical_sequence)
{
    BMX_ASSERT(mVC2SubDescriptor);
    mVC2SubDescriptor->setVC2SequenceHeadersIdentical(identical_sequence);
}

void VC2MXFDescriptorHelper::SetCompleteSequence(bool complete_sequences)
{
    BMX_ASSERT(mVC2SubDescriptor);
    mVC2SubDescriptor->setVC2EditUnitsAreCompleteSequences(complete_sequences);
}

mxfUL VC2MXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    if (mFrameWrapped)
        return MXF_EC_L(VC2FrameWrapped);
    else
        return MXF_EC_L(VC2ClipWrapped);
}
