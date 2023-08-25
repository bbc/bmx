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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>



static void add_delta_entry(MXFIndexTableSegment *segment, MXFDeltaEntry *entry)
{
    if (segment->deltaEntryArray == NULL)
    {
        segment->deltaEntryArray = entry;
    }
    else
    {
        uint32_t deltaEntryArrayLen = 0;
        MXFDeltaEntry *lastEntry = segment->deltaEntryArray;
        while (lastEntry->next != NULL)
        {
            deltaEntryArrayLen++;
            lastEntry = lastEntry->next;
        }
        assert(8 + deltaEntryArrayLen * 6 <= UINT16_MAX);
        lastEntry->next = entry;
    }
}

static void add_index_entry(MXFIndexTableSegment *segment, MXFIndexEntry *entry)
{
    if (segment->indexEntryArray == NULL)
    {
        segment->indexEntryArray = entry;
    }
    else
    {
        uint32_t indexEntryArrayLen = 0;
        MXFIndexEntry *lastEntry = segment->indexEntryArray;
        while (lastEntry->next != NULL)
        {
            indexEntryArrayLen++;
            lastEntry = lastEntry->next;
        }
        assert(8 + indexEntryArrayLen * (11 + segment->sliceCount * 4 + segment->posTableCount * 8) <= UINT16_MAX);
        lastEntry->next = entry;
    }
}

static void free_index_entry(MXFIndexEntry **entry)
{
    if (*entry == NULL)
    {
        return;
    }

    SAFE_FREE((*entry)->sliceOffset);
    SAFE_FREE((*entry)->posTable);
    // free of list is done in mxf_free_index_table_segment
    (*entry)->next = NULL;
    SAFE_FREE(*entry);
}

static int create_index_entry(MXFIndexTableSegment *segment, MXFIndexEntry **entry)
{
    MXFIndexEntry *newEntry = NULL;

    CHK_MALLOC_ORET(newEntry, MXFIndexEntry);
    memset(newEntry, 0, sizeof(MXFIndexEntry));

    if (segment->sliceCount > 0)
    {
        CHK_MALLOC_ARRAY_OFAIL(newEntry->sliceOffset, uint32_t, segment->sliceCount);
        memset(newEntry->sliceOffset, 0, sizeof(uint32_t) * segment->sliceCount);
    }
    if (segment->posTableCount > 0)
    {
        CHK_MALLOC_ARRAY_OFAIL(newEntry->posTable, mxfRational, segment->posTableCount);
        memset(newEntry->posTable, 0, sizeof(mxfRational) * segment->posTableCount);
    }

    add_index_entry(segment, newEntry);
    *entry = newEntry;
    return 1;

fail:
    free_index_entry(&newEntry);
    return 0;
}

static void free_delta_entry(MXFDeltaEntry **entry)
{
    if (*entry == NULL)
    {
        return;
    }

    // free of list is done in mxf_free_index_table_segment
    (*entry)->next = NULL;
    SAFE_FREE(*entry);
}

static int create_delta_entry(MXFIndexTableSegment *segment, MXFDeltaEntry **entry)
{
    MXFDeltaEntry *newEntry;

    CHK_MALLOC_ORET(newEntry, MXFDeltaEntry);
    memset(newEntry, 0, sizeof(MXFDeltaEntry));

    add_delta_entry(segment, newEntry);
    *entry = newEntry;
    return 1;
}



int mxf_is_index_table_segment(const mxfKey *key)
{
    return mxf_equals_key(key, &g_IndexTableSegment_key);
}

int mxf_create_index_table_segment(MXFIndexTableSegment **segment)
{
    MXFIndexTableSegment *newSegment;

    CHK_MALLOC_ORET(newSegment, MXFIndexTableSegment);
    memset(newSegment, 0, sizeof(MXFIndexTableSegment));

    *segment = newSegment;
    return 1;
}

