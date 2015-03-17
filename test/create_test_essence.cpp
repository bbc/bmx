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
    TYPE_UNKNOWN = 0,
    TYPE_16BIT_PCM,
    TYPE_IEC_DV25,
    TYPE_DVBASED_DV25,
    TYPE_DV50,
    TYPE_DV100_1080I,
    TYPE_DV100_720P,
    TYPE_AVCI100_1080I,
    TYPE_AVCI100_1080P,
    TYPE_AVCI50_1080I,
    TYPE_AVCI50_1080P,
    TYPE_D10_50,
    TYPE_D10_40,
    TYPE_D10_30,
    TYPE_MPEG2LG_422P_HL_1080I,
    TYPE_MPEG2LG_MP_H14_1080I,
    TYPE_MPEG2LG_MP_HL_1080I,
    TYPE_UNC_SD,
    TYPE_UNC_HD_1080I,
    TYPE_UNC_HD_1080P,
    TYPE_UNC_HD_720P,
    TYPE_MPEG2LG_422P_HL_1080P,
    TYPE_MPEG2LG_MP_H14_1080P,
    TYPE_MPEG2LG_MP_HL_1080P,
    TYPE_MPEG2LG_MP_HL_1080I_1440,
    TYPE_MPEG2LG_MP_HL_1080P_1440,
    TYPE_MPEG2LG_422P_HL_720P,
    TYPE_MPEG2LG_MP_HL_720P,
    TYPE_DV100_1080P,
    TYPE_AVCI100_720P,
    TYPE_AVCI50_720P,
    TYPE_VC3_1080P_1235,
    TYPE_VC3_1080P_1237,
    TYPE_VC3_1080P_1238,
    TYPE_VC3_1080I_1241,
    TYPE_VC3_1080I_1242,
    TYPE_VC3_1080I_1243,
    TYPE_VC3_720P_1250,
    TYPE_VC3_720P_1251,
    TYPE_VC3_720P_1252,
    TYPE_VC3_1080P_1253,
    TYPE_AVID_ALPHA_HD_1080I,
    TYPE_24BIT_PCM,
    TYPE_ANC_DATA,
    TYPE_VBI_DATA,
    TYPE_UNC_UHD_3840,
    TYPE_AVCI200_1080I,
    TYPE_AVCI200_1080P,
    TYPE_AVCI200_720P,
    TYPE_VC3_1080I_1244,
    TYPE_VC3_720P_1258,
    TYPE_VC3_1080P_1259,
    TYPE_VC3_1080I_1260,
    TYPE_END,
} EssenceType;



// access unit delimiter NAL + start of sequence parameter set NAL
static const unsigned char AVCI_FIRST_FRAME_PREFIX_DATA[] =
    {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x00, 0x00, 0x00, 0x01, 0x67};

// access unit delimiter NAL + start of SEI messages NAL
static const unsigned char AVCI_NON_FIRST_FRAME_PREFIX_DATA[] =
    {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x00, 0x00, 0x00, 0x01, 0x06};

// don't care data
static unsigned char DATA[4096];



static void init_data()
{
    size_t i;
    for (i = 0; i < sizeof(DATA); i++)
        DATA[i] = (unsigned char)i;
}

static void set_mpeg_bit(unsigned char *data, uint32_t bit_offset, unsigned char bit)
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
        set_mpeg_bit(data, bit_offset + i, (value >> (num_bits - i - 1)) & 1);
}

static void set_mpeg_start_code(unsigned char *data, uint32_t offset, uint32_t code)
{
    data[offset] = (unsigned char)((code >> 24) & 0xff);
    data[offset + 1] = (unsigned char)((code >> 16) & 0xff);
    data[offset + 2] = (unsigned char)((code >> 8) & 0xff);
    data[offset + 3] = (unsigned char)(code & 0xff);
}

