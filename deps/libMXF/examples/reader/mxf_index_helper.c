/*
 * Utility functions for navigating through the essence data
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "mxf_reader_int.h"
#include "mxf_index_helper.h"
#include <mxf/mxf_macros.h>


typedef struct
{
    MXFPartition *partition;
    int ownPartition;
    int64_t partitionStartPos; /* partition->thisPartition + mxf_get_runin_len() */
    int64_t partitionDataStartPos; /* file position after partition pack */
    int64_t essenceStartPos; /* file position of first essence content package */
    int64_t numContentPackages;
    mxfPosition startPosition; /* frame number corresponding to first content package */
} PartitionIndexEntry;

struct FileIndex
{
    uint32_t indexSID;
    uint32_t bodySID;

    MXFList partitionIndex;
    size_t currentPartition;

    int isComplete;

    mxfPosition currentPosition;
    mxfLength indexedDuration;

    mxfKey startContentPackageKey;
    uint64_t contentPackageLen;

    mxfKey nextKey;
    uint8_t nextLLen;
    uint64_t nextLen;
};




static void backup_index(FileIndex *index, FileIndex *backup)
{
    /* Note: not all data needs to be backed up */
    backup->currentPartition = index->currentPartition;
    backup->currentPosition = index->currentPosition;
    backup->nextKey = index->nextKey;
    backup->nextLLen = index->nextLLen;
    backup->nextLen = index->nextLen;
}

static void restore_index(FileIndex *backup, FileIndex *index)
{
    index->currentPartition = backup->currentPartition;
    index->currentPosition = backup->currentPosition;
    index->nextKey = backup->nextKey;
    index->nextLLen = backup->nextLLen;
    index->nextLen = backup->nextLen;
}

static void free_partition_index_entry(void *entry)
{
    if (entry == NULL)
    {
        return;
    }

    if (((PartitionIndexEntry*)entry)->ownPartition)
    {
        mxf_free_partition(&((PartitionIndexEntry*)entry)->partition);
    }
    else
    {
        ((PartitionIndexEntry*)entry)->partition = NULL;
    }
    free(entry);
}

static int partition_has_essence(FileIndex *index, PartitionIndexEntry *entry)
{
    return index->bodySID != 0 && index->bodySID == entry->partition->bodySID;
}

static int create_partition_index_entry(MXFFile *mxfFile, MXFPartition **partition,
    int takePartition, PartitionIndexEntry **entry)
{
    PartitionIndexEntry *newEntry = NULL;

    CHK_MALLOC_OFAIL(newEntry, PartitionIndexEntry);
    memset(newEntry, 0, sizeof(PartitionIndexEntry));
    newEntry->partitionDataStartPos = -1;
    newEntry->essenceStartPos = -1;
    newEntry->startPosition = -1;
    newEntry->partitionStartPos = mxf_get_runin_len(mxfFile) + (*partition)->thisPartition;
    CHK_OFAIL((newEntry->partitionDataStartPos = mxf_file_tell(mxfFile)) >= 0);

    newEntry->partition = *partition;
    if (takePartition)
    {
        newEntry->ownPartition = 1;
        *partition = NULL; /* entry now owns it */
    }

    *entry = newEntry;
    return 1;

fail:
    SAFE_FREE(newEntry);
    return 0;
}

static int add_partition_index_entry(MXFFile *mxfFile, FileIndex *index, const mxfKey *key, uint64_t len,
    int append, PartitionIndexEntry **entry)
{
    MXFPartition *partition = NULL;
    PartitionIndexEntry *newEntry = NULL;
    PartitionIndexEntry *prevEntry = NULL;
    PartitionIndexEntry *prevEssenceEntry = NULL;
    size_t numPartitions;
    size_t i;


    /* read the partition pack */
    CHK_ORET(mxf_is_partition_pack(key));
    CHK_ORET(mxf_read_partition(mxfFile, key, len, &partition));

    /* create entry */
    CHK_OFAIL(create_partition_index_entry(mxfFile, &partition, 1, &newEntry));

    if (append)
    {
        /* get the previous entry containing essence */
        prevEssenceEntry = NULL;
        numPartitions = mxf_get_list_length(&index->partitionIndex);
        i = numPartitions;
        while (i > 0) {
            i--;
            prevEntry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, i);
            if (partition_has_essence(index, prevEntry))
            {
                prevEssenceEntry = prevEntry;
                break;
            }
        }

        if (prevEssenceEntry != NULL)
        {
            /* calculate the number of content packages of previous entry */
            prevEssenceEntry->numContentPackages = (newEntry->partitionStartPos - prevEssenceEntry->essenceStartPos) /
                index->contentPackageLen;

            /* calculate the first frame number of the new entry if it contains essence */
            if (partition_has_essence(index, newEntry))
            {
                newEntry->startPosition = prevEssenceEntry->startPosition + prevEssenceEntry->numContentPackages;
            }
        }
        else
        {
            /* if partition has essence then it contains the first frame number */
            if (partition_has_essence(index, newEntry))
            {
                newEntry->startPosition = 0;
            }
        }

        CHK_OFAIL(mxf_append_list_element(&index->partitionIndex, newEntry));
        newEntry = NULL; /* list now owns it */
    }

    /* if partitions are prepended rather than appended then we don't calculate
       numContentPackages and startPosition */
    else
    {
        CHK_OFAIL(mxf_prepend_list_element(&index->partitionIndex, newEntry));
        newEntry = NULL; /* list now owns it */
    }

    /* the index is complete if we are in the footer */
    if (mxf_is_footer_partition_pack(key))
    {
        index->isComplete = 1;
    }


    *entry = newEntry;
    return 1;

