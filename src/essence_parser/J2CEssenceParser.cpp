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

#include <string.h>

#include <bmx/essence_parser/J2CEssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


typedef enum
{
    SOC = 0xff4f,
    SOT = 0xff90,
    SOD = 0xff93,
    EOC = 0xffd9,
    SIZ = 0xff51,
    COD = 0xff52,
    COC = 0xff53,
    RGN = 0xff5e,
    QCD = 0xff5c,
    QCC = 0xff5d,
    POC = 0xff5f,
    TLM = 0xff55,
    PLM = 0xff57,
    PLT = 0xff58,
    PPM = 0xff60,
    PPT = 0xff61,
    SOP = 0xff91,
    EPH = 0xff92,
    CRG = 0xff63,
    COM = 0xff64
} MarkerType;


class InvalidData : public exception
{};


J2CEssenceParser::J2CEssenceParser()
: EssenceParser()
{
    ResetFrameInfo();
}

J2CEssenceParser::~J2CEssenceParser()
{
}

uint32_t J2CEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    ByteBuffer data_reader(data, data_size, true);
    try
    {
        uint16_t marker = data_reader.GetUInt16();
        if (marker != SOC)
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }
    catch (const InsufficientBytes&)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }

    return 0;
}

uint32_t J2CEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    try
    {
        ParseFrameInfo(data, data_size);
        return mFrameSize;
    }
    catch (const InsufficientBytes&)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }
    catch (const InvalidData&)
    {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }
}

void J2CEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    ResetFrameInfo();

    map<uint8_t, TilePartData> tlm_index;
    uint32_t sot_offset = 0;
    uint32_t psot = 0;
    uint16_t tile_part_index = 0;

    ByteBuffer data_reader(data, data_size, true);

    // A codestream starts with a SOC marker and ends with a EOC marker
    while (true) {
        uint16_t marker = data_reader.GetUInt16();

        // Expect a SOC marker at the start only
        if ((data_reader.GetPos() == 2 && marker != SOC) ||
            (data_reader.GetPos() > 2 && marker == SOC))
        {
            throw InvalidData();
        }

        if (IsMarkerOnly(marker)) {
            if (marker == EOC) {
                // End of codestream
                break;
            } else if (marker == SOD) {
                // Skip the tile part data using the Psot or tile part index data information
                SkipTilePartData(data_reader, tile_part_index, tlm_index, sot_offset, psot);
                tile_part_index++;
            }
        } else {
            uint16_t length = data_reader.GetUInt16();
            if (length < 2)
                throw InvalidData();

            // Skip any remaining bytes when leaving this context
            ByteBufferLengthContext length_context(data_reader, length - 2);

            if (marker == SOT) {
                // Record the SOT offset and the Psot for skipping the tile part data
                sot_offset = data_reader.GetPos() - 4;
                data_reader.GetUInt16();   // Isot
                psot = data_reader.GetUInt32();
                if (psot > 0 && psot < 14)
                    throw InvalidData();
            } else if (marker == TLM) {
                // Construct the tile part data index that can be used to skip the tile part data
                // if Psot is set to zero
                ParseTLM(data_reader, length, &tlm_index);
            } else if (marker == SIZ) {
                ParseSIZ(data_reader);
            } else if (marker == COD) {
                ParseCOD(data_reader, length);
            } else if (marker == QCD) {
                ParseQCD(data_reader, length);
            }
        }
    }

    mFrameSize = data_reader.GetPos();
}

uint16_t J2CEssenceParser::GetProfile()
{
    if (mRsiz <= 7) {
        return mRsiz;
    } else if ((mRsiz & 0x010f) == mRsiz) {
        return mRsiz >> 4;
    } else if ((mRsiz & 0x020f) == mRsiz) {
        return mRsiz >> 4;
    } else if ((mRsiz & 0x0306) == mRsiz) {
        return mRsiz;
    } else if ((mRsiz & 0x0307) == mRsiz) {
        return mRsiz;
    } else {
        return mRsiz >> 8;
    }
}

uint8_t J2CEssenceParser::GetMainLevel()
{
    if (mRsiz <= 7) {
        return 255;
    } else if ((mRsiz & 0x010f) == mRsiz) {
        return (uint8_t)(mRsiz & 0x000f);
    } else if ((mRsiz & 0x020f) == mRsiz) {
        return (uint8_t)(mRsiz & 0x000f);
    } else if ((mRsiz & 0x0306) == mRsiz) {
        return 6;
    } else if ((mRsiz & 0x0307) == mRsiz) {
        return 7;
    } else {
        return (uint8_t)(mRsiz & 0x0f);
    }
}

