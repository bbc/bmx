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
#include <set>

#include <bmx/mxf_reader/MXFSequenceReader.h>
#include <bmx/mxf_reader/MXFSequenceTrackReader.h>
#include <bmx/mxf_reader/MXFGroupReader.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define DISABLED_SEG_READ_LIMIT      (-9999)

#define CONVERT_SEQ_DUR(dur)    convert_duration_higher(dur, mSampleSequences[i], mSampleSequenceSizes[i])
#define CONVERT_GROUP_DUR(dur)  convert_duration_lower(dur, mSampleSequences[i], mSampleSequenceSizes[i])
#define CONVERT_SEQ_POS(pos)    convert_position_higher(pos, mSampleSequences[i], mSampleSequenceSizes[i])
#define CONVERT_GROUP_POS(pos)  convert_position_lower(pos, mSampleSequences[i], mSampleSequenceSizes[i])



static bool compare_group_reader(const MXFGroupReader *left, const MXFGroupReader *right)
{
    Timecode left_tc, right_tc;
    string left_source_name, right_source_name;

    // playout timecode at origin position
    left_tc = left->GetPlayoutTimecode(left->GetFixedLeadFillerOffset());
    right_tc = right->GetPlayoutTimecode(right->GetFixedLeadFillerOffset());

    return left_tc < right_tc;
}



MXFSequenceReader::MXFSequenceReader()
: MXFReader()
{
    mEmptyFrames = false;
    mEmptyFramesSet = false;
    mReadStartPosition = 0;
    mReadDuration = -1;
    mPosition = 0;
}

MXFSequenceReader::~MXFSequenceReader()
{
    size_t i;
    if (mGroupSegments.empty()) {
        for (i = 0; i < mReaders.size(); i++)
            delete mReaders[i];
    } else {
        for (i = 0; i < mGroupSegments.size(); i++)
            delete mGroupSegments[i];
    }
    for (i = 0; i < mTrackReaders.size(); i++)
        delete mTrackReaders[i];
}

void MXFSequenceReader::SetEmptyFrames(bool enable)
{
    mEmptyFrames = enable;
    mEmptyFramesSet = true;

    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++)
        mTrackReaders[i]->SetEmptyFrames(enable);
}

void MXFSequenceReader::SetFileIndex(MXFFileIndex *file_index, bool take_ownership)
{
    MXFReader::SetFileIndex(file_index, take_ownership);

    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        mReaders[i]->SetFileIndex(file_index, false);
}

void MXFSequenceReader::AddReader(MXFReader *reader)
{
    // TODO: support incomplete files
    if (!reader->IsComplete())
        BMX_EXCEPTION(("MXF sequence reader currently only supports complete files with known duration"));

    reader->SetFileIndex(mFileIndex, false);
    mReaders.push_back(reader);
}

