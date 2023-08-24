/*
 * Parse raw PCM essence data
 *
 * Copyright (C) 2009, British Broadcasting Corporation
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

#ifndef __PCM_ESSENCE_PARSER_H__
#define __PCM_ESSENCE_PARSER_H__

#include "RawEssenceParser.h"


class PCMEssenceParser : public RawEssenceParser
{
public:
    PCMEssenceParser(mxfpp::File *file, int64_t essence_length, mxfUL essence_label, mxfRational edit_rate,
                     const mxfpp::FileDescriptor *file_descriptor);
    virtual ~PCMEssenceParser();

public:
    // from RawEssenceParser
    virtual bool Read(DynamicByteArray *data, uint32_t *num_samples);
    virtual bool Seek(int64_t position);
    virtual int64_t DetermineDuration();

private:
    uint32_t mFrameSizeSequence[5];
    int mSequenceLen;
    uint32_t mFrameSequenceSize;
    unsigned int mBytesPerSample;
};


#endif

