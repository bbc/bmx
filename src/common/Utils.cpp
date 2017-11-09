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

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <cstring>
#include <cstdio>
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
#include <sys/timeb.h>
#include <time.h>
#include <direct.h> // _getcwd
#include <windows.h>
#else
#include <uuid/uuid.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <algorithm>

#include <bmx/Utils.h>
#include <bmx/URI.h>
#include <bmx/MD5.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;


#define MAX_INT32               2147483647

#define XML_PROLOG_MAX_SIZE     512


typedef struct
{
    mxfRational lower_edit_rate;
    mxfRational higher_edit_rate;
    uint32_t sample_sequence[6];
} SampleSequence;

static const SampleSequence SAMPLE_SEQUENCES[] =
{
    {{30000, 1001}, {48000,1}, {1602, 1601, 1602, 1601, 1602, 0}},
    {{60000, 1001}, {48000,1}, { 801,  801,  800,  801,  801, 0}},
};


typedef struct
{
    uint8_t cp_rate_id;
    int32_t frame_rate_per_sec;
} ContentPackageRate;

static const ContentPackageRate CONTENT_PACKAGE_RATES[] =
{
    { 1, 24}, { 2,  25}, { 3,  30},
    { 4, 48}, { 5,  50}, { 6,  60},
    { 7, 72}, { 8,  75}, { 9,  90},
    {10, 96}, {11, 100}, {12, 120}
};


static const string BMX_NAMESPACE = "http://bbc.co.uk/rd/bmx";


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};



static int32_t gcd(int32_t a, int32_t b)
{
    if (b == 0)
        return a;
    else
        return gcd(b, a % b);
}

static void get_xml_prolog_encoding(const char *data, size_t size, bmx::TextEncoding *encoding,
                                    bool *prolog_present, bool *encoding_present)
{
#define XML_SPACE(c)    ((c) == 0x20 || (c) == 0x09 || (c) == 0x0d || (c) == 0x0a)

    *prolog_present   = false;
    *encoding_present = false;

    if (size < 7)
        return;

    size_t prolog_size_limit = size;
    if (prolog_size_limit > XML_PROLOG_MAX_SIZE)
        prolog_size_limit = XML_PROLOG_MAX_SIZE;
    const char *data_ptr = data;
    const char *end_data = (const char*)(data + prolog_size_limit);

    // check for '<?xml '
    if (data_ptr + 6 >= end_data || strncmp(data_ptr, "<?xml ", 6) != 0)
        return;
    data_ptr += 6;

    // search for ?>
    const char *end_prolog_ptr = data_ptr;
    while (end_prolog_ptr != end_data && *end_prolog_ptr && !(end_prolog_ptr[-1] == '?' && *end_prolog_ptr == '>'))
        end_prolog_ptr++;
    if (end_prolog_ptr == end_data ||
        !(end_prolog_ptr[-1] == '?' && *end_prolog_ptr == '>') ||
        data_ptr + 8 >= end_prolog_ptr)
    {
        return;
    }

    *prolog_present = true;

    // search for encoding="
    data_ptr += 8;
    while (data_ptr != end_prolog_ptr && data_ptr[-1] != 'g' && strncmp(&data_ptr[-8], "encoding", 8) != 0)
        data_ptr++;
    while (data_ptr != end_prolog_ptr && XML_SPACE(*data_ptr))
        data_ptr++;
    if (data_ptr == end_prolog_ptr || *data_ptr != '=')
        return;
    data_ptr++;
    while (data_ptr != end_prolog_ptr && XML_SPACE(*data_ptr))
        data_ptr++;
    if (data_ptr == end_prolog_ptr || !(*data_ptr == '\'' || *data_ptr == '"'))
        return;
    char quote_char = *data_ptr;
    data_ptr++;
    while (data_ptr != end_prolog_ptr && XML_SPACE(*data_ptr))
        data_ptr++;
    if (data_ptr == end_prolog_ptr)
      return;

    *encoding_present = true;
    *encoding = bmx::UNKNOWN_TEXT_ENCODING;

    // check for utf-8 and utf-16
    if (data_ptr == end_prolog_ptr || !(*data_ptr == 'U' || *data_ptr == 'u'))
        return;
    data_ptr++;
    if (data_ptr == end_prolog_ptr || !(*data_ptr == 'T' || *data_ptr == 't'))
        return;
    data_ptr++;
    if (data_ptr == end_prolog_ptr || !(*data_ptr == 'F' || *data_ptr == 'f'))
        return;
    data_ptr++;
    if (data_ptr != end_prolog_ptr && *data_ptr == '-')
        data_ptr++;
    if (data_ptr != end_prolog_ptr && *data_ptr == '8') {
        *encoding = bmx::UTF8;
    } else if (data_ptr != end_prolog_ptr && *data_ptr == '1') {
        data_ptr++;
        if (data_ptr != end_prolog_ptr && *data_ptr == '6')
            *encoding = bmx::UTF16;
    }
    if (*encoding != bmx::UNKNOWN_TEXT_ENCODING) {
        data_ptr++;
        while (data_ptr != end_prolog_ptr && XML_SPACE(*data_ptr))
            data_ptr++;
        if (data_ptr == end_prolog_ptr || *data_ptr != quote_char)
            *encoding = bmx::UNKNOWN_TEXT_ENCODING;
    }

#undef XML_SPACE
}