fail:
    mxf_free_partition(&partition);
    SAFE_FREE(newEntry);
    return 0;
}

static int position_at_start_essence(MXFFile *mxfFile, FileIndex *index, PartitionIndexEntry *entry)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    CHK_ORET(mxf_file_is_seekable(mxfFile));
    CHK_ORET(partition_has_essence(index, entry));


    /* seek to previously found position */
    if (entry->essenceStartPos >= 0)
    {
        CHK_ORET(mxf_file_seek(mxfFile, entry->essenceStartPos, SEEK_SET));
        CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
    }
    else
    {
        /* seek to just after the partition pack */
        CHK_ORET(mxf_file_seek(mxfFile, entry->partitionDataStartPos, SEEK_SET));

        /* skip initial filler which is not included in any header or index byte counts */
        CHK_ORET(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));

        /* skip header metadata */
        if (entry->partition->headerByteCount > 0)
        {
            CHK_ORET(mxf_skip(mxfFile, entry->partition->headerByteCount - mxfKey_extlen - llen));
            CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
        }

        /* skip index table segments */
        if (entry->partition->indexByteCount > 0)
        {
            CHK_ORET(mxf_skip(mxfFile, entry->partition->indexByteCount - mxfKey_extlen - llen));
            CHK_ORET(mxf_read_kl(mxfFile, &key, &llen, &len));
        }

        /* check it is a essence element key and get file position */
        CHK_ORET(mxf_is_gc_essence_element(&key));
        CHK_ORET((entry->essenceStartPos = mxf_file_tell(mxfFile)) >= 0);
        entry->essenceStartPos -= (mxfKey_extlen + llen);
    }

    set_next_kl(index, &key, llen, len);

    return 1;
}

static int move_to_next_partition_with_essence(MXFFile *mxfFile, FileIndex *index)
{
    PartitionIndexEntry *entry = NULL;
    size_t nextPartition = (size_t)(-1);
    size_t numPartitions;
    size_t i;
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    /* try find next partition with essence */
    /* Note: index->currentPartition can be -1 */
    numPartitions = mxf_get_list_length(&index->partitionIndex);
    for (i = index->currentPartition + 1; i < numPartitions; i++)
    {
        entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, i);
        if (partition_has_essence(index, entry))
        {
            nextPartition = i;
            break;
        }
    }

    if (nextPartition == (size_t)(-1) && index->isComplete)
    {
        /* no next partition with essence found in a complete index */
        return 0;
    }
    else
    {
        /* move to next partition */
        if (nextPartition != (size_t)(-1))
        {
            index->currentPartition = nextPartition;
            entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, index->currentPartition);
            CHK_ORET(mxf_file_seek(mxfFile, entry->partitionDataStartPos, SEEK_SET));
        }
        /* else if we are not in the last indexed partition, then move to start of last indexed partition */
        else if (index->currentPartition + 1 < numPartitions)
        {
            index->currentPartition = numPartitions - 1;
            entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, index->currentPartition);
            CHK_ORET(mxf_file_seek(mxfFile, entry->partitionDataStartPos, SEEK_SET));
        }
        /* else index this new partition */
        else
        {
            CHK_ORET(add_partition_index_entry(mxfFile, index, &index->nextKey, index->nextLen, 1, &entry));
            index->currentPartition++;
        }

        /* continue until a partition is found with the essence data */
        while (!index->isComplete && !partition_has_essence(index, entry))
        {
            /* skip partition contents */
            if (!mxf_read_kl(mxfFile, &key, &llen, &len))
            {
                CHK_ORET(mxf_file_eof(mxfFile));
                set_next_kl(index, &g_Null_Key, 0, 0);
                return 0;
            }
            while (!mxf_is_partition_pack(&key))
            {
                CHK_ORET(mxf_skip(mxfFile, len));
                if (!mxf_read_kl(mxfFile, &key, &llen, &len))
                {
                    CHK_ORET(mxf_file_eof(mxfFile));
                    set_next_kl(index, &g_Null_Key, 0, 0);
                    return 0;
                }
            }

            /* create partition index entry */
            CHK_ORET(add_partition_index_entry(mxfFile, index, &key, len, 1, &entry));
            index->currentPartition++;
        }

        /* no partition with essence found */
        if (!partition_has_essence(index, entry))
        {
            return 0;
        }
    }

    /* position the file at the first content package in this partition */
    CHK_ORET(position_at_start_essence(mxfFile, index, entry));

    return 1;
}

