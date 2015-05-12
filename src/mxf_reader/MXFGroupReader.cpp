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

#define __STDC_LIMIT_MACROS

#include <algorithm>
#include <set>

#include <bmx/mxf_reader/MXFGroupReader.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
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
    if (left_info->data_def != right_info->data_def)
        return left_info->data_def < right_info->data_def;

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
    mEmptyFrames = false;
    mEmptyFramesSet = false;
    mReadStartPosition = 0;
    mReadDuration = -1;
}

MXFGroupReader::~MXFGroupReader()
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        delete mReaders[i];
}

void MXFGroupReader::SetEmptyFrames(bool enable)
{
    mEmptyFrames = enable;
    mEmptyFramesSet = true;

    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++)
        mTrackReaders[i]->SetEmptyFrames(enable);
}

void MXFGroupReader::SetFileIndex(MXFFileIndex *file_index, bool take_ownership)
{
    MXFReader::SetFileIndex(file_index, take_ownership);

    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        mReaders[i]->SetFileIndex(file_index, false);
}

void MXFGroupReader::AddReader(MXFReader *reader)
{
    reader->SetFileIndex(mFileIndex, false);
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

        // the lowest input edit rate is the group edit rate
        float lowest_edit_rate = 1000000.0;
        size_t i;
        for (i = 0; i < mReaders.size(); i++) {
            float member_edit_rate = mReaders[i]->GetEditRate().numerator /
                                        (float)mReaders[i]->GetEditRate().denominator;
            if (member_edit_rate < lowest_edit_rate) {
                mEditRate = mReaders[i]->GetEditRate();
                lowest_edit_rate = member_edit_rate;
            }
        }
        BMX_CHECK(mEditRate.numerator != 0);


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
            if (!get_sample_sequence(mEditRate, mReaders[i]->GetEditRate(), &sample_sequence)) {
                mxfRational member_edit_rate = mReaders[i]->GetEditRate();
                log_error("Incompatible group edit rate (%d/%d) and member edit rate (%d/%d)\n",
                          mEditRate.numerator, mEditRate.denominator,
                          member_edit_rate.numerator, member_edit_rate.denominator);
                throw false;
            }

            mSampleSequences.push_back(sample_sequence);
            mSampleSequenceSizes.push_back(get_sequence_size(sample_sequence));
        }


        // group duration is the minimum member duration
        mDuration = -1;
        for (i = 0; i < mReaders.size(); i++) {
            if (mReaders[i]->GetDuration() < 0) {
                mDuration = -1;
                break;
            }

            int64_t member_duration = CONVERT_MEMBER_DUR(mReaders[i]->GetDuration());
            if (mDuration < 0 || member_duration < mDuration)
                mDuration = member_duration;
        }

        // group origin is the maximum member origin
        mOrigin = 0;
        for (i = 0; i < mReaders.size(); i++) {
            int64_t member_origin = CONVERT_MEMBER_POS(mReaders[i]->GetOrigin());
            if (member_origin > mOrigin)
                mOrigin = member_origin;
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
                mMaterialPackage = mReaders[i]->GetMaterialPackage();
            } else {
    #define CHECK_TIMECODE(tc_var) \
                if ((tc_var != 0) != (mReaders[i]->tc_var != 0) || \
                    (tc_var && *tc_var != *mReaders[i]->tc_var)) \
                { \
                    delete tc_var; \
                    tc_var = 0; \
                }
                CHECK_TIMECODE(mMaterialStartTimecode)
                CHECK_TIMECODE(mFileSourceStartTimecode)
                CHECK_TIMECODE(mPhysicalSourceStartTimecode)
                if (mMaterialPackageName != mReaders[i]->mMaterialPackageName)
                    mMaterialPackageName.clear();
                if (mMaterialPackageUID != mReaders[i]->mMaterialPackageUID) {
                    mMaterialPackageUID = g_Null_UMID;
                    mMaterialPackage = 0;
                }
                if (mPhysicalSourcePackageName != mReaders[i]->mPhysicalSourcePackageName)
                    mPhysicalSourcePackageName.clear();
            }
        }

        if (IsComplete()) {
            if (GetMaxPrecharge(0, true) != GetMaxPrecharge(0, false)) {
                log_warn("Possibly not enough precharge available in group (available=%d, required=%d)\n",
                         GetMaxPrecharge(0, true), GetMaxPrecharge(0, false));
            }
            if (GetMaxRollout(mDuration - 1, true) != GetMaxRollout(mDuration - 1, false)) {
                log_warn("Possibly not enough rollout available in group (available=%d, required=%d)\n",
                         GetMaxRollout(mDuration - 1, true), GetMaxRollout(mDuration - 1, false));
            }

            SetReadLimits();
        } else if (mDuration > 0) {
            SetReadLimits(- mOrigin, mOrigin + mDuration, false);
        }

        if (mEmptyFramesSet) {
            for (i = 0; i < mTrackReaders.size(); i++)
                mTrackReaders[i]->SetEmptyFrames(mEmptyFrames);
        }

        return true;
    }
    catch (const bool &err)
    {
        (void)err;

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

MXFFileReader* MXFGroupReader::GetFileReader(size_t file_id)
{
    MXFFileReader *reader = 0;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        reader = mReaders[i]->GetFileReader(file_id);
        if (reader)
            break;
    }

    return reader;
}

