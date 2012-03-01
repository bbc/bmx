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
#include <math.h>


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

    uint64_t value;
    int64_t svalue;
    uint64_t next_value;

    uint64_t chroma_format_idc;

    int indent;
} ParseContext;



static int init_context(ParseContext *context, const char *filename)
{
    memset(context, 0, sizeof(*context));

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


static int next_bits(ParseContext *context, uint8_t num_bits, uint8_t *advance_bits)
{
    assert(num_bits <= 64);

    if (context->bit_pos + num_bits > context->end_bit_pos)
        return 0;

    const unsigned char *byte = context->buffer + context->bit_pos / 8;
    uint32_t num_bytes = ((context->bit_pos % 8) + num_bits + 7) / 8;
    uint8_t skip_bits = 0;

    context->next_value = 0;
    uint32_t i;
    for (i = 0; i < num_bytes; i++) {
        if (*byte == 0x03 && context->bit_pos + i * 8 >= 16 && byte[-1] == 0x00 && byte[-2] == 0x00) {
            skip_bits += 8;
            byte++;
        }
        context->next_value = (context->next_value << 8) | (*byte);
        byte++;
    }

    context->next_value >>= (7 - ((context->bit_pos + num_bits - 1) % 8));
    context->next_value &=  (1UL << num_bits) - 1;

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

    CHK(read_bits(context, leading_zero_bits));
    code_num = (1ULL << leading_zero_bits) - 1 + context->value;

    context->value = code_num;

    return 1;
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

    while (!b && bit_pos > context->bit_pos) {
        b = context->buffer[bit_pos / 8] & (0x80 >> (bit_pos % 8));
        bit_pos--;
    }

    return (bit_pos > context->bit_pos);
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


static int hrd_parameters(ParseContext *context)
{
    uint64_t sched_sel_idx;
    uint64_t cpb_cnt_minus1;

    ue(); PRINT_UINT("cpb_cnt_minus1");
    cpb_cnt_minus1 = context->value;
    u(4); PRINT_UINT("bit_rate_scale");
    u(4); PRINT_UINT("cpb_size_scale");
    context->indent++;
    for (sched_sel_idx = 0; sched_sel_idx < cpb_cnt_minus1; sched_sel_idx++) {
        ue(); PRINT_UINT("bit_rate_value_minus1");
        ue(); PRINT_UINT("cpb_size_value_minus1");
        u(1); PRINT_UINT("cbr_flag");
    }
    context->indent--;
    u(5); PRINT_UINT("initial_cpb_removal_delay_length_minus1");
    u(5); PRINT_UINT("cpb_removal_delay_length_minus1");
    u(5); PRINT_UINT("dpb_removal_delay_length_minus1");
    u(5); PRINT_UINT("time_offset_length");

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
    u(1); PRINT_UINT("pict_struct_present_flag");
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

static int scaling_list(ParseContext *context, uint8_t list_size)
{
    uint8_t last_scale = 8, next_scale = 8;
    int64_t delta_scale;
    uint8_t j;

    for (j = 0; j < list_size; j++) {
        if (next_scale != 0) {
            se(); PRINT_INT("delta_scale");
            delta_scale = context->svalue;
            next_scale = (uint8_t)((last_scale + delta_scale + 256) % 256);
        }
        last_scale = (next_scale == 0 ? last_scale : next_scale);
    }

    return 1;
}

static int sequence_parameter_set(ParseContext *context)
{
    uint8_t profile_idc;
    uint8_t constraint_flags;
    uint64_t pict_order_cnt_type;

    printf("%*c sequence_parameter_set:\n", context->indent * 4, ' ');
    context->indent++;

    u(8); profile_idc = (uint8_t)context->value;
    u(1); constraint_flags = (uint8_t)context->value;
    u(1); constraint_flags |= (uint8_t)context->value << 1;
    u(1); constraint_flags |= (uint8_t)context->value << 2;
    u(1); constraint_flags |= (uint8_t)context->value << 3;
    u(1); constraint_flags |= (uint8_t)context->value << 4;
    u(1); constraint_flags |= (uint8_t)context->value << 5;
    print_profile_and_flags(context, profile_idc, constraint_flags);
    u(2); PRINT_UINT("reserved_zero_2bits");
    u(8); print_level(context, (uint8_t)context->value, constraint_flags);
    ue(); PRINT_UINT("seq_parameter_set_id");

    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 || profile_idc ==  44 ||
        profile_idc ==  83 || profile_idc ==  86 || profile_idc == 118 ||
        profile_idc == 128)
    {
        ue(); print_chroma_format(context, context->value);
        context->chroma_format_idc = context->value;
        if (context->chroma_format_idc == 3) {
            u(1); PRINT_UINT("separate_colour_plane_flag");
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
                        CHK(scaling_list(context, 16));
                    } else {
                        CHK(scaling_list(context, 64));
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

    CHK(rbsp_trailing_bits(context));

    context->indent--;
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
            for (i = 0; i < num_slice_groups_minus1; i++) {
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
            for (i = 0; i < pic_size_in_map_units_minus1; i++) {
                /* note: assuming ue() because spec has u(v) */
                ue(); PRINT_UINT("slice_group_id");
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
                        CHK(scaling_list(context, 16));
                    } else {
                        CHK(scaling_list(context, 64));
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
        case 7:
            CHK(sequence_parameter_set(context));
            break;
        case 8:
            CHK(picture_parameter_set(context));
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

