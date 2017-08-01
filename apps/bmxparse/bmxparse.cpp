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

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>

#include <bmx/EssenceType.h>
#include <bmx/essence_parser/AVCEssenceParser.h>
#include <bmx/essence_parser/DVEssenceParser.h>
#include <bmx/essence_parser/MJPEGEssenceParser.h>
#include <bmx/essence_parser/MPEG2EssenceParser.h>
#include <bmx/essence_parser/RDD36EssenceParser.h>
#include <bmx/essence_parser/VC3EssenceParser.h>
#include <bmx/essence_parser/VC2EssenceParser.h>
#include <bmx/apps/AppTextInfoWriter.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace bmx;


#define MAX_BUFFER_SIZE     50000000
#define BUFFER_INCREMENT    8192

static const char *APP_NAME = "bmxparse";


typedef enum
{
    AVC_INPUT,
    DV_INPUT,
    MJPEG_INPUT,
    M2V_INPUT,
    RDD36_INPUT,
    VC3_INPUT,
    VC2_INPUT,
} InputType;

static const EnumInfo RDD36_CHROMA_FORMAT_ENUM[] =
{
    {0,     "Reserved"},
    {1,     "Reserved"},
    {2,     "4:2:2"},
    {3,     "4:4:4"},
    {0, 0}
};

static const EnumInfo RDD36_INTERLACE_MODE_ENUM[] =
{
    {0,     "Progressive frame"},
    {1,     "Interlaced frame (TFF)"},
    {2,     "Interlaced frame (BFF)"},
    {3,     "Reserved"},
    {0, 0}
};

static const EnumInfo RDD36_ASPECT_RATIO_ENUM[] =
{
    {0,     "Reserved"},
    {1,     "Square pixels"},
    {2,     "4:3"},
    {3,     "16:9"},
    {0, 0}
};

static const EnumInfo ITU_COLOR_PRIMARIES_ENUM[] =
{
    {0,     "Unknown/unspecified"},
    {1,     "ITU-R BT.709"},
    {2,     "Unknown/unspecified"},
    {3,     "Reserved"},
    {4,     "ITU-R BT.470-6 M"},
    {5,     "ITU-R BT.601 625"},
    {6,     "ITU-R BT.601 525"},
    {7,     "SMPTE 240M"},
    {8,     "Film"},
    {9,     "ITU-R BT.2020"},
    {10,    "SMPTE ST 428-1"},
    {11,    "DCI P3"},
    {12,    "P3 D65"},
    {0, 0}
};

static const EnumInfo ITU_TRANSFER_CHAR_ENUM[] =
{
    {0,     "Unknown/unspecified"},
    {1,     "ITU-R BT.601/BT.709/BT.2020"},
    {2,     "Unknown/unspecified"},
    {3,     "Reserved"},
    {4,     "ITU-R BT.470-6 gamma 2.2"},
    {5,     "ITU-R BT.470-6 gamma 2.8"},
    {6,     "SMPTE 170M"},
    {7,     "SMPTE 240M"},
    {8,     "Linear"},
    {9,     "Log"},
    {10,    "Log Sqrt"},
    {11,    "IEC 61966-2-4"},
    {12,    "ITU-R BT.1361"},
    {13,    "IEC 61966-2-1"},
    {14,    "ITU-R BT.2020 10 bit"},
    {15,    "ITU-R BT.2020 12 bit"},
    {16,    "SMPTE ST 2084"},
    {17,    "SMPTE ST 428-1"},
    {18,    "HLG"},
    {0, 0}
};

static const EnumInfo ITU_MATRIX_COEFF_ENUM[] =
{
    {0,     "GBR"},
    {1,     "ITU-R BT.709"},
    {2,     "Unknown/unspecified"},
    {3,     "Reserved"},
    {4,     "FCC"},
    {5,     "ITU-R BT.601 625"},
    {6,     "ITU-R BT.601 525"},
    {7,     "SMPTE 240M"},
    {8,     "YCgCo"},
    {9,     "ITU-R BT.2020 NCL"},
    {10,    "ITU-R BT.2020 CL"},
    {11,    "SMPTE ST 2085"},
    {0, 0}
};

