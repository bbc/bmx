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

#ifndef BMX_APP_UTILS_H_
#define BMX_APP_UTILS_H_

#include <string>
#include <vector>
#include <set>

#include <bmx/BMXTypes.h>
#include <bmx/EssenceType.h>
#include <bmx/Logging.h>
#include <bmx/URI.h>
#include <bmx/clip_writer/ClipWriterTrack.h>
#include <bmx/as02/AS02Manifest.h>
#include <bmx/Checksum.h>



namespace bmx
{


typedef enum
{
    NO_CLIP_SUB_TYPE,
    AS11_CLIP_SUB_TYPE,
} ClipSubType;

typedef enum
{
    ALL_ANC_DATA,
    ST2020_ANC_DATA,
    ST2016_ANC_DATA,
    RDD8_SDP_ANC_DATA,
    ST12M_ANC_DATA,
    ST334_ANC_DATA,
} ANCDataType;

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


std::string get_app_version_info(const char *app_name);

std::string clip_type_to_string(ClipWriterType clip_type, ClipSubType sub_clip_type);

size_t get_num_avci_header_formats();
const char* get_avci_header_format_string(size_t index);

size_t get_num_ps_avci_header_formats();
const char* get_ps_avci_header_format_string(size_t index);

bool parse_log_level(const char *level_str, LogLevel *level);
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
bool parse_uuid(const char *uuid_str, UUID *uuid);
bool parse_product_version(const char *version_str, mxfProductVersion *uuid);
bool parse_product_info(const char **info_strings, size_t num_info_strings,
                        std::string *company_name, std::string *product_name, mxfProductVersion *product_version,
                        std::string *version, UUID *product_uid);
bool parse_avid_import_name(const char *import_name, URI *uri);
bool parse_clip_type(const char *clip_type_str, ClipWriterType *clip_type, ClipSubType *clip_sub_type);
bool parse_mic_type(const char *mic_type_str, MICType *mic_type);
bool parse_klv_opt(const char *klv_opt_str, mxfKey *key, uint32_t *track_num);
bool parse_anc_data_types(const char *types_str, std::set<ANCDataType> *types);
bool parse_checksum_type(const char *type_str, ChecksumType *type);
bool parse_rdd6_lines(const char *lines_str, uint16_t *lines);

std::string create_mxf_track_filename(const char *prefix, uint32_t track_number, MXFDataDefEnum data_def);

bool have_avci_header_data(EssenceType essence_type, Rational sample_rate,
                           std::vector<AVCIHeaderInput> &avci_header_inputs);
bool read_avci_header_data(EssenceType essence_type, Rational sample_rate,
                           std::vector<AVCIHeaderInput> &avci_header_inputs,
                           unsigned char *buffer, size_t buffer_size);

bool have_ps_avci_header_data(EssenceType essence_type, Rational sample_rate);
bool get_ps_avci_header_data(EssenceType essence_type, Rational sample_rate,
                             unsigned char *buffer, size_t buffer_size);
void check_avid_avci_stop_bit(const unsigned char *input_data, const unsigned char *ps_data, size_t data_size,
                              bool *missing_stop_bit, bool *other_differences);

void init_progress(float *next_update);
void print_progress(int64_t count, int64_t duration, float *next_update);

void sleep_msec(uint32_t msec);
uint32_t get_tick_count();
uint32_t delta_tick_count(uint32_t from, uint32_t to);
void rt_sleep(float rt_factor, uint32_t start_tick, Rational sample_rate, int64_t num_samples);



};


#endif

