/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <inttypes.h>


#define MPEG_PICTURE_START_CODE     0x00000100
#define MPEG_SEQUENCE_HEADER_CODE   0x000001b3
#define MPEG_SEQUENCE_EXT_CODE      0x000001b5
#define MPEG_GROUP_HEADER_CODE      0x000001b8

#define MPEG_I_FRAME_TYPE           0x01
#define MPEG_P_FRAME_TYPE           0x02
#define MPEG_B_FRAME_TYPE           0x03


typedef enum
{
    TYPE_UNKNOWN                        = 0,
    TYPE_16BIT_PCM                      = 1,
    TYPE_IEC_DV25                       = 2,
    TYPE_DVBASED_DV25                   = 3,
    TYPE_DV50                           = 4,
    TYPE_DV100_1080I                    = 5,
    TYPE_DV100_720P                     = 6,
    TYPE_AVCI100_1080I                  = 7,
    TYPE_AVCI100_1080P                  = 8,
    TYPE_AVCI50_1080I                   = 9,
    TYPE_AVCI50_1080P                   = 10,
    TYPE_D10_50                         = 11,
    TYPE_D10_40                         = 12,
    TYPE_D10_30                         = 13,
    TYPE_MPEG2LG_422P_HL_1080I          = 14,
    TYPE_MPEG2LG_MP_H14_1080I           = 15,
    TYPE_MPEG2LG_MP_HL_1080I            = 16,
    TYPE_UNC_SD                         = 17,
    TYPE_UNC_HD_1080I                   = 18,
    TYPE_UNC_HD_1080P                   = 19,
    TYPE_UNC_HD_720P                    = 20,
    TYPE_MPEG2LG_422P_HL_1080P          = 21,
    TYPE_MPEG2LG_MP_H14_1080P           = 22,
    TYPE_MPEG2LG_MP_HL_1080P            = 23,
    TYPE_MPEG2LG_MP_HL_1080I_1440       = 24,
    TYPE_MPEG2LG_MP_HL_1080P_1440       = 25,
    TYPE_MPEG2LG_422P_HL_720P           = 26,
    TYPE_MPEG2LG_MP_HL_720P             = 27,
    TYPE_DV100_1080P                    = 28,
    TYPE_AVCI100_720P                   = 29,
    TYPE_AVCI50_720P                    = 30,
    TYPE_VC3_1080P_1235                 = 31,
    TYPE_VC3_1080P_1237                 = 32,
    TYPE_VC3_1080P_1238                 = 33,
    TYPE_VC3_1080I_1241                 = 34,
    TYPE_VC3_1080I_1242                 = 35,
    TYPE_VC3_1080I_1243                 = 36,
    TYPE_VC3_720P_1250                  = 37,
    TYPE_VC3_720P_1251                  = 38,
    TYPE_VC3_720P_1252                  = 39,
    TYPE_VC3_1080P_1253                 = 40,
    TYPE_AVID_ALPHA_HD_1080I            = 41,
    TYPE_24BIT_PCM                      = 42,
    TYPE_ANC_DATA                       = 43,
    TYPE_VBI_DATA                       = 44,
    TYPE_UNC_UHD_3840                   = 45,
    TYPE_AVCI200_1080I                  = 46,
    TYPE_AVCI200_1080P                  = 47,
    TYPE_AVCI200_720P                   = 48,
    TYPE_VC3_1080I_1244                 = 49,
    TYPE_VC3_720P_1258                  = 50,
    TYPE_VC3_1080P_1259                 = 51,
    TYPE_VC3_1080I_1260                 = 52,
    TYPE_AS10_MPEG2LG_422P_HL_1080I     = 53,
    TYPE_VC2                            = 54,
    TYPE_RDD36_422                      = 55,
    TYPE_RDD36_4444                     = 56,
    TYPE_16BIT_PCM_SAMPLES              = 57,
    TYPE_END                            = 58,
} EssenceType;

typedef struct
{
    bool seq_header;
    uint8_t profile_level;
    bool is_progressive;
    uint8_t chroma_format;
    uint32_t bit_rate;
    bool low_delay;
    bool closed_gop;
    uint32_t h_size;
    uint32_t v_size;
    uint32_t temporal_ref;
    uint8_t frame_type;
} MPEGInfo;


static const unsigned char AVCI_AUD_DATA[] =
{
    0x00, 0x00, 0x00, 0x01, 0x09, 0x10
};

