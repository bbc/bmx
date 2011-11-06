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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <string>
#include <vector>

#include <libMXF++/MXFException.h>

#include <im/as11/AS11Clip.h>
#include "FrameworkHelper.h"
#include <im/essence_parser/DVEssenceParser.h>
#include <im/essence_parser/MPEG2EssenceParser.h>
#include <im/essence_parser/AVCIRawEssenceReader.h>
#include <im/essence_parser/RawEssenceReader.h>
#include <im/MXFUtils.h>
#include "../AppUtils.h"
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef enum
{
    AS11_CORE_FRAMEWORK_TYPE,
    DPP_FRAMEWORK_TYPE,
} FrameworkType;

typedef struct
{
    FrameworkType type;
    string name;
    string value;
} FrameworkProperty;

typedef enum
{
    NO_ESSENCE_GROUP = 0,
    DV_ESSENCE_GROUP,
    MPEG2LG_ESSENCE_GROUP,
    D10_ESSENCE_GROUP,
} EssenceTypeGroup;

typedef struct
{
    AS11ClipType clip_type;
    EssenceTypeGroup essence_type_group;
    AS11EssenceType essence_type;

    AS11Track *track;

    const char *filename;
    int64_t file_start_offset;
    int64_t file_max_length;

    RawEssenceReader *raw_reader;
    uint32_t sample_sequence[32];
    size_t sample_sequence_size;
    size_t sample_sequence_offset;

    // general
    uint32_t track_number;
    bool track_number_set;

    // picture
    mxfRational aspect_ratio;
    bool aspect_ratio_set;
    uint8_t afd;
    uint32_t component_depth;
    bool image_type_set;
    uint8_t mpeg2lg_signal_standard;
    uint8_t mpeg2lg_frame_layout;

    // sound
    mxfRational sampling_rate;
    uint32_t audio_quant_bits;
    bool locked;
    bool locked_set;
    int8_t audio_ref_level;
    bool audio_ref_level_set;
    int8_t dial_norm;
    bool dial_norm_set;
} RawInput;


extern bool IM_REGRESSION_TEST;



static bool read_samples(RawInput *input)
{
    uint32_t num_samples = input->sample_sequence[input->sample_sequence_offset];
    input->sample_sequence_offset = (input->sample_sequence_offset + 1) % input->sample_sequence_size;

    return input->raw_reader->ReadSamples(num_samples) == num_samples;
}

static bool open_raw_reader(RawInput *input)
{
    if (input->raw_reader) {
        input->raw_reader->Reset();
        return true;
    }

    FILE *raw_file = fopen(input->filename, "rb");
    if (!raw_file) {
        log_error("Failed to open input file '%s': %s\n", input->filename, strerror(errno));
        return false;
    }

    if (input->essence_type == AS11_AVCI100_1080I ||
        input->essence_type == AS11_AVCI100_1080P ||
        input->essence_type == AS11_AVCI100_720P ||
        input->essence_type == AS11_AVCI50_1080I ||
        input->essence_type == AS11_AVCI50_1080P ||
        input->essence_type == AS11_AVCI50_720P)
    {
        input->raw_reader = new AVCIRawEssenceReader(raw_file);
    }
    else
    {
        input->raw_reader = new RawEssenceReader(raw_file);
    }
    if (input->file_start_offset)
        input->raw_reader->SeekToOffset(input->file_start_offset);
    if (input->file_max_length > 0)
        input->raw_reader->SetMaxReadLength(input->file_max_length);

    return true;
}

static void init_input(RawInput *input)
{
    memset(input, 0, sizeof(*input));
    input->aspect_ratio = ASPECT_RATIO_16_9;
    input->aspect_ratio_set = false;
    input->sampling_rate = SAMPLING_RATE_48K;
    input->audio_quant_bits = 16;
    input->component_depth = 8;
    input->image_type_set = false;
    input->mpeg2lg_signal_standard = MXF_SIGNAL_STANDARD_SMPTE274M;
    input->mpeg2lg_frame_layout = MXF_SEPARATE_FIELDS;
}

static void clear_input(RawInput *input)
{
    delete input->raw_reader;
}

static string trim_string(string value)
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

static bool parse_clip_type(const char *clip_type_str, AS11ClipType *clip_type)
{
    if (strcmp(clip_type_str, "op1a") == 0)
        *clip_type = AS11_OP1A_CLIP_TYPE;
    else if (strcmp(clip_type_str, "d10") == 0)
        *clip_type = AS11_D10_CLIP_TYPE;
    else
        return false;

    return true;
}

static bool parse_framework_type(const char *fwork_str, FrameworkType *type)
{
    if (strcmp(fwork_str, "as11") == 0)
        *type = AS11_CORE_FRAMEWORK_TYPE;
    else if (strcmp(fwork_str, "dpp") == 0)
        *type = DPP_FRAMEWORK_TYPE;
    else
        return false;

    return true;
}

