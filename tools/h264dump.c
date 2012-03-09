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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <assert.h>
#include <ctype.h>


#define READ_BLOCK_SIZE         (64 * 1024)
#define INIT_BUFFER_SIZE        (1 * 1024 * 1024)
#define MAX_BUFFER_SIZE         (20 * 1024 * 1024)


#define CEIL_DIVISION(a, b)     (((a) + (b) - 1) / (b))

#define PRINT_UINT(name)        printf("%*c " name ": %"PRIu64"\n", context->indent * 4, ' ', context->value)
#define PRINT_INT(name)         printf("%*c " name ": %"PRId64"\n", context->indent * 4, ' ', context->svalue)

#define CHK(cmd)                                                                \
    if (!(cmd))                                                                 \
    {                                                                           \
        fprintf(stderr, "'%s' check failed at line %d\n", #cmd, __LINE__);      \
        return 0;                                                               \
    }


#define EXTENDED_SAR            255



typedef struct
{
    FILE *file;

    unsigned char *buffer;
    uint32_t buffer_size;

    uint64_t data_start_file_pos;
    uint32_t data_size;
    uint32_t data_pos;

    uint32_t nal_start;
    uint32_t num_bytes_in_nal_unit;
    uint32_t nal_size;

    uint64_t bit_pos;
    uint64_t end_bit_pos;

    uint64_t emu_prevent_count;

    uint64_t value;
    int64_t svalue;
    uint64_t next_value;

    uint8_t profile_idc;
    uint64_t chroma_format_idc;
    uint64_t chroma_array_type;
    uint8_t cpb_dpb_delays_present_flag;
    uint8_t cpb_removal_delay_length_minus1;
    uint8_t dpb_removal_delay_length_minus1;
    uint8_t pict_struct_present_flag;
    uint8_t time_offset_length;

    int indent;
} ParseContext;



static int init_context(ParseContext *context, const char *filename)
{
    memset(context, 0, sizeof(*context));
    context->time_offset_length = 24;
    context->chroma_format_idc = 1;
    context->chroma_array_type = 1;

    context->file = fopen(filename, "rb");
    if (!context->file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, strerror(errno));
        return 0;
    }

    context->buffer_size = INIT_BUFFER_SIZE;
    context->buffer = (unsigned char*)malloc(context->buffer_size);
    if (!context->buffer) {
        fprintf(stderr, "Failed to allocate buffer\n");
        return 0;
    }

    return 1;
}

static void deinit_context(ParseContext *context)
{
    if (context->file)
        fclose(context->file);

    free(context->buffer);
}


static int read_next_page(ParseContext *context)
{
    uint32_t num_read;

    if (context->data_size + READ_BLOCK_SIZE > context->buffer_size) {
        unsigned char *new_buffer;
        if (context->data_size + READ_BLOCK_SIZE > MAX_BUFFER_SIZE) {
            fprintf(stderr, "Num bytes in byte stream NAL unit exceeds maximum buffer size %u\n", MAX_BUFFER_SIZE);
            return 0;
        }
        if (context->buffer_size * 2 <= MAX_BUFFER_SIZE)
            context->buffer_size *= 2;
        else
            context->buffer_size = MAX_BUFFER_SIZE;

        new_buffer = (unsigned char*)realloc(context->buffer, context->buffer_size);
        if (!new_buffer) {
            fprintf(stderr, "Failed to reallocate buffer\n");
            return 0;
        }
        context->buffer = new_buffer;
    }

    num_read = fread(&context->buffer[context->data_size], 1, READ_BLOCK_SIZE, context->file);
    context->data_size += num_read;

    return num_read != 0;
}

static int load_byte(ParseContext *context, unsigned char *byte)
{
    if (context->data_pos >= context->data_size) {
        if (!read_next_page(context))
            return 0;
    }

    *byte = context->buffer[context->data_pos];
    context->data_pos++;

    return 1;
}


static int parse_byte_stream_nal_unit(ParseContext *context)
{
    uint8_t byte;
    uint32_t rem_size;
    uint32_t state;
    uint8_t nal_unit_type;

    rem_size = context->data_size - (context->nal_start + context->nal_size);
    if (rem_size > 0)
        memmove(context->buffer, &context->buffer[context->nal_start + context->nal_size], rem_size);
    context->data_start_file_pos += context->data_size - rem_size;
    context->data_size = rem_size;
    context->data_pos = 0;

    state = 0xffffff00;
    while (state != 0x000001 && state != 0x00000001) {
        if (!load_byte(context, &byte))
            return 0;
        state = (state << 8) | byte;
    }
    if (state != 0x00000001) {
        CHK(load_byte(context, &byte));
        context->data_pos--;
        nal_unit_type = byte & 0x1f;
        /* TODO: this is incomplete. Need to get the last VCL unit. See section 7.4.1.2.3  and B.1.2 */
        if (context->data_start_file_pos == 0 || nal_unit_type == 7 || nal_unit_type == 8)
            fprintf(stderr, "Warning: missing zero_byte before start_code_prefix_one_3bytes\n");
    }
    context->nal_start = context->data_pos;
    context->nal_size = 0;

    state = 0xffffff00;
    while (state != 0x000001 && state != 0x000000) {
        if (!load_byte(context, &byte))
            break;
        state = (state << 8) | byte;
    }

    context->num_bytes_in_nal_unit = context->data_pos - context->nal_start;
    if (state == 0x000001 || state == 0x000000) {
        context->num_bytes_in_nal_unit -= 3;
        if (state == 0x000000) {
            while (state != 0x00000001) {
                if (!load_byte(context, &byte))
                    break;
                state = (state << 8) | byte;
            }
        }
    }

    context->nal_size = context->data_pos - context->nal_start;
    if (state == 0x000001 || state == 0x00000001) {
        context->nal_size -= 3;
        if (load_byte(context, &byte)) {
            context->data_pos--;
            nal_unit_type = byte & 0x1f;
            /* TODO: this is incomplete. Need to get the last VCL unit. See section 7.4.1.2.3  and B.1.2 */
            if ((nal_unit_type == 7 || nal_unit_type == 8) || nal_unit_type == 9) {
                if (state != 0x00000001)
                    fprintf(stderr, "Warning: missing zero_byte before start_code_prefix_one_3bytes\n");
                else
                    context->nal_size -= 1;
            }
        }
    }

    context->bit_pos = context->nal_start * 8;
    context->end_bit_pos = context->nal_start * 8 + context->num_bytes_in_nal_unit * 8;

    return 1;
}