static void fill_mpeg_frame(unsigned char *data, uint32_t data_size,
                            bool seq_header, uint32_t bit_rate, bool low_delay, bool closed_gop,
                            uint32_t h_size, uint32_t v_size,
                            uint32_t temporal_ref, uint32_t frame_type)
{
    assert(data_size > 400);

    uint32_t set_offset = 0;
    if (seq_header) {
        set_mpeg_start_code(data, 0, MPEG_SEQUENCE_HEADER_CODE);
        set_offset = 0;
        set_mpeg_bits(data, set_offset * 8 + 32, 12, h_size);
        set_mpeg_bits(data, set_offset * 8 + 44, 12, v_size);
        set_mpeg_bits(data, set_offset * 8 + 64, 18, bit_rate);
        set_offset += 100;

        set_mpeg_start_code(data, set_offset, MPEG_SEQUENCE_EXT_CODE);
        set_mpeg_bits(data, set_offset * 8 + 32, 4, 0x01);
        set_mpeg_bits(data, set_offset * 8 + 47, 2, h_size >> 12);
        set_mpeg_bits(data, set_offset * 8 + 49, 2, v_size >> 12);
        set_mpeg_bits(data, set_offset * 8 + 51, 12, bit_rate >> 18);
        set_mpeg_bits(data, set_offset * 8 + 72, 1, low_delay);
        set_offset += 100;

        set_mpeg_start_code(data, set_offset, MPEG_GROUP_HEADER_CODE);
        set_mpeg_bits(data, set_offset * 8 + 57, 1, closed_gop);
        set_offset += 100;
    }

    set_mpeg_start_code(data, set_offset, MPEG_PICTURE_START_CODE);
    set_mpeg_bits(data, set_offset * 8 + 32, 10, temporal_ref);
    set_mpeg_bits(data, set_offset * 8 + 42, 3, frame_type);
}

static void write_buffer(FILE *file, const unsigned char *buffer, size_t size)
{
    if (fwrite(buffer, 1, size, file) != size) {
        fprintf(stderr, "Failed to write data: %s\n", strerror(errno));
        exit(1);
    }
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

static void write_pcm(FILE *file, uint16_t block_align, unsigned int duration)
{
    // 1920 samples per 25Hz frame, block_align bytes per sample
    write_data(file, duration * 1920 * block_align);
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

static void write_avci_frame(FILE *file, unsigned int frame_size, bool is_first)
{
    if (is_first) {
        write_buffer(file, AVCI_FIRST_FRAME_PREFIX_DATA, sizeof(AVCI_FIRST_FRAME_PREFIX_DATA));
        write_data(file, frame_size - sizeof(AVCI_FIRST_FRAME_PREFIX_DATA));
    } else {
        write_buffer(file, AVCI_NON_FIRST_FRAME_PREFIX_DATA, sizeof(AVCI_NON_FIRST_FRAME_PREFIX_DATA));
        write_data(file, frame_size - sizeof(AVCI_NON_FIRST_FRAME_PREFIX_DATA));
    }
}

static void write_avci200_1080(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 19 * 512 + 1134592, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 18 * 512 + 1134592, false);
}

static void write_avci100_1080(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 19 * 512 + 559104, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 18 * 512 + 559104, false);
}

static void write_avci200_720(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 11 * 512 + 566784, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 10 * 512 + 566784, false);
}

static void write_avci100_720(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 11 * 512 + 279040, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 10 * 512 + 279040, false);
}

static void write_avci50_1080(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 19 * 512 + 271360, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 18 * 512 + 271360, false);
}

static void write_avci50_720(FILE *file, unsigned int duration)
{
    write_avci_frame(file, 11 * 512 + 135168, true);

    unsigned int i;
    for (i = 1; i < duration; i++)
        write_avci_frame(file, 10 * 512 + 135168, false);
}

