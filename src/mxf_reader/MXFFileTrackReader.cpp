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
#include <map>
#include <set>

#include <bmx/mxf_reader/MXFFileTrackReader.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/GenericStreamReader.h>
#include <bmx/mxf_helper/ADMCHNAMXFDescriptorHelper.h>
#include <bmx/essence_parser/AVCEssenceParser.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



MXFFileTrackReader::MXFFileTrackReader(MXFFileReader *file_reader, size_t track_index, MXFTrackInfo *track_info,
                                       FileDescriptor *file_descriptor, SourcePackage *file_source_package)
{
    mFileReader = file_reader;
    mTrackIndex = track_index;
    mTrackInfo = track_info;
    mFileDescriptor = file_descriptor;
    mFileSourcePackage = file_source_package;

    mIsEnabled = true;
    mFrameBuffer.SetTargetBuffer(new DefaultFrameBuffer(), true);

    mAVCIHeader = 0;
    mWaveCHNA = 0;

    // get MXFWaveChunks for each RIFFChunkReferenceSubDescriptor sub-descriptor reference
    // get a WaveCHNA from a ADM_CHNASubDescriptor
    WaveAudioDescriptor *wave_descriptor = dynamic_cast<WaveAudioDescriptor*>(file_descriptor);
    if (wave_descriptor && wave_descriptor->haveSubDescriptors()) {
        vector<SubDescriptor*> sub_descriptors = wave_descriptor->getSubDescriptors();
        size_t i;
        for (i = 0; i < sub_descriptors.size(); i++) {
            RIFFChunkReferencesSubDescriptor *chunk_ref_descriptor =
                dynamic_cast<RIFFChunkReferencesSubDescriptor*>(sub_descriptors[i]);

            if (chunk_ref_descriptor) {
                // Get the chunk descriptors associated with the File Source Package descriptor
                map<uint32_t, const RIFFChunkDefinitionSubDescriptor*> chunk_descriptors;
                MultipleDescriptor *multi_descriptor = dynamic_cast<MultipleDescriptor*>(file_source_package->getDescriptor());
                if (multi_descriptor && multi_descriptor->haveSubDescriptors()) {
                    vector<SubDescriptor*> fsp_sub_descriptors = multi_descriptor->getSubDescriptors();
                    size_t k;
                    for (k = 0; k < fsp_sub_descriptors.size(); k++) {
                        RIFFChunkDefinitionSubDescriptor *chunk_descriptor =
                            dynamic_cast<RIFFChunkDefinitionSubDescriptor*>(fsp_sub_descriptors[k]);
                        if (chunk_descriptor)
                            chunk_descriptors[chunk_descriptor->getRIFFChunkStreamID()] = chunk_descriptor;
                    }

                    // Create a MXFWaveChunk for each chunk referenced
                    vector<uint32_t> stream_ids = chunk_ref_descriptor->getRIFFChunkStreamIDsArray();
                    set<uint32_t> unique_stream_ids(stream_ids.begin(), stream_ids.end());
                    set<uint32_t>::const_iterator iter;
                    for (iter = unique_stream_ids.begin(); iter != unique_stream_ids.end(); iter++) {
                        const RIFFChunkDefinitionSubDescriptor *chunk_descriptor = 0;
                        try {
                            chunk_descriptor = chunk_descriptors.at(*iter);
                        } catch (...) {
                            log_warn("Referenced chunk descriptor with stream id %u does not exist in package descriptor\n", *iter);
                            continue;
                        }

                        mWaveChunks.push_back(new MXFWaveChunk(file_reader->mFile, chunk_descriptor));
                    }
                }
            } else {
                ADM_CHNASubDescriptor *chna_descriptor =
                    dynamic_cast<ADM_CHNASubDescriptor*>(sub_descriptors[i]);
                if (chna_descriptor)
                    mWaveCHNA = convert_adm_chna_descriptor_to_chunk(chna_descriptor);
            }
        }
    }
}

MXFFileTrackReader::~MXFFileTrackReader()
{
    delete mTrackInfo;
    delete [] mAVCIHeader;

    size_t i;
    for (i = 0; i < mWaveChunks.size(); i++)
        delete mWaveChunks[i];
    delete mWaveCHNA;
}

void MXFFileTrackReader::SetEmptyFrames(bool enable)
{
    mFrameBuffer.SetEmptyFrames(enable);
}

void MXFFileTrackReader::SetEnable(bool enable)
{
    mIsEnabled = enable;
}