static bool parse_framework_file(const char *filename, FrameworkType type, vector<FrameworkProperty> *properties)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, strerror(errno));
        return false;
    }

    int line_num = 0;
    int c = '1';
    while (c != EOF) {
        // move file pointer past the newline characters
        while ((c = fgetc(file)) != EOF && (c == '\r' || c == '\n')) {
            if (c == '\n')
                line_num++;
        }

        FrameworkProperty property;
        property.type = type;
        bool parse_name = true;
        while (c != EOF && (c != '\r' && c != '\n')) {
            if (c == ':' && parse_name) {
                parse_name = false;
            } else {
                if (parse_name)
                    property.name += c;
                else
                    property.value += c;
            }

            c = fgetc(file);
        }
        if (!property.name.empty()) {
            if (parse_name) {
                fprintf(stderr, "Failed to parse line %d\n", line_num);
                fclose(file);
                return false;
            }

            property.name = trim_string(property.name);
            property.value = trim_string(property.value);
            properties->push_back(property);
        }

        if (c == '\n')
            line_num++;
    }

    fclose(file);

    return true;
}

static bool parse_segmentation_file(const char *filename, Rational frame_rate, vector<AS11TCSegment> *segments,
                                    bool *filler_complete_segments)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, strerror(errno));
        return false;
    }

    bool last_tc_xx = false;
    int line_num = 0;
    int c = '1';
    while (c != EOF) {
        // move file pointer past the newline characters
        while ((c = fgetc(file)) != EOF && (c == '\r' || c == '\n')) {
            if (c == '\n')
                line_num++;
        }

        AS11TCSegment segment;
        segment.start.Init(frame_rate, false);

        string part_number_string;
        string tc_pair_string;
        int space_count = 0;
        size_t second_tc_start = 0;
        while (c != EOF && (c != '\r' && c != '\n')) {
            if (space_count < 2) {
                tc_pair_string += c;
                if (c == ' ') {
                    if (space_count == 0)
                        second_tc_start = tc_pair_string.size();
                    space_count++;
                }
                if (space_count == 2) {
                    last_tc_xx = false;

                    Timecode end;
                    if (!parse_timecode(tc_pair_string.c_str(), frame_rate, &segment.start)) {
                        fprintf(stderr, "Failed to parse 1st timecode on line %d\n", line_num);
                        fclose(file);
                        return false;
                    }
                    if (!parse_timecode(&tc_pair_string[second_tc_start], frame_rate, &end)) {
                        if (trim_string(&tc_pair_string[second_tc_start]) != "xx:xx:xx:xx") {
                            fprintf(stderr, "Failed to parse 2nd timecode on line %d\n", line_num);
                            fclose(file);
                            return false;
                        }

                        // segment extends to package duration
                        last_tc_xx = true;
                        segment.duration = 0;
                    } else {
                        segment.duration = end.GetOffset() - segment.start.GetOffset() + 1;
                        if (segment.duration < 0) // assume crossed midnight
                            segment.duration += end.GetMaxOffset();
                    }
                }
            } else {
                part_number_string += c;
            }

            c = fgetc(file);
        }
        if (!tc_pair_string.empty()) {
            if (space_count != 2) {
                fprintf(stderr, "Failed to parse line %d\n", line_num);
                fclose(file);
                return false;
            }

            int pnum, ptotal;
            if (sscanf(part_number_string.c_str(), "%d/%d", &pnum, &ptotal) != 2 ||
                pnum < 0 || ptotal <= 0)
            {
                fprintf(stderr, "Failed to parse valid part number on line %d\n", line_num);
                fclose(file);
                return false;
            }
            segment.part_number = pnum;
            segment.part_total = ptotal;

            segments->push_back(segment);
        }

        if (c == '\n')
            line_num++;
    }

    *filler_complete_segments = !last_tc_xx;


    fclose(file);

    return true;
}