uint8_t J2CEssenceParser::GetSubLevel()
{
    if (mRsiz <= 7) {
        return 255;
    } else if ((mRsiz & 0x010f) == mRsiz) {
        return 255;
    } else if ((mRsiz & 0x020f) == mRsiz) {
        return 255;
    } else if ((mRsiz & 0x0306) == mRsiz) {
        return 255;
    } else if ((mRsiz & 0x0307) == mRsiz) {
        return 255;
    } else {
        return (uint8_t)((mRsiz >> 4) & 0x0f);
    }
}

bool J2CEssenceParser::GetCompSigned(size_t index)
{
    return ((mComponentSizings.at(index).s_siz >> 7) & 0x1) != 0;
}

uint8_t J2CEssenceParser::GetCompBitDepth(size_t index)
{
    return (mComponentSizings.at(index).s_siz & 0x7f) + 1;
}

bool J2CEssenceParser::GuessRGBALayout(bool is_yuv, bool is_rgb, bool include_fill, mxfRGBALayout *layout)
{
    if (mCsiz == 3) {
        // RGB or YUV

        // Check all components unsigned and <= 32 bit
        for (int i = 0; i < 3; i++) {
            if (GetCompSigned(i))
                return false;
            if (GetCompBitDepth(i) > 32)
                return false;
        }

        if (is_yuv || (!is_rgb && mSGcodTransformUsage == 0x00)) {
            // if both is_yuv and is_rgb are false then guess YUV because no component de-corellation was required

            // Check 444, 422, 420 or 411 sub-sampling
            if (mComponentSizings[0].xr_siz != 1 ||
                mComponentSizings[0].yr_siz != 1 ||
                (mComponentSizings[1].xr_siz < 1 || mComponentSizings[1].xr_siz > 2) ||
                (mComponentSizings[1].yr_siz < 1 || mComponentSizings[1].yr_siz > 2) ||
                (mComponentSizings[2].xr_siz < 1 || mComponentSizings[2].xr_siz > 2) ||
                (mComponentSizings[2].yr_siz < 1 || mComponentSizings[2].yr_siz > 2))
            {
                return false;
            }

            memset(layout, 0, sizeof(*layout));
            layout->components[0].code = 'Y';
            layout->components[1].code = 'U';
            layout->components[2].code = 'V';

            int bits = 0;
            for (int i = 0; i < 3; i++) {
                layout->components[i].depth = GetCompBitDepth(i);
                bits += layout->components[i].depth;
            }

            if (include_fill && (bits & 0x07)) {
                // Assume a raw pixel layout with fill for the remaining bits after YUV
                layout->components[3].code = 'F';
                layout->components[3].depth = 8 - (bits & 0x07);
            }

            return true;
        } else if (is_rgb || (!is_yuv && mSGcodTransformUsage == 0x01)) {
            // if both is_yuv and is_rgb are false then guess RGB because component de-corellation was required

            // Check 444
            if (mComponentSizings[0].xr_siz != 1 ||
                mComponentSizings[0].yr_siz != 1 ||
                mComponentSizings[1].xr_siz != 1 ||
                mComponentSizings[1].yr_siz != 1 ||
                mComponentSizings[2].xr_siz != 1 ||
                mComponentSizings[2].yr_siz != 1)
            {
                return false;
            }

            memset(layout, 0, sizeof(*layout));
            layout->components[0].code = 'R';
            layout->components[1].code = 'G';
            layout->components[2].code = 'B';

            int bits = 0;
            for (int i = 0; i < 3; i++) {
                layout->components[i].depth = GetCompBitDepth(i);
                bits += layout->components[i].depth;
            }

            if (include_fill && (bits & 0x07)) {
                // Assume a raw pixel layout with fill for the remaining bits after RGB
                layout->components[3].code = 'F';
                layout->components[3].depth = 8 - (bits & 0x07);
            }

            return true;
        }
    }

    return false;
}

void J2CEssenceParser::ResetFrameInfo()
{
    mFrameSize = 0;
    mRsiz = 0;
    mXsiz = 0;
    mYsiz = 0;
    mXOsiz = 0;
    mYOsiz = 0;
    mXTsiz = 0;
    mYTsiz = 0;
    mXTOsiz = 0;
    mYTOsiz = 0;
    mCsiz = 0;
    mComponentSizings.clear();

    mScod = 0;
    mSGcodProgOrder = 0;
    mSGcodNumLayers = 0;
    mSGcodTransformUsage = 0;
    mSPcodNumLevels = 0;
    mSPcodWidth = 0;
    mSPcodHeight = 0;
    mSPcodStyle = 0;
    mSPcodTransformType = 0;
    mSPcodPrecintSizes.clear();

    mSqcd = 0;
    mSPqcd.clear();
}

bool J2CEssenceParser::IsMarkerOnly(uint16_t marker)
{
    if (marker >= 0xff30 && marker <= 0xff3f)
        return true;

    if (marker == SOC || marker == EOC || marker == SOD || marker == EPH)
        return true;

    return false;
}