static void get_xml_prolog_encoding_16bit_be(const unsigned char *data, size_t size, bmx::TextEncoding *encoding)
{
  char utf8_buf[XML_PROLOG_MAX_SIZE];
  size_t utf8_size = 0;

  // convert to single byte chars
  size_t i;
  for (i = 0; i < size && utf8_size < sizeof(utf8_buf); i += 2, utf8_size++) {
      if (data[i] || data[i + 1] > 0x7f)
          break;
      utf8_buf[utf8_size] = (char)data[i + 1];
  }

  bool prolog_present;
  bool encoding_present;
  get_xml_prolog_encoding(utf8_buf, utf8_size, encoding, &prolog_present, &encoding_present);

  if (*encoding != bmx::UTF16)
      *encoding = bmx::UNKNOWN_TEXT_ENCODING; // ignore contradictory prolog encoding utf-8
}

static void get_xml_prolog_encoding_16bit_le(const unsigned char *data, size_t size, bmx::TextEncoding *encoding)
{
  char utf8_buf[XML_PROLOG_MAX_SIZE];
  size_t utf8_size = 0;

  // convert to single byte chars
  size_t i;
  for (i = 0; i < size && utf8_size < sizeof(utf8_buf); i += 2, utf8_size++) {
      if (data[i] > 0x7f || data[i + 1])
          break;
      utf8_buf[utf8_size] = (char)data[i];
  }

  bool prolog_present;
  bool encoding_present;
  get_xml_prolog_encoding(utf8_buf, utf8_size, encoding, &prolog_present, &encoding_present);

  if (*encoding != bmx::UTF16)
      *encoding = bmx::UNKNOWN_TEXT_ENCODING; // ignore contradictory prolog encoding utf-8
}



int64_t bmx::convert_position(int64_t in_position, int64_t factor_top, int64_t factor_bottom, Rounding rounding)
{
    if (in_position == 0 || factor_top == factor_bottom)
        return in_position;

    if (in_position < 0) {
        if (rounding == ROUND_UP || (rounding == ROUND_AUTO && factor_top < factor_bottom))
            return -convert_position(-in_position, factor_top, factor_bottom, ROUND_DOWN);
        else
            return -convert_position(-in_position, factor_top, factor_bottom, ROUND_UP);
    }

    // don't expect factors to be > MAX_INT32. Expect to see numbers such as 25, 48000, 30000 (30000/1001)
    if (factor_top > MAX_INT32 || factor_bottom > MAX_INT32)
        log_warn("Convert position calculation may overflow\n");

    int64_t round_num = 0;
    if (rounding == ROUND_UP || (rounding == ROUND_AUTO && factor_top < factor_bottom))
        round_num = factor_bottom - 1;
    else if (rounding == ROUND_NEAREST)
        round_num = factor_bottom / 2;


    if (in_position <= MAX_INT32) {
        // no chance of overflow (assuming there exists a result that fits into int64_t)
        return (in_position * factor_top + round_num) / factor_bottom;
    } else {
        // separate the calculation into 2 parts so there is no chance of an overflow (assuming there exists
        // a result that fits into int64_t)
        // a*b/c = ((a/c)*c + a%c) * b) / c = (a/c)*b + (a%c)*b/c
        return (in_position / factor_bottom) * factor_top +
               ((in_position % factor_bottom) * factor_top + round_num) / factor_bottom;
    }
}

int64_t bmx::convert_position(Rational in_edit_rate, int64_t in_position, Rational out_edit_rate, Rounding rounding)
{
    return convert_position(in_position,
                            (int64_t)out_edit_rate.numerator * in_edit_rate.denominator,
                            (int64_t)out_edit_rate.denominator * in_edit_rate.numerator,
                            rounding);
}

int64_t bmx::convert_duration(int64_t in_duration, int64_t factor_top, int64_t factor_bottom, Rounding rounding)
{
    if (in_duration <= 0)
        return 0;

    if (rounding == ROUND_AUTO) {
        return convert_position(in_duration, factor_top, factor_bottom,
                                (factor_top < factor_bottom ? ROUND_DOWN : ROUND_UP));
    } else {
        return convert_position(in_duration, factor_top, factor_bottom, rounding);
    }
}

int64_t bmx::convert_duration(Rational in_edit_rate, int64_t in_duration, Rational out_edit_rate, Rounding rounding)
{
    if (in_duration <= 0)
        return 0;

    return convert_duration(in_duration,
                            (int64_t)out_edit_rate.numerator * in_edit_rate.denominator,
                            (int64_t)out_edit_rate.denominator * in_edit_rate.numerator,
                            rounding);
}

