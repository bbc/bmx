/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#include <bmx/mxf_reader/MXFFileIndex.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



MXFFileIndex::MXFFileIndex()
{
}

MXFFileIndex::~MXFFileIndex()
{
}

size_t MXFFileIndex::RegisterFile(const IndexEntry &entry)
{
    BMX_CHECK(!entry.abs_uri.IsEmpty());

    size_t i;
    for (i = 0; i < mEntries.size(); i++) {
        if (entry.abs_uri == mEntries[i].abs_uri)
            return i;
    }

    mEntries.push_back(entry);

    return mEntries.size() - 1;
}

size_t MXFFileIndex::RegisterFile(const URI &abs_uri, const URI &rel_uri, const string &filename)
{
    IndexEntry entry;
    entry.abs_uri  = abs_uri;
    entry.rel_uri  = rel_uri;
    entry.filename = filename;

    return RegisterFile(entry);
}

void MXFFileIndex::RegisterFiles(const MXFFileIndex *replaced_index)
{
    size_t i;
    for (i = 0; i < replaced_index->mEntries.size(); i++)
        RegisterFile(replaced_index->GetEntry(i));
}

const MXFFileIndex::IndexEntry& MXFFileIndex::GetEntry(size_t id) const
{
    BMX_CHECK(id < mEntries.size());
    return mEntries[id];
}

URI MXFFileIndex::GetAbsoluteURI(size_t id) const
{
    if (id < mEntries.size())
        return mEntries[id].abs_uri;
    else
        return URI();
}

URI MXFFileIndex::GetRelativeURI(size_t id) const
{
    if (id < mEntries.size())
        return mEntries[id].rel_uri;
    else
        return URI();
}

string MXFFileIndex::GetFilename(size_t id) const
{
    if (id < mEntries.size())
        return mEntries[id].filename;
    else
        return "";
}