bool MXFSequenceReader::Finalize(bool check_is_complete, bool keep_input_order)
{
    try
    {
        if (mReaders.empty()) {
            log_error("Sequence reader has no tracks\n");
            throw false;
        }

        // the lowest input edit rate is the sequence reader edit rate
        float lowest_edit_rate = 1000000.0;
        size_t i;
        for (i = 0; i < mReaders.size(); i++) {
            float edit_rate = mReaders[i]->GetEditRate().numerator /
                                        (float)mReaders[i]->GetEditRate().denominator;
            if (edit_rate < lowest_edit_rate) {
                mEditRate = mReaders[i]->GetEditRate();
                lowest_edit_rate = edit_rate;
            }
        }
        BMX_CHECK(mEditRate.numerator != 0);


        // group inputs by material package uid and lead filler offset
        // need to consider the leading filler offset for spanned P2 files
        map<pair<mxfUMID, int64_t>, MXFGroupReader*> group_ids;
        for (i = 0; i < mReaders.size(); i++) {
            int64_t lead_offset = convert_position(mReaders[i]->GetEditRate(),
                                                   mReaders[i]->GetFixedLeadFillerOffset(),
                                                   mEditRate,
                                                   ROUND_UP);

            MXFGroupReader *group_reader;
            if (mReaders[i]->GetMaterialPackageUID() == g_Null_UMID) {
                mGroupSegments.push_back(new MXFGroupReader());
                group_reader = mGroupSegments.back();
            } else {
                map<pair<mxfUMID, int64_t>, MXFGroupReader*>::const_iterator group_id =
                    group_ids.find(make_pair(mReaders[i]->GetMaterialPackageUID(), lead_offset));
                if (group_id == group_ids.end()) {
                    mGroupSegments.push_back(new MXFGroupReader());
                    group_reader = mGroupSegments.back();
                    group_ids[make_pair(mReaders[i]->GetMaterialPackageUID(), lead_offset)] = group_reader;
                } else {
                    group_reader = group_id->second;
                }
            }

            group_reader->AddReader(mReaders[i]);
        }
        for (i = 0; i < mGroupSegments.size(); i++) {
            if (!mGroupSegments[i]->Finalize())
                throw false;
        }


        // order groups by playout timecode
        if (!keep_input_order && mGroupSegments.size() > 1) {
            stable_sort(mGroupSegments.begin(), mGroupSegments.end(), compare_group_reader);

            // handle case where a sequence of groups passes through (max one) midnight
            size_t seq_start_index = 0;
            if (FindSequenceStart(mGroupSegments, &seq_start_index) && seq_start_index > 0)
                rotate(mGroupSegments.begin(), mGroupSegments.begin() + seq_start_index, mGroupSegments.end());
        }


        // check only the first group has precharge and only the last group has rollout
        for (i = 0; i < mGroupSegments.size(); i++) {
            if (i > 0 && mGroupSegments[i]->GetMaxPrecharge(0, false) > 0) {
                log_error("Non-first group in sequence with precharge is not supported\n");
                throw false;
            }
            if (i < mGroupSegments.size() - 1 &&
                mGroupSegments[i]->GetMaxRollout(mGroupSegments[i]->GetDuration() - 1, false) > 0)
            {
                log_error("Appending to segment with rollout is not supported\n");
                throw false;
            }
        }


        // check playout timecode is continuous
        // log warning and ignore timecode discontinuities if not reordering (keep_input_order is true)
        Timecode expected_start_tc;
        for (i = 0; i < mGroupSegments.size(); i++) {
            if (i == 0) {
                expected_start_tc = mGroupSegments[i]->GetPlayoutTimecode(mGroupSegments[i]->GetFixedLeadFillerOffset());
            } else {
                Timecode start_tc = mGroupSegments[i]->GetPlayoutTimecode(mGroupSegments[i]->GetFixedLeadFillerOffset());
                if (mGroupSegments[0]->HavePlayoutTimecode() &&
                    (!mGroupSegments[i]->HavePlayoutTimecode() || start_tc != expected_start_tc))
                {
                    if (keep_input_order) {
                        log_warn("Ignoring timecode discontinuity between sequence track segments\n");
                        break;
                    } else {
                        log_error("Timecode discontinuity between sequence track segments\n");
                        throw false;
                    }
                }
            }

            expected_start_tc.AddOffset(mGroupSegments[i]->GetDuration(), mGroupSegments[i]->GetEditRate());
        }


        // create tracks from the first group
        for (i = 0; i < mGroupSegments[0]->GetNumTrackReaders(); i++) {
            MXFTrackReader *group_track = mGroupSegments[0]->GetTrackReader(i);
            if (!group_track->IsEnabled())
                continue;

            MXFSequenceTrackReader *seq_track = new MXFSequenceTrackReader(this, mTrackReaders.size());
            mTrackReaders.push_back(seq_track);

            seq_track->AppendSegment(group_track);
        }


        // add compatible tracks from other groups
        for (i = 1; i < mGroupSegments.size(); i++) {
            MXFSequenceTrackReader *first_extended_seq_track = 0;
            size_t j;
            for (j = 0; j < mGroupSegments[i]->GetNumTrackReaders(); j++) {
                MXFTrackReader *group_track = mGroupSegments[i]->GetTrackReader(j);
                if (!group_track->IsEnabled())
                    continue;

                // append group track to first available and compatible sequence track
                size_t k;
                for (k = 0; k < mTrackReaders.size(); k++) {
                    MXFSequenceTrackReader *seq_track = mTrackReaders[k];
                    if (seq_track->GetNumSegments() != i)
                        continue; // incomplete track or new segment already appended

                    // skip tracks where sequence duration != first appended track's sequence duration
                    if (first_extended_seq_track) {
                        int64_t seq_track_duration = convert_duration(seq_track->GetEditRate(),
                                                                      seq_track->GetDuration(),
                                                                      mEditRate,
                                                                      ROUND_AUTO);
                        int64_t group_track_duration = convert_duration(group_track->GetEditRate(),
                                                                        group_track->GetDuration(),
                                                                        mEditRate,
                                                                        ROUND_AUTO);
                        int64_t first_seq_track_duration = convert_duration(first_extended_seq_track->GetEditRate(),
                                                                            first_extended_seq_track->GetDuration(),
                                                                            mEditRate,
                                                                            ROUND_AUTO);
                        if (seq_track_duration + group_track_duration != first_seq_track_duration)
                        {
                            log_warn("Not appending track segment resulting in different sequence duration\n");
                            continue;
                        }
                    }

                    // append track segment if compatible
                    if (seq_track->IsCompatible(group_track)) {
                        seq_track->AppendSegment(group_track);
                        if (!first_extended_seq_track)
                            first_extended_seq_track = seq_track;
                        break;
                    }
                }

                // disable group track if it was not added to the sequence
                if (k >= mTrackReaders.size())
                    group_track->SetEnable(false);
            }

            // abort if this group fails to contribute any tracks
            if (!first_extended_seq_track) {
                log_error("No track could be appended from the group to the sequence\n");
                throw false;
            }
        }


        // delete incomplete tracks
        for (i = 0; i < mTrackReaders.size(); i++) {
            MXFSequenceTrackReader *seq_track = mTrackReaders[i];
            if (seq_track->GetNumSegments() != mGroupSegments.size()) {
                if (check_is_complete) {
                    log_error("Incomplete track sequence\n");
                    return false;
                }

                // first disable track in groups
                seq_track->SetEnable(false);

                delete seq_track;
                mTrackReaders.erase(mTrackReaders.begin() + i);
                i--;
            }
        }
        if (mTrackReaders.empty()) {
            log_error("All tracks in sequence are incomplete\n");
            throw false;
        }


        // extract the sample sequences for each group
        for (i = 0; i < mGroupSegments.size(); i++) {
            vector<uint32_t> sample_sequence;
            if (!get_sample_sequence(mEditRate, mGroupSegments[i]->GetEditRate(), &sample_sequence)) {
                mxfRational group_edit_rate = mGroupSegments[i]->GetEditRate();
                log_error("Incompatible sequence edit rate (%d/%d) and group edit rate (%d/%d)\n",
                          mEditRate.numerator, mEditRate.denominator,
                          group_edit_rate.numerator, group_edit_rate.denominator);
                throw false;
            }

            mSampleSequences.push_back(sample_sequence);
            mSampleSequenceSizes.push_back(get_sequence_size(sample_sequence));
        }


        // get the segment offsets, offset adjustments and total duration
        int64_t offset = 0;
        int64_t offset_adjustment = 0;
        for (i = 0; i < mGroupSegments.size(); i++) {
            mSegmentOffsets.push_back(offset);
            mSegmentOffsetAdjustments.push_back(offset_adjustment);
            offset += CONVERT_GROUP_DUR(mGroupSegments[i]->GetDuration());
            offset_adjustment = mGroupSegments[i]->GetDuration() -
                                    CONVERT_SEQ_DUR(CONVERT_GROUP_DUR(mGroupSegments[i]->GetDuration()));
        }
        mDuration = offset;


        // extract the sequence timecodes if present and continuous
        Timecode mat_expected_start_tc, fs_expected_start_tc, ps_expected_start_tc;
        bool mat_valid = false, fs_valid = false, ps_valid = false;
        for (i = 0; i < mGroupSegments.size(); i++) {
            if (i == 0) {
                mat_expected_start_tc = mGroupSegments[i]->GetMaterialTimecode(
                                            mGroupSegments[i]->GetFixedLeadFillerOffset());
                mat_valid = mGroupSegments[i]->HaveMaterialTimecode();
                fs_expected_start_tc = mGroupSegments[i]->GetFileSourceTimecode(0);
                fs_valid = mGroupSegments[i]->HaveFileSourceTimecode();
                ps_expected_start_tc = mGroupSegments[i]->GetPhysicalSourceTimecode(0);
                ps_valid = mGroupSegments[i]->HavePhysicalSourceTimecode();
            } else {
                Timecode mat_start_tc = mGroupSegments[i]->GetMaterialTimecode(
                                            mGroupSegments[i]->GetFixedLeadFillerOffset());
                if (mGroupSegments[0]->HaveMaterialTimecode() &&
                    (!mGroupSegments[i]->HaveMaterialTimecode() || mat_start_tc != mat_expected_start_tc))
                {
                    mat_valid = false;
                }
                Timecode fs_start_tc = mGroupSegments[i]->GetFileSourceTimecode(0);
                if (mGroupSegments[0]->HaveFileSourceTimecode() &&
                    (!mGroupSegments[i]->HaveFileSourceTimecode() || fs_start_tc != fs_expected_start_tc))
                {
                    fs_valid = false;
                }
                Timecode ps_start_tc = mGroupSegments[i]->GetPhysicalSourceTimecode(0);
                if (mGroupSegments[0]->HavePhysicalSourceTimecode() &&
                    (!mGroupSegments[i]->HavePhysicalSourceTimecode() || ps_start_tc != ps_expected_start_tc))
                {
                    ps_valid = false;
                }
            }

            mat_expected_start_tc.AddOffset(mGroupSegments[i]->GetDuration(), mGroupSegments[i]->GetEditRate());
            fs_expected_start_tc.AddOffset(mGroupSegments[i]->GetDuration(), mGroupSegments[i]->GetEditRate());
            ps_expected_start_tc.AddOffset(mGroupSegments[i]->GetDuration(), mGroupSegments[i]->GetEditRate());
        }
        if (mat_valid) {
            mMaterialStartTimecode = new Timecode(mGroupSegments[0]->GetMaterialTimecode(
                                                            mGroupSegments[0]->GetFixedLeadFillerOffset()));
        }
        if (fs_valid) {
            mFileSourceStartTimecode = new Timecode(mGroupSegments[0]->GetFileSourceTimecode(0));
        }
        if (ps_valid) {
            mPhysicalSourceStartTimecode = new Timecode(mGroupSegments[0]->GetPhysicalSourceTimecode(0));;
        }


        // enable/disable empty frames
        if (mEmptyFramesSet) {
            for (i = 0; i < mTrackReaders.size(); i++)
                mTrackReaders[i]->SetEmptyFrames(mEmptyFrames);
        }


        // set default group sequence read limits
        SetReadLimits();

        return true;
    }
    catch (const bool &err)
    {
        (void)err;

        size_t i;
        for (i = 0; i < mGroupSegments.size(); i++)
            delete mGroupSegments[i];
        mGroupSegments.clear();

        for (i = 0; i < mTrackReaders.size(); i++)
            delete mTrackReaders[i];
        mTrackReaders.clear();

        mSampleSequences.clear();
        mSampleSequenceSizes.clear();
        mSegmentOffsets.clear();
        mDuration = 0;

        return false;
    }
}