bool bmx::get_sample_sequence(Rational lower_edit_rate, Rational higher_edit_rate, vector<uint32_t> *sample_sequence)
{
    if (lower_edit_rate == higher_edit_rate) {
        sample_sequence->push_back(1);
    } else {
        int64_t num_higher_samples = ((int64_t)higher_edit_rate.numerator * lower_edit_rate.denominator) /
                                        ((int64_t)higher_edit_rate.denominator * lower_edit_rate.numerator);
        int64_t remainder = ((int64_t)higher_edit_rate.numerator * lower_edit_rate.denominator) %
                                    ((int64_t)higher_edit_rate.denominator * lower_edit_rate.numerator);
        if (num_higher_samples > 0) {
            if (remainder == 0) {
                // fixed integer number of reader samples for each clip sample
                BMX_ASSERT(num_higher_samples <= UINT32_MAX);
                sample_sequence->push_back((uint32_t)num_higher_samples);
            } else {
                // try known sample sequences
                size_t i;
                for (i = 0; i < BMX_ARRAY_SIZE(SAMPLE_SEQUENCES); i++) {
                    if (lower_edit_rate == SAMPLE_SEQUENCES[i].lower_edit_rate &&
                        higher_edit_rate == SAMPLE_SEQUENCES[i].higher_edit_rate)
                    {
                        size_t j = 0;
                        while (SAMPLE_SEQUENCES[i].sample_sequence[j] != 0) {
                            sample_sequence->push_back(SAMPLE_SEQUENCES[i].sample_sequence[j]);
                            j++;
                        }
                        break;
                    }
                }
            }
        }
        // else lower_edit_rate > higher_edit_rate
    }

    return !sample_sequence->empty();
}

int64_t bmx::get_sequence_size(const std::vector<uint32_t> &sequence)
{
    int64_t size = 0;
    size_t i;
    for (i = 0; i < sequence.size(); i++)
        size += sequence[i];

    return size;
}

void bmx::offset_sample_sequence(vector<uint32_t> &sample_sequence, uint8_t offset)
{
    rotate(sample_sequence.begin(),
           sample_sequence.begin() + (offset % sample_sequence.size()),
           sample_sequence.end());
}

int64_t bmx::convert_position_lower(int64_t position, const std::vector<uint32_t> &sequence, int64_t sequence_size_in)
{
    int64_t sequence_size = sequence_size_in;
    if (sequence_size <= 0)
        sequence_size = get_sequence_size(sequence);
    BMX_ASSERT(sequence_size > 0);

    int64_t lower_position = position / sequence_size * sequence.size();

    if (position >= 0) {
        int64_t sequence_remainder = position % sequence_size;
        size_t sequence_offset = 0;
        while (sequence_remainder > 0) {
            sequence_remainder -= sequence[sequence_offset];
            // rounding up
            lower_position++;
            sequence_offset = (sequence_offset + 1) % sequence.size();
        }
    } else {
        int64_t sequence_remainder = ( - position) % sequence_size;
        size_t sequence_offset = 0;
        while (sequence_remainder > 0) {
            sequence_remainder -= sequence[sequence.size() - sequence_offset - 1];
            // rounding up (positive)
            if (sequence_remainder >= 0)
                lower_position--;
            sequence_offset = (sequence_offset + 1) % sequence.size();
        }
    }

    return lower_position;
}

int64_t bmx::convert_position_higher(int64_t position, const std::vector<uint32_t> &sequence, int64_t sequence_size_in)
{
    int64_t sequence_size = sequence_size_in;
    if (sequence_size <= 0)
        sequence_size = get_sequence_size(sequence);
    BMX_ASSERT(sequence_size > 0);

    int64_t higher_position = position / sequence.size() * sequence_size;

    if (position >= 0) {
        size_t sequence_remainder = (size_t)(position % sequence.size());
        uint32_t i;
        for (i = 0; i < sequence_remainder; i++)
            higher_position += sequence[i];
    } else {
        size_t sequence_remainder = (size_t)(( - position) % sequence.size());
        uint32_t i;
        for (i = 0; i < sequence_remainder; i++)
            higher_position -= sequence[sequence.size() - i - 1];
    }

    return higher_position;
}

int64_t bmx::convert_duration_lower(int64_t duration, const std::vector<uint32_t> &sequence, int64_t sequence_size_in)
{
    if (duration <= 0)
        return 0;

    int64_t sequence_size = sequence_size_in;
    if (sequence_size <= 0)
        sequence_size = get_sequence_size(sequence);
    BMX_ASSERT(sequence_size > 0);

    int64_t lower_duration = duration / sequence_size * sequence.size();

    int64_t sequence_remainder = duration % sequence_size;
    size_t sequence_offset = 0;
    while (sequence_remainder > 0) {
        sequence_remainder -= sequence[sequence_offset];
        // rounding down
        if (sequence_remainder >= 0)
            lower_duration++;
        sequence_offset = (sequence_offset + 1) % sequence.size();
    }

    return lower_duration;
}

int64_t bmx::convert_duration_lower(int64_t duration, int64_t position, const std::vector<uint32_t> &sequence,
                                    int64_t sequence_size_in)
{
    if (duration <= 0)
        return 0;

    int64_t sequence_size = sequence_size_in;
    if (sequence_size <= 0)
        sequence_size = get_sequence_size(sequence);
    BMX_ASSERT(sequence_size > 0);

    int64_t lower_position = convert_position_lower(position, sequence, sequence_size);
    int64_t round_up_position = convert_position_higher(lower_position, sequence, sequence_size);

    int64_t adjusted_duration;
    size_t sequence_offset;
    if (position >= 0) {
        sequence_offset = (size_t)(lower_position % sequence.size());
        // samples before the rounded position are not considered to be part of the duration
        adjusted_duration = duration - (round_up_position - position);
    } else {
        sequence_offset = sequence.size() - (size_t)((( - lower_position) % sequence.size()));
        // samples before the rounded position are not considered to be part of the duration
        adjusted_duration = duration - (position - round_up_position);
    }

    int64_t lower_duration = adjusted_duration / sequence_size * sequence.size();

    int64_t sequence_remainder = adjusted_duration % sequence_size;
    while (sequence_remainder > 0) {
        sequence_remainder -= sequence[sequence_offset];
        // rounding down
        if (sequence_remainder >= 0)
            lower_duration++;
        sequence_offset = (sequence_offset + 1) % sequence.size();
    }

    return lower_duration;
}