void mxf_free_index_table_segment(MXFIndexTableSegment **segment)
{
    MXFIndexEntry *indexEntry;
    MXFIndexEntry *tmpNextIndexEntry;
    MXFDeltaEntry *deltaEntry;
    MXFDeltaEntry *tmpNextDeltaEntry;

    if (*segment == NULL)
    {
        return;
    }

    indexEntry = (*segment)->indexEntryArray;
    deltaEntry = (*segment)->deltaEntryArray;

    // free list using loop instead of recursive call in free_index_entry
    while (indexEntry != NULL)
    {
        tmpNextIndexEntry = indexEntry->next;
        free_index_entry(&indexEntry);
        indexEntry = tmpNextIndexEntry;
    }

    // free list using loop instead of recursive call in free_delta_entry
    while (deltaEntry != NULL)
    {
        tmpNextDeltaEntry = deltaEntry->next;
        free_delta_entry(&deltaEntry);
        deltaEntry = tmpNextDeltaEntry;
    }

    SAFE_FREE(*segment);
}

int mxf_default_add_delta_entry(void *data, uint32_t numEntries, MXFIndexTableSegment *segment, int8_t posTableIndex,
                                uint8_t slice, uint32_t elementData)
{
    MXFDeltaEntry *newEntry;

    (void)data;
    (void)numEntries;

    CHK_ORET(create_delta_entry(segment, &newEntry));
    newEntry->posTableIndex = posTableIndex;
    newEntry->slice = slice;
    newEntry->elementData = elementData;

    return 1;
}

int mxf_default_add_index_entry(void *data, uint32_t numEntries, MXFIndexTableSegment *segment, int8_t temporalOffset,
                                int8_t keyFrameOffset, uint8_t flags, uint64_t streamOffset,
                                uint32_t *sliceOffset, mxfRational *posTable)
{
    MXFIndexEntry *newEntry;

    (void)data;
    (void)numEntries;

    CHK_ORET(create_index_entry(segment, &newEntry));
    newEntry->temporalOffset = temporalOffset;
    newEntry->keyFrameOffset = keyFrameOffset;
    newEntry->flags = flags;
    newEntry->streamOffset = streamOffset;
    if (segment->sliceCount > 0)
    {
        memcpy(newEntry->sliceOffset, sliceOffset, sizeof(uint32_t) * segment->sliceCount);
    }
    if (segment->posTableCount > 0)
    {
        memcpy(newEntry->posTable, posTable, sizeof(mxfRational) * segment->posTableCount);
    }

    return 1;
}