static const EnumInfo RDD36_ALPHA_CHANNEL_TYPE_ENUM[] =
{
    {0,     "Not present"},
    {1,     "8 bits/sample integral"},
    {2,     "16 bits/sample integral"},
    {0, 0}
};

static const EnumInfo VC2_LEVEL_ENUM[] =
{
    {0,     "Low Delay Profile"},
    {1,     "Simple Profile"},
    {2,     "Main Profile"},
    {3,     "High Quality Profile"},
    {0, 0}
};

static const EnumInfo VC2_BASE_FORMAT_ENUM[] =
{
    {0,     "Custom Format"},
    {1,     "QSIF525"},
    {2,     "QCIF"},
    {3,     "SIF525"},
    {4,     "CIF"},
    {5,     "4SIF525"},
    {6,     "4CIF"},
    {7,     "SD 480I-60"},
    {8,     "SD 576I-50"},
    {9,     "HD 720P-60"},
    {10,    "HD 720P-50"},
    {11,    "HD 1080I-60"},
    {12,    "HD 1080I-50"},
    {13,    "HD 1080P-60"},
    {14,    "HD 1080P-50"},
    {15,    "DC 2K-24"},
    {16,    "DC 4K-24"},
    {17,    "UHDTV 4K-60"},
    {18,    "UHDTV 4K-50"},
    {19,    "UHDTV 8K-60"},
    {20,    "UHDTV 8K-50"},
    {21,    "HD 1080P-24"},
    {22,    "SD Pro486"},
    {0, 0}
};

static const EnumInfo VC2_COLOR_DIFF_ENUM[] =
{
    {0,     "4:4:4"},
    {1,     "4:2:2"},
    {2,     "4:2:0"},
    {0, 0}
};

static const EnumInfo VC2_SOURCE_SAMPLING_ENUM[] =
{
    {0,     "Progressive"},
    {1,     "Interlaced"},
    {0, 0}
};

static const EnumInfo VC2_PICTURE_CODING_MODE_ENUM[] =
{
    {0,     "Frames"},
    {1,     "Fields"},
    {0, 0}
};

static const EnumInfo VC2_COLOR_PRIMARIES_ENUM[] =
{
    {0,     "HDTV"},
    {1,     "SDTV 525"},
    {2,     "SDTV 625"},
    {3,     "D-Cinema"},
    {0, 0}
};

static const EnumInfo VC2_COLOR_MATRIX_ENUM[] =
{
    {0,     "HDTV"},
    {1,     "SDTV"},
    {2,     "Reversible"},
    {3,     "RGB"},
    {0, 0}
};

static const EnumInfo VC2_TRANSFER_FUNCTION_ENUM[] =
{
    {0,     "TV Gamma"},
    {1,     "Extended Gamut"},
    {2,     "Linear"},
    {3,     "D-Cinema Transfer Function"},
    {0, 0}
};

static const EnumInfo VC2_WAVELET_FILTER_ENUM[] =
{
    {0,     "Deslauriers-Dubuc (9,7)"},
    {1,     "LeGall (5,3)"},
    {2,     "Deslauriers-Dubuc (13,7)"},
    {3,     "Haar with no shift"},
    {4,     "Haar with single shift per level"},
    {5,     "Fidelity filter"},
    {6,     "Daubechies (9,7) integer approximation"},
    {0, 0}
};


class Buffer
{
public:
    Buffer()
    {
        data = 0;
        alloc_size = 0;
        size = 0;
    }

    ~Buffer()
    {
        delete [] data;
    }

    size_t Fill(FILE *file)
    {
        if (size + BUFFER_INCREMENT > alloc_size) {
            if (alloc_size + BUFFER_INCREMENT > MAX_BUFFER_SIZE) {
                log_error("Maximum buffer size %u exceeded\n", MAX_BUFFER_SIZE);
                throw false;
            }

            unsigned char *new_data = new unsigned char[alloc_size + BUFFER_INCREMENT];
            memcpy(new_data, data, size);
            delete [] data;
            data = new_data;
            alloc_size += BUFFER_INCREMENT;
        }

        size_t num_read = fread(data + size, 1, BUFFER_INCREMENT, file);
        if (num_read == 0 && ferror(file)) {
            log_error("File read failed: %s\n", bmx_strerror(errno).c_str());
            throw false;
        }
        size += num_read;

        return num_read;
    }