int64_t bmx::convert_duration_higher(int64_t duration, const std::vector<uint32_t> &sequence, int64_t sequence_size_in)
{
    if (duration <= 0)
        return 0;

    int64_t sequence_size = sequence_size_in;
    if (sequence_size <= 0)
        sequence_size = get_sequence_size(sequence);
    BMX_ASSERT(sequence_size > 0);

    int64_t higher_duration = duration / sequence.size() * sequence_size;

    size_t sequence_remainder = (size_t)(duration % sequence.size());
    uint32_t i;
    for (i = 0; i < sequence_remainder; i++)
        higher_duration += sequence[i];

    return higher_duration;
}

int64_t bmx::convert_duration_higher(int64_t duration, int64_t position, const std::vector<uint32_t> &sequence,
                                     int64_t sequence_size_in)
{
    if (duration <= 0)
        return 0;

    int64_t sequence_size = sequence_size_in;
    if (sequence_size <= 0)
        sequence_size = get_sequence_size(sequence);
    BMX_ASSERT(sequence_size > 0);

    int64_t higher_duration = duration / sequence.size() * sequence_size;

    size_t sequence_remainder = (size_t)(duration % sequence.size());
    size_t sequence_offset;
    if (position >= 0)
        sequence_offset = (size_t)(position % sequence.size());
    else
        sequence_offset = sequence.size() - (size_t)((( - position) % sequence.size()));

    while (sequence_remainder > 0) {
        higher_duration += sequence[sequence_offset];
        sequence_remainder--;
        sequence_offset = (sequence_offset + 1) % sequence.size();
    }

    return higher_duration;
}

string bmx::strip_path(string filename)
{
    size_t sep_index;
#if defined(_WIN32)
    if ((sep_index = filename.find_last_of("/\\:")) != string::npos)
#else
    if ((sep_index = filename.find_last_of("/")) != string::npos)
#endif
        return filename.substr(sep_index + 1);
    else
        return filename;
}

string bmx::strip_name(string filename)
{
    size_t sep_index;
#if defined(_WIN32)
    if ((sep_index = filename.find_last_of("/\\")) != string::npos)
        return filename.substr(0, sep_index);
    else if ((sep_index = filename.find_last_of(":")) != string::npos)
        return filename.substr(0, sep_index + 1);
#else
    if ((sep_index = filename.find_last_of("/")) != string::npos) {
        if (sep_index == 0)
            return filename.substr(0, sep_index + 1);
        else
            return filename.substr(0, sep_index);
    }
#endif
    else
        return "";
}

string bmx::strip_suffix(string filename)
{
    size_t suffix_index;
#if defined(_WIN32)
    if ((suffix_index = filename.find_last_of(".")) != string::npos &&
        filename.find_first_of("/\\:", suffix_index + 1) == string::npos)
    {
        return filename.substr(0, suffix_index);
    }
#else
    if ((suffix_index = filename.find_last_of(".")) != string::npos &&
        filename.find_first_of("/", suffix_index + 1) == string::npos)
    {
        return filename.substr(0, suffix_index);
    }
#endif
    else
        return filename;
}

string bmx::get_cwd()
{
#define MAX_REASONABLE_PATH_SIZE    (10 * 1024)

    string cwd;
    char *temp_path;
    size_t path_size = 1024;
    while (true) {
        temp_path = new char[path_size];
#if defined(_WIN32)
        if (_getcwd(temp_path, (int)path_size))
#else
        if (getcwd(temp_path, path_size))
#endif
            break;
        if (errno == EEXIST)
            break;

        delete [] temp_path;
        if (errno != ERANGE) {
            throw BMXException("Failed to get current working directory: %s", bmx_strerror(errno).c_str());
        } else if (path_size >= MAX_REASONABLE_PATH_SIZE) {
            throw BMXException("Maximum path size (%d) for current working directory exceeded",
                               MAX_REASONABLE_PATH_SIZE);
        }
        path_size += 1024;
    }
    cwd = temp_path;
    delete [] temp_path;

    return cwd;
}

string bmx::get_abs_filename(string base_dir, string filename)
{
    URI uri;
    uri.ParseFilename(filename);

    if (uri.IsRelative()) {
        URI base_uri;
        base_uri.ParseDirectory(base_dir);

        uri.MakeAbsolute(base_uri);
    }

    return uri.ToFilename();
}

bool bmx::check_file_exists(string filename)
{
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file)
        return false;

    fclose(file);
    return true;
}

bool bmx::check_is_dir(string name)
{
#if defined(_WIN32)
    struct _stati64 buf;
    if (_stati64(name.c_str(), &buf) != 0)
        return false;
    return ((buf.st_mode & _S_IFMT) == _S_IFDIR);
#else
    struct stat buf;
    if (stat(name.c_str(), &buf) != 0)
        return false;
    return S_ISDIR(buf.st_mode);
#endif
}