int mxf_write_index_table_segment(MXFFile *mxfFile, const MXFIndexTableSegment *segment)
{
    uint64_t segmentLen = 80;
    uint32_t deltaEntryArrayLen = 0;
    uint32_t indexEntryArrayLen = 0;
    if (segment->deltaEntryArray != NULL)
    {
        MXFDeltaEntry *entry = segment->deltaEntryArray;
        segmentLen += 12;
        while (entry != NULL)
        {
            segmentLen += 6;
            deltaEntryArrayLen++;
            entry = entry->next;
        }
    }
    if (segment->indexEntryArray != NULL)
    {
        MXFIndexEntry *entry = segment->indexEntryArray;
        segmentLen += 22; /* includes PosTableCount and SliceCount */
        while (entry != NULL)
        {
            segmentLen += 11 + segment->sliceCount * 4 + segment->posTableCount * 8;
            indexEntryArrayLen++;
            entry = entry->next;
        }
    }
    else if (segment->forceWriteSliceCount)
    {
        segmentLen += 5;
    }
    if (segment->extStartOffset)
    {
        segmentLen += 12;
    }
    if (segment->vbeByteCount)
    {
        segmentLen += 12;
    }
    if (segment->singleIndexLocation)
    {
        segmentLen += 5;
    }
    if (segment->singleEssenceLocation)
    {
        segmentLen += 5;
    }
    if (segment->forwardIndexDirection)
    {
        segmentLen += 5;
    }

    CHK_ORET(mxf_write_kl(mxfFile, &g_IndexTableSegment_key, segmentLen));

    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3c0a, mxfUUID_extlen));
    CHK_ORET(mxf_write_uuid(mxfFile, &segment->instanceUID));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0b, 8));
    CHK_ORET(mxf_write_int32(mxfFile, segment->indexEditRate.numerator));
    CHK_ORET(mxf_write_int32(mxfFile, segment->indexEditRate.denominator));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0c, 8));
    CHK_ORET(mxf_write_int64(mxfFile, segment->indexStartPosition));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0d, 8));
    CHK_ORET(mxf_write_int64(mxfFile, segment->indexEntryArray != NULL || !segment->forceWriteCBEDuration0 ? segment->indexDuration : 0));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f05, 4));
    CHK_ORET(mxf_write_uint32(mxfFile, segment->editUnitByteCount));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f06, 4));
    CHK_ORET(mxf_write_uint32(mxfFile, segment->indexSID));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f07, 4));
    CHK_ORET(mxf_write_uint32(mxfFile, segment->bodySID));
    if (segment->indexEntryArray != NULL)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f08, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, segment->sliceCount));
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0e, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, segment->posTableCount));
    }
    else if (segment->forceWriteSliceCount)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f08, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, segment->sliceCount));
    }
    if (segment->extStartOffset)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0f, 8));
        CHK_ORET(mxf_write_uint64(mxfFile, segment->extStartOffset));
    }
    if (segment->vbeByteCount)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f10, 8));
        CHK_ORET(mxf_write_uint64(mxfFile, segment->vbeByteCount));
    }
    if (segment->singleIndexLocation)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f11, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, (segment->singleIndexLocation == MXF_OPT_BOOL_TRUE)));
    }
    if (segment->singleEssenceLocation)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f12, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, (segment->singleEssenceLocation == MXF_OPT_BOOL_TRUE)));
    }
    if (segment->forwardIndexDirection)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f13, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, (segment->forwardIndexDirection == MXF_OPT_BOOL_TRUE)));
    }

    if (segment->deltaEntryArray != NULL)
    {
        MXFDeltaEntry *entry = segment->deltaEntryArray;
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f09, (uint16_t)(8 + deltaEntryArrayLen * 6)));
        CHK_ORET(mxf_write_uint32(mxfFile, deltaEntryArrayLen));
        CHK_ORET(mxf_write_uint32(mxfFile, 6));
        while (entry != NULL)
        {
            CHK_ORET(mxf_write_int8(mxfFile, entry->posTableIndex));
            CHK_ORET(mxf_write_uint8(mxfFile, entry->slice));
            CHK_ORET(mxf_write_uint32(mxfFile, entry->elementData));
            entry = entry->next;
        }
    }
    if (segment->indexEntryArray != NULL)
    {
        MXFIndexEntry *entry = segment->indexEntryArray;
        uint32_t i;
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0a, (uint16_t)(8 + indexEntryArrayLen *
                                                        (11 + segment->sliceCount * 4 + segment->posTableCount * 8))));
        CHK_ORET(mxf_write_uint32(mxfFile, indexEntryArrayLen));
        CHK_ORET(mxf_write_uint32(mxfFile, 11 + segment->sliceCount * 4 + segment->posTableCount * 8));
        while (entry != NULL)
        {
            CHK_ORET(mxf_write_uint8(mxfFile, entry->temporalOffset));
            CHK_ORET(mxf_write_uint8(mxfFile, entry->keyFrameOffset));
            CHK_ORET(mxf_write_uint8(mxfFile, entry->flags));
            CHK_ORET(mxf_write_uint64(mxfFile, entry->streamOffset));
            for (i = 0; i < segment->sliceCount; i++)
            {
                CHK_ORET(mxf_write_uint32(mxfFile, entry->sliceOffset[i]));
            }
            for (i = 0; i < segment->posTableCount; i++)
            {
                CHK_ORET(mxf_write_int32(mxfFile, entry->posTable[i].numerator));
                CHK_ORET(mxf_write_int32(mxfFile, entry->posTable[i].denominator));
            }
            entry = entry->next;
        }
    }

    return 1;
}

