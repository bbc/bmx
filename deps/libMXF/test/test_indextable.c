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

#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>



int test_read(const char *filename)
{
    MXFFile *mxfFile = NULL;
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int i;
    int k;
    MXFIndexTableSegment *indexSegment = NULL;
    MXFDeltaEntry *deltaEntry;
    MXFIndexEntry *indexEntry;


    if (!mxf_disk_file_open_read(filename, &mxfFile))
    {
        mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    mxf_initialise_file_partitions(&partitions);

    /* read header pp */
    CHK_OFAIL(mxf_read_header_pp_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &headerPartition));
    CHK_OFAIL(mxf_append_partition(&partitions, headerPartition));


    /* TEST */

    /* read index table segment */

    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_index_table_segment(&key));
    CHK_OFAIL(mxf_read_index_table_segment(mxfFile, len, &indexSegment));
    CHK_OFAIL(indexSegment->indexEditRate.numerator == 25 && indexSegment->indexEditRate.denominator == 1);
    CHK_OFAIL(indexSegment->indexStartPosition == 0);
    CHK_OFAIL(indexSegment->indexDuration == 0x64);
    CHK_OFAIL(indexSegment->editUnitByteCount == 0x100);
    CHK_OFAIL(indexSegment->indexSID == 1);
    CHK_OFAIL(indexSegment->bodySID == 2);
    CHK_OFAIL(indexSegment->sliceCount == 0);
    CHK_OFAIL(indexSegment->posTableCount == 0);
    CHK_OFAIL(indexSegment->extStartOffset == 0);
    CHK_OFAIL(indexSegment->vbeByteCount == 0);
    CHK_OFAIL(indexSegment->singleIndexLocation == MXF_OPT_BOOL_NOT_PRESENT);
    CHK_OFAIL(indexSegment->singleEssenceLocation == MXF_OPT_BOOL_NOT_PRESENT);
    CHK_OFAIL(indexSegment->forwardIndexDirection == MXF_OPT_BOOL_NOT_PRESENT);
    CHK_OFAIL(indexSegment->deltaEntryArray != 0);
    CHK_OFAIL(indexSegment->indexEntryArray == 0);
    deltaEntry = indexSegment->deltaEntryArray;
    for (i = 0; i < 4; i++)
    {
        CHK_OFAIL(deltaEntry != 0);

        CHK_OFAIL(deltaEntry->posTableIndex == i);
        CHK_OFAIL(deltaEntry->slice == i);
        CHK_OFAIL((int)deltaEntry->elementData == i);

        deltaEntry = deltaEntry->next;
    }

    mxf_free_index_table_segment(&indexSegment);

    /* read index table segment */

    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_index_table_segment(&key));
    CHK_OFAIL(mxf_read_index_table_segment(mxfFile, len, &indexSegment));
    CHK_OFAIL(indexSegment->indexEditRate.numerator == 25 && indexSegment->indexEditRate.denominator == 1);
    CHK_OFAIL(indexSegment->indexStartPosition == 0);
    CHK_OFAIL(indexSegment->indexDuration == 0x0a);
    CHK_OFAIL(indexSegment->editUnitByteCount == 0);
    CHK_OFAIL(indexSegment->indexSID == 1);
    CHK_OFAIL(indexSegment->bodySID == 2);
    CHK_OFAIL(indexSegment->sliceCount == 2);
    CHK_OFAIL(indexSegment->posTableCount == 2);
    CHK_OFAIL(indexSegment->extStartOffset == 0);
    CHK_OFAIL(indexSegment->vbeByteCount == 1);
    CHK_OFAIL(indexSegment->singleIndexLocation == MXF_OPT_BOOL_NOT_PRESENT);
    CHK_OFAIL(indexSegment->singleEssenceLocation == MXF_OPT_BOOL_TRUE);
    CHK_OFAIL(indexSegment->forwardIndexDirection == MXF_OPT_BOOL_FALSE);
    CHK_OFAIL(indexSegment->deltaEntryArray == 0);
    CHK_OFAIL(indexSegment->indexEntryArray != 0);
    indexEntry = indexSegment->indexEntryArray;
    for (i = 0; i < indexSegment->indexDuration; i++)
    {
        CHK_OFAIL(indexEntry != 0);

        CHK_OFAIL(indexEntry->temporalOffset == i);
        CHK_OFAIL(indexEntry->keyFrameOffset == i);
        CHK_OFAIL(indexEntry->flags == i);
        CHK_OFAIL((int)indexEntry->streamOffset == i);

        for (k = 0; k < indexSegment->sliceCount; k++)
        {
            CHK_OFAIL((int)indexEntry->sliceOffset[k] == i);
        }

        for (k = 0; k < indexSegment->posTableCount; k++)
        {
            CHK_OFAIL(indexEntry->posTable[k].numerator == i);
            CHK_OFAIL(indexEntry->posTable[k].denominator == i + 1);
        }
        indexEntry = indexEntry->next;
    }



    mxf_free_index_table_segment(&indexSegment);
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    return 1;