bool bmx::check_is_abs_path(string name)
{
#if defined(_WIN32)
    if (((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= 'a' && name[0] <= 'z')) &&
          name[1] == ':')
    {
        return true;
    }
#else
    if (name[0] == '/')
        return true;
#endif
    return false;
}

bool bmx::check_ends_with_dir_separator(string name)
{
#if defined(_WIN32)
    return !name.empty() && (name[name.size() - 1] == '/' || name[name.size() - 1] == '\\');
#else
    return !name.empty() && name[name.size() - 1] == '/';
#endif
}

string bmx::trim_string(string value)
{
    size_t start;
    size_t len;

    // trim spaces from the start
    start = 0;
    while (start < value.size() && isspace(value[start]))
        start++;
    if (start >= value.size())
        return "";

    // trim spaces from the end by reducing the length
    len = value.size() - start;
    while (len > 0 && isspace(value[start + len - 1]))
        len--;

    return value.substr(start, len);
}

vector<string> bmx::split_string(string value, char separator, bool allow_empty)
{
    vector<string> result;
    size_t start = 0;
    size_t end = 0;
    while (end < value.size()) {
        if (value[end] == separator) {
            if (allow_empty || end != start)
                result.push_back(value.substr(start, end - start));
            start = end + 1;
            end = start;
        }
        end++;
    }
    if (end != start)
        result.push_back(value.substr(start, end - start));

    return result;
}

void bmx::get_xml_encoding(const unsigned char *data, size_t size, bmx::TextEncoding *encoding, bmx::ByteOrder *byte_order)
{
    // see also section F in the XML specification, http://www.w3.org/TR/REC-xml/

    *encoding   = UNKNOWN_TEXT_ENCODING;
    *byte_order = UNKNOWN_BYTE_ORDER;

    if (size < 4)
        return;

    if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0xfe && data[3] == 0xff) {
        // UCS-4
        *byte_order = BMX_BIG_ENDIAN;
    } else if (data[0] == 0xff && data[1] == 0xfe && data[2] == 0x00 && data[3] == 0x00) {
        // UCS-4
        *byte_order = BMX_LITTLE_ENDIAN;
    } else if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0xff && data[3] == 0xfe) {
        // UCS-4, unusual octet order
    } else if (data[0] == 0xfe && data[1] == 0xff && data[2] == 0x00 && data[3] == 0x00) {
        // UCS-4, unusual octet order
    } else if (data[0] == 0xfe && data[1] == 0xff && (data[2] != 0x00 || data[3] != 0x00)) {
        *encoding   = UTF16;
        *byte_order = BMX_BIG_ENDIAN;
    } else if (data[0] == 0xff && data[1] == 0xfe && (data[2] != 0x00 || data[3] != 0x00)) {
        *encoding   = UTF16;
        *byte_order = BMX_LITTLE_ENDIAN;
    } else if (data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf) {
        *encoding   = UTF8;
        *byte_order = BMX_BYTE_ORIENTED;
    } else if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x3c) {
        // 32-bit encoding
        *byte_order = BMX_BIG_ENDIAN;
    } else if (data[0] == 0x3c && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x00) {
        // 32-bit encoding
        *byte_order = BMX_LITTLE_ENDIAN;
    } else if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x3c && data[3] == 0x00) {
        // 32-bit encoding, unusual octet order
    } else if (data[0] == 0x00 && data[1] == 0x3c && data[2] == 0x00 && data[3] == 0x00) {
        // 32-bit encoding, unusual octet order
    } else if (data[0] == 0x00 && data[1] == 0x3c && data[2] == 0x00 && data[3] == 0x3f) {
        *byte_order = BMX_BIG_ENDIAN;
        get_xml_prolog_encoding_16bit_be(data, size, encoding);
    } else if (data[0] == 0x3c && data[1] == 0x00 && data[2] == 0x3f && data[3] == 0x00) {
        *byte_order = BMX_LITTLE_ENDIAN;
        get_xml_prolog_encoding_16bit_le(data, size, encoding);
    } else if (data[0] == 0x4c && data[1] == 0x6f && data[2] == 0xa7 && data[3] == 0x94) {
        // EBCDIC
        *byte_order = BMX_BYTE_ORIENTED;
    } else {
        bool prolog_present;
        bool encoding_present;
        get_xml_prolog_encoding((const char*)data, size, encoding, &prolog_present, &encoding_present);
        if (!encoding_present)
            *encoding = UTF8;
        if (*encoding == UTF8 || prolog_present)
            *byte_order = BMX_BYTE_ORIENTED;
        if (*encoding != UTF8)
            *encoding = UNKNOWN_TEXT_ENCODING; // ignore contradictory prolog encoding utf-16
    }
}

