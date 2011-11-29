/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#include <cstdio>

#include <im/d10_mxf/D10Track.h>
#include <im/d10_mxf/D10File.h>
#include <im/d10_mxf/D10MPEGTrack.h>
#include <im/d10_mxf/D10PCMTrack.h>
#include <im/MXFUtils.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    D10EssenceType d10_essence_type;
    MXFDescriptorHelper::EssenceType mh_essence_type;
} EssenceTypeMap;

static const EssenceTypeMap ESSENCE_TYPE_MAP[] =
{
    {D10_MPEG_30,          MXFDescriptorHelper::D10_30},
    {D10_MPEG_40,          MXFDescriptorHelper::D10_40},
    {D10_MPEG_50,          MXFDescriptorHelper::D10_50},
    {D10_PCM,              MXFDescriptorHelper::WAVE_PCM},
};


typedef struct
{
    D10EssenceType essence_type;
    mxfRational sample_rate[10];
} D10SampleRateSupport;

static const D10SampleRateSupport D10_SAMPLE_RATE_SUPPORT[] =
{
    {D10_MPEG_30,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {D10_MPEG_40,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {D10_MPEG_50,    {{25, 1}, {30000, 1001}, {0, 0}}},
    {D10_PCM,        {{48000, 1}, {0, 0}}},
};



bool D10Track::IsSupported(D10EssenceType essence_type, mxfRational sample_rate)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(D10_SAMPLE_RATE_SUPPORT); i++) {
        if (essence_type == D10_SAMPLE_RATE_SUPPORT[i].essence_type) {
            size_t j = 0;
            while (D10_SAMPLE_RATE_SUPPORT[i].sample_rate[j].numerator) {
                if (sample_rate == D10_SAMPLE_RATE_SUPPORT[i].sample_rate[j])
                    return true;
                j++;
            }
        }
    }

    return false;
}

MXFDescriptorHelper::EssenceType D10Track::ConvertEssenceType(D10EssenceType d10_essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (ESSENCE_TYPE_MAP[i].d10_essence_type == d10_essence_type)
            return ESSENCE_TYPE_MAP[i].mh_essence_type;
    }

    return MXFDescriptorHelper::UNKNOWN_ESSENCE;
}

D10EssenceType D10Track::ConvertEssenceType(MXFDescriptorHelper::EssenceType mh_essence_type)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(ESSENCE_TYPE_MAP); i++) {
        if (ESSENCE_TYPE_MAP[i].mh_essence_type == mh_essence_type)
            return ESSENCE_TYPE_MAP[i].d10_essence_type;
    }

    return D10_UNKNOWN_ESSENCE;
}

D10Track* D10Track::Create(D10File *file, uint32_t track_index, mxfRational frame_rate, D10EssenceType essence_type)
{
    switch (essence_type)
    {
        case D10_MPEG_30:
        case D10_MPEG_40:
        case D10_MPEG_50:
            return new D10MPEGTrack(file, track_index, frame_rate, essence_type);
        case D10_PCM:
            return new D10PCMTrack(file, track_index, frame_rate, essence_type);
        case D10_UNKNOWN_ESSENCE:
            IM_ASSERT(false);
    }

    return 0;
}

D10Track::D10Track(D10File *file, uint32_t track_index, mxfRational frame_rate, D10EssenceType essence_type)
{
    mD10File = file;
    mTrackIndex = track_index;
    mFrameRate = frame_rate;
    mCPManager = file->GetContentPackageManager();
    mIsPicture = true;

    mEssenceType = essence_type;
    mDescriptorHelper = MXFDescriptorHelper::Create(ConvertEssenceType(essence_type));
    mDescriptorHelper->SetFlavour(MXFDescriptorHelper::SMPTE_377_2004_FLAVOUR);
    mDescriptorHelper->SetFrameWrapped(true);
    mDescriptorHelper->SetSampleRate(frame_rate);
}

D10Track::~D10Track()
{
    delete mDescriptorHelper;
}

void D10Track::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    mD10File->WriteSamples(mTrackIndex, data, size, num_samples);
}

uint32_t D10Track::GetSampleSize()
{
    return mDescriptorHelper->GetSampleSize();
}

void D10Track::WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    IM_ASSERT(data && size && num_samples);

    mCPManager->WriteSamples(mTrackIndex, data, size, num_samples);
}

