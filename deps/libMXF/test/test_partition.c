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

#define RUNIN_LEN       8



int test_read(const char *filename)
{
    MXFFile *mxfFile = NULL;
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    MXFPartition *bodyPartition1 = NULL;
    MXFPartition *bodyPartition2 = NULL;
    MXFPartition *footerPartition = NULL;
    MXFListIterator iter;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int i;
    MXFRIP rip;

    if (!mxf_disk_file_open_read(filename, &mxfFile))
    {
        mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    mxf_initialise_file_partitions(&partitions);


    /* TEST */

    /* read RIP */
    CHK_OFAIL(mxf_read_rip(mxfFile, &rip));
    CHK_OFAIL(mxf_get_list_length(&rip.entries) == 4);
    CHK_OFAIL(mxf_file_seek(mxfFile, 0, SEEK_SET));
    mxf_initialise_list_iter(&iter, &rip.entries);
    i = 0;
    while (mxf_next_list_iter_element(&iter))
    {
        MXFRIPEntry *entry = (MXFRIPEntry*)mxf_get_iter_element(&iter);
        if (i == 0)
        {
            CHK_OFAIL(entry->bodySID == 0);
            CHK_OFAIL(entry->thisPartition == RUNIN_LEN);
        }
        else if (i == 1)
        {
            CHK_OFAIL(entry->bodySID == 2);
            CHK_OFAIL(entry->thisPartition == RUNIN_LEN + 1161);
        }
        else if (i == 2)
        {
            CHK_OFAIL(entry->bodySID == 3);
            CHK_OFAIL(entry->thisPartition == RUNIN_LEN + 1298);
        }
        else
        {
            CHK_OFAIL(entry->bodySID == 0);
            CHK_OFAIL(entry->thisPartition == RUNIN_LEN + 1554);
        }
        i++;
    }

    /* read header pp, allowing for runin */
    CHK_OFAIL(mxf_read_header_pp_kl_with_runin(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &headerPartition));
    CHK_OFAIL(mxf_append_partition(&partitions, headerPartition));
    CHK_OFAIL(headerPartition->indexSID == 1);
    CHK_OFAIL(headerPartition->bodySID == 0);
    CHK_OFAIL(mxf_get_list_length(&headerPartition->essenceContainers) == 2);
    mxf_initialise_list_iter(&iter, &headerPartition->essenceContainers);
    i = 0;
    while (mxf_next_list_iter_element(&iter))
    {
        mxfUL *label = (mxfUL*)mxf_get_iter_element(&iter);
        if (i == 0)
        {
            CHK_OFAIL(mxf_equals_ul(label, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)));
        }
        else
        {
            CHK_OFAIL(mxf_equals_ul(label, &MXF_EC_L(BWFFrameWrapped)));
        }
        i++;
    }

    /* skip filler and read body pp 1 */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_partition_pack(&key));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &bodyPartition1));
    CHK_OFAIL(bodyPartition1->indexSID == 0);
    CHK_OFAIL(bodyPartition1->bodySID == 2);
    CHK_OFAIL(mxf_append_partition(&partitions, bodyPartition1));

    /* skip filler and read body pp 2 */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_partition_pack(&key));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &bodyPartition2));
    CHK_OFAIL(bodyPartition2->indexSID == 0);
    CHK_OFAIL(bodyPartition2->bodySID == 3);
    CHK_OFAIL(mxf_append_partition(&partitions, bodyPartition2));

    /* skip filler and read footer pp */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_partition_pack(&key));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &footerPartition));
    CHK_OFAIL(footerPartition->bodySID == 0);
    CHK_OFAIL(footerPartition->indexSID == 1);
    CHK_OFAIL(mxf_append_partition(&partitions, footerPartition));
    mxf_initialise_list_iter(&iter, &footerPartition->essenceContainers);
    i = 0;
    while (mxf_next_list_iter_element(&iter))
    {
        mxfUL *label = (mxfUL*)mxf_get_iter_element(&iter);
        if (i == 0)
        {
            CHK_OFAIL(mxf_equals_ul(label, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)));
        }
        else
        {
            CHK_OFAIL(mxf_equals_ul(label, &MXF_EC_L(BWFFrameWrapped)));
        }
        i++;
    }



    mxf_clear_rip(&rip);
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    return 1;

fail:
    mxf_clear_rip(&rip);
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    return 0;
}

int test_create_and_write(const char *filename)
{
    MXFFile *mxfFile = NULL;
    uint8_t runin[RUNIN_LEN];
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    MXFPartition *bodyPartition1 = NULL;
    MXFPartition *bodyPartition2 = NULL;
    MXFPartition *footerPartition = NULL;


    if (!mxf_disk_file_open_new(filename, &mxfFile))
    {
        mxf_log_error("Failed to create '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    mxf_initialise_file_partitions(&partitions);

    /* TEST */

    /* write 8 bytes of runin */
    memset(runin, 0, RUNIN_LEN);
    CHK_OFAIL(mxf_file_write(mxfFile, runin, RUNIN_LEN) == RUNIN_LEN);

    /* write the header pp */
    CHK_OFAIL(mxf_append_new_partition(&partitions, &headerPartition));
    headerPartition->key = MXF_PP_K(ClosedComplete, Header);
    headerPartition->indexSID = 1;
    CHK_OFAIL(mxf_append_partition_esscont_label(headerPartition, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)));
    CHK_OFAIL(mxf_append_partition_esscont_label(headerPartition, &MXF_EC_L(BWFFrameWrapped)));
    CHK_OFAIL(mxf_write_partition(mxfFile, headerPartition));

    /* write empty header metadata */
    CHK_OFAIL(mxf_mark_header_start(mxfFile, headerPartition));
    CHK_OFAIL(mxf_allocate_space(mxfFile, 1024));
    CHK_OFAIL(mxf_mark_header_end(mxfFile, headerPartition));

    /* write the body pp 1 */
    CHK_OFAIL(mxf_append_new_from_partition(&partitions, headerPartition, &bodyPartition1));
    bodyPartition1->key = MXF_PP_K(ClosedComplete, Body);
    bodyPartition1->bodySID = 2;
    CHK_OFAIL(mxf_write_partition(mxfFile, bodyPartition1));

    /* write the body pp 2, with KAG 256 */
    CHK_OFAIL(mxf_append_new_from_partition(&partitions, headerPartition, &bodyPartition2));
    bodyPartition2->key = MXF_PP_K(ClosedComplete, Body);
    bodyPartition2->bodySID = 3;
    bodyPartition2->kagSize = 256;
    CHK_OFAIL(mxf_write_partition(mxfFile, bodyPartition2));
    CHK_OFAIL(mxf_fill_to_kag(mxfFile, bodyPartition2));

    /* write the footer pp */
    CHK_OFAIL(mxf_append_new_from_partition(&partitions, headerPartition, &footerPartition));
    footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    footerPartition->indexSID = 1;
    CHK_OFAIL(mxf_write_partition(mxfFile, footerPartition));

    /* write RIP */
    CHK_OFAIL(mxf_write_rip(mxfFile, &partitions));

    /* update the partitions */
    CHK_OFAIL(mxf_update_partitions(mxfFile, &partitions));


    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
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