bmx::Timestamp bmx::generate_timestamp_now()
{
    Timestamp now;
    struct tm gmt;

    if (BMX_REGRESSION_TEST) {
        now.year  = 1970;
        now.month = 1;
        now.day   = 1;
        now.hour  = 0;
        now.min   = 0;
        now.sec   = 0;
        now.qmsec = 0;
        return now;
    }

#if HAVE_GMTIME_R
    time_t t = time(0);
    BMX_CHECK(gmtime_r(&t, &gmt));
#elif defined(_MSC_VER)
    struct _timeb tb;
    BMX_CHECK(_ftime_s(&tb) == 0);
    BMX_CHECK(gmtime_s(&gmt, &tb.time) == 0);
#else
    // Note: gmtime is not thread-safe
    time_t t = time(0);
    const struct tm *gmt_ptr = gmtime(&t);
    BMX_CHECK(gmt_ptr);
    gmt = *gmt_ptr;
#endif

    now.year  = gmt.tm_year + 1900;
    now.month = gmt.tm_mon + 1;
    now.day   = gmt.tm_mday;
    now.hour  = gmt.tm_hour;
    now.min   = gmt.tm_min;
    now.sec   = gmt.tm_sec;
    now.qmsec = 0;

    return now;
}

bmx::UUID bmx::generate_uuid()
{
    UUID bmx_uuid;

    if (BMX_REGRESSION_TEST) {
        static uint32_t count = 65537; // not 1 to avoid clashing with libMXF generated regtest UUID

        memset(&bmx_uuid, 0, sizeof(bmx_uuid));
        bmx_uuid.octet12 = (uint8_t)((count >> 24) & 0xff);
        bmx_uuid.octet13 = (uint8_t)((count >> 16) & 0xff);
        bmx_uuid.octet14 = (uint8_t)((count >> 8)  & 0xff);
        bmx_uuid.octet15 = (uint8_t)( count        & 0xff);

        count++;
    } else {
#if defined(_WIN32)
        GUID guid;
        CoCreateGuid(&guid);
        bmx_uuid.octet0 = (uint8_t)((guid.Data1 >> 24) & 0xff);
        bmx_uuid.octet1 = (uint8_t)((guid.Data1 >> 16) & 0xff);
        bmx_uuid.octet2 = (uint8_t)((guid.Data1 >>  8) & 0xff);
        bmx_uuid.octet3 = (uint8_t)((guid.Data1      ) & 0xff);
        bmx_uuid.octet4 = (uint8_t)((guid.Data2 >>  8) & 0xff);
        bmx_uuid.octet5 = (uint8_t)((guid.Data2      ) & 0xff);
        bmx_uuid.octet6 = (uint8_t)((guid.Data3 >>  8) & 0xff);
        bmx_uuid.octet7 = (uint8_t)((guid.Data3      ) & 0xff);
        memcpy(&bmx_uuid.octet8, guid.Data4, 8);
#else
        uuid_t uuid;
        uuid_generate(uuid);
        memcpy(&bmx_uuid, &uuid, sizeof(bmx_uuid));
#endif
    }

    return bmx_uuid;
}

bmx::UMID bmx::generate_umid()
{
    // material type not identified, UUID material generation method, no instance method defined
    static const unsigned char umid_prefix[16] = {0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05, 0x01, 0x01, 0x0f, 0x20,
                                                  0x13, 0x00, 0x00, 0x00};

    UMID umid;
    memcpy(&umid.octet0, umid_prefix, sizeof(umid_prefix));

    UUID material_number = generate_uuid();
    memcpy(&umid.octet16, &material_number, sizeof(material_number));

    return umid;
}

bmx::UUID bmx::create_uuid_from_name(const void *ns, size_t ns_size, const string &name)
{
    unsigned char digest[16];
    MD5Context ctx;
    md5_init(&ctx);
    md5_update(&ctx, (const unsigned char*)ns, (uint32_t)ns_size);
    md5_update(&ctx, (const unsigned char*)name.c_str(), (uint32_t)name.size());
    md5_final(digest, &ctx);

    UUID uuid;
    memcpy(&uuid, digest, 16);
    uuid.octet6 = (uuid.octet6 & 0x0f) | 0x30;  // version 3 - MD5 hash and namespace
    uuid.octet8 = (uuid.octet8 & 0x3f) | 0x80;  // variant rfc 4122

    return uuid;
}

bmx::UUID bmx::create_uuid_from_name(const string &name)
{
  return create_uuid_from_name(BMX_NAMESPACE.c_str(), BMX_NAMESPACE.size(), name);
}

uint16_t bmx::get_rounded_tc_base(Rational rate)
{
    return (uint16_t)((rate.numerator + rate.denominator/2) / rate.denominator);
}

string bmx::get_duration_string(int64_t count, Rational rate)
{
    if (count < 0 || rate.numerator == 0 || rate.denominator == 0)
        return "00:00:00:00";

    uint32_t rounded_rate = (uint32_t)((rate.numerator + rate.denominator/2) / rate.denominator);

    int64_t frame = count % rounded_rate;
    int64_t sec = count / rounded_rate;
    int64_t min = sec / 60;
    sec %= 60;
    int64_t hour = min / 60;
    min %= 60;

    char buffer[64];
    bmx_snprintf(buffer, sizeof(buffer), "%02" PRId64 ":%02d:%02d:%02" PRId64, hour, (int)min, (int)sec, frame);

    return buffer;
}

string bmx::get_generic_duration_string(int64_t count, Rational rate)
{
    if (count <= 0 || rate.numerator == 0 || rate.denominator == 0)
        return "00:00:00.00";

    Rational msec_rate = {1000, 1};
    int64_t msec = convert_position(rate, count, msec_rate, ROUND_DOWN);
    int64_t sec = msec / 1000;
    int64_t min = sec / 60;
    sec %= 60;
    int64_t hour = min / 60;
    min %= 60;
    int64_t sec_frac = 100 * (msec % 1000) / 1000;

    char buffer[64];
    bmx_snprintf(buffer, sizeof(buffer), "%02" PRId64 ":%02d:%02d.%02d", hour, (int)min, (int)sec, (int)sec_frac);

    return buffer;
}