static int complete_partition_index(MXFFile *mxfFile, FileIndex *index)
{
    PartitionIndexEntry *prevEntry = NULL;
    PartitionIndexEntry *entry = NULL;
    PartitionIndexEntry *nextEntry = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    size_t i;
    size_t numPartitions;


    /* get the content package length and first element key */

    index->contentPackageLen = 0;
    CHK_ORET(move_to_next_partition_with_essence(mxfFile, index));
    index->currentPosition = 0;
    index->startContentPackageKey = index->nextKey;

    key = index->nextKey;
    llen = index->nextLLen;
    len = index->nextLen;
    do
    {
        index->contentPackageLen += mxfKey_extlen + llen;

        CHK_ORET(mxf_skip(mxfFile, len));
        index->contentPackageLen += len;

        if (!mxf_read_kl(mxfFile, &key, &llen, &len))
        {
            CHK_ORET(mxf_file_eof(mxfFile));
            break;
        }
    }
    while (!mxf_equals_key(&key, &index->startContentPackageKey) && !mxf_is_partition_pack(&key));


    /* get the start of essence position, number of content packages and start position for each partition */

    numPartitions = mxf_get_list_length(&index->partitionIndex);
    prevEntry = NULL;
    for (i = 0; i < numPartitions; i++)
    {
        entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, i);
        if (partition_has_essence(index, entry))
        {
            /* position at start of essence, and this fills in the essenceStartPos */
            CHK_ORET(position_at_start_essence(mxfFile, index, entry));

            /* calculate the number of content packages */
            if (i + 1 < numPartitions)
            {
                nextEntry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, i + 1);
                entry->numContentPackages = (nextEntry->partitionStartPos - entry->essenceStartPos)
                    / index->contentPackageLen;
            }
            else
            {
                entry->numContentPackages = -1;
            }

            /* set the first frame number */
            if (prevEntry != NULL)
            {
                entry->startPosition = prevEntry->startPosition + prevEntry->numContentPackages;
            }
            else
            {
                entry->startPosition = 0;
            }

            /* update the indexed duration */
            if (entry->numContentPackages > 0)
            {
                index->indexedDuration += entry->numContentPackages;
            }

            prevEntry = entry;
        }

    }

    return 1;
}




int create_index(MXFFile *mxfFile, MXFList *partitions, uint32_t indexSID, uint32_t bodySID, FileIndex **index)
{
    FileIndex *newIndex;
    MXFListIterator iter;
    MXFPartition *partition;
    PartitionIndexEntry *entry = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    /* only index seekable files */
    CHK_ORET(mxf_file_is_seekable(mxfFile));

    CHK_MALLOC_ORET(newIndex, FileIndex);
    memset(newIndex, 0, sizeof(FileIndex));
    newIndex->indexSID = indexSID;
    newIndex->bodySID = bodySID;
    newIndex->currentPartition = -1;
    newIndex->currentPosition = -1;
    mxf_initialise_list(&newIndex->partitionIndex, free_partition_index_entry);


    /* add partitions to the index */
    mxf_initialise_list_iter(&iter, partitions);
    while (mxf_next_list_iter_element(&iter))
    {
        partition = (MXFPartition*)mxf_get_iter_element(&iter);

        /* move to just after the partition pack */
        CHK_OFAIL(mxf_file_seek(mxfFile, mxf_get_runin_len(mxfFile) + partition->thisPartition, SEEK_SET));
        CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
        CHK_OFAIL(mxf_skip(mxfFile, len));

        CHK_OFAIL(create_partition_index_entry(mxfFile, &partition, 0, &entry));
        CHK_OFAIL(mxf_append_list_element(&newIndex->partitionIndex, entry));
        entry = NULL; /* list now owns it */

        if (mxf_is_footer_partition_pack(&partition->key))
        {
            newIndex->isComplete = 1;
        }
    }

    /* complete the index */
    CHK_OFAIL(complete_partition_index(mxfFile, newIndex));

    /* position file at the start */
    newIndex->currentPartition = -1;
    newIndex->currentPosition = -1;
    CHK_OFAIL(move_to_next_partition_with_essence(mxfFile, newIndex));
    newIndex->currentPosition = 0;

    *index = newIndex;
    return 1;

fail:
    SAFE_FREE(entry);
    return 0;
}

