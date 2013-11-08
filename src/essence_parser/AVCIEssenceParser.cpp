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

#include <bmx/essence_parser/AVCIEssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



AVCIEssenceParser::AVCIEssenceParser()
{
    Reset();
}

AVCIEssenceParser::~AVCIEssenceParser()
{
}

uint32_t AVCIEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    uint32_t offset = 0;
    if (!NextStartPrefix(data, data_size, &offset))
        return ESSENCE_PARSER_NULL_OFFSET;

    if (offset == 1 && data[0] == 0x00)
        return 0;

    return offset;
}

uint32_t AVCIEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    // TODO
    (void)data;
    (void)data_size;

    BMX_ASSERT(false);
    return ESSENCE_PARSER_NULL_FRAME_SIZE;
}

void AVCIEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    uint32_t offset = 0;

    // start NAL and check whether it is an access unit delimiter
    BMX_CHECK(NextStartPrefix(data, data_size, &offset));
    BMX_CHECK(offset == 0 || (offset == 1 && data[0] == 0x00));
    mHaveAccessUnitDelimiter = ((data[offset + 3] & 0x1f) != 9);

    // check whether the next NAL is a sequence parameter set
    offset += 3;
    BMX_CHECK(NextStartPrefix(data, data_size, &offset));
    mHaveSequenceParameterSet = ((data[offset + 3] & 0x1f) == 7);
}

bool AVCIEssenceParser::NextStartPrefix(const unsigned char *data, uint32_t size, uint32_t *offset)
{
    uint32_t state = (uint32_t)(-1);
    for (; (*offset) < size - 1; (*offset)++) {
        state <<= 8;
        state |= data[(*offset)];
        if ((state & 0x00ffffff) == 0x000001) {
            (*offset) -= 2;
            return true;
        }
    }

    return false;
}

void AVCIEssenceParser::Reset()
{
    mHaveAccessUnitDelimiter = false;
    mHaveSequenceParameterSet = false;
}

