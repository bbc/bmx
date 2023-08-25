/*
 * MXF file partitions
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

#ifndef MXF_PARTITION_H_
#define MXF_PARTITION_H_


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct
{
    mxfUL key;
    uint16_t majorVersion;
    uint16_t minorVersion;
    uint32_t kagSize;
    uint64_t thisPartition;
    uint64_t previousPartition;
    uint64_t footerPartition;
    uint64_t headerByteCount;
    uint64_t indexByteCount;
    uint32_t indexSID;
    uint64_t bodyOffset;
    uint32_t bodySID;
    mxfUL operationalPattern;
    MXFList essenceContainers;

    /* used when marking header and index starts to calc header/indexByteCount */
    int64_t headerMarkInPos;
    int64_t indexMarkInPos;
} MXFPartition;

typedef MXFList MXFFilePartitions;

typedef struct
{
    uint32_t bodySID;
    uint64_t thisPartition;
} MXFRIPEntry;

typedef struct
{
    MXFList entries;
} MXFRIP;


int mxf_is_header_partition_pack(const mxfKey *key);
int mxf_is_body_partition_pack(const mxfKey *key);
int mxf_is_generic_stream_partition_pack(const mxfKey *key);
int mxf_is_footer_partition_pack(const mxfKey *key);
int mxf_is_partition_pack(const mxfKey *key);
int mxf_partition_is_closed(const mxfKey *key);
int mxf_partition_is_complete(const mxfKey *key);
int mxf_partition_is_closed_and_complete(const mxfKey *key);

int mxf_create_file_partitions(MXFFilePartitions **partitions);
void mxf_free_file_partitions(MXFFilePartitions **partitions);
void mxf_initialise_file_partitions(MXFFilePartitions *partitions);
void mxf_clear_file_partitions(MXFFilePartitions *partitions);
void mxf_clear_rip(MXFRIP *rip);

int mxf_create_partition(MXFPartition **partition);
int mxf_create_from_partition(const MXFPartition *sourcePartition, MXFPartition **partition);
void mxf_free_partition(MXFPartition **partition);

void mxf_initialise_partition(MXFPartition *partition);
int mxf_initialise_with_partition(const MXFPartition *sourcePartition, MXFPartition *partition);
void mxf_clear_partition(MXFPartition *partition);

int mxf_append_new_partition(MXFFilePartitions *partitions, MXFPartition **partition);
int mxf_append_new_from_partition(MXFFilePartitions *partitions, MXFPartition *sourcePartition,
                                  MXFPartition **partition);
int mxf_append_partition(MXFFilePartitions *partitions, MXFPartition *partition);

int mxf_mark_header_start(MXFFile *mxfFile, MXFPartition *partition);
int mxf_mark_header_end(MXFFile *mxfFile, MXFPartition *partition);
int mxf_mark_index_start(MXFFile *mxfFile, MXFPartition *partition);
int mxf_mark_index_end(MXFFile *mxfFile, MXFPartition *partition);

int mxf_append_partition_esscont_label(MXFPartition *partition, const mxfUL *label);

int mxf_write_partition(MXFFile *mxfFile, MXFPartition *partition);
int mxf_update_partitions(MXFFile *mxfFile, MXFFilePartitions *partitions);
void mxf_update_partitions_in_memory(MXFFilePartitions *partitions);
int mxf_rewrite_partitions(MXFFile *mxfFile, MXFFilePartitions *partitions);
int mxf_read_partition(MXFFile *mxfFile, const mxfKey *key, uint64_t len, MXFPartition **partition);

int mxf_is_filler(const mxfKey *key);
int mxf_fill_to_kag(MXFFile *mxfFile, MXFPartition *partition);
int mxf_fill_to_position(MXFFile *mxfFile, uint64_t position);
int mxf_allocate_space_to_kag(MXFFile *mxfFile, MXFPartition *partition, uint32_t size);
int mxf_allocate_space(MXFFile *mxfFile, uint32_t size);
int mxf_write_fill(MXFFile *mxfFile, uint32_t size);
int mxf_read_next_nonfiller_kl(MXFFile *mxfFile, mxfKey *key, uint8_t *llen, uint64_t *len);

int mxf_read_rip(MXFFile *mxfFile, MXFRIP *rip);
int mxf_write_rip(MXFFile *mxfFile, MXFFilePartitions *partitions);

int mxf_read_header_pp_kl_with_runin(MXFFile *mxfFile, mxfKey *key, uint8_t *llen, uint64_t *len);
int mxf_read_header_pp_kl(MXFFile *mxfFile, mxfKey *key, uint8_t *llen, uint64_t *len);

int mxf_find_footer_partition(MXFFile *mxfFile);


#ifdef __cplusplus
}
#endif


#endif