    void Shift(uint32_t start)
    {
        memmove(data, data + start, size - start);
        size -= start;
    }

public:
    unsigned char *data;
    size_t alloc_size;
    size_t size;
};


typedef void (*print_frame_info_f)(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                   Buffer *buffer, uint32_t frame_size, int64_t frame_num);


static void print_avc_frame_info(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                 Buffer *buffer, uint32_t frame_size, int64_t frame_num)
{
    POCState *poc_state = static_cast<POCState*>(parser_data);
    AVCEssenceParser *avc_parser = dynamic_cast<AVCEssenceParser*>(parser);
    avc_parser->ParseFrameInfo(buffer->data, frame_size);

    int32_t pic_order_cnt;
    avc_parser->DecodePOC(poc_state, &pic_order_cnt);

    EssenceType essence_type = avc_parser->GetEssenceType();
    EssenceType avci_essence_type = avc_parser->GetAVCIEssenceType(frame_size, false, false);

    info_writer->StartArrayElement("frame", (size_t)frame_num);

    info_writer->WriteIntegerItem("size", frame_size);
    info_writer->WriteIntegerItem("profile", avc_parser->GetProfile(), true);
    info_writer->WriteIntegerItem("profile_constraint", avc_parser->GetProfileConstraint(), true);
    info_writer->WriteIntegerItem("level", avc_parser->GetLevel());
    info_writer->WriteIntegerItem("max_bit_rate", avc_parser->GetMaxBitRate());
    info_writer->WriteIntegerItem("max_num_ref_frames", avc_parser->GetMaxNumRefFrames());
    info_writer->WriteBoolItem("fixed_frame_rate", avc_parser->IsFixedFrameRate());
    info_writer->WriteRationalItem("frame_rate", avc_parser->GetFrameRate());
    info_writer->WriteIntegerItem("stored_width", avc_parser->GetStoredWidth());
    info_writer->WriteIntegerItem("stored_height", avc_parser->GetStoredHeight());
    info_writer->WriteIntegerItem("display_width", avc_parser->GetDisplayWidth());
    info_writer->WriteIntegerItem("display_height", avc_parser->GetDisplayHeight());
    info_writer->WriteIntegerItem("display_x_offset", avc_parser->GetDisplayYOffset());
    info_writer->WriteIntegerItem("display_y_offset", avc_parser->GetDisplayYOffset());
    info_writer->WriteRationalItem("sample_aspect_ratio", avc_parser->GetSampleAspectRatio());
    if (avc_parser->IsIDRFrame()) {
        info_writer->WriteStringItem("frame_type", "I_IDR");
    } else {
        switch (avc_parser->GetFrameType())
        {
            case I_FRAME: info_writer->WriteStringItem("frame_type", "I_Non_IDR"); break;
            case P_FRAME: info_writer->WriteStringItem("frame_type", "P"); break;
            case B_FRAME: info_writer->WriteStringItem("frame_type", "B"); break;
            default:      info_writer->WriteStringItem("frame_type", "Unknown"); break;
        }
    }
    info_writer->WriteIntegerItem("frame_num", avc_parser->GetFrameNum());
    info_writer->WriteIntegerItem("pic_order_cnt", pic_order_cnt);
    info_writer->WriteBoolItem("frame_mbs_only", avc_parser->IsFrameMBSOnly());
    if (!avc_parser->IsFrameMBSOnly()) {
        info_writer->WriteBoolItem("mb_adaptive_ff_encoding", avc_parser->IsMBAdaptiveFFEncoding());
        info_writer->WriteBoolItem("field_picture", avc_parser->IsFieldPicture());
        if (avc_parser->IsFieldPicture())
            info_writer->WriteBoolItem("bottom_field", avc_parser->IsBottomField());
    }
    info_writer->WriteStringItem("essence_type", essence_type_to_enum_string(essence_type));
    if (avci_essence_type != UNKNOWN_ESSENCE_TYPE)
        info_writer->WriteStringItem("avci_essence_type", essence_type_to_enum_string(avci_essence_type));

    info_writer->EndArrayElement();
}

