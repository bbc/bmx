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

static const mxfKey someKey =
    {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};

static const mxfUL someUL =
    {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06};

static const mxfUID someUID =
    {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};

static const mxfUUID someUUID =
    {0xf1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a};

static uint8_t data[256];



int test_read(const char *filename)
{
    MXFFile *mxfFile = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    mxfLocalTag tag;
    uint8_t indata[256];
    uint8_t valueu8;
    uint16_t valueu16;
    uint32_t valueu32;
    uint64_t valueu64;
    int8_t value8;
    int16_t value16;
    int32_t value32;
    int64_t value64;
    mxfUL ul;
    mxfUID uid;
    mxfUUID uuid;
    uint32_t ablen;
    uint32_t abelen;

    if (filename == NULL)
    {
        if (!mxf_stdin_wrap_read(&mxfFile))
        {
            mxf_log_error("Failed to open stdin" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }
    else
    {
        if (!mxf_disk_file_open_read(filename, &mxfFile))
        {
            mxf_log_error("Failed to open '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
            return 0;
        }
    }

    /* TEST */
    CHK_OFAIL(mxf_file_read(mxfFile, indata, 100) == 100);
    CHK_OFAIL(memcmp(data, indata, 100) == 0);
    CHK_OFAIL(mxf_file_getc(mxfFile) == 0xff);
    CHK_OFAIL(mxf_file_getc(mxfFile) == 0xff);
    CHK_OFAIL(mxf_read_uint8(mxfFile, &valueu8));
    CHK_OFAIL(valueu8 == 0x0f);
    CHK_OFAIL(mxf_read_uint16(mxfFile, &valueu16));
    CHK_OFAIL(valueu16 == 0x0f00);
    CHK_OFAIL(mxf_read_uint32(mxfFile, &valueu32));
    CHK_OFAIL(valueu32 == 0x0f000000);
    CHK_OFAIL(mxf_read_uint64(mxfFile, &valueu64));
    CHK_OFAIL(valueu64 == 0x0f00000000000000LL);
    CHK_OFAIL(mxf_read_int8(mxfFile, &value8));
    CHK_OFAIL(value8 == -0x0f);
    CHK_OFAIL(mxf_read_int16(mxfFile, &value16));
    CHK_OFAIL(value16 == -0x0f00);
    CHK_OFAIL(mxf_read_int32(mxfFile, &value32));
    CHK_OFAIL(value32 == -0x0f000000);
    CHK_OFAIL(mxf_read_int64(mxfFile, &value64));
    CHK_OFAIL(value64 == -0x0f00000000000000LL);
    CHK_OFAIL(mxf_read_local_tag(mxfFile, &tag));
    CHK_OFAIL(tag == 0xffaa);
    CHK_OFAIL(mxf_read_k(mxfFile, &key));
    CHK_OFAIL(mxf_equals_key(&key, &someKey));
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 1 && len == 0x01);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 2 && len == 0x80);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 3 && len == 0x8000);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 4 && len == 0x800000);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 5 && len == 0x80000000);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 6 && len == 0x8000000000LL);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 7 && len == 0x800000000000LL);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 8 && len == 0x80000000000000LL);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 9 && len == 0x8000000000000000LL);
    CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_equals_key(&key, &someKey));
    CHK_OFAIL(llen == 3 && len == 0xf100);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 8 && len == 0x10);
    CHK_OFAIL(mxf_read_l(mxfFile, &llen, &len));
    CHK_OFAIL(llen == 4 && len == 0x10);
    CHK_OFAIL(mxf_read_kl(mxfFile, &key, &llen, &len));
    CHK_OFAIL(mxf_equals_key(&key, &someKey));
    CHK_OFAIL(llen == 8 && len == 0x1000);
    CHK_OFAIL(mxf_read_ul(mxfFile, &ul));
    CHK_OFAIL(mxf_equals_ul(&ul, &someUL));
    CHK_OFAIL(mxf_read_uid(mxfFile, &uid));
    CHK_OFAIL(mxf_equals_uid(&uid, &someUID));
    CHK_OFAIL(mxf_read_uuid(mxfFile, &uuid));
    CHK_OFAIL(mxf_equals_uuid(&uuid, &someUUID));
    CHK_OFAIL(mxf_read_batch_header(mxfFile, &ablen, &abelen));
    CHK_OFAIL(ablen == 2 && abelen == 16);
    CHK_OFAIL(mxf_read_array_header(mxfFile, &ablen, &abelen));
    CHK_OFAIL(ablen == 4 && abelen == 32);

    /* skip junk */
    CHK_ORET(mxf_file_seek(mxfFile, 99, SEEK_CUR));
    CHK_ORET(mxf_file_getc(mxfFile) == 0);

    mxf_file_close(&mxfFile);


    return 1;

fail:
    mxf_file_close(&mxfFile);
    return 0;
}

