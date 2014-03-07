/*
 * Copyright (C) 2014, British Broadcasting Corporation
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

#include <bmx/essence_parser/MPEG2AspectRatioFilter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define PICTURE_START_CODE      0x00000100
#define SEQUENCE_HEADER_CODE    0x000001b3
#define SEQUENCE_EXT_CODE       0x000001b5
#define GROUP_HEADER_CODE       0x000001b8


typedef struct
{
    uint8_t info;
    Rational aspect_ratio;
} AspectRatioMap;

static const AspectRatioMap ASPECT_RATIO_MAP[] =
{
    {0x02,  { 4, 3}},
    {0x03,  {16, 9}},
};



MPEG2AspectRatioFilter::MPEG2AspectRatioFilter()
{
    SetAspectRatio(ASPECT_RATIO_16_9);
}

MPEG2AspectRatioFilter::MPEG2AspectRatioFilter(Rational aspect_ratio)
{
    SetAspectRatio(aspect_ratio);
}

MPEG2AspectRatioFilter::~MPEG2AspectRatioFilter()
{
}

void MPEG2AspectRatioFilter::SetAspectRatio(Rational aspect_ratio)
{
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(ASPECT_RATIO_MAP); i++) {
        if (ASPECT_RATIO_MAP[i].aspect_ratio == aspect_ratio) {
            mAspectRatioInfo = ASPECT_RATIO_MAP[i].info;
            break;
        }
    }
    if (i >= BMX_ARRAY_SIZE(ASPECT_RATIO_MAP)) {
        BMX_EXCEPTION(("Aspect ratio %u/%u not supported in MPEG-2 aspect ratio filter",
                       aspect_ratio.numerator, aspect_ratio.denominator));
    }
}

void MPEG2AspectRatioFilter::Filter(const unsigned char *data_in, uint32_t size_in,
                                    unsigned char **data_out, uint32_t *size_out)
{
    unsigned char *new_data = 0;
    try
    {
        new_data = new unsigned char[size_in];
        memcpy(new_data, data_in, size_in);
        Filter(new_data, size_in);

        *data_out = new_data;
        *size_out = size_in;
    }
    catch (...)
    {
        delete [] new_data;
        throw;
    }
}

void MPEG2AspectRatioFilter::Filter(unsigned char *data, uint32_t size)
{
    uint32_t state = 0xffffffff;
    uint32_t offset = 0;
    while (offset < size) {
        state = (state << 8) | data[offset];
        if (state == SEQUENCE_HEADER_CODE) {
            data[(offset - 3) + 7] = (data[(offset - 3) + 7] & 0x0f) | (mAspectRatioInfo << 4);
            break;
        } else if (state == PICTURE_START_CODE) {
            break;
        }

        offset++;
    }
}
