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

#define __STDC_FORMAT_MACROS

#include <algorithm>

#include <im/mxf_reader/MXFFileReader.h>
#include <im/mxf_helper/PictureMXFDescriptorHelper.h>
#include <im/MXFUtils.h>
#include <im/Utils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;


#define TO_ESS_READER_POS(pos)      (pos + mOrigin)
#define FROM_ESS_READER_POS(pos)    (pos - mOrigin)

#define CONVERT_EXTERNAL_DUR(dur)   convert_duration_lower(dur, mExternalSampleSequences[i], mExternalSampleSequenceSizes[i])
#define CONVERT_INTERNAL_POS(pos)   convert_position_higher(pos, mExternalSampleSequences[i], mExternalSampleSequenceSizes[i])
#define CONVERT_EXTERNAL_POS(pos)   convert_position_lower(pos, mExternalSampleSequences[i], mExternalSampleSequenceSizes[i])



static const char *RESULT_STRINGS[] =
{
    "success",
    "could not open file",
    "invalid file",
    "not supported",
    "header metadata not found",
    "invalid header metadata",
    "no essence available",
    "no essence index table",
    "incomplete index table",
    "general error",
};

#define RESULT_STRINGS_SIZE     (sizeof(RESULT_STRINGS) / sizeof(char*))

