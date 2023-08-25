/*
 * Copyright (C) 2006, British Broadcasting Corporation
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

#ifndef TIMECODE_INDEX_H_
#define TIMECODE_INDEX_H_

#ifdef __cplusplus
extern "C"
{
#endif


#include <mxf/mxf_list.h>
#include <mxf/mxf_types.h>
#include <mxf/mxf_app_types.h>


typedef struct
{
    uint8_t frozen;
    int64_t timecodePos;
    int64_t duration;
} TimecodeIndexElement;

typedef struct
{
    TimecodeIndexElement *elements;
    int numElements;
} TimecodeIndexArray;

typedef struct
{
    int arraySize;
    MXFList indexArrays;
    uint16_t roundedTCBase;
    int allowDropFrame;
} TimecodeIndex;


typedef struct
{
    MXFListIterator indexArrayIter;
    TimecodeIndex *index;
    int elementNum;
    int64_t elementOffset;
    int64_t position;
    int atEnd;
    int beforeStart;
} TimecodeIndexSearcher;


void initialise_timecode_index(TimecodeIndex *index, const mxfRational *frameRate, int arraySize);
void clear_timecode_index(TimecodeIndex *index);

int add_timecode_to_index(TimecodeIndex *index, ArchiveTimecode *timecode);

int is_null_timecode_index(TimecodeIndex *index);


void initialise_timecode_index_searcher(TimecodeIndex *index, TimecodeIndexSearcher *searcher);

int find_timecode(TimecodeIndexSearcher *searcher, int64_t position, ArchiveTimecode *timecode);
int find_position(TimecodeIndexSearcher *searcher, const ArchiveTimecode *timecode, int64_t *position);

int find_position_at_dual_timecode(TimecodeIndexSearcher *vitcSearcher, const ArchiveTimecode *vitcTimecode,
                                   TimecodeIndexSearcher *ltcSearcher, const ArchiveTimecode *ltcTimecode,
                                   int64_t *position);


#ifdef __cplusplus
}
#endif


#endif