static const unsigned char AVCI_SPS_DATA[] =
{
    0x00, 0x00, 0x00, 0x01, 0x67, 0x7a, 0x10, 0x29,
    0xb6, 0xd4, 0x20, 0x22, 0x33, 0x19, 0xc6, 0x63,
    0x23, 0x21, 0x01, 0x11, 0x98, 0xce, 0x33, 0x19,
    0x18, 0x21, 0x03, 0x3a, 0x46, 0x65, 0x6a, 0x65,
    0x24, 0xad, 0xe9, 0x12, 0x32, 0x14, 0x1a, 0x26,
    0x34, 0xad, 0xa4, 0x41, 0x82, 0x23, 0x01, 0x50,
    0x2b, 0x1a, 0x24, 0x69, 0x48, 0x30, 0x40, 0x2e,
    0x11, 0x12, 0x08, 0xc6, 0x8c, 0x04, 0x41, 0x28,
    0x4c, 0x34, 0xf0, 0x1e, 0x01, 0x13, 0xf2, 0xe0,
    0x3c, 0x60, 0x20, 0x20, 0x28, 0x00, 0x00, 0x03,
    0x00, 0x08, 0x00, 0x00, 0x03, 0x01, 0x94, 0x20
};

static const unsigned char AVCI_PPS_DATA[] =
{
    0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x33, 0x48,
    0xd0
};

static const unsigned char AVCI_SLICE_DATA[] =
{
    0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x80, 0x80,
    0x02, 0x14, 0xf0, 0xcb, 0x07, 0x00, 0x40, 0x00,
    0x42, 0x01, 0x20, 0xe0, 0x00, 0x21, 0x02, 0x00,
    0x02, 0x3e, 0xbc, 0x70, 0x00, 0x38, 0x00, 0x4a,
    0x8e, 0x09, 0x15, 0xcf, 0xe0, 0xe0, 0x00, 0x5c,
    0x17, 0x83, 0x80, 0x04, 0xd5, 0x72, 0xb1, 0xec
};

static const unsigned char VC2_SEQUENCE_HEADER[] =
    {0x70, 0x85, 0x58, 0x84, 0x3f};

static const unsigned char VC2_PICTURE[] =
    {0x00, 0x00, 0x00, 0x00, 0x2d, 0x50, 0x18, 0x08, 0x1b, 0x7f, 0x10};

// don't care data
static unsigned char DATA[4096];



static void init_data(int init_val)
{
    if (init_val >= 0) {
      memset(DATA, init_val, sizeof(DATA));
    } else {
      size_t i;
      for (i = 0; i < sizeof(DATA); i++)
          DATA[i] = (unsigned char)i;
    }
}

static void set_bit(unsigned char *data, uint32_t bit_offset, unsigned char bit)
{
    unsigned char bit_byte = bit << (7 - (bit_offset % 8));
    unsigned char byte = data[bit_offset / 8];
    byte &= ~bit_byte;
    byte |= bit_byte;
    data[bit_offset / 8] = byte;
}

static void set_mpeg_bits(unsigned char *data, uint32_t bit_offset, uint8_t num_bits, uint32_t value)
{
    assert(num_bits <= 32);

    uint8_t i;
    for (i = 0; i < num_bits; i++)
        set_bit(data, bit_offset + i, (value >> (num_bits - i - 1)) & 1);
}

static void set_mpeg_start_code(unsigned char *data, uint32_t offset, uint32_t code)
{
    data[offset    ] = (unsigned char)((code >> 24) & 0xff);
    data[offset + 1] = (unsigned char)((code >> 16) & 0xff);
    data[offset + 2] = (unsigned char)((code >> 8) & 0xff);
    data[offset + 3] = (unsigned char)(code & 0xff);
}

static void fill_mpeg_frame(unsigned char *data, uint32_t data_size, const MPEGInfo *info)
{
    assert(data_size > 400);

    uint32_t set_offset = 0;
    if (info->seq_header) {
        set_mpeg_start_code(data, 0, MPEG_SEQUENCE_HEADER_CODE);
        set_offset = 0;
        set_mpeg_bits(data, set_offset * 8 + 32, 12, info->h_size);
        set_mpeg_bits(data, set_offset * 8 + 44, 12, info->v_size);
        set_mpeg_bits(data, set_offset * 8 + 64, 18, info->bit_rate);
        set_offset += 100;

        set_mpeg_start_code(data, set_offset, MPEG_SEQUENCE_EXT_CODE);
        set_mpeg_bits(data, set_offset * 8 + 32, 4, 0x01);
        set_mpeg_bits(data, set_offset * 8 + 36, 8,  info->profile_level);
        set_mpeg_bits(data, set_offset * 8 + 44, 1,  info->is_progressive);
        set_mpeg_bits(data, set_offset * 8 + 45, 2,  info->chroma_format);
        set_mpeg_bits(data, set_offset * 8 + 47, 2,  info->h_size >> 12);
        set_mpeg_bits(data, set_offset * 8 + 49, 2,  info->v_size >> 12);
        set_mpeg_bits(data, set_offset * 8 + 51, 12, info->bit_rate >> 18);
        set_mpeg_bits(data, set_offset * 8 + 72, 1,  info->low_delay);
        set_offset += 100;

        set_mpeg_start_code(data, set_offset, MPEG_GROUP_HEADER_CODE);
        set_mpeg_bits(data, set_offset * 8 + 57, 1, info->closed_gop);
        set_offset += 100;
    }

    set_mpeg_start_code(data, set_offset, MPEG_PICTURE_START_CODE);
    set_mpeg_bits(data, set_offset * 8 + 32, 10, info->temporal_ref);
    set_mpeg_bits(data, set_offset * 8 + 42, 3,  info->frame_type);
}

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