#define THROW_RESULT(result) \
{ \
    log_warn("Open error '%s' near %s:%d\n", #result, __FILE__, __LINE__); \
    throw result; \
}



static bool compare_track_reader(const MXFTrackReader *left_reader, const MXFTrackReader *right_reader)
{
    const MXFTrackInfo *left = left_reader->GetTrackInfo();
    const MXFTrackInfo *right = right_reader->GetTrackInfo();

    // data kind
    if (left->is_picture != right->is_picture)
        return left->is_picture;

    // track number
    if (left->material_track_number != 0) {
        return right->material_track_number == 0 ||
               left->material_track_number < right->material_track_number;
    }
    if (right->material_track_number != 0)
        return false;

    // track id
    if (left->material_track_id != 0) {
        return right->material_track_id == 0 ||
               left->material_track_id < right->material_track_id;
    }
    return right->material_track_id == 0;
}



MXFFileReader::OpenResult MXFFileReader::Open(string filename, MXFPackageResolver *resolver,
                                              bool resolver_take_ownership, MXFFileReader **file_reader)
{
    File *file = 0;
    try
    {
        file = File::openRead(filename);

        OpenResult result = Open(file, filename, resolver, resolver_take_ownership, file_reader);
        if (result != MXF_RESULT_SUCCESS)
            delete file;

        return result;
    }
    catch (...)
    {
        delete file;
        return MXF_RESULT_OPEN_FAIL;
    }
}

MXFFileReader::OpenResult MXFFileReader::Open(File *file, string filename, MXFPackageResolver *resolver,
                                              bool resolver_take_ownership, MXFFileReader **file_reader)
{
    IM_ASSERT(MXF_RESULT_FAIL + 1 == RESULT_STRINGS_SIZE);

    try
    {
        // read the header partition pack and check the operational pattern
        if (!file->readHeaderPartition())
            THROW_RESULT(MXF_RESULT_INVALID_FILE);
        Partition &header_partition = file->getPartition(0);

        if (!is_op_atom(header_partition.getOperationalPattern()) &&
            !is_op_1a(header_partition.getOperationalPattern()) &&
            !is_op_1b(header_partition.getOperationalPattern()))
        {
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        }

        *file_reader = new MXFFileReader(filename, file, resolver, resolver_take_ownership);

        return MXF_RESULT_SUCCESS;
    }
    catch (const OpenResult &ex)
    {
        if (resolver_take_ownership)
            delete resolver;
        return ex;
    }
    catch (const IMException &ex)
    {
        log_error("Exception: %s\n", ex.what());
        if (resolver_take_ownership)
            delete resolver;
        return MXF_RESULT_FAIL;
    }
    catch (...)
    {
        if (resolver_take_ownership)
            delete resolver;
        return MXF_RESULT_FAIL;
    }
}

string MXFFileReader::ResultToString(OpenResult result)
{
    size_t index = (size_t)(result);
    IM_ASSERT(index < RESULT_STRINGS_SIZE);

    return RESULT_STRINGS[index];
}


MXFFileReader::MXFFileReader(string filename, File *file, MXFPackageResolver *resolver, bool resolver_take_ownership)
: MXFReader()
{
    mFilename = filename;
    mFile = file;
    mPackageResolver = resolver;
    mOwnPackageResolver = (resolver ? resolver_take_ownership : false);
    mHeaderMetadata = 0;
    mDataModel = 0;
    mIsClipWrapped = false;
    mBodySID = 0;
    mIndexSID = 0;
    mOrigin = 0;
    mReadStartPosition = 0;
    mReadEndPosition = 0;
    mEssenceReader = 0;
    Partition &header_partition = file->getPartition(0);

    try
    {
        IM_CHECK(mAbsoluteURI.ParseFilename(filename));
        if (mAbsoluteURI.IsRelative()) {
            URI base_uri;
            IM_CHECK(base_uri.ParseDirectory(get_abs_cwd()));
            mAbsoluteURI.MakeAbsolute(base_uri);
        }

        // TODO: require a table that maps essence container labels to wrapping type

        // guess the wrapping type based on the OP
        mIsClipWrapped = is_op_atom(header_partition.getOperationalPattern());

        // change frame wrapped guess if file is op1a containing clip wrapped pcm audio
        if (!mIsClipWrapped) {
            vector<mxfUL> essence_labels = header_partition.getEssenceContainers();
            size_t i;
            for (i = 0; i < essence_labels.size(); i++) {
                if (mxf_equals_ul_mod_regver(&essence_labels[i], &MXF_EC_L(BWFClipWrapped)) ||
                    mxf_equals_ul_mod_regver(&essence_labels[i], &MXF_EC_L(AES3ClipWrapped)))
                {
                    mIsClipWrapped = true;
                    break;
                }
            }
        }


        mDataModel = new DataModel();
        mHeaderMetadata = new AvidHeaderMetadata(mDataModel);

        // find last partition with header metadata

        if (!mFile->readPartitions())
            log_warn("Failed to read all partitions. File may be incomplete or invalid\n");

        const vector<Partition*> &partitions = mFile->getPartitions();
        Partition *metadata_partition = 0;
        size_t i;
        for (i = partitions.size(); i > 0 ; i--) {
            if (partitions[i - 1]->getHeaderByteCount() > 0) {
                metadata_partition = partitions[i - 1];
                break;
            }
        }
        if (!metadata_partition)
            THROW_RESULT(MXF_RESULT_NO_HEADER_METADATA);

        // read the header metadata

        mxfKey key;
        uint8_t llen;
        uint64_t len;

        mFile->seek(metadata_partition->getThisPartition(), SEEK_SET);
        mFile->readKL(&key, &llen, &len);
        mFile->skip(len);
        mFile->readNextNonFillerKL(&key, &llen, &len);
        IM_CHECK(mxf_is_header_metadata(&key));
        mHeaderMetadata->read(mFile, metadata_partition, &key, llen, len);

        // default package resolver
        if (!mPackageResolver) {
            mPackageResolver = new MXFDefaultPackageResolver(this);
            mOwnPackageResolver = true;
        }

        // process header metadata
        ProcessMetadata(metadata_partition);
        if (mTrackReaders.empty())
            THROW_RESULT(MXF_RESULT_NO_ESSENCE);

        if (mIsClipWrapped && mInternalTrackReaders.size() > 1)
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED); // only support single track for clip-wrapped essence

        // set clip sample rate if not set (ie. essence is external)
        if (mSampleRate.numerator == 0) {
            IM_ASSERT(mInternalTrackReaders.empty());
            // the lowest external sample rate is the clip sample rate
            float lowest_sample_rate = 1000000.0;
            size_t i;
            for (i = 0; i < mTrackReaders.size(); i++) {
                float track_sample_rate = mTrackReaders[i]->GetSampleRate().numerator /
                                            (float)mTrackReaders[i]->GetSampleRate().denominator;
                if (track_sample_rate < lowest_sample_rate) {
                    mSampleRate = mTrackReaders[i]->GetSampleRate();
                    lowest_sample_rate = track_sample_rate;
                }
            }
            IM_CHECK(mSampleRate.numerator != 0);
        } else {
            IM_ASSERT(!mInternalTrackReaders.empty());
        }

        // extract the sample sequences for each external reader
        for (i = 0; i < mExternalReaders.size(); i++) {
            vector<uint32_t> sample_sequence;
            if (!get_sample_sequence(mSampleRate, mExternalReaders[i]->GetSampleRate(), &sample_sequence)) {
                mxfRational external_sample_rate = mExternalReaders[i]->GetSampleRate();
                log_error("Incompatible clip sample rate (%d/%d) for referenced file (%d/%d)\n",
                          mSampleRate.numerator, mSampleRate.denominator,
                          external_sample_rate.numerator, external_sample_rate.denominator);
                THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
            }

            mExternalSampleSequences.push_back(sample_sequence);

            int64_t sequence_size = 0;
            size_t j;
            for (j = 0; j < sample_sequence.size(); j++)
                sequence_size += sample_sequence[j];
            mExternalSampleSequenceSizes.push_back(sequence_size);
        }

        // clip duration is the minimum duration (tracks should really have equal duration)
        mDuration = -1;
        for (i = 0; i < mInternalTrackReaders.size(); i++) {
            int64_t track_duration = convert_duration(mInternalTrackReaders[i]->GetTrackInfo()->edit_rate,
                                                      mInternalTrackReaders[i]->GetTrackInfo()->duration,
                                                      mSampleRate,
                                                      ROUND_AUTO);
            if (mDuration < 0 || track_duration < mDuration)
                mDuration = track_duration;
        }
        for (i = 0; i < mExternalReaders.size(); i++) {
            int64_t internal_duration = CONVERT_EXTERNAL_DUR(mExternalReaders[i]->GetDuration());
            if (mDuration < 0 || internal_duration < mDuration)
                mDuration = internal_duration;
        }

        // force external readers to have the clip's duration
        for (i = 0; i < mExternalReaders.size(); i++)
            mExternalReaders[i]->ForceDuration(CONVERT_INTERNAL_POS(mDuration));

        // disable unused external tracks
        for (i = 0; i < mExternalReaders.size(); i++) {
            size_t j;
            for (j = 0; j < mExternalReaders[i]->GetNumTrackReaders(); j++) {
                size_t k;
                for (k = 0; k < mExternalTrackReaders.size(); k++) {
                    if (mExternalTrackReaders[k] == mExternalReaders[i]->GetTrackReader(j))
                        break;
                }
                if (k >= mExternalTrackReaders.size())
                    mExternalReaders[i]->GetTrackReader(j)->SetEnable(false);
            }
        }

        // create internal essence reader
        if (!mInternalTrackReaders.empty()) {
            mEssenceReader = new EssenceReader(this);
            IM_CHECK(mEssenceReader->GetEditRate() == mSampleRate);
        }

        // do some checks, set read limits to [0+precharge, duration+rollout) and seek to start (0+precharge)
        if (mEssenceReader && mEssenceReader->GetIndexedDuration() < mDuration) {
            log_warn("Essence index duration %"PRId64" is less than track duration %"PRId64"\n",
                     mEssenceReader->GetIndexedDuration(), mDuration);
        }
        if (GetMaxPrecharge(0, true) != GetMaxPrecharge(0, false)) {
            log_warn("Possibly not enough precharge available (available=%d, required=%d)\n",
                     GetMaxPrecharge(0, true), GetMaxPrecharge(0, false));
        }
        if (GetMaxRollout(mDuration - 1, true) != GetMaxRollout(mDuration - 1, false)) {
            log_warn("Possibly not enough rollout available (available=%d, required=%d)\n",
                     GetMaxRollout(mDuration - 1, true), GetMaxRollout(mDuration - 1, false));
        }
        SetReadLimits();
    }
    catch (...)
    {
        delete mPackageResolver;
        delete mEssenceReader;
        // mFile ownership reverts back to caller
        delete mHeaderMetadata;
        delete mDataModel;
        size_t i;
        for (i = 0; i < mInternalTrackReaders.size(); i++)
            delete mInternalTrackReaders[i];
        // mPackageResolver owns external readers
        throw;
    }
}

