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
#include <mxf/mxf_cache_file.h>


#define DATA_SIZE   10000
#define PAGE_SIZE   8192



#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in %s:%d\n", #cmd, __FILENAME__, __LINE__); \
        exit(1); \
    }



static void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s filename\n", cmd);
}

int main(int argc, const char *argv[])
{
    MXFFile *target;
    MXFCacheFile *cacheFile;
    MXFFile *mxfFile;
    unsigned char *writeData;
    unsigned char *readData;

    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }

    writeData = malloc(DATA_SIZE);
    memset(writeData, 122, DATA_SIZE);
    writeData[0] = 0x01;
    writeData[DATA_SIZE / 2] = 0x02;
    readData = malloc(DATA_SIZE);


    CHECK(mxf_disk_file_open_new(argv[1], &target));
    CHECK(mxf_cache_file_open(target, PAGE_SIZE, 4 * DATA_SIZE, &cacheFile));
    mxfFile = mxf_cache_file_get_file(cacheFile);

    CHECK(mxf_file_write(mxfFile, writeData, DATA_SIZE) == DATA_SIZE);

    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_read(mxfFile, readData, DATA_SIZE) == DATA_SIZE);
    CHECK(!mxf_file_eof(mxfFile));
    CHECK(mxf_file_read(mxfFile, readData, DATA_SIZE) == 0);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_seek(mxfFile, -DATA_SIZE / 2, SEEK_CUR));
    CHECK(mxf_file_read(mxfFile, readData, DATA_SIZE) == DATA_SIZE / 2);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_getc(mxfFile) == EOF);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE * 2, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_getc(mxfFile) == EOF);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE, SEEK_SET));
    CHECK(mxf_file_write(mxfFile, writeData, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_read(mxfFile, readData, DATA_SIZE) == 0);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_write(mxfFile, writeData, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE * 4, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 4);
    CHECK(mxf_file_getc(mxfFile) == EOF);
    CHECK(mxf_file_size(mxfFile) == 2 * DATA_SIZE);
    CHECK(mxf_file_write(mxfFile, writeData, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_size(mxfFile) == 5 * DATA_SIZE);
    CHECK(mxf_file_write(mxfFile, writeData, DATA_SIZE / 3) == DATA_SIZE / 3);
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE * 5 + DATA_SIZE / 3);
    CHECK(mxf_file_write(mxfFile, writeData, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_size(mxfFile) == DATA_SIZE * 6 + DATA_SIZE / 3);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_write(mxfFile, writeData, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_getc(mxfFile) == 0x01);
    CHECK(mxf_file_seek(mxfFile, 4 * DATA_SIZE, SEEK_SET));
    CHECK(mxf_file_write(mxfFile, &writeData[DATA_SIZE / 2], DATA_SIZE / 2) == DATA_SIZE / 2);
    CHECK(mxf_file_seek(mxfFile, PAGE_SIZE / 2, SEEK_SET));
    CHECK(mxf_file_write(mxfFile, &writeData[DATA_SIZE / 2], DATA_SIZE / 2) == DATA_SIZE / 2);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_getc(mxfFile) == 0x01);

    mxf_file_close(&mxfFile);


    free(writeData);
    free(readData);

    return 0;
}

