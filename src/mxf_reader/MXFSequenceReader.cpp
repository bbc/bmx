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

#include <im/mxf_reader/MXFSequenceReader.h>
#include <im/mxf_reader/MXFSequenceTrackReader.h>
#include <im/mxf_reader/MXFGroupReader.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
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
    left_tc = left->GetPlayoutTimecode(-left->GetReadStartPosition());
    right_tc = right->GetPlayoutTimecode(-right->GetReadStartPosition());

    return left_tc < right_tc;
}



MXFSequenceReader::MXFSequenceReader()
: MXFReader()
{
    mReadStartPosition = 0;
    mReadEndPosition = 0;
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

void MXFSequenceReader::AddReader(MXFReader *reader)
{
    mReaders.push_back(reader);
}

bool MXFSequenceReader::Finalize(bool check_is_complete, bool keep_input_order)
{
    if (mReaders.empty()) {
        log_error("Sequence reader has no tracks\n");
        return false;
    }

    // the lowest input sample rate is the sequence reader sample rate
    float lowest_sample_rate = 1000000.0;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        float sample_rate = mReaders[i]->GetSampleRate().numerator /
                                    (float)mReaders[i]->GetSampleRate().denominator;
        if (sample_rate < lowest_sample_rate) {
            mSampleRate = mReaders[i]->GetSampleRate();
            lowest_sample_rate = sample_rate;
        }
    }
    IM_CHECK(mSampleRate.numerator != 0);


    // group inputs by material package uid and lead filler offset
    // need to consider the leading filler offset for spanned P2 files
    map<pair<mxfUMID, int64_t>, MXFGroupReader*> group_ids;
    for (i = 0; i < mReaders.size(); i++) {
        int64_t lead_offset = convert_position(mReaders[i]->GetSampleRate(),
                                               mReaders[i]->GetFixedLeadFillerOffset(),
                                               mSampleRate,
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
            return false;
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
            return false;
        }
        if (i < mGroupSegments.size() - 1 &&
            mGroupSegments[i]->GetMaxRollout(mGroupSegments[i]->GetDuration() - 1, false) > 0)
        {
            log_error("Appending to segment with rollout is not supported\n");
            return false;
        }
    }


    // check playout timecode is continuous
    // log warning and ignore timecode discontinuities if not reordering (keep_input_order is true)
    Timecode expected_start_tc;
    for (i = 0; i < mGroupSegments.size(); i++) {
        if (i == 0) {
            expected_start_tc = mGroupSegments[i]->GetPlayoutTimecode(- mGroupSegments[i]->GetReadStartPosition());
        } else {
            Timecode start_tc = mGroupSegments[i]->GetPlayoutTimecode(- mGroupSegments[i]->GetReadStartPosition());
            if (mGroupSegments[0]->HavePlayoutTimecode() &&
                (!mGroupSegments[i]->HavePlayoutTimecode() || start_tc != expected_start_tc))
            {
                if (keep_input_order) {
                    log_warn("Ignoring timecode discontinuity between sequence track segments\n");
                    break;
                } else {
                    log_error("Timecode discontinuity between sequence track segments\n");
                    return false;
                }
            }
        }

        expected_start_tc.AddOffset(mGroupSegments[i]->GetDuration(), mGroupSegments[i]->GetSampleRate());
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
                MXFSequenceTrackReader *seq_track = dynamic_cast<MXFSequenceTrackReader*>(mTrackReaders[k]);
                if (seq_track->GetNumSegments() != i)
                    continue; // incomplete track or new segment already appended

                // skip tracks where sequence duration != first appended track's sequence duration
                if (first_extended_seq_track) {
                    int64_t seq_track_duration = convert_duration(seq_track->GetSampleRate(),
                                                                  seq_track->GetDuration(),
                                                                  mSampleRate,
                                                                  ROUND_AUTO);
                    int64_t group_track_duration = convert_duration(group_track->GetSampleRate(),
                                                                    group_track->GetDuration(),
                                                                    mSampleRate,
                                                                    ROUND_AUTO);
                    int64_t first_seq_track_duration = convert_duration(first_extended_seq_track->GetSampleRate(),
                                                                        first_extended_seq_track->GetDuration(),
                                                                        mSampleRate,
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
            return false;
        }
    }


    // delete incomplete tracks
    for (i = 0; i < mTrackReaders.size(); i++) {
        MXFSequenceTrackReader *seq_track = dynamic_cast<MXFSequenceTrackReader*>(mTrackReaders[i]);
        if (seq_track->GetNumSegments() != mGroupSegments.size()) {
            if (check_is_complete) {
                log_error("Incomplete track sequence\n");
                return false;
            }

            // first disable track in groups
            mTrackReaders[i]->SetEnable(false);

            delete mTrackReaders[i];
            mTrackReaders.erase(mTrackReaders.begin() + i);
            i--;
        }
    }
    if (mTrackReaders.empty()) {
        log_error("All tracks in sequence are incomplete\n");
        return false;
    }


    // get the segment offsets
    mSegmentOffsets.push_back(0);
    for (i = 1; i < mGroupSegments.size(); i++) {
        int64_t offset = mSegmentOffsets[i - 1] + mGroupSegments[i - 1]->GetDuration();
        mSegmentOffsets.push_back(offset);
    }


    // extract the sample sequences for each group
    for (i = 0; i < mGroupSegments.size(); i++) {
        vector<uint32_t> sample_sequence;
        if (!get_sample_sequence(mSampleRate, mGroupSegments[i]->GetSampleRate(), &sample_sequence)) {
            mxfRational group_sample_rate = mGroupSegments[i]->GetSampleRate();
            log_error("Incompatible sequence sample rate (%d/%d) and group sample rate (%d/%d)\n",
                      mSampleRate.numerator, mSampleRate.denominator,
                      group_sample_rate.numerator, group_sample_rate.denominator);
            return false;
        }

        mSampleSequences.push_back(sample_sequence);

        int64_t sequence_size = 0;
        size_t j;
        for (j = 0; j < sample_sequence.size(); j++)
            sequence_size += sample_sequence[j];
        mSampleSequenceSizes.push_back(sequence_size);
    }


    // sequence duration is the sum of segment durations
    mDuration = 0;
    for (i = 0; i < mGroupSegments.size(); i++)
        mDuration += CONVERT_GROUP_DUR(mGroupSegments[i]->GetDuration());


    // set default group sequence read limits
    SetReadLimits();

    return true;
}

void MXFSequenceReader::UpdateReadLimits()
{
    mReadStartPosition = 0;
    mReadEndPosition = 0;

    if (mGroupSegments.empty())
        return;

    // get read start position from first enabled segment
    int64_t read_duration = 0;
    size_t i;
    for (i = 0; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i]->GetReadStartPosition() > DISABLED_SEG_READ_LIMIT)
            break;
        read_duration += mGroupSegments[i]->GetDuration();
    }
    if (i >= mGroupSegments.size())
        return; // nothing enabled
    mReadStartPosition = read_duration + mGroupSegments[i]->GetReadStartPosition();

    // get read end position from the last enabled segment
    for (; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i]->GetReadEndPosition() <= DISABLED_SEG_READ_LIMIT)
            break;
        read_duration += mGroupSegments[i]->GetDuration();
    }
    if (i == 0) {
        mReadEndPosition = mGroupSegments[0]->GetReadEndPosition();
    } else {
        mReadEndPosition = read_duration - mGroupSegments[i - 1]->GetDuration() +
                                mGroupSegments[i - 1]->GetReadEndPosition();
    }
}