MXFFileReader::~MXFFileReader()
{
    if (mOwnPackageResolver)
        delete mPackageResolver;
    delete mEssenceReader;
    delete mFile;
    delete mHeaderMetadata;
    delete mDataModel;

    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++)
        delete mInternalTrackReaders[i];

    // mPackageResolver owns external readers
}

void MXFFileReader::SetReadLimits()
{
    SetReadLimits(GetMaxPrecharge(0, true), mDuration + GetMaxRollout(mDuration - 1, true), true);
}

void MXFFileReader::SetReadLimits(int64_t start_position, int64_t end_position, bool seek_to_start)
{
    mReadStartPosition = start_position;
    mReadEndPosition = end_position;

    if (InternalIsEnabled())
        mEssenceReader->SetReadLimits(TO_ESS_READER_POS(start_position), TO_ESS_READER_POS(end_position));

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        int64_t external_start_position = CONVERT_INTERNAL_POS(start_position);
        int64_t external_end_position = CONVERT_INTERNAL_POS(end_position);
        mExternalReaders[i]->SetReadLimits(external_start_position, external_end_position, false /* seek done below */);
    }

    if (seek_to_start)
        Seek(start_position);
}

uint32_t MXFFileReader::Read(uint32_t num_samples, bool is_top)
{
    int64_t current_position = GetPosition();

    if (is_top)
        SetNextFramePosition(current_position);

    uint32_t max_num_read = 0;
    if (InternalIsEnabled())
        max_num_read = mEssenceReader->Read(num_samples);

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        int64_t external_current_position = CONVERT_INTERNAL_POS(current_position);

        // ensure external reader is in sync
        if (mExternalReaders[i]->GetPosition() != external_current_position)
            mExternalReaders[i]->Seek(external_current_position);


        uint32_t num_external_samples = (uint32_t)convert_duration_higher(num_samples,
                                                                          current_position,
                                                                          mExternalSampleSequences[i],
                                                                          mExternalSampleSequenceSizes[i]);

        uint32_t external_num_read = mExternalReaders[i]->Read(num_external_samples, false);

        uint32_t internal_num_read = (uint32_t)convert_duration_lower(external_num_read,
                                                                      external_current_position,
                                                                      mExternalSampleSequences[i],
                                                                      mExternalSampleSequenceSizes[i]);

        if (internal_num_read > max_num_read)
            max_num_read = internal_num_read;
    }

    IM_ASSERT(max_num_read <= num_samples);
    return max_num_read;
}

void MXFFileReader::Seek(int64_t position)
{
    if (InternalIsEnabled())
        mEssenceReader->Seek(TO_ESS_READER_POS(position));

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        mExternalReaders[i]->Seek(CONVERT_INTERNAL_POS(position));
    }
}

int64_t MXFFileReader::GetPosition() const
{
    int64_t position = 0;
    if (InternalIsEnabled()) {
        position = FROM_ESS_READER_POS(mEssenceReader->GetPosition());
    } else {
        size_t i;
        for (i = 0; i < mExternalReaders.size(); i++) {
            if (!mExternalReaders[i]->IsEnabled())
                continue;

            position = CONVERT_EXTERNAL_POS(mExternalReaders[i]->GetPosition());
            break;
        }
    }

    return position;
}

int16_t MXFFileReader::GetMaxPrecharge(int64_t position, bool limit_to_available) const
{
    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    int16_t precharge = 0;
    if (InternalIsEnabled())
        precharge = GetInternalPrecharge(target_position, limit_to_available);

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        int16_t ext_reader_precharge = mExternalReaders[i]->GetMaxPrecharge(CONVERT_INTERNAL_POS(target_position),
                                                                            limit_to_available);
        if (ext_reader_precharge < precharge) {
            IM_CHECK_M(mExternalReaders[i]->GetSampleRate() == mSampleRate,
                       ("Currently only support precharge in external reader if "
                        "external reader sample rate equals group sample rate"));
            precharge = ext_reader_precharge;
        }
    }

    return precharge;
}

int16_t MXFFileReader::GetMaxRollout(int64_t position, bool limit_to_available) const
{
    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    int16_t rollout = 0;
    if (InternalIsEnabled())
        rollout = GetInternalRollout(target_position, limit_to_available);

    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (!mExternalReaders[i]->IsEnabled())
            continue;

        int16_t ext_reader_rollout = mExternalReaders[i]->GetMaxRollout(CONVERT_INTERNAL_POS(target_position),
                                                                        limit_to_available);
        if (ext_reader_rollout > rollout) {
            IM_CHECK_M(mExternalReaders[i]->GetSampleRate() == mSampleRate,
                       ("Currently only support rollout in external reader if "
                        "external reader sample rate equals group sample rate"));
            rollout = ext_reader_rollout;
        }
    }

    return rollout;
}

