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

#ifndef BMX_UTILS_H_
#define BMX_UTILS_H_


#include <cstdarg>

#include <string>
#include <vector>

#include <bmx/BMXTypes.h>


#define BMX_ARRAY_SIZE(array)   (sizeof(array) / sizeof((array)[0]))



namespace bmx
{


typedef enum
{
    ROUND_AUTO,
    ROUND_DOWN,
    ROUND_UP,
    ROUND_NEAREST
} Rounding;


/* General rules for rounding
        Position: lower rate sample is at or after the higher rate sample
            ROUND_UP    : converting TO the lower edit rate
            ROUND_DOWN  : converting FROM the lower edit rate
        Duration: lower rate sample contain only complete sets of higher rate samples
            ROUND_DOWN  : converting TO the lower edit rate
            ROUND_UP    : converting FROM the lower edit rate
   ROUND_AUTO follows the above rules
*/

int64_t convert_position(int64_t in_position, int64_t factor_top, int64_t factor_bottom, Rounding rounding);
int64_t convert_position(Rational in_edit_rate, int64_t in_position, Rational out_edit_rate, Rounding rounding);
int64_t convert_duration(int64_t in_duration, int64_t factor_top, int64_t factor_bottom, Rounding rounding);
int64_t convert_duration(Rational in_edit_rate, int64_t in_duration, Rational out_edit_rate, Rounding rounding);

bool get_sample_sequence(Rational lower_edit_rate, Rational higher_edit_rate, std::vector<uint32_t> *sample_sequence);
int64_t get_sequence_size(const std::vector<uint32_t> &sequence);
void offset_sample_sequence(std::vector<uint32_t> &sample_sequence, uint8_t offset);

int64_t convert_position_lower(int64_t position, const std::vector<uint32_t> &sequence, int64_t sequence_size = 0);
int64_t convert_position_higher(int64_t position, const std::vector<uint32_t> &sequence, int64_t sequence_size = 0);
int64_t convert_duration_lower(int64_t duration, const std::vector<uint32_t> &sequence, int64_t sequence_size = 0);
int64_t convert_duration_lower(int64_t duration, int64_t position, const std::vector<uint32_t> &sequence,
                               int64_t sequence_size = 0);
int64_t convert_duration_higher(int64_t duration, const std::vector<uint32_t> &sequence, int64_t sequence_size = 0);
int64_t convert_duration_higher(int64_t duration, int64_t position, const std::vector<uint32_t> &sequence,
                                int64_t sequence_size = 0);

std::string strip_path(std::string filename);
std::string strip_name(std::string filename);
std::string strip_suffix(std::string filename);

std::string get_cwd();
std::string get_abs_filename(std::string base_dir, std::string filename);

bool check_file_exists(std::string filename);
bool check_is_dir(std::string name);
bool check_is_abs_path(std::string name);
bool check_ends_with_dir_separator(std::string name);

Timestamp generate_timestamp_now();

UUID generate_uuid();
UMID generate_umid();

uint16_t get_rounded_tc_base(Rational rate);

std::string get_duration_string(int64_t count, Rational rate);
std::string get_generic_duration_string(int64_t count, Rational rate);
std::string get_generic_duration_string_2(int64_t count, Rational rate);

Rational convert_int_to_rational(int32_t value);

std::string get_timecode_string(Timecode timecode);
std::string get_umid_string(UMID umid);

Rational normalize_rate(Rational rate);

uint8_t get_system_item_cp_rate(Rational frame_rate);

uint32_t get_kag_fill_size(int64_t klv_size, uint32_t kag_size, uint8_t min_llen);
int64_t get_kag_aligned_size(int64_t klv_size, uint32_t kag_size, uint8_t min_llen);

void decode_smpte_timecode(Rational frame_rate, const unsigned char *smpte_tc, unsigned int size,
                           Timecode *timecode, bool *field_mark);
Timecode decode_smpte_timecode(Rational frame_rate, const unsigned char *smpte_tc, unsigned int size);
void encode_smpte_timecode(Timecode timecode, bool field_mark, unsigned char *smpte_tc, unsigned int size);

bool check_excess_d10_padding(const unsigned char *data, uint32_t data_size, uint32_t target_size);

void bmx_snprintf(char *str, size_t size, const char *format, ...);
void bmx_vsnprintf(char *str, size_t size, const char *format, va_list ap);

std::string bmx_strerror(int errnum);


};



#endif