void MXFSequenceReader::SetReadLimits()
{
    SetReadLimits(GetMaxPrecharge(0, true), mDuration + GetMaxRollout(mDuration - 1, true), true);
}

void MXFSequenceReader::SetReadLimits(int64_t start_position, int64_t end_position, bool seek_start_position)
{
    MXFGroupReader *start_segment, *end_segment;
    size_t segment_index;
    int64_t start_segment_position, end_segment_position;
    GetSegmentPosition(start_position, &start_segment, &segment_index, &start_segment_position);
    GetSegmentPosition(end_position, &end_segment, &segment_index, &end_segment_position);

    if (start_segment == end_segment) {
        start_segment->SetReadLimits(start_segment_position, end_segment_position, false);
    } else {
        // end == start_segment->GetDuration() is safe because the start segment has 0 rollout
        start_segment->SetReadLimits(start_segment_position, start_segment->GetDuration(), false);
        // start == 0 is safe because the end segment has 0 pre-charge
        end_segment->SetReadLimits(0, end_segment_position, false);
    }

    // effectively disable segments before the start segment and after the end segment
    size_t i;
    for (i = 0; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i] == start_segment)
            break;

        mGroupSegments[i]->SetReadLimits(DISABLED_SEG_READ_LIMIT, DISABLED_SEG_READ_LIMIT, false);
    }
    for (; i < mGroupSegments.size(); i++) {
        if (mGroupSegments[i] == end_segment)
            break;
    }
    for (i = i + 1; i < mGroupSegments.size(); i++)
        mGroupSegments[i]->SetReadLimits(DISABLED_SEG_READ_LIMIT, DISABLED_SEG_READ_LIMIT, false);


    UpdateReadLimits();
    for (i = 0; i < mTrackReaders.size(); i++)
        dynamic_cast<MXFSequenceTrackReader*>(mTrackReaders[i])->UpdateReadLimits();


    if (seek_start_position)
        Seek(start_position);
}

