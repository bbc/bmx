/*
 * Parse raw essence data
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

#ifndef __RAW_ESSENCE_PARSER_H__
#define __RAW_ESSENCE_PARSER_H__

#include <vector>

#include "../Common/DynamicByteArray.h"
#include "FrameOffsetIndexTable.h"

namespace mxfpp
{
class File;
class FileDescriptor;
};


class RawEssenceParser
{
public:
    static RawEssenceParser* Create(mxfpp::File *file, int64_t essence_length, mxfUL essence_label,
                                    const mxfpp::FileDescriptor *file_descriptor, mxfRational edit_rate,
                                    uint32_t frame_size, FrameOffsetIndexTableSegment *index_table);

public:
    virtual ~RawEssenceParser();

    mxfUL GetEssenceContainerLabel() { return mEssenceLabel; }
    int64_t GetDuration() { return mDuration; }
    int64_t GetPosition() { return mPosition; }

    int64_t GetEssenceOffset() { return mEssenceOffset; }

    bool IsEOF();

public:
    virtual bool Read(DynamicByteArray *data, uint32_t *num_samples) = 0;
    virtual bool Seek(int64_t position) = 0;
    virtual int64_t DetermineDuration() = 0;

protected:
    RawEssenceParser(mxfpp::File *file, int64_t essence_length, mxfUL essence_label);

protected:
    mxfpp::File *mFile;
    int64_t mEssenceLength;
    mxfUL mEssenceLabel;
    int64_t mEssenceStartOffset;

    int64_t mPosition;
    int64_t mDuration;
    int64_t mEssenceOffset;
};


#endif