static int byte_aligned(ParseContext *context)
{
    return !(context->bit_pos & 7);
}

static int next_bits(ParseContext *context, uint8_t num_bits, uint8_t *advance_bits)
{
    const unsigned char *byte;
    uint32_t num_bytes;
    uint8_t skip_bits = 0;
    uint32_t i;

    CHK(num_bits > 0);
    CHK(num_bits <= 64);

    if (context->bit_pos + num_bits > context->end_bit_pos)
        return 0;

    byte = context->buffer + context->bit_pos / 8;
    num_bytes = ((context->bit_pos % 8) + num_bits + 7) / 8;
    context->next_value = 0;
    for (i = 0; i < num_bytes; i++) {
        if (*byte == 0x03 && context->bit_pos + i * 8 >= 16 && byte[-1] == 0x00 && byte[-2] == 0x00) {
            skip_bits += 8;
            context->emu_prevent_count++;
            byte++;
        }
        context->next_value = (context->next_value << 8) | (*byte);
        byte++;
    }

    context->next_value >>= (7 - ((context->bit_pos + num_bits - 1) % 8));
    context->next_value &= UINT64_MAX >> (64 - num_bits);

    if (advance_bits)
        *advance_bits = num_bits + skip_bits;

    return 1;
}

static int read_bits(ParseContext *context, uint8_t num_bits)
{
    uint8_t advance_bits;
    if (!next_bits(context, num_bits, &advance_bits))
        return 0;

    context->value = context->next_value;
    context->bit_pos += advance_bits;

    return 1;
}

static int skip_bits(ParseContext *context, uint64_t num_bits)
{
    CHK(context->bit_pos + num_bits <= context->end_bit_pos);

    context->bit_pos += num_bits;

    return 1;
}

static int exp_golumb(ParseContext *context)
{
    int8_t leading_zero_bits = -1;
    uint8_t b;
    uint64_t code_num;

    for (b = 0; !b && leading_zero_bits < 63; leading_zero_bits++) {
        CHK(read_bits(context, 1));
        b = (uint8_t)context->value;
    }
    if (!b) {
        fprintf(stderr, "Exp-Golumb size >= %d not supported\n", leading_zero_bits);
        return 0;
    }

    if (leading_zero_bits == 0) {
        context->value = 0;
    } else {
        CHK(read_bits(context, leading_zero_bits));
        code_num = (1ULL << leading_zero_bits) - 1 + context->value;
        context->value = code_num;
    }

    return 1;
}

static uint8_t get_bits_required(uint64_t value)
{
    uint8_t count = 63;
    while (count && !(value & (1ULL << count)))
        count--;

    return count;
}


#define f(a)    CHK(_f(context, a))
static int _f(ParseContext *context, uint8_t num_bits)
{
    return read_bits(context, num_bits);
}

#define u(a)    CHK(_u(context, a))
static int _u(ParseContext *context, uint8_t num_bits)
{
    return read_bits(context, num_bits);
}

#define ii(a)    CHK(_ii(context, a))
static int _ii(ParseContext *context, uint8_t num_bits)
{
    int result = read_bits(context, num_bits);
    if (!result)
        return result;

    if (context->value & (1ULL << (num_bits - 1)))
        context->svalue = (UINT64_MAX << num_bits) | context->value;
    else
        context->svalue = context->value;

    return 1;
}

#define ue()    CHK(_ue(context))
static int _ue(ParseContext *context)
{
    return exp_golumb(context);
}

#define se()    CHK(_se(context))
static int _se(ParseContext *context)
{
    int result = exp_golumb(context);
    if (!result)
        return result;

    context->svalue = ((context->value & 1) ? 1 : -1) * CEIL_DIVISION(context->value, 2);

    return 1;
}

static int more_rbsp_data(ParseContext *context)
{
    uint64_t bit_pos = context->end_bit_pos - 1;
    uint8_t b = 0;

    while (!b && bit_pos >= context->bit_pos) {
        b = context->buffer[bit_pos / 8] & (0x80 >> (bit_pos % 8));
        bit_pos--;
    }

    return (bit_pos >= context->bit_pos);
}

static int rbsp_trailing_bits(ParseContext *context)
{
    /* found that Avid AVCI 1080i content was missing a stop bit in the sequence parameter set and therefore only
       printing a message when this function fails */

    int valid;

    f(1); valid = (context->value != 0);
    while (valid && (context->bit_pos % 8) != 0) {
        f(1); valid = (context->value == 0);
    }

    if (!valid)
        fprintf(stderr, "Warning: invalid rbsp_trailing_bits\n");

    return 1;
}


static void print_nal_unit_type(ParseContext *context)
{
    static const char *NAL_UNIT_TYPE_STRINGS[] =
    {
        "Unspecified",
        "Coded slice of non-IDR picture",
        "Coded slice data partition A",
        "Coded slice data partition B",
        "Coded slice data partition C",
        "Coded slice of an IDR picture",
        "SEI",
        "Sequence parameter set",
        "Picture parameter set",
        "Access unit delimiter",
        "End of sequence",
        "End of stream",
        "Filler data",
        "Sequence parameter set extension",
        "Prefix NAL unit",
        "Subset sequence parameter set",
        "Reserved",
        "Reserved",
        "Reserved",
        "Coded slice of an auxillary coded picture without partitioning",
        "Coded slice extension",
        "Reserved",
        "Reserved",
        "Reserved",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
    };

    uint8_t nal_unit_type = context->value & 31;

    printf("%*c nal_unit_type: %u (%s)\n", context->indent * 4, ' ', nal_unit_type,
           NAL_UNIT_TYPE_STRINGS[nal_unit_type]);
}