static void write_d10(FILE *file, int type, unsigned int duration)
{
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

    unsigned char *data = new unsigned char[frame_size];
    memset(data, 0, frame_size);
    fill_mpeg_frame(data, frame_size,
                    true, (frame_size * 25 * 8) / 400, true, false,
                    720, 608,
                    0, MPEG_I_FRAME_TYPE);

    unsigned int i;
    for (i = 0; i < duration; i++)
        write_buffer(file, data, frame_size);
}

static void write_mpeg2lg(FILE *file, int type, unsigned int duration)
{
    uint32_t i_frame_size, non_i_frame_size;
    uint32_t width, height;
    uint32_t bit_rate;
    switch (type)
    {
        case TYPE_MPEG2LG_422P_HL_1080I:
        case TYPE_MPEG2LG_422P_HL_1080P:
        case TYPE_MPEG2LG_MP_HL_1080I:
        case TYPE_MPEG2LG_MP_HL_1080P:
            i_frame_size = 270000;
            non_i_frame_size = 240000;
            width = 1920;
            height = 1080;
            bit_rate = (50 * 1000 * 1000) / 400;
            break;
        case TYPE_MPEG2LG_422P_HL_720P:
        case TYPE_MPEG2LG_MP_HL_720P:
            i_frame_size = 270000;
            non_i_frame_size = 240000;
            width = 1280;
            height = 720;
            bit_rate = (50 * 1000 * 1000) / 400;
            break;
        case TYPE_MPEG2LG_MP_H14_1080I:
        case TYPE_MPEG2LG_MP_H14_1080P:
        case TYPE_MPEG2LG_MP_HL_1080I_1440:
        case TYPE_MPEG2LG_MP_HL_1080P_1440:
        default:
            i_frame_size = 195000;
            non_i_frame_size = 165000;
            width = 1440;
            height = 1080;
            bit_rate = (35 * 1000 * 1000) / 400;
            break;
    }
    uint32_t max_frame_size = i_frame_size > non_i_frame_size ? i_frame_size : non_i_frame_size;

    unsigned char *data = new unsigned char[max_frame_size];

    unsigned int i;
    for (i = 0; i < duration; i++) {
        memset(data, 0, max_frame_size);
        switch (i % 12) {
            case 0:
                fill_mpeg_frame(data, i_frame_size, true, bit_rate, true, i == 0, width, height, 2, MPEG_I_FRAME_TYPE);
                break;
            case 1:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 0, MPEG_B_FRAME_TYPE);
                break;
            case 2:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 1, MPEG_B_FRAME_TYPE);
                break;
            case 3:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 5, MPEG_P_FRAME_TYPE);
                break;
            case 4:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 3, MPEG_B_FRAME_TYPE);
                break;
            case 5:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 4, MPEG_B_FRAME_TYPE);
                break;
            case 6:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 8, MPEG_P_FRAME_TYPE);
                break;
            case 7:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 6, MPEG_B_FRAME_TYPE);
                break;
            case 8:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 7, MPEG_B_FRAME_TYPE);
                break;
            case 9:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 11, MPEG_P_FRAME_TYPE);
                break;
            case 10:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 9, MPEG_B_FRAME_TYPE);
                break;
            case 11:
            default:
                fill_mpeg_frame(data, non_i_frame_size, false, bit_rate, true, false, width, height, 10, MPEG_B_FRAME_TYPE);
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

    write_data(file, duration * frame_size);
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
}

int main(int argc, const char **argv)
{
    const char *filename;
    unsigned int duration = 25;
    int type = TYPE_UNKNOWN;
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


    init_data();

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
            write_mpeg2lg(file, type, duration);
            break;
        case TYPE_UNC_SD:
        case TYPE_UNC_HD_1080I:
        case TYPE_UNC_HD_1080P:
        case TYPE_UNC_HD_720P:
        case TYPE_UNC_UHD_3840:
            write_unc(file, type, duration);
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
        case TYPE_UNKNOWN:
        case TYPE_END:
            assert(false);
    }

    fclose(file);

    return 0;
}

