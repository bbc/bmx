/*
 * Copyright (C) 2022, British Broadcasting Corporation
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

#include <limits.h>

#include <bmx/mxf_reader/MXFWaveChunk.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


MXFWaveChunk::MXFWaveChunk(File *mxf_file, const RIFFChunkDefinitionSubDescriptor *riff_chunk_descriptor)
: WaveChunk(riff_chunk_descriptor->getRIFFChunkID())
{
    mGSReader = new GenericStreamReader(mxf_file, riff_chunk_descriptor->getRIFFChunkStreamID(), vector<mxfKey>());
}

MXFWaveChunk::~MXFWaveChunk()
{
    delete mGSReader;
}

uint32_t MXFWaveChunk::Read(unsigned char *data, uint32_t size)
{
    return mGSReader->Read(data, size);
}

int64_t MXFWaveChunk::Tell()
{
    return mGSReader->Tell();
}

int64_t MXFWaveChunk::Size()
{
    int64_t size = mGSReader->Size();

    if (size > (int64_t)UINT32_MAX)
        BMX_EXCEPTION(("Chunk data size in MXF exceeds size that can be represented in a Wave file"));

    return size;
}