vector<size_t> MXFGroupReader::GetFileIds(bool internal_ess_only) const
{
    set<size_t> file_id_set;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        vector<size_t> track_file_ids = mTrackReaders[i]->GetFileIds(internal_ess_only);
        file_id_set.insert(track_file_ids.begin(), track_file_ids.end());
    }

    vector<size_t> file_ids;
    file_ids.insert(file_ids.begin(), file_id_set.begin(), file_id_set.end());

    return file_ids;
}

bool MXFGroupReader::IsComplete() const
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsComplete())
            return false;
    }

    return true;
}

bool MXFGroupReader::IsSeekable() const
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsSeekable())
            return false;
    }

    return true;
}

void MXFGroupReader::GetReadLimits(bool limit_to_available, int64_t *start_position, int64_t *duration) const
{
    int16_t precharge = GetMaxPrecharge(0, limit_to_available);
    int16_t rollout = GetMaxRollout(mDuration - 1, limit_to_available);
    *start_position = 0 + precharge;
    *duration = - precharge + mDuration + rollout;
}

void MXFGroupReader::SetReadLimits()
{
    int64_t start_position;
    int64_t duration;
    GetReadLimits(false, &start_position, &duration);
    SetReadLimits(start_position, duration, true);
}

void MXFGroupReader::SetReadLimits(int64_t start_position, int64_t duration, bool seek_start_position)
{
    mReadStartPosition = start_position;
    mReadDuration = duration;

    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int64_t member_start_position = CONVERT_GROUP_POS(start_position);
        int64_t member_duration;
        if (duration == 0)
            member_duration = 0;
        else
            member_duration = CONVERT_GROUP_DUR(start_position + duration) - member_start_position;
        mReaders[i]->SetReadLimits(member_start_position, member_duration, false);
    }

    if (seek_start_position)
        Seek(start_position);
}