static void print_dv_frame_info(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                Buffer *buffer, uint32_t frame_size, int64_t frame_num)
{
    (void)parser_data;
    DVEssenceParser *dv_parser = dynamic_cast<DVEssenceParser*>(parser);
    dv_parser->ParseFrameInfo(buffer->data, frame_size);

    Rational frame_rate;
    if (dv_parser->Is50Hz())
        frame_rate = FRAME_RATE_25;
    else
        frame_rate = FRAME_RATE_2997;

    info_writer->StartArrayElement("frame", (size_t)frame_num);

    info_writer->WriteIntegerItem("size", frame_size);
    switch (dv_parser->GetEssenceType())
    {
        case DVEssenceParser::IEC_DV25:     info_writer->WriteStringItem("type", "IEC_DV25"); break;
        case DVEssenceParser::DVBASED_DV25: info_writer->WriteStringItem("type", "DVBASED_DV25"); break;
        case DVEssenceParser::DV50:         info_writer->WriteStringItem("type", "DV50"); break;
        case DVEssenceParser::DV100_1080I:  info_writer->WriteStringItem("type", "DV100_1080I"); break;
        case DVEssenceParser::DV100_720P:   info_writer->WriteStringItem("type", "DV100_720P"); break;
        default:                            info_writer->WriteStringItem("type", "Unknown"); break;
    }
    info_writer->WriteRationalItem("frame_rate", frame_rate);
    info_writer->WriteRationalItem("aspect_ratio", dv_parser->GetAspectRatio());

    info_writer->EndArrayElement();
}

static void print_mjpeg_frame_info(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                   Buffer *buffer, uint32_t frame_size, int64_t frame_num)
{
    (void)parser_data;
    MJPEGEssenceParser *mjpeg_parser = dynamic_cast<MJPEGEssenceParser*>(parser);
    mjpeg_parser->ParseFrameInfo(buffer->data, frame_size);

    info_writer->StartArrayElement("frame", (size_t)frame_num);

    info_writer->WriteIntegerItem("size", frame_size);

    info_writer->EndArrayElement();
}

static void print_m2v_frame_info(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                 Buffer *buffer, uint32_t frame_size, int64_t frame_num)
{
    (void)parser_data;
    MPEG2EssenceParser *m2v_parser = dynamic_cast<MPEG2EssenceParser*>(parser);
    m2v_parser->ParseFrameAllInfo(buffer->data, frame_size);

    info_writer->StartArrayElement("frame", (size_t)frame_num);

    info_writer->WriteIntegerItem("size", frame_size);
    info_writer->WriteBoolItem("sequence_header", m2v_parser->HaveSequenceHeader());
    if (m2v_parser->HaveSequenceHeader()) {
        info_writer->WriteIntegerItem("horizontal_size", m2v_parser->GetHorizontalSize());
        info_writer->WriteIntegerItem("vertical_size", m2v_parser->GetVerticalSize());
        if (m2v_parser->HaveKnownAspectRatio())
            info_writer->WriteRationalItem("aspect_ratio", m2v_parser->GetAspectRatio());
        if (m2v_parser->HaveKnownFrameRate())
            info_writer->WriteRationalItem("frame_rate", m2v_parser->GetFrameRate());
        info_writer->WriteIntegerItem("bit_rate", m2v_parser->GetBitRate());
        info_writer->WriteBoolItem("low_delay", m2v_parser->IsLowDelay());
        info_writer->WriteIntegerItem("profile_and_level", m2v_parser->GetProfileAndLevel());
        info_writer->WriteBoolItem("progressive", m2v_parser->IsProgressive());
        info_writer->WriteIntegerItem("chroma_format", m2v_parser->GetChromaFormat());
    }
    if (m2v_parser->HaveDisplayExtension()) {
        info_writer->WriteIntegerItem("video_format", m2v_parser->GetVideoFormat());
        info_writer->WriteIntegerItem("display_horiz_size", m2v_parser->GetDHorizontalSize());
        info_writer->WriteIntegerItem("display_vert_size", m2v_parser->GetDVerticalSize());
        if (m2v_parser->HaveColorDescription()) {
            info_writer->WriteIntegerItem("color_primaries", m2v_parser->GetColorPrimaries());
            info_writer->WriteIntegerItem("transfer_chars", m2v_parser->GetTransferCharacteristics());
            info_writer->WriteIntegerItem("matrix_coeffs", m2v_parser->GetMatrixCoeffs());
        }
    }
    info_writer->WriteBoolItem("gop_header", m2v_parser->HaveGOPHeader());
    if (m2v_parser->HaveGOPHeader())
        info_writer->WriteBoolItem("closed_gop", m2v_parser->IsClosedGOP());
    switch (m2v_parser->GetFrameType())
    {
        case I_FRAME: info_writer->WriteStringItem("frame_type", "I"); break;
        case P_FRAME: info_writer->WriteStringItem("frame_type", "P"); break;
        case B_FRAME: info_writer->WriteStringItem("frame_type", "B"); break;
        default:      info_writer->WriteStringItem("frame_type", "Unknown"); break;
    }
    info_writer->WriteIntegerItem("temporal_ref", m2v_parser->GetTemporalReference());
    info_writer->WriteIntegerItem("vbv_delay", m2v_parser->GetVBVDelay());
    if (m2v_parser->HavePicCodingExtension())
        info_writer->WriteBoolItem("top_field_first", m2v_parser->IsTFF());

    info_writer->EndArrayElement();
}

