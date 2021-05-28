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

#include <libMXF++/MXF.h>

#include <bmx/writer_helper/D10WriterHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const mxfKey BASE_D10_ELEMENT_KEY = MXF_D10_PICTURE_EE_K(0x00);


D10WriterHelper::D10WriterHelper()
{
    mZeroBuffer.SetAllocBlockSize(8192);
    memset(mDataArray, 0, sizeof(mDataArray));
    mMaxSampleSize = 0;
    mOutputMaxSampleSize = false;
    mHaveInfoRemovePadding = false;
    mHaveInfoAddPadding = false;
    mHaveWarnKLPrefixRemoved = false;
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

    // Remove D-10 MXF KL from the start of the video frame essence.
    // Quicktime files sometimes include the D-10 MXF KL prefix in the video frame essence.
    uint32_t d10_kl_prefix_len = CheckForD10KL(mDataArray[0].data, mDataArray[0].size);
    if (d10_kl_prefix_len > 0) {
        mDataArray[0].data += d10_kl_prefix_len;
        mDataArray[0].size -= d10_kl_prefix_len;

        if (!mHaveWarnKLPrefixRemoved) {
            log_warn("Removing D-10 MXF KL prefix from the start of the D-10 frame essence\n");
            mHaveWarnKLPrefixRemoved = true;
        }
    }

    if (mOutputMaxSampleSize && mDataArray[0].size < mMaxSampleSize) {
        if (!mHaveInfoAddPadding) {
            log_info("Adding padding bytes to D-10 frame\n");
            mHaveInfoAddPadding = true;
        }

        uint32_t zero_size = mZeroBuffer.GetSizeAvailable();
        if (zero_size < mMaxSampleSize - mDataArray[0].size) {
            mZeroBuffer.Reallocate(mMaxSampleSize - mDataArray[0].size);
            memset(&mZeroBuffer.GetBytesAvailable()[zero_size], 0,
                   mZeroBuffer.GetSizeAvailable() - zero_size);
        }

        mDataArray[1].data = mZeroBuffer.GetBytesAvailable();
        mDataArray[1].size = mMaxSampleSize - mDataArray[0].size;
        array_size++;
    } else if (mDataArray[0].size > mMaxSampleSize) {
        BMX_CHECK_M(check_excess_d10_padding(mDataArray[0].data, mDataArray[0].size, mMaxSampleSize),
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

uint32_t D10WriterHelper::CheckForD10KL(const unsigned char *data, uint32_t size)
{
    // Check whether the data starts with a D-10 MXF KL and if it does return the KL length
    if (size >= 17) {
        mxfKey prefix;
        mxf_get_ul(data, static_cast<mxfUL*>(&prefix));
        if (mxf_equals_key_prefix(&prefix, &BASE_D10_ELEMENT_KEY, 15)) {
            uint32_t llen = 1;
            if ((data[16] & 0x80))
                llen += (data[16] & 0x7f);

            if (llen <= 9 && size >= 16 + llen)
                return 16 + llen;
        }
    }

    return 0;
}