void MXFSequenceReader::UpdateReadLimits()
{
    mReadStartPosition = 0;
    mReadDuration = -1;

    if (mGroupSegments.empty())
        return;

    // get read start position from first enabled segment
    size_t i;
    for (i = 0; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i]->GetReadStartPosition() > DISABLED_SEG_READ_LIMIT)
            break;
        mReadStartPosition += CONVERT_GROUP_DUR(mGroupSegments[i]->GetDuration());
    }
    if (i >= mGroupSegments.size())
        return; // nothing enabled

    mReadStartPosition += CONVERT_GROUP_POS(mGroupSegments[i]->GetReadStartPosition());
    mReadDuration = CONVERT_GROUP_DUR(mGroupSegments[i]->GetReadDuration());

    // get read end position from the last enabled segment
    for (i = i + 1; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i]->GetReadStartPosition() <= DISABLED_SEG_READ_LIMIT)
            break;
        mReadDuration += CONVERT_GROUP_DUR(mGroupSegments[i]->GetReadDuration());
    }
}

MXFFileReader* MXFSequenceReader::GetFileReader(size_t file_id)
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

vector<size_t> MXFSequenceReader::GetFileIds(bool internal_ess_only) const
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

bool MXFSequenceReader::IsComplete() const
{
    // TODO: support incomplete files
    return true;
}

