/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#include <bmx/writer_helper/D10WriterHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



D10WriterHelper::D10WriterHelper()
{
    mZeroBuffer.SetAllocBlockSize(8192);
    memset(mDataArray, 0, sizeof(mDataArray));
    mMaxSampleSize = 0;
    mOutputMaxSampleSize = false;
    mHaveInfoRemovePadding = false;
    mHaveInfoAddPadding = false;
}

D10WriterHelper::~D10WriterHelper()
{
}

void D10WriterHelper::SetMaxSampleSize(uint32_t size)
{
    mMaxSampleSize = size;
}

void D10WriterHelper::SetOutputMaxSampleSize(bool enable)
{
    mOutputMaxSampleSize = enable;
}

void D10WriterHelper::ProcessFrame(const unsigned char *data, uint32_t size,
                                   const CDataBuffer **data_array, uint32_t *array_size_out)
{
    uint32_t array_size;

    mDataArray[0].data = (unsigned char*)data;
    mDataArray[0].size = size;
    array_size = 1;

    if (mOutputMaxSampleSize && size < mMaxSampleSize) {
        if (!mHaveInfoAddPadding) {
            log_info("Adding padding bytes to D-10 frame\n");
            mHaveInfoAddPadding = true;
        }

        uint32_t zero_size = mZeroBuffer.GetSizeAvailable();
        if (zero_size < mMaxSampleSize - size) {
            mZeroBuffer.Reallocate(mMaxSampleSize - size);
            memset(&mZeroBuffer.GetBytesAvailable()[zero_size], 0,
                   mZeroBuffer.GetSizeAvailable() - zero_size);
        }

        mDataArray[1].data = mZeroBuffer.GetBytesAvailable();
        mDataArray[1].size = mMaxSampleSize - size;
        array_size++;
    } else if (size > mMaxSampleSize) {
        BMX_CHECK_M(check_excess_d10_padding(data, size, mMaxSampleSize),
                    ("Failed to remove D-10 frame padding bytes; found non-zero bytes"));
        if (!mHaveInfoRemovePadding) {
            log_info("Removing padding bytes from D-10 frame\n");
            mHaveInfoRemovePadding = true;
        }

        mDataArray[0].size = mMaxSampleSize;
    }

    *array_size_out = array_size;
    *data_array = mDataArray;
}