bool MXFFileReader::HaveFixedLeadFillerOffset() const
{
    int64_t fixed_offset = 0;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        // note that edit_rate and lead_filler_offset are from this MXF file's material package
        int64_t offset = convert_position(mTrackReaders[i]->GetTrackInfo()->edit_rate,
                                          mTrackReaders[i]->GetTrackInfo()->lead_filler_offset,
                                          mSampleRate,
                                          ROUND_UP);
        if (i == 0)
            fixed_offset = offset;
        else if (fixed_offset != offset)
            return false;
    }

    return true;
}

int64_t MXFFileReader::GetFixedLeadFillerOffset() const
{
    int64_t fixed_offset = 0;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        // note that edit_rate and lead_filler_offset are from this MXF file's material package
        int64_t offset = convert_position(mTrackReaders[i]->GetTrackInfo()->edit_rate,
                                          mTrackReaders[i]->GetTrackInfo()->lead_filler_offset,
                                          mSampleRate,
                                          ROUND_UP);
        if (i == 0)
            fixed_offset = offset;
        else if (fixed_offset != offset)
            return 0; // default to 0 if HaveFixedLeadFillerOffset returns false
    }

    return fixed_offset;
}

MXFTrackReader* MXFFileReader::GetTrackReader(size_t index) const
{
    IM_CHECK(index < mTrackReaders.size());
    return mTrackReaders[index];
}

bool MXFFileReader::IsEnabled() const
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            return true;
    }

    return false;
}

void MXFFileReader::SetNextFramePosition(int64_t position)
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEnabled())
            mTrackReaders[i]->GetFrameBuffer()->SetNextFramePosition(position);
    }
}

void MXFFileReader::ProcessMetadata(Partition *partition)
{
    Preface *preface = mHeaderMetadata->getPreface();

    // create track readers for each picture or sound material package track

    MaterialPackage *material_package = preface->findMaterialPackage();
    IM_CHECK(material_package);
    mMaterialPackageUID = material_package->getPackageUID();
    if (material_package->haveName())
        mMaterialPackageName = material_package->getName();

    vector<SourcePackage*> file_source_packages = preface->findFileSourcePackages();
    if (file_source_packages.empty())
        THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);

    Track *infile_mp_track = 0;
    vector<GenericTrack*> mp_tracks = material_package->getTracks();
    size_t i;
    for (i = 0; i < mp_tracks.size(); i++) {
        Track *mp_track = dynamic_cast<Track*>(mp_tracks[i]);
        if (!mp_track)
            continue;

        // skip if not picture or sound
        StructuralComponent *track_sequence = mp_track->getSequence();
        mxfUL data_def = track_sequence->getDataDefinition();
        bool is_picture = mxf_is_picture(&data_def);
        bool is_sound = mxf_is_sound(&data_def);
        if (!is_picture && !is_sound)
            continue;

        // check material package track origin is 0
        IM_CHECK(mp_track->getOrigin() == 0);

        // skip if not a Sequence->SourceClip or SourceClip
        int64_t lead_filler_offset = 0;
        Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
        SourceClip *mp_source_clip = dynamic_cast<SourceClip*>(track_sequence);
        if (sequence) {
            vector<StructuralComponent*> components = sequence->getStructuralComponents();
            size_t j;
            for (j = 0; j < components.size(); j++) {
                mp_source_clip = dynamic_cast<SourceClip*>(components[j]);
                if (mp_source_clip) {
                    break;
                } else {
                    // lead Filler segments, eg. used in P2 spanned clips
                    IM_CHECK(mxf_equals_key(components[j]->getKey(), &MXF_SET_K(Filler)));
                    lead_filler_offset += components[j]->getDuration();
                }
            }
        }
        if (!mp_source_clip)
            continue;

        // currently only support 0 start position
        IM_CHECK_M(mp_source_clip->getStartPosition() == 0,
                   ("Non-zero material package source clip start position is not yet supported"));

        // skip if could not resolve the source clip
        vector<ResolvedPackage> resolved_packages = mPackageResolver->ResolveSourceClip(mp_source_clip);
        if (resolved_packages.empty())
            continue;

        // require top level file source package to be described in this file
        const ResolvedPackage *resolved_package = 0;
        size_t j;
        for (j = 0; j < resolved_packages.size(); j++) {
            if (resolved_packages[j].is_file_source_package && resolved_packages[j].file_reader == this) {
                resolved_package = &resolved_packages[j];
                break;
            }
        }
        if (!resolved_package)
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        SourcePackage *file_source_package = dynamic_cast<SourcePackage*>(resolved_package->package);
        IM_CHECK(file_source_package);

        MXFTrackReader *track_reader = 0;
        if (resolved_package->external_essence) {
            track_reader = GetExternalTrackReader(mp_source_clip, file_source_package);
            if (!track_reader)
                continue;

            // modify material package info in track info
            MXFTrackInfo *track_info = track_reader->GetTrackInfo();
            track_info->material_package_uid = material_package->getPackageUID();
            if (mp_track->haveTrackID())
                track_info->material_track_id = mp_track->getTrackID();
            else
                track_info->material_track_id = 0;
            track_info->material_track_number = mp_track->getTrackNumber();
            track_info->edit_rate = mp_track->getEditRate();
            track_info->duration = mp_source_clip->getDuration();
            track_info->lead_filler_offset = lead_filler_offset;
        } else {
            track_reader = CreateInternalTrackReader(partition, material_package, mp_track, mp_source_clip,
                                                     is_picture, resolved_package);
            track_reader->GetTrackInfo()->lead_filler_offset = lead_filler_offset;
            IM_ASSERT(track_reader);
        }
        mTrackReaders.push_back(track_reader);

        // this material package track will be used to extract timecodes later on
        if (!infile_mp_track)
            infile_mp_track = mp_track;
    }

    // order tracks by material track number / id
    stable_sort(mTrackReaders.begin(), mTrackReaders.end(), compare_track_reader);


    // extract start timecodes and physical source package name

    GetStartTimecodes(preface, material_package, infile_mp_track);


    // get the body and index SIDs linked to (singular) internal essence file source package

    if (!mInternalTrackReaders.empty()) {
        ContentStorage *content_storage = preface->getContentStorage();
        if (!content_storage->haveEssenceContainerData())
            THROW_RESULT(MXF_RESULT_NO_ESSENCE);
        vector<EssenceContainerData*> ess_data = content_storage->getEssenceContainerData();
        if (ess_data.empty())
            THROW_RESULT(MXF_RESULT_NO_ESSENCE);

        // only support single essence container (not OP-1B with multiple internal essence file source packages)
        if (ess_data.size() != 1)
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
        EssenceContainerData *single_ess_data = ess_data[0];

        mxfUMID linked_package_uid = single_ess_data->getLinkedPackageUID();
        for (i = 0; i < mInternalTrackReaders.size(); i++) {
            if (!mxf_equals_umid(&mInternalTrackReaders[i]->GetTrackInfo()->file_package_uid, &linked_package_uid))
                THROW_RESULT(MXF_RESULT_NO_ESSENCE);
        }

        mBodySID = single_ess_data->getBodySID();
        if (mBodySID == 0)
            THROW_RESULT(MXF_RESULT_NO_ESSENCE);

        // require index table
        mIndexSID = 0;
        if (single_ess_data->haveIndexSID())
            mIndexSID = single_ess_data->getIndexSID();
        if (mIndexSID == 0)
            THROW_RESULT(MXF_RESULT_NO_ESSENCE_INDEX);
    }
}

