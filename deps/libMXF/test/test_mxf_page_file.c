/*
 * Copyright (C) 2007, British Broadcasting Corporation
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
#include <mxf/mxf_page_file.h>
#include <mxf/mxf_macros.h>


#define PAGE_SIZE   1024
#define DATA_SIZE   (PAGE_SIZE * 3 / 2)

static const char *g_testFile = "pagetest___%d.mxf";



#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in %s:%d\n", #cmd, __FILENAME__, __LINE__); \
        exit(1); \
    }


static void remove_test_files()
{
    char filename[4096];
    int count = 0;

    while (1)
    {
        mxf_snprintf(filename, sizeof(filename), g_testFile, count);
        if (remove(filename) != 0)
        {
            break;
        }
        count++;
    }
}

int main()
{
    MXFPageFile *mxfPageFile;
    MXFFile *mxfFile;
    uint8_t *data;
    int i;

    data = malloc(DATA_SIZE);


    remove_test_files();


    CHECK(mxf_page_file_open_new(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 0, DATA_SIZE);

    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);

    mxf_file_close(&mxfFile);



    CHECK(mxf_page_file_open_modify(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 1, DATA_SIZE);

    CHECK(mxf_file_size(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);

    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);

    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);

    mxf_file_close(&mxfFile);



    CHECK(mxf_page_file_open_read(g_testFile, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 1, DATA_SIZE);

    CHECK(mxf_file_size(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_eof(mxfFile));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE + 1, SEEK_SET));
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE - 1) == DATA_SIZE - 1);
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE - 1, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE - 1);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE * 2, SEEK_SET));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, 0, SEEK_END));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE * 2);
    CHECK(mxf_file_seek(mxfFile, -DATA_SIZE * 2, SEEK_END));
    CHECK(mxf_file_tell(mxfFile) == 0);
    CHECK(mxf_file_seek(mxfFile, DATA_SIZE - 5, SEEK_CUR));
    CHECK(mxf_file_tell(mxfFile) == DATA_SIZE - 5);
    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);

    mxf_file_close(&mxfFile);



    CHECK(mxf_page_file_open_new("pagetest_out__%d.mxf", PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);

    mxf_file_close(&mxfFile);

    CHECK(mxf_page_file_remove("pagetest_out__%d.mxf"));


    /* test forward truncate */

    CHECK(mxf_page_file_open_new(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 0, DATA_SIZE);

    for (i = 0; i < 10; i++)
    {
        CHECK(mxf_file_write(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    }

    mxf_file_close(&mxfFile);


    CHECK(mxf_page_file_open_modify(g_testFile, PAGE_SIZE, &mxfPageFile));
    mxfFile = mxf_page_file_get_file(mxfPageFile);

    memset(data, 1, DATA_SIZE);

    CHECK(mxf_page_file_forward_truncate(mxfPageFile));

    for (i = 0; i < 5; i++)
    {
        CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    }
    CHECK(mxf_page_file_forward_truncate(mxfPageFile));

    for (i = 0; i < 4; i++)
    {
        CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    }
    CHECK(mxf_page_file_forward_truncate(mxfPageFile));

    CHECK(mxf_file_read(mxfFile, data, DATA_SIZE) == DATA_SIZE);
    CHECK(mxf_page_file_forward_truncate(mxfPageFile));

    mxf_file_close(&mxfFile);

    CHECK(mxf_page_file_remove(g_testFile));


    free(data);

    return 0;
}

