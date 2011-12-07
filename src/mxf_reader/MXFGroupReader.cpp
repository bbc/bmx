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

#include <algorithm>

#include <im/mxf_reader/MXFGroupReader.h>
#include <im/mxf_reader/MXFFileReader.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



#define CONVERT_GROUP_DUR(dur)   convert_duration_higher(dur, mSampleSequences[i], mSampleSequenceSizes[i])
#define CONVERT_MEMBER_DUR(dur)  convert_duration_lower(dur, mSampleSequences[i], mSampleSequenceSizes[i])
#define CONVERT_GROUP_POS(pos)   convert_position_higher(pos, mSampleSequences[i], mSampleSequenceSizes[i])
#define CONVERT_MEMBER_POS(pos)  convert_position_lower(pos, mSampleSequences[i], mSampleSequenceSizes[i])


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
    try
    {
        if (mReaders.empty()) {
            log_error("Group reader has no members\n");
            throw false;
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
            vector<uint32_t> sample_sequence;
            if (!get_sample_sequence(mSampleRate, mReaders[i]->GetSampleRate(), &sample_sequence)) {
                mxfRational member_sample_rate = mReaders[i]->GetSampleRate();
                log_error("Incompatible group sample rate (%d/%d) and member sample rate (%d/%d)\n",
                          mSampleRate.numerator, mSampleRate.denominator,
                          member_sample_rate.numerator, member_sample_rate.denominator);
                throw false;
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
            int64_t member_duration = CONVERT_MEMBER_DUR(mReaders[i]->GetDuration());

            if (mDuration < 0 || member_duration < mDuration)
                mDuration = member_duration;
        }


        // set group timecodes and package infos if all members have the same values
        for (i = 0; i < mReaders.size(); i++) {
            if (i == 0) {
                if (mReaders[i]->mMaterialStartTimecode)
                    mMaterialStartTimecode = new Timecode(*mReaders[i]->mMaterialStartTimecode);
                if (mReaders[i]->mFileSourceStartTimecode)
                    mFileSourceStartTimecode = new Timecode(*mReaders[i]->mFileSourceStartTimecode);
                if (mReaders[i]->mPhysicalSourceStartTimecode)
                    mPhysicalSourceStartTimecode = new Timecode(*mReaders[i]->mPhysicalSourceStartTimecode);
                mMaterialPackageName = mReaders[i]->mMaterialPackageName;
                mMaterialPackageUID = mReaders[i]->mMaterialPackageUID;
                mPhysicalSourcePackageName = mReaders[i]->mPhysicalSourcePackageName;
            } else {
    #define CHECK_TIMECODE(tc_var) \
                if ((tc_var != 0) != (mReaders[i]->tc_var != 0) || \
                    (tc_var && tc_var != mReaders[i]->tc_var)) \
                { \
                    delete tc_var; \
                    tc_var = 0; \
                }
                CHECK_TIMECODE(mMaterialStartTimecode)
                CHECK_TIMECODE(mFileSourceStartTimecode)
                CHECK_TIMECODE(mPhysicalSourceStartTimecode)
                if (mMaterialPackageName != mReaders[i]->mMaterialPackageName)
                    mMaterialPackageName.clear();
                if (mMaterialPackageUID != mReaders[i]->mMaterialPackageUID)
                    mMaterialPackageUID = g_Null_UMID;
                if (mPhysicalSourcePackageName != mReaders[i]->mPhysicalSourcePackageName)
                    mPhysicalSourcePackageName.clear();
            }
        }

        // set default group read limits
        SetReadLimits();

        return true;
    }
    catch (const bool &err)
    {
        size_t i;
        for (i = 0; i < mTrackReaders.size(); i++)
            delete mTrackReaders[i];
        mTrackReaders.clear();

        mSampleSequences.clear();
        mSampleSequenceSizes.clear();
        mDuration = 0;

        return false;
    }
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

        mReaders[i]->SetReadLimits(CONVERT_GROUP_POS(start_position),
                                   CONVERT_GROUP_POS(end_position), false);
    }

    if (seek_start_position)
        Seek(start_position);
}

