/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef MXF_MEMORY_FILE_H_
#define MXF_MEMORY_FILE_H_


#ifdef __cplusplus
extern "C"
{
#endif


#include <mxf/mxf_file.h>


typedef struct MXFMemoryFile MXFMemoryFile;



int mxf_mem_file_open_new(uint32_t chunkSize, int64_t virtualStartPos, MXFMemoryFile **mxfMemFile);
int mxf_mem_file_open_read(const unsigned char *data, int64_t size, int64_t virtualStartPos, MXFMemoryFile **mxfMemFile);

MXFFile* mxf_mem_file_get_file(MXFMemoryFile *mxfMemFile);

size_t mxf_mem_file_get_num_chunks(MXFMemoryFile *mxfMemFile);
unsigned char* mxf_mem_file_get_chunk_data(MXFMemoryFile *mxfMemFile, size_t chunkIndex);
int64_t mxf_mem_file_get_chunk_size(MXFMemoryFile *mxfMemFile, size_t chunkIndex);

int64_t mxf_mem_file_get_size(MXFMemoryFile *mxfMemFile);

int mxf_mem_file_flush_to_file(MXFMemoryFile *mxfMemFile, MXFFile *mxfFile);



#ifdef __cplusplus
}
#endif


#endif

