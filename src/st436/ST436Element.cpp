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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <cstring>

#include <mxf/mxf.h>

#include <bmx/st436/ST436Element.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define LINE_HEADER_SIZE    14



ST436Line::ST436Line(bool is_vbi_in)
{
    is_vbi = is_vbi_in;

    line_number = 0;
    wrapping_type = 0;
    payload_sample_coding = 0;
    payload_sample_count = 0;
    payload_data = 0;
    payload_size = 0;
}

ST436Line::~ST436Line()
{
}

void ST436Line::Construct(ByteArray *data)
{
    uint32_t aligned_payload_size = (payload_size + 3) & ~3U;

    data->Grow(LINE_HEADER_SIZE + aligned_payload_size);
    unsigned char *header_data = data->GetBytesAvailable();

    mxf_set_uint16(line_number,                   header_data);
    mxf_set_uint8(wrapping_type,                  &header_data[2]);
    mxf_set_uint8(payload_sample_coding,          &header_data[3]);
    mxf_set_uint16(payload_sample_count,          &header_data[4]);
    mxf_set_array_header(aligned_payload_size, 1, &header_data[6]);
    data->IncrementSize(LINE_HEADER_SIZE);

    if (payload_size > 0)
        data->Append(payload_data, payload_size);

    if (aligned_payload_size > payload_size) {
      uint32_t padding_size = aligned_payload_size - payload_size;
      memset(data->GetBytesAvailable(), 0, padding_size);
      data->IncrementSize(padding_size);
    }
}

void ST436Line::Parse(const unsigned char *data, uint64_t *size_inout)
{
    uint64_t size = *size_inout;

    if (size < LINE_HEADER_SIZE)
        BMX_EXCEPTION(("ST 436 line size %"PRIu64" is insufficient to contain the header %u", size, LINE_HEADER_SIZE));

    uint32_t array_len, array_item_len;
    mxf_get_uint16(data,           &line_number);
    mxf_get_uint8(&data[2],        &wrapping_type);
    mxf_get_uint8(&data[3],        &payload_sample_coding);
    mxf_get_uint16(&data[4],       &payload_sample_count);
    mxf_get_array_header(&data[6], &array_len, &array_item_len);

    if (array_len > 0) {
        if (array_item_len != 1)
            BMX_EXCEPTION(("Invalid ST 436 line payload byte array item length %u\n", array_item_len));
        if (array_len > size - LINE_HEADER_SIZE) {
            BMX_EXCEPTION(("ST 436 line payload byte array size %u exceeds available size %"PRIu64"\n",
                           array_len, size - LINE_HEADER_SIZE));
        }
        payload_data = &data[LINE_HEADER_SIZE];
        payload_size = array_len;
    } else {
        payload_data = 0;
        payload_size = 0;
    }

    *size_inout = size - (LINE_HEADER_SIZE + payload_size);
}



ST436Element::ST436Element(bool is_vbi_in)
{
    is_vbi = is_vbi_in;
}

ST436Element::~ST436Element()
{
}

void ST436Element::Construct(ByteArray *data)
{
    if (lines.size() > UINT16_MAX)
        BMX_EXCEPTION(("Number of ST 436 lines %"PRIszt" exceeds maximum %u", lines.size(), UINT16_MAX));

    data->Grow(2);
    mxf_set_uint16((uint16_t)lines.size(), data->GetBytesAvailable());
    data->IncrementSize(2);

    size_t i;
    for (i = 0; i < lines.size(); i++)
        lines[i].Construct(data);
}

void ST436Element::Parse(const unsigned char *data, uint64_t size)
{
    lines.clear();

    if (size == 0)
        return;

    if (size < 2)
        BMX_EXCEPTION(("ST 436 element data size %"PRIu64" is too small", size));

    uint16_t num_lines = 0;
    mxf_get_uint16(data, &num_lines);

    uint64_t rem_size = size - 2;
    uint16_t i;
    for (i = 0; i < num_lines; i++) {
        ST436Line line(is_vbi);
        line.Parse(&data[size - rem_size], &rem_size);
        lines.push_back(line);
    }
}

