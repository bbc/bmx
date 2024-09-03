/*
 * Copyright (C) 2024, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <limits.h>

#include <bmx/essence_parser/EssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


ParsedFrameSize::ParsedFrameSize()
{
    Reset();
}

ParsedFrameSize::ParsedFrameSize(const ParsedFrameSize &other)
{
    mIsFields = other.mIsFields;
    mFieldSizes = other.mFieldSizes;
}

ParsedFrameSize::~ParsedFrameSize()
{
}

ParsedFrameSize::ParsedFrameSize(uint32_t frame_size)
{
    Reset();
    SetSize(frame_size);
}

ParsedFrameSize::ParsedFrameSize(std::pair<uint32_t, uint32_t> field_sizes)
{
    Reset();
    SetFirstFieldSize(field_sizes.first);
    SetSecondFieldSize(field_sizes.second);
}

ParsedFrameSize& ParsedFrameSize::operator=(const ParsedFrameSize &other)
{
    mIsFields = other.mIsFields;
    mFieldSizes = other.mFieldSizes;
    return *this;
}

pair<uint32_t, uint32_t> ParsedFrameSize::GetFieldSizes() const
{
    if (IsFields() && IsComplete())
        return mFieldSizes;
    else if (!IsFields() && IsComplete())
        return make_pair(mFieldSizes.first, 0);
    else
        return make_pair(0, 0);
}
uint32_t ParsedFrameSize::GetFirstFieldSize() const
{
    if (HaveFirstField())
        return mFieldSizes.first;
    else
        return 0;
}

uint32_t ParsedFrameSize::GetSecondFieldSize() const
{
    if (HaveSecondField())
        return mFieldSizes.second;
    else
        return 0;
}

uint32_t ParsedFrameSize::GetSize() const
{
    if (IsFields() && IsComplete()) {
        // A field coded picture pair
        int64_t size = (int64_t)mFieldSizes.first + mFieldSizes.second;
        BMX_CHECK(size < UINT32_MAX && size != ESSENCE_PARSER_NULL_OFFSET);
        return (uint32_t)size;
    } else if (!IsFields() && IsComplete()) {
        // A frame coded picture
        return mFieldSizes.first;
    } else {
        return 0;
    }
}

uint32_t ParsedFrameSize::GetFirstFieldOrFrameSize() const
{
    if (mIsFields)
        return GetFirstFieldSize();
    else
        return GetSize();
}

bool ParsedFrameSize::IsUnknown() const
{
    if (mIsFields)
        return mFieldSizes.first == ESSENCE_PARSER_NULL_OFFSET || mFieldSizes.second == ESSENCE_PARSER_NULL_OFFSET;
    else
        return mFieldSizes.first == ESSENCE_PARSER_NULL_OFFSET;
}

bool ParsedFrameSize::IsNull() const
{
    if (mIsFields)
        return mFieldSizes.first == 0 || mFieldSizes.second == 0;
    else
        return mFieldSizes.first == 0;
}

bool ParsedFrameSize::IsFrame() const
{
    return !mIsFields;
}

bool ParsedFrameSize::IsFields() const
{
    return mIsFields;
}

bool ParsedFrameSize::HaveFirstField() const
{
    return mIsFields && mFieldSizes.first > 0 && mFieldSizes.first != ESSENCE_PARSER_NULL_OFFSET;
}

bool ParsedFrameSize::HaveSecondField() const
{
    return mIsFields && mFieldSizes.second > 0 && mFieldSizes.second != ESSENCE_PARSER_NULL_OFFSET;
}

bool ParsedFrameSize::HaveFirstFieldOrFrame() const
{
    return mFieldSizes.first > 0 && mFieldSizes.first != ESSENCE_PARSER_NULL_OFFSET;
}

bool ParsedFrameSize::IsComplete() const
{
    if (mIsFields)
        return HaveFirstField() && HaveSecondField();
    else
        return HaveFirstFieldOrFrame();
}

void ParsedFrameSize::SetSize(uint32_t size)
{
    mIsFields = false;
    mFieldSizes = make_pair(size, ESSENCE_PARSER_NULL_FRAME_SIZE);
}

void ParsedFrameSize::SetFirstFieldSize(uint32_t size)
{
    BMX_CHECK(!HaveSecondField());
    mIsFields = true;
    mFieldSizes = make_pair(size, ESSENCE_PARSER_NULL_OFFSET);
}

void ParsedFrameSize::SetSecondFieldSize(uint32_t size)
{
    BMX_CHECK(HaveFirstField());
    mIsFields = true;
    mFieldSizes.second = size;
}

bool ParsedFrameSize::CompleteSize(uint32_t data_size)
{
    if (IsNull() || IsComplete() || data_size == 0)
        return false;

    if (mIsFields && HaveFirstField()) {
        if (GetFirstFieldSize() >= data_size)
            return false;

        SetSecondFieldSize(data_size - GetFirstFieldSize());
    } else {
        SetSize(data_size);
    }

    return true;
}

void ParsedFrameSize::Reset()
{
    mIsFields = true;
    mFieldSizes = make_pair(ESSENCE_PARSER_NULL_OFFSET, ESSENCE_PARSER_NULL_OFFSET);
}


uint32_t EssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    (void)data;
    (void)data_size;
    BMX_ASSERT(false);
    return 0;
}

ParsedFrameSize EssenceParser::ParseFrameSize2(const unsigned char *data, uint32_t data_size)
{
    uint32_t frame_size = ParseFrameSize(data, data_size);
    return ParsedFrameSize(frame_size);
}

void EssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    (void)data;
    (void)data_size;
    BMX_ASSERT(false);
}

ParsedFrameSize EssenceParser::ParseFrameInfo2(const unsigned char *data, ParsedFrameSize frame_size)
{
    ParseFrameInfo(data, frame_size.GetSize());
    return frame_size;
}