static void write_data(FILE *file, int64_t size)
{
    int64_t i;
    int64_t num_data_chunks = size / sizeof(DATA);
    int64_t partial_data_size = size % sizeof(DATA);

    for (i = 0; i < num_data_chunks; i++)
        write_buffer(file, DATA, sizeof(DATA));

    if (partial_data_size > 0)
        write_buffer(file, DATA, (size_t)partial_data_size);
}

static void write_pcm_samples(FILE *file, uint16_t block_align, unsigned int samples)
{
    write_data(file, samples * block_align);
}

static void write_pcm(FILE *file, uint16_t block_align, unsigned int duration)
{
    // 25 Hz, 1920 samples per frame
    write_pcm_samples(file, block_align, 1920 * duration);
}

static void write_dv25(FILE *file, unsigned int duration)
{
    write_data(file, duration * 144000);
}

static void write_dv50(FILE *file, unsigned int duration)
{
    write_data(file, duration * 288000);
}

static void write_dv100_1080i(FILE *file, unsigned int duration)
{
    write_data(file, duration * 576000);
}

static void write_dv100_720p(FILE *file, unsigned int duration)
{
    write_data(file, duration * 288000);
}

static void write_avci_frame(FILE *file, unsigned int non_vcl_size, unsigned int vcl_size, bool is_first)
{
    if (is_first) {
        write_buffer(file, AVCI_AUD_DATA, sizeof(AVCI_AUD_DATA));
        write_buffer(file, AVCI_SPS_DATA, sizeof(AVCI_SPS_DATA));
        write_zeros(file, 256 - (sizeof(AVCI_AUD_DATA) + sizeof(AVCI_SPS_DATA)));
        write_buffer(file, AVCI_PPS_DATA, sizeof(AVCI_PPS_DATA));
        write_zeros(file, non_vcl_size - (256 + sizeof(AVCI_PPS_DATA)));
    } else {
        write_buffer(file, AVCI_AUD_DATA, sizeof(AVCI_AUD_DATA));
        write_zeros(file, non_vcl_size - sizeof(AVCI_AUD_DATA));
    }
    write_buffer(file, AVCI_SLICE_DATA, sizeof(AVCI_SLICE_DATA));
    write_data(file, vcl_size - sizeof(AVCI_SLICE_DATA));
}

static void write_avci200_1080(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 19 * 512, 1134592, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 18 * 512, 1134592, false);
}

static void write_avci100_1080(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 19 * 512, 559104, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 18 * 512, 559104, false);
}

static void write_avci200_720(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 11 * 512, 566784, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 10 * 512, 566784, false);
}

static void write_avci100_720(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 11 * 512, 279040, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 10 * 512, 279040, false);
}

static void write_avci50_1080(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 19 * 512, 271360, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 18 * 512, 271360, false);
}

static void write_avci50_720(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 11 * 512, 135168, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 10 * 512, 135168, false);
}

static void write_d10(FILE *file, int type, unsigned int duration)
{
    MPEGInfo mpeg_info;
    memset(&mpeg_info, 0, sizeof(mpeg_info));
    mpeg_info.seq_header     = true;
    mpeg_info.profile_level  = 133;
    mpeg_info.chroma_format  = 2;
    mpeg_info.low_delay      = true;
    mpeg_info.h_size         = 720;
    mpeg_info.v_size         = 608;
    mpeg_info.frame_type     = MPEG_I_FRAME_TYPE;

    uint32_t frame_size;
    switch (type)
    {
        case TYPE_D10_50:
            frame_size = 250000;
            break;
        case TYPE_D10_40:
            frame_size = 200000;
            break;
        case TYPE_D10_30:
        default:
            frame_size = 150000;
            break;
    }
    mpeg_info.bit_rate = (frame_size * 25 * 8) / 400;

    unsigned char *data = new unsigned char[frame_size];
    memset(data, 0, frame_size);
    fill_mpeg_frame(data, frame_size, &mpeg_info);

    unsigned int i;
    for (i = 0; i < duration; i++)
        write_buffer(file, data, frame_size);
}