void J2CEssenceParser::ParseSIZ(ByteBuffer &data_reader)
{
    mRsiz = data_reader.GetUInt16();
    mXsiz = data_reader.GetUInt32();
    mYsiz = data_reader.GetUInt32();
    mXOsiz = data_reader.GetUInt32();
    mYOsiz = data_reader.GetUInt32();
    mXTsiz = data_reader.GetUInt32();
    mYTsiz = data_reader.GetUInt32();
    mXTOsiz = data_reader.GetUInt32();
    mYTOsiz = data_reader.GetUInt32();
    mCsiz = data_reader.GetUInt16();

    mComponentSizings.clear();
    for (uint16_t i = 0 ; i < mCsiz; i++) {
        mxfJ2KComponentSizing sizing;
        sizing.s_siz = data_reader.GetUInt8();
        sizing.xr_siz = data_reader.GetUInt8();
        sizing.yr_siz = data_reader.GetUInt8();

        mComponentSizings.push_back(sizing);
    }
}

void J2CEssenceParser::ParseCOD(ByteBuffer &data_reader, uint16_t length)
{
    mScod = data_reader.GetUInt8();

    mSGcodProgOrder = data_reader.GetUInt8();
    mSGcodNumLayers = data_reader.GetUInt16();
    mSGcodTransformUsage = data_reader.GetUInt8();

    mSPcodNumLevels = data_reader.GetUInt8();
    mSPcodWidth = data_reader.GetUInt8();
    mSPcodHeight = data_reader.GetUInt8();
    mSPcodStyle = data_reader.GetUInt8();
    mSPcodTransformType = data_reader.GetUInt8();

    if (length > 12)
        mSPcodPrecintSizes = data_reader.GetBytes(length - 12);
}

void J2CEssenceParser::ParseQCD(ByteBuffer &data_reader, uint16_t length)
{
    mSqcd = data_reader.GetUInt8();
    if (length > 3)
        mSPqcd = data_reader.GetBytes(length - 3);
}

void J2CEssenceParser::ParseTLM(ByteBuffer &data_reader, uint16_t length, map<uint8_t, TilePartData> *tlm_index)
{
    uint16_t rem_len = length - 2;
    TilePartData tile_part_data;

    data_reader.GetUInt8();  // Ztlm
    tile_part_data.index = data_reader.GetUInt8();  // Stlm

    rem_len -= 2;

    uint8_t st = (tile_part_data.index >> 4) & 0x03;
    if (st == 3)
        throw InvalidData();
    uint8_t sp = (tile_part_data.index >> 6) & 0x01;

    uint8_t ttlm_len = st;
    uint8_t ptlm_len = (sp + 1) * 2;
    uint16_t num_tile_parts = rem_len / (ttlm_len + ptlm_len);
    if (rem_len != num_tile_parts * (ttlm_len + ptlm_len))
        throw InvalidData();

    for (uint16_t i = 0; i < num_tile_parts; i++) {
        if (st == 1)
            tile_part_data.tile_indexes.push_back(data_reader.GetUInt8());
        else if (st == 2)
            tile_part_data.tile_indexes.push_back(data_reader.GetUInt16());

        if (sp == 0)
            tile_part_data.tile_part_lengths.push_back(data_reader.GetUInt16());
        else
            tile_part_data.tile_part_lengths.push_back(data_reader.GetUInt32());
    }

    (*tlm_index)[tile_part_data.index] = tile_part_data;
}

void J2CEssenceParser::SkipTilePartData(ByteBuffer &data_reader, uint16_t tile_part_index,
                                             map<uint8_t, TilePartData> &tlm_index, uint32_t sot_offset, uint32_t psot)
{
    uint32_t tile_part_length = 0;
    if (psot > 0) {
        tile_part_length = psot;
    } else if (!tlm_index.empty()) {
        size_t i = 0;
        map<uint8_t, TilePartData>::const_iterator iter;
        for (iter = tlm_index.begin(); iter != tlm_index.end(); iter++) {
            const TilePartData &tile_part_data = iter->second;
            if (tile_part_index < i + tile_part_data.tile_part_lengths.size()) {
                tile_part_length = tile_part_data.tile_part_lengths[tile_part_index - i];
                break;
            }
            i += tile_part_data.tile_part_lengths.size();
        }
    }

    if (sot_offset + tile_part_length >= data_reader.GetPos()) {
        uint32_t skip_size = tile_part_length - (data_reader.GetPos() - sot_offset);
        data_reader.Skip(skip_size);
    } else if (tile_part_length > 0) {
        throw InvalidData();
    } else {
        // Tile part data extends to the EOC at the end of the codestream
        // Can't rely on detecting a EOC at the end because it could appear in the tile data
        throw InsufficientBytes();
    }
}
