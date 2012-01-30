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

#include <cstring>
#include <cstdio>
#include <cerrno>

#if defined(_WIN32)
#include <sys/timeb.h>
#include <time.h>
#include <direct.h> // _getcwd
#include <windows.h>
#else
#include <uuid/uuid.h>
#include <sys/time.h>
#endif

#include <algorithm>

#include <bmx/Utils.h>
#include <bmx/URI.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;


#define MAX_INT32   2147483647


typedef struct
{
    mxfRational lower_edit_rate;
    mxfRational higher_edit_rate;
    uint32_t sample_sequence[11];
} SampleSequence;

static const SampleSequence SAMPLE_SEQUENCES[] =
{
    {{30000, 1001}, {48000,1}, {1602, 1601, 1602, 1601, 1602, 0,   0,   0,   0,   0,   0}},
    {{60000, 1001}, {48000,1}, {801,  801,  801,  800,  801,  801, 801, 800, 801, 801, 0}},
};



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
                BMX_ASSERT(num_higher_samples <= 0xffffffff);
                sample_sequence->push_back((uint32_t)num_higher_samples);
            } else {
                // try known sample sequences
                size_t i;
                for (i = 0; i < ARRAY_SIZE(SAMPLE_SEQUENCES); i++) {
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

void bmx::offset_sample_sequence(vector<uint32_t> &sample_sequence, uint8_t offset)
{
    rotate(sample_sequence.begin(),
           sample_sequence.begin() + (offset % sample_sequence.size()),
           sample_sequence.end());
}

int64_t bmx::convert_position_lower(int64_t position, const std::vector<uint32_t> &sequence, int64_t sequence_size)
{
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

int64_t bmx::convert_position_higher(int64_t position, const std::vector<uint32_t> &sequence, int64_t sequence_size)
{
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

int64_t bmx::convert_duration_lower(int64_t duration, const std::vector<uint32_t> &sequence, int64_t sequence_size)
{
    if (duration <= 0)
        return 0;

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
                                    int64_t sequence_size)
{
    if (duration <= 0)
        return 0;

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

int64_t bmx::convert_duration_higher(int64_t duration, const std::vector<uint32_t> &sequence, int64_t sequence_size)
{
    if (duration <= 0)
        return 0;

    int64_t higher_duration = duration / sequence.size() * sequence_size;

    size_t sequence_remainder = (size_t)(duration % sequence.size());
    uint32_t i;
    for (i = 0; i < sequence_remainder; i++)
        higher_duration += sequence[i];

    return higher_duration;
}

int64_t bmx::convert_duration_higher(int64_t duration, int64_t position, const std::vector<uint32_t> &sequence,
                                     int64_t sequence_size)
{
    if (duration <= 0)
        return 0;

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
    if ((sep_index = filename.rfind("/")) != string::npos)
        return filename.substr(sep_index + 1);
    else
        return filename;
}

string bmx::strip_name(string filename)
{
    size_t sep_index;
    if ((sep_index = filename.rfind("/")) != string::npos)
        return filename.substr(0, sep_index);
    else
        return "";
}

string bmx::strip_suffix(string filename)
{
    size_t suffix_index;
    if ((suffix_index = filename.rfind(".")) != string::npos)
        return filename.substr(0, suffix_index);
    else
        return filename;
}

string bmx::get_abs_cwd()
{
#define MAX_REASONABLE_PATH_SIZE    (10 * 1024)

    string abs_cwd;
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
            throw BMXException("Failed to get current working directory: %s", strerror(errno));
        } else if (path_size >= MAX_REASONABLE_PATH_SIZE) {
            throw BMXException("Maximum path size (%d) for current working directory exceeded",
                               MAX_REASONABLE_PATH_SIZE, strerror(errno));
        }
        path_size += 1024;
    }
    abs_cwd = temp_path;
    delete [] temp_path;

    return abs_cwd;
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

bmx::Timestamp bmx::generate_timestamp_now()
{
    Timestamp now;

    struct tm gmt;
    time_t t = time(0);

#if defined(_WIN32)
    // TODO: need thread-safe (reentrant) version
    const struct tm *gmt_ptr = gmtime(&t);
    BMX_CHECK(gmt_ptr);
    gmt = *gmt_ptr;
#else
    BMX_CHECK(gmtime_r(&t, &gmt));
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

#if defined(_WIN32)
    GUID guid;
    CoCreateGuid(&guid);
    memcpy(&bmx_uuid, &guid, sizeof(bmx_uuid));
#else
    uuid_t uuid;
    uuid_generate(uuid);
    memcpy(&bmx_uuid, &uuid, sizeof(bmx_uuid));
#endif

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

uint16_t bmx::get_rounded_tc_base(Rational rate)
{
    return (uint16_t)((rate.numerator + rate.denominator/2) / rate.denominator);
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
    sprintf(buffer, "%02"PRId64":%02d:%02d.%02d", hour, (int)min, (int)sec, (int)sec_frac);

    return buffer;
}

string bmx::get_generic_duration_string_2(int64_t count, Rational rate)
{
    if (count < 0 || rate.numerator == 0 || rate.denominator == 0)
        return "00:00:00:00";

    uint16_t rounded_rate = get_rounded_tc_base(rate);

    int64_t frame = count % rounded_rate;
    int64_t sec = count / rounded_rate;
    int64_t min = sec / 60;
    sec %= 60;
    int64_t hour = min / 60;
    min %= 60;

    char buffer[64];
    sprintf(buffer, "%02"PRId64":%02d:%02d:%02d @%ufps", hour, (int)min, (int)sec, (int)frame, rounded_rate);

    return buffer;
}

bmx::Rational bmx::convert_int_to_rational(int32_t value)
{
    Rational ret = {value, 1};
    return ret;
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

