/*
 * Read a clip consisting of a set of MXF OP-Atom files
 *
 * Copyright (C) 2009, British Broadcasting Corporation
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

#include <libMXF++/MXF.h>

#include "OPAtomClipReader.h"

using namespace std;
using namespace mxfpp;



pair<OPAtomOpenResult, string> OPAtomClipReader::Open(const vector<std::string> &track_filenames,
                                                      OPAtomClipReader **clip_reader)
{
    MXFPP_ASSERT(!track_filenames.empty());

    vector<OPAtomTrackReader*> track_readers;
    string filename;
    try
    {
        OPAtomOpenResult result;
        OPAtomTrackReader *track_reader;
        size_t i;
        for (i = 0; i < track_filenames.size(); i++) {
            filename = track_filenames[i];

            result = OPAtomTrackReader::Open(filename, &track_reader);
            if (result != OP_ATOM_SUCCESS)
                throw result;
            track_readers.push_back(track_reader);
        }

        *clip_reader = new OPAtomClipReader(track_readers);

        return make_pair(OP_ATOM_SUCCESS, "");
    }
    catch (const OPAtomOpenResult &ex)
    {
        size_t i;
        for (i = 0; i < track_readers.size(); i++)
            delete track_readers[i];

        return make_pair(ex, filename);
    }
    catch (...)
    {
        size_t i;
        for (i = 0; i < track_readers.size(); i++)
            delete track_readers[i];

        return make_pair(OP_ATOM_FAIL, filename);
    }
}

string OPAtomClipReader::ErrorToString(OPAtomOpenResult result)
{
    return OPAtomTrackReader::ErrorToString(result);
}



OPAtomClipReader::OPAtomClipReader(const std::vector<OPAtomTrackReader*> &track_readers)
{
    MXFPP_ASSERT(!track_readers.empty());

    mTrackReaders = track_readers;
    mPosition = 0;
    mDuration = -1;

    GetDuration();
}

OPAtomClipReader::~OPAtomClipReader()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++)
        delete mTrackReaders[i];
}

int64_t OPAtomClipReader::GetDuration()
{
    if (mDuration >= 0)
        return mDuration;

    int64_t min_duration = -1;
    int64_t duration = -1;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        duration = mTrackReaders[i]->GetDuration();
        if (duration < 0) {
            min_duration = -1;
            break;
        }

        if (min_duration < 0 || duration < min_duration)
            min_duration = duration;
    }

    mDuration = min_duration;

    return mDuration;
}

int64_t OPAtomClipReader::DetermineDuration()
{
    int64_t min_duration = -1;
    int64_t duration = -1;
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        duration = mTrackReaders[i]->DetermineDuration();
        if (duration < 0) {
            min_duration = -1;
            break;
        }

        if (min_duration < 0 || duration < min_duration)
            min_duration = duration;
    }

    mDuration = min_duration;

    return mDuration;
}

int64_t OPAtomClipReader::GetPosition()
{
    return mPosition;
}

const OPAtomContentPackage *OPAtomClipReader::Read()
{
    int64_t prev_position = GetPosition();

    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (!mTrackReaders[i]->Read(&mContentPackage)) {
            size_t j;
            for (j = 0; j <= i; j++)
                mTrackReaders[j]->Seek(prev_position);
            return 0;
        }
    }

    mPosition++;

    return &mContentPackage;
}

bool OPAtomClipReader::Seek(int64_t position)
{
    int64_t prev_position = GetPosition();

    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (!mTrackReaders[i]->Seek(position)) {
            size_t j;
            for (j = 0; j <= i; j++)
                mTrackReaders[j]->Seek(prev_position);
            return false;
        }
    }

    mPosition = position;

    return true;
}

bool OPAtomClipReader::IsEOF()
{
    size_t i;
    for (i = 0; i < mTrackReaders.size(); i++) {
        if (mTrackReaders[i]->IsEOF())
            return true;
    }

    return false;
}

