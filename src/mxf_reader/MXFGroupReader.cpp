/*
 * Copyright (C) 2011  British Broadcasting Corporation.
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>

#include <im/mxf_reader/MXFGroupReader.h>
#include <im/mxf_reader/MXFFileReader.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    mxfRational group_sample_rate;
    mxfRational member_sample_rate;
    uint32_t sample_sequence[11];
} SampleSequence;

static const SampleSequence SAMPLE_SEQUENCES[] =
{
    {{30000, 1001}, {48000,1}, {1602, 1601, 1602, 1601, 1602, 0, 0, 0, 0, 0}},
    {{60000, 1001}, {48000,1}, {801, 801, 801, 800, 801, 801, 801, 800, 801, 801, 0}},
    {{24000, 1001}, {48000,1}, {2002, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

#define SAMPLE_SEQUENCES_SIZE   (sizeof(SAMPLE_SEQUENCES) / sizeof(SampleSequence))


typedef struct
{
    size_t index;
    MXFTrackReader *track_reader;
} GroupTrackReader;



static bool compare_group_track_reader(const GroupTrackReader &left_reader, const GroupTrackReader &right_reader)
{
    const MXFTrackInfo *left_info = left_reader.track_reader->GetTrackInfo();
    const MXFTrackInfo *right_info = right_reader.track_reader->GetTrackInfo();

    // material package
    if (left_info->material_package_uid != right_info->material_package_uid)
        return left_reader.index < right_reader.index;

    // data kind
    if (left_info->is_picture != right_info->is_picture)
        return left_info->is_picture;

    // track number
    if (left_info->material_track_number != 0) {
        return right_info->material_track_number == 0 ||
               left_info->material_track_number < right_info->material_track_number;
    }
    if (right_info->material_track_number != 0)
        return false;

    // track id
    if (left_info->material_track_id != 0) {
        return right_info->material_track_id == 0 ||
               left_info->material_track_id < right_info->material_track_id;
    }
    return right_info->material_track_id == 0;
}



MXFGroupReader::MXFGroupReader()
: MXFReader()
{
    mReadStartPosition = 0;
    mReadEndPosition = 0;
}

MXFGroupReader::~MXFGroupReader()
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        delete mReaders[i];
}

void MXFGroupReader::AddReader(MXFReader *reader)
{
    mReaders.push_back(reader);
}

bool MXFGroupReader::Finalize()
{
    if (mReaders.empty()) {
        log_warn("Group reader has no members\n");
        return false;
    }

    // the lowest input sample rate is the group sample rate
    float lowest_sample_rate = 1000000.0;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        float member_sample_rate = mReaders[i]->GetSampleRate().numerator /
                                    (float)mReaders[i]->GetSampleRate().denominator;
        if (member_sample_rate < lowest_sample_rate) {
            mSampleRate = mReaders[i]->GetSampleRate();
            lowest_sample_rate = member_sample_rate;
        }
    }
    IM_CHECK(mSampleRate.numerator != 0);


    // create temporary group track readers and sort according to material package, data kind, track number and track id
    vector<GroupTrackReader> group_track_readers;
    GroupTrackReader group_track_reader;
    size_t j;
    for (i = 0; i < mReaders.size(); i++) {
        for (j = 0; j < mReaders[i]->GetNumTrackReaders(); j++) {
            group_track_reader.index = i;
            group_track_reader.track_reader = mReaders[i]->GetTrackReader(j);
            group_track_readers.push_back(group_track_reader);
        }
    }
    stable_sort(group_track_readers.begin(), group_track_readers.end(), compare_group_track_reader);

    // create track readers from sorted list
    for (i = 0; i < group_track_readers.size(); i++)
        mTrackReaders.push_back(group_track_readers[i].track_reader);


    // extract the sample sequences for each reader. The samples sequences determine how many member samples
    // are read for each group sample. They are also used for converting position and durations
    for (i = 0; i < mReaders.size(); i++) {
        vector<uint32_t> sample_sequence = GetSampleSequence(i);
        if (sample_sequence.empty()) {
            mxfRational member_sample_rate = mReaders[i]->GetSampleRate();
            log_error("Incompatible group sample rate (%d/%d) and member sample rate (%d/%d)\n",
                      mSampleRate.numerator, mSampleRate.denominator,
                      member_sample_rate.numerator, member_sample_rate.denominator);
            return false;
        }

        mSampleSequences.push_back(sample_sequence);

        int64_t sequence_size = 0;
        size_t j;
        for (j = 0; j < sample_sequence.size(); j++)
            sequence_size += sample_sequence[j];
        mSampleSequenceSizes.push_back(sequence_size);
    }


    // group duration is the minimum member duration
    mDuration = -1;
    for (i = 0; i < mReaders.size(); i++) {
        int64_t member_duration = ConvertMemberDuration(mReaders[i]->GetDuration(), i);

        if (mDuration < 0 || member_duration < mDuration)
            mDuration = member_duration;
    }


    // set default group read limits
    SetReadLimits();

    return true;
}

void MXFGroupReader::SetReadLimits()
{
    SetReadLimits(GetMaxPrecharge(0, true), mDuration + GetMaxRollout(mDuration - 1, true), true);
}

void MXFGroupReader::SetReadLimits(int64_t start_position, int64_t end_position, bool seek_start_position)
{
    mReadStartPosition = start_position;
    mReadEndPosition = end_position;

    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        mReaders[i]->SetReadLimits(ConvertGroupPosition(start_position, i),
                                   ConvertGroupPosition(end_position, i), false);
    }

    if (seek_start_position)
        Seek(start_position);
}

uint32_t MXFGroupReader::Read(uint32_t num_samples, int64_t frame_position_in)
{
    int64_t current_position = GetPosition();

    // ensure members are in sync
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        if (mReaders[i]->GetPosition() != ConvertGroupPosition(current_position, i))
            mReaders[i]->Seek(ConvertGroupPosition(current_position, i));
    }

    int64_t frame_position = frame_position_in;
    if (frame_position_in == CURRENT_POSITION_VALUE)
        frame_position = current_position;

    uint32_t max_read_num_samples = 0;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        uint32_t member_num_samples = (uint32_t)ConvertMemberDuration(
            mReaders[i]->Read((uint32_t)ConvertGroupDuration(num_samples, i), frame_position), i);
        if (member_num_samples > max_read_num_samples)
            max_read_num_samples = member_num_samples;
    }

    return max_read_num_samples;
}

void MXFGroupReader::Seek(int64_t position)
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        mReaders[i]->Seek(ConvertGroupPosition(position, i));
    }
}

int64_t MXFGroupReader::GetPosition()
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        return ConvertMemberPosition(mReaders[i]->GetPosition(), i);
    }

    return 0;
}

int16_t MXFGroupReader::GetMaxPrecharge(int64_t position, bool limit_to_available)
{
    int16_t max_precharge = 0;

    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int16_t precharge = mReaders[i]->GetMaxPrecharge(ConvertGroupPosition(position, i), limit_to_available);
        if (precharge < max_precharge) {
            IM_CHECK_M(mReaders[i]->GetSampleRate() == mSampleRate,
                       ("Currently only support precharge in group members if "
                        "member sample rate equals group sample rate"));
            max_precharge = precharge;
        }
    }

    return max_precharge;
}

int16_t MXFGroupReader::GetMaxRollout(int64_t position, bool limit_to_available)
{
    int16_t max_rollout = 0;

    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int16_t rollout = mReaders[i]->GetMaxRollout(ConvertGroupPosition(position, i), limit_to_available);
        if (rollout > max_rollout) {
            IM_CHECK_M(mReaders[i]->GetSampleRate() == mSampleRate,
                       ("Currently only support rollout in group members if "
                        "member sample rate equals group sample rate"));
            max_rollout = rollout;
        }
    }

    return max_rollout;
}

bool MXFGroupReader::HaveFixedLeadFillerOffset()
{
    int64_t fixed_offset = -1;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        if (!mReaders[i]->HaveFixedLeadFillerOffset())
            return false;

        int64_t reader_fixed_offset = ConvertMemberPosition(mReaders[i]->GetFixedLeadFillerOffset(), i);
        if (fixed_offset == -1)
            fixed_offset = reader_fixed_offset;
        else if (fixed_offset != reader_fixed_offset)
            return false;
    }

    return true;
}

int64_t MXFGroupReader::GetFixedLeadFillerOffset()
{
    int64_t fixed_offset = -1;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        if (!mReaders[i]->HaveFixedLeadFillerOffset())
            return 0;

        int64_t reader_fixed_offset = ConvertMemberPosition(mReaders[i]->GetFixedLeadFillerOffset(), i);
        if (fixed_offset == -1)
            fixed_offset = reader_fixed_offset;
        else if (fixed_offset != reader_fixed_offset)
            return 0;
    }

    return fixed_offset >= 0 ? fixed_offset : 0;
}

MXFTrackReader* MXFGroupReader::GetTrackReader(size_t track_index) const
{
    IM_CHECK(track_index < mTrackReaders.size());
    return mTrackReaders[track_index];
}

bool MXFGroupReader::IsEnabled() const
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (mReaders[i]->IsEnabled())
            return true;
    }

    return false;
}

vector<uint32_t> MXFGroupReader::GetSampleSequence(size_t member_index)
{
    vector<uint32_t> sample_sequence;

    mxfRational member_sample_rate = mReaders[member_index]->GetSampleRate();

    if (mSampleRate == member_sample_rate) {
        // 1 track sample equals 1 group sample
        sample_sequence.push_back(1);
    } else {
        int64_t num_samples = (int64_t)(member_sample_rate.numerator * mSampleRate.denominator) /
                                    (int64_t)(member_sample_rate.denominator * mSampleRate.numerator);
        int64_t remainder = (int64_t)(member_sample_rate.numerator * mSampleRate.denominator) %
                                    (int64_t)(member_sample_rate.denominator * mSampleRate.numerator);
        if (num_samples > 0) {
            if (remainder == 0) {
                // fixed integer number of reader samples for each clip sample
                sample_sequence.push_back(num_samples);
            } else {
                // try well known sample sequences
                size_t i;
                for (i = 0; i < SAMPLE_SEQUENCES_SIZE; i++) {
                    if (mSampleRate == SAMPLE_SEQUENCES[i].group_sample_rate &&
                        member_sample_rate == SAMPLE_SEQUENCES[i].member_sample_rate)
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
        // else input sample rate > mSampleRate and therefore input sample doesn't fit into group sample
    }

    return sample_sequence;
}

int64_t MXFGroupReader::ConvertGroupDuration(int64_t group_duration, size_t member_index)
{
    vector<uint32_t> &sample_sequence = mSampleSequences[member_index];

    int64_t num_sequences = group_duration / sample_sequence.size();
    int64_t sequence_remainder = group_duration % sample_sequence.size();

    int64_t member_duration = num_sequences * mSampleSequenceSizes[member_index];
    uint32_t i;
    for (i = 0; i < sequence_remainder; i++)
        member_duration += sample_sequence[i];

    return member_duration;
}

int64_t MXFGroupReader::ConvertGroupPosition(int64_t group_position, size_t member_index)
{
    return ConvertGroupDuration(group_position, member_index);
}

int64_t MXFGroupReader::ConvertMemberDuration(int64_t member_duration, size_t member_index)
{
    vector<uint32_t> &sample_sequence = mSampleSequences[member_index];

    int64_t num_sequences = member_duration / mSampleSequenceSizes[member_index];
    int64_t sequence_remainder = member_duration % mSampleSequenceSizes[member_index];

    int64_t group_duration = num_sequences * sample_sequence.size();
    size_t sequence_offset = 0;
    while (sequence_remainder > 0) {
        sequence_remainder -= sample_sequence[sequence_offset];
        // rounding down
        if (sequence_remainder >= 0)
            group_duration++;
        sequence_offset = (sequence_offset + 1) % sample_sequence.size();
    }

    return group_duration;
}

int64_t MXFGroupReader::ConvertMemberPosition(int64_t member_position, size_t member_index)
{
    vector<uint32_t> &sample_sequence = mSampleSequences[member_index];

    int64_t num_sequences = member_position / mSampleSequenceSizes[member_index];
    int64_t sequence_remainder = member_position % mSampleSequenceSizes[member_index];

    int64_t group_position = num_sequences * sample_sequence.size();
    size_t sequence_offset = 0;
    while (sequence_remainder > 0) {
        sequence_remainder -= sample_sequence[sequence_offset];
        // rounding up
        group_position++;
        sequence_offset = (sequence_offset + 1) % sample_sequence.size();
    }

    return group_position;
}

