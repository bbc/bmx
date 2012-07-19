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

#ifndef BMX_AVCI_ESSENCE_PARSER_H_
#define BMX_AVCI_ESSENCE_PARSER_H_


#include <bmx/essence_parser/EssenceParser.h>


#define AVCI_HEADER_SIZE    512


namespace bmx
{


// access unit delimiter = zero byte (0x00) + start prefix (0x000001) + type (9) +
// primary pic type (0 == I slices) + stop_bit
static const unsigned char AVCI_ACCESS_UNIT_DELIMITER[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10};

// filler = zero byte (0x00) + start prefix (0x000001) + type (12) + stop bit
static const unsigned char AVCI_FILLER[6] = {0x00, 0x00, 0x00, 0x01, 0x0c, 0x80};


class AVCIEssenceParser : public EssenceParser
{
public:
    AVCIEssenceParser();
    virtual ~AVCIEssenceParser();

    virtual uint32_t ParseFrameStart(const unsigned char *data, uint32_t data_size);
    virtual uint32_t ParseFrameSize(const unsigned char *data, uint32_t data_size);

    virtual void ParseFrameInfo(const unsigned char *data, uint32_t data_size);

public:
    bool HaveAccessUnitDelimiter() const { return mHaveAccessUnitDelimiter; }
    bool HaveSequenceParameterSet() const { return mHaveSequenceParameterSet; }

private:
    void Reset();

    bool NextStartPrefix(const unsigned char *data, uint32_t size, uint32_t *offset);

private:
    bool mHaveAccessUnitDelimiter;
    bool mHaveSequenceParameterSet;
};


};



#endif

