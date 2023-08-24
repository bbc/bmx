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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "timecode_index.h"
#include <mxf/mxf_utils.h>
#include <mxf/mxf_logging.h>
#include <mxf/mxf_macros.h>



static void free_index_array(TimecodeIndexArray **indexArray)
{
    if (*indexArray == NULL)
    {
        return;
    }

    SAFE_FREE((*indexArray)->elements);

    SAFE_FREE(*indexArray);
}

static void free_index_array_in_list(void *data)
{
    TimecodeIndexArray *indexArray;

    if (data == NULL)
    {
        return;
    }

    indexArray = (TimecodeIndexArray*)data;
    free_index_array(&indexArray);
}

static int64_t timecode_to_position(const ArchiveTimecode *timecode, uint16_t roundedTCBase, int allowDropFrame)
{
    int64_t position = timecode->hour * 60 * 60 * roundedTCBase +
                       timecode->min * 60 * roundedTCBase +
                       timecode->sec * roundedTCBase +
                       timecode->frame;

    if (allowDropFrame && timecode->dropFrame)
    {
        // first 2 frame numbers shall be omitted at the start of each minute,
        //   except minutes 0, 10, 20, 30, 40 and 50

        position -= (60-6) * 2 * timecode->hour;   // every whole hour
        position -= (timecode->min / 10) * 9 * 2;  // every whole 10 min
        position -= (timecode->min % 10) * 2;      // every whole min, except min 0 in 10
    }

    return position;
}

static void position_to_timecode(int64_t position, uint16_t roundedTCBase, ArchiveTimecode *timecode)
{
    timecode->hour  = (uint8_t)(  position / (60 * 60 * roundedTCBase));
    timecode->min   = (uint8_t)(( position % (60 * 60 * roundedTCBase)) / (60 * roundedTCBase));
    timecode->sec   = (uint8_t)(((position % (60 * 60 * roundedTCBase)) % (60 * roundedTCBase)) / roundedTCBase);
    timecode->frame = (uint8_t)(((position % (60 * 60 * roundedTCBase)) % (60 * roundedTCBase)) % roundedTCBase);
    timecode->dropFrame = 0;
}

static int move_timecode_index_searcher_to_next_element(TimecodeIndexSearcher *searcher)
{
    TimecodeIndexArray *indexArray;
    TimecodeIndexElement *arrayElement;
    TimecodeIndexSearcher searcherCopy = *searcher;

    if (searcher->atEnd)
    {
        return 0;
    }


    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    arrayElement = &indexArray->elements[searcher->elementNum];

    if (searcher->elementNum + 1 < indexArray->numElements)
    {
        /* move to next element in array */
        searcher->position += arrayElement->duration - searcher->elementOffset;
        searcher->elementOffset = 0;
        searcher->elementNum++;
        return 1;
    }
    else if (indexArray->numElements == searcher->index->arraySize)
    {
        /* move to next array in list */
        searcher->position += arrayElement->duration - searcher->elementOffset;
        searcher->elementOffset = 0;
        searcher->elementNum = 0;
        searcher->atEnd = !mxf_next_list_iter_element(&searcher->indexArrayIter);
        if (!searcher->atEnd)
        {
            return 1;
        }
        /* else at end of index */
    }
    /* else at end of index */

    *searcher = searcherCopy;
    return 0;
}

static int move_timecode_index_searcher(TimecodeIndexSearcher *searcher, int64_t position)
{
    TimecodeIndexArray *indexArray;
    TimecodeIndexElement *arrayElement;
    TimecodeIndexSearcher searcherCopy = *searcher;

    if (position == searcher->position)
    {
        return 1;
    }
    if (searcher->atEnd || position < searcher->position)
    {
        return 0;
    }


    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    while (1)
    {
        arrayElement = &indexArray->elements[searcher->elementNum];

        if (position < searcher->position + (arrayElement->duration - searcher->elementOffset))
        {
            /* found it the right element */
            searcher->elementOffset += position - searcher->position;
            searcher->position = position;
            searcher->beforeStart = 0;
            return 1;
        }
        else if (searcher->elementNum + 1 < indexArray->numElements)
        {
            /* move to next element in array */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum++;
        }
        else if (searcher->elementNum + 1 >= indexArray->numElements &&
            indexArray->numElements == searcher->index->arraySize)
        {
            /* move to next array in list */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum = 0;
            searcher->atEnd = !mxf_next_list_iter_element(&searcher->indexArrayIter);
            if (searcher->atEnd)
            {
                /* end of index */
                break;
            }
            indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
        }
        else
        {
            /* end of index */
            break;
        }
    }


    *searcher = searcherCopy;
    return 0;
}

