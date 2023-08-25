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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mxf/mxf.h>
#include <mxf/mxf_rw_intl_file.h>
#include <mxf/mxf_memory_file.h>


#define BLOCK_SIZE          (64 * 1024)
#define DATA_SIZE           (2 * BLOCK_SIZE)
#define MEM_CHUNK_SIZE      (10 * BLOCK_SIZE)
#define MEM_READER_SIZE     (10 * BLOCK_SIZE)



#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in %s:%d\n", #cmd, __FILENAME__, __LINE__); \
        exit(1); \
    }



int main()
{
    MXFRWInterleaver *interleaver;
    MXFMemoryFile *memoryFile;
    MXFFile *reader;
    MXFFile *writer;
    unsigned char *data;
    unsigned char *readerData;

    data = malloc(DATA_SIZE);
    memset(data, 122, DATA_SIZE);
    readerData = malloc(MEM_READER_SIZE);
    memset(readerData, 122, MEM_READER_SIZE);


    CHECK(mxf_create_rw_intl(BLOCK_SIZE, 2 * 1024 * 1024, &interleaver));

    CHECK(mxf_mem_file_open_read(readerData, MEM_READER_SIZE, 0, &memoryFile));
    CHECK(mxf_rw_intl_open(interleaver, mxf_mem_file_get_file(memoryFile), 0, &reader));

    CHECK(mxf_mem_file_open_new(MEM_CHUNK_SIZE, 0, &memoryFile));
    CHECK(mxf_rw_intl_open(interleaver, mxf_mem_file_get_file(memoryFile), 1, &writer));


    CHECK(mxf_file_write(writer, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_write(writer, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_read(reader, data, DATA_SIZE / 4) == DATA_SIZE / 4);
    CHECK(mxf_file_read(reader, data, DATA_SIZE / 4) == DATA_SIZE / 4);
    CHECK(mxf_file_read(reader, data, DATA_SIZE / 4) == DATA_SIZE / 4);
    CHECK(mxf_file_read(reader, data, DATA_SIZE / 4) == DATA_SIZE / 4);


    mxf_file_close(&reader);
    mxf_file_close(&writer);
    mxf_free_rw_intl(&interleaver);

    free(readerData);
    free(data);

    return 0;
}