uint32_t MXFGroupReader::Read(uint32_t num_samples, bool is_top)
{
    int64_t current_position = GetPosition();

    if (is_top) {
        SetNextFramePosition(current_position);
        SetNextFrameTrackPositions();
    }

    uint32_t max_read_num_samples = 0;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int64_t member_current_position = CONVERT_GROUP_POS(current_position);

        // ensure external reader is in sync
        if (mReaders[i]->GetPosition() != member_current_position)
            mReaders[i]->Seek(member_current_position);


        uint32_t member_num_samples = (uint32_t)convert_duration_higher(num_samples,
                                                                        current_position,
                                                                        mSampleSequences[i],
                                                                        mSampleSequenceSizes[i]);

        uint32_t member_num_read = mReaders[i]->Read(member_num_samples, false);

        uint32_t group_num_read = (uint32_t)convert_duration_lower(member_num_read,
                                                                   member_current_position,
                                                                   mSampleSequences[i],
                                                                   mSampleSequenceSizes[i]);

        if (group_num_read > max_read_num_samples)
            max_read_num_samples = group_num_read;
    }

    return max_read_num_samples;
}

void MXFGroupReader::Seek(int64_t position)
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        mReaders[i]->Seek(CONVERT_GROUP_POS(position));
    }
}

int64_t MXFGroupReader::GetPosition() const
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        return CONVERT_MEMBER_POS(mReaders[i]->GetPosition());
    }

    return 0;
}

int16_t MXFGroupReader::GetMaxPrecharge(int64_t position, bool limit_to_available) const
{
    int16_t max_precharge = 0;

    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int16_t precharge = mReaders[i]->GetMaxPrecharge(CONVERT_GROUP_POS(position), limit_to_available);
        if (precharge < max_precharge) {
            IM_CHECK_M(mReaders[i]->GetSampleRate() == mSampleRate,
                       ("Currently only support precharge in group members if "
                        "member sample rate equals group sample rate"));
            max_precharge = precharge;
        }
    }

    return max_precharge;
}

int16_t MXFGroupReader::GetMaxRollout(int64_t position, bool limit_to_available) const
{
    int16_t max_rollout = 0;

    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int16_t rollout = mReaders[i]->GetMaxRollout(CONVERT_GROUP_POS(position), limit_to_available);
        if (rollout > max_rollout) {
            IM_CHECK_M(mReaders[i]->GetSampleRate() == mSampleRate,
                       ("Currently only support rollout in group members if "
                        "member sample rate equals group sample rate"));
            max_rollout = rollout;
        }
    }

    return max_rollout;
}

bool MXFGroupReader::HaveFixedLeadFillerOffset() const
{
    int64_t fixed_offset = -1;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        if (!mReaders[i]->HaveFixedLeadFillerOffset())
            return false;

        int64_t reader_fixed_offset = CONVERT_MEMBER_POS(mReaders[i]->GetFixedLeadFillerOffset());
        if (fixed_offset == -1)
            fixed_offset = reader_fixed_offset;
        else if (fixed_offset != reader_fixed_offset)
            return false;
    }

    return true;
}

int64_t MXFGroupReader::GetFixedLeadFillerOffset() const
{
    int64_t fixed_offset = -1;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        if (!mReaders[i]->HaveFixedLeadFillerOffset())
            return 0;

        int64_t reader_fixed_offset = CONVERT_MEMBER_POS(mReaders[i]->GetFixedLeadFillerOffset());
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

void MXFGroupReader::SetNextFramePosition(int64_t position)
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (mReaders[i]->IsEnabled())
            mReaders[i]->SetNextFramePosition(position);
    }
}

void MXFGroupReader::SetNextFrameTrackPositions()
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (mReaders[i]->IsEnabled())
            mReaders[i]->SetNextFrameTrackPositions();
    }
}