static void print_profile_and_flags(ParseContext *context, uint8_t profile_idc, uint8_t flags)
{
    typedef struct
    {
        uint8_t profile_idc;
        uint8_t flags;
        const char *profile_str;
    } ProfileName;

    static const ProfileName PROFILE_NAMES[] =
    {
        { 66,   0x02,   "Constrained Baseline"},
        { 66,   0x00,   "Baseline"},
        { 77,   0x00,   "Main"},
        { 88,   0x00,   "Extended"},
        {100,   0x00,   "High"},
        {110,   0x08,   "High 10 Intra"},
        {110,   0x00,   "High 10"},
        {122,   0x08,   "High 4:2:2 Intra"},
        {122,   0x00,   "High 4:2:2"},
        {244,   0x08,   "High 4:4:4 Intra"},
        {244,   0x00,   "High 4:4:4"},
        { 44,   0x00,   "CAVLC 4:4:4 Intra"},
    };

    const char *profile_str = "unknown";
    size_t i;
    for (i = 0; i < sizeof(PROFILE_NAMES) / sizeof(PROFILE_NAMES[0]); i++) {
        if (profile_idc == PROFILE_NAMES[i].profile_idc &&
            (PROFILE_NAMES[i].flags == 0 || (flags & PROFILE_NAMES[i].flags)))
        {
            profile_str = PROFILE_NAMES[i].profile_str;
            break;
        }
    }

    printf("%*c profile_idc: %u (%s)\n", context->indent * 4, ' ', profile_idc, profile_str);

    printf("%*c constraint_set0_flag: %u\n", context->indent * 4, ' ', flags & 1);
    printf("%*c constraint_set1_flag: %u\n", context->indent * 4, ' ', (flags >> 1) & 1);
    printf("%*c constraint_set2_flag: %u\n", context->indent * 4, ' ', (flags >> 2) & 1);
    printf("%*c constraint_set3_flag: %u\n", context->indent * 4, ' ', (flags >> 3) & 1);
    printf("%*c constraint_set4_flag: %u\n", context->indent * 4, ' ', (flags >> 4) & 1);
    printf("%*c constraint_set5_flag: %u\n", context->indent * 4, ' ', (flags >> 5) & 1);
}

static void print_level(ParseContext *context, uint8_t level_idc, uint8_t flags)
{
    if (level_idc == 11 && (flags & 0x08))
        printf("%*c level_idc: %u (1b)\n", context->indent * 4, ' ', level_idc);
    else
        printf("%*c level_idc: %u (%.1f)\n", context->indent * 4, ' ', level_idc, level_idc / 10.0);
}

static void print_chroma_format(ParseContext *context, uint8_t chroma_format_idc)
{
    static const char *CHROMA_FORMAT_STRINGS[] =
    {
        "Monochrome",
        "4:2:0",
        "4:2:2",
        "4:4:4",
    };

    if (chroma_format_idc < sizeof(CHROMA_FORMAT_STRINGS) / sizeof(CHROMA_FORMAT_STRINGS[0])) {
        printf("%*c chroma_format_idc: %u (%s)\n", context->indent * 4, ' ', chroma_format_idc,
               CHROMA_FORMAT_STRINGS[chroma_format_idc]);
    } else {
        printf("%*c chroma_format_idc: %u (unknown)\n", context->indent * 4, ' ', chroma_format_idc);
    }
}

static void print_aspect_ratio(ParseContext *context, uint8_t aspect_ratio_idc)
{
    static const char *SAMPLE_ASPECT_RATIO_STRINGS[] =
    {
        "Unspecified",
        "1:1",
        "12:11",
        "10:11",
        "16:11",
        "40:33",
        "24:11",
        "20:11",
        "32:11",
        "80:33",
        "18:11",
        "15:11",
        "64:33",
        "160:99",
        "4:3",
        "3:2",
        "2:1",
    };

    if (aspect_ratio_idc < sizeof(SAMPLE_ASPECT_RATIO_STRINGS) / sizeof(SAMPLE_ASPECT_RATIO_STRINGS[0])) {
        printf("%*c aspect_ratio_idc: %u (%s)\n", context->indent * 4, ' ', aspect_ratio_idc,
               SAMPLE_ASPECT_RATIO_STRINGS[aspect_ratio_idc]);
    } else if (aspect_ratio_idc == EXTENDED_SAR) {
        printf("%*c aspect_ratio_idc: %u (Extended_SAR)\n", context->indent * 4, ' ', aspect_ratio_idc);
    } else {
        printf("%*c aspect_ratio_idc: %u (Reserved)\n", context->indent * 4, ' ', aspect_ratio_idc);
    }
}

static void print_video_format(ParseContext *context, uint8_t video_format)
{
    static const char *VIDEO_FORMAT_STRINGS[] =
    {
        "Component",
        "PAL",
        "NTSC",
        "SECAM",
        "MAC",
        "Unspecified",
        "Reserved",
        "Reserved",
    };

    printf("%*c video_format: %u (%s)\n", context->indent * 4, ' ', video_format,
           VIDEO_FORMAT_STRINGS[video_format & 0x07]);
}

static void print_pict_struct(ParseContext *context, uint8_t pict_struct)
{
    static const char *PICT_STRUCT_STRINGS[] =
    {
        "(progressive) frame",
        "top field",
        "bottom field",
        "top field, bottom field, in that order",
        "bottom field, top field, in that order",
        "top field, bottom field, top field repeated, in that order",
        "bottom field, top field, bottom field repeated in that order",
        "frame doubling",
        "frame tripling",
    };

    if (pict_struct < sizeof(PICT_STRUCT_STRINGS) / sizeof(PICT_STRUCT_STRINGS[0])) {
        printf("%*c pict_struct: %u (%s)\n", context->indent * 4, ' ', pict_struct,
               PICT_STRUCT_STRINGS[pict_struct]);
    } else {
        printf("%*c pict_struct: %u (reserved)\n", context->indent * 4, ' ', pict_struct);
    }
}

static void print_uuid(ParseContext *context, uint64_t uuid_high, uint64_t uuid_low)
{
    int i;

    printf("%*c uuid_iso_iec11576: ", context->indent * 4, ' ');
    for (i = 0; i < 4; i++)
        printf("%02x", (unsigned char)((uuid_high >> ((7 - i) * 8)) & 0xff));
    printf("-");
    for (; i < 6; i++)
        printf("%02x", (unsigned char)((uuid_high >> ((7 - i) * 8)) & 0xff));
    printf("-");
    for (; i < 8; i++)
        printf("%02x", (unsigned char)((uuid_high >> ((7 - i) * 8)) & 0xff));
    printf("-");
    for (i = 0; i < 2; i++)
        printf("%02x", (unsigned char)((uuid_low >> ((7 - i) * 8)) & 0xff));
    printf("-");
    for (; i < 8; i++)
        printf("%02x", (unsigned char)((uuid_low >> ((7 - i) * 8)) & 0xff));
    printf("\n");
}

