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

#ifndef _JXS_H_
#define _JXS_H_

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/frame/FrameBuffer.h>
#include <assert.h>
#include <string>
#include <list>
#include <cstring>

namespace bmx
{
    #define NO_COPY_CONSTRUCT(T)   \
                                    T(const T&); \
                                    T& operator=(const T&)
    #define ui64_C(c) (uint64_t)(c)
    #define SUCCESS(v) (((v) < 0) ? 0 : 1)
    #define FAILURE(v) (((v) < 0) ? 1 : 0)

    inline uint16_t Swap16(uint16_t i)
    {
        return ((i << 8) | ((i & 0xff00) >> 8));
    }

    inline uint32_t Swap32(uint32_t i)
    {
        return
            ((i & 0x000000ffUL) << 24) |
            ((i & 0xff000000UL) >> 24) |
            ((i & 0x0000ff00UL) << 8) |
            ((i & 0x00ff0000UL) >> 8);
    }

    struct JPEGXSPictureSubDescriptor
    {
        uint16_t  JPEGXSPpih;
        uint16_t  JPEGXSPlev;
        uint16_t  JPEGXSWf;
        uint16_t  JPEGXSHf;
        uint8_t   JPEGXSNc;
        ByteArray JPEGXSComponentTable;
        uint16_t  JPEGXSCw;
        uint16_t  JPEGXSHsl;
        uint32_t  JPEGXSMaximumBitRate;
    };

    struct GenericDescriptor
    {
        uint8_t FrameLayout;
        uint32_t StoredWidth;
        uint32_t StoredHeight;
        Rational AspectRatio;
        UL PictureEssenceCoding;
        uint64_t ContainerDuration;
    };

    const size_t MaxComponents = 4; // ISO 21122-1 Annex A.2 up to 8 components
    const size_t MaxHorizontalLevels = 15;
    const size_t MaxVerticalLevels = 2;

#pragma pack(1)
    struct ImageComponent_t  // Essentially, a lookalike of the CDT marker, just with less bit-packing
    {
        unsigned char Bc;  // Bitdepth (literal, not -1)
        unsigned char Sx;
        unsigned char Sy;  // Subsampling factors, horizontal and vertically. Bit-packed in the marker.
    };
#pragma pack()

    const unsigned char Magic[] = { 0xff, 0x10, 0xff };

    enum Marker_t
    {
        MRK_NIL = 0,
        MRK_SOC = 0xff10, // Start of codestream
        MRK_EOC = 0xff11, // End of codestream
        MRK_PIH = 0xff12, // Picture header
        MRK_CDT = 0xff13, // Component table
        MRK_WGT = 0xff14, // Weights table
        MRK_COM = 0xff15, // Extensions marker
        MRK_NLT = 0xff16, // Nonlinearity marker (21122-1 2nd)
        MRK_CWD = 0xff17, // Component-dependent decomposition marker (21122-1 2nd)
        MRK_CTS = 0xff18, // Colour transformation spec marker (21122-1 2nd)
        MRK_CRG = 0xff19, // Component registration marker (21122-1 2nd)
        MRK_SLH = 0xff20, // Slice header
        MRK_CAP = 0xff50  // Capabilities marker
    };

    //
    class Marker
    {
        NO_COPY_CONSTRUCT(Marker);

    public:
        Marker_t             m_Type;
        bool                 m_IsSegment;
        size_t               m_DataSize;
        const unsigned char* m_Data;

        Marker() : m_Type(MRK_NIL), m_IsSegment(false), m_DataSize(0), m_Data(0) {}
        ~Marker() {}

    };

    // accessor objects for marker segments
    namespace Accessor
    {
        // capability marker
        class CAP
        {
            const unsigned char* m_MarkerData;
            uint32_t m_DataSize;
            NO_COPY_CONSTRUCT(CAP);
            CAP();

        public:
            CAP(const Marker& M)
            {
                assert(M.m_Type == MRK_CAP);
                m_MarkerData = M.m_Data;
                m_DataSize = M.m_DataSize;
            }

            ~CAP() {}

            inline uint16_t Size()   const { return Swap16(*(uint16_t*)(m_MarkerData - 2)); }

        };

        // Nonlinearity marker(21122 - 1 2nd)
        class NLT
        {
            const unsigned char* m_MarkerData;
            NO_COPY_CONSTRUCT(NLT);
            NLT();

        public:
            NLT(const Marker& M)
            {
                assert(M.m_Type == MRK_NLT);
                m_MarkerData = M.m_Data;
            }

            ~NLT() {}

            inline uint16_t Size()   const { return Swap16(*(uint16_t*)(m_MarkerData - 2)); }

        };

        // picture header
        class PIH
        {
            const unsigned char*    m_MarkerData;
            uint32_t                m_DataSize;
            NO_COPY_CONSTRUCT(PIH);
            PIH();

        public:
            PIH(const Marker& M)
            {
                assert(M.m_Type == MRK_PIH);
                m_MarkerData = M.m_Data;
                m_DataSize = M.m_DataSize;
            }