bool MXFSequenceReader::IsSeekable() const
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (!mReaders[i]->IsSeekable())
            return false;
    }

    return true;
}

void MXFSequenceReader::GetReadLimits(bool limit_to_available, int64_t *start_position, int64_t *duration) const
{
    int16_t precharge = GetMaxPrecharge(0, limit_to_available);
    int16_t rollout = GetMaxRollout(mDuration - 1, limit_to_available);
    *start_position = 0 + precharge;
    *duration = - precharge + mDuration + rollout;
}

void MXFSequenceReader::SetReadLimits()
{
    int64_t start_position;
    int64_t duration;
    GetReadLimits(false, &start_position, &duration);
    SetReadLimits(start_position, duration, true);
}

void MXFSequenceReader::SetReadLimits(int64_t start_position, int64_t duration, bool seek_start_position)
{
    MXFGroupReader *start_segment, *end_segment;
    size_t segment_index;
    int64_t start_segment_position;
    int64_t end_segment_duration;
    GetSegmentPosition(start_position, &start_segment, &segment_index, &start_segment_position);
    GetSegmentPosition(start_position + duration, &end_segment, &segment_index, &end_segment_duration);

    if (start_segment == end_segment) {
        start_segment->SetReadLimits(start_segment_position, end_segment_duration - start_segment_position, false);
    } else {
        // note that start segment has 0 rollout
        start_segment->SetReadLimits(start_segment_position,
                                     start_segment->GetDuration() - start_segment_position,
                                     false);
        // note that end segment has 0 pre-charge
        end_segment->SetReadLimits(0, end_segment_duration, false);
    }

    // effectively disable segments before the start segment and after the end segment
    size_t i;
    for (i = 0; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i] == start_segment)
            break;

        mGroupSegments[i]->SetReadLimits(DISABLED_SEG_READ_LIMIT, 0, false);
    }
    for (; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i] == end_segment)
            break;
    }
    for (i = i + 1; i < mGroupSegments.size(); i++)
        mGroupSegments[i]->SetReadLimits(DISABLED_SEG_READ_LIMIT, 0, false);


    UpdateReadLimits();
    for (i = 0; i < mTrackReaders.size(); i++)
        mTrackReaders[i]->UpdateReadLimits();


    if (seek_start_position)
        Seek(start_position);
}

