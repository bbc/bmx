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

#define __STDC_FORMAT_MACROS

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#if defined(_WIN32)
#define open        _open
#define close       _close
#define ftruncate   _chsize_s
#define O_WRONLY    _O_WRONLY
#endif


static void print_usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s <length> <filename>\n", cmd);
}

int main(int argc, const char **argv)
{
    const char *filename;
    int64_t length;
    int fd;
    int res;

    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (sscanf(argv[1], "%"PRId64, &length) != 1 || length < 0) {
        print_usage(argv[0]);
        fprintf(stderr, "Invalid <length> %s\n", argv[1]);
        return 1;
    }

    filename = argv[2];

    fd = open(filename, O_WRONLY);
    if (fd == -1) {
        fprintf(stderr, "%s: failed to open file: %s\n", filename, strerror(errno));
        return 1;
    }

    res = ftruncate(fd, length);
    if (res != 0)
        fprintf(stderr, "%s: failed to truncate file: %s\n", filename, strerror(errno));

    close(fd);

    return res == 0 ? 0 : 1;
}

