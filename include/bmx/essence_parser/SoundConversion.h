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

#ifndef BMX_AUDIO_CONVERSION_H_
#define BMX_AUDIO_CONVERSION_H_


#include <bmx/BMXTypes.h>



namespace bmx
{


uint8_t get_aes3_channel_valid_flags(const unsigned char *aes3_data, uint32_t aes3_data_size);
uint16_t get_aes3_sample_count(const unsigned char *aes3_data, uint32_t aes3_data_size);

uint32_t convert_aes3_to_pcm(const unsigned char *aes3_data, uint32_t aes3_data_size,
                             uint32_t bits_per_sample, uint8_t channel_num,
                             unsigned char *pcm_data, uint32_t pcm_data_size);

uint32_t convert_aes3_to_mc_pcm(const unsigned char *aes3_data, uint32_t aes3_data_size,
                                uint32_t bits_per_sample, uint8_t channel_count,
                                unsigned char *pcm_data, uint32_t pcm_data_size);

void deinterleave_audio(const unsigned char *input_data, uint32_t input_data_size,
                        uint32_t bits_per_sample, uint16_t channel_count, uint16_t channel_num,
                        unsigned char *output_data, uint32_t output_data_size);



};



#endif
