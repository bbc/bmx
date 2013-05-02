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

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>

#include <bmx/essence_parser/KLVEssenceSource.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace bmx;
using namespace std;



KLVEssenceSource::KLVEssenceSource(EssenceSource *child_source)
{
    mChildSource = child_source;
    mKey = g_Null_Key;
    mTrackNum = 0;
    mState = READ_KL_STATE;
    mValueLen = 0;
}

KLVEssenceSource::KLVEssenceSource(EssenceSource *child_source, const mxfKey *key)
{
    mChildSource = child_source;
    mKey = *key;
    mTrackNum = 0;
    mState = READ_KL_STATE;
    mValueLen = 0;
}

KLVEssenceSource::KLVEssenceSource(EssenceSource *child_source, uint32_t track_num)
{
    mChildSource = child_source;
    mKey = g_Null_Key;
    mTrackNum = track_num;
    mState = READ_KL_STATE;
    mValueLen = 0;
}

KLVEssenceSource::~KLVEssenceSource()
{
    delete mChildSource;
}

uint32_t KLVEssenceSource::Read(unsigned char *data, uint32_t size)
{
    mxfKey key;
    unsigned char len_buffer[9];
    uint32_t total_read = 0;

    while (total_read < size) {
        if (mState == READ_KL_STATE) {
            if (mChildSource->Read(&key.octet0, mxfKey_extlen) != mxfKey_extlen)
                break;
            if (mChildSource->Read(len_buffer, 1) != 1)
                break;
            mValueLen = 0;
            if (len_buffer[0] < 0x80) {
                mValueLen = len_buffer[0];
            } else {
                uint8_t i;
                uint8_t len_size = len_buffer[0] & 0x7f;
                BMX_CHECK(len_size <= 8);
                if (mChildSource->Read(&len_buffer[1], len_size) != len_size)
                    break;
                for (i = 0; i < len_size; i++) {
                    mValueLen <<= 8;
                    mValueLen |= len_buffer[i + 1];
                }
                if (mValueLen > INT64_MAX) {
                    log_error("Unsupported KLV length %"PRIu64" in essence source\n", mValueLen);
                    break;
                }
            }
            if (mTrackNum) {
                if ((mxf_is_gc_essence_element(&key) || mxf_avid_is_essence_element(&key)) &&
                    mxf_get_track_number(&key) == mTrackNum)
                {
                    mState = READ_V_STATE;
                }
            } else if (mKey == key) {
                mState = READ_V_STATE;
            } else if (mKey == g_Null_Key) {
                mKey = key;
                mState = READ_V_STATE;
            }
            if (mState != READ_V_STATE && !mChildSource->Skip(mValueLen))
                break;
        } else if (mState == READ_V_STATE) {
            uint32_t num_read = 0;
            uint32_t next_read = size - total_read;
            if (next_read > mValueLen)
                next_read = (uint32_t)mValueLen;
            if (data)
                num_read = mChildSource->Read(&data[total_read], next_read);
            else if (mChildSource->Skip(next_read))
                num_read = next_read;
            total_read += num_read;
            mValueLen -= num_read;
            if (num_read < next_read)
                break;
            if (mValueLen == 0)
                mState = READ_KL_STATE;
        } else {
            break;
        }
    }

    if (total_read < size)
        mState = READ_END_STATE;

    return total_read;
}

bool KLVEssenceSource::SeekStart()
{
    bool res = mChildSource->SeekStart();
    if (res)
        mState = READ_KL_STATE;
    else
        mState = READ_END_STATE;
    return res;
}

bool KLVEssenceSource::Skip(int64_t offset)
{
    BMX_CHECK(offset >= 0); // a negative offset is not supported

    int64_t total_read = 0;
    uint32_t num_read, next_read;
    while (total_read < offset) {
        if (offset - total_read > UINT32_MAX)
            next_read = UINT32_MAX;
        else
            next_read = (uint32_t)(offset - total_read);
        num_read = Read(0, next_read);
        total_read += num_read;
        if (num_read < next_read)
            break;
    }

    return !mChildSource->HaveError();
}

bool KLVEssenceSource::HaveError() const
{
    return mChildSource->HaveError();
}

int KLVEssenceSource::GetErrno() const
{
    return mChildSource->GetErrno();
}

string KLVEssenceSource::GetStrError() const
{
    return mChildSource->GetStrError();
}