void free_index(FileIndex **index)
{
    if (*index == NULL)
    {
        return;
    }

    mxf_clear_list(&(*index)->partitionIndex);

    SAFE_FREE(*index);
}

int set_position(MXFFile *mxfFile, FileIndex *index, mxfPosition position)
{
    PartitionIndexEntry *entry = NULL;
    PartitionIndexEntry *nextEntry = NULL;
    size_t numPartitions;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int64_t filePos;
    FileIndex backup;

    /* make backup of index state so that we can reset if something fails */
    backup_index(index, &backup);
    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);

    if (index->currentPosition < 0)
    {
        /* Note: index->currentPartition is assumed to be -1 */
        CHK_OFAIL(move_to_next_partition_with_essence(mxfFile, index));
        index->currentPosition = 0;
    }

    else if (position == index->currentPosition)
    {
        /* if we are not at the start of a content package, then move to start of essence in a following partition */
        if (!mxf_equals_key(&index->nextKey, &index->startContentPackageKey))
        {
            CHK_OFAIL(mxf_is_partition_pack(&index->nextKey));
            CHK_OFAIL(move_to_next_partition_with_essence(mxfFile, index));
        }
    }

    else if (position < index->currentPosition)
    {
        while (position < index->currentPosition)
        {
            entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, index->currentPartition);

            /* seek to position in this partition */
            if (partition_has_essence(index, entry) && position >= entry->startPosition)
            {
                CHK_OFAIL(mxf_file_seek(mxfFile,
                    entry->essenceStartPos + (position - entry->startPosition) * index->contentPackageLen,
                    SEEK_SET));
                index->currentPosition = position;

                if (!mxf_read_kl(mxfFile, &key, &llen, &len))
                {
                    CHK_OFAIL(mxf_file_eof(mxfFile));
                    set_next_kl(index, &g_Null_Key, 0, 0);
                    goto fail;
                }
                set_next_kl(index, &key, llen, len);
            }
            /* move to previous partition */
            else
            {
                CHK_OFAIL(index->currentPartition > 0);
                index->currentPartition--;
            }
        }
    }

    else /* frameNumber > index->currentPosition */
    {
        numPartitions = mxf_get_list_length(&index->partitionIndex);

        while (position > index->currentPosition)
        {
            /* this is not the last partition */
            if (index->currentPartition + 1 < numPartitions)
            {
                entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, index->currentPartition);
                nextEntry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, index->currentPartition + 1);

                /* seek to position in this partition */
                if ((uint64_t)(position - entry->startPosition) <=
                    (nextEntry->partitionStartPos - entry->essenceStartPos) / index->contentPackageLen)
                {
                    CHK_OFAIL(mxf_file_seek(mxfFile,
                        entry->essenceStartPos + (position - entry->startPosition) * index->contentPackageLen,
                        SEEK_SET));
                    index->currentPosition = position;

                    if (!mxf_read_kl(mxfFile, &key, &llen, &len))
                    {
                        CHK_OFAIL(mxf_file_eof(mxfFile));
                        set_next_kl(index, &g_Null_Key, 0, 0);
                        goto fail;
                    }
                    set_next_kl(index, &key, llen, len);
                }
                /* move to next partition if frame is not in this partition */
                else
                {
                    /* Note: because there is a following partition in the index we don't need to be
                       positioned at the start of the following partition */
                    CHK_OFAIL(move_to_next_partition_with_essence(mxfFile, index));
                    index->currentPosition = entry->startPosition;
                }
            }

            /* this is the last indexed partition */
            else
            {
                /* seek to position in partition if the index is complete*/
                /* TODO: HACK FOR INGEX PLAYER
                   SEEKING ONE FRAME AT A TIME IS UNNECCESSARY WHEN ONLY 1 PARTITION WITH
                   ESSENCE IS PRESENT */
                if (1==1) //index->isComplete)
                {
                    entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, index->currentPartition);
                    if (!partition_has_essence(index, entry))
                    {
                        /* end of essence reached */
                        set_next_kl(index, &g_Null_Key, 0, 0);
                        break;
                    }

                    CHK_OFAIL(mxf_file_seek(mxfFile,
                        entry->essenceStartPos + (position - entry->startPosition) * index->contentPackageLen,
                        SEEK_SET));
                    index->currentPosition = position;

                    if (!mxf_read_kl(mxfFile, &key, &llen, &len))
                    {
                        CHK_OFAIL(mxf_file_eof(mxfFile));
                        set_next_kl(index, &g_Null_Key, 0, 0);
                        goto fail;
                    }
                    set_next_kl(index, &key, llen, len);
                }

                /* move one content package at a time */
                else
                {
                    /* skip content package */
                    if (mxf_equals_key(&index->nextKey, &index->startContentPackageKey))
                    {
                        CHK_OFAIL(mxf_skip(mxfFile, index->contentPackageLen - mxfKey_extlen - index->nextLLen));
                        index->currentPosition++;

                        if (!mxf_read_kl(mxfFile, &key, &llen, &len))
                        {
                            CHK_OFAIL(mxf_file_eof(mxfFile));
                            set_next_kl(index, &g_Null_Key, 0, 0);
                            goto fail;
                        }
                        set_next_kl(index, &key, llen, len);
                    }
                    /* move to next partition */
                    else if (mxf_is_partition_pack(&index->nextKey))
                    {
                        CHK_OFAIL(move_to_next_partition_with_essence(mxfFile, index));
                    }
                    else
                    {
                        mxf_log_error("Unexpected key found in essence data" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
                        goto fail;
                    }
                }
            }
        }
    }

    if (position != index->currentPosition)
    {
        /* we end up here if we have reached the end of essence or file */
        goto fail;
    }

    return 1;

