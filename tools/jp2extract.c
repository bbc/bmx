/*
 * Copyright (C) 2020, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#define CHK(cmd)                                                                    \
    do {                                                                            \
        if (!(cmd)) {                                                               \
            fprintf(stderr, "'%s' check failed at line %d\n", #cmd, __LINE__);      \
            return 0;                                                               \
        }                                                                           \
    } while (0)


typedef struct
{
    FILE *input_file;
    int is_seekable;
    int eof;

    FILE *output_file;

    uint32_t box_len;
    uint32_t box_type;
    uint64_t box_ext_len;
} ParseContext;


static const uint32_t SIGNATURE_BOX = 0x6a502020;  // "jP\040\040"
static const uint32_t JP2C_BOX      = 0x6a703263;  // "jp2c"


static int init_context(ParseContext *context, const char *input_filename, const char *output_filename)
{
    memset(context, 0, sizeof(*context));

    if (strcmp(input_filename, "-") == 0) {
        context->input_file = stdin;
        context->is_seekable = 0;
    } else {
        struct stat stat_buf;

        context->input_file = fopen(input_filename, "rb");
        if (!context->input_file) {
            fprintf(stderr, "Failed to open input file '%s': %s\n", input_filename, strerror(errno));
            return 0;
        }

        if (stat(input_filename, &stat_buf) == 0) {
#if defined(_WIN32)
            context->is_seekable = ((stat_buf.st_mode & S_IFMT) == S_IFREG);
#else
            context->is_seekable = S_ISREG(stat_buf.st_mode);
#endif
        }
    }

    if (strcmp(output_filename, "-") == 0) {
        context->output_file = stdout;
    } else {
        context->output_file = fopen(output_filename, "wb");
        if (!context->output_file) {
            fprintf(stderr, "Failed to open output file '%s': %s\n", output_filename, strerror(errno));
            return 0;
        }
    }

    return 1;
}

static void deinit_context(ParseContext *context)
{
    if (context->input_file && context->input_file != stdin)
        fclose(context->input_file);

    if (context->output_file && context->output_file != stdout)
        fclose(context->output_file);
}

static size_t read_bytes(ParseContext *context, size_t count, uint8_t *bytes)
{
    size_t num_read;

    if (context->eof)
        return 0;

    num_read = fread(bytes, 1, count, context->input_file);
    if (num_read != count) {
        if (feof(context->input_file))
            context->eof = 1;
        else
            fprintf(stderr, "File read error: %s\n", strerror(errno));
    }

    return num_read;
}

static uint64_t read_skip_bytes(ParseContext *context, uint64_t count)
{
    static uint8_t buffer[8192];
    uint64_t rem_count = count;

    while (rem_count > 0) {
        size_t num_read;
        size_t read_count = sizeof(buffer);
        if ((uint64_t)read_count > rem_count)
            read_count = (size_t)rem_count;

        num_read = read_bytes(context, read_count, buffer);
        rem_count -= num_read;
        if (num_read != read_count)
            break;
    }

    return count - rem_count;
}

static int skip_bytes(ParseContext *context, uint64_t count)
{
    if (context->is_seekable) {
#if defined(_WIN32)
        if (_fseeki64(context->input_file, count, SEEK_CUR) < 0) {
#else
        if (fseeko(context->input_file, count, SEEK_CUR) < 0) {
#endif
            fprintf(stderr, "Seek error: %s\n", strerror(errno));
            return 0;
        }
    } else {
        if (read_skip_bytes(context, count) != count) {
            fprintf(stderr, "File read error for seek: %s\n", strerror(errno));
            return 0;
        }
    }

    return 1;
}

static int read_uint32(ParseContext *context, uint32_t *value)
{
    uint8_t bytes[4];
    if (read_bytes(context, sizeof(bytes), bytes) != sizeof(bytes))
        return 0;

    *value = ((uint32_t)bytes[0] << 24) |
             ((uint32_t)bytes[1] << 16) |
             ((uint32_t)bytes[2] << 8)  |
             bytes[3];

    return 1;
}

static int read_uint64(ParseContext *context, uint64_t *value)
{
    uint8_t bytes[8];
    if (read_bytes(context, sizeof(bytes), bytes) != sizeof(bytes))
        return 0;

    *value = ((uint64_t)bytes[0] << 56) |
             ((uint64_t)bytes[1] << 48) |
             ((uint64_t)bytes[2] << 40) |
             ((uint64_t)bytes[3] << 32) |
             ((uint64_t)bytes[4] << 24) |
             ((uint64_t)bytes[5] << 16) |
             ((uint64_t)bytes[6] << 8)  |
             bytes[7];

    return 1;
}

static size_t write_bytes(ParseContext *context, size_t count, uint8_t *bytes)
{
    size_t num_write = fwrite(bytes, 1, count, context->output_file);
    if (num_write != count)
        fprintf(stderr, "File write error: %s\n", strerror(errno));

    return num_write;
}

static int write_uint32(ParseContext *context, uint32_t value)
{
    uint8_t bytes[4];

    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)(value);

    if (write_bytes(context, sizeof(bytes), bytes) != sizeof(bytes))
        return 0;

    return 1;
}

static int write_uint64(ParseContext *context, uint64_t value)
{
    uint8_t bytes[8];

    bytes[0] = (uint8_t)(value >> 56);
    bytes[1] = (uint8_t)(value >> 48);
    bytes[2] = (uint8_t)(value >> 40);
    bytes[3] = (uint8_t)(value >> 32);
    bytes[4] = (uint8_t)(value >> 24);
    bytes[5] = (uint8_t)(value >> 16);
    bytes[6] = (uint8_t)(value >> 8);
    bytes[7] = (uint8_t)(value);

    if (write_bytes(context, sizeof(bytes), bytes) != sizeof(bytes))
        return 0;

    return 1;
}

static int parse_box_header(ParseContext *context, uint32_t *box_type)
{
    context->box_len = 0;
    context->box_type = 0;
    context->box_ext_len = 0;

    if (!read_uint32(context, &context->box_len))
        return 0;

    CHK(read_uint32(context, &context->box_type));
    if (context->box_len == 1)
        CHK(read_uint64(context, &context->box_ext_len));

    *box_type = context->box_type;
    return 1;
}

static int skip_box(ParseContext *context)
{
    uint64_t skip_len = 0;

    if (context->box_len == 0) {
        // box extends to the end of the file
        context->eof = 1;
        return 1;
    } else if (context->box_len == 1) {
        skip_len = context->box_ext_len - 16;
    } else if (context->box_len < 8) {
        fprintf(stderr, "Invalid box len %u\n", context->box_len);
        return 0;
    } else {
        skip_len = context->box_len - 8;
    }

    if (skip_len > 0)
        CHK(skip_bytes(context, skip_len));

    return 1;
}

static int extract_box(ParseContext *context, int with_header)
{
    static uint8_t buffer[8192];
    uint64_t rem_extract_len = 0;
    int extract_to_end = 0;

    if (context->box_len == 0) {
        // box extends to the end of the file
        extract_to_end = 1;
    } else if (context->box_len == 1) {
        rem_extract_len = context->box_ext_len - 16;
    } else if (context->box_len < 8) {
        fprintf(stderr, "Invalid box len %u\n", context->box_len);
        return 0;
    } else {
        rem_extract_len = context->box_len - 8;
    }

    if (with_header) {
        CHK(write_uint32(context, context->box_len));
        CHK(write_uint32(context, context->box_type));
        if (context->box_len == 1)
            CHK(write_uint64(context, context->box_ext_len));
    }

    while (rem_extract_len > 0 || extract_to_end) {
        size_t num_read = sizeof(buffer);
        size_t actual_read;

        if (!extract_to_end && (uint64_t)num_read > rem_extract_len)
            num_read = (size_t)rem_extract_len;

        actual_read = read_bytes(context, num_read, buffer);
        if (actual_read > 0) {
            if (fwrite(buffer, 1, actual_read, context->output_file) != actual_read) {
                fprintf(stderr, "Failed to write: %s\n", strerror(errno));
                return 0;
            }
        }
        if (actual_read != num_read) {
            if (extract_to_end && context->eof)
                break;
            else
                return 0;
        }

        if (!extract_to_end)
            rem_extract_len -= actual_read;
    }

    return 1;
}

static void print_usage(const char *cmd)
{
    fprintf(stderr, "Extract J2C codestreams from 'jp2c' boxes from a JP2 file (ISO/IEC 15444-1 Annex I)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [options] <input filename> <output filename>\n", cmd);
    fprintf(stderr, "  Indicate standard input or output using '-'\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h | --help      Show help and exit\n");
    fprintf(stderr, "  --with-header    Include the input file's 'jp2c' box header in the output\n");
    fprintf(stderr, "  --start <n>      Start extracting from the <n>th 'jp2c' box\n");
    fprintf(stderr, "  --count <n>      Extract at most <n> 'jp2c' boxes\n");
}

int main(int argc, const char **argv)
{
    const char *input_filename;
    const char *output_filename;
    int with_header = 0;
    int64_t start = 0;
    int64_t count = -1;
    int64_t end = -1;
    int cmdln_index;
    ParseContext context;
    uint32_t box_type;
    int first_box = 1;
    int64_t jp2c_count = 0;
    int result = 0;

    if (argc <= 1) {
        print_usage(argv[0]);
        return 0;
    }

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++) {
        if (strcmp(argv[cmdln_index], "-h") == 0 ||
            strcmp(argv[cmdln_index], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdln_index], "--with-header") == 0)
        {
            with_header = 1;
        }
        else if (strcmp(argv[cmdln_index], "--start") == 0)
        {
            if (cmdln_index + 1 >= argc) {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%" PRId64, &start) != 1) {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid argument '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--count") == 0)
        {
            if (cmdln_index + 1 >= argc) {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%" PRId64, &count) != 1) {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid argument '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else
        {
            break;
        }
    }

    if (cmdln_index + 2 < argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Unknown option '%s'\n", argv[cmdln_index]);
        return 1;
    }
    if (cmdln_index >= argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Missing <input filename> and <output filename>\n");
        return 1;
    }
    if (cmdln_index + 1 >= argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Missing <output filename>\n");
        return 1;
    }

    input_filename = argv[cmdln_index];
    output_filename = argv[cmdln_index + 1];
    if (count >= 0)
        end = start + count;


    if (!init_context(&context, input_filename, output_filename))
        return 1;

    while ((end < 0 || jp2c_count < end) && parse_box_header(&context, &box_type)) {
        if (first_box) {
            if (box_type != SIGNATURE_BOX) {
                fprintf(stderr, "Warning: first box type, 0x%04x, is not a JPEG 2000 file signature, 0x%04x\n",
                        box_type, SIGNATURE_BOX);
            }
            first_box = 0;
        }

        if (box_type == JP2C_BOX && jp2c_count >= start) {
            if (!extract_box(&context, with_header)) {
                result = 1;
                break;
            }
        } else if (!skip_box(&context)) {
            result = 1;
            break;
        }

        if (box_type == JP2C_BOX)
            jp2c_count++;
    }

    deinit_context(&context);

    return result;
}
