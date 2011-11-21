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

#include <cstring>

#include <im/essence_parser/SoundConversion.h>
#include <im/IMException.h>
#include <im/Logging.h>



uint8_t im::get_aes3_channel_valid_flags(const unsigned char *aes3_data, uint32_t aes3_data_size)
{
    IM_CHECK(aes3_data_size >= 3);
    return aes3_data[3];
}

uint16_t im::get_aes3_sample_count(const unsigned char *aes3_data, uint32_t aes3_data_size)
{
    IM_CHECK(aes3_data_size >= 2);
    return ((uint16_t)aes3_data[2] << 8) | aes3_data[1];
}

void im::convert_aes3_to_pcm(const unsigned char *aes3_data, uint32_t aes3_data_size,
                             uint32_t bits_per_sample, uint8_t channel_num,
                             unsigned char *pcm_data, uint32_t pcm_data_size)
{
    uint16_t audio_sample_count = get_aes3_sample_count(aes3_data, aes3_data_size);
    uint8_t valid_flags         = get_aes3_channel_valid_flags(aes3_data, aes3_data_size);
    uint32_t block_align        = (bits_per_sample + 7) / 8;

    IM_CHECK(audio_sample_count == (aes3_data_size - 4) / (8 * 4)); // 4 bytes per sample, 8 channels
    IM_CHECK(block_align == 2 || block_align == 3); // only 16-bit to 24-bit sample size allowed
    IM_CHECK(channel_num < 8);
    IM_CHECK(pcm_data_size >= block_align * audio_sample_count);

    if (!(valid_flags & (1 << channel_num))) {
        memset(pcm_data, 0, audio_sample_count * block_align);
        return;
    }

    const unsigned char *aes_data_ptr = &aes3_data[4];
    unsigned char *pcm_data_ptr = &pcm_data[0];
    uint16_t sample_num;

    if (block_align == 2) {
        for (sample_num = 0; sample_num < audio_sample_count; sample_num++) {
            aes_data_ptr += channel_num * 4;
            pcm_data_ptr[0] = (aes_data_ptr[1] >> 4) |
                              (aes_data_ptr[2] << 4);
            pcm_data_ptr[1] = (aes_data_ptr[2] >> 4) |
                              (aes_data_ptr[3] << 4);
            pcm_data_ptr += block_align;
            aes_data_ptr += (8 - channel_num) * 4;
        }
    } else {
        for (sample_num = 0; sample_num < audio_sample_count; sample_num++) {
            aes_data_ptr += channel_num * 4;
            pcm_data_ptr[0] = (aes_data_ptr[0] >> 4) |
                              (aes_data_ptr[1] << 4);
            pcm_data_ptr[1] = (aes_data_ptr[1] >> 4) |
                              (aes_data_ptr[2] << 4);
            pcm_data_ptr[2] = (aes_data_ptr[2] >> 4) |
                              (aes_data_ptr[3] << 4);
            pcm_data_ptr += block_align;
            aes_data_ptr += (8 - channel_num) * 4;
        }
    }
}

void im::deinterleave_audio(const unsigned char *input_data, uint32_t input_data_size,
                            uint32_t bits_per_sample, uint8_t channel_count, uint8_t channel_num,
                            unsigned char *output_data, uint32_t output_data_size)
{
    uint32_t input_block_align = channel_count * ((bits_per_sample + 7) / 8);
    uint32_t output_block_align = (bits_per_sample + 7) / 8;
    uint32_t channel_offset = channel_num * output_block_align;
    uint32_t sample_count = input_data_size / input_block_align;
    uint32_t i, j;

    IM_CHECK(output_data_size >= sample_count * output_block_align);

    for (i = 0; i < sample_count; i++) {
        for (j = 0; j < output_block_align; j++)
            output_data[i * output_block_align + j] = input_data[i * input_block_align + channel_offset + j];
    }
}

