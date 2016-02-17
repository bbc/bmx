/*
 * Copyright (C) 2016, British Broadcasting Corporation
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

#include <bmx/as10/AS10RDD9Validator.h>
#include <bmx/rdd9_mxf/RDD9File.h>
#include <bmx/rdd9_mxf/RDD9PCMTrack.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


AS10RDD9Validator::AS10RDD9Validator(AS10Shim shim, bool loose_checks)
{
    mShim = shim;
    mLooseChecks = loose_checks;
}

AS10RDD9Validator::~AS10RDD9Validator()
{
}

void AS10RDD9Validator::PrepareHeaderMetadata()
{
    uint32_t bps = (mShim == AS10_HIGH_HD_2014 ? 24 : 16);
    uint32_t channel_count = 0;
    uint32_t i;
    for (i = 0; i < mRDD9File->GetNumTracks(); i++) {
        RDD9PCMTrack *pcm_track = dynamic_cast<RDD9PCMTrack*>(mRDD9File->GetTrack(i));
        if (!pcm_track)
            continue;

        if (pcm_track->GetSamplingRate() != SAMPLING_RATE_48K || bps != pcm_track->GetQuantizationBits()) {
            log_error("sampling rate %d/%d, bits per sample %u, is not supported by AS-10 shim '%s'\n",
                      pcm_track->GetSamplingRate().numerator, pcm_track->GetSamplingRate().denominator,
                      pcm_track->GetQuantizationBits(), get_as10_shim_name(mShim));
            if (!mLooseChecks)
                BMX_EXCEPTION(("shim spec violation(s), incorrect essence"));
        }

        channel_count += pcm_track->GetChannelCount();
    }

    if (mShim != AS10_UNKNOWN_SHIM) {
        uint32_t required_channel_count = 8;
        if (mShim == AS10_JVC_HD_35_VBR_2012 || mShim == AS10_JVC_HD_25_CBR_2012)
            required_channel_count = 2;
        else if (mShim == AS10_CNN_HD_2012)
            required_channel_count = 4;

        if (channel_count != required_channel_count) {
            log_error("%u PCM channels are required for AS-10 shim '%s', found %u channels\n",
                      required_channel_count, get_as10_shim_name(mShim), channel_count);
            if (!mLooseChecks)
                BMX_EXCEPTION(("shim spec violation(s), incorrect essence"));
        }
    } else {
        if (channel_count != 2 && channel_count != 4 && channel_count != 8) {
            log_error("incorrect PCM channel count for AS-10, found %u channels\n", channel_count);
            if (!mLooseChecks)
                BMX_EXCEPTION(("shim spec violation(s), incorrect essence"));
        }
    }
}
