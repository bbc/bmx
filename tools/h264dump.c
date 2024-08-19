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
#include <stdarg.h>


#define READ_BLOCK_SIZE         (64 * 1024)
#define INIT_BUFFER_SIZE        (1 * 1024 * 1024)
#define MAX_BUFFER_SIZE         (20 * 1024 * 1024)


#define CEIL_DIVISION(a, b)     (((a) + (b) - 1) / (b))

#define DUMP_UINT(name)        dumpf(context, "%*c " name ": %" PRIu64 "\n", context->indent * 4, ' ', context->value)
#define DUMP_INT(name)         dumpf(context, "%*c " name ": %" PRId64 "\n", context->indent * 4, ' ', context->svalue)

#define ARRAY_SIZE(array)       (sizeof(array) / sizeof((array)[0]))

#define CHK(cmd)                                                                    \
    do {                                                                            \
        if (!(cmd)) {                                                               \
            fprintf(stderr, "'%s' check failed at line %d\n", #cmd, __LINE__);      \
            return 0;                                                               \
        }                                                                           \
    } while (0)


#define EXTENDED_SAR            255


typedef enum
{
    CODED_SLICE_NON_IDR_PICT        = 1,
    CODED_SLICE_DATA_PART_A         = 2,
    CODED_SLICE_DATA_PART_B         = 3,
    CODED_SLICE_DATA_PART_C         = 4,
    CODED_SLICE_IDR_PICT            = 5,
    SEI                             = 6,
    SEQUENCE_PARAMETER_SET          = 7,
    PICTURE_PARAMETER_SET           = 8,
    ACCESS_UNIT_DELIMITER           = 9,
    END_OF_SEQUENCE                 = 10,
    END_OF_STREAM                   = 11,
    FILLER                          = 12,
    SEQUENCE_PARAMETER_SET_EXT      = 13,
    PREFIX_NAL_UNIT                 = 14,
    SUBSET_SEQUENCE_PARAMETER_SET   = 15,
    CODED_SLICE_AUX_NO_PART         = 19,
    CODED_SLICE_EXT                 = 20,
} NALUnitType;

typedef enum
{
    P_SLICE     = 0,
    B_SLICE     = 1,
    I_SLICE     = 2,
    SP_SLICE    = 3,
    SI_SLICE    = 4,
    P_SLICE_2   = 5,
    B_SLICE_2   = 6,
    I_SLICE_2   = 7,
    SP_SLICE_2  = 8,
    SI_SLICE_2  = 9,
} SliceType;

#define IS_P(t)     ((t) == P_SLICE  || (t) == P_SLICE_2)
#define IS_B(t)     ((t) == B_SLICE  || (t) == B_SLICE_2)
#define IS_I(t)     ((t) == I_SLICE  || (t) == I_SLICE_2)
#define IS_SP(t)    ((t) == SP_SLICE || (t) == SP_SLICE_2)
#define IS_SI(t)    ((t) == SI_SLICE || (t) == SI_SLICE_2)

typedef struct
{
    uint8_t seq_parameter_set_id;
    uint8_t profile_idc;
    uint8_t chroma_format_idc;
    uint8_t separate_colour_plane_flag;
    uint64_t chroma_array_type;
    uint8_t cpb_dpb_delays_present_flag;
    uint8_t initial_cpb_removal_delay_length_minus1;
    uint8_t cpb_removal_delay_length_minus1;
    uint8_t dpb_output_delay_length_minus1;
    uint8_t pic_struct_present_flag;
    uint8_t time_offset_length;
    uint8_t log2_max_frame_num_minus4;
    uint8_t frame_mbs_only_flag;
    uint64_t pic_order_cnt_type;
    uint8_t log2_max_pic_order_cnt_lsb_minus4;
    uint8_t delta_pic_order_always_zero_flag;
    uint64_t pic_width_in_mbs_minus1;
    uint64_t pic_height_in_map_units_minus1;
    uint64_t cpb_cnt_minus1;
    uint8_t nal_hrd_parameters_present_flag;
    uint8_t vcl_hrd_parameters_present_flag;
} SPS;

typedef struct
{
    uint8_t pic_parameter_set_id;
    uint8_t seq_parameter_set_id;
    uint8_t bottom_field_pic_order_in_frame_present_flag;
    uint8_t redundant_pic_cnt_present_flag;
    uint8_t weighted_pred_flag;
    uint8_t weighted_bipred_idc;
    uint8_t entropy_coding_mode_flag;
    uint8_t deblocking_filter_control_present_flag;
    uint64_t num_slice_groups_minus1;
    uint64_t slice_group_map_type;
    uint64_t slice_group_change_rate_minus1;
    uint64_t num_ref_idx_l0_default_active_minus1;
    uint64_t num_ref_idx_l1_default_active_minus1;
} PPS;

typedef struct PicTimingBuffer PicTimingBuffer;

typedef struct
{
    int indent;

    FILE *file;
    int end_of_read;

    unsigned char *buffer;
    uint32_t buffer_size;

    uint64_t data_start_file_pos;
    uint32_t data_size;
    uint32_t data_pos;

    uint32_t nal_start;
    uint32_t nal_size;
    uint32_t nal_padding;

    uint64_t bit_pos;
    uint64_t end_bit_pos;

    uint64_t emu_prevent_count;

    uint64_t value;
    int64_t svalue;
    uint64_t next_value;

    SPS sps_cache[32];
    PPS pps_cache[256];

    SPS *sps;
    PPS *pps;

    // NAL
    uint8_t nal_ref_idc;
    uint8_t nal_unit_type;

    // Slice NAL
    uint64_t slice_type;
    uint64_t num_ref_idx_l0_active_minus1;
    uint64_t num_ref_idx_l1_active_minus1;

    // delayed pic timing
    PicTimingBuffer *pic_timing_buf;

    // text dump is sent here if pic_timing_buf is non-null
    char *dump_buf;
    size_t dump_buf_alloc_size;
    size_t dump_buf_size;
} ParseContext;

struct PicTimingBuffer
{
    ParseContext context;
    uint64_t payload_type;
    uint64_t payload_size;

    unsigned char *buffer;
    uint32_t buffer_size;
};


static int dump_buffered_pic_timing(ParseContext *context);


static void dumpf(ParseContext *context, const char *format, ...)
{
    va_list ap;

    if (context->pic_timing_buf) {
        // buffer the text dump
        int size;

        va_start(ap, format);
        size = vsnprintf(&context->dump_buf[context->dump_buf_size],
                         context->dump_buf_alloc_size - context->dump_buf_size,
                         format, ap);
        va_end(ap);
        if (size < 0)
            return;

        if (size + 1 <= (int)(context->dump_buf_alloc_size - context->dump_buf_size)) { // "+ 1" for null terminnator
            context->dump_buf_size += size;
        } else {
            size_t new_alloc_size = context->dump_buf_size + size + 8192;
            context->dump_buf = realloc(context->dump_buf, new_alloc_size);
            if (!context->dump_buf) {
                context->dump_buf_alloc_size = 0;
                context->dump_buf_size = 0;
                return;
            }
            context->dump_buf_alloc_size = new_alloc_size;

            va_start(ap, format);
            size = vsnprintf(&context->dump_buf[context->dump_buf_size],
                             context->dump_buf_alloc_size - context->dump_buf_size,
                             format, ap);
            va_end(ap);
            if (size > 0)
                context->dump_buf_size += size;
        }
    } else {
        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);
    }
}

static int init_context(ParseContext *context, const char *filename)
{
    memset(context, 0, sizeof(*context));
    context->sps_cache[0].seq_parameter_set_id = 255;
    context->pps_cache[0].pic_parameter_set_id = 255;

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
    if (context->pic_timing_buf) {
        free(context->pic_timing_buf->buffer);
        free(context->pic_timing_buf);
    }
    free(context->dump_buf);
    free(context->buffer);
}

static void set_and_init_sps(ParseContext *context, uint8_t seq_parameter_set_id)
{
    context->sps = &context->sps_cache[seq_parameter_set_id];
    memset(context->sps, 0, sizeof(*context->sps));
    context->sps->seq_parameter_set_id = seq_parameter_set_id;
    context->sps->initial_cpb_removal_delay_length_minus1 = 23;
    context->sps->cpb_removal_delay_length_minus1 = 23;
    context->sps->dpb_output_delay_length_minus1 = 23;
    context->sps->time_offset_length = 24;
    context->sps->chroma_format_idc = 1;
    context->sps->chroma_array_type = 1;
}

static void set_and_init_pps(ParseContext *context, uint8_t pic_parameter_set_id)
{
    context->pps = &context->pps_cache[pic_parameter_set_id];
    memset(context->pps, 0, sizeof(*context->pps));
    context->pps->pic_parameter_set_id = pic_parameter_set_id;
}

static int set_sps(ParseContext *context, uint8_t seq_parameter_set_id)
{
    if (seq_parameter_set_id >= ARRAY_SIZE(context->sps_cache) ||
        context->sps_cache[seq_parameter_set_id].seq_parameter_set_id != seq_parameter_set_id)
    {
        return 0;
    }

    context->sps = &context->sps_cache[seq_parameter_set_id];
    return 1;
}

static int set_pps(ParseContext *context, uint8_t pic_parameter_set_id)
{
    if (context->pps_cache[pic_parameter_set_id].pic_parameter_set_id != pic_parameter_set_id)
        return 0;

    if (!set_sps(context, context->pps_cache[pic_parameter_set_id].seq_parameter_set_id))
        return 0;

    context->pps = &context->pps_cache[pic_parameter_set_id];
    return 1;
}

static int read_next_block(ParseContext *context)
{
    size_t num_read;

    if (context->end_of_read)
        return 0;

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
    context->data_size   += (uint32_t)num_read;
    context->end_of_read  = (num_read == 0);

    return !context->end_of_read;
}

static int read_byte(ParseContext *context, unsigned char *byte)
{
    if (context->data_pos >= context->data_size) {
        if (!read_next_block(context))
            return 0;
    }

    *byte = context->buffer[context->data_pos];
    context->data_pos++;

    return 1;
}

static int peek_byte(ParseContext *context, unsigned char *byte)
{
    if (!read_byte(context, byte))
        return 0;

    context->data_pos--;

    return 1;
}