static void print_rdd36_frame_info(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                   Buffer *buffer, uint32_t frame_size, int64_t frame_num)
{
    (void)parser_data;
    RDD36EssenceParser *rdd36_parser = dynamic_cast<RDD36EssenceParser*>(parser);
    rdd36_parser->ParseFrameInfo(buffer->data, frame_size);

    info_writer->StartArrayElement("frame", (size_t)frame_num);

    info_writer->WriteIntegerItem("size", frame_size);

    info_writer->WriteRationalItem("frame_rate", rdd36_parser->GetFrameRate());
    info_writer->WriteIntegerItem("frame_width", rdd36_parser->GetWidth());
    info_writer->WriteIntegerItem("frame_height", rdd36_parser->GetHeight());
    info_writer->WriteEnumItem("chroma_format", RDD36_CHROMA_FORMAT_ENUM, rdd36_parser->GetChromaFormat());
    info_writer->WriteEnumItem("interlace_mode", RDD36_INTERLACE_MODE_ENUM, rdd36_parser->GetInterlaceMode());
    info_writer->WriteEnumItem("aspect_ratio", RDD36_ASPECT_RATIO_ENUM, rdd36_parser->GetAspectRatioCode());
    info_writer->WriteEnumItem("color_primaries", ITU_COLOR_PRIMARIES_ENUM, rdd36_parser->GetColorPrimaries(), "Reserved");
    info_writer->WriteEnumItem("transfer_characteristic", ITU_TRANSFER_CHAR_ENUM, rdd36_parser->GetTransferCharacteristic(), "Reserved");
    info_writer->WriteEnumItem("matrix_coefficients", ITU_MATRIX_COEFF_ENUM, rdd36_parser->GetMatrixCoefficients(), "Reserved");
    info_writer->WriteEnumItem("alpha_channel_type", RDD36_ALPHA_CHANNEL_TYPE_ENUM, rdd36_parser->GetAlphaChannelType(), "Reserved");

    info_writer->EndArrayElement();
}