uint32_t MXFSequenceReader::Read(uint32_t num_samples, bool is_top)
{
    if (!IsEnabled())
        return 0;

    if (is_top)
        SetNextFramePosition(mPosition);

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

    return total_num_read;
}

void MXFSequenceReader::Seek(int64_t position)
{
    IM_CHECK(!mGroupSegments.empty());

    MXFGroupReader *segment;
    size_t segment_index;
    int64_t segment_position;
    GetSegmentPosition(position, &segment, &segment_index, &segment_position);

    segment->Seek(segment_position);

    mPosition = position;
}

int16_t MXFSequenceReader::GetMaxPrecharge(int64_t position, bool limit_to_available) const
{
    IM_CHECK(!mGroupSegments.empty());

    size_t i = 0;
    return mGroupSegments[i]->GetMaxPrecharge(CONVERT_SEQ_POS(position), limit_to_available);
}

int16_t MXFSequenceReader::GetMaxRollout(int64_t position, bool limit_to_available) const
{
    IM_CHECK(!mGroupSegments.empty());

    size_t i = mGroupSegments.size() - 1;
    return mGroupSegments[i]->GetMaxRollout(CONVERT_SEQ_POS(position), limit_to_available);
}

bool MXFSequenceReader::HaveFixedLeadFillerOffset() const
{
    IM_CHECK(!mGroupSegments.empty());
    return mGroupSegments[0]->HaveFixedLeadFillerOffset();
}

int64_t MXFSequenceReader::GetFixedLeadFillerOffset() const
{
    IM_CHECK(!mGroupSegments.empty());
    return mGroupSegments[0]->GetFixedLeadFillerOffset();
}

MXFTrackReader* MXFSequenceReader::GetTrackReader(size_t track_index) const
{
    IM_CHECK(track_index < mTrackReaders.size());
    return mTrackReaders[track_index];
}

bool MXFSequenceReader::IsEnabled() const
{
    IM_CHECK(!mGroupSegments.empty());
    return mGroupSegments[0]->IsEnabled();
}

void MXFSequenceReader::SetNextFramePosition(int64_t position)
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetFrameBuffer()->SetNextFramePosition(position);
    }
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

        Timecode start_timecode = group_readers[index]->GetPlayoutTimecode(- group_readers[index]->GetReadStartPosition());

        if (i > 0 && start_timecode != expected_start_timecode) {
            if (seq_start_index == (size_t)(-1))
                seq_start_index = index;
            else
                return false;
        }

        expected_start_timecode = start_timecode;
        expected_start_timecode.AddOffset(group_readers[index]->GetDuration(), group_readers[index]->GetSampleRate());
    }

    *seq_start_index_out = (seq_start_index == (size_t)(-1) ? 0 : seq_start_index);
    return true;
}

void MXFSequenceReader::GetSegmentPosition(int64_t position, MXFGroupReader **segment, size_t *segment_index,
                                           int64_t *segment_position) const
{
    IM_CHECK(!mGroupSegments.empty());

    size_t i;
    for (i = 0; i < mSegmentOffsets.size(); i++) {
        if (position < mSegmentOffsets[i])
            break;
    }

    if (i == 0) {
        *segment = mGroupSegments[0];
        *segment_index = 0;
        *segment_position = position;
    } else {
        *segment = mGroupSegments[i - 1];
        *segment_index = i - 1;
        *segment_position = position - mSegmentOffsets[i - 1];
    }
}