string bmx::get_generic_duration_string_2(int64_t count, Rational rate)
{
    if (count < 0 || rate.numerator == 0 || rate.denominator == 0)
        return "00:00:00:00";

    uint32_t rounded_rate = (uint32_t)((rate.numerator + rate.denominator/2) / rate.denominator);

    int64_t frame = count % rounded_rate;
    int64_t sec = count / rounded_rate;
    int64_t min = sec / 60;
    sec %= 60;
    int64_t hour = min / 60;
    min %= 60;

    char buffer[64];
    bmx_snprintf(buffer, sizeof(buffer), "%02" PRId64 ":%02d:%02d:%02" PRId64 " @%ufps", hour, (int)min, (int)sec,
                                         frame, rounded_rate);

    return buffer;
}

bmx::Rational bmx::convert_int_to_rational(int32_t value)
{
    Rational ret = {value, 1};
    return ret;
}

string bmx::get_timecode_string(Timecode timecode)
{
    char buffer[64];
    bmx_snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d%c%02d",
                 timecode.GetHour(),
                 timecode.GetMin(),
                 timecode.GetSec(),
                 timecode.IsDropFrame() ? ';' : ':',
                 timecode.GetFrame());
    return buffer;
}

string bmx::get_umid_string(UMID umid)
{
    static const char hex_chars[] = "0123456789abcdef";
    char buffer[128];
    int offset = 0;

    int i, j;
    for (i = 0; i < 32; i += 4) {
        if (i != 0)
            buffer[offset++] = '.';
        for (j = 0; j < 4; j++) {
            buffer[offset++] = hex_chars[((&umid.octet0)[i + j] >> 4) & 0x0f];
            buffer[offset++] = hex_chars[ (&umid.octet0)[i + j]       & 0x0f];
        }
    }
    buffer[offset] = '\0';

    return buffer;
}

string bmx::get_uuid_string(UUID uuid)
{
    char buffer[64];

    bmx_snprintf(buffer, sizeof(buffer),
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 uuid.octet0,  uuid.octet1,  uuid.octet2,  uuid.octet3,
                 uuid.octet4,  uuid.octet5,  uuid.octet6,  uuid.octet7,
                 uuid.octet8,  uuid.octet9,  uuid.octet10, uuid.octet11,
                 uuid.octet12, uuid.octet13, uuid.octet14, uuid.octet15);

    return buffer;
}

bmx::Rational bmx::reduce_rational(Rational rational)
{
    if (rational.numerator == 0 || rational.denominator == 0)
        return ZERO_RATIONAL;

    Rational result = rational;

    int32_t sign = 1;
    if ((result.numerator < 0) != (result.denominator < 0))
        sign = -1;

    if (result.numerator < 0)
        result.numerator = -result.numerator;
    if (result.denominator < 0)
        result.denominator = -result.denominator;

    int32_t d = gcd(result.numerator, result.denominator);
    result.numerator   /= d;
    result.denominator /= d;

    result.numerator *= sign;

    return result;
}

bmx::Rational bmx::normalize_rate(Rational rate)
{
    Rational result = reduce_rational(rate);

    // can't normalize an invalid negative rate
    if (result.numerator < 0)
        return rate;

    if (result.numerator == 2997 && result.denominator == 1000)
        return FRAME_RATE_2997;
    else if (result.numerator == 2997 && result.denominator == 500)
        return FRAME_RATE_5994;

    return result;
}

uint8_t bmx::get_system_item_cp_rate(Rational frame_rate)
{
    if (frame_rate.denominator != 1 && frame_rate.denominator != 1001)
        return 0;

    int32_t frame_rate_per_sec = frame_rate.numerator;
    if (frame_rate.denominator == 1001)
        frame_rate_per_sec /= 1000;

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(CONTENT_PACKAGE_RATES); i++) {
        if (CONTENT_PACKAGE_RATES[i].frame_rate_per_sec == frame_rate_per_sec)
            return (CONTENT_PACKAGE_RATES[i].cp_rate_id << 1) | (frame_rate.denominator == 1 ? 0 : 1);
    }

    return 0;
}

uint32_t bmx::get_kag_fill_size(int64_t klv_size, uint32_t kag_size, uint8_t min_llen)
{
    // assuming the partition pack is aligned to the kag working from the first byte of the file

    uint32_t fill_size = 0;
    uint32_t klv_in_kag_size = (uint32_t)(klv_size % kag_size);
    if (klv_in_kag_size > 0) {
        fill_size = kag_size - klv_in_kag_size;
        while (fill_size < (uint32_t)min_llen + mxfKey_extlen)
            fill_size += kag_size;
    }

    return fill_size;
}

int64_t bmx::get_kag_aligned_size(int64_t klv_size, uint32_t kag_size, uint8_t min_llen)
{
    return klv_size + get_kag_fill_size(klv_size, kag_size, min_llen);
}