static void print_vc2_frame_info(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                 Buffer *buffer, uint32_t frame_size, int64_t frame_num)
{
    (void)parser_data;
    VC2EssenceParser *vc2_parser = dynamic_cast<VC2EssenceParser*>(parser);
    vc2_parser->ParseFrameInfo(buffer->data, frame_size);

    info_writer->StartArrayElement("frame", (size_t)frame_num);

    info_writer->WriteIntegerItem("size", frame_size);
    if (vc2_parser->FrameHasSequenceHeader()) {
        const VC2EssenceParser::SequenceHeader *sequence_header = vc2_parser->GetSequenceHeader();
        info_writer->StartSection("sequence_header");

        info_writer->WriteIntegerItem("major_version", sequence_header->major_version);
        info_writer->WriteIntegerItem("minor_version", sequence_header->minor_version);
        info_writer->WriteIntegerItem("profile", sequence_header->profile);
        info_writer->WriteEnumItem("level", VC2_LEVEL_ENUM, sequence_header->level);
        info_writer->WriteEnumItem("base_video_format", VC2_BASE_FORMAT_ENUM, sequence_header->base_video_format);
        info_writer->WriteEnumItem("picture_coding_mode", VC2_PICTURE_CODING_MODE_ENUM, sequence_header->picture_coding_mode);

        const VC2EssenceParser::VideoParameters *source_params = &sequence_header->source_params;
        info_writer->StartSection("source_parameters");
        info_writer->WriteIntegerItem("frame_width", source_params->frame_width);
        info_writer->WriteIntegerItem("frame_height", source_params->frame_height);
        info_writer->WriteEnumItem("color_diff_format", VC2_COLOR_DIFF_ENUM, source_params->color_diff_format_index);
        info_writer->WriteEnumItem("source_sampling", VC2_SOURCE_SAMPLING_ENUM, source_params->source_sampling);
        info_writer->WriteBoolItem("top_field_first", source_params->top_field_first);
        info_writer->WriteRationalItem("frame_rate", vc2_parser->GetFrameRate());
        Rational pixel_aspect_ratio;
        pixel_aspect_ratio.numerator   = (int32_t)source_params->pixel_aspect_ratio_numer;
        pixel_aspect_ratio.denominator = (int32_t)source_params->pixel_aspect_ratio_denom;
        info_writer->WriteRationalItem("pixel_aspect_ratio", pixel_aspect_ratio);
        info_writer->WriteIntegerItem("clean_width", source_params->clean_width);
        info_writer->WriteIntegerItem("clean_height", source_params->clean_height);
        info_writer->WriteIntegerItem("left_offset", source_params->left_offset);
        info_writer->WriteIntegerItem("top_offset", source_params->top_offset);
        info_writer->WriteIntegerItem("luma_offset", source_params->luma_offset);
        info_writer->WriteIntegerItem("luma_excursion", source_params->luma_excursion);
        info_writer->WriteIntegerItem("color_diff_offset", source_params->color_diff_offset);
        info_writer->WriteIntegerItem("color_diff_excursion", source_params->color_diff_excursion);
        info_writer->WriteEnumItem("color_primaries", VC2_COLOR_PRIMARIES_ENUM, source_params->color_primaries);
        info_writer->WriteEnumItem("color_matrix", VC2_COLOR_MATRIX_ENUM, source_params->color_matrix);
        info_writer->WriteEnumItem("transfer_function", VC2_TRANSFER_FUNCTION_ENUM, source_params->transfer_function);
        info_writer->EndSection();

        const VC2EssenceParser::CodingParameters *coding_params = &sequence_header->coding_params;
        info_writer->StartSection("coding_parameters");
        info_writer->WriteIntegerItem("luma_width", coding_params->luma_width);
        info_writer->WriteIntegerItem("luma_height", coding_params->luma_height);
        info_writer->WriteIntegerItem("color_diff_width", coding_params->color_diff_width);
        info_writer->WriteIntegerItem("color_diff_height", coding_params->color_diff_height);
        info_writer->WriteIntegerItem("luma_depth", coding_params->luma_depth);
        info_writer->WriteIntegerItem("color_diff_depth", coding_params->color_diff_depth);
        info_writer->EndSection();

        info_writer->EndSection();
    }
    int i;
    for (i = 0; i < vc2_parser->GetPictureCount(); i++) {
        const VC2EssenceParser::PictureHeader *picture_header;
        if (i == 0)
            picture_header = vc2_parser->GetPictureHeader1();
        else
            picture_header = vc2_parser->GetPictureHeader2();
        info_writer->StartArrayElement("picture", i);
        info_writer->WriteIntegerItem("picture_number", picture_header->picture_number);
        info_writer->WriteEnumItem("wavelet", VC2_WAVELET_FILTER_ENUM, picture_header->wavelet_index);
        info_writer->EndArrayElement();
    }

    info_writer->EndArrayElement();
}

