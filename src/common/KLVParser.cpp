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

#include <cstring>

#include <bmx/KLVParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



KLVParser::KLVParser(KLVParserListener *listener)
{
    mListener = listener;

    memset(mKLBuffer, 0, sizeof(mKLBuffer));
    mNumKLBytes = 0;
    mLLen = 0;
    mLen = 0;
    mVRemainder = 0;
}

KLVParser::~KLVParser()
{
}

bool KLVParser::Parse(const unsigned char *bytes_in, uint32_t size_in, uint32_t *parsed_size)
{
    const unsigned char *bytes = bytes_in;
    uint32_t size = size_in;

    while (size > 0) {
        if (mVRemainder == 0) {
            // key-length
            if (mNumKLBytes < 16) {
                // key
                uint32_t k_size = 16 - mNumKLBytes;
                if (k_size > size)
                    k_size = size;
                memcpy(mKLBuffer, bytes, k_size);
                mNumKLBytes += k_size;
                size -= k_size;
                bytes += k_size;
            } else {
                // length
                if (mLLen == 0) {
                    mLLen = 1;
                    if (*bytes >= 0x80)
                        mLLen += (*bytes) & 0x7f;
                    if (mLLen > 9) {
                        log_error("Invalid llen %u\n", mLLen);
                        *parsed_size = size_in - size;
                        return false;
                    }
                    mKLBuffer[mNumKLBytes] = *bytes;
                    mNumKLBytes++;
                    size--;
                    bytes++;
                }

                if (mLLen > 1) {
                    uint32_t l_bytes = mLLen - (mNumKLBytes - 16);
                    if (l_bytes > size)
                        l_bytes = size;
                    memcpy(mKLBuffer + mNumKLBytes, bytes, l_bytes);
                    mNumKLBytes += l_bytes;
                    size -= l_bytes;
                    bytes += l_bytes;
                }

                if (mNumKLBytes == 16 + mLLen) {
                    mLen = 0;
                    if (mLLen == 1) {
                        mLen = mKLBuffer[mNumKLBytes - 1];
                    } else {
                        uint32_t i;
                        for (i = 1; i < mLLen; i++)
                            mLen = (mLen << 8) | mKLBuffer[16 + i];
                    }
                    if (mListener)
                        mListener->ReadKLEvent(mKLBuffer, mLen, mLLen);

                    mNumKLBytes = 0;
                    mLLen = 0;
                    mVRemainder = mLen;
                }
            }

        } else {
            // value

            uint32_t v_size = size;
            if (v_size > mVRemainder)
                v_size = (uint32_t)mVRemainder;
            if (mListener)
                mListener->ReadVEvent(bytes, v_size, mLen, mLen - mVRemainder);

            mVRemainder -= v_size;
            size -= v_size;
            bytes += v_size;
        }
    }

    *parsed_size = size_in - size;
    return true;
}