fail:
    mxf_free_index_table_segment(&indexSegment);
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    return 0;
}

int test_create_and_write(const char *filename)
{
    MXFFile *mxfFile = NULL;
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    MXFIndexTableSegment *indexSegment = NULL;
    uint32_t sliceOffset[2];
    mxfRational posTable[2];
    const mxfRational editRate = {25, 1};
    int i;
    int k;


    if (!mxf_disk_file_open_new(filename, &mxfFile))
    {
        mxf_log_error("Failed to create '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    mxf_initialise_file_partitions(&partitions);

    /* TEST */

    /* write the header pp */
    CHK_OFAIL(mxf_append_new_partition(&partitions, &headerPartition));
    headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    headerPartition->kagSize = 256;
    CHK_OFAIL(mxf_write_partition(mxfFile, headerPartition));

    /* write index table segment */
    CHK_OFAIL(mxf_mark_index_start(mxfFile, headerPartition));

    CHK_OFAIL(mxf_create_index_table_segment(&indexSegment));
    mxf_generate_uuid(&indexSegment->instanceUID);
    indexSegment->indexEditRate = editRate;
    indexSegment->indexDuration = 0x64;
    indexSegment->editUnitByteCount = 0x100;
    indexSegment->indexSID = 1;
    indexSegment->bodySID = 2;
    for (i = 0; i < 4; i++)
    {
        CHK_OFAIL(mxf_default_add_delta_entry(NULL, 0, indexSegment, i, i, i));
    }
    CHK_OFAIL(mxf_write_index_table_segment(mxfFile, indexSegment));

    CHK_OFAIL(mxf_fill_to_kag(mxfFile, headerPartition));

    CHK_OFAIL(mxf_mark_index_end(mxfFile, headerPartition));

    mxf_free_index_table_segment(&indexSegment);


    /* write index table segment */
    CHK_OFAIL(mxf_mark_index_start(mxfFile, headerPartition));

    CHK_OFAIL(mxf_create_index_table_segment(&indexSegment));
    mxf_generate_uuid(&indexSegment->instanceUID);
    indexSegment->indexEditRate = editRate;
    indexSegment->indexDuration = 0x0a;
    indexSegment->editUnitByteCount = 0;
    indexSegment->indexSID = 1;
    indexSegment->bodySID = 2;
    indexSegment->sliceCount = 2;
    indexSegment->posTableCount = 2;
    indexSegment->vbeByteCount = 1;
    indexSegment->singleEssenceLocation = MXF_OPT_BOOL_TRUE;
    indexSegment->forwardIndexDirection = MXF_OPT_BOOL_FALSE;
    for (i = 0; i < indexSegment->indexDuration; i++)
    {
        for (k = 0; k < indexSegment->sliceCount; k++)
        {
            sliceOffset[k] = i;
        }
        for (k = 0; k < indexSegment->posTableCount; k++)
        {
            posTable[k].numerator = i;
            posTable[k].denominator = i + 1;
        }
        CHK_OFAIL(mxf_default_add_index_entry(NULL, 0, indexSegment, i, i, i, i,
            sliceOffset, posTable));
    }
    CHK_OFAIL(mxf_write_index_table_segment(mxfFile, indexSegment));

    CHK_OFAIL(mxf_fill_to_kag(mxfFile, headerPartition));

    CHK_OFAIL(mxf_mark_index_end(mxfFile, headerPartition));



    /* update the partitions */
    CHK_OFAIL(mxf_update_partitions(mxfFile, &partitions));


    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_free_index_table_segment(&indexSegment);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_free_index_table_segment(&indexSegment);
    return 0;
}


void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s filename\n", cmd);
}

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }

    /* replace util functions to ensure files are identical */
    mxf_set_regtest_funcs();


    if (!test_create_and_write(argv[1]))
    {
        return 1;
    }

    if (!test_read(argv[1]))
    {
        return 1;
    }

    return 0;
}