static void print_vc3_frame_info(AppInfoWriter *info_writer, EssenceParser *parser, void *parser_data,
                                 Buffer *buffer, uint32_t frame_size, int64_t frame_num)
{
    (void)parser_data;
    VC3EssenceParser *vc3_parser = dynamic_cast<VC3EssenceParser*>(parser);
    vc3_parser->ParseFrameInfo(buffer->data, frame_size);

    info_writer->StartArrayElement("frame", (size_t)frame_num);

    info_writer->WriteIntegerItem("size", frame_size);
    info_writer->WriteIntegerItem("compression_id", vc3_parser->GetCompressionId());
    info_writer->WriteBoolItem("progressive", vc3_parser->IsProgressive());
    info_writer->WriteIntegerItem("frame_width", vc3_parser->GetFrameWidth());
    info_writer->WriteIntegerItem("frame_height", vc3_parser->GetFrameHeight());
    info_writer->WriteIntegerItem("bit_depth", vc3_parser->GetBitDepth());

    info_writer->EndArrayElement();
}


static bool parse_type_str(const char *type_str, InputType *input_type)
{
    if (strcmp(type_str, "avc") == 0)
        *input_type = AVC_INPUT;
    else if (strcmp(type_str, "dv") == 0)
        *input_type = DV_INPUT;
    else if (strcmp(type_str, "mjpeg") == 0)
        *input_type = MJPEG_INPUT;
    else if (strcmp(type_str, "m2v") == 0)
        *input_type = M2V_INPUT;
    else if (strcmp(type_str, "vc2") == 0)
        *input_type = VC2_INPUT;
    else if (strcmp(type_str, "rdd36") == 0)
        *input_type = RDD36_INPUT;
    else if (strcmp(type_str, "vc3") == 0)
        *input_type = VC3_INPUT;
    else
        return false;

    return true;
}

static void usage(const char *cmd)
{
    fprintf(stderr, "%s\n", get_app_version_info(APP_NAME).c_str());
    fprintf(stderr, "Usage: %s <<options>> <type> <filename>\n", cmd);
    fprintf(stderr, "    <type> is 'avc', 'dv', 'mjpeg', 'm2v', 'rdd36', 'vc2' or 'vc3'\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -h | --help           Show usage and exit\n");
    fprintf(stderr, " -v | --version        Print version info to stderr\n");
    fprintf(stderr, " -l <file>             Log filename. Default log to stderr\n");
    fprintf(stderr, " --log-level <level>   Set the log level. 0=debug, 1=info, 2=warning, 3=error. Default is 1\n");
    fprintf(stderr, " --single-field        Assume MJPEG single field encoding. Default is to parse field pairs\n");
}