static void write_mpeg2lg(FILE *file, int type, unsigned int duration, bool low_delay, bool closed_gop)
{
    MPEGInfo mpeg_info;
    memset(&mpeg_info, 0, sizeof(mpeg_info));
    mpeg_info.is_progressive = false;
    mpeg_info.low_delay      = low_delay;

    uint32_t i_frame_size, non_i_frame_size;
    switch (type)
    {
        case TYPE_MPEG2LG_422P_HL_1080I:
        case TYPE_MPEG2LG_422P_HL_1080P:
            i_frame_size     = 270000;
            non_i_frame_size = 240000;
            mpeg_info.profile_level = 0x82;
            mpeg_info.chroma_format = 2;
            mpeg_info.h_size        = 1920;
            mpeg_info.v_size        = 1080;
            mpeg_info.bit_rate      = (50 * 1000 * 1000) / 400;
            break;
        case TYPE_MPEG2LG_422P_HL_720P:
            i_frame_size     = 270000;
            non_i_frame_size = 240000;
            mpeg_info.profile_level = 0x82;
            mpeg_info.chroma_format = 2;
            mpeg_info.h_size        = 1280;
            mpeg_info.v_size        = 720;
            mpeg_info.bit_rate      = (50 * 1000 * 1000) / 400;
            break;
        case TYPE_MPEG2LG_MP_HL_1080I:
        case TYPE_MPEG2LG_MP_HL_1080P:
            i_frame_size     = 270000;
            non_i_frame_size = 240000;
            mpeg_info.profile_level = 0x44;
            mpeg_info.chroma_format = 1;
            mpeg_info.h_size        = 1920;
            mpeg_info.v_size        = 1080;
            mpeg_info.bit_rate      = (50 * 1000 * 1000) / 400;
            break;
        case TYPE_MPEG2LG_MP_HL_720P:
            i_frame_size     = 270000;
            non_i_frame_size = 240000;
            mpeg_info.profile_level = 0x44;
            mpeg_info.chroma_format = 1;
            mpeg_info.h_size        = 1280;
            mpeg_info.v_size        = 720;
            mpeg_info.bit_rate      = (50 * 1000 * 1000) / 400;
            break;
        case TYPE_MPEG2LG_MP_HL_1080I_1440:
        case TYPE_MPEG2LG_MP_HL_1080P_1440:
            i_frame_size     = 195000;
            non_i_frame_size = 165000;
            mpeg_info.profile_level = 0x44;
            mpeg_info.chroma_format = 1;
            mpeg_info.h_size        = 1440;
            mpeg_info.v_size        = 1080;
            mpeg_info.bit_rate      = (35 * 1000 * 1000) / 400;
            break;
        case TYPE_MPEG2LG_MP_H14_1080I:
        case TYPE_MPEG2LG_MP_H14_1080P:
        default:
            i_frame_size     = 195000;
            non_i_frame_size = 165000;
            mpeg_info.profile_level = 0x46;
            mpeg_info.chroma_format = 1;
            mpeg_info.h_size        = 1440;
            mpeg_info.v_size        = 1080;
            mpeg_info.bit_rate      = (35 * 1000 * 1000) / 400;
            break;
    }
    uint32_t max_frame_size = i_frame_size > non_i_frame_size ? i_frame_size : non_i_frame_size;

    unsigned char *data = new unsigned char[max_frame_size];

    unsigned int i;
    for (i = 0; i < duration; i++) {
        memset(data, 0, max_frame_size);
        switch (i % 12) {
            case 0:
                mpeg_info.seq_header   = true;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 0;
                    mpeg_info.frame_type   = MPEG_I_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = (i == 0);
                    mpeg_info.temporal_ref = 2;
                    mpeg_info.frame_type   = MPEG_I_FRAME_TYPE;
                }
                fill_mpeg_frame(data, i_frame_size, &mpeg_info);
                break;
            case 1:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 3;
                    mpeg_info.frame_type   = MPEG_P_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 0;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 2:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 1;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 1;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 3:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 2;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 5;
                    mpeg_info.frame_type   = MPEG_P_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 4:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 6;
                    mpeg_info.frame_type   = MPEG_P_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 3;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 5:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 4;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 4;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 6:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 5;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 8;
                    mpeg_info.frame_type   = MPEG_P_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 7:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 9;
                    mpeg_info.frame_type   = MPEG_P_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 6;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 8:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 7;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 7;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 9:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 8;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 11;
                    mpeg_info.frame_type   = MPEG_P_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 10:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 11;
                    mpeg_info.frame_type   = MPEG_P_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 9;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
            case 11:
            default:
                mpeg_info.seq_header   = false;
                if (closed_gop) {
                    mpeg_info.closed_gop   = true;
                    mpeg_info.temporal_ref = 10;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                } else {
                    mpeg_info.closed_gop   = false;
                    mpeg_info.temporal_ref = 10;
                    mpeg_info.frame_type   = MPEG_B_FRAME_TYPE;
                }
                fill_mpeg_frame(data, non_i_frame_size, &mpeg_info);
                break;
        }

        if (i % 12 == 0)
            write_buffer(file, data, i_frame_size);
        else
            write_buffer(file, data, non_i_frame_size);
    }

    delete [] data;
}

