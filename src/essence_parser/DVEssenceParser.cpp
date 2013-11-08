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

#include <bmx/essence_parser/DVEssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define DV_DIF_BLOCK_SIZE           80
#define DV_DIF_SEQUENCE_SIZE        (150 * DV_DIF_BLOCK_SIZE)

#define HEADER_SECTION_OFFSET       0
#define SUBCODE_SECTION_OFFSET      DV_DIF_BLOCK_SIZE
#define VAUX_SECTION_OFFSET         (3 * DV_DIF_BLOCK_SIZE)
#define VAUX_SECTION_VS0_OFFSET     (VAUX_SECTION_OFFSET + 2 * DV_DIF_BLOCK_SIZE + 3 + 9 * 5)
#define VAUX_SECTION_VSC0_OFFSET    (VAUX_SECTION_OFFSET + 2 * DV_DIF_BLOCK_SIZE + 3 + 10 * 5)
#define AUDIO_SECTION_OFFSET        (6 * DV_DIF_BLOCK_SIZE)
#define VIDEO_SECTION_OFFSET        (15 * DV_DIF_BLOCK_SIZE)



DVEssenceParser::DVEssenceParser()
{
    mEssenceType = UNKNOWN_DV;
    mIs50Hz = false;
    mFrameSize = 0;
    mAspectRatio = ZERO_RATIONAL;
}

DVEssenceParser::~DVEssenceParser()
{
}

uint32_t DVEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    (void)data;
    return 0;
}

uint32_t DVEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_ASSERT(DV_PARSER_MIN_DATA_SIZE >= DV_DIF_SEQUENCE_SIZE);
    if (data_size < DV_PARSER_MIN_DATA_SIZE)
        return ESSENCE_PARSER_NULL_OFFSET; // insufficient data

    // check section ids
    if ((data[HEADER_SECTION_OFFSET]  & 0xe0) != 0x00 ||
        (data[SUBCODE_SECTION_OFFSET] & 0xe0) != 0x20 ||
        (data[VAUX_SECTION_OFFSET]    & 0xe0) != 0x40 ||
        (data[AUDIO_SECTION_OFFSET]   & 0xe0) != 0x60 ||
        (data[VIDEO_SECTION_OFFSET]   & 0xe0) != 0x80)
    {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    // check video and vaux section are transmitted
    if ((data[HEADER_SECTION_OFFSET + 6] & 0x80)) {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    // check starts with first channel
    if ((data[HEADER_SECTION_OFFSET + 1] & 0x0c) != 0x04) {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }

    uint32_t frame_size = ParseFrameSizeInt(data, data_size);
    if (data_size < frame_size)
        return ESSENCE_PARSER_NULL_OFFSET;

    return frame_size;
}

void DVEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size >= DV_DIF_SEQUENCE_SIZE);

    // 50Hz/60Hz is extracted from the VAUX section, VS pack
    unsigned char byte = data[VAUX_SECTION_VS0_OFFSET + 3];
    mIs50Hz = (byte & 0x20);

    mFrameSize = ParseFrameSizeInt(data, data_size);

    // IEC/DV-based is extracted from the header sections, APT
    bool is_dvbased = (data[HEADER_SECTION_OFFSET + 4] & 0x03);

    // essence type is extracted from the VAUX section, VS pack, STYPE bits
    byte = data[VAUX_SECTION_VS0_OFFSET + 3];
    byte &= 0x1f;
    if (byte == 0x00)
        mEssenceType = (is_dvbased ? DVBASED_DV25 : IEC_DV25);
    else if (byte == 0x04)
        mEssenceType = DV50;
    else if (byte == 0x14)
        mEssenceType = DV100_1080I;
    else if (byte == 0x18)
        mEssenceType = DV100_720P;
    else
        // unknown type - shouldn't be here if ParseFrameSize called
        BMX_CHECK(false);

    // aspect ratio is extracted from the VAUX section, VSC pack, DISP bits
    byte = data[VAUX_SECTION_VSC0_OFFSET + 2];
    byte &= 0x07;
    if (byte == 0x02)
        mAspectRatio = ASPECT_RATIO_16_9;
    else if (byte == 0x00)
        mAspectRatio = ASPECT_RATIO_4_3;
    else // unknown
        mAspectRatio = ZERO_RATIONAL;

    // TODO: repeated field/frames and order for 1080 and 720 (FF/FS flags in VAUX section)
}

uint32_t DVEssenceParser::ParseFrameSizeInt(const unsigned char *data, uint32_t data_size)
{
    BMX_ASSERT(data_size >= DV_DIF_SEQUENCE_SIZE);

    // 50Hz/60Hz is extracted from the VAUX section, VS pack, STYPE bits
    unsigned char byte = data[VAUX_SECTION_VS0_OFFSET + 3];
    bool is_50hz = (byte & 0x20);

    // frame size is extracted from the VAUX section, VS pack, STYPE bits
    byte = data[VAUX_SECTION_VS0_OFFSET + 3];
    byte &= 0x1f;
    if (byte == 0x00) {
        // 4:1:1/4:2:0 compression -> IEC-DV25 or DV-based DV25
        if (is_50hz)
            return 144000; // 12 DIF sequences per channel, 1 channel
        else
            return 120000; // 10 DIF sequences per channel, 1 channel
    } else if (byte == 0x04) {
        // 4:2:2 compression -> DV-based DV50
        if (is_50hz)
            return 288000; // 12 DIF sequences per channel, 2 channels
        else
            return 240000; // 10 DIF sequences per channel, 2 channels
    } else if (byte == 0x14) {
        // 1080i50 and 1080i60
        if (is_50hz)
            return 576000; // 12 DIF sequences per channel, 4 channels
        else
            return 480000; // 10 DIF sequences per channel, 4 channels
    } else if (byte == 0x18) {
        // 720p50 and 720p60
        if (is_50hz)
            return 288000; // 12 DIF sequences per channel, 2 channels
        else
            return 240000; // 10 DIF sequences per channel, 2 channels
    }

    // unknown DV type
    return ESSENCE_PARSER_NULL_FRAME_SIZE;
}