static void shift_next_nal_data(ParseContext *context)
{
    uint32_t rem_size;

    rem_size = context->data_size - (context->nal_start + context->nal_size);
    if (rem_size > 0)
        memmove(context->buffer, &context->buffer[context->nal_start + context->nal_size], rem_size);
    context->data_start_file_pos += context->data_size - rem_size;
    context->data_size            = rem_size;
    context->data_pos             = 0;
}

static int parse_byte_stream_nal_unit(ParseContext *context)
{
    uint8_t byte;
    uint32_t state;
    uint8_t nal_unit_type;
    uint32_t num_nal_unit_parse_bytes;

    shift_next_nal_data(context);

    // search for start of nal unit
    state = 0xffffffff;
    while ((state & 0x00ffffff) != 0x000001) {
        if (!read_byte(context, &byte))
            return 0;
        state = (state << 8) | byte;
    }
    if (state != 0x00000001) {
        CHK(peek_byte(context, &byte));
        nal_unit_type = byte & 0x1f;
        /* TODO: this is incomplete. Need to get the last VCL unit. See section 7.4.1.2.3  and B.1.2 */
        if (context->data_start_file_pos == 0 || nal_unit_type == 7 || nal_unit_type == 8)
            dumpf(context, "Warning: missing zero_byte before start_code_prefix_one_3bytes\n");
    }
    context->nal_start = context->data_pos;
    context->nal_size = 0;
    context->nal_padding = 0;

    // search for start of next nal unit or zero bytes to give the number of bytes that need to be parsed
    state = 0xffffffff;
    while ((state & 0x00ffffff) != 0x000001 && (state & 0x00ffffff) != 0x000000) {
        if (!read_byte(context, &byte))
            break;
        state = (state << 8) | byte;
    }
    num_nal_unit_parse_bytes = context->data_pos - context->nal_start;
    if ((state & 0x00ffffff) == 0x000001 || (state & 0x00ffffff) == 0x000000) {
        num_nal_unit_parse_bytes -= 3;
        if ((state & 0x00ffffff) == 0x000000) {
            context->nal_padding += 3;

            /* add a byte to the NAL unit bytes to be parsed as a workaround for the missing sequence
               parameter set stop bit in Avid Transfer Manager AVCI files. The last couple of properties
               in the SPS are zero and the last byte is wrongly assumed to be padding because of the
               missing stop bit. This results in not be enough bits being available and parsing fails */
            num_nal_unit_parse_bytes++;
        }
    }

    // search past the nal padding to the start of the next nal unit
    if ((state & 0x00ffffff) == 0x000000) {
        state = 0;
        while ((state & 0x00ffffff) != 0x000001) {
            if (!read_byte(context, &byte))
                break;
            if (!state && !(byte == 0x00 || byte == 0x01))
                dumpf(context, "Warning: zero bytes not followed by start code prefix. Possibly missing emulation_prevention_three_byte\n");
            state = (state << 8) | byte;
            context->nal_padding++;
        }
        if ((state & 0x00ffffff) == 0x000001)
            context->nal_padding -= 3;
    }

    context->nal_size = context->data_pos - context->nal_start;
    if ((state & 0x00ffffff) == 0x000001) {
        context->nal_size -= 3;
        if (peek_byte(context, &byte)) {
            nal_unit_type = byte & 0x1f;
            /* TODO: this is incomplete. Need to get the last VCL unit. See section 7.4.1.2.3 and B.1.2 */
            if (nal_unit_type == 7 || nal_unit_type == 8 || nal_unit_type == 9) {
                if (state != 0x00000001) {
                    dumpf(context, "Warning: missing zero_byte before start_code_prefix_one_3bytes\n");
                } else {
                    context->nal_size--;
                    context->nal_padding--;
                }
            }
        }
    }

    context->bit_pos     = context->nal_start * 8;
    context->end_bit_pos = context->nal_start * 8 + num_nal_unit_parse_bytes * 8;

    return 1;
}

static int parse_isom_nal_unit(ParseContext *context, unsigned int isom_llen)
{
    uint8_t byte;
    uint32_t nal_size = 0;
    unsigned int i;

    shift_next_nal_data(context);

    for (i = 0; i < isom_llen; i++) {
        if (!read_byte(context, &byte))
            return 0;
        nal_size = (nal_size << 8) | byte;
    }

    while (context->data_size < context->data_pos + nal_size) {
        if (!read_next_block(context))
            return 0;
    }

    context->nal_start   = context->data_pos;
    context->nal_size    = nal_size;
    context->bit_pos     = context->nal_start * 8;
    context->end_bit_pos = context->bit_pos + context->nal_size * 8;

    return 1;
}

static int byte_aligned(ParseContext *context)
{
    return !(context->bit_pos & 7);
}