static string get_version_info()
{
    char buffer[256];
    sprintf(buffer, "raw2as11, %s v%s (%s %s)", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
    return buffer;
}

static void usage(const char* cmd)
{
    fprintf(stderr, "%s\n", get_version_info().c_str());
    fprintf(stderr, "Usage: %s <<options>> [<<input options>> <input>]+ <output>\n", cmd);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -l <file>               Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, "  -t <type>               Clip type: op1a, d10. Default is op1a\n");
    fprintf(stderr, "  -f <rate>               Frame rate: 25, 30 (30000/1001), 50 or 60 (60000/1001). Default parsed or 25\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --part <interval>       Essence partition interval in frames, or seconds with 's' suffix. Default single partition\n");
    fprintf(stderr, "  --out-start <offset>    Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    fprintf(stderr, "  --out-end <offset>      Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    fprintf(stderr, "  --seq-off <value>       Sound sample sequence offset. Default 0 for d10 and not set (0) for op1a\n");
    fprintf(stderr, "  --dm <fwork> <name> <value>      Set descriptive framework property. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "  --dm-file <fwork> <name>         Parse and set descriptive framework properties from text file <name>. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "  --seg <name>                     Parse and set segmentation data from text file <name>\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames. Default is minimum available duration\n");
    fprintf(stderr, "Input Options (must precede the input to which it applies):\n");
    fprintf(stderr, "  -a <n:d>                Image aspect ratio. Either 4:3 or 16:9. Default parsed or 16:9\n");
    fprintf(stderr, "  --afd <value>           Active Format Descriptor code. Default not set\n");
    fprintf(stderr, "  -c <depth>              Component depth for uncompressed/DV100 video. Either 8 or 10. Default parsed or 8\n");
    fprintf(stderr, "  -q <bps>                Audio quantization bits per sample. Either 16 or 24. Default 16\n");
    fprintf(stderr, "  --locked <bool>         Indicate whether the number of audio samples is locked to the video. Either true or false. Default not set\n");
    fprintf(stderr, "  --audio-ref <level>     Audio reference level, number of dBm for 0VU. Default not set\n");
    fprintf(stderr, "  --dial-norm <value>     Gain to be applied to normalize perceived loudness of the clip. Default not set\n");
    fprintf(stderr, "  --off <bytes>           Skip bytes at start of the next input/track's file\n");
    fprintf(stderr, "  --maxlen <bytes>        Maximum number of bytes to read from next input/track's file\n");
    fprintf(stderr, "  --imgtype <type>        MPEG-2 Long GOP video type: '1080i', '1080p', '720p'. Default '1080i'\n");
    fprintf(stderr, "  --track-num <num>       Set the track number. Default starts at 1 or equals last track of same picture/sound type + 1\n");
    fprintf(stderr, "                          The channel index equals track number - 1 for the D10 clip type\n");
    fprintf(stderr, "Inputs:\n");
    fprintf(stderr, "  --dv <name>             Raw DV video input file\n");
    fprintf(stderr, "  --iecdv25 <name>        Raw IEC DV25 video input file\n");
    fprintf(stderr, "  --dvbased25 <name>      Raw DV-Based DV25 video input file\n");
    fprintf(stderr, "  --dv50 <name>           Raw DV50 video input file\n");
    fprintf(stderr, "  --dv100_1080i <name>    Raw DV100 1080i video input file\n");
    fprintf(stderr, "  --dv100_1080p <name>    Raw DV100 1080p video input file\n");
    fprintf(stderr, "  --dv100_720p <name>     Raw DV100 720p video input file\n");
    fprintf(stderr, "  --d10 <name>            Raw D10 video input file\n");
    fprintf(stderr, "  --d10_30 <name>         Raw D10 30Mbps video input file\n");
    fprintf(stderr, "  --d10_40 <name>         Raw D10 40Mbps video input file\n");
    fprintf(stderr, "  --d10_50 <name>         Raw D10 50Mbps video input file\n");
    fprintf(stderr, "  --avci100_1080i <name>  Raw AVC-Intra 100 1080i video input file\n");
    fprintf(stderr, "  --avci100_1080p <name>  Raw AVC-Intra 100 1080p video input file\n");
    fprintf(stderr, "  --avci100_720p <name>   Raw AVC-Intra 100 720p video input file\n");
    fprintf(stderr, "  --avci50_1080i <name>   Raw AVC-Intra 50 1080i video input file\n");
    fprintf(stderr, "  --avci50_1080p <name>   Raw AVC-Intra 50 1080p video input file\n");
    fprintf(stderr, "  --avci50_720p <name>    Raw AVC-Intra 50 720p video input file\n");
    fprintf(stderr, "  --unc <name>            Raw uncompressed SD UYVY 422 video input file\n");
    fprintf(stderr, "  --unc_1080i <name>      Raw uncompressed HD 1080i UYVY 422 video input file\n");
    fprintf(stderr, "  --unc_1080p <name>      Raw uncompressed HD 1080p UYVY 422 video input file\n");
    fprintf(stderr, "  --unc_720p <name>       Raw uncompressed HD 720p UYVY 422 video input file\n");
    fprintf(stderr, "  --mpeg2lg <name>          Raw MPEG-2 Long GOP video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl <name>  Raw MPEG-2 Long GOP 422P@HL video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl <name>    Raw MPEG-2 Long GOP MP@HL video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_h14 <name>   Raw MPEG-2 Long GOP MP@H14 video input file\n");
    fprintf(stderr, "  --pcm <name>            Raw PCM audio input file\n");
}

int main(int argc, const char** argv)
{
    const char *log_filename = 0;
    AS11ClipType clip_type = AS11_OP1A_CLIP_TYPE;
    const char *output_filename = 0;
    vector<RawInput> inputs;
    RawInput input;
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    Rational frame_rate = FRAME_RATE_25;
    bool frame_rate_set = false;
    const char *clip_name = 0;
    const char *partition_interval_str = 0;
    int64_t partition_interval = 0;
    int64_t output_start_offset = 0;
    int64_t output_end_offset = 0;
    bool sequence_offset_set = false;
    uint8_t sequence_offset = 0;
    vector<FrameworkProperty> framework_properties;
    const char *segmentation_filename = 0;
    vector<AS11TCSegment> segments;
    bool filler_complete_segments = false;
    int64_t duration = -1;
    bool do_print_version = false;
    int value, num, den;
    unsigned int uvalue;
    int cmdln_index;


    if (argc == 1) {
        usage(argv[0]);
        return 0;
    }

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++)
    {
        if (strcmp(argv[cmdln_index], "--help") == 0 ||
            strcmp(argv[cmdln_index], "-h") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[cmdln_index], "--version") == 0 ||
                 strcmp(argv[cmdln_index], "-v") == 0)
        {
            if (argc == 2) {
                printf("%s\n", get_version_info().c_str());
                return 0;
            }
            do_print_version = true;
        }
        else if (strcmp(argv[cmdln_index], "-l") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            log_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-t") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_clip_type(argv[cmdln_index + 1], &clip_type))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-f") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &value) != 1 ||
                (value != 25 && value != 30 && value != 50 && value != 60))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            if (value == 25 || value == 50) {
                frame_rate.numerator = value;
                frame_rate.denominator = 1;
            } else if (value == 30) {
                frame_rate.numerator = 30000;
                frame_rate.denominator = 1001;
            } else {
                frame_rate.numerator = 60000;
                frame_rate.denominator = 1001;
            }
            frame_rate_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-y") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            start_timecode_str = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--clip") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            clip_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--part") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            partition_interval_str = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--out-start") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &output_start_offset) != 1 ||
                output_start_offset < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--out-end") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &output_end_offset) != 1 ||
                output_end_offset < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--seq-off") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &value) != 1 ||
                value < 0 || value > 255)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            sequence_offset = (uint8_t)value;
            sequence_offset_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dm") == 0)
        {
            if (cmdln_index + 3 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            FrameworkProperty framework_property;
            if (!parse_framework_type(argv[cmdln_index + 1], &framework_property.type))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            framework_property.name = argv[cmdln_index + 2];
            framework_property.value = argv[cmdln_index + 3];
            framework_properties.push_back(framework_property);
            cmdln_index += 3;
        }
        else if (strcmp(argv[cmdln_index], "--dm-file") == 0)
        {
            if (cmdln_index + 2 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            FrameworkType framework_type;
            if (!parse_framework_type(argv[cmdln_index + 1], &framework_type))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            if (!parse_framework_file(argv[cmdln_index + 2], framework_type, &framework_properties)) {
                fprintf(stderr, "Failed to parse framework file '%s'\n", argv[cmdln_index + 2]);
                return 1;
            }
            cmdln_index += 2;
        }
        else if (strcmp(argv[cmdln_index], "--seg") == 0)
        {
            if (cmdln_index + 2 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            segmentation_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dur") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &duration) != 1 || duration < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--regtest") == 0)
        {
            IM_REGRESSION_TEST = true;
        }
        else
        {
            break;
        }
    }

    init_input(&input);
    for (; cmdln_index < argc; cmdln_index++)
    {
        if (strcmp(argv[cmdln_index], "-a") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d:%d", &num, &den) != 2 ||
                ((num != 4 || den != 3) && (num != 16 || den != 9)))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            if (num == 4)
                input.aspect_ratio = ASPECT_RATIO_4_3;
            else
                input.aspect_ratio = ASPECT_RATIO_16_9;
            input.aspect_ratio_set = true;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--afd") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &value) != 1 ||
                value <= 0 || value > 255)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.afd = value;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "-q") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &value) != 1 ||
                (value != 16 && value != 24))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.audio_quant_bits = value;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "-c") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &value) != 1 ||
                (value != 8 && value != 10))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.component_depth = value;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--locked") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_bool(argv[cmdln_index + 1], &input.locked))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.locked_set = true;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--audio-ref") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &value) != 1 ||
                value < -128 || value > 127)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.audio_ref_level = value;
            input.audio_ref_level_set = true;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--dial-norm") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%d", &value) != 1 ||
                value < -128 || value > 127)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.dial_norm = value;
            input.dial_norm_set = true;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--off") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &input.file_start_offset) != 1 ||
                input.file_start_offset < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--maxlen") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &input.file_max_length) != 1 ||
                input.file_max_length <= 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--imgtype") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_image_type(argv[cmdln_index + 1], &input.mpeg2lg_signal_standard, &input.mpeg2lg_frame_layout))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.image_type_set = true;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--track-num") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &uvalue) != 1)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.track_number = uvalue;
            input.track_number_set = true;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--dv") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type_group = DV_ESSENCE_GROUP;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--iecdv25") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_IEC_DV25;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dvbased25") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_DVBASED_DV25;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dv50") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_DV50;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dv100_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_DV100_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dv100_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_DV100_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dv100_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_DV100_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--d10") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type_group = D10_ESSENCE_GROUP;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--d10_30") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_D10_30;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--d10_40") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_D10_40;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--d10_50") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_D10_50;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci100_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_AVCI100_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci100_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_AVCI100_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci100_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_AVCI100_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci50_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_AVCI50_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci50_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_AVCI50_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci50_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_AVCI50_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--unc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_UNC_SD;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--unc_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_UNC_HD_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--unc_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_UNC_HD_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--unc_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_UNC_HD_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type_group = MPEG2LG_ESSENCE_GROUP;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_422p_hl") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_MPEG2LG_422P_HL;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_MPEG2LG_MP_HL;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_h14") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_MPEG2LG_MP_H14;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--pcm") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AS11_PCM;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else
        {
            break;
        }

        init_input(&input);
    }

    if (cmdln_index < argc - 1) {
        usage(argv[0]);
        fprintf(stderr, "Unknown input or option '%s'\n", argv[cmdln_index]);
        return 1;
    } else if (inputs.empty()) {
        usage(argv[0]);
        fprintf(stderr, "No raw inputs given\n");
        return 1;
    } else if (cmdln_index > argc - 1) {
        usage(argv[0]);
        fprintf(stderr, "Missing output filename\n");
        return 1;
    }

    output_filename = argv[cmdln_index];


    if (input.file_start_offset) {
        usage(argv[0]);
        fprintf(stderr, "Found --off after any inputs; it must precede the input to which it applies\n");
        return 1;
    }
    if (input.file_max_length) {
        usage(argv[0]);
        fprintf(stderr, "Found --maxlen after any inputs; it must precede the input to which it applies\n");
        return 1;
    }

    if (log_filename) {
        if (!open_log_file(log_filename))
            return 1;
    }

    connect_libmxf_logging();

    if (IM_REGRESSION_TEST)
        mxf_set_regtest_funcs();

    if (do_print_version)
        log_info("%s\n", get_version_info().c_str());


    int cmd_result = 0;
    try
    {
        // extract essence info
        size_t i;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

            input->clip_type = clip_type;

            // TODO: require parse friendly essence data in regression test for all formats
            if (IM_REGRESSION_TEST) {
                if (input->essence_type_group != NO_ESSENCE_GROUP) {
                    log_error("Regression test requires specific input format type, eg. --iecdv25 rather than --dv\n");
                    throw false;
                }
                continue;
            }

            if (!open_raw_reader(input))
                throw false;

            if (input->essence_type_group == DV_ESSENCE_GROUP ||
                input->essence_type == AS11_IEC_DV25 ||
                input->essence_type == AS11_DVBASED_DV25 ||
                input->essence_type == AS11_DV50 ||
                input->essence_type == AS11_DV100_1080I ||
                input->essence_type == AS11_DV100_1080P ||
                input->essence_type == AS11_DV100_720P)
            {
                DVEssenceParser *dv_parser = new DVEssenceParser();
                input->raw_reader->SetEssenceParser(dv_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to IEC DV-25 if no essence samples
                    if (input->essence_type_group == DV_ESSENCE_GROUP)
                        input->essence_type = AS11_IEC_DV25;
                } else {
                    dv_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == DV_ESSENCE_GROUP) {
                        switch (dv_parser->GetEssenceType())
                        {
                            case DVEssenceParser::IEC_DV25:
                                input->essence_type = AS11_IEC_DV25;
                                break;
                            case DVEssenceParser::DVBASED_DV25:
                                input->essence_type = AS11_DVBASED_DV25;
                                break;
                            case DVEssenceParser::DV50:
                                input->essence_type = AS11_DV50;
                                break;
                            case DVEssenceParser::DV100_1080I:
                                input->essence_type = AS11_DV100_1080I;
                                break;
                            case DVEssenceParser::DV100_720P:
                                input->essence_type = AS11_DV100_720P;
                                break;
                            case DVEssenceParser::UNKNOWN_DV:
                                log_error("Unknown DV essence type\n");
                                throw false;
                        }
                    }

                    input->raw_reader->SetFixedSampleSize(dv_parser->GetFrameSize());

                    if (!frame_rate_set) {
                        if (dv_parser->Is50Hz()) {
                            if (input->essence_type == AS11_DV100_720P)
                                frame_rate = FRAME_RATE_50;
                            else
                                frame_rate = FRAME_RATE_25;
                        } else {
                            if (input->essence_type == AS11_DV100_720P)
                                frame_rate = FRAME_RATE_5994;
                            else
                                frame_rate = FRAME_RATE_2997;
                        }
                    }

                    if (!input->aspect_ratio_set)
                        input->aspect_ratio = dv_parser->GetAspectRatio();
                }
            }
            else if (input->essence_type_group == D10_ESSENCE_GROUP ||
                     input->essence_type == AS11_D10_30 ||
                     input->essence_type == AS11_D10_40 ||
                     input->essence_type == AS11_D10_50)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to D10 50Mbps if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = AS11_D10_50;
                } else {
                    input->raw_reader->SetFixedSampleSize(input->raw_reader->GetSampleDataSize());

                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == D10_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetBitRate())
                        {
                            case 75000:
                                input->essence_type = AS11_D10_30;
                                break;
                            case 100000:
                                input->essence_type = AS11_D10_40;
                                break;
                            case 125000:
                                input->essence_type = AS11_D10_50;
                                break;
                            default:
                                log_error("Unknown D10 bit rate %u\n", mpeg2_parser->GetBitRate());
                                throw false;
                        }
                    }

                    if (!frame_rate_set)
                        frame_rate = mpeg2_parser->GetFrameRate();

                    if (!input->aspect_ratio_set)
                        input->aspect_ratio = mpeg2_parser->GetAspectRatio();
                }
            }
            else if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP ||
                     input->essence_type == AS11_MPEG2LG_422P_HL ||
                     input->essence_type == AS11_MPEG2LG_MP_HL ||
                     input->essence_type == AS11_MPEG2LG_MP_H14)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to 422P@HL if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = AS11_MPEG2LG_422P_HL;
                } else {
                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetProfileAndLevel())
                        {
                            case 0x82:
                                input->essence_type = AS11_MPEG2LG_422P_HL;
                                break;
                            case 0x44:
                                input->essence_type = AS11_MPEG2LG_MP_HL;
                                break;
                            case 0x46:
                                input->essence_type = AS11_MPEG2LG_MP_H14;
                                break;
                            default:
                                log_error("Unexpected MPEG-2 Long GOP profile and level %u\n",
                                          mpeg2_parser->GetProfileAndLevel());
                                throw false;
                        }
                    }

                    if (!input->image_type_set) {
                        if (mpeg2_parser->IsProgressive()) {
                            if (mpeg2_parser->GetVerticalSize() == 1080) {
                                input->mpeg2lg_signal_standard = MXF_SIGNAL_STANDARD_SMPTE274M;
                                input->mpeg2lg_frame_layout = MXF_FULL_FRAME;
                            } else if (mpeg2_parser->GetVerticalSize() == 720) {
                                input->mpeg2lg_signal_standard = MXF_SIGNAL_STANDARD_SMPTE296M;
                                input->mpeg2lg_frame_layout = MXF_FULL_FRAME;
                            } else {
                                log_error("Unexpected MPEG-2 Long GOP vertical size %u; expected 1080 or 720\n",
                                          mpeg2_parser->GetVerticalSize());
                                throw false;
                            }
                        } else {
                            if (mpeg2_parser->GetVerticalSize() != 1080) {
                                log_error("Unexpected MPEG-2 Long GOP vertical size %u; expecting 1080 \n",
                                          mpeg2_parser->GetVerticalSize());
                                throw false;
                            }
                            input->mpeg2lg_signal_standard = MXF_SIGNAL_STANDARD_SMPTE274M;
                            input->mpeg2lg_frame_layout = MXF_SEPARATE_FIELDS;
                        }
                    }

                    if (!frame_rate_set)
                        frame_rate = mpeg2_parser->GetFrameRate();

                    if (!input->aspect_ratio_set)
                        input->aspect_ratio = mpeg2_parser->GetAspectRatio();
                }
            }
        }


        // complete commandline parsing

        start_timecode.Init(frame_rate, false);
        if (start_timecode_str && !parse_timecode(start_timecode_str, frame_rate, &start_timecode)) {
            usage(argv[0]);
            log_error("Invalid value '%s' for option '-t'\n", start_timecode_str);
            throw false;
        }

        if (partition_interval_str &&
            !parse_partition_interval(partition_interval_str, frame_rate, &partition_interval))
        {
            usage(argv[0]);
            log_error("Invalid value '%s' for option '--part'\n", partition_interval_str);
            throw false;
        }

        if (segmentation_filename &&
            !parse_segmentation_file(segmentation_filename, frame_rate, &segments, &filler_complete_segments))
        {
            log_error("Failed to parse segmentation file '%s'\n", segmentation_filename);
            throw false;
        }


        // check support for essence type and frame/sampling rates
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->essence_type == AS11_PCM) {
                if (!AS11Track::IsSupported(clip_type, input->essence_type, false, input->sampling_rate)) {
                    log_error("PCM sampling rate %d/%d not supported\n",
                              input->sampling_rate.numerator, input->sampling_rate.denominator);
                    throw false;
                }
            } else {
                bool is_mpeg2lg_720p = false;
                if (input->mpeg2lg_signal_standard == MXF_SIGNAL_STANDARD_SMPTE296M &&
                    (input->essence_type == AS11_MPEG2LG_422P_HL ||
                        input->essence_type == AS11_MPEG2LG_MP_HL))
                {
                    is_mpeg2lg_720p = true;
                }

                if (!AS11Track::IsSupported(clip_type, input->essence_type, is_mpeg2lg_720p, frame_rate)) {
                    log_error("Essence type '%s' @%d/%d fps not supported for clip type '%s'\n",
                              AS11Track::EssenceTypeToString(input->essence_type).c_str(),
                              frame_rate.numerator, frame_rate.denominator,
                              AS11Clip::AS11ClipTypeToString(input->clip_type).c_str());
                    throw false;
                }
            }
        }

        AS11Clip *clip = 0;
        switch (clip_type)
        {
            case AS11_OP1A_CLIP_TYPE:
                clip = AS11Clip::OpenNewOP1AClip(output_filename, frame_rate);
                break;
            case AS11_D10_CLIP_TYPE:
                clip = AS11Clip::OpenNewD10Clip(output_filename, frame_rate);
                break;
            case AS11_UNKNOWN_CLIP_TYPE:
                IM_ASSERT(false);
                break;
        }


        // initialize data structures and inputs

        clip->SetStartTimecode(start_timecode);
        if (partition_interval > 0)
            clip->SetPartitionInterval(partition_interval);
        if (output_start_offset < 0)
            clip->SetOutputStartOffset(output_start_offset);
        if (output_end_offset < 0)
            clip->SetOutputEndOffset(- output_end_offset);

        if (clip_name) {
            clip->SetClipName(clip_name);
        } else {
            for (i = 0; i < framework_properties.size(); i++) {
                if (framework_properties[i].type == AS11_CORE_FRAMEWORK_TYPE &&
                    framework_properties[i].name == "ProgrammeTitle")
                {
                    clip->SetClipName(framework_properties[i].value);
                    break;
                }
            }
        }

        uint32_t picture_track_count = 0;
        uint32_t sound_track_count = 0;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

            // open raw file reader

            if (!open_raw_reader(input))
                throw false;


            // create track and configure

            bool is_picture = (input->essence_type != AS11_PCM);
            input->track = clip->CreateTrack(input->essence_type);

            if (input->track_number_set)
                input->track->SetTrackNumber(input->track_number);

            switch (input->essence_type)
            {
                case AS11_IEC_DV25:
                case AS11_DVBASED_DV25:
                case AS11_DV50:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case AS11_DV100_1080I:
                case AS11_DV100_1080P:
                case AS11_DV100_720P:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetComponentDepth(input->component_depth);
                    break;
                case AS11_D10_30:
                case AS11_D10_40:
                case AS11_D10_50:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    if (input->raw_reader->GetFixedSampleSize() != 0)
                        input->track->SetSampleSize(input->raw_reader->GetFixedSampleSize());
                    break;
                case AS11_AVCI100_1080I:
                case AS11_AVCI100_1080P:
                case AS11_AVCI100_720P:
                case AS11_AVCI50_1080I:
                case AS11_AVCI50_1080P:
                case AS11_AVCI50_720P:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case AS11_UNC_SD:
                case AS11_UNC_HD_1080I:
                case AS11_UNC_HD_1080P:
                case AS11_UNC_HD_720P:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetComponentDepth(input->component_depth);
                    break;
                case AS11_MPEG2LG_422P_HL:
                case AS11_MPEG2LG_MP_HL:
                case AS11_MPEG2LG_MP_H14:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetMPEG2LGSignalStandard(input->mpeg2lg_signal_standard);
                    input->track->SetMPEG2LGFrameLayout(input->mpeg2lg_frame_layout);
                    break;
                case AS11_PCM:
                    input->track->SetSamplingRate(input->sampling_rate);
                    input->track->SetQuantizationBits(input->audio_quant_bits);
                    if (input->locked_set)
                        input->track->SetLocked(input->locked);
                    if (input->audio_ref_level_set)
                        input->track->SetAudioRefLevel(input->audio_ref_level);
                    if (input->dial_norm_set)
                        input->track->SetDialNorm(input->dial_norm);
                    if (sequence_offset_set || clip_type == AS11_D10_CLIP_TYPE)
                        input->track->SetSequenceOffset(sequence_offset);
                    break;
                case AS11_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }

            // set input sample info and create buffer
            switch (input->essence_type)
            {
                case AS11_IEC_DV25:
                case AS11_DVBASED_DV25:
                case AS11_DV50:
                case AS11_DV100_1080I:
                case AS11_DV100_1080P:
                case AS11_DV100_720P:
                case AS11_D10_30:
                case AS11_D10_40:
                case AS11_D10_50:
                case AS11_AVCI100_1080I:
                case AS11_AVCI100_1080P:
                case AS11_AVCI100_720P:
                case AS11_AVCI50_1080I:
                case AS11_AVCI50_1080P:
                case AS11_AVCI50_720P:
                case AS11_UNC_SD:
                case AS11_UNC_HD_1080I:
                case AS11_UNC_HD_1080P:
                case AS11_UNC_HD_720P:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader->GetFixedSampleSize() == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    break;
                case AS11_MPEG2LG_422P_HL:
                case AS11_MPEG2LG_MP_HL:
                case AS11_MPEG2LG_MP_H14:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MPEG2EssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case AS11_PCM:
                    {
                        vector<uint32_t> shifted_sample_sequence = input->track->GetShiftedSampleSequence();
                        IM_ASSERT(shifted_sample_sequence.size() < sizeof(input->sample_sequence) / sizeof(uint32_t));
                        memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                               shifted_sample_sequence.size() * sizeof(uint32_t));
                        input->sample_sequence_size = shifted_sample_sequence.size();
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    }
                    break;
                case AS11_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }

            if (is_picture)
                picture_track_count++;
            else
                sound_track_count++;
        }


        // add descriptive metadata

        bool have_dpp_properties = false;
        bool have_dpp_total_number_of_parts = false;
        bool have_dpp_total_programme_duration = false;

        clip->PrepareHeaderMetadata();
        AS11DMS::RegisterExtensions(clip->GetDataModel());
        UKDPPDMS::RegisterExtensions(clip->GetDataModel());

        FrameworkHelper as11_framework_h(clip->GetDataModel(), new AS11CoreFramework(clip->GetHeaderMetadata()));
        FrameworkHelper dpp_framework_h(clip->GetDataModel(), new UKDPPFramework(clip->GetHeaderMetadata()));

        for (i = 0; i < framework_properties.size(); i++) {
            switch (framework_properties[i].type)
            {
                case AS11_CORE_FRAMEWORK_TYPE:
                    if (!as11_framework_h.SetProperty(framework_properties[i].name, framework_properties[i].value)) {
                        log_warn("Failed to set AS11CoreFramework property '%s' to '%s'\n",
                                 framework_properties[i].name.c_str(), framework_properties[i].value.c_str());
                    }
                    break;
                case DPP_FRAMEWORK_TYPE:
                    if (!dpp_framework_h.SetProperty(framework_properties[i].name, framework_properties[i].value)) {
                        log_warn("Failed to set UKDPPFramework property '%s' to '%s'\n",
                                 framework_properties[i].name.c_str(), framework_properties[i].value.c_str());
                    } else {
                        have_dpp_properties = true;
                    }
                    break;
            }
        }

        clip->InsertAS11CoreFramework(dynamic_cast<AS11CoreFramework*>(as11_framework_h.GetFramework()));
        IM_CHECK_M(as11_framework_h.GetFramework()->validate(true), ("AS11 Framework validation failed"));

        if (!segments.empty())
            clip->InsertTCSegmentation(segments);

        if (have_dpp_properties) {
            clip->InsertUKDPPFramework(dynamic_cast<UKDPPFramework*>(dpp_framework_h.GetFramework()));

            // make sure UKDPPTotalNumberOfParts and UKDPPTotalProgrammeDuration are set (default 0) for validation
            UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(dpp_framework_h.GetFramework());
            if (dpp_framework->haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalNumberOfParts)))
                have_dpp_total_number_of_parts = true;
            else
                dpp_framework->SetTotalNumberOfParts(0);
            if (dpp_framework->haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalProgrammeDuration)))
                have_dpp_total_programme_duration = true;
            else
                dpp_framework->SetTotalProgrammeDuration(0);

            IM_CHECK_M(dpp_framework->validate(true), ("UK DPP Framework validation failed"));
        } else {
            IM_CHECK(mxf_remove_set(clip->GetHeaderMetadata()->getCHeaderMetadata(),
                                    dpp_framework_h.GetFramework()->getCMetadataSet()));
        }


        clip->PrepareWrite();

        // write samples
        while (duration < 0 || clip->GetDuration() < duration) {
            // read samples into input buffers first to ensure the frame data is available for all tracks
            for (i = 0; i < inputs.size(); i++) {
                RawInput *input = &inputs[i];
                if (!read_samples(input))
                    break;
            }
            if (i != inputs.size())
                break;

            // write samples from input buffers
            for (i = 0; i < inputs.size(); i++) {
                RawInput *input = &inputs[i];
                input->track->WriteSamples(input->raw_reader->GetSampleData(),
                                           input->raw_reader->GetSampleDataSize(),
                                           input->raw_reader->GetNumSamples());
            }
        }

        if (!segments.empty())
            clip->CompleteSegmentation(filler_complete_segments);

        if (have_dpp_properties) {
            // calculate or check total number of parts and programme duration
            UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(dpp_framework_h.GetFramework());
            if (have_dpp_total_number_of_parts) {
                if (clip->GetTotalSegments() != dpp_framework->GetTotalNumberOfParts()) {
                    log_warn("UKDPPTotalNumberOfParts value %u does not equal actual total part count %u\n",
                             dpp_framework->GetTotalNumberOfParts(), clip->GetTotalSegments());
                }
            } else {
                dpp_framework->SetTotalNumberOfParts(clip->GetTotalSegments());
            }
            if (have_dpp_total_programme_duration) {
                if (clip->GetTotalSegmentDuration() != dpp_framework->GetTotalProgrammeDuration()) {
                    log_warn("UKDPPTotalProgrammeDuration value %u does not equal actual total part duration %u\n",
                             dpp_framework->GetTotalProgrammeDuration(), clip->GetTotalSegmentDuration());
                }
            } else {
                dpp_framework->SetTotalProgrammeDuration(clip->GetTotalSegmentDuration());
            }
        }


        clip->CompleteWrite();

        log_info("Duration: %s (%"PRId64" frames @%d/%d fps)\n",
                 get_generic_duration_string(clip->GetDuration(), clip->GetFrameRate()).c_str(),
                 clip->GetDuration(), clip->GetFrameRate().numerator, clip->GetFrameRate().denominator);


        // clean up
        for (i = 0; i < inputs.size(); i++)
            clear_input(&inputs[i]);
        delete clip;
    }
    catch (const MXFException &ex)
    {
        log_error("MXF exception caught: %s\n", ex.getMessage().c_str());
        cmd_result = 1;
    }
    catch (const IMException &ex)
    {
        log_error("IM exception caught: %s\n", ex.what());
        cmd_result = 1;
    }
    catch (const bool &ex)
    {
        cmd_result = 1;
    }
    catch (...)
    {
        log_error("Unknown exception caught\n");
        cmd_result = 1;
    }


    if (log_filename)
        close_log_file();


    return cmd_result;
}

