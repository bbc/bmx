/*
 * Copyright (C) 2023, Fraunhofer IIS
 * All Rights Reserved.
 *
 * Author: Nisha Bhaskar
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

#include <assert.h>
#include <bmx/essence_parser/JXSEssenceParser.h>
#include <bmx/Logging.h>
#include <bmx/ByteBuffer.h>
#include <bmx/BMXException.h>


using namespace bmx;
using namespace std;

class InvalidData : public exception
{};

int32_t GetNextMarker(const uint8_t** buf, const uint8_t** end, Marker& Marker)
{
    assert((buf != 0) && (*buf != 0));

    if ((*buf + 2) > *end)
        throw InsufficientBytes();

    if (*(*buf)++ != 0xff)
        return -1;

    Marker.m_Type = (Marker_t)(0xff00 | *(*buf)++);
    Marker.m_IsSegment = Marker.m_Type != MRK_SOC && Marker.m_Type != MRK_SLH && Marker.m_Type != MRK_EOC;

    if (Marker.m_IsSegment)
    {
        if ((*buf + 2) > *end)
            throw InsufficientBytes();

        Marker.m_DataSize = *(*buf)++ << 8;
        Marker.m_DataSize |= *(*buf)++;
        Marker.m_DataSize -= 2;
        Marker.m_Data = *buf;

        if (*buf + Marker.m_DataSize > *end)
            throw InsufficientBytes();

        *buf += Marker.m_DataSize;
    }

    return 1;
}

const char*
GetMarkerString(Marker_t m)
{
    switch (m)
    {
    case MRK_NIL: return "NIL"; break;
    case MRK_SOC: return "SOC: Start of codestream"; break;
    case MRK_EOC: return "SOT: End of codestream"; break;
    case MRK_PIH: return "PIH: Picture header"; break;
    case MRK_CDT: return "CDT: Component table"; break;
    case MRK_WGT: return "WGT: Weights table"; break;
    case MRK_COM: return "COM: Extension marker"; break;
    case MRK_NLT: return "NLT: Nonlinearity marker"; break;
    case MRK_CWD: return "CWD: Component dependent wavelet decomposition marker"; break;
    case MRK_CTS: return "CTS: Colour transformation specification marker"; break;
    case MRK_CRG: return "CRG: Component registration marker"; break;
    case MRK_SLH: return "SLH: Slice header"; break;
    case MRK_CAP: return "CAP: Capabilities marker"; break;
    }

    return "Unknown marker code";
}

uint32_t JXSEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    ByteBuffer data_reader(data, data_size, true);
    try
    {
        uint16_t marker = data_reader.GetUInt16();
        if (marker != MRK_SOC)
            return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }
    catch (const InsufficientBytes&)
    {
        return ESSENCE_PARSER_NULL_OFFSET;
    }

    return 0;
}

void JXSEssenceParser::ResetParseFrameSize()
{
}

uint32_t JXSEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    try
    {
        ParseFrameInfo(data, data_size);
        return mFrameSize;
    }
    catch (const InsufficientBytes &ex)
    {
        if (ex.GetRequiredSize() > 0)
            return ex.GetRequiredSize();
        else
            return ESSENCE_PARSER_NULL_OFFSET;
    }
    catch (const InvalidData&)
    {
        return ESSENCE_PARSER_NULL_FRAME_SIZE;
    }
}

void JXSEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    int32_t result = 0;
    Marker NextMarker;
    unsigned char *start_of_data = 0;
    const unsigned char* p = data; 
    const unsigned char* end_p = p + data_size;
    mFrameSize = 0;
    // initialize optional items
    bool  pih = false;
    bool  cdt = false;
    bool  soc = false;

    ImageComponent_t image_components[MaxComponents];

    while (p < end_p && SUCCESS(result))
    {
        result = GetNextMarker(&p, &end_p, NextMarker);

        if (FAILURE(result))
        {
            throw InvalidData();
        }

        switch (NextMarker.m_Type)
        {
        case MRK_SOC:
        {
            // This is the SOC symbol.
            if (soc)
            {
                throw InvalidData();
            }
            soc = true;
        }
        break;
        case MRK_EOC:
        {
            throw InvalidData();
        }
        break;
        case MRK_PIH:
        {
            // This is the real picture header.

            if (!soc)
            {
                throw InvalidData();
            }
        
            if (pih) 
            {
                throw InvalidData();
            }
            else 
            {
                uint32_t size;
                Accessor::PIH PIH_(NextMarker);
                pih = true;
                size = PIH_.LpihSize();

                if (size != 28 - 2)
                {
                    throw InvalidData();
                }
                    
                // size of the bitstream
                size = PIH_.LcodSize();
                if (size > data_size)
                    throw InsufficientBytes(size);
                mFrameSize = size;
                // Profile and level
                m_subDesc.JPEGXSPpih = PIH_.Ppih(); // profile
                m_subDesc.JPEGXSPlev = PIH_.Plev(); // level 
                // Width and Height
                m_picDesc.StoredWidth = PIH_.Wf();
                m_picDesc.StoredHeight = PIH_.Hf();
                m_subDesc.JPEGXSWf = PIH_.Wf();
                m_subDesc.JPEGXSHf = PIH_.Hf();
                m_subDesc.JPEGXSCw = PIH_.Cw();
                m_subDesc.JPEGXSHsl = PIH_.Hsl(); // height in slices
                if (PIH_.Hsl() < 1) // This includes the EOF check
                {
                    throw InvalidData();
                }
                // Number of compoennts.
                m_subDesc.JPEGXSNc = PIH_.Nc();
                if (m_subDesc.JPEGXSNc != 3)
                {
                    throw InvalidData();
                }
                if (PIH_.Ng() != 4)
                    throw InvalidData();
                if (PIH_.Ss() != 8)
                    throw InvalidData();
                if (PIH_.Nlx() == 0)
                    throw InvalidData();
                if (PIH_.Nly() > MaxVerticalLevels)
                    throw InvalidData();
            }
        }
        break;
        case MRK_CDT:
        {
            if (pih) {
                Accessor::CDT CDT_(NextMarker);
                int i, count = NextMarker.m_DataSize >> 1;
                for (i = 0; i < count && i < m_subDesc.JPEGXSNc; i++) {
                    image_components[i].Bc = CDT_.Bc(i);
                    image_components[i].Sx = CDT_.Sx(i); // subsampling in x
                    image_components[i].Sy = CDT_.Sy(i); // y
                }
                cdt = true;
            }
            else {
                throw InvalidData();
            }
        }
        break;
        case MRK_SLH: // slice header: the entropy coded data starts here
            if (start_of_data != 0)
                *start_of_data = p - data; 
            p = end_p;
            break;
        case MRK_NIL:
        case MRK_WGT:
        case MRK_COM:
        case MRK_NLT:
        case MRK_CWD:
        case MRK_CTS:
        case MRK_CRG:
        case MRK_CAP:
            break;
        }
        
    }
    // Check if pih, soc and cdt have been parsed
    if (mFrameSize > 0)
    {
        if (!soc || !pih || !cdt)
            throw InvalidData();
    }
    else
        throw InsufficientBytes();

    int comps = (m_subDesc.JPEGXSNc > 8) ? 8 : m_subDesc.JPEGXSNc;
    m_subDesc.JPEGXSComponentTable.Allocate(4 + (comps << 1));
    m_subDesc.JPEGXSComponentTable.SetSize(4 + (comps << 1));

    unsigned char *buffer = m_subDesc.JPEGXSComponentTable.GetBytes();

    buffer[0] = 0xff;
    buffer[1] = 0x13; // the marker
    buffer[2] = 0x00;
    buffer[3] = comps * 2 + 2; // The size.
    for (int j = 0; j < comps; j++) {
        buffer[4 + (j << 1)] = image_components[j].Bc;
        buffer[5 + (j << 1)] = (image_components[j].Sx << 4) | (image_components[j].Sy);
    }
}

JXSEssenceParser::JXSEssenceParser()
{
    m_subDesc.JPEGXSWf = 0;
    m_subDesc.JPEGXSHf = 0;
    m_subDesc.JPEGXSCw = 0;
    m_subDesc.JPEGXSHsl = 0;
    m_subDesc.JPEGXSPlev = 0;
    m_subDesc.JPEGXSPpih = 0;
    m_subDesc.JPEGXSMaximumBitRate = 0;
    m_subDesc.JPEGXSComponentTable.Clear();

    m_picDesc.FrameLayout = 0;
    m_picDesc.StoredHeight = 0;
    m_picDesc.StoredWidth = 0;
    m_picDesc.ContainerDuration = 0;
    m_picDesc.PictureEssenceCoding = g_Null_UL;

    mFrameSize = 0;
}