static int next_bits(ParseContext *context, uint8_t num_bits, uint8_t *advance_bits)
{
    const unsigned char *byte;
    uint8_t min_consume_bits;
    int16_t consumed_bits = 0;
    uint8_t skip_bits = 0;

    CHK(num_bits > 0);
    CHK(num_bits <= 64);

    if (context->bit_pos + num_bits > context->end_bit_pos)
        return 0;

    byte = &context->buffer[context->bit_pos >> 3];
    min_consume_bits = (uint8_t)((context->bit_pos & 0x07) + num_bits);
    context->next_value = 0;
    while (consumed_bits < min_consume_bits) {
        if (*byte == 0x03 && context->bit_pos + consumed_bits >= 16 && byte[-1] == 0x00 && byte[-2] == 0x00) {
            skip_bits += 8;
            context->emu_prevent_count++;
            byte++;
        }
        context->next_value = (context->next_value << 8) | (*byte);
        byte++;
        consumed_bits += 8;
    }

    context->next_value >>= consumed_bits - min_consume_bits;
    if (consumed_bits > 64) {/* restore first byte bits that have been shifted out */
        context->next_value |= ((uint64_t)context->buffer[context->bit_pos >> 3]) <<
                                    (64 - (consumed_bits - min_consume_bits));
    }
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
        dumpf(context, "Warning: Exp-Golumb size >= %d not supported\n", leading_zero_bits);
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

#define ue_m(max)    CHK(_ue_m(context, max))
static int _ue_m(ParseContext *context, uint64_t max)
{
    return exp_golumb(context) && context->value <= max;
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

    return (b && bit_pos >= context->bit_pos);
}

static int rbsp_trailing_bits(ParseContext *context)
{
    /* found that Avid AVCI 1080i content was missing a stop bit in the sequence parameter set and therefore only
       printing a message when this function fails */

    int valid;
    unsigned int zero_bit_count = 0;

    f(1); valid = (context->value != 0);
    while (valid && (context->bit_pos % 8) != 0) {
        f(1); valid = (context->value == 0);
        zero_bit_count++;
    }

    if (valid) {
        dumpf(context, "%*c rbsp_stop_one_bit: 1\n", context->indent * 4, ' ');
        if (zero_bit_count > 0)
            dumpf(context, "%*c rbsp_alignment_zero_bit: 0 x %u\n", context->indent * 4, ' ', zero_bit_count);
    } else {
        dumpf(context, "Warning: invalid rbsp_trailing_bits\n");
    }

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

    uint8_t nal_unit_type = (uint8_t)(context->value & 31);

    dumpf(context, "%*c nal_unit_type: %u (%s)\n", context->indent * 4, ' ', nal_unit_type,
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
        {100,   0x08,   "High Intra"},
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
    for (i = 0; i < ARRAY_SIZE(PROFILE_NAMES); i++) {
        if (profile_idc == PROFILE_NAMES[i].profile_idc &&
            (PROFILE_NAMES[i].flags == 0 || (flags & PROFILE_NAMES[i].flags)))
        {
            profile_str = PROFILE_NAMES[i].profile_str;
            break;
        }
    }

    dumpf(context, "%*c profile_idc: %u (%s)\n", context->indent * 4, ' ', profile_idc, profile_str);

    dumpf(context, "%*c constraint_set0_flag: %u\n", context->indent * 4, ' ', flags & 1);
    dumpf(context, "%*c constraint_set1_flag: %u\n", context->indent * 4, ' ', (flags >> 1) & 1);
    dumpf(context, "%*c constraint_set2_flag: %u\n", context->indent * 4, ' ', (flags >> 2) & 1);
    dumpf(context, "%*c constraint_set3_flag: %u\n", context->indent * 4, ' ', (flags >> 3) & 1);
    dumpf(context, "%*c constraint_set4_flag: %u\n", context->indent * 4, ' ', (flags >> 4) & 1);
    dumpf(context, "%*c constraint_set5_flag: %u\n", context->indent * 4, ' ', (flags >> 5) & 1);
}

static void print_level(ParseContext *context, uint8_t level_idc, uint8_t flags)
{
    if (level_idc == 11 && (flags & 0x08))
        dumpf(context, "%*c level_idc: %u (1b)\n", context->indent * 4, ' ', level_idc);
    else
        dumpf(context, "%*c level_idc: %u (%.1f)\n", context->indent * 4, ' ', level_idc, level_idc / 10.0);
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

    if (chroma_format_idc < ARRAY_SIZE(CHROMA_FORMAT_STRINGS)) {
        dumpf(context, "%*c chroma_format_idc: %u (%s)\n", context->indent * 4, ' ', chroma_format_idc,
              CHROMA_FORMAT_STRINGS[chroma_format_idc]);
    } else {
        dumpf(context, "%*c chroma_format_idc: %u (unknown)\n", context->indent * 4, ' ', chroma_format_idc);
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

    if (aspect_ratio_idc < ARRAY_SIZE(SAMPLE_ASPECT_RATIO_STRINGS)) {
        dumpf(context, "%*c aspect_ratio_idc: %u (%s)\n", context->indent * 4, ' ', aspect_ratio_idc,
              SAMPLE_ASPECT_RATIO_STRINGS[aspect_ratio_idc]);
    } else if (aspect_ratio_idc == EXTENDED_SAR) {
        dumpf(context, "%*c aspect_ratio_idc: %u (Extended_SAR)\n", context->indent * 4, ' ', aspect_ratio_idc);
    } else {
        dumpf(context, "%*c aspect_ratio_idc: %u (Reserved)\n", context->indent * 4, ' ', aspect_ratio_idc);
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

    dumpf(context, "%*c video_format: %u (%s)\n", context->indent * 4, ' ', video_format,
          VIDEO_FORMAT_STRINGS[video_format & 0x07]);
}

static void print_pic_struct(ParseContext *context, uint8_t pic_struct)
{
    static const char *PIC_STRUCT_STRINGS[] =
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

    if (pic_struct < ARRAY_SIZE(PIC_STRUCT_STRINGS)) {
        dumpf(context, "%*c pic_struct: %u (%s)\n", context->indent * 4, ' ', pic_struct,
              PIC_STRUCT_STRINGS[pic_struct]);
    } else {
        dumpf(context, "%*c pic_struct: %u (reserved)\n", context->indent * 4, ' ', pic_struct);
    }
}

static void print_primary_pic_type(ParseContext *context, uint8_t type)
{
    static const char *PRIMARY_PIC_TYPES[] =
    {
        "I",
        "P, I",
        "P, B, I",
        "SI",
        "SP, SI",
        "I, SI",
        "P, I, SP, SI",
        "P, B, I, SP, SI"
    };

    dumpf(context, "%*c primary_pic_type: %u (%s)\n", context->indent * 4, ' ', type,
          PRIMARY_PIC_TYPES[type]);
}

static void print_slice_type(ParseContext *context, uint64_t type)
{
    static const char *SLICE_TYPES[] =
    {
        "P",
        "B",
        "I",
        "SP",
        "SI",
        "P",
        "B",
        "I",
        "SP",
        "SI",
    };

    if (type < ARRAY_SIZE(SLICE_TYPES)) {
        dumpf(context, "%*c slice_type: %" PRIu64 " (%s)\n", context->indent * 4, ' ', type,
              SLICE_TYPES[type]);
    } else {
        dumpf(context, "%*c slice_type: %" PRIu64 " (unknown)\n", context->indent * 4, ' ', type);
    }
}

static void print_scaling_list_flag_and_index(ParseContext *context, const char *flag_name, uint8_t flag, uint8_t index)
{
    static const char *SCALING_LIST_NAMES[] =
    {
        "SI_4x4_Intra_Y",
        "SI_4x4_Intra_Cb",
        "SI_4x4_Intra_Cr",
        "SI_4x4_Inter_Y",
        "SI_4x4_Inter_Cb",
        "SI_4x4_Inter_Cr",
        "SI_8x8_Intra_Y",
        "SI_8x8_Inter_Y",
        "SI_8x8_Intra_Cb",
        "SI_8x8_Inter_Cb",
        "SI_8x8_Intra_Cr",
        "SI_8x8_Inter_Cr",
    };

    dumpf(context, "%*c %s[%u]: %u (%s)\n", context->indent * 4, ' ', flag_name, index, flag, SCALING_LIST_NAMES[index]);
}

static void print_uuid(ParseContext *context, uint64_t uuid_high, uint64_t uuid_low)
{
    int i;

    dumpf(context, "%*c uuid_iso_iec11578: ", context->indent * 4, ' ');
    for (i = 0; i < 4; i++)
        dumpf(context, "%02x", (unsigned char)((uuid_high >> ((7 - i) * 8)) & 0xff));
    dumpf(context, "-");
    for (; i < 6; i++)
        dumpf(context, "%02x", (unsigned char)((uuid_high >> ((7 - i) * 8)) & 0xff));
    dumpf(context, "-");
    for (; i < 8; i++)
        dumpf(context, "%02x", (unsigned char)((uuid_high >> ((7 - i) * 8)) & 0xff));
    dumpf(context, "-");
    for (i = 0; i < 2; i++)
        dumpf(context, "%02x", (unsigned char)((uuid_low >> ((7 - i) * 8)) & 0xff));
    dumpf(context, "-");
    for (; i < 8; i++)
        dumpf(context, "%02x", (unsigned char)((uuid_low >> ((7 - i) * 8)) & 0xff));
    dumpf(context, "\n");
}

static void print_bytes_line(ParseContext *context, uint64_t index, uint8_t *line, size_t num_values, size_t line_size)
{
    size_t i;

    dumpf(context, "%*c %06" PRIx64 " ", context->indent * 4, ' ', index);
    for (i = 0; i < num_values; i++)
        dumpf(context, " %02x", line[i]);
    for (; i < line_size; i++)
        dumpf(context, "   ");
    dumpf(context, "  |");
    for (i = 0; i < num_values; i++) {
        if (isprint(line[i]))
            dumpf(context, "%c", line[i]);
        else
            dumpf(context, ".");
    }
    dumpf(context, "|\n");
}

static int read_and_print_bytes(ParseContext *context, uint64_t num_bytes)
{
    uint64_t i;
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

    CHK(context->sps);

    ue(); DUMP_UINT("cpb_cnt_minus1");
    context->sps->cpb_cnt_minus1 = context->value;
    u(4); DUMP_UINT("bit_rate_scale");
    u(4); DUMP_UINT("cpb_size_scale");
    context->indent++;
    for (sched_sel_idx = 0; sched_sel_idx <= context->sps->cpb_cnt_minus1; sched_sel_idx++) {
        ue(); DUMP_UINT("bit_rate_value_minus1");
        ue(); DUMP_UINT("cpb_size_value_minus1");
        u(1); DUMP_UINT("cbr_flag");
    }
    context->indent--;
    u(5); DUMP_UINT("initial_cpb_removal_delay_length_minus1");
    context->sps->initial_cpb_removal_delay_length_minus1 = (uint8_t)context->value;
    u(5); DUMP_UINT("cpb_removal_delay_length_minus1");
    context->sps->cpb_removal_delay_length_minus1 = (uint8_t)context->value;
    u(5); DUMP_UINT("dpb_output_delay_length_minus1");
    context->sps->dpb_output_delay_length_minus1 = (uint8_t)context->value;
    u(5); DUMP_UINT("time_offset_length");
    context->sps->time_offset_length = (uint8_t)context->value;

    return 1;
}

static int vui_parameters(ParseContext *context)
{
    CHK(context->sps);

    u(1); DUMP_UINT("aspect_ratio_present_flag");
    if (context->value) {
        context->indent++;
        u(8); print_aspect_ratio(context, (uint8_t)context->value);
        if (context->value == EXTENDED_SAR) {
            context->indent++;
            u(16); DUMP_UINT("sar_width");
            u(16); DUMP_UINT("sar_height");
            context->indent--;
        }
        context->indent--;
    }
    u(1); DUMP_UINT("overscan_info_present_flag");
    if (context->value) {
        context->indent++;
        u(1); DUMP_UINT("overscan_appropriate_flag");
        context->indent--;
    }
    u(1); DUMP_UINT("video_signal_type_present_flag");
    if (context->value) {
        context->indent++;
        u(3); print_video_format(context, (uint8_t)context->value);
        u(1); DUMP_UINT("video_full_range_flag");
        u(1); DUMP_UINT("colour_description_present_flag");
        if (context->value) {
            context->indent++;
            u(8); DUMP_UINT("colour_primaries");
            u(8); DUMP_UINT("transfer_characteristics");
            u(8); DUMP_UINT("matrix_coefficients");
            context->indent--;
        }
        context->indent--;
    }
    u(1); DUMP_UINT("chroma_loc_info_present_flag");
    if (context->value) {
        context->indent++;
        ue_m(5); DUMP_UINT("chroma_sample_loc_type_top_field");
        ue_m(5); DUMP_UINT("chroma_sample_loc_type_bottom_field");
        context->indent--;
    }
    u(1); DUMP_UINT("timing_info_present_flag");
    if (context->value) {
        context->indent++;
        u(32); DUMP_UINT("num_units_in_tick");
        u(32); DUMP_UINT("time_scale");
        u(1); DUMP_UINT("fixed_frame_rate_flag");
        context->indent--;
    }
    u(1); DUMP_UINT("nal_hrd_parameters_present_flag");
    context->sps->nal_hrd_parameters_present_flag = (uint8_t)context->value;
    if (context->value) {
        context->indent++;
        CHK(hrd_parameters(context));
        context->indent--;
    }
    u(1); DUMP_UINT("vcl_hrd_parameters_present_flag");
    context->sps->vcl_hrd_parameters_present_flag = (uint8_t)context->value;
    if (context->value) {
        context->indent++;
        CHK(hrd_parameters(context));
        context->indent--;
    }
    if (context->sps->nal_hrd_parameters_present_flag || context->sps->vcl_hrd_parameters_present_flag) {
        u(1); DUMP_UINT("low_delay_hrd_flag");
    }
    context->sps->cpb_dpb_delays_present_flag = context->sps->nal_hrd_parameters_present_flag ||
                                                context->sps->vcl_hrd_parameters_present_flag;
    u(1); DUMP_UINT("pic_struct_present_flag");
    context->sps->pic_struct_present_flag = (uint8_t)context->value;
    u(1); DUMP_UINT("bitstream_restriction_flag");
    if (context->value) {
        u(1); DUMP_UINT("motion_vectors_over_pic_boundaries_flag");
        ue(); DUMP_UINT("max_bytes_per_pic_denom");
        ue(); DUMP_UINT("max_bytes_per_mb_denom");
        ue(); DUMP_UINT("log2_max_mv_length_horizontal");
        ue(); DUMP_UINT("log2_max_mv_length_vertical");
        ue(); DUMP_UINT("num_reorder_frames");
        ue(); DUMP_UINT("max_dec_frame_buffering");
    }

    return 1;
}

static int scaling_list(ParseContext *context, int is_8x8)
{
    static const uint8_t dezigzag_8x8[64] =
    {
         0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };
    static const uint8_t dezigzag_4x4[16] =
    {
        0,  1,  4,  8,
        5,  2,  3,  6,
        9, 12, 13, 10,
        7, 11, 14, 15
    };

    uint8_t matrix[64];
    const uint8_t *dezigzag = (is_8x8 ? dezigzag_8x8 : dezigzag_4x4);
    uint8_t last_scale = 8, next_scale = 8;
    int dim = (is_8x8 ? 8 : 4);
    int size = dim * dim;
    int i;

    for (i = 0; i < size; i++) {
        if (next_scale != 0) {
            se();
            next_scale = (uint8_t)((last_scale + context->svalue + 256) % 256);
        }
        last_scale = (next_scale == 0 ? last_scale : next_scale);
        matrix[dezigzag[i]] = last_scale;
    }

    for (i = 0; i < size; i++) {
        if (i % dim == 0) {
            if (i != 0)
                dumpf(context, "\n");
            dumpf(context, "%*c", context->indent * 4, ' ');
        }
        dumpf(context, " %02x", matrix[i]);
    }
    dumpf(context, "\n");

    return 1;
}

static int ref_pic_list_mvc_modification(ParseContext *context)
{
    uint64_t modification_of_pic_nums_idc;

    dumpf(context, "%*c ref_pic_list_mvc_modification:\n", context->indent * 4, ' ');
    context->indent++;

    if (context->slice_type % 5 != 2 && context->slice_type % 5 != 4) {
        u(1); DUMP_UINT("ref_pic_list_modification_flag_l0");
        if (context->value) {
            do {
                ue(); DUMP_UINT("modification_of_pic_nums_idc");
                modification_of_pic_nums_idc = context->value;
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                    ue(); DUMP_UINT("abs_diff_pic_num_minus1");
                } else if (modification_of_pic_nums_idc == 2) {
                    ue(); DUMP_UINT("long_term_pic_num");
                } else if (modification_of_pic_nums_idc == 4 || modification_of_pic_nums_idc == 5) {
                    ue(); DUMP_UINT("abs_diff_view_idx_minus1");
                }
            } while (modification_of_pic_nums_idc != 3);
        }
    }
    if (context->slice_type % 5 == 1) {
        u(1); DUMP_UINT("ref_pic_list_modification_flag_l1");
        if (context->value) {
            do {
                ue(); DUMP_UINT("modification_of_pic_nums_idc");
                modification_of_pic_nums_idc = context->value;
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                    ue(); DUMP_UINT("abs_diff_pic_num_minus1");
                } else if (modification_of_pic_nums_idc == 2) {
                    ue(); DUMP_UINT("long_term_pic_num");
                } else if (modification_of_pic_nums_idc == 4 || modification_of_pic_nums_idc == 5) {
                    ue(); DUMP_UINT("abs_diff_view_idx_minus1");
                }
            } while (modification_of_pic_nums_idc != 3);
        }
    }

    context->indent--;
    return 1;
}

static int ref_pic_list_modification(ParseContext *context)
{
    uint64_t modification_of_pic_nums_idc;

    dumpf(context, "%*c ref_pic_list_modification:\n", context->indent * 4, ' ');
    context->indent++;

    if (context->slice_type % 5 != 2 && context->slice_type % 5 != 4) {
        u(1); DUMP_UINT("ref_pic_list_modification_flag_l0");
        if (context->value) {
            do {
                ue(); DUMP_UINT("modification_of_pic_nums_idc");
                modification_of_pic_nums_idc = context->value;
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                    ue(); DUMP_UINT("abs_diff_pic_num_minus1");
                } else if (modification_of_pic_nums_idc == 2) {
                    ue(); DUMP_UINT("long_term_pic_num");
                }
            } while (modification_of_pic_nums_idc != 3);
        }
    }
    if (context->slice_type % 5 == 1) {
        u(1); DUMP_UINT("ref_pic_list_modification_flag_l1");
        if (context->value) {
            do {
                ue(); DUMP_UINT("modification_of_pic_nums_idc");
                modification_of_pic_nums_idc = context->value;
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                    ue(); DUMP_UINT("abs_diff_pic_num_minus1");
                } else if (modification_of_pic_nums_idc == 2) {
                    ue(); DUMP_UINT("long_term_pic_num");
                }
            } while (modification_of_pic_nums_idc != 3);
        }
    }

    context->indent--;
    return 1;
}