static void print_bytes_line(ParseContext *context, uint64_t index, uint8_t *line, size_t num_values, size_t line_size)
{
    size_t i;

    printf("%*c %06"PRIx64" ", context->indent * 4, ' ', index);
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

static int read_and_print_bytes(ParseContext *context, uint32_t num_bytes)
{
    uint32_t i;
    uint8_t line[16];
    size_t num_values = 0;

    for (i = 0; i < num_bytes; i++) {
        u(8); line[num_values] = (uint8_t)context->value;
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


static int hrd_parameters(ParseContext *context)
{
    uint64_t sched_sel_idx;
    uint64_t cpb_cnt_minus1;

    ue(); PRINT_UINT("cpb_cnt_minus1");
    cpb_cnt_minus1 = context->value;
    u(4); PRINT_UINT("bit_rate_scale");
    u(4); PRINT_UINT("cpb_size_scale");
    context->indent++;
    for (sched_sel_idx = 0; sched_sel_idx <= cpb_cnt_minus1; sched_sel_idx++) {
        ue(); PRINT_UINT("bit_rate_value_minus1");
        ue(); PRINT_UINT("cpb_size_value_minus1");
        u(1); PRINT_UINT("cbr_flag");
    }
    context->indent--;
    u(5); PRINT_UINT("initial_cpb_removal_delay_length_minus1");
    u(5); PRINT_UINT("cpb_removal_delay_length_minus1");
    context->cpb_removal_delay_length_minus1 = (uint8_t)context->value;
    u(5); PRINT_UINT("dpb_removal_delay_length_minus1");
    context->dpb_removal_delay_length_minus1 = (uint8_t)context->value;
    u(5); PRINT_UINT("time_offset_length");
    context->time_offset_length = (uint8_t)context->value;

    return 1;
}

static int vui_parameters(ParseContext *context)
{
    uint8_t nal_hrd_parameters_present_flag;
    uint8_t vcl_hrd_parameters_present_flag;

    u(1); PRINT_UINT("aspect_ratio_present_flag");
    if (context->value) {
        context->indent++;
        u(8); print_aspect_ratio(context, (uint8_t)context->value);
        if (context->value == EXTENDED_SAR) {
            context->indent++;
            u(16); PRINT_UINT("sar_width");
            u(16); PRINT_UINT("sar_height");
            context->indent--;
        }
        context->indent--;
    }
    u(1); PRINT_UINT("overscan_info_present_flag");
    if (context->value) {
        context->indent++;
        u(1); PRINT_UINT("overscan_appropriate_flag");
        context->indent--;
    }
    u(1); PRINT_UINT("video_signal_type_present_flag");
    if (context->value) {
        context->indent++;
        u(3); print_video_format(context, (uint8_t)context->value);
        u(1); PRINT_UINT("video_full_range");
        u(1); PRINT_UINT("colour_description_present_flag");
        if (context->value) {
            u(8); PRINT_UINT("colour_primaries");
            u(8); PRINT_UINT("transfer_characteristics");
            u(8); PRINT_UINT("matrix_coefficients");
        }
        context->indent--;
    }
    u(1); PRINT_UINT("chroma_loc_info_present_flag");
    if (context->value) {
        context->indent++;
        ue(); PRINT_UINT("chroma_sample_loc_type_top_field");
        ue(); PRINT_UINT("chroma_sample_loc_type_bottom_field");
        context->indent--;
    }
    u(1); PRINT_UINT("timing_info_present_flag");
    if (context->value) {
        context->indent++;
        u(32); PRINT_UINT("num_units_in_tick");
        u(32); PRINT_UINT("time_scale");
        u(1); PRINT_UINT("fixed_frame_rate_flag");
        context->indent--;
    }
    u(1); PRINT_UINT("nal_hrd_parameters_present_flag");
    nal_hrd_parameters_present_flag = context->value;
    if (context->value) {
        context->indent++;
        CHK(hrd_parameters(context));
        context->indent--;
    }
    u(1); PRINT_UINT("vcl_hrd_parameters_present_flag");
    vcl_hrd_parameters_present_flag = context->value;
    if (context->value) {
        context->indent++;
        CHK(hrd_parameters(context));
        context->indent--;
    }
    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
        u(1); PRINT_UINT("low_delay_hrd_flag");
    }
    context->cpb_dpb_delays_present_flag = nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag;
    u(1); PRINT_UINT("pict_struct_present_flag");
    context->pict_struct_present_flag = (uint8_t)context->value;
    u(1); PRINT_UINT("bitstream_restriction_flag");
    if (context->value) {
        u(1); PRINT_UINT("motion_vectors_over_pic_boundaries_flag");
        ue(); PRINT_UINT("max_bytes_per_pic_denom");
        ue(); PRINT_UINT("max_bytes_per_mb_denom");
        ue(); PRINT_UINT("log2_max_mv_length_horizontal");
        ue(); PRINT_UINT("log2_max_mv_length_vertical");
        ue(); PRINT_UINT("num_reorder_frames");
        ue(); PRINT_UINT("max_dec_frame_buffering");
    }

    return 1;
}

static int scaling_list(ParseContext *context, uint32_t dim)
{
    uint8_t last_scale = 8, next_scale = 8;
    uint32_t list_size = dim * dim;
    uint8_t i;

    for (i = 0; i < list_size; i++) {
        if (next_scale != 0) {
            se();
            next_scale = (uint8_t)((last_scale + context->svalue + 256) % 256);
        }
        last_scale = (next_scale == 0 ? last_scale : next_scale);

        if (i % dim == 0)
            printf("%*c %02x", context->indent * 4, ' ', last_scale);
        else if ((i + 1) % dim == 0)
            printf(" %02x\n", last_scale);
        else
            printf(" %02x", last_scale);
    }

    return 1;
}

static int pic_timing(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    printf("%*c pic_timing (type=%"PRIu64", size=%"PRIu64"):\n", context->indent * 4, ' ', payload_type, payload_size);
    context->indent++;

    if (context->cpb_dpb_delays_present_flag) {
        u(context->cpb_removal_delay_length_minus1 + 1); PRINT_UINT("cpb_removal_delay");
        u(context->dpb_removal_delay_length_minus1 + 1); PRINT_UINT("dpb_removal_delay");
    }
    if (context->pict_struct_present_flag) {
        uint64_t pict_struct;
        uint8_t num_clock_ts = 0;
        uint8_t i;

        u(4); print_pict_struct(context, (uint8_t)context->value);
        pict_struct = context->value;
        switch (pict_struct)
        {
            case 0:
            case 1:
            case 2:
                num_clock_ts = 1;
                break;
            case 3:
            case 4:
            case 7:
                num_clock_ts = 2;
                break;
            case 5:
            case 6:
            case 8:
                num_clock_ts = 3;
                break;
            default:
                CHK(pict_struct <= 8);
                break;
        }

        for (i = 0; i < num_clock_ts; i++) {
            u(1); PRINT_UINT("clock_timestamp_flag");
            if (context->value) {
                uint8_t full_timestamp_flag;

                context->indent++;

                u(2); PRINT_UINT("ct_type");
                u(1); PRINT_UINT("nuit_field_based_flag");
                u(5); PRINT_UINT("counting_type");
                u(1); PRINT_UINT("full_timestamp_flag");
                full_timestamp_flag = (uint8_t)context->value;
                u(1); PRINT_UINT("discontinuity_flag");
                u(1); PRINT_UINT("cnt_dropped_flag");
                u(8); PRINT_UINT("n_frames");
                if (full_timestamp_flag) {
                    u(6); PRINT_UINT("seconds");
                    u(6); PRINT_UINT("minutes");
                    u(5); PRINT_UINT("hours");
                } else {
                    u(1); PRINT_UINT("seconds_flag");
                    if (context->value) {
                        u(6); PRINT_UINT("seconds");
                        u(1); PRINT_UINT("minutes_flag");
                        if (context->value) {
                            u(6); PRINT_UINT("minutes");
                            u(1); PRINT_UINT("hours_flag");
                            if (context->value) {
                                u(5); PRINT_UINT("hours");
                            }
                        }
                    }
                }
                if (context->time_offset_length > 0) {
                    ii(context->time_offset_length); PRINT_INT("time_offset");
                }

                context->indent--;
            }
        }
    }

    context->indent--;
    return 1;
}

static int user_data_unregistered(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    uint64_t uuid_high, uuid_low;

    printf("%*c user_data_unregistered (type=%"PRIu64", size=%"PRIu64"):\n", context->indent * 4, ' ', payload_type, payload_size);
    context->indent++;

    u(64); uuid_high = context->value;
    u(64); uuid_low = context->value;
    print_uuid(context, uuid_high, uuid_low);

    printf("%*c payload_data:\n", context->indent * 4, ' ');
    context->indent++;
    CHK(read_and_print_bytes(context, payload_size - 16));
    context->indent--;

    context->indent--;
    return 1;
}

static int sei_payload(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    uint64_t next_bit_pos = context->bit_pos + payload_size * 8;
    context->emu_prevent_count = 0;

    if (payload_type == 1) {
        CHK(pic_timing(context, payload_type, payload_size));
    } else if (payload_type == 5) {
        CHK(user_data_unregistered(context, payload_type, payload_size));
    } else {
        printf("%*c payload (type=%"PRIu64", size=%"PRIu64")\n", context->indent * 4, ' ', payload_type, payload_size);
        context->indent++;
        printf("%*c data:\n", context->indent * 4, ' ');
        context->indent++;
        CHK(read_and_print_bytes(context, payload_size));
        context->indent--;
        context->indent--;
    }

    next_bit_pos -= context->emu_prevent_count * 8;

    CHK(context->bit_pos <= next_bit_pos);
    if (context->bit_pos < next_bit_pos) {
        CHK(skip_bits(context, next_bit_pos - context->bit_pos));
    }

    if (!byte_aligned(context)) {
        CHK(rbsp_trailing_bits(context));
    }

    return 1;
}

static int sei_message(ParseContext *context)
{
    uint64_t payload_type = 0;
    uint64_t payload_size = 0;

    while (next_bits(context, 8, NULL) && context->next_value == 0xff) {
        f(8);
        payload_type += 255;
    }
    u(8); payload_type += context->value;

    while (next_bits(context, 8, NULL) && context->next_value == 0xff) {
        f(8);
        payload_size += 255;
    }
    u(8); payload_size += context->value;


    CHK(sei_payload(context, payload_type, payload_size));

    return 1;
}

static int sei(ParseContext *context)
{
    printf("%*c sei:\n", context->indent * 4, ' ');
    context->indent++;

    do {
        CHK(sei_message(context));
    } while (more_rbsp_data(context));

    CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int sequence_parameter_set_data(ParseContext *context)
{
    uint8_t constraint_flags;
    uint64_t pict_order_cnt_type;

    printf("%*c sequence_parameter_set:\n", context->indent * 4, ' ');
    context->indent++;

    u(8); context->profile_idc = (uint8_t)context->value;
    u(1); constraint_flags = (uint8_t)context->value;
    u(1); constraint_flags |= (uint8_t)context->value << 1;
    u(1); constraint_flags |= (uint8_t)context->value << 2;
    u(1); constraint_flags |= (uint8_t)context->value << 3;
    u(1); constraint_flags |= (uint8_t)context->value << 4;
    u(1); constraint_flags |= (uint8_t)context->value << 5;
    print_profile_and_flags(context, context->profile_idc, constraint_flags);
    u(2); PRINT_UINT("reserved_zero_2bits");
    u(8); print_level(context, (uint8_t)context->value, constraint_flags);
    ue(); PRINT_UINT("seq_parameter_set_id");

    if (context->profile_idc == 100 || context->profile_idc == 110 ||
        context->profile_idc == 122 || context->profile_idc == 244 || context->profile_idc ==  44 ||
        context->profile_idc ==  83 || context->profile_idc ==  86 || context->profile_idc == 118 ||
        context->profile_idc == 128)
    {
        ue(); print_chroma_format(context, context->value);
        context->chroma_format_idc = context->value;
        if (context->chroma_format_idc == 3) {
            u(1); PRINT_UINT("separate_colour_plane_flag");
            if (context->value == 0)
                context->chroma_array_type = context->chroma_format_idc;
            else
                context->chroma_array_type = 0;
        }
        ue(); PRINT_UINT("bit_depth_luma_minus8");
        ue(); PRINT_UINT("bit_depth_chroma_minus8");
        u(1); PRINT_UINT("qpprime_y_zero_transform_bypass_flag");
        u(1); PRINT_UINT("seq_scaling_matrix_present_flag");
        if (context->value) {
            uint8_t i;

            context->indent++;
            for (i = 0; i < ((context->chroma_format_idc != 3) ? 8 : 12); i++) {
                u(1); PRINT_UINT("seq_scaling_list_present_flag");
                if (context->value) {
                    context->indent++;
                    if (i < 6) {
                        CHK(scaling_list(context, 4));
                    } else {
                        CHK(scaling_list(context, 8));
                    }
                    context->indent--;
                }
            }
            context->indent--;
        }
    }
    ue(); PRINT_UINT("log2_max_frame_num_minus4");
    ue(); PRINT_UINT("pict_order_cnt_type");
    pict_order_cnt_type = context->value;
    if (pict_order_cnt_type == 0) {
        ue(); PRINT_UINT("log2_max_pic_order_cnt_lsb_minus4");
    } else if (pict_order_cnt_type == 1) {
        uint64_t num_ref_frames_in_pic_order_cnt_cycle;
        uint64_t i;

        u(1); PRINT_UINT("delta_pic_order_always_zero_flag");
        se(); PRINT_UINT("offset_for_non_ref_pic");
        se(); PRINT_UINT("offset_for_top_to_bottom_field");
        ue(); PRINT_UINT("num_ref_frames_in_pic_order_cnt_cycle");
        num_ref_frames_in_pic_order_cnt_cycle = context->value;
        context->indent++;
        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            se(); PRINT_UINT("offset_for_ref_frame");
        }
        context->indent--;
    }
    ue(); PRINT_UINT("max_num_ref_frames");
    u(1); PRINT_UINT("gaps_in_frame_num_value_allowed_flag");
    ue(); PRINT_UINT("pic_width_in_mbs_minus1");
    ue(); PRINT_UINT("pic_height_in_map_units_minus1");
    u(1); PRINT_UINT("frame_mbs_only_flag");
    if (!context->value) {
        u(1); PRINT_UINT("mb_adaptive_frame_field_flag");
    }
    u(1); PRINT_UINT("direct_8x8_inference_flag");
    u(1); PRINT_UINT("frame_cropping_flag");
    if (context->value) {
        context->indent++;
        ue(); PRINT_UINT("frame_crop_left_offset");
        ue(); PRINT_UINT("frame_crop_right_offset");
        ue(); PRINT_UINT("frame_crop_top_offset");
        ue(); PRINT_UINT("frame_crop_bottom_offset");
        context->indent--;
    }
    u(1); PRINT_UINT("vui_parameters_present_flag");
    if (context->value) {
        context->indent++;
        CHK(vui_parameters(context));
        context->indent--;
    }

    context->indent--;
    return 1;
}

static int sequence_parameter_set(ParseContext *context)
{
    CHK(sequence_parameter_set_data(context));
    CHK(rbsp_trailing_bits(context));

    return 1;
}

static int picture_parameter_set(ParseContext *context)
{
    uint64_t num_slice_groups_minus1;

    printf("%*c picture_parameter_set:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); PRINT_UINT("pic_parameter_set_id");
    ue(); PRINT_UINT("seq_parameter_set_id");
    u(1); PRINT_UINT("entropy_coding_mode_flag");
    u(1); PRINT_UINT("bottom_field_pic_order_in_frame_present_flag");
    ue(); PRINT_UINT("num_slice_groups_minus1");
    num_slice_groups_minus1 = context->value;
    if (context->value > 0) {
        uint64_t slice_group_map_type;
        uint64_t i;

        ue(); PRINT_UINT("slice_group_map_type");
        slice_group_map_type = context->value;

        context->indent++;
        if (slice_group_map_type == 0) {
            for (i = 0; i <= num_slice_groups_minus1; i++) {
                ue(); PRINT_UINT("run_length_minus1");
            }
        } else if (slice_group_map_type == 2) {
            for (i = 0; i < num_slice_groups_minus1; i++) {
                ue(); PRINT_UINT("top_left");
                ue(); PRINT_UINT("bottom_right");
            }
        } else if (slice_group_map_type == 3 || slice_group_map_type == 4 || slice_group_map_type == 5) {
            u(1); PRINT_UINT("slice_group_change_direction_flag");
            ue(); PRINT_UINT("slice_group_change_rate_minus1");
        } else if (slice_group_map_type == 6) {
            uint64_t pic_size_in_map_units_minus1;

            ue(); PRINT_UINT("pic_size_in_map_units_minus1");
            pic_size_in_map_units_minus1 = context->value;
            context->indent++;
            for (i = 0; i <= pic_size_in_map_units_minus1; i++) {
                u(get_bits_required(num_slice_groups_minus1 + 1)); PRINT_UINT("slice_group_id");
            }
            context->indent--;
        }
        context->indent--;
    }
    ue(); PRINT_UINT("num_ref_idx_10_default_active_minus1");
    ue(); PRINT_UINT("num_ref_idx_11_default_active_minus1");
    u(1); PRINT_UINT("weighted_pred_flag");
    u(2); PRINT_UINT("weighted_bipred_idc");
    se(); PRINT_INT("pic_init_qp_minus26");
    se(); PRINT_INT("pic_init_qs_minus26");
    se(); PRINT_INT("chroma_qp_index_offset");
    u(1); PRINT_UINT("deblocking_filter_control_present_flag");
    u(1); PRINT_UINT("constrained_intra_pred_flag");
    u(1); PRINT_UINT("redundant_pic_cnt_present_flag");
    if (more_rbsp_data(context)) {
        uint64_t transform_8x8_mode_flag;

        u(1); PRINT_UINT("transform_8x8_mode_flag");
        transform_8x8_mode_flag = context->value;
        u(1); PRINT_UINT("pic_scaling_matrix_present_flag");
        if (context->value) {
            uint64_t i;

            context->indent++;
            for (i = 0; i < 6 + (context->chroma_format_idc != 3 ? 2 : 6) * transform_8x8_mode_flag; i++) {
                u(1); PRINT_UINT("pic_scaling_present_flag");
                if (context->value) {
                    context->indent++;
                    if (i < 6) {
                        CHK(scaling_list(context, 4));
                    } else {
                        CHK(scaling_list(context, 8));
                    }
                    context->indent--;
                }
            }
            context->indent--;
        }
        se(); PRINT_INT("second_chroma_qp_index_offset");
    }

    CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int sequence_parameter_set_extension(ParseContext *context)
{
    uint64_t bit_depth_aux_minus8;
    printf("%*c sequence_parameter_set_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); PRINT_UINT("seq_parameter_set_id");
    ue(); PRINT_UINT("aux_format_idc");
    if (context->value) {
        ue(); PRINT_UINT("bit_depth_aux_minus8");
        bit_depth_aux_minus8 = context->value;
        u(1); PRINT_UINT("alpha_incr_flag");
        u(bit_depth_aux_minus8 + 9); PRINT_UINT("alpha_opaque_value");
        u(bit_depth_aux_minus8 + 9); PRINT_UINT("alpha_transparent_value");
    }
    u(1); PRINT_UINT("additional_extension_flag");
    CHK(rbsp_trailing_bits(context));

    context->indent++;
    return 1;
}

static int sequence_parameter_set_svc_extension(ParseContext *context)
{
    uint64_t extended_spatial_scalability_idc;

    printf("%*c sequence_parameter_set_svc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    u(1); PRINT_UINT("inter_layer_deblocking_filter_control_present_flag");
    u(2); PRINT_UINT("extended_spatial_scalability_idc");
    extended_spatial_scalability_idc = context->value;
    if (context->chroma_array_type == 1 || context->chroma_array_type == 2) {
        u(1); PRINT_UINT("chroma_phase_x_plus1_flag");
    }
    if (context->chroma_array_type == 1) {
        u(2); PRINT_UINT("chroma_phase_y_plus1");
    }
    if (extended_spatial_scalability_idc == 1) {
        if (context->chroma_array_type > 0) {
            u(1); PRINT_UINT("seq_ref_layer_chroma_phase_x_plus1_flag");
            u(2); PRINT_UINT("seq_ref_layer_chroma_phase_y_plus1");
        }
        se(); PRINT_INT("seq_scaled_ref_layer_left_offset");
        se(); PRINT_INT("seq_scaled_ref_layer_top_offset");
        se(); PRINT_INT("seq_scaled_ref_layer_right_offset");
        se(); PRINT_INT("seq_scaled_ref_layer_bottom_offset");
    }
    u(1); PRINT_UINT("seq_tcoeff_level_prediction_flag");
    if (context->value) {
        u(1); PRINT_UINT("adaptive_tcoeff_level_prediction_flag");
    }
    u(1); PRINT_UINT("slice_header_restriction_flag");

    context->indent++;
    return 1;
}

static int svc_vui_parameters_extension(ParseContext *context)
{
    uint64_t vui_ext_num_entries_minus1;
    uint8_t vui_ext_nal_hrd_parameters_present_flag;
    uint8_t vui_ext_vcl_hrd_parameters_present_flag;
    uint64_t i;

    printf("%*c svc_vui_parameters_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); PRINT_UINT("vui_ext_num_entries_minus1");
    vui_ext_num_entries_minus1 = context->value;
    for (i = 0; i <= vui_ext_num_entries_minus1; i++) {
        printf("%*c entry %"PRIu64":\n", context->indent * 4, ' ', i);
        context->indent++;

        u(3); PRINT_UINT("vui_ext_dependency_id");
        u(4); PRINT_UINT("vui_ext_quality_id");
        u(3); PRINT_UINT("vui_ext_temporal_id");
        u(1); PRINT_UINT("vui_ext_timing_info_present_flag");
        if (context->value) {
            context->indent++;
            u(32); PRINT_UINT("vui_ext_num_units_in_tick");
            u(32); PRINT_UINT("vui_ext_time_scale");
            u(1); PRINT_UINT("vui_ext_fixed_frame_rate_flag");
            context->indent--;
        }
        u(1); PRINT_UINT("vui_ext_nal_hrd_parameters_present_flag");
        vui_ext_nal_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        u(1); PRINT_UINT("vui_ext_vcl_hrd_parameters_present_flag");
        vui_ext_vcl_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        if (vui_ext_nal_hrd_parameters_present_flag || vui_ext_vcl_hrd_parameters_present_flag) {
            u(1); PRINT_UINT("vui_ext_low_delay_hrd_flag");
        }
        u(1); PRINT_UINT("vui_ext_pic_struct_present_flag");

        context->indent--;
    }

    context->indent++;
    return 1;
}

static int sequence_parameter_set_mvc_extension(ParseContext *context)
{
    uint64_t num_views_minus1, num_refs, num_level_values_signalled_minus1,
             num_applicable_ops_minus1, applicable_op_num_target_views_minus1;
    uint64_t i, j, k;

    printf("%*c sequence_parameter_set_mvc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); PRINT_UINT("num_views_minus1");
    num_views_minus1 = context->value;
    for (i = 0; i <= num_views_minus1; i++) {
        ue(); PRINT_UINT("view_id");
    }
    for (i = 1; i <= num_views_minus1; i++) {
        ue(); PRINT_UINT("num_anchor_refs_I0");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); PRINT_UINT("anchor_ref_I0");
        }
        ue(); PRINT_UINT("num_anchor_refs_I1");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); PRINT_UINT("anchor_ref_I1");
        }
    }
    for (i = 1; i <= num_views_minus1; i++) {
        ue(); PRINT_UINT("num_non_anchor_refs_I0");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); PRINT_UINT("non_anchor_ref_I0");
        }
        ue(); PRINT_UINT("num_non_anchor_refs_I1");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); PRINT_UINT("non_anchor_ref_I1");
        }
    }
    ue(); PRINT_UINT("num_level_values_signalled_minus1");
    num_level_values_signalled_minus1 = context->value;
    for (i = 0; i <= num_level_values_signalled_minus1; i++) {
        u(8); PRINT_UINT("level_idc");
        ue(); PRINT_UINT("num_applicable_ops_minus1");
        num_applicable_ops_minus1 = context->value;
        for (j = 0; j <= num_applicable_ops_minus1; j++) {
            u(3); PRINT_UINT("applicable_op_temporal_id");
            ue(); PRINT_UINT("applicable_op_num_target_views_minus1");
            applicable_op_num_target_views_minus1 = context->value;
            for (k = 0; k <= applicable_op_num_target_views_minus1; k++) {
                ue(); PRINT_UINT("applicable_op_target_view_id");
            }
            ue(); PRINT_UINT("applicable_op_num_views_minus1");
        }
    }

    context->indent++;
    return 1;
}