int mxf_read_index_table_segment_2(MXFFile *mxfFile, uint64_t segmentLen,
                                   mxf_add_delta_entry *addDeltaEntry, void *addDeltaEntryData,
                                   mxf_add_index_entry *addIndexEntry, void *addIndexEntryData,
                                   MXFIndexTableSegment **segment)
{
    MXFIndexTableSegment *newSegment = NULL;
    mxfLocalTag localTag;
    uint16_t localLen;
    uint64_t totalLen;
    uint32_t deltaEntryArrayLen;
    uint32_t deltaEntryLen;
    int8_t posTableIndex;
    uint8_t slice;
    uint32_t elementData;
    uint32_t *sliceOffset = NULL;
    mxfRational *posTable = NULL;
    uint32_t indexEntryArrayLen;
    uint32_t indexEntryLen;
    uint8_t temporalOffset;
    uint8_t keyFrameOffset;
    uint8_t flags;
    uint64_t streamOffset;
    uint32_t entry;
    uint32_t actualEntryLen;
    uint8_t tempOptBool;
    uint8_t i;

    CHK_ORET(mxf_create_index_table_segment(&newSegment));

    totalLen = 0;
    while (totalLen < segmentLen)
    {
        CHK_OFAIL(mxf_read_local_tl(mxfFile, &localTag, &localLen));
        totalLen += mxfLocalTag_extlen + 2;

        switch (localTag)
        {
            case 0x3c0a:
                CHK_OFAIL(localLen == mxfUUID_extlen);
                CHK_OFAIL(mxf_read_uuid(mxfFile, &newSegment->instanceUID));
                break;
            case 0x3f0b:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_int32(mxfFile, &newSegment->indexEditRate.numerator));
                CHK_OFAIL(mxf_read_int32(mxfFile, &newSegment->indexEditRate.denominator));
                break;
            case 0x3f0c:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_int64(mxfFile, &newSegment->indexStartPosition));
                break;
            case 0x3f0d:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_int64(mxfFile, &newSegment->indexDuration));
                break;
            case 0x3f05:
                CHK_OFAIL(localLen == 4);
                CHK_OFAIL(mxf_read_uint32(mxfFile, &newSegment->editUnitByteCount));
                break;
            case 0x3f06:
                CHK_OFAIL(localLen == 4);
                CHK_OFAIL(mxf_read_uint32(mxfFile, &newSegment->indexSID));
                break;
            case 0x3f07:
                CHK_OFAIL(localLen == 4);
                CHK_OFAIL(mxf_read_uint32(mxfFile, &newSegment->bodySID));
                break;
            case 0x3f08:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &newSegment->sliceCount));
                break;
            case 0x3f0e:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &newSegment->posTableCount));
                break;
            case 0x3f0f:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_uint64(mxfFile, &newSegment->extStartOffset));
                break;
            case 0x3f10:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_uint64(mxfFile, &newSegment->vbeByteCount));
                break;
            case 0x3f11:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &tempOptBool));
                newSegment->singleIndexLocation = (tempOptBool ? MXF_OPT_BOOL_TRUE : MXF_OPT_BOOL_FALSE);
                break;
            case 0x3f12:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &tempOptBool));
                newSegment->singleEssenceLocation = (tempOptBool ? MXF_OPT_BOOL_TRUE : MXF_OPT_BOOL_FALSE);
                break;
            case 0x3f13:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &tempOptBool));
                newSegment->forwardIndexDirection = (tempOptBool ? MXF_OPT_BOOL_TRUE : MXF_OPT_BOOL_FALSE);
                break;
            case 0x3f09:
                CHK_ORET(mxf_read_uint32(mxfFile, &deltaEntryArrayLen));
                CHK_ORET(mxf_read_uint32(mxfFile, &deltaEntryLen));
                if (deltaEntryArrayLen != 0)
                {
                    CHK_ORET(deltaEntryLen == 6);
                }
                CHK_ORET(localLen == 8 + deltaEntryArrayLen * 6);
                entry = deltaEntryArrayLen;
                for (; entry > 0; entry--)
                {
                    CHK_ORET(mxf_read_int8(mxfFile, &posTableIndex));
                    CHK_ORET(mxf_read_uint8(mxfFile, &slice));
                    CHK_ORET(mxf_read_uint32(mxfFile, &elementData));
                    if (addDeltaEntry != NULL)
                    {
                        CHK_OFAIL((*addDeltaEntry)(addDeltaEntryData, deltaEntryArrayLen, newSegment, posTableIndex,
                                                   slice, elementData));
                    }
                }
                break;
            case 0x3f0a:
                if (newSegment->sliceCount > 0)
                {
                    CHK_MALLOC_ARRAY_ORET(sliceOffset, uint32_t, newSegment->sliceCount);
                }
                if (newSegment->posTableCount > 0)
                {
                    CHK_MALLOC_ARRAY_ORET(posTable, mxfRational, newSegment->posTableCount);
                }
                CHK_OFAIL(mxf_read_uint32(mxfFile, &indexEntryArrayLen));
                CHK_OFAIL(mxf_read_uint32(mxfFile, &indexEntryLen));
                if (indexEntryArrayLen != 0 &&
                    indexEntryLen != (uint32_t)11 + newSegment->sliceCount * 4 + newSegment->posTableCount * 8)
                {
                    /* GrassValley D10 sample files where found to have SliceCount 1 but index entry length 11 */
                    mxf_log_warn("Index entry length %u is incorrect; it should be %u\n", indexEntryLen,
                                 (uint32_t)11 + newSegment->sliceCount * 4 + newSegment->posTableCount * 8);
                }
                CHK_OFAIL(localLen == 8 + indexEntryArrayLen * indexEntryLen);
                entry = indexEntryArrayLen;
                for (; entry > 0; entry--)
                {
                    actualEntryLen = 0;
                    CHK_OFAIL(mxf_read_uint8(mxfFile, &temporalOffset));
                    CHK_OFAIL(mxf_read_uint8(mxfFile, &keyFrameOffset));
                    CHK_OFAIL(mxf_read_uint8(mxfFile, &flags));
                    CHK_OFAIL(mxf_read_uint64(mxfFile, &streamOffset));
                    actualEntryLen += 11;
                    for (i = 0; i < newSegment->sliceCount && actualEntryLen < indexEntryLen; i++)
                    {
                        CHK_OFAIL(mxf_read_uint32(mxfFile, &sliceOffset[i]));
                        actualEntryLen += 4;
                    }
                    for (i = 0; i < newSegment->posTableCount && actualEntryLen < indexEntryLen; i++)
                    {
                        CHK_OFAIL(mxf_read_int32(mxfFile, &posTable[i].numerator));
                        CHK_OFAIL(mxf_read_int32(mxfFile, &posTable[i].denominator));
                        actualEntryLen += 8;
                    }
                    if (addIndexEntry != NULL)
                    {
                        CHK_OFAIL((*addIndexEntry)(addIndexEntryData, indexEntryArrayLen, newSegment, temporalOffset,
                                                   keyFrameOffset, flags, streamOffset, sliceOffset, posTable));
                    }

                    if (actualEntryLen < indexEntryLen)
                    {
                        CHK_OFAIL(mxf_skip(mxfFile, indexEntryLen - actualEntryLen));
                    }
                }
                SAFE_FREE(sliceOffset);
                SAFE_FREE(posTable);
                break;
            default:
                mxf_log_warn("Unknown local item (%u) in index table segment\n", localTag);
                CHK_OFAIL(mxf_skip(mxfFile, localLen));
        }

        totalLen += localLen;
    }
    CHK_ORET(totalLen == segmentLen);

    *segment = newSegment;
    return 1;

