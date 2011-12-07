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

#include <im/as11/AS11Track.h>
#include <im/mxf_op1a/OP1APictureTrack.h>
#include <im/mxf_op1a/OP1ADVTrack.h>
#include <im/mxf_op1a/OP1AUncTrack.h>
#include <im/mxf_op1a/OP1AD10Track.h>
#include <im/mxf_op1a/OP1AMPEG2LGTrack.h>
#include <im/mxf_op1a/OP1AAVCITrack.h>
#include <im/mxf_op1a/OP1APCMTrack.h>
#include <im/d10_mxf/D10MPEGTrack.h>
#include <im/d10_mxf/D10PCMTrack.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;



bool AS11Track::IsSupported(AS11ClipType clip_type, EssenceType essence_type, Rational sample_rate)
{
    switch (clip_type)
    {
        case AS11_OP1A_CLIP_TYPE:
            return OP1ATrack::IsSupported(essence_type, sample_rate);
        case AS11_D10_CLIP_TYPE:
            return D10Track::IsSupported(essence_type, sample_rate);
        case AS11_UNKNOWN_CLIP_TYPE:
            break;
    }

    return false;
}

AS11Track::AS11Track(EssenceType essence_type, OP1ATrack *track)
{
    mClipType = AS11_OP1A_CLIP_TYPE;
    mEssenceType = essence_type;
    mOP1ATrack = track;
    mD10Track = 0;
}

AS11Track::AS11Track(EssenceType essence_type, D10Track *track)
{
    mClipType = AS11_D10_CLIP_TYPE;
    mEssenceType = essence_type;
    mOP1ATrack = 0;
    mD10Track = track;
}

AS11Track::~AS11Track()
{
}

void AS11Track::SetOutputTrackNumber(uint32_t track_number)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1ATrack->SetOutputTrackNumber(track_number);
            break;
        case AS11_D10_CLIP_TYPE:
            mD10Track->SetOutputTrackNumber(track_number);
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetAspectRatio(Rational aspect_ratio)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APictureTrack *pict_track = dynamic_cast<OP1APictureTrack*>(mOP1ATrack);
            if (pict_track)
                pict_track->SetAspectRatio(aspect_ratio);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10MPEGTrack *pict_track = dynamic_cast<D10MPEGTrack*>(mD10Track);
            if (pict_track)
                pict_track->SetAspectRatio(aspect_ratio);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetComponentDepth(uint32_t depth)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1ADVTrack *dv_track = dynamic_cast<OP1ADVTrack*>(mOP1ATrack);
            OP1AUncTrack *unc_track = dynamic_cast<OP1AUncTrack*>(mOP1ATrack);
            if (dv_track)
                dv_track->SetComponentDepth(depth);
            else if (unc_track)
                unc_track->SetComponentDepth(depth);
            break;
        }
        case AS11_D10_CLIP_TYPE:
            log_warn("Setting component depth not supported in D10 clip\n");
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetSampleSize(uint32_t size)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1AD10Track *d10_track = dynamic_cast<OP1AD10Track*>(mOP1ATrack);
            if (d10_track)
                d10_track->SetSampleSize(size);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10MPEGTrack *d10_track = dynamic_cast<D10MPEGTrack*>(mD10Track);
            if (d10_track)
                d10_track->SetSampleSize(size);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetAVCIMode(OP1AAVCIMode mode)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1AAVCITrack *avci_track = dynamic_cast<OP1AAVCITrack*>(mOP1ATrack);
            if (avci_track)
                avci_track->SetMode(mode);
            break;
        }
        case AS11_D10_CLIP_TYPE:
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetAVCIHeader(const unsigned char *data, uint32_t size)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1AAVCITrack *avci_track = dynamic_cast<OP1AAVCITrack*>(mOP1ATrack);
            if (avci_track)
                avci_track->SetHeader(data, size);
            break;
        }
        case AS11_D10_CLIP_TYPE:
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetAFD(uint8_t afd)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APictureTrack *picture_track = dynamic_cast<OP1APictureTrack*>(mOP1ATrack);
            if (picture_track)
                picture_track->SetAFD(afd);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10MPEGTrack *mpeg_track = dynamic_cast<D10MPEGTrack*>(mD10Track);
            if (mpeg_track)
                mpeg_track->SetAFD(afd);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetSamplingRate(Rational sampling_rate)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetSamplingRate(sampling_rate);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetSamplingRate(sampling_rate);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetQuantizationBits(uint32_t bits)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetQuantizationBits(bits);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetQuantizationBits(bits);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetChannelCount(uint32_t count)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetChannelCount(count);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetChannelCount(count);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetLocked(bool locked)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetLocked(locked);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetLocked(locked);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetAudioRefLevel(int8_t level)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetAudioRefLevel(level);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetAudioRefLevel(level);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetDialNorm(int8_t dial_norm)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetDialNorm(dial_norm);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetDialNorm(dial_norm);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

void AS11Track::SetSequenceOffset(uint8_t offset)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                pcm_track->SetSequenceOffset(offset);
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                pcm_track->SetSequenceOffset(offset);
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

vector<uint32_t> AS11Track::GetSampleSequence() const
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                return pcm_track->GetSampleSequence();
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                return pcm_track->GetSampleSequence();
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return vector<uint32_t>(1);
}

uint8_t AS11Track::GetSequenceOffset() const
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                return pcm_track->GetSequenceOffset();
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                return pcm_track->GetSequenceOffset();
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

vector<uint32_t> AS11Track::GetShiftedSampleSequence() const
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1APCMTrack *pcm_track = dynamic_cast<OP1APCMTrack*>(mOP1ATrack);
            if (pcm_track)
                return pcm_track->GetShiftedSampleSequence();
            break;
        }
        case AS11_D10_CLIP_TYPE:
        {
            D10PCMTrack *pcm_track = dynamic_cast<D10PCMTrack*>(mD10Track);
            if (pcm_track)
                return pcm_track->GetShiftedSampleSequence();
            break;
        }
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return vector<uint32_t>(1);
}

void AS11Track::WriteSamples(const unsigned char *data, uint32_t size, uint32_t num_samples)
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
            mOP1ATrack->WriteSamples(data, size, num_samples);
            break;
        case AS11_D10_CLIP_TYPE:
            mD10Track->WriteSamples(data, size, num_samples);
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }
}

bool AS11Track::IsPicture() const
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
            return mOP1ATrack->IsPicture();
        case AS11_D10_CLIP_TYPE:
            return mD10Track->IsPicture();
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return false;
}

uint32_t AS11Track::GetSampleSize()
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
            return mOP1ATrack->GetSampleSize();
        case AS11_D10_CLIP_TYPE:
            return mD10Track->GetSampleSize();
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

uint32_t AS11Track::GetAVCISampleWithoutHeaderSize()
{
    switch (mClipType)
    {
        case AS11_OP1A_CLIP_TYPE:
        {
            OP1AAVCITrack *avci_track = dynamic_cast<OP1AAVCITrack*>(mOP1ATrack);
            if (avci_track)
                return avci_track->GetSampleWithoutHeaderSize();
            break;
        }
        case AS11_D10_CLIP_TYPE:
            break;
        case AS11_UNKNOWN_CLIP_TYPE:
            IM_ASSERT(false);
            break;
    }

    return 0;
}