void MXFFileTrackReader::SetFrameBuffer(FrameBuffer *frame_buffer, bool take_ownership)
{
    mFrameBuffer.SetTargetBuffer(frame_buffer, take_ownership);
}

vector<size_t> MXFFileTrackReader::GetFileIds(bool internal_ess_only) const
{
    (void)internal_ess_only;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4267) // avoid bogus VS8 warning: 'argument' : conversion from 'size_t' to 'const uint32_t', possible loss of data
#endif

    return vector<size_t>(1, mFileReader->GetFileId());

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

void MXFFileTrackReader::GetReadLimits(bool limit_to_available, int64_t *start_position, int64_t *duration) const
{
    return mFileReader->GetReadLimits(limit_to_available, start_position, duration);
}

void MXFFileTrackReader::SetReadLimits()
{
    mFileReader->SetReadLimits();
}

void MXFFileTrackReader::SetReadLimits(int64_t start_position, int64_t duration, bool seek_to_start)
{
    mFileReader->SetReadLimits(start_position, duration, seek_to_start);
}

int64_t MXFFileTrackReader::GetReadStartPosition() const
{
    return mFileReader->GetReadStartPosition();
}

int64_t MXFFileTrackReader::GetReadDuration() const
{
    return mFileReader->GetReadDuration();
}

uint32_t MXFFileTrackReader::Read(uint32_t num_samples, bool is_top)
{
    return mFileReader->Read(num_samples, is_top);
}

bool MXFFileTrackReader::ReadError() const
{
    return mFileReader->ReadError();
}

string MXFFileTrackReader::ReadErrorMessage() const
{
    return mFileReader->ReadErrorMessage();
}

void MXFFileTrackReader::Seek(int64_t position)
{
    mFileReader->Seek(position);
}

int64_t MXFFileTrackReader::GetPosition() const
{
    return mFileReader->GetPosition();
}

mxfRational MXFFileTrackReader::GetEditRate() const
{
    return mFileReader->GetEditRate();
}

int64_t MXFFileTrackReader::GetDuration() const
{
    return mFileReader->GetDuration();
}

int64_t MXFFileTrackReader::GetOrigin() const
{
    return mFileReader->GetOrigin();
}

bool MXFFileTrackReader::GetIndexEntry(MXFIndexEntryExt *entry, int64_t position) const
{
    return mFileReader->GetInternalIndexEntry(entry, position);
}

int16_t MXFFileTrackReader::GetPrecharge(int64_t position, bool limit_to_available) const
{
    return mFileReader->GetInternalPrecharge(position, limit_to_available);
}

int64_t MXFFileTrackReader::GetAvailablePrecharge(int64_t position) const
{
    return mFileReader->GetInternalAvailablePrecharge(position);
}

int16_t MXFFileTrackReader::GetRollout(int64_t position, bool limit_to_available) const
{
    return mFileReader->GetInternalRollout(position, limit_to_available);
}

int64_t MXFFileTrackReader::GetAvailableRollout(int64_t position) const
{
    return mFileReader->GetInternalAvailableRollout(position);
}

void MXFFileTrackReader::SetNextFramePosition(Rational edit_rate, int64_t position)
{
    mFileReader->SetNextFramePosition(edit_rate, position);
}

void MXFFileTrackReader::SetAVCIHeader(const unsigned char *frame_data, uint32_t frame_data_size)
{
    BMX_CHECK(frame_data_size >= AVCI_HEADER_SIZE);

    delete [] mAVCIHeader;

    mAVCIHeader = new unsigned char[AVCI_HEADER_SIZE];
    memcpy(mAVCIHeader, frame_data, AVCI_HEADER_SIZE);
}

vector<WaveCHNA::AudioID> MXFFileTrackReader::GetCHNAAudioIDs(uint32_t channel_index) const
{
    if (!mWaveCHNA)
        return vector<WaveCHNA::AudioID>();

    // + 1 because chna track_index starts from 1
    return mWaveCHNA->GetAudioIDs(channel_index + 1);
}

MXFWaveChunk* MXFFileTrackReader::GetWaveChunk(size_t index) const
{
    BMX_CHECK(index < mWaveChunks.size());
    return mWaveChunks[index];
}

MXFWaveChunk* MXFFileTrackReader::GetWaveChunk(WaveChunkTag tag) const
{
    size_t i;
    for (i = 0; i < mWaveChunks.size(); i++) {
        if (tag == mWaveChunks[i]->Tag())
            return mWaveChunks[i];
    }

    return 0;
}
