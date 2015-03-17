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

#include <bmx/essence_parser/VC3EssenceParser.h>
#include "EssenceParserUtils.h"
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define HEADER_PREFIX_HVN0  0x000000028000LL
#define HEADER_PREFIX_S     0x000002800000LL


typedef struct
{
    uint32_t compression_id;
    bool is_progressive;
    uint16_t frame_width;
    uint16_t frame_height;
    uint8_t bit_depth;
    uint32_t frame_size;
} CompressionParameters;

static const CompressionParameters COMPRESSION_PARAMETERS[] =
{
    {1235,  true,   1920,   1080,   10,  917504},
    {1237,  true,   1920,   1080,   8,   606208},
    {1238,  true,   1920,   1080,   8,   917504},
    {1241,  false,  1920,   1080,   10,  917504},
    {1242,  false,  1920,   1080,   8,   606208},
    {1243,  false,  1920,   1080,   8,   917504},
    {1244,  false,  1920,   1080,   8,   606208},
    {1250,  true,   1280,   720,    10,  458752},
    {1251,  true,   1280,   720,    8,   458752},
    {1252,  true,   1280,   720,    8,   303104},
    {1253,  true,   1920,   1080,   8,   188416},
    {1258,  true,   1280,   720,    8,   212992},
    {1259,  true,   1920,   1080,   8,   417792},
    {1260,  false,  1920,   1080,   8,   417792},
};



static uint64_t get_uint64(const unsigned char *data)
{
    return (((uint64_t)data[0]) << 56) |
           (((uint64_t)data[1]) << 48) |
           (((uint64_t)data[2]) << 40) |
           (((uint64_t)data[3]) << 32) |
           (((uint64_t)data[4]) << 24) |
           (((uint64_t)data[5]) << 16) |
           (((uint64_t)data[6]) << 8) |
             (uint64_t)data[7];
}

static uint32_t get_uint32(const unsigned char *data)
{
    return (((uint32_t)data[0]) << 24) |
           (((uint32_t)data[1]) << 16) |
           (((uint32_t)data[2]) << 8) |
             (uint32_t)data[3];
}

static uint16_t get_uint16(const unsigned char *data)
{
    return (((uint16_t)data[0]) << 8) |
             (uint16_t)data[1];
}



VC3EssenceParser::VC3EssenceParser()
{
    mCompressionId = 0;
    mIsProgressive = false;
    mFrameWidth = 0;
    mFrameHeight = 0;
    mBitDepth = 0;
    mFrameSize = 0;
}

VC3EssenceParser::~VC3EssenceParser()
{
}

uint32_t VC3EssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    uint64_t state = 0;
    uint32_t i;
    for (i = 0; i < data_size; i++) {
        state = (state << 8) | data[i];
        if ((state & 0xffffffff0000LL) == HEADER_PREFIX_S &&
            (state & 0x000000000003LL) < 3)    // coding unit is progressive frame or field 1
        {
            return i - 5;
        }
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

uint32_t VC3EssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    if (data_size < VC3_PARSER_MIN_DATA_SIZE)
        return ESSENCE_PARSER_NULL_OFFSET;

    // check header prefix
    uint64_t prefix = get_uint64(data) >> 24;
    BMX_CHECK( (prefix & 0xffffffff00LL) == HEADER_PREFIX_HVN0 &&
              ((prefix & 0x00000000ffLL) == 1 || (prefix & 0x00000000ffLL) == 2));

    uint32_t compression_id = get_uint32(data + 40);

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(COMPRESSION_PARAMETERS); i++)
    {
        if (compression_id == COMPRESSION_PARAMETERS[i].compression_id) {
            if (data_size >= COMPRESSION_PARAMETERS[i].frame_size)
                return COMPRESSION_PARAMETERS[i].frame_size;
            else
                return ESSENCE_PARSER_NULL_OFFSET;
        }
    }

    return ESSENCE_PARSER_NULL_FRAME_SIZE;
}

void VC3EssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);
    BMX_CHECK(data_size >= VC3_PARSER_MIN_DATA_SIZE);

    // check header prefix
    uint64_t prefix = get_uint64(data) >> 24;
    BMX_CHECK( (prefix & 0xffffffff00LL) == HEADER_PREFIX_HVN0 &&
              ((prefix & 0x00000000ffLL) == 1 || (prefix & 0x00000000ffLL) == 2));

    // compression id
    mCompressionId = get_uint32(data + 40);
    size_t param_index;
    for (param_index = 0; param_index < BMX_ARRAY_SIZE(COMPRESSION_PARAMETERS); param_index++)
    {
        if (mCompressionId == COMPRESSION_PARAMETERS[param_index].compression_id)
            break;
    }
    BMX_CHECK(param_index < BMX_ARRAY_SIZE(COMPRESSION_PARAMETERS));

    // Note: found that an Avid MC v3.0 file containing 1252 720p50 had FFC=01h and SST=1;
    //       SST should be 0 for progressive scan. DNxHD_Compliance_Issue_To_Licensees-1.doc
    //       states that some Avid bitstreams may have SST incorrectly set to 1080i
    //       Ignore the bitstream information and use the scan type associated with the compression id
    mIsProgressive = COMPRESSION_PARAMETERS[param_index].is_progressive;

    // image geometry
    // Note: DNxHD_Compliance_Issue_To_Licensees-1.doc states that some Avid bitstreams,
    //       e.g. produced by Avid Media Composer 3.0, may have ALPF incorrectly set to 1080 for
    //       1080i sources. Ignore the bitstream information and use the frame height associated
    //       with the compression id
    mFrameHeight = COMPRESSION_PARAMETERS[param_index].frame_height;
    mFrameWidth = get_uint16(data + 26);
    BMX_CHECK(mFrameWidth == COMPRESSION_PARAMETERS[param_index].frame_width);
    uint32_t sbd_bits = get_bits(data, data_size, 33 * 8, 3);
    BMX_CHECK(sbd_bits == 2 || sbd_bits == 1);
    mBitDepth = (sbd_bits == 2 ? 10 : 8);
    BMX_CHECK(mBitDepth == COMPRESSION_PARAMETERS[param_index].bit_depth);

    mFrameSize = COMPRESSION_PARAMETERS[param_index].frame_size;
}