            ~PIH() {}

            inline uint16_t LpihSize()	        const { return Swap16(*(uint16_t*)(m_MarkerData - 2)); }
            inline uint32_t LcodSize()	        const { return Swap32(*(uint32_t*)(m_MarkerData)); }
            inline uint16_t Ppih()		        const { return Swap16(*(uint16_t*)(m_MarkerData + 4)); }
            inline uint16_t Plev()		        const { return Swap16(*(uint16_t*)(m_MarkerData + 6)); }
            inline uint16_t Wf()		       	const { return Swap16(*(uint16_t*)(m_MarkerData + 8)); }
            inline uint16_t Hf()			    const { return Swap16(*(uint16_t*)(m_MarkerData + 10)); }
            inline uint16_t Cw()			    const { return Swap16(*(uint16_t*)(m_MarkerData + 12)); }
            inline uint16_t Hsl()			    const { return Swap16(*(uint16_t*)(m_MarkerData + 14)); }
            inline unsigned char  Nc()			const { return (*(unsigned char*)(m_MarkerData + 16)); }
            inline unsigned char  Ng()			const { return *(unsigned char*)(m_MarkerData + 17); }
            inline unsigned char  Ss()			const { return *(unsigned char*)(m_MarkerData + 18); }
            inline unsigned char  Cpih()		const { return (*(unsigned char*)(m_MarkerData + 21)) & 0x0f; }
            inline unsigned char  Nlx()			const { return (*(unsigned char*)(m_MarkerData + 22)) >> 4; }
            inline unsigned char  Nly()			const { return (*(unsigned char*)(m_MarkerData + 22)) & 0x0f; }

        };

        class CDT
        {
            const unsigned char* m_MarkerData;
            NO_COPY_CONSTRUCT(CDT);
            CDT();

        public:
            CDT(const Marker& M)
            {
                assert(M.m_Type == MRK_CDT);
                m_MarkerData = M.m_Data;
            }

            ~CDT() {}

            inline unsigned char  Bc(int i)          const { return *(m_MarkerData + (i << 1)); }
            inline unsigned char  Sx(int i)          const { return *(m_MarkerData + 1 + (i << 1)) >> 4; }
            inline unsigned char  Sy(int i)          const { return *(m_MarkerData + 1 + (i << 1)) & 0x0f; }

        };

        class WGT
        {
            const unsigned char* m_MarkerData;
            NO_COPY_CONSTRUCT(WGT);
            WGT();

        public:
            WGT(const Marker& M)
            {
                assert(M.m_Type == MRK_WGT);
                m_MarkerData = M.m_Data;
            }

            ~WGT() {}

        };

        class COM
        {
            const unsigned char* m_MarkerData;
            NO_COPY_CONSTRUCT(COM);
            COM();

        public:
            COM(const Marker& M)
            {
                assert(M.m_Type == MRK_COM);
                m_MarkerData = M.m_Data;
            }

            ~COM() {}

        };
    }

    // this is the same as the codestream parser
    class JXSEssenceParser : public EssenceParser
    {
        uint32_t mFrameSize;
    public:
        JXSEssenceParser();
        virtual ~JXSEssenceParser() {}

        virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
        virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);
        virtual void     ParseFrameInfo(const unsigned char *data, uint32_t data_size);

        uint16_t       GetJPEGXSPpih()              { return m_subDesc.JPEGXSPpih; }
        uint16_t       GetJPEGXSPlev()              { return m_subDesc.JPEGXSPlev; }
        uint16_t       GetJPEGXSWf()                { return m_subDesc.JPEGXSWf; }
        uint16_t       GetJPEGXSHf()                { return m_subDesc.JPEGXSHf; }
        uint8_t        GetJPEGXSNc()                { return m_subDesc.JPEGXSNc; }
        ByteArray      GetJPEGXSComponentTable()    { return m_subDesc.JPEGXSComponentTable; }
        uint16_t       GetJPEGXSCw()                { return m_subDesc.JPEGXSCw; }
        uint16_t       GetJPEGXSHsl()               { return m_subDesc.JPEGXSHsl; }
        uint32_t       GetJPEGXSMaximumBitRate()    { return m_subDesc.JPEGXSMaximumBitRate; }

        uint8_t        GetFrameLayout()             { return m_picDesc.FrameLayout; }
        Rational       GetAspectRatio()             { return m_picDesc.AspectRatio; }
        uint32_t       GetStoredWidth()             { return m_picDesc.StoredWidth; }
        uint32_t       GetStoredHeight()            { return m_picDesc.StoredHeight; }
        UL             GetPictureEssenceCoding()    { return m_picDesc.PictureEssenceCoding; }
        uint64_t       GetContainerDuration()       { return m_picDesc.ContainerDuration; }

        JPEGXSPictureSubDescriptor m_subDesc;
        GenericDescriptor m_picDesc;

    };

}

#endif