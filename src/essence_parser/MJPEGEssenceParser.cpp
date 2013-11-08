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

#include <bmx/essence_parser/MJPEGEssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



MJPEGEssenceParser::MJPEGEssenceParser(bool single_field)
{
    mSingleField = single_field;
    Reset();
}

MJPEGEssenceParser::~MJPEGEssenceParser()
{
}

uint32_t MJPEGEssenceParser::ParseFrameStart(const unsigned char *data, uint32_t data_size)
{
    BMX_CHECK(data_size != ESSENCE_PARSER_NULL_OFFSET);

    (void)data;
    return 0;
}

uint32_t MJPEGEssenceParser::ParseFrameSize(const unsigned char *data, uint32_t data_size)
{
    // states
    // 0 = search for 0xff
    // 1 = test for 0xd8 (start of image)
    // 2 = search for 0xff - start of marker
    // 3 = test for 0xd9 (end of image), else skip
    // 4 = skip marker segment data
    //
    // transitions
    // 0 -> 1 (data == 0xff)
    // 1 -> 0 (data != 0xd8 && data != 0xff)
    // 1 -> 2 (data == 0xd8)
    // 2 -> 3 (data == 0xff)
    // 3 -> 0 (data == 0xd9)
    // 3 -> 2 (data >= 0xd0 && data <= 0xd7 || data == 0x01 || data == 0x00)
    // 3 -> 4 (else and data != 0xff)

    while (mOffset < data_size) {
        switch (mState)
        {
            case 0:
                if (data[mOffset] == 0xff) {
                    mState = 1;
                } else {
                    // invalid image start
                    Reset();
                    return ESSENCE_PARSER_NULL_FRAME_SIZE;
                }
                break;
            case 1:
                if (data[mOffset] == 0xd8) // start of image
                    mState = 2;
                else if (data[mOffset] != 0xff) // 0xff is fill byte
                    mState = 0;
                break;
            case 2:
                if (data[mOffset] == 0xff)
                    mState = 3;
                break;
            case 3:
                if (data[mOffset] == 0xd9) // end of image
                {
                    uint32_t image_size = mOffset + 1;
                    if (mSingleField || mFieldCount == 1) {
                        Reset();
                        return image_size;
                    }
                    mFieldCount++;
                    mState = 0;
                }
                // 0xd0-0xd7 and 0x01 are empty markers and 0x00 is stuffed zero
                else if ((data[mOffset] >= 0xd0 && data[mOffset] <= 0xd7) ||
                         data[mOffset] == 0x01 ||
                         data[mOffset] == 0x00)
                {
                    mState = 2;
                }
                else if (data[mOffset] != 0xff) // 0xff is fill byte
                {
                    mState = 4;
                    // initializations for state 4
                    mHaveLenByte1 = false;
                    mHaveLenByte2 = false;
                    mSkipCount = 0;
                }
                break;
            case 4:
                if (!mHaveLenByte1) {
                    mHaveLenByte1 = true;
                    mSkipCount = data[mOffset] << 8;
                } else if (!mHaveLenByte2) {
                    mHaveLenByte2 = true;
                    mSkipCount += data[mOffset];
                    mSkipCount -= 1; // length includes the 2 length bytes, one subtracted here and one below
                }

                if (mHaveLenByte1 && mHaveLenByte2) {
                    mSkipCount--;
                    if (mSkipCount == 0)
                        mState = 2;
                }
                break;
            default:
                BMX_ASSERT(false); // won't get here
        }

        mOffset++;
    }

    return ESSENCE_PARSER_NULL_OFFSET;
}

void MJPEGEssenceParser::ParseFrameInfo(const unsigned char *data, uint32_t data_size)
{
    (void)data;
    (void)data_size;
}

void MJPEGEssenceParser::Reset()
{
    mOffset = 0;
    mState = 0;
    mHaveLenByte1 = false;
    mHaveLenByte2 = false;
    mSkipCount = 0;
    mFieldCount = 0;
}