fail:
    restore_index(&backup, index);
    CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));
    return 0;
}

/* TODO/NOTE: this only works for a single partition !! */
int64_t ix_get_last_written_frame_number(MXFFile *mxfFile, FileIndex *index, int64_t duration)
{
    PartitionIndexEntry *entry = NULL;
    int64_t fileSize;
    int64_t targetPosition;

    if (duration == 0)
    {
        /* always on last frame ? */
        return 1;
    }

    if (index->isComplete)
    {
        /* easy */
        return index->indexedDuration - 1;
    }

    if (mxf_get_list_length(&index->partitionIndex) != 1)
    {
        /* only single partition with essence supported */
        return -1;
    }

    entry = (PartitionIndexEntry*)mxf_get_list_element(&index->partitionIndex, index->currentPartition);
    if (!partition_has_essence(index, entry))
    {
        /* end of essence reached */
        return -1;
    }


    /* get file size and calculate the furthest position */
    if ((fileSize = mxf_file_size(mxfFile)) < 0)
    {
        return -1;
    }
    targetPosition = entry->startPosition + ((fileSize - entry->essenceStartPos) / index->contentPackageLen) - 1;
    if (duration >= 0 && targetPosition >= duration)
    {
        targetPosition = duration - 1;
    }

    return targetPosition;
}

int end_of_essence(FileIndex *index)
{
    return index->currentPosition < 0 ||
           (index->isComplete && index->currentPosition >= index->indexedDuration);
}

void set_next_kl(FileIndex *index, const mxfKey *key, uint8_t llen, uint64_t len)
{
    index->nextKey = *key;
    index->nextLLen = llen;
    index->nextLen = len;
}

void get_next_kl(FileIndex *index, mxfKey *key, uint8_t *llen, uint64_t *len)
{
    *key = index->nextKey;
    *llen = index->nextLLen;
    *len = index->nextLen;
}

void get_start_cp_key(FileIndex *index, mxfKey *key)
{
    *key = index->startContentPackageKey;
}

uint64_t get_cp_len(FileIndex *index)
{
    return index->contentPackageLen;
}

void increment_current_position(FileIndex *index)
{
    index->currentPosition++;
}

mxfPosition get_current_position(FileIndex *index)
{
    return index->currentPosition;
}

mxfLength get_indexed_duration(FileIndex *index)
{
    return index->indexedDuration;
}