MXFTrackReader* MXFFileReader::CreateInternalTrackReader(Partition *partition, MaterialPackage *material_package,
                                                         Track *mp_track, SourceClip *mp_source_clip,
                                                         bool is_picture, const ResolvedPackage *resolved_package)
{
    SourcePackage *file_source_package = dynamic_cast<SourcePackage*>(resolved_package->package);
    IM_CHECK(file_source_package);

    Track *fsp_track = dynamic_cast<Track*>(resolved_package->generic_track);
    IM_CHECK(fsp_track);


    // get track origin (pre-charge)
    int64_t origin = fsp_track->getOrigin();
    IM_CHECK_M(origin >= 0,
               ("Negative track Origin %"PRId64" in top-level file Source Package not supported", origin));
    IM_CHECK_M(mInternalTrackReaders.empty() || mOrigin == origin,
               ("Different track Origins (%"PRId64" != %"PRId64") in top-level file Source Package", mOrigin, origin));
    mOrigin = origin;


    // get the file descriptor

    FileDescriptor *file_desc = 0;
    MultipleDescriptor *mult_desc = dynamic_cast<MultipleDescriptor*>(file_source_package->getDescriptor());
    if (mult_desc) {
        IM_CHECK(fsp_track->haveTrackID());
        uint32_t fsp_track_id = fsp_track->getTrackID();

        vector<GenericDescriptor*> child_descs = mult_desc->getSubDescriptorUIDs();
        size_t i;
        for (i = 0; i < child_descs.size(); i++) {
            FileDescriptor *child_file_desc = dynamic_cast<FileDescriptor*>(child_descs[i]);
            if (!child_file_desc || !child_file_desc->haveLinkedTrackID())
                continue;
            if (child_file_desc->getLinkedTrackID() == fsp_track_id) {
                file_desc = child_file_desc;
                break;
            }
        }

        mSampleRate = mult_desc->getSampleRate();
    } else {
        file_desc = dynamic_cast<FileDescriptor*>(file_source_package->getDescriptor());
        IM_CHECK(file_desc);

        mSampleRate = file_desc->getSampleRate();
    }
    IM_CHECK(file_desc);


    // fill in track info

    MXFTrackInfo *track_info = 0;
    MXFPictureTrackInfo *picture_track_info = 0;
    MXFSoundTrackInfo *sound_track_info = 0;
    if (is_picture) {
        picture_track_info = new MXFPictureTrackInfo();
        track_info = picture_track_info;
    } else {
        sound_track_info = new MXFSoundTrackInfo();
        track_info = sound_track_info;
    }

    track_info->material_package_uid = material_package->getPackageUID();
    if (mp_track->haveTrackID())
        track_info->material_track_id = mp_track->getTrackID();
    track_info->material_track_number = mp_track->getTrackNumber();
    track_info->file_package_uid = file_source_package->getPackageUID();
    track_info->edit_rate = mp_track->getEditRate();
    track_info->duration = mp_source_clip->getDuration();
    if (fsp_track->haveTrackID())
        track_info->file_track_id = fsp_track->getTrackID();
    track_info->file_track_number = fsp_track->getTrackNumber();
    IM_CHECK(track_info->file_track_number != 0);

    // use the essence container label in the partition to workaround issue with Avid files where
    // the essence container label in the descriptor is a generic KLV label
    if (is_op_atom(partition->getOperationalPattern())) {
        vector<mxfUL> ec_labels = partition->getEssenceContainers();
        if (ec_labels.size() == 1)
            track_info->essence_container_label = ec_labels[0];
    }

    if (is_picture)
        ProcessPictureDescriptor(file_desc, picture_track_info);
    else
        ProcessSoundDescriptor(file_desc, sound_track_info);


    MXFFileTrackReader *track_reader = new MXFFileTrackReader(this, mInternalTrackReaders.size(), track_info,
                                                              file_desc, file_source_package);
    mInternalTrackReaders.push_back(track_reader);
    mInternalTrackReaderNumberMap[mInternalTrackReaders.back()->GetTrackInfo()->file_track_number] = track_reader;

    return mInternalTrackReaders.back();
}

