/*
 * Parse raw essence data
 *
 * Copyright (C) 2009, British Broadcasting Corporation
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

#include <libMXF++/MXF.h>

#include <mxf/mxf_labels_and_keys.h>
#include <mxf/mxf_avid_labels_and_keys.h>
#include <mxf/mxf_macros.h>

#include "RawEssenceParser.h"
#include "FixedSizeEssenceParser.h"
#include "VariableSizeEssenceParser.h"
#include "PCMEssenceParser.h"

using namespace std;
using namespace mxfpp;



typedef enum
{
    FIXED_FRAME_SIZE_PARSER,
    PCM_PARSER,
    VARIABLE_FRAME_SIZE_PARSER
} ParserType;

typedef struct
{
    const mxfUL essence_label;
    ParserType parser_type;
    uint32_t fixed_frame_size;
} SupportedFormat;

const SupportedFormat SUPPORTED_FORMATS[] =
{
    {MXF_EC_L(IECDV_25_525_60_ClipWrapped),         FIXED_FRAME_SIZE_PARSER,    120000},
    {MXF_EC_L(IECDV_25_625_50_ClipWrapped),         FIXED_FRAME_SIZE_PARSER,    144000},
    {MXF_EC_L(DVBased_25_525_60_ClipWrapped),       FIXED_FRAME_SIZE_PARSER,    120000},
    {MXF_EC_L(DVBased_25_625_50_ClipWrapped),       FIXED_FRAME_SIZE_PARSER,    144000},
    {MXF_EC_L(DVBased_50_525_60_ClipWrapped),       FIXED_FRAME_SIZE_PARSER,    240000},
    {MXF_EC_L(DVBased_50_625_50_ClipWrapped),       FIXED_FRAME_SIZE_PARSER,    288000},
    {MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped),   FIXED_FRAME_SIZE_PARSER,    576000},
    {MXF_EC_L(DVBased_100_720_50_P_ClipWrapped),    FIXED_FRAME_SIZE_PARSER,    288000},
    {MXF_EC_L(DNxHD1080p1235ClipWrapped),           FIXED_FRAME_SIZE_PARSER,    917504},
    {MXF_EC_L(DNxHD1080p1237ClipWrapped),           FIXED_FRAME_SIZE_PARSER,    606208},
    {MXF_EC_L(DNxHD1080p1238ClipWrapped),           FIXED_FRAME_SIZE_PARSER,    917504},
    {MXF_EC_L(DNxHD1080i1241ClipWrapped),           FIXED_FRAME_SIZE_PARSER,    917504},
    {MXF_EC_L(DNxHD1080i1242ClipWrapped),           FIXED_FRAME_SIZE_PARSER,    606208},
    {MXF_EC_L(DNxHD1080i1243ClipWrapped),           FIXED_FRAME_SIZE_PARSER,    917504},
    {MXF_EC_L(DNxHD720p1250ClipWrapped),            FIXED_FRAME_SIZE_PARSER,    458752},
    {MXF_EC_L(DNxHD720p1251ClipWrapped),            FIXED_FRAME_SIZE_PARSER,    458752},
    {MXF_EC_L(DNxHD720p1252ClipWrapped),            FIXED_FRAME_SIZE_PARSER,    303104},
    {MXF_EC_L(DNxHD1080p1253ClipWrapped),           FIXED_FRAME_SIZE_PARSER,    188416},
    {MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped),  FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped),     FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(HD_Unc_720_50p_422_ClipWrapped),      FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(AvidIMX30_625_50),                    FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(AvidIMX40_625_50),                    FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(AvidIMX50_625_50),                    FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(AvidIMX30_525_60),                    FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(AvidIMX40_525_60),                    FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(AvidIMX50_525_60),                    FIXED_FRAME_SIZE_PARSER,         0},
    {MXF_EC_L(AvidMJPEGClipWrapped),                VARIABLE_FRAME_SIZE_PARSER,      0},
    {MXF_EC_L(BWFClipWrapped),                      PCM_PARSER,                      0},
    {MXF_EC_L(AES3ClipWrapped),                     PCM_PARSER,                      0},
};




RawEssenceParser* RawEssenceParser::Create(File *file, int64_t essence_length, mxfUL essence_label,
                                           const FileDescriptor *file_descriptor, mxfRational edit_rate,
                                           uint32_t frame_size, FrameOffsetIndexTableSegment *index_table)
{
    uint32_t fixed_frame_size;
    size_t i;
    for (i = 0; i < ARRAY_SIZE(SUPPORTED_FORMATS); i++) {
        if (mxf_equals_ul(&essence_label, &SUPPORTED_FORMATS[i].essence_label)) {
            switch (SUPPORTED_FORMATS[i].parser_type)
            {
                case FIXED_FRAME_SIZE_PARSER:
                {
                    fixed_frame_size = SUPPORTED_FORMATS[i].fixed_frame_size;
                    if (fixed_frame_size == 0)
                        fixed_frame_size = frame_size;
                    return new FixedSizeEssenceParser(file, essence_length, essence_label, file_descriptor,
                                                      fixed_frame_size);
                }
                case PCM_PARSER:
                {
                    // TODO: edit rate passed in should be the video edit rate
                    mxfRational edit_rate = {25, 1};
                    return new PCMEssenceParser(file, essence_length, essence_label, edit_rate, file_descriptor);
                }
                case VARIABLE_FRAME_SIZE_PARSER:
                {
                    return new VariableSizeEssenceParser(file, essence_length, essence_label, edit_rate,
                                                         file_descriptor, index_table);
                }
            }
        }
    }

    return 0;
}


RawEssenceParser::RawEssenceParser(File *file, int64_t essence_length, mxfUL essence_label)
{
    mFile = file;
    mEssenceLength = essence_length;
    mEssenceLabel = essence_label;
    mPosition = 0;
    mDuration = -1;
    mEssenceOffset = 0;

    mEssenceStartOffset = mFile->tell();
}

RawEssenceParser::~RawEssenceParser()
{
    delete mFile;
}

bool RawEssenceParser::IsEOF()
{
    return mDuration >= 0 && mPosition >= mDuration;
}

