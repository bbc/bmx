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
#include <ctype.h>


#define ARRAY_SIZE(array)   (sizeof(array) / sizeof((array)[0]))

#define CHK(cmd)                                                                    \
    do {                                                                            \
        if (!(cmd)) {                                                               \
            fprintf(stderr, "'%s' check failed at line %d\n", #cmd, __LINE__);      \
            return 0;                                                               \
        }                                                                           \
    } while (0)


typedef enum
{
    SOC = 0xff4f,
    SOT = 0xff90,
    SOD = 0xff93,
    EOC = 0xffd9,
    SIZ = 0xff51,
    COD = 0xff52,
    COC = 0xff53,
    RGN = 0xff5e,
    QCD = 0xff5c,
    QCC = 0xff5d,
    POC = 0xff5f,
    TLM = 0xff55,
    PLM = 0xff57,
    PLT = 0xff58,
    PPM = 0xff60,
    PPT = 0xff61,
    SOP = 0xff91,
    EPH = 0xff92,
    CRG = 0xff63,
    COM = 0xff64
} MarkerType;

typedef struct
{
    MarkerType marker;
    const char *name;
} MarkerNameType;

typedef struct TLMIndex
{
    struct TLMIndex *next;

    uint8_t index;
    uint16_t num_tile_parts;
    uint16_t *tile_indexes;
    uint32_t *tile_part_lengths;
} TLMIndex;

typedef struct
{
    FILE *file;
    int is_seekable;

    int64_t file_pos;
    int eof;

    uint64_t value;
    int indent;
    int64_t frame_count;

    int expect_soc;
    TLMIndex *tlm_index;
    int64_t sot_offset;
    uint32_t psot;
} ParseContext;


static int create_tile_part_index(uint8_t st, uint16_t num_tile_parts, TLMIndex **tlm_index)
{
    TLMIndex *new_tlm_index;

    new_tlm_index = malloc(sizeof(*new_tlm_index));
    if (!new_tlm_index) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 0;
    }
    memset(new_tlm_index, 0, sizeof(*new_tlm_index));
    new_tlm_index->num_tile_parts = num_tile_parts;

    if (st != 0) {
        new_tlm_index->tile_indexes = malloc(sizeof(*new_tlm_index->tile_indexes) * num_tile_parts);
        if (!new_tlm_index->tile_indexes) {
            fprintf(stderr, "Failed to allocate memory\n");
            return 0;
        }
    }
    new_tlm_index->tile_part_lengths = malloc(sizeof(*new_tlm_index->tile_part_lengths) * num_tile_parts);
    if (!new_tlm_index->tile_part_lengths) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 0;
    }

    *tlm_index = new_tlm_index;
    return 1;
}

static void free_tile_part_index(TLMIndex **head_tlm_index)
{
    if (!(*head_tlm_index))
        return;

    TLMIndex *tlm_index = *head_tlm_index;
    while (tlm_index) {
        TLMIndex *next = tlm_index->next;
        free(tlm_index->tile_indexes);
        free(tlm_index->tile_part_lengths);
        free(tlm_index);
        tlm_index = next;
    }

    *head_tlm_index = NULL;
}

static int insert_tile_part_index(ParseContext *context, TLMIndex *tlm_index)
{
    if (!context->tlm_index) {
        context->tlm_index = tlm_index;
        return 1;
    }

    TLMIndex *prev_tlm_index = NULL;
    TLMIndex *current_tlm_index = context->tlm_index;
    while (current_tlm_index) {
        if (tlm_index->index <= current_tlm_index->index) {
            // Insert before or replace
            if (prev_tlm_index)
                prev_tlm_index->next = tlm_index;
            else
                context->tlm_index = tlm_index;

            if (tlm_index->index < current_tlm_index->index) {
                // Insert before
                tlm_index->next = current_tlm_index;
            } else {
                // Replace and free the old one
                tlm_index->next = current_tlm_index->next;

                current_tlm_index->next = NULL;
                free_tile_part_index(&current_tlm_index);
                current_tlm_index = tlm_index;
            }
            break;
        }

        prev_tlm_index = current_tlm_index;
        current_tlm_index = current_tlm_index->next;
    }
    if (!current_tlm_index) {
        // Insert after
        prev_tlm_index->next = tlm_index;
    }

    return 1;
}

