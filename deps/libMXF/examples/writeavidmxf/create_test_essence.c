/*
 * Copyright (C) 2022, British Broadcasting Corporation
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
#include <stdio.h>
#include <string.h>
#include <errno.h>


static void write_buffer(FILE *file, const unsigned char *buffer, size_t size)
{
    if (fwrite(buffer, 1, size, file) != size) {
        fprintf(stderr, "Failed to write data: %s\n", strerror(errno));
        exit(1);
    }
}

static void write_zeros(FILE *file, size_t size)
{
    static const unsigned char zeros[256] = {0};

    size_t whole_count   = size / sizeof(zeros);
    size_t partial_count = size % sizeof(zeros);

    size_t i;
    for (i = 0; i < whole_count; i++)
        write_buffer(file, zeros, sizeof(zeros));

    if (partial_count > 0)
        write_buffer(file, zeros, partial_count);
}

static void print_usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s <size> <filename>\n", cmd);
}

int main(int argc, const char **argv)
{
    const char *filename;
    unsigned int size;
    FILE *file;

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc != 3) {
        print_usage(argv[0]);
        fprintf(stderr, "Invalid argument count\n");
        return 1;
    }

    size = atoi(argv[1]);
    filename = argv[2];

    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, strerror(errno));
        return 1;
    }

    if (size > 0)
        write_zeros(file, size);

    fclose(file);

    return 0;
}