static void write_unc(FILE *file, int type, unsigned int duration)
{
    uint32_t frame_size;
    switch (type)
    {
        case TYPE_UNC_SD:
            frame_size = 720 * 576 * 2;
            break;
        case TYPE_UNC_HD_1080I:
        case TYPE_UNC_HD_1080P:
            frame_size = 1920 * 1080 * 2;
            break;
        case TYPE_UNC_UHD_3840:
            frame_size = 3840 * 2160 * 2;
            break;
        case TYPE_UNC_HD_720P:
        default:
            frame_size = 1280 * 720 * 2;
            break;
    }

    write_data(file, (int64_t)duration * frame_size);
}

static void set_rdd36_bits(unsigned char *data, uint32_t *bit_offset, uint8_t num_bits, uint32_t value)
{
    assert(num_bits <= 32);

    uint8_t i;
    for (i = 0; i < num_bits; i++)
        set_bit(data, (*bit_offset) + i, (value >> (num_bits - i - 1)) & 1);
    *bit_offset += num_bits;
}

static void write_rdd36(FILE *file, int type, unsigned int duration)
{
    #define RDD36_FRAME_SIZE        64
    #define RDD32_FRAME_HEADER_SIZE 20

    unsigned char data[RDD36_FRAME_SIZE];
    memset(data, 0, sizeof(data));
    uint32_t bit_offset = 0;
    set_rdd36_bits(data, &bit_offset, 32, RDD36_FRAME_SIZE);        // frame_size
    set_rdd36_bits(data, &bit_offset, 32, 0x69637066);              // frame identifier
    set_rdd36_bits(data, &bit_offset, 16, RDD32_FRAME_HEADER_SIZE); // frame header size
    set_rdd36_bits(data, &bit_offset,  8, 0);                       // reserved
    set_rdd36_bits(data, &bit_offset,  8, 1);                       // bitstream version
    set_rdd36_bits(data, &bit_offset, 32, 0x20626d78);              // encoder identifier
    set_rdd36_bits(data, &bit_offset, 16, 1920);                    // width
    set_rdd36_bits(data, &bit_offset, 16, 1080);                    // height
    if (type == TYPE_RDD36_422)
        set_rdd36_bits(data, &bit_offset, 2, 2);                    // chroma_format
    else
        set_rdd36_bits(data, &bit_offset, 2, 3);                    // chroma_format
    set_rdd36_bits(data, &bit_offset,  2, 0);                       // reserved
    if (type == TYPE_RDD36_422)
        set_rdd36_bits(data, &bit_offset,  2, 1);                   // interlace mode
    else
        set_rdd36_bits(data, &bit_offset,  2, 0);                   // interlace mode
    set_rdd36_bits(data, &bit_offset,  2, 0);                       // reserved
    set_rdd36_bits(data, &bit_offset,  4, 3);                       // aspect ratio
    if (type == TYPE_RDD36_422)
        set_rdd36_bits(data, &bit_offset,  4, 3);                   // frame rate
    else
        set_rdd36_bits(data, &bit_offset,  4, 6);                   // frame rate
    set_rdd36_bits(data, &bit_offset,  8, 1);                       // color primaries
    set_rdd36_bits(data, &bit_offset,  8, 1);                       // transfer characteristic
    set_rdd36_bits(data, &bit_offset,  8, 1);                       // matrix coefficients
    set_rdd36_bits(data, &bit_offset,  4, 0);                       // reserved
    if (type == TYPE_RDD36_422)
        set_rdd36_bits(data, &bit_offset, 4, 0);                    // alpha channel type
    else
        set_rdd36_bits(data, &bit_offset, 4, 1);                    // alpha channel type
    // the rest is not parsed by RDD36EssenceParser

    unsigned int i;
    for (i = 0; i < duration; i++)
        write_buffer(file, data, RDD36_FRAME_SIZE);
}

static void set_vc2_parse_info(unsigned char *data, uint8_t parse_code, uint32_t next_parse_offset,
                               uint32_t prev_parse_offset)
{
    data[0] = 0x42;
    data[1] = 0x42;
    data[2] = 0x43;
    data[3] = 0x44;
    data[4] = parse_code;
    data[5] = (uint8_t)(next_parse_offset >> 24);
    data[6] = (uint8_t)(next_parse_offset >> 16);
    data[7] = (uint8_t)(next_parse_offset >> 8);
    data[8] = (uint8_t)(next_parse_offset);
    data[9] = (uint8_t)(prev_parse_offset >> 24);
    data[10] = (uint8_t)(prev_parse_offset >> 16);
    data[11] = (uint8_t)(prev_parse_offset >> 8);
    data[12] = (uint8_t)(prev_parse_offset);
}