static int find_frozen_timecode_at_offset(TimecodeIndexSearcher *searcher, int64_t offset)
{
    TimecodeIndexArray *indexArray;
    TimecodeIndexElement *arrayElement;

    if (searcher->atEnd)
    {
        return 0;
    }


    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    arrayElement = &indexArray->elements[searcher->elementNum];

    if (arrayElement->frozen && searcher->elementOffset + offset < arrayElement->duration)
    {
        searcher->elementOffset += offset;
        searcher->position += offset;
        return 1;
    }

    return 0;
}



void initialise_timecode_index(TimecodeIndex *index, const mxfRational *frameRate, int arraySize)
{
    mxf_initialise_list(&index->indexArrays, free_index_array_in_list);
    index->arraySize = arraySize;
    index->roundedTCBase = (uint16_t)(frameRate->numerator / (float)frameRate->denominator + 0.5);
    index->allowDropFrame = (frameRate->denominator == 1001);
}

void clear_timecode_index(TimecodeIndex *index)
{
    mxf_clear_list(&index->indexArrays);
}

int add_timecode_to_index(TimecodeIndex *index, ArchiveTimecode *timecode)
{
    TimecodeIndexArray *newArray = NULL;
    TimecodeIndexArray *lastArray;
    int64_t timecodePos = timecode_to_position(timecode, index->roundedTCBase, index->allowDropFrame);


    if (mxf_get_list_length(&index->indexArrays) == 0)
    {
        CHK_MALLOC_OFAIL(newArray, TimecodeIndexArray);
        CHK_MALLOC_ARRAY_OFAIL(newArray->elements, TimecodeIndexElement, index->arraySize);
        newArray->numElements = 0;
        CHK_OFAIL(mxf_append_list_element(&index->indexArrays, newArray));
        newArray = NULL; /* list has ownership */
    }

    lastArray = (TimecodeIndexArray*)mxf_get_last_list_element(&index->indexArrays);

    if (lastArray->numElements != 0)
    {
        if (lastArray->elements[lastArray->numElements - 1].timecodePos +
                lastArray->elements[lastArray->numElements - 1].duration == timecodePos)
        {
            /* timecode is previous + 1 */
            lastArray->elements[lastArray->numElements - 1].duration++;
        }
        else if (lastArray->elements[lastArray->numElements - 1].timecodePos == timecodePos &&
                    (lastArray->elements[lastArray->numElements - 1].frozen ||
                        lastArray->elements[lastArray->numElements - 1].duration == 1))
        {
            /* timecode is frozen with the previous timecode value */
            lastArray->elements[lastArray->numElements - 1].frozen = 1;
            lastArray->elements[lastArray->numElements - 1].duration++;
        }
        else
        {
            /* timecode is not frozen or previous + 1 */

            if (lastArray->numElements == index->arraySize)
            {
                CHK_MALLOC_OFAIL(newArray, TimecodeIndexArray);
                CHK_MALLOC_ARRAY_OFAIL(newArray->elements, TimecodeIndexElement, index->arraySize);
                newArray->numElements = 0;
                CHK_OFAIL(mxf_append_list_element(&index->indexArrays, newArray));
                newArray = NULL; /* list has ownership */

                lastArray = (TimecodeIndexArray*)mxf_get_last_list_element(&index->indexArrays);
            }

            lastArray->numElements++;
            lastArray->elements[lastArray->numElements - 1].frozen = 0;
            lastArray->elements[lastArray->numElements - 1].timecodePos = timecodePos;
            lastArray->elements[lastArray->numElements - 1].duration = 1;
        }
    }
    else
    {
        lastArray->numElements++;
        lastArray->elements[lastArray->numElements - 1].frozen = 0;
        lastArray->elements[lastArray->numElements - 1].timecodePos = timecodePos;
        lastArray->elements[lastArray->numElements - 1].duration = 1;
    }


    return 1;

fail:
    free_index_array(&newArray);
    return 0;
}