static int get_tile_part_index_entry(ParseContext *context, uint16_t jth_tile_part,
                                     uint16_t *tile_index_out, uint32_t *tile_part_length_out)
{
    TLMIndex *tlm_index = context->tlm_index;
    uint16_t tile_index;
    uint32_t tile_part_length;
    size_t i = 0;
    while (tlm_index) {
        if (jth_tile_part < i + tlm_index->num_tile_parts) {
            if (tlm_index->tile_indexes)
                tile_index = tlm_index->tile_indexes[jth_tile_part - i];
            else
                tile_index = jth_tile_part;
            tile_part_length = tlm_index->tile_part_lengths[jth_tile_part - i];
            break;
        }

        i += tlm_index->num_tile_parts;
        tlm_index = tlm_index->next;
    }
    if (!tlm_index)
        return 0;

    *tile_index_out = tile_index;
    *tile_part_length_out = tile_part_length;
    return 1;
}


static int init_context(ParseContext *context, const char *filename)
{
    memset(context, 0, sizeof(*context));
    context->expect_soc = 1;

    if (strcmp(filename, "-") == 0) {
        context->file = stdin;
        context->is_seekable = 0;
    } else {
        struct stat stat_buf;

        context->file = fopen(filename, "rb");
        if (!context->file) {
            fprintf(stderr, "Failed to open file '%s': %s\n", filename, strerror(errno));
            return 0;
        }

        if (stat(filename, &stat_buf) == 0) {
#if defined(_WIN32)
            context->is_seekable = ((stat_buf.st_mode & S_IFMT) == S_IFREG);
#else
            context->is_seekable = S_ISREG(stat_buf.st_mode);
#endif
        }
    }

    return 1;
}

static void deinit_context(ParseContext *context)
{
    if (context->file && context->file != stdin)
        fclose(context->file);
    free_tile_part_index(&context->tlm_index);
}

static int is_marker_only(uint16_t marker)
{
    if (marker >= 0xff30 && marker <= 0xff3f)
        return 1;

    if (marker == SOC || marker == EOC || marker == SOD || marker == EPH)
        return 1;

    return 0;
}

static int read_bytes(ParseContext *context, size_t count, uint8_t *bytes)
{
    size_t num_read = fread(bytes, 1, count, context->file);
    context->file_pos += num_read;

    if (num_read != count) {
        if (feof(context->file))
            context->eof = 1;
        else
            fprintf(stderr, "File read error: %s\n", strerror(errno));
        return 0;
    }

    return 1;
}

static int64_t read_skip_bytes(ParseContext *context, int64_t count)
{
    static uint8_t buffer[8192];
    int64_t rem_count = count;

    while (rem_count > 0) {
        size_t num_read;
        size_t read_count = sizeof(buffer);
        if ((int64_t)read_count > rem_count)
            read_count = (size_t)rem_count;

        num_read = fread(buffer, 1, read_count, context->file);
        rem_count -= num_read;
        context->file_pos += num_read;

        if (num_read != read_count)
            break;
    }

    return count - rem_count;
}