int do_write(MXFFile *mxfFile)
{
    CHK_ORET(mxf_file_write(mxfFile, NULL, 0) == 0);
    CHK_ORET(mxf_file_write(mxfFile, data, 0) == 0);
    CHK_ORET(mxf_file_write(mxfFile, data, 100) == 100);
    CHK_ORET(mxf_file_putc(mxfFile, 0xff));
    CHK_ORET(mxf_file_putc(mxfFile, 0xff));
    CHK_ORET(mxf_write_uint8(mxfFile, 0x0f));
    CHK_ORET(mxf_write_uint16(mxfFile, 0x0f00));
    CHK_ORET(mxf_write_uint32(mxfFile, 0x0f000000));
    CHK_ORET(mxf_write_uint64(mxfFile, 0x0f00000000000000LL));
    CHK_ORET(mxf_write_int8(mxfFile, -0x0f));
    CHK_ORET(mxf_write_int16(mxfFile, -0x0f00));
    CHK_ORET(mxf_write_int32(mxfFile, -0x0f000000));
    CHK_ORET(mxf_write_int64(mxfFile, -0x0f00000000000000LL));
    CHK_ORET(mxf_write_local_tag(mxfFile, 0xffaa));
    CHK_ORET(mxf_write_k(mxfFile, &someKey));
    CHK_ORET(mxf_write_l(mxfFile, 0x01) == 1);
    CHK_ORET(mxf_write_l(mxfFile, 0x80) == 2);
    CHK_ORET(mxf_write_l(mxfFile, 0x8000) == 3);
    CHK_ORET(mxf_write_l(mxfFile, 0x800000) == 4);
    CHK_ORET(mxf_write_l(mxfFile, 0x80000000) == 5);
    CHK_ORET(mxf_write_l(mxfFile, 0x8000000000LL) == 6);
    CHK_ORET(mxf_write_l(mxfFile, 0x800000000000LL) == 7);
    CHK_ORET(mxf_write_l(mxfFile, 0x80000000000000LL) == 8);
    CHK_ORET(mxf_write_l(mxfFile, 0x8000000000000000LL) == 9);
    CHK_ORET(mxf_write_kl(mxfFile, &someKey, 0xf100));
    CHK_ORET(mxf_write_fixed_l(mxfFile, 8, 0x10));
    CHK_ORET(mxf_write_fixed_l(mxfFile, 4, 0x10));
    CHK_ORET(mxf_write_fixed_kl(mxfFile, &someKey, 8, 0x1000));
    CHK_ORET(mxf_write_ul(mxfFile, &someUL));
    CHK_ORET(mxf_write_uid(mxfFile, &someUID));
    CHK_ORET(mxf_write_uuid(mxfFile, &someUUID));
    CHK_ORET(mxf_write_batch_header(mxfFile, 2, 16));
    CHK_ORET(mxf_write_array_header(mxfFile, 4, 32));

    return 1;
}

int test_write(const char *filename)
{
    MXFFile *mxfFile = NULL;


    if (filename == NULL)
    {
        if (!mxf_stdout_wrap_write(&mxfFile))
        {
            mxf_log_error("Failed to open stdout" LOG_LOC_FORMAT, LOG_LOC_PARAMS);
            return 0;
        }
    }
    else
    {
        if (!mxf_disk_file_open_new(filename, &mxfFile))
        {
            mxf_log_error("Failed to create '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
            return 0;
        }
    }

    /* TEST */
    CHK_OFAIL(do_write(mxfFile));

    /* write junk */
    if (filename == NULL)
    {
        CHK_ORET(mxf_file_seek(mxfFile, 99, SEEK_CUR));
        CHK_ORET(mxf_file_putc(mxfFile, 0) != EOF);
    }
    else
    {
        CHK_ORET(mxf_write_zeros(mxfFile, 100));
    }

    mxf_file_close(&mxfFile);
    return 1;

fail:
    mxf_file_close(&mxfFile);
    return 0;
}

int test_modify(const char *filename)
{
    MXFFile *mxfFile = NULL;


    if (!mxf_disk_file_open_modify(filename, &mxfFile))
    {
        mxf_log_error("Failed to open modify '%s'" LOG_LOC_FORMAT, filename, LOG_LOC_PARAMS);
        return 0;
    }

    /* TEST */

    CHK_OFAIL(do_write(mxfFile));

    mxf_file_close(&mxfFile);

    CHK_OFAIL(test_read(filename));

    return 1;

fail:
    mxf_file_close(&mxfFile);
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

    memset(data, 0xaa, 256);

    if (strcmp(argv[1], "stdin") == 0)
    {
        if (!test_read(NULL))
        {
            return 1;
        }
    }
    else if (strcmp(argv[1], "stdout") == 0)
    {
        if (!test_write(NULL))
        {
            return 1;
        }
    }
    else
    {
        if (!test_write(argv[1]) ||
            !test_read(argv[1]) ||
            !test_modify(argv[1]) ||
            !test_write(argv[1])) /* reset for next run of test_read(NULL) */
        {
            return 1;
        }
    }

    return 0;
}