MXFTrackReader* MXFFileReader::GetExternalTrackReader(SourceClip *mp_source_clip, SourcePackage *file_source_package)
{
    // resolve package using available locators
    GenericDescriptor *descriptor = file_source_package->getDescriptor();
    vector<Locator*> locators;
    if (descriptor->haveLocators())
        locators = descriptor->getLocators();
    vector<ResolvedPackage> resolved_packages = mPackageResolver->ResolveSourceClip(mp_source_clip, locators);
    if (resolved_packages.empty()) {
        log_warn("Failed to resolve external essence\n");
        return 0;
    }

    // require file with internal essence
    const ResolvedPackage *resolved_package = 0;
    size_t i;
    for (i = 0; i < resolved_packages.size(); i++) {
        if (resolved_packages[i].is_file_source_package && !resolved_packages[i].external_essence) {
            resolved_package = &resolved_packages[i];
            break;
        }
    }
    if (!resolved_package)
        return 0;

    MXFTrackReader *external_track_reader =
        resolved_package->file_reader->GetInternalTrackReaderById(resolved_package->track_id);
    if (!external_track_reader)
        return 0;

    // don't support tracks referenced by multiple material tracks
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i] == external_track_reader)
            THROW_RESULT(MXF_RESULT_NOT_SUPPORTED);
    }

    // add external reader if not already present
    for (i = 0; i < mExternalReaders.size(); i++) {
        if (mExternalReaders[i] == resolved_package->file_reader)
            break;
    }
    if (i >= mExternalReaders.size())
        mExternalReaders.push_back(resolved_package->file_reader);

    mExternalTrackReaders.push_back(external_track_reader);
    return external_track_reader;
}

void MXFFileReader::GetStartTimecodes(Preface *preface, MaterialPackage *material_package, Track *infile_mp_track)
{
    Timecode start_timecode;
    GenericPackage *ref_package;
    Track *ref_track;
    int64_t ref_offset;

    // try get start timecodes from the material package, file source package and physical source package
    // also get the physical source package name

    if (GetStartTimecode(material_package, 0, 0, &start_timecode))
        mMaterialStartTimecode = new Timecode(start_timecode);

    if (infile_mp_track) {
        if (GetReferencedPackage(preface, infile_mp_track, 0, FILE_SOURCE_PACKAGE_TYPE,
                                 &ref_package, &ref_track, &ref_offset))
        {
            if (GetStartTimecode(ref_package, ref_track, ref_offset, &start_timecode))
                mFileSourceStartTimecode = new Timecode(start_timecode);

            if (GetReferencedPackage(preface, ref_track, ref_offset, PHYSICAL_SOURCE_PACKAGE_TYPE,
                                     &ref_package, &ref_track, &ref_offset))
            {
                if (GetStartTimecode(ref_package, ref_track, ref_offset, &start_timecode))
                    mPhysicalSourceStartTimecode = new Timecode(start_timecode);

                if (ref_package->haveName())
                    mPhysicalSourcePackageName = ref_package->getName();
            }
        }
    }
}

bool MXFFileReader::GetStartTimecode(GenericPackage *package, Track *track, int64_t offset, Timecode *timecode)
{
    IM_ASSERT(offset == 0 || track);

    // find the timecode component in this package
    TimecodeComponent *tc_component = 0;
    vector<GenericTrack*> tracks = package->getTracks();
    size_t i;
    for (i = 0; i < tracks.size(); i++) {
        Track *track = dynamic_cast<Track*>(tracks[i]);
        if (!track)
            continue;

        StructuralComponent *track_sequence = track->getSequence();
        mxfUL data_def = track_sequence->getDataDefinition();
        if (!mxf_is_timecode(&data_def))
            continue;

        Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
        tc_component = dynamic_cast<TimecodeComponent*>(track_sequence);
        if (sequence) {
            vector<StructuralComponent*> components = sequence->getStructuralComponents();
            size_t j;
            for (j = 0; j < components.size(); j++) {
                tc_component = dynamic_cast<TimecodeComponent*>(components[j]);
                if (tc_component)
                    break;
            }
        }
        if (tc_component)
            break;
    }
    if (!tc_component)
        return false;


    int64_t tc_offset = offset;
    if (track)
        tc_offset = convert_tc_offset(track->getEditRate(), offset, tc_component->getRoundedTimecodeBase());

    timecode->Init(tc_component->getRoundedTimecodeBase(),
                   tc_component->getDropFrame(),
                   tc_component->getStartTimecode() + tc_offset);
    return true;
}

