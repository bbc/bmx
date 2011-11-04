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

#include <im/mxf_reader/MXFReader.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;


typedef struct
{
    mxfRational reader_sample_rate;
    mxfRational target_sample_rate;
    uint32_t sample_sequence[11];
} SampleSequence;

static const SampleSequence SAMPLE_SEQUENCES[] =
{
    {{30000, 1001}, {48000,1}, {1602, 1601, 1602, 1601, 1602, 0, 0, 0, 0, 0}},
    {{60000, 1001}, {48000,1}, {801, 801, 801, 800, 801, 801, 801, 800, 801, 801, 0}},
    {{24000, 1001}, {48000,1}, {2002, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

#define SAMPLE_SEQUENCES_SIZE   (sizeof(SAMPLE_SEQUENCES) / sizeof(SampleSequence))



MXFReader::MXFReader()
{
    mSampleRate = ZERO_RATIONAL;
    mDuration = 0;
    mMaterialStartTimecode = 0;
    mFileSourceStartTimecode = 0;
    mPhysicalSourceStartTimecode = 0;
    mMaterialPackageUID = g_Null_UMID;
}

MXFReader::~MXFReader()
{
    delete mMaterialStartTimecode;
    delete mFileSourceStartTimecode;
    delete mPhysicalSourceStartTimecode;
}

Timecode MXFReader::GetMaterialTimecode(int64_t position) const
{
    if (!HaveMaterialTimecode())
        return Timecode(get_rounded_tc_base(mSampleRate), false);

    return CreateTimecode(mMaterialStartTimecode, position);
}

Timecode MXFReader::GetFileSourceTimecode(int64_t position) const
{
    if (!HaveFileSourceTimecode())
        return Timecode(get_rounded_tc_base(mSampleRate), false);

    return CreateTimecode(mFileSourceStartTimecode, position);
}

Timecode MXFReader::GetPhysicalSourceTimecode(int64_t position) const
{
    if (!HavePhysicalSourceTimecode())
        return Timecode(get_rounded_tc_base(mSampleRate), false);

    return CreateTimecode(mPhysicalSourceStartTimecode, position);
}

bool MXFReader::HavePlayoutTimecode() const
{
    return HaveMaterialTimecode() || HaveFileSourceTimecode() || HavePhysicalSourceTimecode();
}

Timecode MXFReader::GetPlayoutTimecode(int64_t position) const
{
    if (HaveMaterialTimecode())
        return GetMaterialTimecode(position);
    else if (HaveFileSourceTimecode())
        return GetFileSourceTimecode(position);
    else if (HavePhysicalSourceTimecode())
        return GetPhysicalSourceTimecode(position);

    return Timecode(get_rounded_tc_base(mSampleRate), false);
}

bool MXFReader::HaveSourceTimecode() const
{
    return HaveFileSourceTimecode() || HavePhysicalSourceTimecode();
}

Timecode MXFReader::GetSourceTimecode(int64_t position) const
{
    if (HaveFileSourceTimecode())
        return GetFileSourceTimecode(position);
    else if (HavePhysicalSourceTimecode())
        return GetPhysicalSourceTimecode(position);

    return Timecode(get_rounded_tc_base(mSampleRate), false);
}

vector<uint32_t> MXFReader::GetSampleSequence(mxfRational target_sample_rate) const
{
    vector<uint32_t> sample_sequence;

    if (mSampleRate == target_sample_rate) {
        // 1 track sample equals 1 group sample
        sample_sequence.push_back(1);
    } else {
        int64_t num_samples = (int64_t)(target_sample_rate.numerator * mSampleRate.denominator) /
                                    (int64_t)(target_sample_rate.denominator * mSampleRate.numerator);
        int64_t remainder = (int64_t)(target_sample_rate.numerator * mSampleRate.denominator) %
                                    (int64_t)(target_sample_rate.denominator * mSampleRate.numerator);
        if (num_samples > 0) {
            if (remainder == 0) {
                // fixed integer number of reader samples for each clip sample
                sample_sequence.push_back(num_samples);
            } else {
                // try well known sample sequences
                size_t i;
                for (i = 0; i < SAMPLE_SEQUENCES_SIZE; i++) {
                    if (mSampleRate == SAMPLE_SEQUENCES[i].reader_sample_rate &&
                        target_sample_rate == SAMPLE_SEQUENCES[i].target_sample_rate)
                    {
                        size_t j = 0;
                        while (SAMPLE_SEQUENCES[i].sample_sequence[j] != 0) {
                            sample_sequence.push_back(SAMPLE_SEQUENCES[i].sample_sequence[j]);
                            j++;
                        }
                        break;
                    }
                }

                // if sample_sequence is empty then sample sequence is unknown
            }
        }
        // else input sample rate > mSampleRate and therefore input sample doesn't fit into sequence's sample
    }

    return sample_sequence;
}

Timecode MXFReader::CreateTimecode(const Timecode *start_timecode, int64_t position) const
{
    int64_t offset = position;
    if (position == CURRENT_POSITION_VALUE)
        offset = GetPosition();

    Timecode timecode(*start_timecode);
    timecode.AddOffset(offset, mSampleRate);

    return timecode;
}

