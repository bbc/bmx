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

#ifndef MXF_INDEX_HELPER_H_
#define MXF_INDEX_HELPER_H_


typedef struct FileIndex FileIndex;


int create_index(MXFFile *mxfFile, MXFList *partitions, uint32_t indexSID, uint32_t bodySID, FileIndex **index);
void free_index(FileIndex **index);

int set_position(MXFFile *mxfFile, FileIndex *index, mxfPosition frameNumber);
int64_t ix_get_last_written_frame_number(MXFFile *mxfFile, FileIndex *index, int64_t duration);
int end_of_essence(FileIndex *index);

void set_next_kl(FileIndex *index, const mxfKey *key, uint8_t llen, uint64_t len);
void get_next_kl(FileIndex *index, mxfKey *key, uint8_t *llen, uint64_t *len);
void get_start_cp_key(FileIndex *index, mxfKey *key);
uint64_t get_cp_len(FileIndex *index);

void increment_current_position(FileIndex *index);

mxfPosition get_current_position(FileIndex *index);
mxfLength get_indexed_duration(FileIndex *index);


#endif