fail:
    SAFE_FREE(sliceOffset);
    SAFE_FREE(posTable);
    mxf_free_index_table_segment(&newSegment);
    return 0;
}

int mxf_read_index_table_segment(MXFFile *mxfFile, uint64_t segmentLen, MXFIndexTableSegment **segment)
{
    CHK_ORET(mxf_read_index_table_segment_2(mxfFile, segmentLen, mxf_default_add_delta_entry, NULL,
                                            mxf_default_add_index_entry, NULL, segment));

    return 1;
}

int mxf_write_index_table_segment_header(MXFFile *mxfFile, const MXFIndexTableSegment *segment,
                                         uint32_t numDeltaEntries, uint32_t numIndexEntries)
{
    uint64_t segmentLen = 80;
    if (numDeltaEntries > 0)
    {
        segmentLen += 12 + numDeltaEntries * 6;
    }
    if (numIndexEntries > 0)
    {
        segmentLen += 22 /* includes PosTableCount and SliceCount */ +
            numIndexEntries * (11 + segment->sliceCount * 4 + segment->posTableCount * 8);
    }
    else if (segment->forceWriteSliceCount)
    {
      segmentLen += 5;
    }
    if (segment->extStartOffset)
    {
        segmentLen += 12;
    }
    if (segment->vbeByteCount)
    {
        segmentLen += 12;
    }
    if (segment->singleIndexLocation)
    {
        segmentLen += 5;
    }
    if (segment->singleEssenceLocation)
    {
        segmentLen += 5;
    }
    if (segment->forwardIndexDirection)
    {
        segmentLen += 5;
    }

    CHK_ORET(mxf_write_kl(mxfFile, &g_IndexTableSegment_key, segmentLen));

    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3c0a, mxfUUID_extlen));
    CHK_ORET(mxf_write_uuid(mxfFile, &segment->instanceUID));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0b, 8));
    CHK_ORET(mxf_write_int32(mxfFile, segment->indexEditRate.numerator));
    CHK_ORET(mxf_write_int32(mxfFile, segment->indexEditRate.denominator));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0c, 8));
    CHK_ORET(mxf_write_int64(mxfFile, segment->indexStartPosition));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0d, 8));
    CHK_ORET(mxf_write_int64(mxfFile, segment->indexDuration));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f05, 4));
    CHK_ORET(mxf_write_uint32(mxfFile, segment->editUnitByteCount));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f06, 4));
    CHK_ORET(mxf_write_uint32(mxfFile, segment->indexSID));
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f07, 4));
    CHK_ORET(mxf_write_uint32(mxfFile, segment->bodySID));
    if (numIndexEntries > 0)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f08, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, segment->sliceCount));
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0e, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, segment->posTableCount));
    }
    else if (segment->forceWriteSliceCount)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f08, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, segment->sliceCount));
    }
    if (segment->extStartOffset)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0f, 8));
        CHK_ORET(mxf_write_uint64(mxfFile, segment->extStartOffset));
    }
    if (segment->vbeByteCount)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f10, 8));
        CHK_ORET(mxf_write_uint64(mxfFile, segment->vbeByteCount));
    }
    if (segment->singleIndexLocation)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f11, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, (segment->singleIndexLocation == MXF_OPT_BOOL_TRUE)));
    }
    if (segment->singleEssenceLocation)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f12, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, (segment->singleEssenceLocation == MXF_OPT_BOOL_TRUE)));
    }
    if (segment->forwardIndexDirection)
    {
        CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f13, 1));
        CHK_ORET(mxf_write_uint8(mxfFile, (segment->forwardIndexDirection == MXF_OPT_BOOL_TRUE)));
    }


    return 1;
}