static int mvc_vui_parameters_extension(ParseContext *context)
{
    uint64_t vui_mvc_num_ops_minus1, vui_mvc_num_target_output_views_minus1;
    uint8_t vui_mvc_nal_hrd_parameters_present_flag;
    uint8_t vui_mvc_vcl_hrd_parameters_present_flag;
    uint64_t i, j;

    printf("%*c mvc_vui_parameters_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); PRINT_UINT("vui_mvc_num_ops_minus1");
    vui_mvc_num_ops_minus1 = context->value;
    for (i = 0; i <= vui_mvc_num_ops_minus1; i++) {
        printf("%*c entry %"PRIu64":\n", context->indent * 4, ' ', i);
        context->indent++;

        u(3); PRINT_UINT("vui_mvc_temporal_id");
        ue(); PRINT_UINT("vui_mvc_num_target_output_views_minus1");
        vui_mvc_num_target_output_views_minus1 = context->value;
        for (j = 0; j <= vui_mvc_num_target_output_views_minus1; j++) {
            ue(); PRINT_UINT("vui_mvc_view_id");
        }
        u(1); PRINT_UINT("vui_mvc_timing_info_present_flag");
        if (context->value) {
            context->indent++;
            u(32); PRINT_UINT("vui_mvc_num_units_in_tick");
            u(32); PRINT_UINT("vui_mvc_time_scale");
            u(1); PRINT_UINT("vui_mvc_fixed_frame_rate_flag");
            context->indent--;
        }
        u(1); PRINT_UINT("vui_mvc_nal_hrd_parameters_present_flag");
        vui_mvc_nal_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        u(1); PRINT_UINT("vui_mvc_vcl_hrd_parameters_present_flag");
        vui_mvc_vcl_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        if (vui_mvc_nal_hrd_parameters_present_flag || vui_mvc_vcl_hrd_parameters_present_flag) {
            u(1); PRINT_UINT("vui_mvc_low_delay_hrd_flag");
        }
        u(1); PRINT_UINT("vui_mvc_pic_struct_present_flag");

        context->indent--;
    }

    context->indent++;
    return 1;
}

static int subset_sequence_parameter_set(ParseContext *context)
{
    printf("%*c subset_sequence_parameter_set:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(sequence_parameter_set_data(context));
    if (context->profile_idc == 83 || context->profile_idc == 86) {
        CHK(sequence_parameter_set_svc_extension(context));
        u(1); PRINT_UINT("svc_vui_parameter_present_flag");
        if (context->value) {
            CHK(svc_vui_parameters_extension(context));
        }
    } else if (context->profile_idc == 118 || context->profile_idc == 128) {
        f(1); PRINT_UINT("bit_equal_to_one");
        CHK(sequence_parameter_set_mvc_extension(context));
        u(1); PRINT_UINT("mvc_vui_parameter_present_flag");
        if (context->value) {
            CHK(mvc_vui_parameters_extension(context));
        }
    }
    u(1); PRINT_UINT("additional_extension2_flag");
    if (context->value) {
        while (more_rbsp_data(context)) {
            u(1); PRINT_UINT("additional_extension2_data_flag");
        }
    }
    CHK(rbsp_trailing_bits(context));

    context->indent++;
    return 1;
}

static int nal_unit_header_svc_extension(ParseContext *context)
{
    printf("%*c nal_unit_header_svc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    u(1); PRINT_UINT("idr_flag");
    u(6); PRINT_UINT("priority_id");
    u(1); PRINT_UINT("no_inter_layer_pred_flag");
    u(3); PRINT_UINT("dependency_id");
    u(4); PRINT_UINT("quality_id");
    u(3); PRINT_UINT("temporal_id");
    u(1); PRINT_UINT("use_ref_base_pic_flag");
    u(1); PRINT_UINT("discardable_flag");
    u(1); PRINT_UINT("output_flag");
    u(2); PRINT_UINT("reserved_three_2bits");

    context->indent--;
    return 1;
}

static int nal_unit_header_mvc_extension(ParseContext *context)
{
    printf("%*c nal_unit_header_mvc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    u(1); PRINT_UINT("non_idr_flag");
    u(6); PRINT_UINT("priority_id");
    u(10); PRINT_UINT("view_id");
    u(3); PRINT_UINT("temporal_id");
    u(1); PRINT_UINT("anchor_pic_flag");
    u(1); PRINT_UINT("inter_view_flag");
    u(1); PRINT_UINT("reserved_one_bit");

    context->indent--;
    return 1;
}

static int parse_nal_unit(ParseContext *context)
{
    uint8_t nal_unit_type;
    uint64_t bit_pos;

    printf("NAL: pos=%"PRId64", start=%u, size=%u\n",
           context->data_start_file_pos, context->nal_start, context->nal_size);

    context->indent++;

    f(1); PRINT_UINT("forbidden_zero_bit");
    u(2); PRINT_UINT("nal_ref_idc");
    u(5); print_nal_unit_type(context);
    nal_unit_type = (uint8_t)context->value;

    bit_pos = context->bit_pos;
    if (nal_unit_type == 14 || nal_unit_type == 20) {
        u(1); PRINT_UINT("svc_extension_flag");
        if (context->value) {
            CHK(nal_unit_header_svc_extension(context));
        } else {
            CHK(nal_unit_header_mvc_extension(context));
        }
        bit_pos += 3 * 8;
    }
    CHK(context->bit_pos <= bit_pos);
    context->bit_pos = bit_pos;

    switch (nal_unit_type)
    {
        case 6:
            CHK(sei(context));
            break;
        case 7:
            CHK(sequence_parameter_set(context));
            break;
        case 8:
            CHK(picture_parameter_set(context));
            break;
        case 13:
            CHK(sequence_parameter_set_extension(context));
            break;
        case 15:
            CHK(subset_sequence_parameter_set(context));
            break;
        default:
            break;
    }

    context->indent--;

    return 1;
}



static void print_usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s <filename>\n", cmd);
}

int main(int argc, const char **argv)
{
    const char *filename;
    ParseContext context;
    int cmdln_index;

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

    while (parse_byte_stream_nal_unit(&context)) {
        if (!parse_nal_unit(&context)) {
            fprintf(stderr, "Failed to parse NAL unit\n");
            break;
        }
    }

    deinit_context(&context);


    return 0;
}