static int pred_weight_table(ParseContext *context)
{
    uint64_t i;

    dumpf(context, "%*c pred_weight_table:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(context->sps);

    ue(); DUMP_UINT("luma_log2_weight_denom");
    if (context->sps->chroma_array_type != 0) {
        ue(); DUMP_UINT("chroma_log2_weight_denom");
    }
    for (i = 0; i <= context->num_ref_idx_l0_active_minus1; i++) {
        u(1); DUMP_UINT("luma_weight_l0_flag");
        if (context->value) {
            context->indent++;
            se(); dumpf(context, "%*c luma_weight_l0[%" PRIu64 "]: %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
            se(); dumpf(context, "%*c luma_offset_l0[%" PRIu64 "]: %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
            context->indent--;
        }
        if (context->sps->chroma_array_type != 0) {
            u(1); DUMP_UINT("chroma_weight_l0_flag");
            if (context->value) {
                int j;
                context->indent++;
                for (j = 0; j < 2; j++) {
                    se(); dumpf(context, "%*c chroma_weight_l0[%" PRIu64 "][%d]: %" PRId64 "\n", context->indent * 4, ' ', i, j, context->value);
                    se(); dumpf(context, "%*c chroma_offset_l0[%" PRIu64 "][%d]: %" PRId64 "\n", context->indent * 4, ' ', i, j, context->value);
                }
                context->indent--;
            }
        }
    }
    if (context->slice_type % 5 == 1) {
        for (i = 0; i <= context->num_ref_idx_l1_active_minus1; i++) {
            u(1); DUMP_UINT("luma_weight_l1_flag");
            if (context->value) {
                context->indent++;
                se(); dumpf(context, "%*c luma_weight_l1[%" PRIu64 "]: %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
                se(); dumpf(context, "%*c luma_offset_l1[%" PRIu64 "]: %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
                context->indent--;
            }
            if (context->sps->chroma_array_type != 0) {
                u(1); DUMP_UINT("chroma_weight_l1_flag");
                if (context->value) {
                    int j;
                    context->indent++;
                    for (j = 0; j < 2; j++) {
                        se(); dumpf(context, "%*c chroma_weight_l1[%" PRIu64 "][%d]: %" PRId64 "\n", context->indent * 4, ' ', i, j, context->value);
                        se(); dumpf(context, "%*c chroma_offset_l1[%" PRIu64 "][%d]: %" PRId64 "\n", context->indent * 4, ' ', i, j, context->value);
                    }
                    context->indent--;
                }
            }
        }
    }

    context->indent--;
    return 1;
}

static int dec_ref_pic_marking(ParseContext *context)
{
    uint8_t idr_pic_flag = (context->nal_unit_type == 5 ? 1 : 0);

    dumpf(context, "%*c dec_ref_pic_marking:\n", context->indent * 4, ' ');
    context->indent++;

    if (idr_pic_flag) {
        u(1); DUMP_UINT("no_output_of_prior_pics_flag");
        u(1); DUMP_UINT("long_term_reference_flag");
    } else {
        u(1); DUMP_UINT("adaptive_ref_pic_marking_mode_flag");
        if (context->value) {
            uint64_t memory_management_control_operation;
            do {
                ue(); DUMP_UINT("memory_management_control_operation");
                memory_management_control_operation = context->value;
                if (memory_management_control_operation == 1 ||
                    memory_management_control_operation == 3)
                {
                    ue(); DUMP_UINT("difference_of_pic_nums_minus1");
                }
                if (memory_management_control_operation == 2)
                {
                    ue(); DUMP_UINT("long_term_pic_num");
                }
                if (memory_management_control_operation == 3 ||
                    memory_management_control_operation == 6)
                {
                    ue(); DUMP_UINT("long_term_frame_idx");
                }
                if (memory_management_control_operation == 4)
                {
                    ue(); DUMP_UINT("max_long_term_frame_idx_plus1");
                }
            } while (memory_management_control_operation != 0);
        }
    }

    context->indent--;
    return 1;
}

static int slice_header(ParseContext *context)
{
    uint8_t field_pic_flag = 0;
    uint8_t idr_pic_flag = (context->nal_unit_type == 5 ? 1 : 0);
    uint64_t redundant_pic_cnt = 0;

    dumpf(context, "%*c slice_header:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); DUMP_UINT("first_mb_in_slice");
    ue(); print_slice_type(context, context->value);
    context->slice_type = context->value;
    ue_m(255); DUMP_UINT("pic_parameter_set_id");
    CHK(set_pps(context, (uint8_t)context->value));

    context->num_ref_idx_l0_active_minus1 = context->pps->num_ref_idx_l0_default_active_minus1;
    context->num_ref_idx_l1_active_minus1 = context->pps->num_ref_idx_l1_default_active_minus1;

    if (context->sps->separate_colour_plane_flag == 1) {
        u(2); DUMP_UINT("colour_plane_id");
    }
    u(context->sps->log2_max_frame_num_minus4 + 4); DUMP_UINT("frame_num");
    if (!context->sps->frame_mbs_only_flag) {
        u(1); DUMP_UINT("field_pic_flag");
        field_pic_flag = (uint8_t)context->value;
        if (context->value) {
            u(1); DUMP_UINT("bottom_field_flag");
        }
    }
    if (idr_pic_flag) {
        ue(); DUMP_UINT("idr_pic_id");
    }
    if (context->sps->pic_order_cnt_type == 0) {
        u(context->sps->log2_max_pic_order_cnt_lsb_minus4 + 4); DUMP_UINT("pic_order_cnt_lsb");
        if (context->pps->bottom_field_pic_order_in_frame_present_flag && !field_pic_flag) {
            se(); DUMP_INT("delta_pic_order_cnt_bottom");
        }
    }
    if (context->sps->pic_order_cnt_type == 1 && !context->sps->delta_pic_order_always_zero_flag) {
        se(); DUMP_INT("delta_pic_order_cnt[0]");
        if (context->pps->bottom_field_pic_order_in_frame_present_flag && !field_pic_flag) {
            se(); DUMP_INT("delta_pic_order_cnt[1]");
        }
    }
    if (context->pps->redundant_pic_cnt_present_flag) {
        ue(); DUMP_UINT("redundant_pic_cnt");
        redundant_pic_cnt = context->value;
    }
    if (IS_B(context->slice_type)) {
        u(1); DUMP_UINT("direct_spatial_mv_pred_flag");
    }
    if (IS_P(context->slice_type) || IS_SP(context->slice_type) || IS_B(context->slice_type)) {
        u(1); DUMP_UINT("num_ref_idx_active_override_flag");
        if (context->value) {
            ue(); DUMP_UINT("num_ref_idx_l0_active_minus1");
            context->num_ref_idx_l0_active_minus1 = context->value;
            if (IS_B(context->slice_type)) {
                ue(); DUMP_UINT("num_ref_idx_l1_active_minus1");
                context->num_ref_idx_l1_active_minus1 = context->value;
            }
        }
    }
    if (context->nal_unit_type == 20) {
        CHK(ref_pic_list_mvc_modification(context));
    } else {
        CHK(ref_pic_list_modification(context));
    }
    if ((context->pps->weighted_pred_flag && (IS_P(context->slice_type) || IS_SP(context->slice_type))) ||
        (context->pps->weighted_bipred_idc == 1 && IS_B(context->slice_type)))
    {
        CHK(pred_weight_table(context));
    }
    if (context->nal_ref_idc != 0) {
        CHK(dec_ref_pic_marking(context));
    }
    if (context->pps->entropy_coding_mode_flag && !IS_I(context->slice_type) && !IS_SI(context->slice_type)) {
        ue(); DUMP_UINT("cabac_init_idc");
    }
    se(); DUMP_INT("slice_qp_delta");
    if (IS_SP(context->slice_type) || IS_SI(context->slice_type)) {
        if (IS_SP(context->slice_type)) {
            u(1); DUMP_UINT("sp_for_switch_flag");
        }
        se(); DUMP_INT("slice_qs_delta");
    }
    if (context->pps->deblocking_filter_control_present_flag) {
        ue(); DUMP_UINT("disable_deblocking_filter_idc");
        if (context->value != 1) {
            se(); DUMP_INT("slice_alpha_c0_offset_div2");
            se(); DUMP_INT("slice_beta_offset_div2");
        }
    }
    if (context->pps->num_slice_groups_minus1 > 0 &&
        context->pps->slice_group_map_type >= 3 && context->pps->slice_group_map_type <= 5)
    {
        uint64_t pic_size_in_map_units = (context->sps->pic_width_in_mbs_minus1 + 1) *
                                            (context->sps->pic_height_in_map_units_minus1 + 1);
        uint8_t bit_count = get_bits_required(pic_size_in_map_units / (context->pps->slice_group_change_rate_minus1 + 1) + 1);
        u(bit_count); DUMP_UINT("slice_group_change_cycle");
    }

    context->indent--;

    // parse and dump buffered pic timing SEI and dump text data that followed that
    if (context->pic_timing_buf && redundant_pic_cnt == 0)
        CHK(dump_buffered_pic_timing(context));

    return 1;
}

static int slice_data(ParseContext *context)
{
    dumpf(context, "%*c slice_data:\n", context->indent * 4, ' ');
    context->indent++;

    dumpf(context, "%*c ...\n", context->indent * 4, ' ');

    context->indent--;
    return 1;
}

static int slice_layer_without_partitioning(ParseContext *context)
{
    dumpf(context, "%*c slice_layer_without_partitioning:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(slice_header(context));
    CHK(slice_data(context));

    // TODO: uncomment when slice_data implemented
    //CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int slice_data_partition_a_layer(ParseContext *context)
{
    dumpf(context, "%*c slice_data_partition_a_layer:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(slice_header(context));
    ue(); DUMP_UINT("slice_id");
    CHK(slice_data(context));

    // TODO: uncomment when slice_data implemented
    //CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int slice_data_partition_b_layer(ParseContext *context)
{
    dumpf(context, "%*c slice_data_partition_b_layer:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(context->sps && context->pps);

    ue(); DUMP_UINT("slice_id");
    if (context->sps->separate_colour_plane_flag == 1) {
        u(2); DUMP_UINT("colour_plane_id");
    }
    if (context->pps->redundant_pic_cnt_present_flag) {
        ue(); DUMP_UINT("redundant_pic_cnt");
    }
    CHK(slice_data(context));

    // TODO: uncomment when slice_data implemented
    //CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int slice_data_partition_c_layer(ParseContext *context)
{
    dumpf(context, "%*c slice_data_partition_c_layer:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(context->sps && context->pps);

    ue(); DUMP_UINT("slice_id");
    if (context->sps->separate_colour_plane_flag == 1) {
        u(2); DUMP_UINT("colour_plane_id");
    }
    if (context->pps->redundant_pic_cnt_present_flag) {
        ue(); DUMP_UINT("redundant_pic_cnt");
    }
    CHK(slice_data(context));

    // TODO: uncomment when slice_data implemented
    //CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int buffering_period(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    uint64_t sched_sel_idx;

    dumpf(context, "%*c buffering_period (type=%" PRIu64 ", size=%" PRIu64 "):\n", context->indent * 4, ' ', payload_type, payload_size);
    context->indent++;

    ue_m(31); DUMP_UINT("seq_parameter_set_id");
    CHK(set_sps(context, (uint8_t)context->value));

    if (context->sps->nal_hrd_parameters_present_flag) {
        dumpf(context, "%*c nal:\n", context->indent * 4, ' ');
        context->indent++;
        for (sched_sel_idx = 0; sched_sel_idx <= context->sps->cpb_cnt_minus1; sched_sel_idx++) {
            u(context->sps->initial_cpb_removal_delay_length_minus1 + 1);
            dumpf(context, "%*c initial_cpb_removal_delay[%" PRIu64 "]        : %" PRId64 "\n", context->indent * 4, ' ', sched_sel_idx, context->value);
            u(context->sps->initial_cpb_removal_delay_length_minus1 + 1);
            dumpf(context, "%*c initial_cpb_removal_delay_offset[%" PRIu64 "] : %" PRId64 "\n", context->indent * 4, ' ', sched_sel_idx, context->value);
        }
        context->indent--;
    }

    if (context->sps->vcl_hrd_parameters_present_flag) {
        dumpf(context, "%*c vcl:\n", context->indent * 4, ' ');
        context->indent++;
        for (sched_sel_idx = 0; sched_sel_idx <= context->sps->cpb_cnt_minus1; sched_sel_idx++) {
            u(context->sps->initial_cpb_removal_delay_length_minus1 + 1);
            dumpf(context, "%*c initial_cpb_removal_delay [%" PRIu64 "]       : %" PRId64 "\n", context->indent * 4, ' ', sched_sel_idx, context->value);
            u(context->sps->initial_cpb_removal_delay_length_minus1 + 1);
            dumpf(context, "%*c initial_cpb_removal_delay_offset[%" PRIu64 "] : %" PRId64 "\n", context->indent * 4, ' ', sched_sel_idx, context->value);
        }
        context->indent--;
    }

    context->indent--;
    return 1;
}

static int pic_timing(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    dumpf(context, "%*c pic_timing (type=%" PRIu64 ", size=%" PRIu64 "):\n", context->indent * 4, ' ', payload_type, payload_size);
    context->indent++;

    CHK(context->sps);

    if (context->sps->cpb_dpb_delays_present_flag) {
        u(context->sps->cpb_removal_delay_length_minus1 + 1); DUMP_UINT("cpb_removal_delay");
        u(context->sps->dpb_output_delay_length_minus1 + 1); DUMP_UINT("dpb_output_delay");
    }
    if (context->sps->pic_struct_present_flag) {
        uint64_t pic_struct;
        uint8_t num_clock_ts = 0;
        uint8_t i;

        u(4); print_pic_struct(context, (uint8_t)context->value);
        pic_struct = context->value;
        switch (pic_struct)
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
                CHK(pic_struct <= 8);
                break;
        }

        for (i = 0; i < num_clock_ts; i++) {
            u(1); DUMP_UINT("clock_timestamp_flag");
            if (context->value) {
                uint8_t full_timestamp_flag;

                context->indent++;

                u(2); DUMP_UINT("ct_type");
                u(1); DUMP_UINT("nuit_field_based_flag");
                u(5); DUMP_UINT("counting_type");
                u(1); DUMP_UINT("full_timestamp_flag");
                full_timestamp_flag = (uint8_t)context->value;
                u(1); DUMP_UINT("discontinuity_flag");
                u(1); DUMP_UINT("cnt_dropped_flag");
                u(8); DUMP_UINT("n_frames");
                if (full_timestamp_flag) {
                    u(6); DUMP_UINT("seconds");
                    u(6); DUMP_UINT("minutes");
                    u(5); DUMP_UINT("hours");
                } else {
                    u(1); DUMP_UINT("seconds_flag");
                    if (context->value) {
                        u(6); DUMP_UINT("seconds");
                        u(1); DUMP_UINT("minutes_flag");
                        if (context->value) {
                            u(6); DUMP_UINT("minutes");
                            u(1); DUMP_UINT("hours_flag");
                            if (context->value) {
                                u(5); DUMP_UINT("hours");
                            }
                        }
                    }
                }
                if (context->sps->time_offset_length > 0) {
                    ii(context->sps->time_offset_length); DUMP_INT("time_offset");
                }

                context->indent--;
            }
        }
    }

    context->indent--;
    return 1;
}

static int buffer_pic_timing(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    CHK(!context->pic_timing_buf && context->dump_buf_size == 0);

    context->pic_timing_buf = malloc(sizeof(*context->pic_timing_buf));
    if (!context->pic_timing_buf) {
        fprintf(stderr, "Failed to allocate pic timing struct\n");
        return 0;
    }
    memcpy(&context->pic_timing_buf->context, context, sizeof(*context));
    context->pic_timing_buf->payload_type = payload_type;
    context->pic_timing_buf->payload_size = payload_size;
    context->pic_timing_buf->buffer = malloc(context->nal_size);
    if (!context->pic_timing_buf->buffer) {
        fprintf(stderr, "Failed to allocate pic timing buffer\n");
        return 0;
    }
    context->pic_timing_buf->buffer_size = context->nal_size;
    memcpy(context->pic_timing_buf->buffer, &context->buffer[context->nal_start], context->nal_size);

    context->pic_timing_buf->context.bit_pos     -= context->pic_timing_buf->context.nal_start * 8;
    context->pic_timing_buf->context.end_bit_pos -= context->pic_timing_buf->context.nal_start * 8;
    context->pic_timing_buf->context.nal_start    = 0;

    return 1;
}

static int dump_buffered_pic_timing(ParseContext *context)
{
    ParseContext *pic_timing_context = &context->pic_timing_buf->context;
    pic_timing_context->pic_timing_buf = 0;
    pic_timing_context->buffer         = context->pic_timing_buf->buffer;
    pic_timing_context->buffer_size    = context->pic_timing_buf->buffer_size;
    pic_timing_context->sps            = context->sps;
    CHK(pic_timing(pic_timing_context, context->pic_timing_buf->payload_type, context->pic_timing_buf->payload_size));

    free(context->pic_timing_buf->context.buffer);
    free(context->pic_timing_buf);
    context->pic_timing_buf = 0;

    if (context->dump_buf_size > 0) {
        dumpf(context, "%s", context->dump_buf);
        context->dump_buf_size = 0;
    }

    return 1;
}

static int x264_sei_version(ParseContext *context, uint64_t data_size)
{
    uint32_t i;
    char line[512];
    size_t str_size = 0;
    int parse_options = 0;

    /* payload data is ascii text. A " - " is used to separate parts of the text.
       The last part is "options:" which contains a set of name/value separated by a ' ' */

    dumpf(context, "%*c x264_sei_version:\n", context->indent * 4, ' ');
    context->indent++;

#define PRINT_LINE(end_off)                                 \
    line[str_size - end_off] = 0;                           \
    dumpf(context, "%*c %s\n", context->indent * 4, ' ', line);     \
    str_size = 0;

    for (i = 0; i < data_size; i++) {
        u(8); line[str_size] = (char)context->value;
        str_size++;

        if (!line[str_size - 1]) {
            str_size--;
            i++;
            break;
        }

        if (parse_options) {
            if (parse_options == 1) {
                if (line[str_size - 1] == ' ') {
                    str_size = 0;
                } else {
                    parse_options = 2;
                }
            } else if (line[str_size - 1] == ' ') {
                PRINT_LINE(1)
                parse_options = 1;
            }
        } else {
            if (str_size >= 3 && strncmp(&line[str_size - 3], " - ", 3) == 0) {
                PRINT_LINE(3)
            } else if (str_size >= 8 && strncmp(&line[str_size - 8], "options:", 8) == 0) {
                PRINT_LINE(0)
                parse_options = 1;
                context->indent++;
            }
        }

        if (str_size == sizeof(line) - 1) {
            PRINT_LINE(0)
        }
    }
    if (str_size > 0) {
        PRINT_LINE(0)
    }
    if (i < data_size)
        CHK(skip_bits(context, (data_size - i) * 8));

    if (parse_options)
        context->indent--;
    context->indent--;

    return 1;
}

static int user_data_unregistered(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    static const uint64_t X264_USER_DATA_ID_HIGH = UINT64_C(0xdc45e9bde6d948b7);
    static const uint64_t X264_USER_DATA_ID_LOW  = UINT64_C(0x962cd820d923eeef);
    uint64_t uuid_high, uuid_low;

    dumpf(context, "%*c user_data_unregistered (type=%" PRIu64 ", size=%" PRIu64 "):\n", context->indent * 4, ' ', payload_type, payload_size);
    context->indent++;

    u(64); uuid_high = context->value;
    u(64); uuid_low = context->value;
    print_uuid(context, uuid_high, uuid_low);

    if (uuid_high == X264_USER_DATA_ID_HIGH && uuid_low == X264_USER_DATA_ID_LOW) {
        CHK(x264_sei_version(context, payload_size - 16));
    } else {
        dumpf(context, "%*c user_data:\n", context->indent * 4, ' ');
        context->indent++;
        CHK(read_and_print_bytes(context, payload_size - 16));
        context->indent--;
    }

    context->indent--;
    return 1;
}

static int recovery_point(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    dumpf(context, "%*c recovery_point (type=%" PRIu64 ", size=%" PRIu64 "):\n", context->indent * 4, ' ', payload_type, payload_size);
    context->indent++;

    ue(); DUMP_UINT("recovery_frame_count");
    u(1); DUMP_UINT("exact_match_flag");
    u(1); DUMP_UINT("broken_link_flag");
    u(2); DUMP_UINT("changing_slice_group_idc");

    context->indent--;
    return 1;
}

static int skip_sei_payload(ParseContext *context, uint64_t num_bytes)
{
    uint64_t rem_num_bytes = num_bytes;
    while (rem_num_bytes > 0) {
        uint8_t read_num_bytes = 8;
        if (read_num_bytes > rem_num_bytes)
            read_num_bytes = rem_num_bytes;

        /* Skip payload by calling read_bits to ensure context->emu_prevention_count is updated */
        if (!read_bits(context, read_num_bytes * 8))
            return 0;

        rem_num_bytes -= read_num_bytes;
    }

    return 1;
}

static int sei_payload(ParseContext *context, uint64_t payload_type, uint64_t payload_size)
{
    uint64_t next_bit_pos = context->bit_pos + payload_size * 8;
    context->emu_prevent_count = 0;

    if (payload_type == 0) {
        CHK(buffering_period(context, payload_type, payload_size));
    } else if (payload_type == 1) {
        // delay dump until active SPS is known (TODO: could avoid delay if there is a buffering period SEI message)
        CHK(buffer_pic_timing(context, payload_type, payload_size));
        CHK(skip_sei_payload(context, payload_size));
    } else if (payload_type == 5) {
        CHK(user_data_unregistered(context, payload_type, payload_size));
    } else if (payload_type == 6) {
        CHK(recovery_point(context, payload_type, payload_size));
    } else {
        dumpf(context, "%*c payload (type=%" PRIu64 ", size=%" PRIu64 ")\n", context->indent * 4, ' ', payload_type, payload_size);
        context->indent++;
        dumpf(context, "%*c data:\n", context->indent * 4, ' ');
        context->indent++;
        CHK(read_and_print_bytes(context, payload_size));
        context->indent--;
        context->indent--;
    }

    next_bit_pos += context->emu_prevent_count * 8;

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
    dumpf(context, "%*c sei:\n", context->indent * 4, ' ');
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
    uint8_t profile_idc;

    dumpf(context, "%*c sequence_parameter_set:\n", context->indent * 4, ' ');
    context->indent++;

    u(8); profile_idc = (uint8_t)context->value;
    u(1); constraint_flags = (uint8_t)context->value;
    u(1); constraint_flags |= (uint8_t)context->value << 1;
    u(1); constraint_flags |= (uint8_t)context->value << 2;
    u(1); constraint_flags |= (uint8_t)context->value << 3;
    u(1); constraint_flags |= (uint8_t)context->value << 4;
    u(1); constraint_flags |= (uint8_t)context->value << 5;
    print_profile_and_flags(context, profile_idc, constraint_flags);
    u(2); DUMP_UINT("reserved_zero_2bits");
    u(8); print_level(context, (uint8_t)context->value, constraint_flags);
    ue_m(31); DUMP_UINT("seq_parameter_set_id");
    set_and_init_sps(context, (uint8_t)context->value);
    context->sps->profile_idc = profile_idc;

    if (context->sps->profile_idc == 100 || context->sps->profile_idc == 110 ||
        context->sps->profile_idc == 122 || context->sps->profile_idc == 244 || context->sps->profile_idc ==  44 ||
        context->sps->profile_idc ==  83 || context->sps->profile_idc ==  86 || context->sps->profile_idc == 118 ||
        context->sps->profile_idc == 128)
    {
        ue_m(3); print_chroma_format(context, (uint8_t)context->value);
        context->sps->chroma_format_idc = (uint8_t)context->value;
        if (context->sps->chroma_format_idc == 3) {
            u(1); DUMP_UINT("separate_colour_plane_flag");
            context->sps->separate_colour_plane_flag = (uint8_t)context->value;
        }
        if (context->sps->separate_colour_plane_flag == 0)
            context->sps->chroma_array_type = context->sps->chroma_format_idc;
        else
            context->sps->chroma_array_type = 0;
        ue_m(6); DUMP_UINT("bit_depth_luma_minus8");
        ue_m(6); DUMP_UINT("bit_depth_chroma_minus8");
        u(1); DUMP_UINT("qpprime_y_zero_transform_bypass_flag");
        u(1); DUMP_UINT("seq_scaling_matrix_present_flag");
        if (context->value) {
            uint8_t i;

            context->indent++;
            for (i = 0; i < ((context->sps->chroma_format_idc != 3) ? 8 : 12); i++) {
                u(1); print_scaling_list_flag_and_index(context, "seq_scaling_list_present_flag",
                                                        (uint8_t)context->value, i);
                if (context->value) {
                    context->indent++;
                    if (i < 6) {
                        CHK(scaling_list(context, 0));
                    } else {
                        CHK(scaling_list(context, 1));
                    }
                    context->indent--;
                }
            }
            context->indent--;
        }
    }
    ue(); DUMP_UINT("log2_max_frame_num_minus4");
    context->sps->log2_max_frame_num_minus4 = (uint8_t)context->value;
    ue(); DUMP_UINT("pic_order_cnt_type");
    context->sps->pic_order_cnt_type = context->value;
    if (context->sps->pic_order_cnt_type == 0) {
        ue(); DUMP_UINT("log2_max_pic_order_cnt_lsb_minus4");
        context->sps->log2_max_pic_order_cnt_lsb_minus4 = (uint8_t)context->value;
    } else if (context->sps->pic_order_cnt_type == 1) {
        uint8_t num_ref_frames_in_pic_order_cnt_cycle;
        uint8_t i;

        u(1); DUMP_UINT("delta_pic_order_always_zero_flag");
        context->sps->delta_pic_order_always_zero_flag = (uint8_t)context->value;
        se(); DUMP_INT("offset_for_non_ref_pic");
        se(); DUMP_INT("offset_for_top_to_bottom_field");
        ue_m(255); DUMP_UINT("num_ref_frames_in_pic_order_cnt_cycle");
        num_ref_frames_in_pic_order_cnt_cycle = (uint8_t)context->value;
        context->indent++;
        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            se(); DUMP_INT("offset_for_ref_frame");
        }
        context->indent--;
    }
    ue(); DUMP_UINT("max_num_ref_frames");
    u(1); DUMP_UINT("gaps_in_frame_num_value_allowed_flag");
    ue(); DUMP_UINT("pic_width_in_mbs_minus1");
    context->sps->pic_width_in_mbs_minus1 = context->value;
    ue(); DUMP_UINT("pic_height_in_map_units_minus1");
    context->sps->pic_height_in_map_units_minus1 = context->value;
    u(1); DUMP_UINT("frame_mbs_only_flag");
    context->sps->frame_mbs_only_flag = (uint8_t)context->value;
    if (!context->value) {
        u(1); DUMP_UINT("mb_adaptive_frame_field_flag");
    }
    u(1); DUMP_UINT("direct_8x8_inference_flag");
    u(1); DUMP_UINT("frame_cropping_flag");
    if (context->value) {
        context->indent++;
        ue(); DUMP_UINT("frame_crop_left_offset");
        ue(); DUMP_UINT("frame_crop_right_offset");
        ue(); DUMP_UINT("frame_crop_top_offset");
        ue(); DUMP_UINT("frame_crop_bottom_offset");
        context->indent--;
    }
    u(1); DUMP_UINT("vui_parameters_present_flag");
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
    dumpf(context, "%*c picture_parameter_set:\n", context->indent * 4, ' ');
    context->indent++;

    ue_m(255); DUMP_UINT("pic_parameter_set_id");
    set_and_init_pps(context, (uint8_t)context->value);
    ue_m(31); DUMP_UINT("seq_parameter_set_id");
    context->pps->seq_parameter_set_id = (uint8_t)context->value;
    CHK(set_sps(context, (uint8_t)context->value));

    u(1); DUMP_UINT("entropy_coding_mode_flag");
    context->pps->entropy_coding_mode_flag = (uint8_t)context->value;
    u(1); DUMP_UINT("bottom_field_pic_order_in_frame_present_flag");
    context->pps->bottom_field_pic_order_in_frame_present_flag = (uint8_t)context->value;
    ue(); DUMP_UINT("num_slice_groups_minus1");
    context->pps->num_slice_groups_minus1 = context->value;
    if (context->value > 0) {
        uint64_t i;

        ue(); DUMP_UINT("slice_group_map_type");
        context->pps->slice_group_map_type = context->value;

        context->indent++;
        if (context->pps->slice_group_map_type == 0)
        {
            for (i = 0; i <= context->pps->num_slice_groups_minus1; i++) {
                ue(); dumpf(context, "%*c run_length_minus1[%" PRIu64 "]: %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
            }
        }
        else if (context->pps->slice_group_map_type == 2)
        {
            for (i = 0; i < context->pps->num_slice_groups_minus1; i++) {
                ue(); dumpf(context, "%*c top_left[%" PRIu64 "]    : %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
                ue(); dumpf(context, "%*c bottom_right[%" PRIu64 "]: %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
            }
        }
        else if (context->pps->slice_group_map_type == 3 ||
                 context->pps->slice_group_map_type == 4 ||
                 context->pps->slice_group_map_type == 5)
        {
            u(1); DUMP_UINT("slice_group_change_direction_flag");
            ue(); DUMP_UINT("slice_group_change_rate_minus1");
            context->pps->slice_group_change_rate_minus1 = context->value;
        }
        else if (context->pps->slice_group_map_type == 6)
        {
            uint64_t pic_size_in_map_units_minus1;

            ue(); DUMP_UINT("pic_size_in_map_units_minus1");
            pic_size_in_map_units_minus1 = context->value;
            context->indent++;
            for (i = 0; i <= pic_size_in_map_units_minus1; i++) {
                u(get_bits_required(context->pps->num_slice_groups_minus1 + 1));
                dumpf(context, "%*c slice_group_id[%" PRIu64 "]: %" PRId64 "\n", context->indent * 4, ' ', i, context->value);
            }
            context->indent--;
        }
        context->indent--;
    }
    ue(); DUMP_UINT("num_ref_idx_l0_default_active_minus1");
    context->pps->num_ref_idx_l0_default_active_minus1 = context->value;
    ue(); DUMP_UINT("num_ref_idx_l1_default_active_minus1");
    context->pps->num_ref_idx_l1_default_active_minus1 = context->value;
    u(1); DUMP_UINT("weighted_pred_flag");
    context->pps->weighted_pred_flag = (uint8_t)context->value;
    u(2); DUMP_UINT("weighted_bipred_idc");
    context->pps->weighted_bipred_idc = (uint8_t)context->value;
    se(); DUMP_INT("pic_init_qp_minus26");
    se(); DUMP_INT("pic_init_qs_minus26");
    se(); DUMP_INT("chroma_qp_index_offset");
    u(1); DUMP_UINT("deblocking_filter_control_present_flag");
    context->pps->deblocking_filter_control_present_flag = (uint8_t)context->value;
    u(1); DUMP_UINT("constrained_intra_pred_flag");
    u(1); DUMP_UINT("redundant_pic_cnt_present_flag");
    context->pps->redundant_pic_cnt_present_flag = (uint8_t)context->value;
    if (more_rbsp_data(context)) {
        uint64_t transform_8x8_mode_flag;

        u(1); DUMP_UINT("transform_8x8_mode_flag");
        transform_8x8_mode_flag = context->value;
        u(1); DUMP_UINT("pic_scaling_matrix_present_flag");
        if (context->value) {
            uint8_t i;

            context->indent++;
            for (i = 0; i < 6 + (context->sps->chroma_format_idc != 3 ? 2 : 6) * transform_8x8_mode_flag; i++) {
                u(1); print_scaling_list_flag_and_index(context, "pic_scaling_present_flag",
                                                        (uint8_t)context->value, i);
                if (context->value) {
                    context->indent++;
                    if (i < 6) {
                        CHK(scaling_list(context, 0));
                    } else {
                        CHK(scaling_list(context, 1));
                    }
                    context->indent--;
                }
            }
            context->indent--;
        }
        se(); DUMP_INT("second_chroma_qp_index_offset");
    }

    CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int access_unit_delimiter(ParseContext *context)
{
    dumpf(context, "%*c access_unit_delimiter:\n", context->indent * 4, ' ');
    context->indent++;

    u(3); print_primary_pic_type(context, (uint8_t)context->value);

    CHK(rbsp_trailing_bits(context));

    context->indent--;
    return 1;
}

static int sequence_parameter_set_extension(ParseContext *context)
{
    uint8_t bit_depth_aux_minus8;
    dumpf(context, "%*c sequence_parameter_set_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue_m(31); DUMP_UINT("seq_parameter_set_id");
    CHK(set_sps(context, (uint8_t)context->value));
    ue(); DUMP_UINT("aux_format_idc");
    if (context->value) {
        ue(); DUMP_UINT("bit_depth_aux_minus8");
        bit_depth_aux_minus8 = (uint8_t)context->value;
        u(1); DUMP_UINT("alpha_incr_flag");
        u(bit_depth_aux_minus8 + 9); DUMP_UINT("alpha_opaque_value");
        u(bit_depth_aux_minus8 + 9); DUMP_UINT("alpha_transparent_value");
    }
    u(1); DUMP_UINT("additional_extension_flag");
    CHK(rbsp_trailing_bits(context));

    context->indent++;
    return 1;
}

static int sequence_parameter_set_svc_extension(ParseContext *context)
{
    uint64_t extended_spatial_scalability_idc;

    dumpf(context, "%*c sequence_parameter_set_svc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(context->sps);

    u(1); DUMP_UINT("inter_layer_deblocking_filter_control_present_flag");
    u(2); DUMP_UINT("extended_spatial_scalability_idc");
    extended_spatial_scalability_idc = context->value;
    if (context->sps->chroma_array_type == 1 || context->sps->chroma_array_type == 2) {
        u(1); DUMP_UINT("chroma_phase_x_plus1_flag");
    }
    if (context->sps->chroma_array_type == 1) {
        u(2); DUMP_UINT("chroma_phase_y_plus1");
    }
    if (extended_spatial_scalability_idc == 1) {
        if (context->sps->chroma_array_type > 0) {
            u(1); DUMP_UINT("seq_ref_layer_chroma_phase_x_plus1_flag");
            u(2); DUMP_UINT("seq_ref_layer_chroma_phase_y_plus1");
        }
        se(); DUMP_INT("seq_scaled_ref_layer_left_offset");
        se(); DUMP_INT("seq_scaled_ref_layer_top_offset");
        se(); DUMP_INT("seq_scaled_ref_layer_right_offset");
        se(); DUMP_INT("seq_scaled_ref_layer_bottom_offset");
    }
    u(1); DUMP_UINT("seq_tcoeff_level_prediction_flag");
    if (context->value) {
        u(1); DUMP_UINT("adaptive_tcoeff_level_prediction_flag");
    }
    u(1); DUMP_UINT("slice_header_restriction_flag");

    context->indent++;
    return 1;
}

static int svc_vui_parameters_extension(ParseContext *context)
{
    uint64_t vui_ext_num_entries_minus1;
    uint8_t vui_ext_nal_hrd_parameters_present_flag;
    uint8_t vui_ext_vcl_hrd_parameters_present_flag;
    uint64_t i;

    dumpf(context, "%*c svc_vui_parameters_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); DUMP_UINT("vui_ext_num_entries_minus1");
    vui_ext_num_entries_minus1 = context->value;
    for (i = 0; i <= vui_ext_num_entries_minus1; i++) {
        dumpf(context, "%*c entry %" PRIu64 ":\n", context->indent * 4, ' ', i);
        context->indent++;

        u(3); DUMP_UINT("vui_ext_dependency_id");
        u(4); DUMP_UINT("vui_ext_quality_id");
        u(3); DUMP_UINT("vui_ext_temporal_id");
        u(1); DUMP_UINT("vui_ext_timing_info_present_flag");
        if (context->value) {
            context->indent++;
            u(32); DUMP_UINT("vui_ext_num_units_in_tick");
            u(32); DUMP_UINT("vui_ext_time_scale");
            u(1); DUMP_UINT("vui_ext_fixed_frame_rate_flag");
            context->indent--;
        }
        u(1); DUMP_UINT("vui_ext_nal_hrd_parameters_present_flag");
        vui_ext_nal_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        u(1); DUMP_UINT("vui_ext_vcl_hrd_parameters_present_flag");
        vui_ext_vcl_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        if (vui_ext_nal_hrd_parameters_present_flag || vui_ext_vcl_hrd_parameters_present_flag) {
            u(1); DUMP_UINT("vui_ext_low_delay_hrd_flag");
        }
        u(1); DUMP_UINT("vui_ext_pic_struct_present_flag");

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

    dumpf(context, "%*c sequence_parameter_set_mvc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); DUMP_UINT("num_views_minus1");
    num_views_minus1 = context->value;
    for (i = 0; i <= num_views_minus1; i++) {
        ue(); DUMP_UINT("view_id");
    }
    for (i = 1; i <= num_views_minus1; i++) {
        ue(); DUMP_UINT("num_anchor_refs_I0");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); DUMP_UINT("anchor_ref_I0");
        }
        ue(); DUMP_UINT("num_anchor_refs_I1");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); DUMP_UINT("anchor_ref_I1");
        }
    }
    for (i = 1; i <= num_views_minus1; i++) {
        ue(); DUMP_UINT("num_non_anchor_refs_I0");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); DUMP_UINT("non_anchor_ref_I0");
        }
        ue(); DUMP_UINT("num_non_anchor_refs_I1");
        num_refs = context->value;
        for (j = 0; j < num_refs; j++) {
            ue(); DUMP_UINT("non_anchor_ref_I1");
        }
    }
    ue(); DUMP_UINT("num_level_values_signalled_minus1");
    num_level_values_signalled_minus1 = context->value;
    for (i = 0; i <= num_level_values_signalled_minus1; i++) {
        u(8); DUMP_UINT("level_idc");
        ue(); DUMP_UINT("num_applicable_ops_minus1");
        num_applicable_ops_minus1 = context->value;
        for (j = 0; j <= num_applicable_ops_minus1; j++) {
            u(3); DUMP_UINT("applicable_op_temporal_id");
            ue(); DUMP_UINT("applicable_op_num_target_views_minus1");
            applicable_op_num_target_views_minus1 = context->value;
            for (k = 0; k <= applicable_op_num_target_views_minus1; k++) {
                ue(); DUMP_UINT("applicable_op_target_view_id");
            }
            ue(); DUMP_UINT("applicable_op_num_views_minus1");
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

    dumpf(context, "%*c mvc_vui_parameters_extension:\n", context->indent * 4, ' ');
    context->indent++;

    ue(); DUMP_UINT("vui_mvc_num_ops_minus1");
    vui_mvc_num_ops_minus1 = context->value;
    for (i = 0; i <= vui_mvc_num_ops_minus1; i++) {
        dumpf(context, "%*c entry %" PRIu64 ":\n", context->indent * 4, ' ', i);
        context->indent++;

        u(3); DUMP_UINT("vui_mvc_temporal_id");
        ue(); DUMP_UINT("vui_mvc_num_target_output_views_minus1");
        vui_mvc_num_target_output_views_minus1 = context->value;
        for (j = 0; j <= vui_mvc_num_target_output_views_minus1; j++) {
            ue(); DUMP_UINT("vui_mvc_view_id");
        }
        u(1); DUMP_UINT("vui_mvc_timing_info_present_flag");
        if (context->value) {
            context->indent++;
            u(32); DUMP_UINT("vui_mvc_num_units_in_tick");
            u(32); DUMP_UINT("vui_mvc_time_scale");
            u(1); DUMP_UINT("vui_mvc_fixed_frame_rate_flag");
            context->indent--;
        }
        u(1); DUMP_UINT("vui_mvc_nal_hrd_parameters_present_flag");
        vui_mvc_nal_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        u(1); DUMP_UINT("vui_mvc_vcl_hrd_parameters_present_flag");
        vui_mvc_vcl_hrd_parameters_present_flag = (uint8_t)context->value;
        if (context->value) {
            context->indent++;
            CHK(hrd_parameters(context));
            context->indent--;
        }
        if (vui_mvc_nal_hrd_parameters_present_flag || vui_mvc_vcl_hrd_parameters_present_flag) {
            u(1); DUMP_UINT("vui_mvc_low_delay_hrd_flag");
        }
        u(1); DUMP_UINT("vui_mvc_pic_struct_present_flag");

        context->indent--;
    }

    context->indent++;
    return 1;
}

static int subset_sequence_parameter_set(ParseContext *context)
{
    dumpf(context, "%*c subset_sequence_parameter_set:\n", context->indent * 4, ' ');
    context->indent++;

    CHK(sequence_parameter_set_data(context));
    if (context->sps->profile_idc == 83 || context->sps->profile_idc == 86) {
        CHK(sequence_parameter_set_svc_extension(context));
        u(1); DUMP_UINT("svc_vui_parameter_present_flag");
        if (context->value) {
            CHK(svc_vui_parameters_extension(context));
        }
    } else if (context->sps->profile_idc == 118 || context->sps->profile_idc == 128) {
        f(1); DUMP_UINT("bit_equal_to_one");
        CHK(sequence_parameter_set_mvc_extension(context));
        u(1); DUMP_UINT("mvc_vui_parameter_present_flag");
        if (context->value) {
            CHK(mvc_vui_parameters_extension(context));
        }
    }
    u(1); DUMP_UINT("additional_extension2_flag");
    if (context->value) {
        while (more_rbsp_data(context)) {
            u(1); DUMP_UINT("additional_extension2_data_flag");
        }
    }
    CHK(rbsp_trailing_bits(context));

    context->indent++;
    return 1;
}

static int nal_unit_header_svc_extension(ParseContext *context)
{
    dumpf(context, "%*c nal_unit_header_svc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    u(1); DUMP_UINT("idr_flag");
    u(6); DUMP_UINT("priority_id");
    u(1); DUMP_UINT("no_inter_layer_pred_flag");
    u(3); DUMP_UINT("dependency_id");
    u(4); DUMP_UINT("quality_id");
    u(3); DUMP_UINT("temporal_id");
    u(1); DUMP_UINT("use_ref_base_pic_flag");
    u(1); DUMP_UINT("discardable_flag");
    u(1); DUMP_UINT("output_flag");
    u(2); DUMP_UINT("reserved_three_2bits");

    context->indent--;
    return 1;
}

static int nal_unit_header_mvc_extension(ParseContext *context)
{
    dumpf(context, "%*c nal_unit_header_mvc_extension:\n", context->indent * 4, ' ');
    context->indent++;

    u(1); DUMP_UINT("non_idr_flag");
    u(6); DUMP_UINT("priority_id");
    u(10); DUMP_UINT("view_id");
    u(3); DUMP_UINT("temporal_id");
    u(1); DUMP_UINT("anchor_pic_flag");
    u(1); DUMP_UINT("inter_view_flag");
    u(1); DUMP_UINT("reserved_one_bit");

    context->indent--;
    return 1;
}

static int parse_nal_unit(ParseContext *context)
{
    uint64_t bit_pos;

    dumpf(context, "NAL: pos=%" PRId64 ", start=%u, size=%u, padding=%u\n",
          context->data_start_file_pos, context->nal_start, context->nal_size, context->nal_padding);

    context->indent++;

    f(1); DUMP_UINT("forbidden_zero_bit");
    u(2); DUMP_UINT("nal_ref_idc");
    context->nal_ref_idc = (uint8_t)context->value;
    u(5); print_nal_unit_type(context);
    context->nal_unit_type = (uint8_t)context->value;

    bit_pos = context->bit_pos;
    if (context->nal_unit_type == 14 || context->nal_unit_type == 20) {
        u(1); DUMP_UINT("svc_extension_flag");
        if (context->value) {
            CHK(nal_unit_header_svc_extension(context));
        } else {
            CHK(nal_unit_header_mvc_extension(context));
        }
        bit_pos += 3 * 8;
    }
    CHK(context->bit_pos <= bit_pos);
    context->bit_pos = bit_pos;

    switch (context->nal_unit_type)
    {
        case CODED_SLICE_NON_IDR_PICT:
            CHK(slice_layer_without_partitioning(context));
            break;
        case CODED_SLICE_DATA_PART_A:
            CHK(slice_data_partition_a_layer(context));
            break;
        case CODED_SLICE_DATA_PART_B:
            CHK(slice_data_partition_b_layer(context));
            break;
        case CODED_SLICE_DATA_PART_C:
            CHK(slice_data_partition_c_layer(context));
            break;
        case CODED_SLICE_IDR_PICT:
            CHK(slice_layer_without_partitioning(context));
            break;
        case SEI:
            CHK(sei(context));
            break;
        case SEQUENCE_PARAMETER_SET:
            CHK(sequence_parameter_set(context));
            break;
        case PICTURE_PARAMETER_SET:
            CHK(picture_parameter_set(context));
            break;
        case ACCESS_UNIT_DELIMITER:
            CHK(access_unit_delimiter(context));
            break;
        case SEQUENCE_PARAMETER_SET_EXT:
            CHK(sequence_parameter_set_extension(context));
            break;
        case SUBSET_SEQUENCE_PARAMETER_SET:
            CHK(subset_sequence_parameter_set(context));
            break;
        case END_OF_SEQUENCE:
        case END_OF_STREAM:
        case FILLER:
        default:
            break;
    }

    context->indent--;

    return 1;
}



static void print_usage(const char *cmd, int error)
{
    FILE *output = error ? stderr: stdout;

    fprintf(output, "Text dump raw H.264 bitstream files\n");
    fprintf(output, "\n");
    fprintf(output, "Usage: %s [options] <filename>\n", cmd);
    fprintf(output, "Options:\n");
    fprintf(output, "  -h | --help      Show help and exit\n");
    fprintf(output, "  --isom <llen>    NAL units are prefixed by a <llen> big-endian size word\n");
    fprintf(output, "                   NOTE: NAL units will not be parsed correctly if there is no SPS or PPS\n");
    fprintf(output, "                   An Annex B byte stream format is assumed by default\n");
    if (error)
        fprintf(output, "\n");
}

int main(int argc, const char **argv)
{
    const char *filename;
    ParseContext context;
    unsigned int isom_llen = 0;
    int cmdln_index;

    if (argc <= 1) {
        print_usage(argv[0], 0);
        return 0;
    }

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++) {
        if (strcmp(argv[cmdln_index], "-h") == 0 ||
            strcmp(argv[cmdln_index], "--help") == 0)
        {
            print_usage(argv[0], 0);
            return 0;
        }
        else if (strcmp(argv[cmdln_index], "--isom") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                print_usage(argv[0], 1);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &isom_llen) != 1 || isom_llen == 0 || isom_llen > 4)
            {
                print_usage(argv[0], 1);
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

    if (cmdln_index + 1 < argc) {
        print_usage(argv[0], 1);
        fprintf(stderr, "Unknown option '%s'\n", argv[cmdln_index]);
        return 1;
    }
    if (cmdln_index >= argc) {
        print_usage(argv[0], 1);
        fprintf(stderr, "Missing <filename>\n");
        return 1;
    }

    filename = argv[cmdln_index];


    if (!init_context(&context, filename))
        return 1;

    if (isom_llen == 0) {
        while (parse_byte_stream_nal_unit(&context)) {
            if (!parse_nal_unit(&context)) {
                fprintf(stderr, "Failed to parse NAL unit\n");
                break;
            }
        }
    } else {
        while (parse_isom_nal_unit(&context, isom_llen)) {
            if (!parse_nal_unit(&context)) {
                fprintf(stderr, "Failed to parse NAL unit\n");
                break;
            }
        }
    }

    deinit_context(&context);


    return 0;
}