static int skip_bytes(ParseContext *context, int64_t count)
{
    if (context->is_seekable) {
#if defined(_WIN32)
        if (_fseeki64(context->file, count, SEEK_CUR) < 0) {
#else
        if (fseeko(context->file, count, SEEK_CUR) < 0) {
#endif
            fprintf(stderr, "Seek error: %s\n", strerror(errno));
            return 0;
        }

        context->file_pos += count;
    } else {
        if (read_skip_bytes(context, count) != count) {
            fprintf(stderr, "File read error for seek: %s\n", strerror(errno));
            return 0;
        }
    }

    printf("%*c(skipped=%" PRId64 ")\n", context->indent * 4, ' ', count);

    return 1;
}

static int read_uint8(ParseContext *context)
{
    uint8_t byte;
    if (!read_bytes(context, 1, &byte))
        return 0;

    context->value = byte;

    return 1;
}

static int read_uint16(ParseContext *context)
{
    uint8_t bytes[2];
    if (!read_bytes(context, sizeof(bytes), bytes))
        return 0;

    context->value = ((uint16_t)bytes[0] << 8) | bytes[1];

    return 1;
}

static int read_uint32(ParseContext *context)
{
    uint8_t bytes[4];
    if (!read_bytes(context, sizeof(bytes), bytes))
        return 0;

    context->value = ((uint32_t)bytes[0] << 24) |
                     ((uint32_t)bytes[1] << 16) |
                     ((uint32_t)bytes[2] << 8)  |
                     bytes[3];

    return 1;
}

static const char* get_marker_name(uint16_t marker)
{
    static const char *unknown_name = "???";
    static const MarkerNameType marker_names[] = {
        {SOC, "SOC"},
        {SOT, "SOT"},
        {SOD, "SOD"},
        {EOC, "EOC"},
        {SIZ, "SIZ"},
        {COD, "COD"},
        {COC, "COC"},
        {RGN, "RGN"},
        {QCD, "QCD"},
        {QCC, "QCC"},
        {POC, "POC"},
        {TLM, "TLM"},
        {PLM, "PLM"},
        {PLT, "PLT"},
        {PPM, "PPM"},
        {PPT, "PPT"},
        {SOP, "SOP"},
        {EPH, "EPH"},
        {CRG, "CRG"},
        {COM, "COM"}
    };

    for (size_t i = 0; i < ARRAY_SIZE(marker_names); i++) {
        if (marker_names[i].marker == marker)
            return marker_names[i].name;
    }

    return unknown_name;
}

static void print_marker(ParseContext *context, uint16_t marker)
{
    if (context->indent == 0) {
        printf("%s (0x%04x): size=2, offset=%" PRId64 "\n",
               get_marker_name(marker), marker, context->file_pos - 2);
    } else {
        printf("%*c%s (0x%04x): size=2, offset=%" PRId64 "\n", context->indent * 4, ' ',
               get_marker_name(marker), marker, context->file_pos - 2);
    }
}

static void print_marker_segment(ParseContext *context, uint16_t marker, uint16_t length)
{
    printf("%*c%s (0x%04x): size=%d, offset=%" PRId64 "\n", context->indent * 4, ' ',
           get_marker_name(marker), marker, length + 2, context->file_pos - 4);
}

static void print_rsiz(ParseContext *context, uint16_t rsiz)
{
    const char *profile_name;
    uint16_t profile;
    uint8_t mainlevel;
    uint8_t sublevel;

    static const char *profile_names[] = {
        "Capabilities specified in this Recommendation | International Standard only",
        "Codestream restricted as described for Profile 0",
        "Codestream restricted as described for Profile 1",
        "2k Digital Cinema Profile",
        "4k Digital Cinema Profile",
        "Scalable 2k Digital Cinema Profile",
        "Scalable 4k Digital Cinema Profile",
        "Long-term storage Profile",
        "Broadcast Contribution Single Tile Profile",
        "Broadcast Contribution Multi-tile Profile",
        "Broadcast Contribution Multi-tile Reversible Profile",
        "Broadcast Contribution Multi-tile Reversible Profile",
        "2k IMF Single Tile Lossy Profile",
        "4k IMF Single Tile Lossy Profile",
        "8k IMF Single Tile Lossy Profile",
        "2k IMF Single/Multi Tile Reversible Profile",
        "4k IMF Single/Multi Tile Reversible Profile",
        "8k IMF Single/Multi Tile Reversible Profile",
        "All other values reserved for future use by ITU-T | ISO/IEC"
    };

    if (rsiz <= 7) {
        profile_name = profile_names[rsiz];
        profile = rsiz;
        mainlevel = 255;
        sublevel = 255;
    } else if ((rsiz & 0x010f) == rsiz) {
        profile_name = profile_names[8];
        profile = rsiz >> 4;
        mainlevel = (uint8_t)(rsiz & 0x000f);
        sublevel = 255;
    } else if ((rsiz & 0x020f) == rsiz) {
        profile_name = profile_names[9];
        profile = rsiz >> 4;
        mainlevel = (uint8_t)(rsiz & 0x000f);
        sublevel = 255;
    } else if ((rsiz & 0x0306) == rsiz) {
        profile_name = profile_names[10];
        profile = rsiz;
        mainlevel = 6;
        sublevel = 255;
    } else if ((rsiz & 0x0307) == rsiz) {
        profile_name = profile_names[11];
        profile = rsiz;
        mainlevel = 7;
        sublevel = 255;
    } else {
        profile = rsiz >> 8;
        mainlevel = (uint8_t)(rsiz & 0x0f);
        sublevel = (uint8_t)((rsiz >> 4) & 0x0f);
        if (profile >= 0x04 && profile <= 0x09)
            profile_name = profile_names[12 + (profile - 0x04)];
        else
            profile_name = profile_names[18];
    }

    if (sublevel != 255) {
        printf("%*cRsiz: 0x%04x (profile=%u, mainlevel=%u, sublevel=%u, \"%s\")\n", context->indent * 4, ' ',
               rsiz, profile, mainlevel, sublevel, profile_name);
    } else if (mainlevel != 255) {
        printf("%*cRsiz: 0x%04x (profile=%u, mainlevel=%u, \"%s\")\n", context->indent * 4, ' ',
               rsiz, profile, mainlevel, profile_name);
    } else {
        printf("%*cRsiz: 0x%04x (profile=%u, \"%s\")\n", context->indent * 4, ' ',
               rsiz, profile, profile_name);
    }
}

static void print_scod(ParseContext *context, uint8_t scod)
{
    const char *precints_str;
    const char *sop_marker_str;
    const char *entropy_marker_str;

    if ((scod & 0x0001) == 0)
        precints_str = "Entropy coder, precincts with PPx = 15 and PPy = 15";
    else
        precints_str = "Entropy coder, defined precincts";
    if ((scod & 0x0002) == 0)
        sop_marker_str = "No SOP marker segments used";
    else
        sop_marker_str = "SOP marker segments may be used";
    if ((scod & 0x0004) == 0)
        entropy_marker_str = "No EPH marker used";
    else
        entropy_marker_str = "EPH marker shall be used";

    printf("%*cScod: 0x%02x (\"%s; %s; %s\")\n", context->indent * 4, ' ',
            scod, precints_str, sop_marker_str, entropy_marker_str);
}

static void print_bytes_line(ParseContext *context, uint64_t index, uint8_t *line, size_t num_values, size_t line_size)
{
    size_t i;

    printf("%*c%06" PRIx64 " ", context->indent * 4, ' ', index);
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

static int read_and_print_bytes(ParseContext *context, int64_t num_bytes)
{
    int64_t i;
    uint8_t line[16];
    size_t num_values = 0;

    for (i = 0; i < num_bytes; i++) {
        CHK(read_uint8(context));
        line[num_values] = (uint8_t)context->value;
        num_values++;
        if (num_values == sizeof(line)) {
            print_bytes_line(context, i + 1 - num_values, line, num_values, sizeof(line));
            num_values = 0;
        }
    }
    if (num_values > 0)
        print_bytes_line(context, i - num_values, line, num_values, sizeof(line));

    return 1;
}

static int parse_uint8(ParseContext *context, const char *name)
{
    CHK(read_uint8(context));
    printf("%*c%s: %u\n", context->indent * 4, ' ', name, (uint8_t)context->value);
    return 1;
}

static int parse_uint8_hex(ParseContext *context, const char *name)
{
    CHK(read_uint8(context));
    printf("%*c%s: 0x%02x\n", context->indent * 4, ' ', name, (uint8_t)context->value);
    return 1;
}

static int parse_uint16(ParseContext *context, const char *name)
{
    CHK(read_uint16(context));
    printf("%*c%s: %u\n", context->indent * 4, ' ', name, (uint16_t)context->value);
    return 1;
}

static int parse_uint32(ParseContext *context, const char *name)
{
    CHK(read_uint32(context));
    printf("%*c%s: %u\n", context->indent * 4, ' ', name, (uint32_t)context->value);
    return 1;
}

static int parse_marker(ParseContext *context, uint16_t *marker)
{
    if (!read_uint16(context))
        return 0;

    *marker = (uint16_t)context->value;

    if (((*marker) & 0xff00) != 0xff00 ||
        ((*marker) & 0x00ff) < 0x01 ||
        ((*marker) & 0x00ff) > 0xfe)
    {
        fprintf(stderr, "Invalid marker 0x%04x\n", *marker);
        return 0;
    }

    return 1;
}

static int parse_marker_segment_length(ParseContext *context, uint16_t *length)
{
    if (!read_uint16(context))
        return 0;

    *length = (uint16_t)context->value;

    if (*length < 2) {
        fprintf(stderr, "Marker segment length %d does not include 2-byte length\n", *length);
        return 0;
    }

    return 1;
}

static int parse_sot(ParseContext *context, uint16_t length)
{
    if (length != 10) {
        fprintf(stderr, "SOT length %u != 10\n", length);
        return 0;
    }

    context->sot_offset = context->file_pos - 4;

    CHK(parse_uint16(context, "Isot"));

    CHK(parse_uint32(context, "Psot"));
    context->psot = (uint32_t)context->value;
    if (context->psot > 0 && context->psot < 14) {
        fprintf(stderr, "Psot value %d is too small\n", (uint16_t)context->value);
        return 0;
    }

    CHK(parse_uint8(context, "TPsot"));
    if (context->value > 254) {
        fprintf(stderr, "Invalid TPsot %u\n", (uint8_t)context->value);
        return 0;
    }
    CHK(parse_uint8(context, "TNsot"));

    return 1;
}

static int parse_siz(ParseContext *context, uint16_t length)
{
    uint16_t rem_len = length - 2;
    uint16_t num_components;

    if (rem_len < 36) {
        fprintf(stderr, "SIZ length %u too small\n", length);
        return 0;
    }

    CHK(read_uint16(context));
    print_rsiz(context, (uint16_t)context->value);

    CHK(parse_uint32(context, "Xsiz"));
    CHK(parse_uint32(context, "Ysiz"));
    CHK(parse_uint32(context, "XOsiz"));
    CHK(parse_uint32(context, "YOsiz"));
    CHK(parse_uint32(context, "XTsiz"));
    CHK(parse_uint32(context, "YTsiz"));
    CHK(parse_uint32(context, "XTOsiz"));
    CHK(parse_uint32(context, "YTOsiz"));
    CHK(parse_uint16(context, "Csiz"));
    num_components = (uint16_t)context->value;

    rem_len -= 36;

    if (rem_len != 3 * num_components) {
        fprintf(stderr, "SIZE component length %u incorrect\n", length);
        return 0;
    }

    printf("%*cSsiz/XRsiz/YRsiz:\n", context->indent * 4, ' ');

    context->indent++;
    for (uint16_t i = 0 ; i < num_components; i++) {
        char sign;
        uint8_t bit_depth;
        uint8_t xrsiz;
        uint8_t yrsiz;

        CHK(read_uint8(context));
        sign = ((context->value >> 7) & 0x1) ? 's' : 'u';
        bit_depth = (uint8_t)(context->value & 0x7f) + 1;

        CHK(read_uint8(context));
        xrsiz = (uint8_t)context->value;

        CHK(read_uint8(context));
        yrsiz = (uint8_t)context->value;

        printf("%*c%u: %c%u, %u, %u\n", context->indent * 4, ' ',
                i, sign, bit_depth, xrsiz, yrsiz);
    }
    context->indent--;

    return 1;
}

static int parse_cod(ParseContext *context, uint16_t length)
{
    uint16_t rem_len = length - 2;

    if (rem_len < 10) {
        fprintf(stderr, "COD length %u too small\n", length);
        return 0;
    }

    CHK(read_uint8(context));
    print_scod(context, (uint8_t)context->value);

    printf("%*cSGcod:\n", context->indent * 4, ' ');
    context->indent++;
    CHK(parse_uint8(context, "Progression order"));
    CHK(parse_uint16(context, "Number of layers"));
    CHK(parse_uint8(context, "Multiple component transformation"));
    context->indent--;

    printf("%*cSPcod:\n", context->indent * 4, ' ');

    context->indent++;

    CHK(parse_uint8(context, "Decomposition levels"));
    CHK(parse_uint8(context, "Code-block width"));
    CHK(parse_uint8(context, "Code-block height"));
    CHK(parse_uint8(context, "Code-block style"));
    CHK(parse_uint8(context, "Transformation"));

    rem_len -= 10;

    if (rem_len > 0) {
        printf("%*cPrecint sizes:\n", context->indent * 4, ' ');
        context->indent++;
        for (uint16_t i = 0; i < rem_len; i++) {
            CHK(read_uint8(context));
            if (i == 0)
                printf("%*cLL sub-band: %u\n", context->indent * 4, ' ', (uint8_t)context->value);
            else
                printf("%*cLevel %u: %u\n", context->indent * 4, ' ', i - 1, (uint8_t)context->value);
        }
        context->indent--;
    }

    context->indent--;


    return 1;
}

static int parse_qcd(ParseContext *context, uint16_t length)
{
    uint16_t rem_len = length - 2;

    if (rem_len < 1) {
        fprintf(stderr, "QCD length %u too small\n", length);
        return 0;
    }

    CHK(parse_uint8(context, "Sqcd"));

    rem_len -= 1;

    printf("%*cSPqcd:\n", context->indent * 4, ' ');
    context->indent++;
    read_and_print_bytes(context, rem_len);
    context->indent--;

    return 1;
}

static int parse_tlm(ParseContext *context, uint16_t length)
{
    uint16_t rem_len = length - 2;
    uint8_t st;
    uint8_t sp;
    uint16_t ttlm_len;
    uint16_t ptlm_len;
    uint8_t tlm_index_index;
    uint16_t num_tile_parts;
    TLMIndex *tlm_index;

    if (length < 6) {
        fprintf(stderr, "TLM length %u is too short\n", length);
        return 0;
    }

    CHK(parse_uint8(context, "Ztlm"));
    CHK(parse_uint8_hex(context, "Stlm"));
    tlm_index_index = (uint8_t)context->value;

    rem_len -= 2;

    st = (context->value >> 4) & 0x03;
    if (st == 3) {
        fprintf(stderr, "Invalid ST=3 in TLM\n");
        return 0;
    }
    sp = (context->value >> 6) & 0x01;

    ttlm_len = st;
    ptlm_len = (sp + 1) * 2;
    num_tile_parts = rem_len / (ttlm_len + ptlm_len);
    if (rem_len != num_tile_parts * (ttlm_len + ptlm_len)) {
        fprintf(stderr, "TLM tile part lengths are invalid: %u != %u\n",
                rem_len, num_tile_parts * (ttlm_len + ptlm_len));
        return 0;
    }

    CHK(create_tile_part_index(st, num_tile_parts, &tlm_index));
    tlm_index->index = tlm_index_index;

    if (st == 0)
        printf("%*cPtml:\n", context->indent * 4, ' ');
    else
        printf("%*cTtlm/Ptml:\n", context->indent * 4, ' ');

    context->indent++;
    for (uint16_t i = 0; i < num_tile_parts; i++) {
        if (st == 1) {
            CHK(read_uint8(context));
            tlm_index->tile_indexes[i] = (uint16_t)context->value;
        } else if (st == 2) {
            CHK(read_uint16(context));
            tlm_index->tile_indexes[i] = (uint16_t)context->value;
        }
        if (sp == 0) {
            CHK(read_uint16(context));
            tlm_index->tile_part_lengths[i] = (uint32_t)context->value;
        } else {
            CHK(read_uint32(context));
            tlm_index->tile_part_lengths[i] = (uint32_t)context->value;
        }

        if (st == 0) {
            printf("%*c%u: %u\n", context->indent * 4, ' ',
                   i, tlm_index->tile_part_lengths[i]);
        } else {
            printf("%*c%u: %u, %u\n", context->indent * 4, ' ',
                   i, tlm_index->tile_indexes[i], tlm_index->tile_part_lengths[i]);
        }
    }
    context->indent--;

    CHK(insert_tile_part_index(context, tlm_index));

    return 1;
}

static int parse_tile_part_data(ParseContext *context, uint16_t tile_part_index)
{
    uint32_t tile_part_length = 0;
    if (context->tlm_index) {
        uint16_t tile_index;
        if (!get_tile_part_index_entry(context, tile_part_index, &tile_index, &tile_part_length))
            fprintf(stderr, "TLM does not contain entry for tile part %u\n", tile_part_index);
    }

    if (context->psot > 0) {
        if (tile_part_length > 0 && tile_part_length != context->psot) {
            fprintf(stderr, "TLM tile part length %u != Psot length %u. Ignoring the TLM length\n",
                    tile_part_length, context->psot);
        }
        tile_part_length = context->psot;
    }

    if (context->sot_offset + tile_part_length >= context->file_pos) {
        int64_t skip_size = tile_part_length - (context->file_pos - context->sot_offset);
        if (skip_size > 0) {
            printf("%*ctile part data: size=%" PRId64 ", offset=%" PRId64 "\n", context->indent * 4, ' ',
                skip_size, context->file_pos);

            context->indent++;
            CHK(skip_bytes(context, skip_size));
            context->indent--;
        }
    } else if (tile_part_length > 0) {
        fprintf(stderr, "File position is beyond the tile part data\n");
        return 0;
    } else {
        // Tile part data extends to the EOC at the end of the file.
        return 2;
    }

    return 1;
}

static int parse_com(ParseContext *context, uint16_t length)
{
    uint16_t rem_len = length - 2;

    if (rem_len < 1) {
        fprintf(stderr, "COM length %u too small\n", length);
        return 0;
    }

    CHK(parse_uint8(context, "Rcom"));

    rem_len -= 1;

    printf("%*cCcom:\n", context->indent * 4, ' ');
    context->indent++;
    read_and_print_bytes(context, rem_len);
    context->indent--;

    return 1;
}

static int parse_marker_segment(ParseContext *context, uint16_t marker, uint16_t length)
{
    print_marker_segment(context, marker, length);

    context->indent++;

    if (marker == SOT) {
        CHK(parse_sot(context, length));
    } else if (marker == SIZ) {
        CHK(parse_siz(context, length));
    } else if (marker == COD) {
        CHK(parse_cod(context, length));
    } else if (marker == QCD) {
        CHK(parse_qcd(context, length));
    } else if (marker == TLM) {
        CHK(parse_tlm(context, length));
    } else if (marker == COM) {
        CHK(parse_com(context, length));
    } else {
        if (length > 2)
            CHK(skip_bytes(context, length - 2));
    }

    context->indent--;

    return 1;
}


static void print_usage(const char *cmd)
{
    fprintf(stderr, "Text dump of J2C codestreams (ISO/IEC 15444-1)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [options] <filename>\n", cmd);
    fprintf(stderr, "  Indicate standard input using '-' for <filename>\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h | --help      Show help and exit\n");
}

int main(int argc, const char **argv)
{
    const char *filename;
    ParseContext context;
    int cmdln_index;
    uint16_t marker;
    int result = 0;
    uint16_t tile_part_index = 0;

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
        else
        {
            break;
        }
    }

    if (cmdln_index + 1 < argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Unknown option '%s'\n", argv[cmdln_index]);
        return 1;
    }
    if (cmdln_index >= argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Missing <filename>\n");
        return 1;
    }

    filename = argv[cmdln_index];


    if (!init_context(&context, filename))
        return 1;

    while (parse_marker(&context, &marker)) {
        if (context.expect_soc) {
            if (marker != SOC) {
                fprintf(stderr, "Expecting SOC marker 0x%04x but read 0x%04x\n", SOC, marker);
                result = 1;
                break;
            }
            context.expect_soc = 0;
        }

        if (is_marker_only(marker)) {
            if (marker == EOC) {
                context.indent--;
            }
            print_marker(&context, marker);
            if (marker == SOC || marker == SOT) {
                context.indent++;
            }

            if (marker == SOD) {
                int res = parse_tile_part_data(&context, tile_part_index);
                if (res != 1) {
                    if (res == 0)
                        result = 1;
                    else
                        result = 0;  // Tile part data to the EOC at the end of the file
                    break;
                }
                tile_part_index++;
            } else if (marker == EOC) {
                context.frame_count++;
                context.expect_soc = 1;
                free_tile_part_index(&context.tlm_index);
                context.sot_offset = 0;
                context.psot = 0;
                tile_part_index = 0;
            }
        } else {
            uint16_t length;
            if (!parse_marker_segment_length(&context, &length)) {
                result = 1;
                break;
            }
            if (!parse_marker_segment(&context, marker, length)) {
                result = 1;
                break;
            }
        }
    }

    printf("Frame count: %" PRId64 "\n", context.frame_count);

    deinit_context(&context);

    return result;
}
