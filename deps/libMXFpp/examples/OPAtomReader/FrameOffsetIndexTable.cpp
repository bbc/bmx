/*
 * Basic index table with frame offsets
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

#include <memory>

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

#include "FrameOffsetIndexTable.h"


using namespace std;
using namespace mxfpp;


int add_frame_offset_index_entry(void *data, uint32_t num_entries, MXFIndexTableSegment *segment,
                                 int8_t temporal_offset, int8_t key_frame_offset, uint8_t flags,
                                 uint64_t stream_offset, uint32_t *slice_offset, mxfRational *pos_table)
{
    FrameOffsetIndexTableSegment *index_table = static_cast<FrameOffsetIndexTableSegment*>(data);

    (void)num_entries;
    (void)segment;
    (void)temporal_offset;
    (void)key_frame_offset;
    (void)flags;
    (void)slice_offset;
    (void)pos_table;

    index_table->appendFrameOffset(stream_offset);

    return 1;
}



FrameOffsetIndexTableSegment* FrameOffsetIndexTableSegment::read(File *file, uint64_t segment_len)
{
    unique_ptr<FrameOffsetIndexTableSegment> index_table(new FrameOffsetIndexTableSegment());
    mxf_free_index_table_segment(&index_table->_cSegment);

    MXFPP_CHECK(mxf_avid_read_index_table_segment_2(file->getCFile(), segment_len, mxf_default_add_delta_entry, 0,
                                                    add_frame_offset_index_entry, index_table.get(),
                                                    &index_table->_cSegment));

    return index_table.release();
}



FrameOffsetIndexTableSegment::FrameOffsetIndexTableSegment()
: IndexTableSegment()
{
    setIndexDuration(-1);
}

FrameOffsetIndexTableSegment::~FrameOffsetIndexTableSegment()
{
}

bool FrameOffsetIndexTableSegment::haveFrameOffset(int64_t position)
{
    return position < (int64_t)mFrameOffsets.size();
}

int64_t FrameOffsetIndexTableSegment::getFrameOffset(int64_t position)
{
    MXFPP_CHECK(position < (int64_t)mFrameOffsets.size());

    return mFrameOffsets[(size_t)position];
}

bool FrameOffsetIndexTableSegment::getLastIndexOffset(int64_t *offset, int64_t *position)
{
    if (mFrameOffsets.empty())
        return false;

    *position = mFrameOffsets.size() - 1;
    *offset = mFrameOffsets[(size_t)(*position)];

    return true;
}

int64_t FrameOffsetIndexTableSegment::getDuration()
{
    if (getIndexDuration() >= 0)
        return getIndexDuration();

    // -1 because the last entry is the last frame's end offset
    return mFrameOffsets.size() - 1;
}

void FrameOffsetIndexTableSegment::appendFrameOffset(int64_t offset)
{
    if (mFrameOffsets.size() == mFrameOffsets.capacity())
        mFrameOffsets.reserve(mFrameOffsets.capacity() + 8192);

    return mFrameOffsets.push_back(offset);
}

