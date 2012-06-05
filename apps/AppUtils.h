/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#ifndef __BMX_APP_UTILS_H__
#define __BMX_APP_UTILS_H__

#include <string>
#include <vector>

#include <bmx/BMXTypes.h>
#include <bmx/EssenceType.h>
#include <bmx/URI.h>



namespace bmx
{


typedef struct
{
    EssenceType essence_type;
    Rational sample_rate;
} AVCIHeaderFormat;

typedef struct
{
    std::vector<AVCIHeaderFormat> formats;
    const char *filename;
    int64_t offset;
} AVCIHeaderInput;


size_t get_num_avci_header_formats();
const char* get_avci_header_format_string(size_t index);


bool parse_frame_rate(const char *rate_str, Rational *frame_rate);

bool parse_timecode(const char *tc_str, Rational frame_rate, Timecode *timecode);

bool parse_position(const char *position_str, Timecode start_timecode, Rational frame_rate, int64_t *position);

bool parse_partition_interval(const char *partition_interval_str, Rational frame_rate, int64_t *partition_interval);

bool parse_bool(const char *bool_str, bool *value);

bool parse_color(const char *color_str, Color *color);

bool parse_avci_header(const char *format_str, const char *filename, const char *offset_str,
                       std::vector<AVCIHeaderInput> *avci_header_inputs);

bool parse_d10_sound_flags(const char *flags_str, uint8_t *flags);

bool parse_timestamp(const char *timestamp_str, Timestamp *timestamp);

bool parse_umid(const char *umid_str, UMID *umid);

bool parse_avid_import_name(const char *import_name, URI *uri);


std::string create_mxf_track_filename(const char *prefix, uint32_t track_number, bool is_picture);



bool have_avci_header_data(EssenceType essence_type, Rational sample_rate,
                           std::vector<AVCIHeaderInput> &avci_header_inputs);
bool read_avci_header_data(EssenceType essence_type, Rational sample_rate,
                           std::vector<AVCIHeaderInput> &avci_header_inputs,
                           unsigned char *buffer, size_t buffer_size);


void init_progress(float *next_update);
void print_progress(int64_t count, int64_t duration, float *next_update);


};


#endif