int main(int argc, const char **argv)
{
    const char *log_filename = 0;
    LogLevel log_level = INFO_LOG;
    bool single_field = false;
    InputType input_type = AVC_INPUT;
    const char *filename = 0;
    int cmdln_index;

    if (argc == 1) {
        usage(argv[0]);
        return 0;
    }

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++)
    {
        if (strcmp(argv[cmdln_index], "--help") == 0 ||
            strcmp(argv[cmdln_index], "-h") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdln_index], "--version") == 0 ||
                 strcmp(argv[cmdln_index], "-v") == 0)
        {
            if (argc == 2) {
                printf("%s\n", get_app_version_info(APP_NAME).c_str());
                return 0;
            }
            fprintf(stderr, "%s\n", get_app_version_info(APP_NAME).c_str());
        }
        else if (strcmp(argv[cmdln_index], "-l") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            log_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--log-level") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_log_level(argv[cmdln_index + 1], &log_level))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--single-field") == 0)
        {
            single_field = true;
        }
        else
        {
            break;
        }
    }
    if (cmdln_index + 2 < argc) {
        usage(argv[0]);
        fprintf(stderr, "Unknown option '%s'\n", argv[cmdln_index]);
        return 1;
    }
    if (cmdln_index + 1 > argc) {
        usage(argv[0]);
        fprintf(stderr, "Missing <type> and <filename>\n");
        return 1;
    }
    if (cmdln_index + 2 > argc) {
        usage(argv[0]);
        fprintf(stderr, "Missing <filename>\n");
        return 1;
    }
    if (!parse_type_str(argv[cmdln_index], &input_type)) {
        usage(argv[0]);
        fprintf(stderr, "Invalid value '%s' for <type>\n", argv[cmdln_index]);
        return 1;
    }
    filename = argv[cmdln_index + 1];


    LOG_LEVEL = log_level;
    if (log_filename && !open_log_file(log_filename))
        return 1;


    int cmd_result = 0;
    try
    {
        FILE *file = fopen(filename, "rb");
        if (!file) {
            log_error("Failed to open '%s': %s\n", filename, bmx_strerror(errno).c_str());
            throw false;
        }

        AppTextInfoWriter *text_info_writer = new AppTextInfoWriter(stdout);
        AppInfoWriter *info_writer = text_info_writer;

        POCState poc_state;
        EssenceParser *parser;
        void *parser_data = 0;
        print_frame_info_f print_frame_info;
        switch (input_type)
        {
            case AVC_INPUT:
                parser = new AVCEssenceParser();
                parser_data = &poc_state;
                print_frame_info = print_avc_frame_info;
                if (text_info_writer)
                    text_info_writer->PushItemValueIndent(strlen("mb_adaptive_ff_encoding "));
                break;
            case DV_INPUT:
                parser = new DVEssenceParser();
                print_frame_info = print_dv_frame_info;
                if (text_info_writer)
                    text_info_writer->PushItemValueIndent(strlen("aspect_ratio "));
                break;
            case MJPEG_INPUT:
                parser = new MJPEGEssenceParser(single_field);
                print_frame_info = print_mjpeg_frame_info;
                if (text_info_writer)
                    text_info_writer->PushItemValueIndent(strlen("size "));
                break;
            case M2V_INPUT:
                parser = new MPEG2EssenceParser();
                print_frame_info = print_m2v_frame_info;
                if (text_info_writer)
                    text_info_writer->PushItemValueIndent(strlen("display_horiz_size "));
                break;
            case RDD36_INPUT:
                parser = new RDD36EssenceParser();
                print_frame_info = print_rdd36_frame_info;
                if (text_info_writer)
                    text_info_writer->PushItemValueIndent(strlen("transfer_characteristic "));
                break;
            case VC2_INPUT:
                parser = new VC2EssenceParser();
                print_frame_info = print_vc2_frame_info;
                if (text_info_writer)
                    text_info_writer->PushItemValueIndent(strlen("color_diff_format_index "));
                break;
            case VC3_INPUT:
                parser = new VC3EssenceParser();
                print_frame_info = print_vc3_frame_info;
                if (text_info_writer)
                    text_info_writer->PushItemValueIndent(strlen("compression_id "));
                break;
        }


        Buffer buffer;
        buffer.Fill(file);
        uint32_t frame_start = parser->ParseFrameStart(buffer.data, (uint32_t)buffer.size);
        if (frame_start == ESSENCE_PARSER_NULL_OFFSET) {
            log_error("Failed to find valid frame start within %" PRIszt " bytes\n", buffer.size);
            throw false;
        }

        info_writer->Start("bmxparse");

        int64_t frame_count = 0;
        uint32_t frame_size;
        while (true) {
            if (frame_start > 0) {
                buffer.Shift(frame_start);
                frame_start = 0;
            }

            frame_size = parser->ParseFrameSize(buffer.data, (uint32_t)buffer.size);
            if (frame_size == ESSENCE_PARSER_NULL_OFFSET) {
                if (buffer.Fill(file) == 0)
                    break;
            } else if (frame_size == ESSENCE_PARSER_NULL_FRAME_SIZE) {
                log_error("Invalid frame start\n");
                throw false;
            } else {
                print_frame_info(info_writer, parser, parser_data, &buffer, frame_size, frame_count);
                frame_start = frame_size;
                frame_count++;
            }
        }
        if (buffer.size > 0) {
            print_frame_info(info_writer, parser, parser_data, &buffer, (uint32_t)buffer.size, frame_count);
            frame_count++;
        }

        info_writer->End();

        delete info_writer;
        delete parser;
        fclose(file);
    }
    catch (const BMXException &ex)
    {
        log_error("BMX exception caught: %s\n", ex.what());
        cmd_result = 1;
    }
    catch (const bool &ex)
    {
        if (ex)
            cmd_result = 0;
        else
            cmd_result = 1;
    }
    catch (...)
    {
        log_error("Unknown exception caught\n");
        cmd_result = 1;
    }

    if (log_filename)
        close_log_file();

    return cmd_result;
}
