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
#include <mxf/mxf_memory_file.h>


#define CHUNK_SIZE  1024
#define DATA_SIZE   (CHUNK_SIZE * 5 / 2)



#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in %s:%d\n", #cmd, __FILENAME__, __LINE__); \
        exit(1); \
    }



int main()
{
    MXFMemoryFile *mxfMemFile;
    MXFFile *mxfFile;
    unsigned char *data;
    int64_t numChunks;

    data = malloc(DATA_SIZE);
    memset(data, 122, DATA_SIZE);


    /* new */

    CHECK(mxf_mem_file_open_new(CHUNK_SIZE, 0, &mxfMemFile));
    mxfFile = mxf_mem_file_get_file(mxfMemFile);

    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);

    numChunks = mxf_mem_file_get_num_chunks(mxfMemFile);
    CHECK(numChunks == (DATA_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE);
    CHECK(mxf_mem_file_get_chunk_size(mxfMemFile, 0) == CHUNK_SIZE);
    CHECK(mxf_mem_file_get_chunk_data(mxfMemFile, 0)[CHUNK_SIZE - 1] == 122);

    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(!mxf_file_eof(mxfFile));
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == 0);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_seek(mxfFile, -DATA_SIZE / 2, SEEK_CUR));
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE / 2);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_getc(mxfFile) == EOF);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE * 2, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_getc(mxfFile) == EOF);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE, SEEK_SET));
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == 0);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE * 4, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 4);
    CHECK(mxf_file_getc(mxfFile) == EOF);
    CHECK(mxf_file_size(mxfFile) == 2 * DATA_SIZE);
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE * 5);
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE / 3) == DATA_SIZE / 3);
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE * 5 + DATA_SIZE / 3);
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE * 6 + DATA_SIZE / 3);

    mxf_file_close(&mxfFile);


    /* read */

    CHECK(mxf_mem_file_open_read(data, DATA_SIZE, 0, &mxfMemFile));
    mxfFile = mxf_mem_file_get_file(mxfMemFile);

    numChunks = mxf_mem_file_get_num_chunks(mxfMemFile);
    CHECK(numChunks == 1);
    CHECK(mxf_mem_file_get_chunk_size(mxfMemFile, 0) == DATA_SIZE);
    CHECK(mxf_mem_file_get_chunk_data(mxfMemFile, 0)[DATA_SIZE - 1] == 122);

    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(!mxf_file_eof(mxfFile));
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == 0);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);

    mxf_file_close(&mxfFile);


    free(data);

    return 0;
}

