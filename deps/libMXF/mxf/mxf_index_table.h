/*
 * MXF index table
 *
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

#ifndef MXF_INDEX_TABLE_H_
#define MXF_INDEX_TABLE_H_


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct MXFDeltaEntry
{
    struct MXFDeltaEntry *next;

    int8_t posTableIndex;
    uint8_t slice;
    uint32_t elementData;
} MXFDeltaEntry;

typedef struct MXFIndexEntry
{
    struct MXFIndexEntry *next;

    int8_t temporalOffset;
    int8_t keyFrameOffset;
    uint8_t flags;
    uint64_t streamOffset;
    uint32_t *sliceOffset;
    mxfRational *posTable;
} MXFIndexEntry;

typedef enum {
  MXF_OPT_BOOL_NOT_PRESENT = 0,
  MXF_OPT_BOOL_TRUE        = 1,
  MXF_OPT_BOOL_FALSE       = 2,
} mxfOptBool;

typedef struct
{
    mxfUUID instanceUID;
    mxfRational indexEditRate;
    mxfPosition indexStartPosition;
    mxfLength indexDuration;
    uint32_t editUnitByteCount;
    uint32_t indexSID;
    uint32_t bodySID;
    uint8_t sliceCount;
    int forceWriteSliceCount;
    int forceWriteCBEDuration0;
    uint8_t posTableCount;
    MXFDeltaEntry *deltaEntryArray;
    MXFIndexEntry *indexEntryArray;
    uint64_t extStartOffset;
    uint64_t vbeByteCount;
    mxfOptBool singleIndexLocation;
    mxfOptBool singleEssenceLocation;
    mxfOptBool forwardIndexDirection;
} MXFIndexTableSegment;


typedef int (mxf_add_delta_entry)(void *data, uint32_t numEntries, MXFIndexTableSegment *segment, int8_t posTableIndex,
                                  uint8_t slice, uint32_t elementData);
typedef int (mxf_add_index_entry)(void *data, uint32_t numEntries, MXFIndexTableSegment *segment, int8_t temporalOffset,
                                  int8_t keyFrameOffset, uint8_t flags, uint64_t streamOffset,
                                  uint32_t *sliceOffset /* assumes sliceCount set in segment */,
                                  mxfRational *posTable /* assumes posTableCount set in segment */);


int mxf_is_index_table_segment(const mxfKey *key);

int mxf_create_index_table_segment(MXFIndexTableSegment **segment);
void mxf_free_index_table_segment(MXFIndexTableSegment **segment);

int mxf_default_add_delta_entry(void *data/*ignored*/, uint32_t numEntries/*ignored*/, MXFIndexTableSegment *segment,
                                int8_t posTableIndex, uint8_t slice, uint32_t elementData);

int mxf_default_add_index_entry(void *data/*ignored*/, uint32_t numEntries/*ignored*/, MXFIndexTableSegment *segment,
                                int8_t temporalOffset, int8_t keyFrameOffset, uint8_t flags, uint64_t streamOffset,
                                uint32_t *sliceOffset, mxfRational *posTable);

int mxf_write_index_table_segment(MXFFile *mxfFile, const MXFIndexTableSegment *segment);

int mxf_read_index_table_segment(MXFFile *mxfFile, uint64_t segmentLen, MXFIndexTableSegment **segment);
int mxf_read_index_table_segment_2(MXFFile *mxfFile, uint64_t segmentLen,
                                   mxf_add_delta_entry *addDeltaEntry, void *addDeltaEntryData,
                                   mxf_add_index_entry *addIndexEntry, void *addIndexEntryData,
                                   MXFIndexTableSegment **segment);

int mxf_write_index_table_segment_header(MXFFile *mxfFile, const MXFIndexTableSegment *segment,
                                         uint32_t numDeltaEntries, uint32_t numIndexEntries);
int mxf_write_delta_entry_array_header(MXFFile *mxfFile, uint32_t numDeltaEntries);
int mxf_write_delta_entry(MXFFile *mxfFile, MXFDeltaEntry *entry);
int mxf_write_index_entry_array_header(MXFFile *mxfFile, uint8_t sliceCount, uint8_t posTableCount,
                                       uint32_t numIndexEntries);
int mxf_write_index_entry(MXFFile *mxfFile, uint8_t sliceCount, uint8_t posTableCount, MXFIndexEntry *entry);


#ifdef __cplusplus
}
#endif


#endif