int is_null_timecode_index(TimecodeIndex *index)
{
    TimecodeIndexArray *indexArray;
    size_t listLen;

    /* index is null if the index is empty or has a duration > 1 with timecode frozen at 00:00:00:00 */

    listLen = mxf_get_list_length(&index->indexArrays);
    if (listLen == 0)
    {
        return 1;
    }
    else if (listLen == 1)
    {
        indexArray = (TimecodeIndexArray*)mxf_get_first_list_element(&index->indexArrays);
        if (indexArray->numElements == 0 ||
            (indexArray->numElements == 1 && indexArray->elements[0].frozen &&
                indexArray->elements[0].timecodePos == 0 && indexArray->elements[0].duration > 1))
        {
            return 1;
        }
    }

    return 0;
}

void initialise_timecode_index_searcher(TimecodeIndex *index, TimecodeIndexSearcher *searcher)
{
    mxf_initialise_list_iter(&searcher->indexArrayIter, &index->indexArrays);
    searcher->elementNum = 0;
    searcher->elementOffset = 0;
    searcher->position = 0;
    searcher->index = index;
    searcher->atEnd = !mxf_next_list_iter_element(&searcher->indexArrayIter);
    searcher->beforeStart = 1;
}

int find_timecode(TimecodeIndexSearcher *searcher, int64_t position, ArchiveTimecode *timecode)
{
    TimecodeIndexArray *indexArray;
    TimecodeIndexElement *arrayElement;
    int64_t timecodePos;

    if (!move_timecode_index_searcher(searcher, position))
    {
        return 0;
    }

    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    arrayElement = &indexArray->elements[searcher->elementNum];
    if (arrayElement->frozen)
    {
        timecodePos = arrayElement->timecodePos;
    }
    else
    {
        timecodePos = arrayElement->timecodePos + searcher->elementOffset;
    }
    position_to_timecode(timecodePos, searcher->index->roundedTCBase, timecode);

    return 1;
}

int find_position(TimecodeIndexSearcher *searcher, const ArchiveTimecode *timecode, int64_t *position)
{
    TimecodeIndexArray *indexArray;
    TimecodeIndexElement *arrayElement;
    int64_t timecodePos = timecode_to_position(timecode, searcher->index->roundedTCBase, searcher->index->allowDropFrame);
    int doneFirst = 0;
    TimecodeIndexSearcher searcherCopy = *searcher;

    if (timecode->hour == INVALID_TIMECODE_HOUR)
    {
        return 0;
    }
    if (searcher->atEnd)
    {
        return 0;
    }

    indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
    while (1)
    {
        arrayElement = &indexArray->elements[searcher->elementNum];

        if ((doneFirst || searcher->beforeStart || timecodePos > arrayElement->timecodePos + searcher->elementOffset) &&
            timecodePos >= arrayElement->timecodePos + searcher->elementOffset &&
            timecodePos < arrayElement->timecodePos + arrayElement->duration)
        {
            /* found it in incrementing timecode element */
            searcher->position += timecodePos - (arrayElement->timecodePos + searcher->elementOffset);
            searcher->elementOffset = timecodePos - arrayElement->timecodePos;
            *position = searcher->position;
            searcher->beforeStart = 0;
            return 1;
        }
        else if (arrayElement->frozen && timecodePos == arrayElement->timecodePos)
        {
            /* found it in frozen timecode element - position is the searcher position */
            *position = searcher->position;
            searcher->beforeStart = 0;
            return 1;
        }
        else if (searcher->elementNum + 1 < indexArray->numElements)
        {
            /* move to next element in array */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum++;
        }
        else if (searcher->elementNum + 1 >= indexArray->numElements &&
                 indexArray->numElements == searcher->index->arraySize)
        {
            /* move to next array in list */
            searcher->position += arrayElement->duration - searcher->elementOffset;
            searcher->elementOffset = 0;
            searcher->elementNum = 0;
            searcher->atEnd = !mxf_next_list_iter_element(&searcher->indexArrayIter);
            if (searcher->atEnd)
            {
                /* end of index */
                break;
            }
            indexArray = (TimecodeIndexArray*)mxf_get_iter_element(&searcher->indexArrayIter);
        }
        else
        {
            /* end of index */
            break;
        }

        doneFirst = 1;
    }


    *searcher = searcherCopy;
    return 0;
}