static void write_vc2(FILE *file, unsigned int duration)
{
    uint32_t next, prev;
    uint32_t size = 0;
    uint32_t seq_end_offset;

    prev = 0;
    next = 13 + sizeof(VC2_SEQUENCE_HEADER);
    set_vc2_parse_info(&DATA[size], 0x00, next, prev);
    memcpy(&DATA[size + 13], VC2_SEQUENCE_HEADER, sizeof(VC2_SEQUENCE_HEADER));
    size += next;
    seq_end_offset = size;

    prev = next;
    next = 13 + 4;
    set_vc2_parse_info(&DATA[size], 0x20, next, prev); // auxilliary
    memset(&DATA[size + 13], 0, 4);
    size += next;

    prev = next;
    next = 13 + sizeof(VC2_PICTURE);
    set_vc2_parse_info(&DATA[size], 0xe8, 0, prev); // next set to 0 to force search for next parse info
    memcpy(&DATA[size + 13], VC2_PICTURE, sizeof(VC2_PICTURE));
    size += next;

    prev = next;
    next = 13 + 4;
    set_vc2_parse_info(&DATA[size], 0x30, next, prev); // padding
    memset(&DATA[size + 13], 0, 4);
    size += next;

    unsigned int i;
    for (i = 0; i < duration; i++) {
        if (i == 0)
            write_buffer(file, DATA, size);
        else
            write_buffer(file, &DATA[seq_end_offset], size - seq_end_offset);
    }
}

static void write_vc3(FILE *file, int type, unsigned int duration)
{
    uint32_t frame_size;
    switch (type)
    {
        case TYPE_VC3_1080P_1235:
            frame_size = 917504;
            break;
        case TYPE_VC3_1080P_1237:
            frame_size = 606208;
            break;
        case TYPE_VC3_1080P_1238:
            frame_size = 917504;
            break;
        case TYPE_VC3_1080I_1241:
            frame_size = 917504;
            break;
        case TYPE_VC3_1080I_1242:
            frame_size = 606208;
            break;
        case TYPE_VC3_1080I_1243:
            frame_size = 917504;
            break;
        case TYPE_VC3_1080I_1244:
            frame_size = 606208;
            break;
        case TYPE_VC3_720P_1250:
            frame_size = 458752;
            break;
        case TYPE_VC3_720P_1251:
            frame_size = 458752;
            break;
        case TYPE_VC3_720P_1252:
            frame_size = 303104;
            break;
        case TYPE_VC3_1080P_1253:
            frame_size = 188416;
            break;
        case TYPE_VC3_720P_1258:
            frame_size = 212992;
            break;
        case TYPE_VC3_1080P_1259:
            frame_size = 417792;
            break;
        case TYPE_VC3_1080I_1260:
        default:
            frame_size = 417792;
            break;
    }

    write_data(file, duration * frame_size);
}

static void write_avid_alpha(FILE *file, int type, unsigned int duration)
{
    uint32_t frame_size;
    switch (type)
    {
        case TYPE_AVID_ALPHA_HD_1080I:
        default:
            frame_size = 1920 * 1080;
            break;
    }

    write_data(file, duration * frame_size);
}

static void write_anc_data(FILE *file, unsigned int duration)
{
    static const unsigned char anc_frame[24] =
        {0x00, 0x01,                                        // single line / packet
         0x00, 0x09, 0x01, 0x04, 0x00, 0x07,                // line 9, VANCFrame, ANC_8_BIT_COMP_LUMA, 7 samples
         0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01,    // payload array header: 8 x 1 bytes
         0x45, 0x04, 0x04, 0x21, 0x22, 0x23, 0x24, 0x00};   // payload array: DID (1), SDID (1), DC (1), UDW (4), padding (1)

    unsigned int i;
    for (i = 0; i < duration; i++)
        write_buffer(file, anc_frame, sizeof(anc_frame));
}

static void write_vbi_data(FILE *file, unsigned int duration)
{
    static const unsigned char vbi_frame[24] =
        {0x00, 0x01,                                        // single line
         0x00, 0x15, 0x01, 0x04, 0x00, 0x08,                // line 21, VBIFrame, VBI_8_BIT_COMP_LUMA, 8 samples
         0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01,    // payload array header: 8 x 1 bytes
         0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};   // payload array

    unsigned int i;
    for (i = 0; i < duration; i++)
        write_buffer(file, vbi_frame, sizeof(vbi_frame));
}