uint32_t MXFSequenceReader::Read(uint32_t num_samples, bool is_top)
{
    if (!IsEnabled())
        return 0;

    if (is_top) {
        SetNextFramePosition(mEditRate, mPosition);
        SetNextFrameTrackPositions();
    }

    MXFGroupReader *segment;
    size_t segment_index;
    int64_t segment_position;
    GetSegmentPosition(mPosition, &segment, &segment_index, &segment_position);

    uint32_t total_num_read = 0;
    MXFGroupReader *prev_segment;
    do {
        // seek if segment is out of position
        if (segment_position != segment->GetPosition())
            segment->Seek(segment_position);

        uint32_t group_num_samples = (uint32_t)convert_duration_higher(num_samples - total_num_read,
                                                                       mPosition + total_num_read,
                                                                       mSampleSequences[segment_index],
                                                                       mSampleSequenceSizes[segment_index]);

        uint32_t group_num_read = segment->Read(group_num_samples, false);

        uint32_t seq_num_read = (uint32_t)convert_duration_lower(group_num_read,
                                                                 segment_position,
                                                                 mSampleSequences[segment_index],
                                                                 mSampleSequenceSizes[segment_index]);

        total_num_read += seq_num_read;

        prev_segment = segment;
        GetSegmentPosition(mPosition + total_num_read, &segment, &segment_index, &segment_position);
    }
    while (total_num_read < num_samples && segment != prev_segment);

    // always be positioned num_samples after previous position
    mPosition += num_samples;

    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++)
        mTrackReaders[i]->UpdatePosition(segment_index);

    return total_num_read;
}

void MXFSequenceReader::Seek(int64_t position)
{
    BMX_CHECK(!mGroupSegments.empty());

    MXFGroupReader *segment;
    size_t segment_index;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_index, &segment_position);

    segment->Seek(segment_position);

    mPosition = position;

    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++)
        mTrackReaders[i]->UpdatePosition(segment_index);
}

int16_t MXFSequenceReader::GetMaxPrecharge(int64_t position, bool limit_to_available) const
{
    BMX_CHECK(!mGroupSegments.empty());

    MXFGroupReader *segment;
    size_t segment_index;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_index, &segment_position);

    return segment->GetMaxPrecharge(segment_position, limit_to_available);
}

int16_t MXFSequenceReader::GetMaxRollout(int64_t position, bool limit_to_available) const
{
    BMX_CHECK(!mGroupSegments.empty());

    MXFGroupReader *segment;
    size_t segment_index;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_index, &segment_position);

    return segment->GetMaxRollout(segment_position, limit_to_available);
}

bool MXFSequenceReader::HaveFixedLeadFillerOffset() const
{
    BMX_CHECK(!mGroupSegments.empty());
    return mGroupSegments[0]->HaveFixedLeadFillerOffset();
}