bool MXFFileReader::GetReferencedPackage(Preface *preface, Track *track, int64_t offset_in, PackageType package_type,
                                         GenericPackage **ref_package_out, Track **ref_track_out,
                                         int64_t *ref_offset_out)
{
    // get the source clip
    StructuralComponent *track_sequence = track->getSequence();
    Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
    SourceClip *source_clip = dynamic_cast<SourceClip*>(track_sequence);
    if (sequence) {
        vector<StructuralComponent*> components = sequence->getStructuralComponents();
        size_t i;
        for (i = 0; i < components.size(); i++) {
            source_clip = dynamic_cast<SourceClip*>(components[i]);
            if (source_clip)
                break;
        }
    }
    if (!source_clip)
        return false;

    // find the referenced package and timeline track
    GenericPackage *ref_package = preface->findPackage(source_clip->getSourcePackageID());
    if (!ref_package)
        return false;
    GenericTrack *ref_generic_track = ref_package->findTrack(source_clip->getSourceTrackID());
    if (!ref_generic_track)
        return false;
    Track *ref_track = dynamic_cast<Track*>(ref_generic_track);
    if (!ref_track)
        return false;

    int64_t ref_offset = convert_position(track->getEditRate(),
                                          source_clip->getStartPosition() + offset_in,
                                          ref_track->getEditRate(),
                                          ROUND_AUTO);
    ref_offset += ref_track->getOrigin();

    // check the package type and try next referenced package if wrong type
    bool type_match = false;
    if (package_type == MATERIAL_PACKAGE_TYPE) {
        type_match = (dynamic_cast<MaterialPackage*>(ref_package) != 0);
    } else {
        SourcePackage *source_package = dynamic_cast<SourcePackage*>(ref_package);
        if (source_package && source_package->haveDescriptor()) {
            GenericDescriptor *descriptor = source_package->getDescriptor();
            if (package_type == FILE_SOURCE_PACKAGE_TYPE)
                type_match = (dynamic_cast<FileDescriptor*>(descriptor) != 0);
            else
                type_match = mDataModel->isSubclassOf(descriptor, &MXF_SET_K(PhysicalDescriptor));
        }
    }
    if (!type_match) {
        return GetReferencedPackage(preface, ref_track, ref_offset, package_type,
                                    ref_package_out, ref_track_out, ref_offset_out);
    }


    *ref_package_out = ref_package;
    *ref_track_out = ref_track;
    *ref_offset_out = ref_offset;
    return true;
}

void MXFFileReader::ProcessDescriptor(mxfpp::FileDescriptor *file_descriptor, MXFTrackInfo *track_info)
{
    track_info->essence_type = MXFDescriptorHelper::IsSupported(file_descriptor, track_info->essence_container_label);

    // set essence_container_label if not already set
    if (track_info->essence_container_label == g_Null_UL)
        track_info->essence_container_label = file_descriptor->getEssenceContainer();
}

void MXFFileReader::ProcessPictureDescriptor(FileDescriptor *file_descriptor, MXFPictureTrackInfo *picture_track_info)
{
    ProcessDescriptor(file_descriptor, picture_track_info);

    GenericPictureEssenceDescriptor *picture_descriptor =
        dynamic_cast<GenericPictureEssenceDescriptor*>(file_descriptor);
    IM_CHECK(picture_descriptor);

    PictureMXFDescriptorHelper *picture_helper =
        PictureMXFDescriptorHelper::Create(file_descriptor, picture_track_info->essence_container_label);
    int32_t avid_resolution_id = 0;
    if (picture_helper->HaveAvidResolutionID())
        avid_resolution_id = picture_helper->GetAvidResolutionID();
    delete picture_helper;

    if (picture_descriptor->havePictureEssenceCoding())
        picture_track_info->picture_essence_coding_label = picture_descriptor->getPictureEssenceCoding();
    if (picture_descriptor->haveSignalStandard())
        picture_track_info->signal_standard = picture_descriptor->getSignalStandard();
    if (picture_descriptor->haveFrameLayout())
        picture_track_info->frame_layout = picture_descriptor->getFrameLayout();

    // fix legacy avid frame layout values for IEC DV-25, DVBased DV-25 and DVBased DV-50
    if ((avid_resolution_id == 0x8c || avid_resolution_id == 0x8d || avid_resolution_id == 0x8e) &&
        picture_track_info->frame_layout == MXF_MIXED_FIELDS)
    {
        picture_track_info->frame_layout = MXF_SEPARATE_FIELDS;
    }

    uint32_t frame_height_factor = 1;
    if (picture_track_info->frame_layout == MXF_SEPARATE_FIELDS)
        frame_height_factor = 2; // double the field height

    if (picture_descriptor->haveStoredWidth())
        picture_track_info->stored_width = picture_descriptor->getStoredWidth();
    if (picture_descriptor->haveStoredHeight())
        picture_track_info->stored_height = frame_height_factor * picture_descriptor->getStoredHeight();

    if (picture_descriptor->haveDisplayWidth())
        picture_track_info->display_width = picture_descriptor->getDisplayWidth();
    else
        picture_track_info->display_width = picture_track_info->stored_width;
    if (picture_descriptor->haveDisplayHeight())
        picture_track_info->display_height = frame_height_factor * picture_descriptor->getDisplayHeight();
    else
        picture_track_info->display_height = picture_track_info->stored_height;

    if (picture_descriptor->haveDisplayXOffset())
        picture_track_info->display_x_offset = picture_descriptor->getDisplayXOffset();
    if (picture_descriptor->haveDisplayYOffset())
        picture_track_info->display_y_offset = frame_height_factor * picture_descriptor->getDisplayYOffset();

    if (picture_descriptor->haveAspectRatio())
        picture_track_info->aspect_ratio = picture_descriptor->getAspectRatio();

    if (picture_descriptor->haveActiveFormatDescriptor())
        picture_track_info->afd = picture_descriptor->getActiveFormatDescriptor();


    CDCIEssenceDescriptor *cdci_descriptor = dynamic_cast<CDCIEssenceDescriptor*>(file_descriptor);
    if (cdci_descriptor) {
        if (cdci_descriptor->haveComponentDepth())
            picture_track_info->component_depth = cdci_descriptor->getComponentDepth();
        if (cdci_descriptor->haveHorizontalSubsampling())
            picture_track_info->horiz_subsampling = cdci_descriptor->getHorizontalSubsampling();
        if (cdci_descriptor->haveVerticalSubsampling())
            picture_track_info->vert_subsampling = cdci_descriptor->getVerticalSubsampling();
        if (cdci_descriptor->haveColorSiting())
            picture_track_info->color_siting = cdci_descriptor->getColorSiting();

        // fix legacy avid subsampling values for DVBased DV-25
        if (avid_resolution_id == 0x8c &&
            picture_track_info->horiz_subsampling == picture_track_info->vert_subsampling)
        {
            picture_track_info->horiz_subsampling = 4;
            picture_track_info->vert_subsampling = 1;
        }
    }
}