uint32_t MXFGroupReader::Read(uint32_t num_samples, bool is_top)
{
    mReadError = false;
    mReadErrorMessage.clear();

    int64_t current_position = GetPosition();

    StartRead();
    try
    {
        if (is_top) {
            SetNextFramePosition(mEditRate, current_position);
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
            if (member_num_read < member_num_samples && mReaders[i]->ReadError())
                throw BMXException(mReaders[i]->ReadErrorMessage());

            uint32_t group_num_read = (uint32_t)convert_duration_lower(member_num_read,
                                                                       member_current_position,
                                                                       mSampleSequences[i],
                                                                       mSampleSequenceSizes[i]);

            if (group_num_read > max_read_num_samples)
                max_read_num_samples = group_num_read;
        }

        CompleteRead();

        return max_read_num_samples;
    }
    catch (const MXFException &ex)
    {
        mReadErrorMessage = ex.getMessage();
    }
    catch (const BMXException &ex)
    {
        mReadErrorMessage = ex.what();
    }
    catch (...)
    {
    }

    mReadError = true;
    AbortRead();
    Seek(current_position);

    return 0;
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
    int64_t max_precharge = 0;

    int64_t max_start_position = INT64_MIN;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int16_t precharge = mReaders[i]->GetMaxPrecharge(CONVERT_GROUP_POS(position), limit_to_available);
        if (precharge != 0) {
            BMX_CHECK_M(mReaders[i]->GetEditRate() == mEditRate,
                        ("Currently only support precharge in group members if "
                         "member edit rate equals group edit rate"));
            if (precharge < max_precharge)
                max_precharge = precharge;
        }

        if (limit_to_available) {
            int64_t mem_start_position, mem_duration;
            mReaders[i]->GetReadLimits(true, &mem_start_position, &mem_duration);
            int64_t mem_max_start_position = CONVERT_MEMBER_POS(mem_start_position);
            if (mem_max_start_position > max_start_position)
                max_start_position = mem_max_start_position;
        }
    }

    if (limit_to_available && max_precharge < max_start_position - position)
        max_precharge = max_start_position - position;

    return max_precharge < 0 ? (int16_t)max_precharge : 0;
}

int16_t MXFGroupReader::GetMaxRollout(int64_t position, bool limit_to_available) const
{
    int64_t max_rollout = 0;

    int64_t min_end_position = INT64_MAX;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsEnabled())
            continue;

        int16_t rollout = mReaders[i]->GetMaxRollout(CONVERT_GROUP_POS(position + 1) - 1, limit_to_available);
        if (rollout != 0) {
            BMX_CHECK_M(mReaders[i]->GetEditRate() == mEditRate,
                        ("Currently only support rollout in group members if "
                         "member edit rate equals group edit rate"));
            if (rollout > max_rollout)
                max_rollout = rollout;
        }

        if (limit_to_available) {
            int64_t mem_start_position, mem_duration;
            mReaders[i]->GetReadLimits(true, &mem_start_position, &mem_duration);
            int64_t mem_min_end_position = CONVERT_MEMBER_DUR(mem_start_position + mem_duration);
            if (mem_min_end_position < min_end_position)
                min_end_position = mem_min_end_position;
        }
    }

    if (limit_to_available && max_rollout > min_end_position - position)
        max_rollout = min_end_position - position;

    return max_rollout > 0 ? (int16_t)max_rollout : 0;
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
    BMX_CHECK(track_index < mTrackReaders.size());
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

int16_t MXFGroupReader::GetTrackPrecharge(size_t track_index, int64_t clip_position, int16_t clip_precharge) const
{
    if (clip_precharge >= 0)
        return 0;

    MXFTrackReader *track_reader = GetTrackReader(track_index);

    BMX_CHECK_M(track_reader->GetEditRate() == mEditRate,
                ("Currently only support precharge in group members if "
                 "member edit rate equals group edit rate"));
    (void)clip_position;

    return clip_precharge;
}

int16_t MXFGroupReader::GetTrackRollout(size_t track_index, int64_t clip_position, int16_t clip_rollout) const
{
    if (clip_rollout <= 0)
        return 0;

    MXFTrackReader *track_reader = GetTrackReader(track_index);

    BMX_CHECK_M(track_reader->GetEditRate() == mEditRate,
                ("Currently only support rollout in group members if "
                 "member edit rate equals group edit rate"));
    (void)clip_position;

    return clip_rollout;
}

void MXFGroupReader::SetNextFramePosition(Rational edit_rate, int64_t position)
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (mReaders[i]->IsEnabled())
            mReaders[i]->SetNextFramePosition(edit_rate, position);
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

void MXFGroupReader::SetTemporaryFrameBuffer(bool enable)
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        mReaders[i]->SetTemporaryFrameBuffer(enable);
}

void MXFGroupReader::StartRead()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->StartRead();
    }
}

void MXFGroupReader::CompleteRead()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->CompleteRead();
    }
}

void MXFGroupReader::AbortRead()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->AbortRead();
    }
}

