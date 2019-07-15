/*
 * Copyright (C) 2018, British Broadcasting Corporation
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

#include "OutputFileManager.h"

#include <errno.h>
#include <string.h>

#include <bmx/BMXException.h>
#include <bmx/Utils.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


OutputFileManager::OutputFileManager()
{
    mSoundDeinterleave = false;
}

OutputFileManager::~OutputFileManager()
{
    map<size_t, TrackFileInfo>::const_iterator iter1;
    for (iter1 = mTrackFiles.begin(); iter1 != mTrackFiles.end(); iter1++) {
        map<uint32_t, FileInfo>::const_iterator iter2;
        for (iter2 = iter1->second.children.begin(); iter2 != iter1->second.children.end(); iter2++) {
            if (iter2->second.file) {
                fclose(iter2->second.file);
            }
        }
    }
}

void OutputFileManager::SetPrefix(const string &prefix)
{
    mPrefix = prefix;
}

void OutputFileManager::SetSoundDeinterleave(bool enable)
{
    mSoundDeinterleave = enable;
}

void OutputFileManager::AddTrackFile(size_t track_index, const MXFTrackInfo *track_info, bool wrap_klv)
{
    const char *ddef_letter = "x";
    switch (track_info->data_def)
    {
        case MXF_PICTURE_DDEF:  ddef_letter = "v"; break;
        case MXF_SOUND_DDEF:    ddef_letter = "a"; break;
        case MXF_DATA_DDEF:     ddef_letter = "d"; break;
        case MXF_TIMECODE_DDEF: ddef_letter = "t"; break;
        case MXF_DM_DDEF:       ddef_letter = "m"; break;
        case MXF_UNKNOWN_DDEF:  ddef_letter = "x"; break;
    }

    char buffer[64];
    uint32_t ddef_count = mDDefCount[track_info->data_def];
    const MXFSoundTrackInfo *sound_info = dynamic_cast<const MXFSoundTrackInfo*>(track_info);
    if (sound_info && mSoundDeinterleave && sound_info->channel_count > 1) {
        const char *suffix = ".raw";
        if (wrap_klv)
            suffix = ".klv";

        uint32_t c;
        for (c = 0; c < sound_info->channel_count; c++) {
            bmx_snprintf(buffer, sizeof(buffer), "_%s%u_%d%s", ddef_letter, ddef_count, c, suffix);

            FileInfo file_info;
            file_info.filename = mPrefix + buffer;
            file_info.file = fopen(file_info.filename.c_str(), "wb");
            if (!file_info.file) {
                log_error("Failed to open raw file '%s': %s\n",
                          file_info.filename.c_str(), bmx_strerror(errno).c_str());
                throw false;
            }
            mTrackFiles[track_index].children[c] = file_info;
        }
    } else if (track_info->essence_type == TIMED_TEXT) {
        TimedTextManifest *manifest = dynamic_cast<const MXFDataTrackInfo*>(track_info)->timed_text_manifest;

        bmx_snprintf(buffer, sizeof(buffer), "_%s%u.xml", ddef_letter, ddef_count);

        FileInfo file_info;
        file_info.filename = mPrefix + buffer;
        file_info.file = fopen(file_info.filename.c_str(), "wb");
        if (!file_info.file) {
            log_error("Failed to open raw file '%s': %s\n",
                      file_info.filename.c_str(), bmx_strerror(errno).c_str());
            throw false;
        }
        mTrackFiles[track_index].children[(uint32_t)(-1)] = file_info;

        std::vector<TimedTextAncillaryResource> &anc_resources = manifest->GetAncillaryResources();
        size_t i;
        for (i = 0; i < anc_resources.size(); i++) {
            bmx_snprintf(buffer, sizeof(buffer), "_%s%u_%d.raw", ddef_letter, ddef_count,
                         anc_resources[i].stream_id);

            FileInfo file_info;
            file_info.filename = mPrefix + buffer;
            file_info.file = fopen(file_info.filename.c_str(), "wb");
            if (!file_info.file) {
                log_error("Failed to open raw file '%s': %s\n",
                          file_info.filename.c_str(), bmx_strerror(errno).c_str());
                throw false;
            }
            mTrackFiles[track_index].children[anc_resources[i].stream_id] = file_info;
        }
    } else {
          const char *suffix = ".raw";
          if (wrap_klv)
              suffix = ".klv";

          bmx_snprintf(buffer, sizeof(buffer), "_%s%u%s", ddef_letter, ddef_count, suffix);

          FileInfo file_info;
          file_info.filename = mPrefix + buffer;
          file_info.file = fopen(file_info.filename.c_str(), "wb");
          if (!file_info.file) {
              log_error("Failed to open raw file '%s': %s\n",
                        file_info.filename.c_str(), bmx_strerror(errno).c_str());
              throw false;
          }
          mTrackFiles[track_index].children[(uint32_t)(-1)] = file_info;
    }

    mDDefCount[track_info->data_def]++;
}

void OutputFileManager::GetTrackFile(size_t track_index, uint32_t child_index,
                                     FILE **file, string *filename)
{
    FileInfo &file_info = mTrackFiles.at(track_index).children.at(child_index);
    *file = file_info.file;
    *filename = file_info.filename;
}

void OutputFileManager::GetTrackFile(size_t track_index,
                                     FILE **file, string *filename)
{
    return GetTrackFile(track_index, (uint32_t)(-1), file, filename);
}