void bmx::decode_smpte_timecode(Rational frame_rate, const unsigned char *smpte_tc, unsigned int size,
                                Timecode *timecode, bool *field_mark)
{
    // see SMPTE 12M-1-2008 and SMPTE 331M-2004 section 8.2 for details

    BMX_ASSERT(size >= 4);

    int hour, min, sec, frame;
    frame = ((smpte_tc[0] & 0x30) >> 4) * 10 + (smpte_tc[0] & 0x0f);
    sec   = ((smpte_tc[1] & 0x70) >> 4) * 10 + (smpte_tc[1] & 0x0f);
    min   = ((smpte_tc[2] & 0x70) >> 4) * 10 + (smpte_tc[2] & 0x0f);
    hour  = ((smpte_tc[3] & 0x30) >> 4) * 10 + (smpte_tc[3] & 0x0f);

    bool drop_frame = (smpte_tc[0] & 0x40);

    uint16_t tc_base = get_rounded_tc_base(frame_rate);
    if (tc_base > 30) {
        frame *= 2;

        // vitc field mark flag indicates first or second of frame pair
        // the preferred method is to have the flag set to 1 for the second frame
        if ((tc_base == 50 && (smpte_tc[3] & 0x80)) ||
            (tc_base == 60 && (smpte_tc[1] & 0x80)))
        {
            *field_mark = true;
        }
        else
        {
            *field_mark = false;
        }
    } else {
        *field_mark = false;
    }

    timecode->Init(frame_rate, drop_frame, hour, min, sec, frame);
}

bmx::Timecode bmx::decode_smpte_timecode(Rational frame_rate, const unsigned char *smpte_tc, unsigned int size)
{
    Timecode timecode;
    bool field_mark;
    decode_smpte_timecode(frame_rate, smpte_tc, size, &timecode, &field_mark);
    return timecode;
}

void bmx::encode_smpte_timecode(Timecode timecode, bool field_mark, unsigned char *smpte_tc, unsigned int size)
{
    // see SMPTE 12M-1-2008 and SMPTE 331M-2004 section 8.2 for details

    BMX_ASSERT(size >= 4);

    int frame = timecode.GetFrame();
    if (timecode.GetRoundedTCBase() > 30)
        frame /= 2;

    smpte_tc[0] = ((frame              % 10) & 0x0f) | (((frame              / 10) << 4) & 0x30);
    smpte_tc[1] = ((timecode.GetSec()  % 10) & 0x0f) | (((timecode.GetSec()  / 10) << 4) & 0x70);
    smpte_tc[2] = ((timecode.GetMin()  % 10) & 0x0f) | (((timecode.GetMin()  / 10) << 4) & 0x70);
    smpte_tc[3] = ((timecode.GetHour() % 10) & 0x0f) | (((timecode.GetHour() / 10) << 4) & 0x30);

    if (timecode.IsDropFrame())
        smpte_tc[0] |= 0x40;

    if (field_mark && timecode.GetRoundedTCBase() > 30 && timecode.GetFrame() % 2 == 1) {
        // indicate second frame of pair use the vitc field mark flag
        switch (timecode.GetRoundedTCBase())
        {
            case 50:
                smpte_tc[3] |= 0x80;
                break;
            case 60:
                smpte_tc[1] |= 0x80;
                break;
            default:
                break;
        }
    }

    if (size > 4)
        memset(&smpte_tc[4], 0, size - 4);
}

bool bmx::check_excess_d10_padding(const unsigned char *data, uint32_t data_size, uint32_t target_size)
{
    // a minimum of 3 bytes would be sufficient for an MPEG-2 frame where marker bits
    // prevent start code byte sequences (0x000001) appearing in the bitstream and extra
    // padding bytes are only present at the end of the frame.
    // 4 bytes are checked below. The data at the end is also checked in case the frame was
    // parsed incorrectly.

    uint32_t c;
    uint32_t i;
    for (i = target_size, c = 0; i < data_size && c < 4; i++, c++) {
        if (data[i])
            return false;
    }
    for (i = data_size, c = 0; i > target_size && i > 0 && c < 4; i--, c++) {
        if (data[i - 1])
            return false;
    }

    return true;
}

void bmx::bmx_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    bmx_vsnprintf(str, size, format, ap);
    va_end(ap);
}

void bmx::bmx_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
#if defined(_MSC_VER)
    int res = _vsnprintf(str, size, format, ap);
    if (str && size > 0) {
        if (res == -1 && errno == EINVAL)
            str[0] = 0;
        else
            str[size - 1] = 0;
    }
#else
    if (vsnprintf(str, size, format, ap) < 0 && str && size > 0)
        str[0] = 0;
#endif
}

string bmx::bmx_strerror(int errnum)
{
    char buf[128];

#ifdef HAVE_STRERROR_R

#ifdef _GNU_SOURCE
    const char *err_str = strerror_r(errnum, buf, sizeof(buf));
    if (err_str != buf)
        bmx_snprintf(buf, sizeof(buf), "%s", err_str);
#else
    if (strerror_r(errnum, buf, sizeof(buf)) != 0)
        bmx_snprintf(buf, sizeof(buf), "unknown error code %d", errnum);
#endif

#elif defined(_MSC_VER)
    if (strerror_s(buf, sizeof(buf), errnum) != 0)
        bmx_snprintf(buf, sizeof(buf), "unknown error code %d", errnum);
#else
    bmx_snprintf(buf, sizeof(buf), "error code %d", errnum);
#endif

    return buf;
}

