/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#ifndef BMX_ST_436_ELEMENT_H_
#define BMX_ST_436_ELEMENT_H_


#include <vector>

#include <bmx/BMXTypes.h>
#include <bmx/ByteArray.h>



namespace bmx
{


typedef enum
{
    VBI_FRAME               = 0x01,
    VBI_FIELD1              = 0x02,
    VBI_FIELD2              = 0x03,
    VBI_PROGRESSIVE_FRAME   = 0x04,
} VBIWrappingType;

typedef enum
{
    VBI_1_BIT_COMP_LUMA         = 1,
    VBI_1_BIT_COMP_COLOR        = 2,
    VBI_1_BIT_COMP_LUMA_COLOR   = 3,
    VBI_8_BIT_COMP_LUMA         = 4,
    VBI_8_BIT_COMP_COLOR        = 5,
    VBI_8_BIT_COMP_LUMA_COLOR   = 6,
    VBI_10_BIT_COMP_LUMA        = 7,
    VBI_10_BIT_COMP_COLOR       = 8,
    VBI_10_BIT_COMP_LUMA_COLOR  = 9,
} VBISampleCoding;

typedef enum
{
    VANC_FRAME              = 0x01,
    VANC_FIELD1             = 0x02,
    VANC_FIELD2             = 0x03,
    VANC_PROGRESSIVE_FRAME  = 0x04,
    HANC_FRAME              = 0x11,
    HANC_FIELD1             = 0x12,
    HANC_FIELD2             = 0x13,
    HANC_PROGRESSIVE_FRAME  = 0x14,
} ANCWrappingType;

typedef enum
{
    ANC_8_BIT_COMP_LUMA             = 4,
    ANC_8_BIT_COMP_COLOR            = 5,
    ANC_8_BIT_COMP_LUMA_COLOR       = 6,
    ANC_10_BIT_COMP_LUMA            = 7,
    ANC_10_BIT_COMP_COLOR           = 8,
    ANC_10_BIT_COMP_LUMA_COLOR      = 9,
    ANC_8_BIT_COMP_LUMA_ERROR       = 10,
    ANC_8_BIT_COMP_COLOR_ERROR      = 11,
    ANC_8_BIT_COMP_LUMA_COLOR_ERROR = 12,
} ANCSampleCoding;


class ST436Line
{
public:
    ST436Line(bool is_vbi_in);
    ~ST436Line();

    void Construct(ByteArray *data);
    void Parse(const unsigned char *data, uint64_t *size_inout);

public:
    uint16_t line_number;
    uint8_t wrapping_type;
    uint8_t payload_sample_coding;
    uint16_t payload_sample_count;
    const unsigned char *payload_data; // only valid for lifetime of referenced data
    uint32_t payload_size;

    bool is_vbi;  // true -> VBI data, false -> ANC data
};


class ST436Element
{
public:
    ST436Element(bool is_vbi_in);
    ~ST436Element();

    void Construct(ByteArray *data);
    void Parse(const unsigned char *data, uint64_t size);

public:
    std::vector<ST436Line> lines;

    bool is_vbi;  // true -> VBI data, false -> ANC data
};


};



#endif