int find_position_at_dual_timecode(TimecodeIndexSearcher *vitcSearcher, const ArchiveTimecode *vitcTimecode,
                                   TimecodeIndexSearcher *ltcSearcher, const ArchiveTimecode *ltcTimecode,
                                   int64_t *position)
{
    TimecodeIndexSearcher vitcSearcherCopy = *vitcSearcher;
    TimecodeIndexSearcher ltcSearcherCopy = *ltcSearcher;
    int64_t prevVITCPosition, vitcPosition;
    int64_t prevLTCPosition, ltcPosition;

    if (vitcTimecode->hour == INVALID_TIMECODE_HOUR &&
        ltcTimecode->hour == INVALID_TIMECODE_HOUR)
    {
        /* can't find position when both timecode are invalid */
        return 0;
    }

    /* find the first hit */
    if (vitcTimecode->hour != INVALID_TIMECODE_HOUR &&
        !find_position(vitcSearcher, vitcTimecode, &vitcPosition))
    {
        goto fail;
    }
    if (ltcTimecode->hour != INVALID_TIMECODE_HOUR &&
        !find_position(ltcSearcher, ltcTimecode, &ltcPosition))
    {
        goto fail;
    }

    if (ltcTimecode->hour == INVALID_TIMECODE_HOUR)
    {
        /* move the LTC timecode to the same position */
        if (!move_timecode_index_searcher(ltcSearcher, vitcPosition))
        {
            goto fail;
        }
    }
    else if (vitcTimecode->hour == INVALID_TIMECODE_HOUR)
    {
        /* move the VITC timecode to the same position */
        if (!move_timecode_index_searcher(vitcSearcher, ltcPosition))
        {
            goto fail;
        }
    }
    else
    {
        /* if they aren't at the same position then look further */
        while (vitcPosition != ltcPosition)
        {
            if (vitcPosition < ltcPosition)
            {
                if (find_frozen_timecode_at_offset(vitcSearcher, ltcPosition - vitcPosition))
                {
                    vitcPosition = ltcPosition;
                    break;
                }

                prevVITCPosition = vitcPosition;
                if (!find_position(vitcSearcher, vitcTimecode, &vitcPosition))
                {
                    goto fail;
                }
                if (prevVITCPosition == vitcPosition &&
                    !move_timecode_index_searcher_to_next_element(vitcSearcher))
                {
                    goto fail;
                }
            }
            else
            {
                if (find_frozen_timecode_at_offset(ltcSearcher, vitcPosition - ltcPosition))
                {
                    ltcPosition = vitcPosition;
                    break;
                }

                prevLTCPosition = ltcPosition;
                if (!find_position(ltcSearcher, ltcTimecode, &ltcPosition))
                {
                    goto fail;
                }
                if (prevLTCPosition == ltcPosition &&
                    !move_timecode_index_searcher_to_next_element(ltcSearcher))
                {
                    goto fail;
                }
            }
        }
    }

    *position = vitcSearcher->position;
    return 1;

fail:
    /* restore the previous state */
    *vitcSearcher = vitcSearcherCopy;
    *ltcSearcher = ltcSearcherCopy;
    return 0;
}