int64_t MXFSequenceReader::GetFixedLeadFillerOffset() const
{
    BMX_CHECK(!mGroupSegments.empty());
    return convert_duration_lower(mGroupSegments[0]->GetFixedLeadFillerOffset(),
                                  mSampleSequences[0], mSampleSequenceSizes[0]);
}

MXFTrackReader* MXFSequenceReader::GetTrackReader(size_t track_index) const
{
    BMX_CHECK(track_index < mTrackReaders.size());
    return mTrackReaders[track_index];
}

bool MXFSequenceReader::IsEnabled() const
{
    BMX_CHECK(!mGroupSegments.empty());
    return mGroupSegments[0]->IsEnabled();
}

int16_t MXFSequenceReader::GetTrackPrecharge(size_t track_index, int64_t clip_position, int16_t clip_precharge) const
{
    if (clip_precharge >= 0)
        return 0;

    BMX_CHECK(!mGroupSegments.empty());

    MXFGroupReader *segment;
    size_t segment_index;
    int64_t segment_position;
    GetSegmentPosition(clip_position, &segment, &segment_index, &segment_position);

    return segment->GetTrackPrecharge(track_index, segment_position, clip_precharge);
}

int16_t MXFSequenceReader::GetTrackRollout(size_t track_index, int64_t clip_position, int16_t clip_rollout) const
{
    if (clip_rollout <= 0)
        return 0;

    BMX_CHECK(!mGroupSegments.empty());

    MXFGroupReader *segment;
    size_t segment_index;
    int64_t segment_position;
    GetSegmentPosition(clip_position, &segment, &segment_index, &segment_position);

    return segment->GetTrackRollout(track_index, segment_position, clip_rollout);
}

void MXFSequenceReader::SetNextFramePosition(Rational edit_rate, int64_t position)
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetMXFFrameBuffer()->SetNextFramePosition(edit_rate, position);
    }
}

void MXFSequenceReader::SetNextFrameTrackPositions()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled()) {
            mTrackReaders[i]->GetMXFFrameBuffer()->SetNextFrameTrackPosition(
                mTrackReaders[i]->GetEditRate(), mTrackReaders[i]->GetPosition());
        }
    }
}

void MXFSequenceReader::SetTemporaryFrameBuffer(bool enable)
{
    size_t i;
    for (i = 0; i < mGroupSegments.size(); i++)
        mGroupSegments[i]->SetTemporaryFrameBuffer(enable);
}

bool MXFSequenceReader::FindSequenceStart(const vector<MXFGroupReader*> &group_readers, size_t *seq_start_index_out) const
{
    Timecode expected_start_timecode;
    size_t seq_start_index = (size_t)(-1);
    size_t i;
    for (i = 0; i < group_readers.size() + 1; i++) { // + 1 to check crossover last to first
        size_t index = i % group_readers.size();

        if (!group_readers[index]->HavePlayoutTimecode())
            return false;

        Timecode start_timecode = group_readers[index]->GetPlayoutTimecode(
            group_readers[index]->GetFixedLeadFillerOffset());

        if (i > 0 && start_timecode != expected_start_timecode) {
            if (seq_start_index == (size_t)(-1))
                seq_start_index = index;
            else
                return false;
        }

        expected_start_timecode = start_timecode;
        expected_start_timecode.AddOffset(group_readers[index]->GetDuration(), group_readers[index]->GetEditRate());
    }

    *seq_start_index_out = (seq_start_index == (size_t)(-1) ? 0 : seq_start_index);
    return true;
}

void MXFSequenceReader::GetSegmentPosition(int64_t position, MXFGroupReader **segment, size_t *segment_index,
                                           int64_t *segment_position) const
{
    BMX_CHECK(!mGroupSegments.empty());

    size_t i;
    for (i = 0; i < mSegmentOffsets.size(); i++) {
        if (position < mSegmentOffsets[i])
            break;
    }

    if (i == 0) {
        *segment = mGroupSegments[0];
        *segment_index = 0;
        *segment_position = CONVERT_SEQ_POS(position);
    } else {
        i--;
        *segment = mGroupSegments[i];
        *segment_index = i;
        *segment_position = CONVERT_SEQ_POS(position) - CONVERT_SEQ_POS(mSegmentOffsets[i]) -
                                mSegmentOffsetAdjustments[i];
    }
}