int mxf_write_delta_entry_array_header(MXFFile *mxfFile, uint32_t numDeltaEntries)
{
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f09, (uint16_t)(8 + numDeltaEntries * 6)));
    CHK_ORET(mxf_write_uint32(mxfFile, numDeltaEntries));
    CHK_ORET(mxf_write_uint32(mxfFile, 6));

    return 1;
}

int mxf_write_delta_entry(MXFFile *mxfFile, MXFDeltaEntry *entry)
{
    CHK_ORET(mxf_write_int8(mxfFile, entry->posTableIndex));
    CHK_ORET(mxf_write_uint8(mxfFile, entry->slice));
    CHK_ORET(mxf_write_uint32(mxfFile, entry->elementData));

    return 1;
}

int mxf_write_index_entry_array_header(MXFFile *mxfFile, uint8_t sliceCount, uint8_t posTableCount,
                                       uint32_t numIndexEntries)
{
    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3f0a, (uint16_t)(8 + numIndexEntries *
                                                                (11 + sliceCount * 4 + posTableCount * 8))));
    CHK_ORET(mxf_write_uint32(mxfFile, numIndexEntries));
    CHK_ORET(mxf_write_uint32(mxfFile, 11 + sliceCount * 4 + posTableCount * 8));

    return 1;
}

int mxf_write_index_entry(MXFFile *mxfFile, uint8_t sliceCount, uint8_t posTableCount, MXFIndexEntry *entry)
{
    uint32_t i;

    CHK_ORET(mxf_write_uint8(mxfFile, entry->temporalOffset));
    CHK_ORET(mxf_write_uint8(mxfFile, entry->keyFrameOffset));
    CHK_ORET(mxf_write_uint8(mxfFile, entry->flags));
    CHK_ORET(mxf_write_uint64(mxfFile, entry->streamOffset));
    for (i = 0; i < sliceCount; i++)
    {
        CHK_ORET(mxf_write_uint32(mxfFile, entry->sliceOffset[i]));
    }
    for (i = 0; i < posTableCount; i++)
    {
        CHK_ORET(mxf_write_int32(mxfFile, entry->posTable[i].numerator));
        CHK_ORET(mxf_write_int32(mxfFile, entry->posTable[i].denominator));
    }

    return 1;
}