void MXFFileReader::ProcessSoundDescriptor(FileDescriptor *file_descriptor, MXFSoundTrackInfo *sound_track_info)
{
    ProcessDescriptor(file_descriptor, sound_track_info);

    GenericSoundEssenceDescriptor *sound_descriptor =
        dynamic_cast<GenericSoundEssenceDescriptor*>(file_descriptor);
    IM_CHECK(sound_descriptor);
    
    if (sound_descriptor->haveAudioSamplingRate())
        sound_track_info->sampling_rate = sound_descriptor->getAudioSamplingRate();

    if (sound_descriptor->haveChannelCount())
        sound_track_info->channel_count = sound_descriptor->getChannelCount();

    if (sound_descriptor->haveQuantizationBits())
        sound_track_info->bits_per_sample = sound_descriptor->getQuantizationBits();

    if (sound_descriptor->haveLocked()) {
        sound_track_info->locked = sound_descriptor->getLocked();
        sound_track_info->locked_set = true;
    }
    if (sound_descriptor->haveAudioRefLevel()) {
        sound_track_info->audio_ref_level = sound_descriptor->getAudioRefLevel();
        sound_track_info->audio_ref_level_set = true;
    }
    if (sound_descriptor->haveDialNorm()) {
        sound_track_info->dial_norm = sound_descriptor->getDialNorm();
        sound_track_info->dial_norm_set = true;
    }

    WaveAudioDescriptor *wave_descriptor = dynamic_cast<WaveAudioDescriptor*>(file_descriptor);
    if (wave_descriptor) {
        sound_track_info->block_align = wave_descriptor->getBlockAlign();
        if (wave_descriptor->haveSequenceOffset())
            sound_track_info->sequence_offset = wave_descriptor->getSequenceOffset();
    } else {
        if (sound_track_info->channel_count > 0) {
            sound_track_info->block_align = sound_track_info->channel_count *
                                                (uint16_t)((sound_track_info->bits_per_sample + 7) / 8);
        } else {
            // assuming channel count 1 is better than block align 0
            sound_track_info->block_align = (sound_track_info->bits_per_sample + 7) / 8;
        }
    }
}

MXFTrackReader* MXFFileReader::GetInternalTrackReader(size_t index) const
{
    IM_CHECK(index < mInternalTrackReaders.size());
    return mInternalTrackReaders[index];
}

MXFTrackReader* MXFFileReader::GetInternalTrackReaderByNumber(uint32_t track_number) const
{
    map<uint32_t, MXFTrackReader*>::const_iterator result = mInternalTrackReaderNumberMap.find(track_number);
    if (result == mInternalTrackReaderNumberMap.end())
        return 0;

    return result->second;
}

MXFTrackReader* MXFFileReader::GetInternalTrackReaderById(uint32_t id) const
{
    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++) {
        if (mInternalTrackReaders[i]->GetTrackInfo()->file_track_id == id)
            return mInternalTrackReaders[i];
    }

    return 0;
}

void MXFFileReader::ForceDuration(int64_t duration)
{
    IM_CHECK(duration <= mDuration);
    mDuration = duration;
}

bool MXFFileReader::GetInternalIndexEntry(MXFIndexEntryExt *entry, int64_t position) const
{
    IM_ASSERT(mEssenceReader);

    return mEssenceReader->GetIndexEntry(entry, TO_ESS_READER_POS(position));
}

int16_t MXFFileReader::GetInternalPrecharge(int64_t position, bool limit_to_available) const
{
    IM_ASSERT(mEssenceReader);

    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    // no precharge if target position outside essence range
    if (FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(TO_ESS_READER_POS(target_position))) != target_position)
        return 0;

    int16_t precharge = 0;
    MXFIndexEntryExt index_entry;
    if (GetInternalIndexEntry(&index_entry, target_position)) {
        int8_t target_index_entry_offset = index_entry.temporal_offset;
        if (target_index_entry_offset != 0) {
            if (GetInternalIndexEntry(&index_entry, target_position + target_index_entry_offset))
                precharge = target_index_entry_offset + index_entry.key_frame_offset;
        } else {
            precharge = index_entry.key_frame_offset;
        }
    }

    if (precharge > 0)
        log_warn("Unexpected positive precharge value %d\n", precharge);
    else if (precharge < 0 && limit_to_available)
        precharge = FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(TO_ESS_READER_POS(target_position + precharge))) - target_position;

    return precharge < 0 ? precharge : 0;
}

int16_t MXFFileReader::GetInternalRollout(int64_t position, bool limit_to_available) const
{
    IM_ASSERT(mEssenceReader);

    int64_t target_position = position;
    if (target_position == CURRENT_POSITION_VALUE)
        target_position = GetPosition();

    // no rollout if target position outside essence range
    if (FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(TO_ESS_READER_POS(target_position))) != target_position)
        return 0;

    int16_t rollout = 0;
    MXFIndexEntryExt index_entry;
    if (GetInternalIndexEntry(&index_entry, target_position) && index_entry.temporal_offset > 0)
        rollout = index_entry.temporal_offset;

    if (rollout < 0)
        log_warn("Unexpected negative rollout value %d\n", rollout);
    else if (rollout > 0 && limit_to_available)
        rollout = FROM_ESS_READER_POS(mEssenceReader->LegitimisePosition(TO_ESS_READER_POS(target_position + rollout))) - target_position;

    return rollout > 0 ? rollout : 0;
}

bool MXFFileReader::InternalIsEnabled() const
{
    size_t i;
    for (i = 0; i < mInternalTrackReaders.size(); i++) {
        if (mInternalTrackReaders[i]->IsEnabled()) {
            IM_ASSERT(mEssenceReader);
            return true;
        }
    }

    return false;
}