static void print_usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s [-d <frames>] -t <type> <filename>\n", cmd);
    fprintf(stderr, "Types:\n");
    fprintf(stderr, "  1: 16-bit PCM\n");
    fprintf(stderr, "  2: IEC DV 25\n");
    fprintf(stderr, "  3: DV Based DV 25\n");
    fprintf(stderr, "  4: DV 50\n");
    fprintf(stderr, "  5: DV 100 1080i\n");
    fprintf(stderr, "  6: DV 100 720p\n");
    fprintf(stderr, "  7: AVC Intra 100 1080i\n");
    fprintf(stderr, "  8: AVC Intra 100 1080p\n");
    fprintf(stderr, "  9: AVC Intra 50 1080i\n");
    fprintf(stderr, " 10: AVC Intra 50 1080p\n");
    fprintf(stderr, " 11: D10 50Mbps\n");
    fprintf(stderr, " 12: D10 40Mbps\n");
    fprintf(stderr, " 13: D10 30Mbps\n");
    fprintf(stderr, " 14: MPEG-2 Long GOP 422P@HL 1080i\n");
    fprintf(stderr, " 15: MPEG-2 Long GOP MP@H14 1080i\n");
    fprintf(stderr, " 16: MPEG-2 Long GOP MP@HL 1080i\n");
    fprintf(stderr, " 17: UNC SD\n");
    fprintf(stderr, " 18: UNC HD 1080I\n");
    fprintf(stderr, " 19: UNC HD 1080P\n");
    fprintf(stderr, " 20: UNC HD 720P\n");
    fprintf(stderr, " 21: MPEG-2 Long GOP 422P@HL 1080p\n");
    fprintf(stderr, " 22: MPEG-2 Long GOP MP@H14 1080p\n");
    fprintf(stderr, " 23: MPEG-2 Long GOP MP@HL 1080p\n");
    fprintf(stderr, " 24: MPEG-2 Long GOP MP@HL 1080i 1440x1080\n");
    fprintf(stderr, " 25: MPEG-2 Long GOP MP@HL 1080p 1440x1080\n");
    fprintf(stderr, " 26: MPEG-2 Long GOP 422P@HL 720p\n");
    fprintf(stderr, " 27: MPEG-2 Long GOP MP@HL 720p\n");
    fprintf(stderr, " 28: DV 100 1080p\n");
    fprintf(stderr, " 29: AVC Intra 100 720p\n");
    fprintf(stderr, " 30: AVC Intra 50 720p\n");
    fprintf(stderr, " 31: VC3/DNxHD 1235 1920x1080p 220/185/175 Mbps 10bit\n");
    fprintf(stderr, " 32: VC3/DNxHD 1237 1920x1080p 145/120/115 Mbps\n");
    fprintf(stderr, " 33: VC3/DNxHD 1238 1920x1080p 220/185/175 Mbps\n");
    fprintf(stderr, " 34: VC3/DNxHD 1241 1920x1080i 220/185 Mbps 10bit\n");
    fprintf(stderr, " 35: VC3/DNxHD 1242 1920x1080i 145/120 Mbps\n");
    fprintf(stderr, " 36: VC3/DNxHD 1243 1920x1080i 220/185 Mbps\n");
    fprintf(stderr, " 37: VC3/DNxHD 1250 1280x720p 220/185/110/90 Mbps 10bit\n");
    fprintf(stderr, " 38: VC3/DNxHD 1251 1280x720p 220/185/110/90 Mbps\n");
    fprintf(stderr, " 39: VC3/DNxHD 1252 1280x720p 220/185/110/90 Mbps\n");
    fprintf(stderr, " 40: VC3/DNxHD 1253 1920x1080p 45/36 Mbps\n");
    fprintf(stderr, " 41: AVID Alpha HD 1080I\n");
    fprintf(stderr, " 42: 24-bit PCM\n");
    fprintf(stderr, " 43: ANC data\n");
    fprintf(stderr, " 44: VBI data\n");
    fprintf(stderr, " 45: UNC UHD 3840\n");
    fprintf(stderr, " 46: AVC Intra 200 1080i\n");
    fprintf(stderr, " 47: AVC Intra 200 1080p\n");
    fprintf(stderr, " 48: AVC Intra 200 720p\n");
    fprintf(stderr, " 49: VC3/DNxHD 1244 1080i\n");
    fprintf(stderr, " 50: VC3/DNxHD 1258 720p\n");
    fprintf(stderr, " 51: VC3/DNxHD 1259 1080p\n");
    fprintf(stderr, " 52: VC3/DNxHD 1260 1080i\n");
    fprintf(stderr, " 53: MPEG-2 Long GOP 422P@HL 1080i (AS-10)\n");
    fprintf(stderr, " 54: VC2\n");
    fprintf(stderr, " 55: RDD-36 422 Profile\n");
    fprintf(stderr, " 56: RDD-36 4444 Profile\n");
    fprintf(stderr, " 57: 16-bit PCM with duration in samples\n");
}

