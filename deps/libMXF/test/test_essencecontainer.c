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


const mxfKey g_bwfClipWrappedEEKey = MXF_AES3BWF_EE_K(1, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 1);
const mxfKey g_bwfFrameWrappedEEKey = MXF_AES3BWF_EE_K(1, MXF_BWF_FRAME_WRAPPED_EE_TYPE, 1);



int test_read(const char *filename)
{
    MXFFile *mxfFile = NULL;
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    MXFPartition *bodyPartition1 = NULL;
    MXFPartition *bodyPartition2 = NULL;
    MXFPartition *footerPartition = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint8_t essenceData[1024];
    MXFEssenceElement *essenceElement = NULL;
    uint32_t numRead;

    if (!mxf_disk_file_open_read(filename, &mxfFile))
    {
        mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    mxf_initialise_file_partitions(&partitions);


    /* TEST */

    /* read header pp */
    CHK_OFAIL(mxf_read_header_pp_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &headerPartition));
    CHK_OFAIL(mxf_append_partition(&partitions, headerPartition));

    /* read essence data directly */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_gc_essence_element(&key));
    CHK_OFAIL(llen == 4 && len == 256);
    CHK_OFAIL(mxf_file_read(mxfFile, essenceData, (uint32_t)len));

    /* read body pp 1 */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_partition_pack(&key));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &bodyPartition1));
    CHK_OFAIL(mxf_append_partition(&partitions, bodyPartition1));

    /* read essence data using MXFEssenceElement */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_open_essence_element_read(mxfFile, &key, llen, len, &essenceElement));
    CHK_OFAIL(llen == 8 && len == 1024);
    CHK_OFAIL(mxf_read_essence_element_data(mxfFile, essenceElement, (uint32_t)len, essenceData, &numRead));
    CHK_OFAIL(numRead == len);
    mxf_close_essence_element(&essenceElement);


    /* read body pp 2 */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_partition_pack(&key));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &bodyPartition2));
    CHK_OFAIL(mxf_append_partition(&partitions, bodyPartition2));

    /* read essence data using MXFEssenceElement */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_open_essence_element_read(mxfFile, &key, llen, len, &essenceElement));
    CHK_OFAIL(llen == 8 && len == 1024);
    CHK_OFAIL(mxf_read_essence_element_data(mxfFile, essenceElement, 256, essenceData, &numRead));
    CHK_OFAIL(numRead == 256);
    CHK_OFAIL(mxf_read_essence_element_data(mxfFile, essenceElement, 768, &essenceData[256], &numRead));
    CHK_OFAIL(numRead == 768);
    mxf_close_essence_element(&essenceElement);


    /* read footer pp */
    CHK_OFAIL(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_is_partition_pack(&key));
    CHK_OFAIL(mxf_read_partition(mxfFile, &key, len, &footerPartition));
    CHK_OFAIL(mxf_append_partition(&partitions, footerPartition));



    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_close_essence_element(&essenceElement);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_close_essence_element(&essenceElement);
    return 0;
}

int test_create_and_write(const char *filename)
{
    MXFFile *mxfFile = NULL;
    MXFFilePartitions partitions;
    MXFPartition *headerPartition = NULL;
    MXFPartition *bodyPartition1 = NULL;
    MXFPartition *bodyPartition2 = NULL;
    MXFPartition *footerPartition = NULL;
    MXFEssenceElement *essenceElement = NULL;
    uint8_t essenceData[1024];
    memset(essenceData, 0, 1024);


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
    headerPartition->bodySID = 1;
    CHK_OFAIL(mxf_append_partition_esscont_label(headerPartition, &MXF_EC_L(BWFFrameWrapped)));
    CHK_OFAIL(mxf_append_partition_esscont_label(headerPartition, &MXF_EC_L(BWFClipWrapped)));
    CHK_OFAIL(mxf_write_partition(mxfFile, headerPartition));

    /* write essence element directly */
    CHK_OFAIL(mxf_write_fixed_kl(mxfFile, &g_bwfFrameWrappedEEKey, 4, 256));
    CHK_OFAIL(mxf_file_write(mxfFile, essenceData, 256));

    /* write the body pp 1 */
    CHK_OFAIL(mxf_append_new_from_partition(&partitions, headerPartition, &bodyPartition1));
    bodyPartition1->key = MXF_PP_K(ClosedComplete, Body);
    bodyPartition1->bodySID = 2;
    CHK_OFAIL(mxf_write_partition(mxfFile, bodyPartition1));

    /* write using MXFEssenceElement with known length */
    CHK_OFAIL(mxf_open_essence_element_write(mxfFile, &g_bwfClipWrappedEEKey, 8, 1024, &essenceElement));
    CHK_OFAIL(mxf_write_essence_element_data(mxfFile, essenceElement, essenceData, 1024));
    mxf_close_essence_element(&essenceElement);


    /* write the body pp 2 */
    CHK_OFAIL(mxf_append_new_from_partition(&partitions, headerPartition, &bodyPartition2));
    bodyPartition2->key = MXF_PP_K(ClosedComplete, Body);
    bodyPartition2->bodySID = 3;
    CHK_OFAIL(mxf_write_partition(mxfFile, bodyPartition2));

    /* write using MXFEssenceElement with uknown length */
    CHK_OFAIL(mxf_open_essence_element_write(mxfFile, &g_bwfClipWrappedEEKey, 8, 0, &essenceElement));
    CHK_OFAIL(mxf_write_essence_element_data(mxfFile, essenceElement, essenceData, 256));
    CHK_OFAIL(mxf_write_essence_element_data(mxfFile, essenceElement, essenceData, 512));
    CHK_OFAIL(mxf_write_essence_element_data(mxfFile, essenceElement, essenceData, 256));
    CHK_ORET(mxf_finalize_essence_element_write(mxfFile, essenceElement));
    mxf_close_essence_element(&essenceElement);


    /* write the footer pp */
    CHK_OFAIL(mxf_append_new_from_partition(&partitions, headerPartition, &footerPartition));
    footerPartition->key = MXF_PP_K(ClosedComplete, Footer);
    CHK_OFAIL(mxf_write_partition(mxfFile, footerPartition));

    /* update the partitions */
    CHK_OFAIL(mxf_update_partitions(mxfFile, &partitions));


    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_close_essence_element(&essenceElement);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    mxf_clear_file_partitions(&partitions);
    mxf_close_essence_element(&essenceElement);
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

