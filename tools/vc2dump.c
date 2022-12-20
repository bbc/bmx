/*
 * Copyright (C) 2015, British Broadcasting Corporation
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>


#define PRINT_BOOL(name)           printf("%*c " name ": %s\n", state.indent * 4, ' ', (state.value ? "true": "false"))
#define PRINT_UINT(name)           printf("%*c " name ": %"PRIu64"\n", state.indent * 4, ' ', state.value)
#define PRINT_UINT8_HEX(name)      printf("%*c " name ": 0x%02x\n", state.indent * 4, ' ', (uint8_t)(state.value))
#define PRINT_UINT16_HEX(name)     printf("%*c " name ": 0x%04x\n", state.indent * 4, ' ', (uint16_t)(state.value))
#define PRINT_UINT32_HEX(name)     printf("%*c " name ": 0x%08x\n", state.indent * 4, ' ', (uint32_t)(state.value))
#define PRINT_UINT64_HEX(name)     printf("%*c " name ": 0x%016"PRIx64"\n", state.indent * 4, ' ', state.value)
#define PRINT_INT(name)            printf("%*c " name ": %"PRId64"\n", state.indent * 4, ' ', state.svalue)

#define PRINT_UINT_V(name, val)    printf("%*c " name ": %"PRIu64"\n", state.indent * 4, ' ', (uint64_t)val)

#define ASSIGN(name)    name = state.value
#define SASSIGN(name)   name = state.svalue

#define ARRAY_SIZE(array)   (sizeof(array) / sizeof((array)[0]))

#define CHK(cmd)                                                                    \
    do {                                                                            \
        if (!(cmd)) {                                                               \
            fprintf(stderr, "'%s' check failed at line %d\n", #cmd, __LINE__);      \
            return 0;                                                               \
        }                                                                           \
    } while (0)

#define PARSE_INFO_SIZE     13
#define PARSE_INFO_PREFIX   0x42424344
#define MAX_SEARCH_COUNT    100000000   // 100MB

#define DEFAULT_SHOW_PICT   16
#define DEFAULT_SHOW_AUX    16
#define DEFAULT_SHOW_PAD    16


typedef struct
{
    int show_pict;
    int show_aux;
    int show_pad;

    int indent;
    uint64_t value;
    int64_t svalue;

    FILE *file;
    int64_t next_read_pos;
    int file_err;
    int eof;

    uint8_t *read_buffer;
    size_t read_buffer_alloc_size;
    size_t read_buffer_size;
    size_t read_buffer_pos;

    uint8_t current_byte;
    int next_bit;

    // parse info
    int64_t parse_info_offset;
    uint8_t parse_code;
    uint32_t next_parse_offset;
    uint32_t prev_parse_offset;

    // sequence header
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t profile;
    uint8_t level;
    uint64_t luma_width;
    uint64_t luma_height;
    uint64_t color_diff_width;
    uint64_t color_diff_height;
    uint64_t luma_depth;
    uint64_t color_diff_depth;

    // picture header
    uint32_t picture_number;
    uint8_t wavelet_index;
    uint64_t dwt_depth;
} State;

typedef struct
{
    uint64_t frame_width;
    uint64_t frame_height;
    uint64_t color_diff_format_index;
    uint64_t source_sampling;
    uint64_t top_field_first;
    uint64_t frame_rate_numer;
    uint64_t frame_rate_denom;
    uint64_t pixel_aspect_ratio_numer;
    uint64_t pixel_aspect_ratio_denom;
    uint64_t clean_width;
    uint64_t clean_height;
    uint64_t left_offset;
    uint64_t top_offset;
    uint64_t luma_offset;
    uint64_t luma_excursion;
    uint64_t color_diff_offset;
    uint64_t color_diff_excursion;
    uint64_t color_primaries;
    uint64_t color_matrix;
    uint64_t transfer_function;
} VideoParameters;

typedef struct
{
    uint32_t numer;
    uint32_t denom;
} Rational;

typedef struct
{
    uint64_t luma_offset;
    uint64_t luma_excursion;
    uint64_t color_diff_offset;
    uint64_t color_diff_excursion;
} SignalRange;

typedef struct
{
    uint64_t color_primaries;
    uint64_t color_matrix;
    uint64_t transfer_function;
} ColorSpec;


static const Rational PRESET_FRAME_RATE[] =
{
    {24000, 1001},
    {   24,    1},
    {   25,    1},
    {30000, 1001},
    {   30,    1},
    {   50,    1},
    {60000, 1001},
    {   60,    1},
    {15000, 1001},
    {   25,    2},
    {   48,    1},
};

static const Rational PRESET_PIXEL_ASPECT_RATIO[] =
{
    { 1,  1},
    {10, 11},
    {12, 11},
    {40, 33},
    {16, 11},
    { 4,  3},
};

static const SignalRange PRESET_SIGNAL_RANGE[] =
{
    {  0,  255,  128,  255},
    { 16,  219,  128,  224},
    { 64,  876,  512,  896},
    {256, 3504, 2048, 3584},
};

static const ColorSpec PRESET_COLOR_SPEC[] =
{
    {0, 0, 0},
    {1, 1, 0},
    {2, 1, 0},
    {0, 0, 0},
    {3, 2, 3},
};

static const VideoParameters DEFAULT_SOURCE_PARAMETERS[] =
{
    { 640,  480, 2, 0, 0, 24000, 1001,  1,  1,  640,  480, 0, 0,   0,  255,  128,  255, 0, 0, 0}, // Custom Format
    { 176,  120, 2, 0, 0, 15000, 1001, 10, 11,  176,  120, 0, 0,   0,  255,  128,  255, 1, 1, 0}, // QSIF525
    { 176,  144, 2, 0, 1,    25,    2, 12, 11,  176,  144, 0, 0,   0,  255,  128,  255, 2, 1, 0}, // QCIF
    { 352,  240, 2, 0, 0, 15000, 1001, 10, 11,  352,  240, 0, 0,   0,  255,  128,  255, 1, 1, 0}, // SIF525
    { 352,  288, 2, 0, 1,    25,    2, 12, 11,  352,  288, 0, 0,   0,  255,  128,  255, 2, 1, 0}, // CIF
    { 704,  480, 2, 0, 0, 15000, 1001, 10, 11,  704,  480, 0, 0,   0,  255,  128,  255, 1, 1, 0}, // 4SIF525
    { 704,  576, 2, 0, 1,    25,    2, 12, 11,  704,  576, 0, 0,   0,  255,  128,  255, 2, 1, 0}, // CIF
    { 720,  480, 1, 1, 0, 30000, 1001, 10, 11,  704,  480, 8, 0,  64,  876,  512,  896, 1, 1, 0}, // SD480I-60
    // note: the pixel aspect ratio in ST 2042-1,Table C1.a index is correct but the values, 10/11 are not
    { 720,  576, 1, 1, 1,    25,    1, 12, 11,  704,  576, 8, 0,  64,  876,  512,  896, 2, 1, 0}, // SD576I-50
    {1280,  720, 1, 0, 1, 60000, 1001,  1,  1, 1280,  720, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD720P-60
    {1280,  720, 1, 0, 1,    50,    1,  1,  1, 1280,  720, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD720P-50
    {1920, 1080, 1, 1, 1, 30000, 1001,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080I-60
    {1920, 1080, 1, 1, 1,    25,    1,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080I-50
    {1920, 1080, 1, 0, 1, 60000, 1001,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080P-60
    {1920, 1080, 1, 0, 1,    50,    1,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080P-50
    {2048, 1080, 0, 0, 1,    24,    1,  1,  1, 2048, 1080, 0, 0, 256, 3504, 2048, 3584, 3, 3, 3}, // DC2K
    {4096, 2160, 0, 0, 1,    24,    1,  1,  1, 4096, 2160, 0, 0, 256, 3504, 2048, 3584, 3, 3, 3}, // DC4K
    {3840, 2160, 1, 0, 1, 60000, 1001,  1,  1, 3840, 2160, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 4K-60
    {3840, 2160, 1, 0, 1,    50,    1,  1,  1, 3840, 2160, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 4K-50
    {7680, 4320, 1, 0, 1, 60000, 1001,  1,  1, 7680, 4320, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 8K-60
    {7680, 4320, 1, 0, 1,    50,    1,  1,  1, 7680, 4320, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // UHDTV 8K-50
    {1920, 1080, 1, 0, 1, 24000, 1001,  1,  1, 1920, 1080, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // HD1080P-24
    { 720,  486, 1, 1, 0, 30000, 1001, 10, 11,  720,  486, 0, 0,  64,  876,  512,  896, 0, 0, 0}, // SD Pro486
};

static State state;
static VideoParameters video_parameters;



static void init_state()
{
    memset(&state, 0, sizeof(state));
    state.show_pict = DEFAULT_SHOW_PICT;
    state.show_aux = DEFAULT_SHOW_AUX;
    state.show_pad = DEFAULT_SHOW_PAD;
}

static void deinit_state()
{
    if (state.file != stdin)
        fclose(state.file);
    free(state.read_buffer);
}

static uint8_t intlog2(uint64_t n)
{
    uint8_t m = 63;
    uint64_t mask = UINT64_C(1) << 63;
    while (mask && !(n & mask)) {
        mask >>= 1;
        m--;
    }
    if (mask && (n & (mask - 1)))
        m++;
    return m;
}

static int64_t get_byte_aligned_pos()
{
    int64_t byte_aligned_pos = state.next_read_pos;
    if (state.next_bit == 7)
        byte_aligned_pos--;
    return byte_aligned_pos;
}

static int backup_with_buffer(const uint8_t *buffer, size_t size)
{
    size_t old_rem_size = state.read_buffer_size - state.read_buffer_pos;
    size_t input_offset = 0;

    if (size == 0)
        return 1;

    // replace state.current_byte with buffer[0] and append state.current_byte to end
    if (old_rem_size == 0 && state.next_bit == 7 && state.next_read_pos > 0)
        input_offset = 1;

    if (state.read_buffer_alloc_size <= old_rem_size + size) {
        uint8_t *new_buffer = malloc(old_rem_size + size);
        if (!new_buffer) {
            fprintf(stderr, "Memory allocation failed\n");
            return 0;
        }
        memcpy(new_buffer, &state.read_buffer[state.read_buffer_pos], old_rem_size);
        memcpy(&new_buffer[old_rem_size], &buffer[input_offset], size - input_offset);
        free(state.read_buffer);
        state.read_buffer = new_buffer;
        state.read_buffer_alloc_size = old_rem_size + size;
    } else {
        if (old_rem_size > 0)
            memmove(state.read_buffer, &state.read_buffer[state.read_buffer_pos], old_rem_size);
        memcpy(&state.read_buffer[old_rem_size], &buffer[input_offset], size - input_offset);
    }
    state.read_buffer_size = old_rem_size + size;
    state.read_buffer_pos = 0;

    if (input_offset == 1) {
        state.read_buffer[state.read_buffer_size - 1] = state.current_byte;
        state.current_byte = buffer[0];
        if (state.file_err || state.eof)
            state.next_read_pos++; // don't include state.current_byte appended to end in adjustment below
    }

    state.next_read_pos -= size;

    return 1;
}

static void read_next_byte()
{
    int c;

    if (state.read_buffer_pos < state.read_buffer_size) {
        c = state.read_buffer[state.read_buffer_pos];
        state.read_buffer_pos++;
        state.next_read_pos++;
    } else {
        if (state.file_err || state.eof)
            return;

        c = fgetc(state.file);
        if (c == EOF) {
            if (feof(state.file))
                state.eof = 1;
            else
                state.file_err = errno;
        } else {
            state.next_read_pos++;
        }
    }

    state.next_bit = 7;
    state.current_byte = (uint8_t)c;
}

#define read_byte() CHK(_read_byte())
static int _read_byte()
{
    if (state.read_buffer_pos >= state.read_buffer_size &&
        (state.file_err || state.eof))
    {
        if (state.file_err)
            fprintf(stderr, "File i/o error: %s\n", strerror(state.file_err));
        return 0;
    }

    state.value = state.current_byte;
    read_next_byte();

    return 1;
}

#define read_bit() CHK(_read_bit())
static int _read_bit()
{
    if (state.read_buffer_pos >= state.read_buffer_size &&
        (state.file_err || state.eof))
    {
        if (state.file_err)
            fprintf(stderr, "File i/o error: %s\n", strerror(state.file_err));
        return 0;
    }

    state.value = (state.current_byte >> state.next_bit) & 0x1;
    state.next_bit--;
    if (state.next_bit < 0) {
        state.next_bit = 7;
        read_next_byte();
    }

    return 1;
}

static void byte_align()
{
    if (state.next_bit != 7)
        read_next_byte();
}

#define read_bool() CHK(_read_bool())
static int _read_bool()
{
    read_bit();
    return 1;
}

#define read_nbits(n) CHK(_read_nbits(n))
static int _read_nbits(int n)
{
    uint64_t val = 0;
    int i;

    if (n > 64) {
        fprintf(stderr, "read_nbits(%d) not supported\n", n);
        return 0;
    }

    for (i = 0; i < n; i++) {
        val <<= 1;
        read_bit();
        val += state.value;
    }
    state.value = val;

    return 1;
}

#define read_uint_lit(n) CHK(_read_uint_lit(n))
static int _read_uint_lit(int n)
{
    byte_align();
    read_nbits(n * 8);
    return 1;
}

#define read_uint() CHK(_read_uint())
static int _read_uint()
{
    uint64_t val = 1;
    while (1) {
        read_bit();
        if (state.value == 1)
            break;
        val <<= 1;
        read_bit();
        if (state.value == 1)
            val += 1;
    }
    state.value = val - 1;
    return 1;
}

static int print_bits(int n)
{
    int i;

    if (n <= 0)
        return 1;

    printf("%*c bits: ", state.indent * 4, ' ');
    for (i = 0; i < n; i++) {
        read_bit();
        printf("%d", (uint8_t)state.value);
    }
    printf("\n");

    return 1;
}

static void print_bytes_line(uint32_t index, uint8_t *line, size_t num_values, size_t line_size)
{
    size_t i;

    printf("%*c 0x%04x", state.indent * 4, ' ', index);
    for (i = 0; i < num_values; i++)
        printf(" %02x", line[i]);
    for (; i < line_size; i++)
        printf("   ");
    printf("  |");
    for (i = 0; i < num_values; i++) {
        if (isprint(line[i]))
            printf("%c", line[i]);
        else
            printf(".");
    }
    printf("|\n");
}

static int print_bytes(uint32_t size)
{
    uint8_t line[16];
    size_t line_size = 0;
    uint32_t i;
    for (i = 0; i < size; i++) {
        read_byte();
        line[line_size++] = (uint8_t)state.value;
        if (line_size == sizeof(line)) {
            print_bytes_line((uint32_t)(i + 1 - line_size), line, line_size, sizeof(line));
            line_size = 0;
        }
    }
    if (line_size > 0)
        print_bytes_line((uint32_t)(i - line_size), line, line_size, sizeof(line));

    return 1;
}

#define print_data_unit_bytes(size) CHK(_print_data_unit_bytes(size))
static int _print_data_unit_bytes(int show_size)
{
    int64_t byte_aligned_pos = get_byte_aligned_pos();
    int rem_bits = (state.next_bit + 1) % 8;
    uint32_t rem_bytes = (uint32_t)(state.next_parse_offset - (byte_aligned_pos - state.parse_info_offset));
    uint32_t i;

    printf("%*c remainder:\n", state.indent * 4, ' ');
    state.indent++;

    if (show_size > 0) {
        uint32_t show_bytes = (uint32_t)show_size;
        if (rem_bits > 0) {
            CHK(print_bits(rem_bits));
            rem_bits = 0;
        }
        if (show_bytes > rem_bytes)
            show_bytes = rem_bytes;
        CHK(print_bytes(show_bytes));
        rem_bytes -= show_bytes;
    }
    if (rem_bits > 0) {
        byte_align();
        printf("%*c ...skipped %d bits\n", state.indent * 4, ' ', rem_bits);
    }
    for (i = 0; i < rem_bytes; i++)
        read_byte();
    if (rem_bytes > 0)
        printf("%*c ...skipped %u bytes\n", state.indent * 4, ' ', rem_bytes);

    state.indent--;

    return 1;
}

// TODO: support printing bytes whilst searching
static int search_next_parse_info()
{
    uint32_t parse_info_prefix = 0;
    uint8_t parse_info_buffer[PARSE_INFO_SIZE];
    int64_t search_count = 0;

    printf("%*c [searching next parse info...]\n", state.indent * 4, ' ');

    // search for the next parse info that has a previous info offset pointing to the current parse info

    byte_align();
    while (1) {
        int i;
        uint32_t prev_parse_offset = 0;

        while (parse_info_prefix != PARSE_INFO_PREFIX &&
               search_count < MAX_SEARCH_COUNT)
        {
            if ((parse_info_prefix & 0x00ffffff) == (PARSE_INFO_PREFIX >> 8)) {
              if (!_read_byte())
                  break;
              parse_info_prefix = (parse_info_prefix << 8) | (uint32_t)state.value;
              search_count++;
            } else if ((parse_info_prefix & 0x0000ffff) == (PARSE_INFO_PREFIX >> 16)) {
              if (!_read_uint_lit(2))
                  break;
              parse_info_prefix = (parse_info_prefix << 16) | (uint32_t)state.value;
              search_count += 2;
            } else if ((parse_info_prefix & 0x000000ff) == (PARSE_INFO_PREFIX >> 24)) {
              if (!_read_uint_lit(3))
                  break;
              parse_info_prefix = (parse_info_prefix << 24) | (uint32_t)state.value;
              search_count += 3;
            } else {
              if (!_read_uint_lit(4))
                  break;
              parse_info_prefix = (uint32_t)state.value;
              search_count += 4;
            }
        }
        if (parse_info_prefix != PARSE_INFO_PREFIX)
            break;
        parse_info_buffer[0] = (uint8_t)(parse_info_prefix >> 24);
        parse_info_buffer[1] = (uint8_t)(parse_info_prefix >> 16);
        parse_info_buffer[2] = (uint8_t)(parse_info_prefix >> 8);
        parse_info_buffer[3] = (uint8_t)(parse_info_prefix);
        for (i = 4; i < PARSE_INFO_SIZE; i++) {
            if (!_read_byte())
                break;
            parse_info_buffer[i] = (uint8_t)state.value;
            if (i >= PARSE_INFO_SIZE - 4)
                prev_parse_offset |= (uint32_t)(state.value << ((PARSE_INFO_SIZE - 1 - i) * 8));
        }
        if (i < PARSE_INFO_SIZE) {
            parse_info_prefix = 0;
            break;
        } else if (get_byte_aligned_pos() - PARSE_INFO_SIZE - prev_parse_offset == state.parse_info_offset) {
            CHK(backup_with_buffer(parse_info_buffer, PARSE_INFO_SIZE));
            break;
        }
        CHK(backup_with_buffer(&parse_info_buffer[4], PARSE_INFO_SIZE - 4));
        parse_info_prefix = 0;
    }

    return parse_info_prefix == PARSE_INFO_PREFIX;
}

static void print_top_header(const char *name)
{
    if (strcmp(name, "parse_info") == 0) {
        printf("%s: pos=%"PRId64", size=%d\n", name, state.parse_info_offset, PARSE_INFO_SIZE);
    } else {
        printf("%s: pos=%"PRId64, name, state.parse_info_offset + PARSE_INFO_SIZE);
        if (state.next_parse_offset > 0)
            printf(", size=%"PRId64"\n", (int64_t)state.next_parse_offset - PARSE_INFO_SIZE);
        else
            printf(", size=?\n");
    }
}

static void print_parse_code()
{
    uint8_t parse_code = (uint8_t)state.value;
    printf("%*c parse_code: 0x%02x ", state.indent * 4, ' ', parse_code);
    if (parse_code == 0x00)
        printf("(Sequence Header)\n");
    else if (parse_code == 0x10)
        printf("(End Of Sequence)\n");
    else if (parse_code == 0x20)
        printf("(Auxiliary Data)\n");
    else if (parse_code == 0x30)
        printf("(Padding Data)\n");
    else if (parse_code == 0x08)
        printf("(Picture, Arithmetic Coding)\n");
    else if (parse_code == 0x48)
        printf("(Picture, No Arithmetic Coding)\n");
    else if (parse_code == 0xc8)
        printf("(Low Delay Picture)\n");
    else if (parse_code == 0xe8)
        printf("(High Quality Picture)\n");
    else if ((parse_code & 0x08) == 0x08)
        printf("(Picture)\n");
    else if ((parse_code & 0xf8) == 0x20)
        printf("(Auxiliary Data, unexpected)\n");
    else
        printf("(unknown)\n");
}

static void print_enum(const char *name, const char **strings, size_t size)
{
    if (state.value < size)
        printf("%*c %s: 0x%02x (%s)\n", state.indent * 4, ' ', name, (uint8_t)state.value, strings[state.value]);
    else
        printf("%*c %s: %"PRIu64" (invalid)\n", state.indent * 4, ' ', name, state.value);
}

static void print_profile()
{
    static const char *PROFILE_STRINGS[] =
    {
        "Low Delay Profile",
        "Simple Profile",
        "Main Profile",
        "High Quality Profile",
    };

    if (state.value < ARRAY_SIZE(PROFILE_STRINGS))
        printf("%*c profile: %u (%s)\n", state.indent * 4, ' ', (uint8_t)state.value, PROFILE_STRINGS[state.value]);
    else
        printf("%*c profile: %"PRIu64" (unknown)\n", state.indent * 4, ' ', state.value);
}

static void print_level()
{
    if (state.value == 0)
        printf("%*c level: 0 (Reserved)\n", state.indent * 4, ' ');
    else if (state.value < 8)
        printf("%*c level: %u (A Default Level)\n", state.indent * 4, ' ', (uint8_t)state.value);
    else if (state.value < 64)
        printf("%*c level: %u (A Generalized Level)\n", state.indent * 4, ' ', (uint8_t)state.value);
    else if (state.value == 64)
        printf("%*c level: 64 (Mezzanine Level Compression of 1080P HD, RP 2047-1)\n", state.indent * 4, ' ');
    else if (state.value == 65)
        printf("%*c level: 65 (Level 65 Compression of HD over SD, RP 2047-3)\n", state.indent * 4, ' ');
    else
        printf("%*c profile: %"PRIu64" (A Specialized Level)\n", state.indent * 4, ' ', state.value);
}

static void print_base_video_format()
{
    static const char *BASE_VIDEO_FORMAT_STRINGS[] =
    {
        "Custom Format",
        "QSIF525",
        "QCIF",
        "SIF525",
        "CIF",
        "4SIF525",
        "4CIF",
        "SD 480I-60",
        "SD 576I-50",
        "HD 720P-60",
        "HD 720P-50",
        "HD 1080I-60",
        "HD 1080I-50",
        "HD 1080P-60",
        "HD 1080P-50",
        "DC 2K-24",
        "DC 4K-24",
        "UHDTV 4K-60",
        "UHDTV 4K-50",
        "UHDTV 8K-60",
        "UHDTV 8K-50",
        "HD 1080P-24",
        "SD Pro486",
    };

    print_enum("base_video_format", BASE_VIDEO_FORMAT_STRINGS, ARRAY_SIZE(BASE_VIDEO_FORMAT_STRINGS));
}

static void print_color_diff_format()
{
    static const char *COLOR_DIFF_FORMAT_STRINGS[] =
    {
        "4:4:4",
        "4:2:2",
        "4:2:0",
    };

    print_enum("color_diff_format_index", COLOR_DIFF_FORMAT_STRINGS, ARRAY_SIZE(COLOR_DIFF_FORMAT_STRINGS));
}

static void print_source_sampling()
{
    static const char *SOURCE_SAMPLING_STRINGS[] =
    {
        "Progressive",
        "Interlaced",
    };

    print_enum("source_sampling", SOURCE_SAMPLING_STRINGS, ARRAY_SIZE(SOURCE_SAMPLING_STRINGS));
}

static void print_picture_coding_mode()
{
    static const char *PICTURE_CODING_MODE_STRINGS[] =
    {
        "Frames",
        "Fields",
    };

    print_enum("picture_coding_mode", PICTURE_CODING_MODE_STRINGS, ARRAY_SIZE(PICTURE_CODING_MODE_STRINGS));
}

static void print_color_primaries()
{
    static const char *COLOR_PRIMARIES_STRINGS[] =
    {
        "HDTV",
        "SDTV 525",
        "SDTV 625",
        "D-Cinema",
    };

    print_enum("color_primaries", COLOR_PRIMARIES_STRINGS, ARRAY_SIZE(COLOR_PRIMARIES_STRINGS));
}

static void print_color_matrix()
{
    static const char *COLOR_MATRIX_STRINGS[] =
    {
        "HDTV",
        "SDTV",
        "Reversible",
        "RGB",
    };

    print_enum("color_matrix", COLOR_MATRIX_STRINGS, ARRAY_SIZE(COLOR_MATRIX_STRINGS));
}

static void print_transfer_function()
{
    static const char *TRANSFER_FUNCTION_STRINGS[] =
    {
        "TV Gamma",
        "Extended Gamut",
        "Linear",
        "D-Cinema Transfer Function",
    };

    print_enum("transfer_function", TRANSFER_FUNCTION_STRINGS, ARRAY_SIZE(TRANSFER_FUNCTION_STRINGS));
}

static void print_wavelet_filter()
{
    static const char *WAVELET_FILTER_STRINGS[] =
    {
        "Deslauriers-Dubuc (9,7)",
        "LeGall (5,3)",
        "Deslauriers-Dubuc (13,7)",
        "Haar with no shift",
        "Haar with single shift per level",
        "Fidelity filter",
        "Daubechies (9,7) integer approximation",
    };

    print_enum("wavelet_index", WAVELET_FILTER_STRINGS, ARRAY_SIZE(WAVELET_FILTER_STRINGS));
}

static void print_codeblock_mode()
{
    static const char *CODEBLOCK_MODE_STRINGS[] =
    {
        "Single quantizer",
        "Multiple quantizers ",
    };

    print_enum("codeblock_mode", CODEBLOCK_MODE_STRINGS, ARRAY_SIZE(CODEBLOCK_MODE_STRINGS));
}

static int is_end_of_sequence()
{
    return state.parse_code == 0x10;
}

static int is_seq_header()
{
    return state.parse_code == 0x00;
}

static int is_picture()
{
    return (state.parse_code & 0x08) == 0x08;
}

static int is_low_delay()
{
    return (state.parse_code & 0x88) == 0x88;
}

static int is_ld_picture()
{
    return (state.parse_code & 0xf8) == 0xc8;
}

static int is_hq_picture()
{
    return (state.parse_code & 0xf8) == 0xe8;
}

static int is_auxiliary_data()
{
    return (state.parse_code & 0xf8) == 0x20;
}

static int is_padding()
{
    return state.parse_code == 0x30;
}

static int parse_info()
{
    state.parse_info_offset = get_byte_aligned_pos();

    print_top_header("parse_info");
    state.indent++;

    read_uint_lit(4); PRINT_UINT32_HEX("parse_info_prefix");
    if (state.value != PARSE_INFO_PREFIX) {
        fprintf(stderr, "Invalid parse info prefix\n");
        return 0;
    }

    read_byte(); print_parse_code(); state.parse_code = (uint8_t)state.value;
    read_uint_lit(4); PRINT_UINT("next_parse_offset"); state.next_parse_offset = (uint32_t)state.value;
    read_uint_lit(4); PRINT_UINT("prev_parse_offset"); state.prev_parse_offset = (uint32_t)state.value;

    state.indent--;

    return 1;
}

static int parse_parameters()
{
    read_uint(); PRINT_UINT("major_version");
    read_uint(); PRINT_UINT("minor_version");
    read_uint(); print_profile();
    read_uint(); print_level();

    return 1;
}

static int source_parameters(uint64_t base_video_format)
{
    if (base_video_format >= ARRAY_SIZE(DEFAULT_SOURCE_PARAMETERS)) {
        fprintf(stderr, "Unknown base video format index\n");
        return 0;
    }
    memcpy(&video_parameters, &DEFAULT_SOURCE_PARAMETERS[base_video_format], sizeof(video_parameters));

    read_bool(); PRINT_BOOL("custom_dimensions_flags");
    if (state.value) {
        state.indent++;
        read_uint(); PRINT_UINT("frame_width"); ASSIGN(video_parameters.frame_width);
        read_uint(); PRINT_UINT("frame_height"); ASSIGN(video_parameters.frame_height);
        state.indent--;
    }

    read_bool(); PRINT_BOOL("custom_color_diff_format");
    if (state.value) {
        state.indent++;
        read_uint(); print_color_diff_format(); ASSIGN(video_parameters.color_diff_format_index);
        state.indent--;
    }

    read_bool(); PRINT_BOOL("custom_scan_format_flag");
    if (state.value) {
        state.indent++;
        read_uint(); print_source_sampling(); ASSIGN(video_parameters.source_sampling);
        state.indent--;
    }

    read_bool(); PRINT_BOOL("custom_frame_rate_flag");
    if (state.value) {
        uint64_t index;
        state.indent++;
        read_uint(); PRINT_UINT("frame_rate_index"); ASSIGN(index);
        if (index == 0) {
            read_uint(); PRINT_UINT("frame_rate_numer"); ASSIGN(video_parameters.frame_rate_numer);
            read_uint(); PRINT_UINT("frame_rate_denom"); ASSIGN(video_parameters.frame_rate_denom);
        } else {
            if (index - 1 >= ARRAY_SIZE(PRESET_FRAME_RATE)) {
                fprintf(stderr, "Unknown frame rate index\n");
                return 0;
            }
            video_parameters.frame_rate_numer = PRESET_FRAME_RATE[index - 1].numer;
            video_parameters.frame_rate_denom = PRESET_FRAME_RATE[index - 1].denom;
        }
        state.indent--;
    }

    read_bool(); PRINT_BOOL("custom_pixel_aspect_ratio_flag");
    if (state.value) {
        uint64_t index;
        state.indent++;
        read_uint(); PRINT_UINT("pixel_aspect_ratio_index"); ASSIGN(index);
        if (index == 0) {
            read_uint(); PRINT_UINT("pixel_aspect_ratio_numer"); ASSIGN(video_parameters.pixel_aspect_ratio_numer);
            read_uint(); PRINT_UINT("pixel_aspect_ratio_denom"); ASSIGN(video_parameters.pixel_aspect_ratio_denom);
        } else {
            if (index - 1 >= ARRAY_SIZE(PRESET_PIXEL_ASPECT_RATIO)) {
                fprintf(stderr, "Unknown pixel aspect ratio index\n");
                return 0;
            }
            video_parameters.pixel_aspect_ratio_numer = PRESET_PIXEL_ASPECT_RATIO[index - 1].numer;
            video_parameters.pixel_aspect_ratio_denom = PRESET_PIXEL_ASPECT_RATIO[index - 1].denom;
        }
        state.indent--;
    }

    read_bool(); PRINT_BOOL("custom_clean_area_flag");
    if (state.value) {
        state.indent++;
        read_uint(); PRINT_UINT("clean_width"); ASSIGN(video_parameters.clean_width);
        read_uint(); PRINT_UINT("clean_height"); ASSIGN(video_parameters.clean_height);
        read_uint(); PRINT_UINT("left_offset"); ASSIGN(video_parameters.left_offset);
        read_uint(); PRINT_UINT("top_offset"); ASSIGN(video_parameters.top_offset);
        state.indent--;
    }

    read_bool(); PRINT_BOOL("custom_signal_range_flag");
    if (state.value) {
        uint64_t index;
        state.indent++;
        read_uint(); PRINT_UINT("signal_range_index"); ASSIGN(index);
        if (index == 0) {
            read_uint(); PRINT_UINT("luma_offset"); ASSIGN(video_parameters.luma_offset);
            read_uint(); PRINT_UINT("luma_excursion"); ASSIGN(video_parameters.luma_excursion);
            read_uint(); PRINT_UINT("color_diff_offset"); ASSIGN(video_parameters.color_diff_offset);
            read_uint(); PRINT_UINT("color_diff_excursion"); ASSIGN(video_parameters.color_diff_excursion);
        } else {
            if (index - 1 >= ARRAY_SIZE(PRESET_SIGNAL_RANGE)) {
                fprintf(stderr, "Unknown signal range index\n");
                return 0;
            }
            video_parameters.luma_offset = PRESET_SIGNAL_RANGE[index - 1].luma_offset;
            video_parameters.luma_excursion = PRESET_SIGNAL_RANGE[index - 1].luma_excursion;
            video_parameters.color_diff_offset = PRESET_SIGNAL_RANGE[index - 1].color_diff_offset;
            video_parameters.color_diff_excursion = PRESET_SIGNAL_RANGE[index - 1].color_diff_excursion;
        }
        state.indent--;
    }

    read_bool(); PRINT_BOOL("custom_color_spec_flag");
    if (state.value) {
        uint64_t index;
        state.indent++;
        read_uint(); PRINT_UINT("color_spec_index"); ASSIGN(index);
        if (index >= ARRAY_SIZE(PRESET_COLOR_SPEC)) {
            fprintf(stderr, "Unknown color spec index\n");
            return 0;
        }
        video_parameters.color_primaries = PRESET_COLOR_SPEC[index].color_primaries;
        video_parameters.color_matrix = PRESET_COLOR_SPEC[index].color_matrix;
        video_parameters.transfer_function = PRESET_COLOR_SPEC[index].transfer_function;
        if (index == 0) {
            read_bool(); PRINT_BOOL("custom_color_primaries_flag");
            if (state.value) {
                read_uint(); print_color_primaries();
                if (state.value > 3) {
                    fprintf(stderr, "Unknown color primaries index\n");
                    return 0;
                }
                ASSIGN(video_parameters.color_primaries);
            }
            read_bool(); PRINT_BOOL("custom_color_matrix_flag");
            if (state.value) {
                read_uint(); print_color_matrix();
                if (state.value > 3) {
                    fprintf(stderr, "Unknown color matrix index\n");
                    return 0;
                }
                ASSIGN(video_parameters.color_matrix);
            }
            read_bool(); PRINT_BOOL("custom_transfer_function_flag");
            if (state.value) {
                read_uint(); print_transfer_function();
                if (state.value > 3) {
                    fprintf(stderr, "Unknown transfer function index\n");
                    return 0;
                }
                ASSIGN(video_parameters.transfer_function);
            }
        }
        state.indent--;
    }

    return 1;
}

static void set_coding_parameters(uint64_t picture_coding_mode)
{
    state.luma_width = video_parameters.frame_width;
    state.luma_height = video_parameters.frame_height;
    state.color_diff_width = state.luma_width;
    state.color_diff_height = state.luma_height;
    if (video_parameters.color_diff_format_index == 1) {
        state.color_diff_width /= 2;
    } else if (video_parameters.color_diff_format_index == 2) {
        state.color_diff_width /= 2;
        state.color_diff_height /= 2;
    }
    if (picture_coding_mode == 1) {
        state.luma_height /= 2;
        state.color_diff_height /= 2;
    }

    state.luma_depth = intlog2(video_parameters.luma_excursion + 1);
    state.color_diff_depth = intlog2(video_parameters.color_diff_excursion + 1);
}

static int sequence_header()
{
    uint64_t base_video_format;
    uint64_t picture_coding_mode;

    print_top_header("sequence_header");
    state.indent++;

    byte_align();
    CHK(parse_parameters());
    read_uint(); print_base_video_format(); ASSIGN(base_video_format);
    CHK(source_parameters(base_video_format));
    read_uint(); print_picture_coding_mode(); ASSIGN(picture_coding_mode);
    set_coding_parameters(picture_coding_mode);

    printf("%*c [source_parameters]:\n", state.indent * 4, ' ');
    state.indent++;
    PRINT_UINT_V("frame_width", video_parameters.frame_width);
    PRINT_UINT_V("frame_height", video_parameters.frame_height);
    state.value = video_parameters.color_diff_format_index; print_color_diff_format();
    state.value = video_parameters.source_sampling; print_source_sampling();
    PRINT_UINT_V("frame_rate_numer", video_parameters.frame_rate_numer);
    PRINT_UINT_V("frame_rate_denom", video_parameters.frame_rate_denom);
    PRINT_UINT_V("pixel_aspect_ratio_numer", video_parameters.pixel_aspect_ratio_numer);
    PRINT_UINT_V("pixel_aspect_ratio_denom", video_parameters.pixel_aspect_ratio_denom);
    PRINT_UINT_V("clean_width", video_parameters.clean_width);
    PRINT_UINT_V("clean_height", video_parameters.clean_height);
    PRINT_UINT_V("left_offset", video_parameters.left_offset);
    PRINT_UINT_V("top_offset", video_parameters.top_offset);
    PRINT_UINT_V("luma_offset", video_parameters.luma_offset);
    PRINT_UINT_V("luma_excursion", video_parameters.luma_excursion);
    PRINT_UINT_V("color_diff_offset", video_parameters.color_diff_offset);
    PRINT_UINT_V("color_diff_excursion", video_parameters.color_diff_excursion);
    state.value = video_parameters.color_primaries; print_color_primaries();
    state.value = video_parameters.color_matrix; print_color_matrix();
    state.value = video_parameters.transfer_function; print_transfer_function();
    state.indent--;

    printf("%*c [coding_parameters]:\n", state.indent * 4, ' ');
    state.indent++;
    PRINT_UINT_V("luma_width", state.luma_width);
    PRINT_UINT_V("luma_height", state.luma_height);
    PRINT_UINT_V("color_diff_width", state.color_diff_width);
    PRINT_UINT_V("color_diff_height", state.color_diff_height);
    PRINT_UINT_V("luma_depth", state.luma_depth);
    PRINT_UINT_V("color_diff_depth", state.color_diff_depth);
    state.indent--;

    state.indent--;

    return 1;
}

static int picture_header()
{
    read_uint_lit(4); PRINT_UINT("picture_number"); state.picture_number = (uint32_t)state.value;
    return 1;
}

static int codeblock_parameters()
{
    read_bool(); PRINT_BOOL("spatial_partition_flag");
    if (state.value) {
        uint64_t i;
        state.indent++;
        for (i = 0; i < state.dwt_depth; i++) {
            read_uint(); printf("%*c codeblocks_x[%"PRIu64"]: %"PRIu64"\n", state.indent * 4, ' ', i, state.value);
            read_uint(); printf("%*c codeblocks_y[%"PRIu64"]: %"PRIu64"\n", state.indent * 4, ' ', i, state.value);
        }
        read_uint(); print_codeblock_mode();
        if (state.value > 1) {
            fprintf(stderr, "Unknown codeblock mode index\n");
            return 0;
        }
        state.indent--;
    }

    return 1;
}

static int slice_parameters()
{
    read_uint(); PRINT_UINT("slices_x");
    read_uint(); PRINT_UINT("slices_y");
    if (is_ld_picture()) {
        read_uint(); PRINT_UINT("slice_bytes_numerator");
        read_uint(); PRINT_UINT("slice_bytes_denominator");
    }
    if (is_hq_picture()) {
        read_uint(); PRINT_UINT("slice_prefix_bytes");
        read_uint(); PRINT_UINT("slice_size_scalar");
    }

    return 1;
}

static int quant_matrix()
{
    read_bool(); PRINT_BOOL("custom_quant_matrix");
    if (state.value) {
        uint64_t i;
        state.indent++;
        read_uint(); PRINT_UINT("quant_matrix[0][LL]");
        for (i = 1; i < state.dwt_depth; i++) {
            read_uint(); printf("%*c quant_matrix[%"PRIu64"][HL]: %"PRIu64"\n", state.indent * 4, ' ', i, state.value);
            read_uint(); printf("%*c quant_matrix[%"PRIu64"][LH]: %"PRIu64"\n", state.indent * 4, ' ', i, state.value);
            read_uint(); printf("%*c quant_matrix[%"PRIu64"][HH]: %"PRIu64"\n", state.indent * 4, ' ', i, state.value);
        }
        state.indent--;
    }

    return 1;
}

static int transform_parameters()
{
    read_uint(); print_wavelet_filter();
    if (state.value > 6) {
        fprintf(stderr, "Unknown wavelet filter index\n");
        return 0;
    }
    state.wavelet_index = (uint8_t)state.value;
    read_uint(); PRINT_UINT("dwt_depth"); ASSIGN(state.dwt_depth);
    if (!is_low_delay()) {
        CHK(codeblock_parameters());
    } else {
        CHK(slice_parameters());
        CHK(quant_matrix());
    }

    return 1;
}

static int transform_data()
{
    printf("%*c transform_data:\n", state.indent * 4, ' ');

    // TODO

    state.indent++;
    if (state.next_parse_offset != 0)
        print_data_unit_bytes(state.show_pict);
    else
        search_next_parse_info();
    state.indent--;

    return 1;
}

static int wavelet_transform()
{
    CHK(transform_parameters());
    byte_align();
    CHK(transform_data());

    return 1;
}

static int picture_parse()
{
    print_top_header("picture");
    state.indent++;

    byte_align();
    CHK(picture_header());
    byte_align();
    CHK(wavelet_transform());

    state.indent--;

    return 1;
}

static int auxiliary_data()
{
    print_top_header("auxiliary_data");
    state.indent++;

    byte_align();
    if (state.next_parse_offset != 0)
        print_data_unit_bytes(state.show_aux);
    else
        search_next_parse_info();

    state.indent--;

    return 1;
}

static int padding()
{
    print_top_header("padding");
    state.indent++;

    byte_align();
    if (state.next_parse_offset != 0)
        print_data_unit_bytes(state.show_pad);
    else
        search_next_parse_info();

    state.indent--;

    return 1;
}

static int parse_vc2()
{
    while (!state.file_err && !state.eof) {
        CHK(parse_info());
        while (!is_end_of_sequence()) {
            if (is_seq_header())
                CHK(sequence_header());
            else if (is_picture())
                CHK(picture_parse());
            else if (is_auxiliary_data())
                CHK(auxiliary_data());
            else if (is_padding())
                CHK(padding());

            if (state.file_err || state.eof) {
                if (state.eof)
                    fprintf(stderr, "Missing End Of Sequence Parse Info at end of stream\n");
                break;
            }
            CHK(parse_info());
        }
    }

    if (state.file_err) {
        fprintf(stderr, "File I/O error: %s\n", strerror(state.file_err));
        return 0;
    }

    return 1;
}

static void print_usage(const char *cmd)
{
    fprintf(stderr, "Text dump SMPTE ST 2042 VC-2 bitstream files\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [options] [<filename>]\n", cmd);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h | --help      Show help and exit\n");
    fprintf(stderr, "  --show-pict <n>  Print the first <n> unparsed bytes of the Picture data. Defaults to %d\n", DEFAULT_SHOW_PICT);
    fprintf(stderr, "  --show-aux <n>   Print the first <n> bytes of the Auxiliary data. Defaults to %d\n", DEFAULT_SHOW_AUX);
    fprintf(stderr, "  --show-pad <n>   Print the first <n> bytes of the Padding data. Defaults to %d\n", DEFAULT_SHOW_PAD);
}

int main(int argc, const char **argv)
{
    const char *filename = NULL;
    int res = 0;
    int cmdln_index;

    init_state();

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++) {
        if (strcmp(argv[cmdln_index], "-h") == 0 ||
            strcmp(argv[cmdln_index], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdln_index], "--show-pict") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &state.show_pict) != 1)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--show-aux") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &state.show_aux) != 1)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--show-pad") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &state.show_pad) != 1)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else
        {
            break;
        }
    }
    if (cmdln_index < argc) {
        if (cmdln_index + 1 < argc) {
            print_usage(argv[0]);
            fprintf(stderr, "Unknown option '%s'\n", argv[cmdln_index]);
            return 1;
        }
        filename = argv[cmdln_index];
    }

    if (filename) {
        state.file = fopen(filename, "rb");
        if (!state.file) {
            fprintf(stderr, "Failed to open '%s': %s\n", filename, strerror(errno));
            return 1;
        }
    } else {
        state.file = stdin;
    }

    res = parse_vc2();

    deinit_state();

    return (res ? 0 : 1);
}