int main(int argc, const char **argv)
{
    const char *filename;
    unsigned int duration = 25;
    int type = TYPE_UNKNOWN;
    int init_val = -1;
    int cmdln_index;

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++) {
        if (strcmp(argv[cmdln_index], "-d") == 0)
        {
            if (cmdln_index + 1 >= argc) {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &duration) != 1) {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid argument '%s' for '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-t") == 0)
        {
            if (cmdln_index + 1 >= argc) {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &type) != 1 ||
                type <= TYPE_UNKNOWN || type >= TYPE_END)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid argument '%s' for '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-s") == 0)
        {
            if (cmdln_index + 1 >= argc) {
                print_usage(argv[0]);
                fprintf(stderr, "Missing argument for '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &init_val) != 1)
            {
                print_usage(argv[0]);
                fprintf(stderr, "Invalid argument '%s' for '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else
        {
            break;
        }
    }

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    if (cmdln_index + 1 < argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Unknown argument '%s'\n", argv[cmdln_index]);
        return 1;
    }
    if (cmdln_index >= argc) {
        print_usage(argv[0]);
        fprintf(stderr, "Missing <filename> argument\n");
        return 1;
    }
    if (type == TYPE_UNKNOWN) {
        print_usage(argv[0]);
        fprintf(stderr, "Missing -t <type> arguments\n");
        return 1;
    }
    if (duration == 0) {
        print_usage(argv[0]);
        fprintf(stderr, "Duration 0 make no sense\n");
        return 1;
    }

    filename = argv[cmdln_index];


    init_data(init_val);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, strerror(errno));
        return 1;
    }

    switch (type)
    {
        case TYPE_16BIT_PCM:
            write_pcm(file, 2, duration);
            break;
        case TYPE_IEC_DV25:
        case TYPE_DVBASED_DV25:
            write_dv25(file, duration);
            break;
        case TYPE_DV50:
            write_dv50(file, duration);
            break;
        case TYPE_DV100_1080I:
        case TYPE_DV100_1080P:
            write_dv100_1080i(file, duration);
            break;
        case TYPE_DV100_720P:
            write_dv100_720p(file, duration);
            break;
        case TYPE_AVCI200_1080I:
        case TYPE_AVCI200_1080P:
            write_avci200_1080(file, duration);
            break;
        case TYPE_AVCI200_720P:
            write_avci200_720(file, duration);
            break;
        case TYPE_AVCI100_1080I:
        case TYPE_AVCI100_1080P:
            write_avci100_1080(file, duration);
            break;
        case TYPE_AVCI100_720P:
            write_avci100_720(file, duration);
            break;
        case TYPE_AVCI50_1080I:
        case TYPE_AVCI50_1080P:
            write_avci50_1080(file, duration);
            break;
        case TYPE_AVCI50_720P:
            write_avci50_720(file, duration);
            break;
        case TYPE_D10_50:
        case TYPE_D10_40:
        case TYPE_D10_30:
            write_d10(file, type, duration);
            break;
        case TYPE_MPEG2LG_422P_HL_1080I:
        case TYPE_MPEG2LG_422P_HL_1080P:
        case TYPE_MPEG2LG_MP_H14_1080I:
        case TYPE_MPEG2LG_MP_H14_1080P:
        case TYPE_MPEG2LG_MP_HL_1080I:
        case TYPE_MPEG2LG_MP_HL_1080P:
        case TYPE_MPEG2LG_MP_HL_1080I_1440:
        case TYPE_MPEG2LG_MP_HL_1080P_1440:
        case TYPE_MPEG2LG_422P_HL_720P:
        case TYPE_MPEG2LG_MP_HL_720P:
            write_mpeg2lg(file, type, duration, true, false);
            break;
        case TYPE_AS10_MPEG2LG_422P_HL_1080I:
            write_mpeg2lg(file, TYPE_MPEG2LG_422P_HL_1080I, duration, false, true);
            break;
        case TYPE_UNC_SD:
        case TYPE_UNC_HD_1080I:
        case TYPE_UNC_HD_1080P:
        case TYPE_UNC_HD_720P:
        case TYPE_UNC_UHD_3840:
            write_unc(file, type, duration);
            break;
        case TYPE_RDD36_422:
        case TYPE_RDD36_4444:
            write_rdd36(file, type, duration);
            break;
        case TYPE_VC2:
            write_vc2(file, duration);
            break;
        case TYPE_VC3_1080P_1235:
        case TYPE_VC3_1080P_1237:
        case TYPE_VC3_1080P_1238:
        case TYPE_VC3_1080I_1241:
        case TYPE_VC3_1080I_1242:
        case TYPE_VC3_1080I_1243:
        case TYPE_VC3_1080I_1244:
        case TYPE_VC3_720P_1250:
        case TYPE_VC3_720P_1251:
        case TYPE_VC3_720P_1252:
        case TYPE_VC3_1080P_1253:
        case TYPE_VC3_720P_1258:
        case TYPE_VC3_1080P_1259:
        case TYPE_VC3_1080I_1260:
            write_vc3(file, type, duration);
            break;
        case TYPE_AVID_ALPHA_HD_1080I:
            write_avid_alpha(file, type, duration);
            break;
        case TYPE_24BIT_PCM:
            write_pcm(file, 3, duration);
            break;
        case TYPE_ANC_DATA:
            write_anc_data(file, duration);
            break;
        case TYPE_VBI_DATA:
            write_vbi_data(file, duration);
            break;
        case TYPE_16BIT_PCM_SAMPLES:
            write_pcm_samples(file, 2, duration);
            break;
        case TYPE_UNKNOWN:
        case TYPE_END:
            assert(false);
    }

    fclose(file);

    return 0;
}

