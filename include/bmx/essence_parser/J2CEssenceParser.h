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

#ifndef BMX_J2C_ESSENCE_PARSER_H_
#define BMX_J2C_ESSENCE_PARSER_H_

#include <map>
#include <vector>

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/ByteBuffer.h>


namespace bmx
{


class J2CEssenceParser : public EssenceParser
{
public:
    J2CEssenceParser();
    virtual ~J2CEssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

public:
    uint16_t GetRsiz()      { return mRsiz; }
    uint32_t GetXsiz()      { return mXsiz; }
    uint32_t GetYsiz()      { return mYsiz; }
    uint32_t GetXOsiz()     { return mXOsiz; }
    uint32_t GetYOsiz()     { return mYOsiz; }
    uint32_t GetXTsiz()     { return mXTsiz; }
    uint32_t GetYTsiz()     { return mYTsiz; }
    uint32_t GetXTOsiz()    { return mXTOsiz; }
    uint32_t GetYTOsiz()    { return mYTOsiz; }
    uint16_t GetCsiz()      { return mCsiz; }

    uint16_t GetProfile();
    uint8_t GetMainLevel();
    uint8_t GetSubLevel();

    const std::vector<mxfJ2KComponentSizing>& GetComponentSizings()  { return mComponentSizings; }
    bool GetCompSigned(size_t index);
    uint8_t GetCompBitDepth(size_t index);

    uint8_t GetScod()                   { return mScod; }
    uint8_t GetSGcodProgOrder()         { return mSGcodProgOrder; }
    uint16_t GetSGcodNumLayers()        { return mSGcodNumLayers; }
    uint8_t GetSGcodTransformUsage()    { return mSGcodTransformUsage; }
    uint8_t GetSPcodNumLevels()         { return mSPcodNumLevels; }
    uint8_t GetSPcodWidth()             { return mSPcodWidth; }
    uint8_t GetSPcodHeight()            { return mSPcodHeight; }
    uint8_t GetSPcodStyle()             { return mSPcodStyle; }
    uint8_t GetSPcodTransformType()     { return mSPcodTransformType; }
    const std::vector<unsigned char>& GetSPcodPrecintSizes() { return mSPcodPrecintSizes; }

    bool GuessRGBALayout(bool is_yuv, bool is_rgb, bool include_fill, mxfRGBALayout *layout);

    uint8_t GetSqcd()   { return mSqcd; }
    const std::vector<unsigned char>& GetSPqcd() { return mSPqcd; }

private:
    typedef struct
    {
        uint8_t index;
        std::vector<uint16_t> tile_indexes;
        std::vector<uint32_t> tile_part_lengths;
    } TilePartData;

private:
    void ResetFrameInfo();

    bool IsMarkerOnly(uint16_t marker);
    void ParseSIZ(ByteBuffer &data_reader);
    void ParseCOD(ByteBuffer &data_reader, uint16_t length);
    void ParseTLM(ByteBuffer &data_reader, uint16_t length, std::map<uint8_t, TilePartData> *tlm_index);
    void ParseQCD(ByteBuffer &data_reader, uint16_t length);

    void SkipTilePartData(ByteBuffer &data_reader, uint16_t tile_part_index,
                          std::map<uint8_t, TilePartData> &tlm_index, uint32_t sot_offset, uint32_t psot);

private:
    uint32_t mFrameSize;
    uint16_t mRsiz;
    uint32_t mXsiz;
    uint32_t mYsiz;
    uint32_t mXOsiz;
    uint32_t mYOsiz;
    uint32_t mXTsiz;
    uint32_t mYTsiz;
    uint32_t mXTOsiz;
    uint32_t mYTOsiz;
    uint16_t mCsiz;
    std::vector<mxfJ2KComponentSizing> mComponentSizings;

    uint8_t mScod;
    uint8_t mSGcodProgOrder;
    uint16_t mSGcodNumLayers;
    uint8_t mSGcodTransformUsage;
    uint8_t mSPcodNumLevels;
    uint8_t mSPcodWidth;
    uint8_t mSPcodHeight;
    uint8_t mSPcodStyle;
    uint8_t mSPcodTransformType;
    std::vector<unsigned char> mSPcodPrecintSizes;

    uint8_t mSqcd;
    std::vector<unsigned char> mSPqcd;
};


};



#endif
