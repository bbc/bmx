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

#include <im/avid_mxf/AvidClip.h>
#include <im/avid_mxf/AvidDVTrack.h>
#include <im/avid_mxf/AvidMPEG2LGTrack.h>
#include <im/avid_mxf/AvidMJPEGTrack.h>
#include <im/avid_mxf/AvidD10Track.h>
#include <im/avid_mxf/AvidAVCITrack.h>
#include <im/avid_mxf/AvidVC3Track.h>
#include <im/avid_mxf/AvidPCMTrack.h>
#include <im/essence_parser/DVEssenceParser.h>
#include <im/essence_parser/MPEG2EssenceParser.h>
#include <im/essence_parser/MJPEGEssenceParser.h>
#include <im/essence_parser/AVCIRawEssenceReader.h>
#include <im/essence_parser/VC3EssenceParser.h>
#include <im/essence_parser/RawEssenceReader.h>
#include <im/URI.h>
#include <im/Utils.h>
#include <im/MXFUtils.h>
#include "../AppUtils.h"
#include <im/IMException.h>
#include <im/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    const char *position_str;
    AvidLocator locator;
} LocatorOption;

typedef enum
{
    NO_ESSENCE_GROUP = 0,
    DV_ESSENCE_GROUP,
    VC3_ESSENCE_GROUP,
    MPEG2LG_ESSENCE_GROUP,
    D10_ESSENCE_GROUP,
} EssenceTypeGroup;

typedef struct
{
    EssenceTypeGroup essence_type_group;
    AvidEssenceType essence_type;

    AvidTrack *track;

    const char *filename;
    int64_t file_start_offset;
    int64_t file_max_length;

    int64_t output_end_offset;

    RawEssenceReader *raw_reader;
    uint32_t sample_sequence[32];
    size_t sample_sequence_size;
    size_t sample_sequence_offset;

    // picture
    mxfRational aspect_ratio;
    bool aspect_ratio_set;
    uint32_t component_depth;

    // sound
    mxfRational sampling_rate;
    uint32_t audio_quant_bits;
    bool locked;
    bool locked_set;
    int8_t audio_ref_level;
    bool audio_ref_level_set;
    int8_t dial_norm;
    bool dial_norm_set;
    bool sequence_offset_set;
    uint8_t sequence_offset;

    // MPEG-2 Long GOP
    bool image_type_set;
    uint8_t mpeg2lg_signal_standard;
    uint8_t mpeg2lg_frame_layout;
} RawInput;


typedef struct
{
    const char *color_str;
    AvidRGBColor color;
} ColorMap;

static ColorMap COLOR_MAP[] =
{
    {"white",   AVID_WHITE},
    {"red",     AVID_RED},
    {"yellow",  AVID_YELLOW},
    {"green",   AVID_GREEN},
    {"cyan",    AVID_CYAN},
    {"blue",    AVID_BLUE},
    {"magenta", AVID_MAGENTA},
    {"black",   AVID_BLACK},
};

#define COLOR_MAP_SIZE (sizeof(COLOR_MAP) / sizeof(ColorMap))


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

    if (input->essence_type == AVID_AVCI100_1080I ||
        input->essence_type == AVID_AVCI100_1080P ||
        input->essence_type == AVID_AVCI100_720P ||
        input->essence_type == AVID_AVCI50_1080I ||
        input->essence_type == AVID_AVCI50_1080P ||
        input->essence_type == AVID_AVCI50_720P)
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

static bool parse_color(const char *color_str, AvidRGBColor *color)
{
    size_t i;
    for (i = 0; i < COLOR_MAP_SIZE; i++) {
        if (strcmp(COLOR_MAP[i].color_str, color_str) == 0) {
            *color = COLOR_MAP[i].color;
            return true;
        }
    }

    return false;
}

static string get_version_info()
{
    char buffer[256];
    sprintf(buffer, "raw2avidmxf, %s v%s (%s %s)", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
    return buffer;
}

static void usage(const char* cmd)
{
    fprintf(stderr, "%s\n", get_version_info().c_str());
    fprintf(stderr, "Usage: %s <<options>> [<<input options>> <input>]+\n", cmd);
    fprintf(stderr, "Options (* means required):\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -l <file>               Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, "* -p <name>               Output file prefix\n");
    fprintf(stderr, "  -f <rate>               Frame rate: 25, 30 (30000/1001), 50 or 60 (60000/1001). Default parsed or 25\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --project <name>        Set the project name\n");
    fprintf(stderr, "  --tape <name>           Source tape name\n");
    fprintf(stderr, "  --comment <string>      Add 'Comments' user comment to the MaterialPackage\n");
    fprintf(stderr, "  --desc <string>         Add 'Descript' user comment to the MaterialPackage\n");
    fprintf(stderr, "  --tag <name> <value>    Add <name> user comment to the MaterialPackage. Option can be used multiple times\n");
    fprintf(stderr, "  --locator <position> <comment> <color>\n");
    fprintf(stderr, "                          Add locator at <position> with <comment> and <color>\n");
    fprintf(stderr, "                          <position> format is o?hh:mm:sscff, where the optional 'o' indicates it is an offset\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames. Default is minimum available duration\n");
    fprintf(stderr, "Input Options (must precede the input to which it applies):\n");
    fprintf(stderr, "  -a <n:d>                Image aspect ratio. Either 4:3 or 16:9. Default parsed or 16:9\n");
    fprintf(stderr, "  -c <depth>              Component depth for uncompressed/DV100 video. Either 8 or 10. Default parsed or 8\n");
    fprintf(stderr, "  -q <bps>                Audio quantization bits per sample. Either 16 or 24. Default 16\n");
    fprintf(stderr, "  --locked <bool>         Indicate whether the number of audio samples is locked to the video. Either true or false. Default not set\n");
    fprintf(stderr, "  --audio-ref <level>     Audio reference level, number of dBm for 0VU. Default not set\n");
    fprintf(stderr, "  --dial-norm <value>     Gain to be applied to normalize perceived loudness of the clip. Default not set\n");
    fprintf(stderr, "  --seq-off <value>       Sound sample sequence offset. Default not set (0)\n");
    fprintf(stderr, "  --off <bytes>           Skip bytes at start of the next input/track's file\n");
    fprintf(stderr, "  --maxlen <bytes>        Maximum number of bytes to read from next input/track's file\n");
    fprintf(stderr, "  --imgtype <type>        MPEG-2 Long GOP video type: '1080i', '1080p', '720p'. Default '1080i'\n");
    fprintf(stderr, "  --out-end <offset>      Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    fprintf(stderr, "Inputs:\n");
    fprintf(stderr, "  --dv <name>             Raw DV video input file\n");
    fprintf(stderr, "  --iecdv25 <name>        Raw IEC DV25 video input file\n");
    fprintf(stderr, "  --dvbased25 <name>      Raw DV-Based DV25 video input file\n");
    fprintf(stderr, "  --dv50 <name>           Raw DV50 video input file\n");
    fprintf(stderr, "  --dv100_1080i <name>    Raw DV100 1080i video input file\n");
    fprintf(stderr, "  --dv100_720p <name>     Raw DV100 720p video input file\n");
    fprintf(stderr, "  --mpeg2lg <name>          Raw MPEG-2 Long GOP video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl <name>  Raw MPEG-2 Long GOP 422P@HL (eg. XDCAM HD422) video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl <name>    Raw MPEG-2 Long GOP MP@HL (eg. XDCAM EX) video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_h14 <name>   Raw MPEG-2 Long GOP MP@H14 (eg. XDCAM HD / HDV) video input file\n");
    fprintf(stderr, "  --mjpeg21 <name>        Raw Avid MJPEG 2:1 video input file\n");
    fprintf(stderr, "  --mjpeg31 <name>        Raw Avid MJPEG 3:1 video input file\n");
    fprintf(stderr, "  --mjpeg101 <name>       Raw Avid MJPEG 10:1 video input file\n");
    fprintf(stderr, "  --mjpeg201 <name>       Raw Avid MJPEG 20:1 video input file\n");
    fprintf(stderr, "  --mjpeg41m <name>       Raw Avid MJPEG 4:1m video input file\n");
    fprintf(stderr, "  --mjpeg101m <name>      Raw Avid MJPEG 10:1m video input file\n");
    fprintf(stderr, "  --mjpeg151s <name>      Raw Avid MJPEG 15:1s video input file\n");
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
    fprintf(stderr, "  --vc3 <name>            Raw VC3/DNxHD input file\n");
    fprintf(stderr, "  --vc3_1080p_1235 <name> Raw VC3/DNxHD 1920x1080p 220/185/175 Mbps 10bit input file\n");
    fprintf(stderr, "  --vc3_1080p_1237 <name> Raw VC3/DNxHD 1920x1080p 145/120/115 Mbps input file\n");
    fprintf(stderr, "  --vc3_1080p_1238 <name> Raw VC3/DNxHD 1920x1080p 220/185/175 Mbps input file\n");
    fprintf(stderr, "  --vc3_1080i_1241 <name> Raw VC3/DNxHD 1920x1080i 220/185 Mbps 10bit input file\n");
    fprintf(stderr, "  --vc3_1080i_1242 <name> Raw VC3/DNxHD 1920x1080i 145/120 Mbps input file\n");
    fprintf(stderr, "  --vc3_1080i_1243 <name> Raw VC3/DNxHD 1920x1080i 220/185 Mbps input file\n");
    fprintf(stderr, "  --vc3_720p_1250 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps 10bit input file\n");
    fprintf(stderr, "  --vc3_720p_1251 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps input file\n");
    fprintf(stderr, "  --vc3_720p_1252 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps input file\n");
    fprintf(stderr, "  --vc3_1080p_1253 <name> Raw VC3/DNxHD 1920x1080p 45/36 Mbps input file\n");
    fprintf(stderr, "  --pcm <name>            Raw PCM audio input file\n");
}

int main(int argc, const char** argv)
{
    const char *log_filename = 0;
    const char *prefix = "";
    vector<RawInput> inputs;
    RawInput input;
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    mxfRational frame_rate = FRAME_RATE_25;
    bool frame_rate_set = false;
    const char *clip_name = 0;
    const char *project_name = 0;
    const char *tape_name = 0;
    map<string, string> user_comments;
    vector<LocatorOption> locators;
    int64_t duration = -1;
    bool do_print_version = false;
    int value, num, den;
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
        else if (strcmp(argv[cmdln_index], "-p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            prefix = argv[cmdln_index + 1];
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
        else if (strcmp(argv[cmdln_index], "--project") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            project_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--tape") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            tape_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--comment") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            user_comments["Comments"] = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--desc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for %s\n", argv[cmdln_index]);
                return 1;
            }
            user_comments["Descript"] = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--tag") == 0)
        {
            if (cmdln_index + 2 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for %s\n", argv[cmdln_index]);
                return 1;
            }
            user_comments[argv[cmdln_index + 1]] = argv[cmdln_index + 2];
            cmdln_index += 2;
        }
        else if (strcmp(argv[cmdln_index], "--locator") == 0)
        {
            if (cmdln_index + 3 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for %s\n", argv[cmdln_index]);
                return 1;
            }
            LocatorOption locator;
            locator.position_str = argv[cmdln_index + 1];
            locator.locator.comment = argv[cmdln_index + 2];
            if (!parse_color(argv[cmdln_index + 3], &locator.locator.color))
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to read --locator <color> value '%s'\n", argv[cmdln_index + 3]);
                return 1;
            }
            locators.push_back(locator);
            cmdln_index += 3;
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
            input.sequence_offset = (uint8_t)value;
            input.sequence_offset_set = true;
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
        else if (strcmp(argv[cmdln_index], "--out-end") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &input.output_end_offset) != 1 ||
                input.output_end_offset < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
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
            input.essence_type = AVID_IEC_DV25;
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
            input.essence_type = AVID_DVBASED_DV25;
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
            input.essence_type = AVID_DV50;
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
            input.essence_type = AVID_DV100_1080I;
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
            input.essence_type = AVID_DV100_720P;
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
            input.essence_type = AVID_MPEG2LG_422P_HL;
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
            input.essence_type = AVID_MPEG2LG_MP_HL;
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
            input.essence_type = AVID_MPEG2LG_MP_H14;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mjpeg21") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_MJPEG_2_1;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mjpeg31") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_MJPEG_3_1;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mjpeg101") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_MJPEG_10_1;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mjpeg201") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_MJPEG_20_1;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mjpeg41m") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_MJPEG_4_1M;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mjpeg101m") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_MJPEG_10_1M;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mjpeg151s") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_MJPEG_15_1S;
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
            input.essence_type = AVID_D10_30;
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
            input.essence_type = AVID_D10_40;
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
            input.essence_type = AVID_D10_50;
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
            input.essence_type = AVID_AVCI100_1080I;
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
            input.essence_type = AVID_AVCI100_1080P;
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
            input.essence_type = AVID_AVCI100_720P;
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
            input.essence_type = AVID_AVCI50_1080I;
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
            input.essence_type = AVID_AVCI50_1080P;
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
            input.essence_type = AVID_AVCI50_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type_group = VC3_ESSENCE_GROUP;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080p_1235") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_1080P_1235;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080p_1237") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_1080P_1237;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080p_1238") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_1080P_1238;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080i_1241") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_1080I_1241;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080i_1242") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_1080I_1242;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080i_1243") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_1080I_1243;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_720p_1250") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_720P_1250;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_720p_1251") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_720P_1251;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_720p_1252") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_720P_1252;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080p_1253") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_VC3_1080P_1253;
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
            input.essence_type = AVID_PCM;
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

    if (cmdln_index != argc) {
        usage(argv[0]);
        fprintf(stderr, "Unknown option '%s'\n", argv[cmdln_index]);
        return 1;
    }

    if (inputs.empty()) {
        usage(argv[0]);
        fprintf(stderr, "No raw inputs given\n");
        return 1;
    }
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

    if (prefix == 0) {
        usage(argv[0]);
        fprintf(stderr, "Missing output prefix (-p)\n");
        return 1;
    }


    if (log_filename) {
        if (!open_log_file(log_filename))
            return 1;
    }

    connect_libmxf_logging();

    if (IM_REGRESSION_TEST) {
        mxf_set_regtest_funcs();
        mxf_avid_set_regtest_funcs();
    }

    if (do_print_version)
        log_info("%s\n", get_version_info().c_str());


    int cmd_result = 0;
    try
    {
        // extract essence info
        size_t i;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

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
                input->essence_type == AVID_IEC_DV25 ||
                input->essence_type == AVID_DVBASED_DV25 ||
                input->essence_type == AVID_DV50 ||
                input->essence_type == AVID_DV100_1080I ||
                input->essence_type == AVID_DV100_720P)
            {
                DVEssenceParser *dv_parser = new DVEssenceParser();
                input->raw_reader->SetEssenceParser(dv_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to IEC DV-25 if no essence samples
                    if (input->essence_type_group == DV_ESSENCE_GROUP)
                        input->essence_type = AVID_IEC_DV25;
                } else {
                    dv_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == DV_ESSENCE_GROUP) {
                        switch (dv_parser->GetEssenceType())
                        {
                            case DVEssenceParser::IEC_DV25:
                                input->essence_type = AVID_IEC_DV25;
                                break;
                            case DVEssenceParser::DVBASED_DV25:
                                input->essence_type = AVID_DVBASED_DV25;
                                break;
                            case DVEssenceParser::DV50:
                                input->essence_type = AVID_DV50;
                                break;
                            case DVEssenceParser::DV100_1080I:
                                input->essence_type = AVID_DV100_1080I;
                                break;
                            case DVEssenceParser::DV100_720P:
                                input->essence_type = AVID_DV100_720P;
                                break;
                            case DVEssenceParser::UNKNOWN_DV:
                                log_error("Unknown DV essence type\n");
                                throw false;
                        }
                    }

                    input->raw_reader->SetFixedSampleSize(dv_parser->GetFrameSize());

                    if (!frame_rate_set) {
                        if (dv_parser->Is50Hz()) {
                            if (input->essence_type == AVID_DV100_720P)
                                frame_rate = FRAME_RATE_50;
                            else
                                frame_rate = FRAME_RATE_25;
                        } else {
                            if (input->essence_type == AVID_DV100_720P)
                                frame_rate = FRAME_RATE_5994;
                            else
                                frame_rate = FRAME_RATE_2997;
                        }
                    }

                    if (!input->aspect_ratio_set)
                        input->aspect_ratio = dv_parser->GetAspectRatio();
                }
            }
            else if (input->essence_type_group == VC3_ESSENCE_GROUP)
            {
                VC3EssenceParser *vc3_parser = new VC3EssenceParser();
                input->raw_reader->SetEssenceParser(vc3_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to AVID_VC3_1080I_1242 if no essence samples
                    input->essence_type = AVID_VC3_1080I_1242;
                } else {
                    vc3_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    switch (vc3_parser->GetCompressionId())
                    {
                        case 1235:
                            input->essence_type = AVID_VC3_1080P_1235;
                            break;
                        case 1237:
                            input->essence_type = AVID_VC3_1080P_1237;
                            break;
                        case 1238:
                            input->essence_type = AVID_VC3_1080P_1238;
                            break;
                        case 1241:
                            input->essence_type = AVID_VC3_1080I_1241;
                            break;
                        case 1242:
                            input->essence_type = AVID_VC3_1080I_1242;
                            break;
                        case 1243:
                            input->essence_type = AVID_VC3_1080I_1243;
                            break;
                        case 1250:
                            input->essence_type = AVID_VC3_720P_1250;
                            break;
                        case 1251:
                            input->essence_type = AVID_VC3_720P_1251;
                            break;
                        case 1252:
                            input->essence_type = AVID_VC3_720P_1252;
                            break;
                        case 1253:
                            input->essence_type = AVID_VC3_1080P_1253;
                            break;
                        default:
                            log_error("Unknown VC3 essence type\n");
                            throw false;
                    }
                }
            }
            else if (input->essence_type_group == D10_ESSENCE_GROUP ||
                     input->essence_type == AVID_D10_30 ||
                     input->essence_type == AVID_D10_40 ||
                     input->essence_type == AVID_D10_50)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to D10 50Mbps if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = AVID_D10_50;
                } else {
                    input->raw_reader->SetFixedSampleSize(input->raw_reader->GetSampleDataSize());

                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == D10_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetBitRate())
                        {
                            case 75000:
                                input->essence_type = AVID_D10_30;
                                break;
                            case 100000:
                                input->essence_type = AVID_D10_40;
                                break;
                            case 125000:
                                input->essence_type = AVID_D10_50;
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
                     input->essence_type == AVID_MPEG2LG_422P_HL ||
                     input->essence_type == AVID_MPEG2LG_MP_HL ||
                     input->essence_type == AVID_MPEG2LG_MP_H14)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to 422P@HL if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = AVID_MPEG2LG_422P_HL;
                } else {
                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetProfileAndLevel())
                        {
                            case 0x82:
                                input->essence_type = AVID_MPEG2LG_422P_HL;
                                break;
                            case 0x44:
                                input->essence_type = AVID_MPEG2LG_MP_HL;
                                break;
                            case 0x46:
                                input->essence_type = AVID_MPEG2LG_MP_H14;
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

        for (i = 0; i < locators.size(); i++) {
            if (!parse_position(locators[i].position_str, start_timecode, frame_rate, &locators[i].locator.position)) {
                usage(argv[0]);
                log_error("Invalid value '%s' for option '--locator'\n", locators[i].position_str);
                throw false;
            }
        }


        // check frame/sampling rates
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->essence_type == AVID_PCM) {
                if (!AvidTrack::IsSupported(input->essence_type, false, input->sampling_rate)) {
                    log_error("PCM sampling rate %d/%d not supported\n",
                              input->sampling_rate.numerator, input->sampling_rate.denominator);
                    throw false;
                }
            } else {
                bool is_mpeg2lg_720p = false;
                if (input->mpeg2lg_signal_standard == MXF_SIGNAL_STANDARD_SMPTE296M &&
                    (input->essence_type == AVID_MPEG2LG_422P_HL ||
                        input->essence_type == AVID_MPEG2LG_MP_HL))
                {
                    is_mpeg2lg_720p = true;
                }

                if (!AvidTrack::IsSupported(input->essence_type, is_mpeg2lg_720p, frame_rate)) {
                    log_error("Frame rate %d/%d not supported for format '%s'\n",
                              frame_rate.numerator, frame_rate.denominator,
                              MXFDescriptorHelper::EssenceTypeToString(AvidTrack::ConvertEssenceType(input->essence_type)).c_str());
                    throw false;
                }
            }
        }


        AvidClip *clip = new AvidClip(frame_rate, prefix);


        // initialize data structures and inputs

        if (clip_name)
            clip->SetClipName(clip_name);
        if (project_name)
            clip->SetProjectName(project_name);
        clip->SetStartTimecode(start_timecode);

        for (i = 0; i < locators.size(); i++)
            clip->AddLocator(locators[i].locator);

        map<string, string>::const_iterator iter;
        for (iter = user_comments.begin(); iter != user_comments.end(); iter++)
            clip->SetUserComment(iter->first, iter->second);

        SourcePackage *tape_package = 0;
        vector<pair<mxfUMID, uint32_t> > tape_package_video_refs;
        vector<pair<mxfUMID, uint32_t> > tape_package_audio_refs;
        if (tape_name) {
            uint32_t num_video_tracks = 0;
            uint32_t num_audio_tracks = 0;
            for (i = 0; i < inputs.size(); i++) {
                if (inputs[i].essence_type == AVID_PCM)
                    num_audio_tracks++;
                else
                    num_video_tracks++;
            }

            tape_package = clip->CreateDefaultTapeSource(tape_name, num_video_tracks, num_audio_tracks);

            tape_package_video_refs = clip->GetPictureSourceReferences(tape_package);
            IM_ASSERT(tape_package_video_refs.size() == num_video_tracks);
            tape_package_audio_refs = clip->GetSoundSourceReferences(tape_package);
            IM_ASSERT(tape_package_audio_refs.size() == num_audio_tracks);
        }

        AvidDVTrack *dv_track = 0;
        AvidMPEG2LGTrack *mpeg2lg_track = 0;
        AvidMJPEGTrack *mjpeg_track = 0;
        AvidD10Track *d10_track = 0;
        AvidAVCITrack *avci_track = 0;
        AvidVC3Track *vc3_track = 0;
        AvidPCMTrack *pcm_track = 0;
        uint32_t video_track_count = 0;
        uint32_t audio_track_count = 0;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            bool is_video = (input->essence_type != AVID_PCM);

            // open raw file reader

            if (!open_raw_reader(input))
                throw false;


            // create track and configure

            input->track = clip->CreateTrack(input->essence_type);
            input->track->SetOutputEndOffset(- input->output_end_offset);

            if (tape_package) {
                if (is_video) {
                    input->track->SetSourceRef(tape_package_video_refs[video_track_count].first,
                                               tape_package_video_refs[video_track_count].second);
                } else {
                    input->track->SetSourceRef(tape_package_video_refs[audio_track_count].first,
                                               tape_package_video_refs[audio_track_count].second);
                }
            } else {
                vector<pair<mxfUMID, uint32_t> > source_refs;
                string name = strip_path(input->filename);
                URI uri;
                uri.ParseFilename(input->filename);
                if (is_video) {
                    SourcePackage *import_package = clip->CreateDefaultImportSource(uri.ToString(), name, 1, 0);
                    source_refs = clip->GetPictureSourceReferences(import_package);
                } else {
                    SourcePackage *import_package = clip->CreateDefaultImportSource(uri.ToString(), name, 0, 1);
                    source_refs = clip->GetSoundSourceReferences(import_package);
                }
                IM_ASSERT(source_refs.size() == 1);
                input->track->SetSourceRef(source_refs[0].first, source_refs[0].second);
            }

            switch (input->essence_type)
            {
                case AVID_IEC_DV25:
                case AVID_DVBASED_DV25:
                case AVID_DV50:
                    dv_track = dynamic_cast<AvidDVTrack*>(input->track);
                    dv_track->SetAspectRatio(input->aspect_ratio);
                    break;
                case AVID_DV100_1080I:
                case AVID_DV100_720P:
                    dv_track = dynamic_cast<AvidDVTrack*>(input->track);
                    dv_track->SetAspectRatio(input->aspect_ratio);
                    dv_track->SetComponentDepth(input->component_depth);
                    break;
                case AVID_MPEG2LG_422P_HL:
                case AVID_MPEG2LG_MP_HL:
                case AVID_MPEG2LG_MP_H14:
                    mpeg2lg_track = dynamic_cast<AvidMPEG2LGTrack*>(input->track);
                    mpeg2lg_track->SetSignalStandard(input->mpeg2lg_signal_standard);
                    mpeg2lg_track->SetFrameLayout(input->mpeg2lg_frame_layout);
                    break;
                case AVID_MJPEG_2_1:
                case AVID_MJPEG_3_1:
                case AVID_MJPEG_10_1:
                case AVID_MJPEG_20_1:
                case AVID_MJPEG_4_1M:
                case AVID_MJPEG_10_1M:
                case AVID_MJPEG_15_1S:
                    mjpeg_track = dynamic_cast<AvidMJPEGTrack*>(input->track);
                    mjpeg_track->SetAspectRatio(input->aspect_ratio);
                    break;
                case AVID_D10_30:
                case AVID_D10_40:
                case AVID_D10_50:
                    d10_track = dynamic_cast<AvidD10Track*>(input->track);
                    d10_track->SetAspectRatio(input->aspect_ratio);
                    if (input->raw_reader->GetFixedSampleSize() != 0)
                        d10_track->SetSampleSize(input->raw_reader->GetFixedSampleSize());
                    break;
                case AVID_AVCI100_1080I:
                case AVID_AVCI100_1080P:
                case AVID_AVCI100_720P:
                case AVID_AVCI50_1080I:
                case AVID_AVCI50_1080P:
                case AVID_AVCI50_720P:
                    avci_track = dynamic_cast<AvidAVCITrack*>(input->track);
                    break;
                case AVID_VC3_1080P_1235:
                case AVID_VC3_1080P_1237:
                case AVID_VC3_1080P_1238:
                case AVID_VC3_1080I_1241:
                case AVID_VC3_1080I_1242:
                case AVID_VC3_1080I_1243:
                case AVID_VC3_720P_1250:
                case AVID_VC3_720P_1251:
                case AVID_VC3_720P_1252:
                case AVID_VC3_1080P_1253:
                    vc3_track = dynamic_cast<AvidVC3Track*>(input->track);
                    break;
                case AVID_PCM:
                    pcm_track = dynamic_cast<AvidPCMTrack*>(input->track);
                    pcm_track->SetSamplingRate(input->sampling_rate);
                    pcm_track->SetQuantizationBits(input->audio_quant_bits);
                    if (input->locked_set)
                        pcm_track->SetLocked(input->locked);
                    if (input->audio_ref_level_set)
                        pcm_track->SetAudioRefLevel(input->audio_ref_level);
                    if (input->dial_norm_set)
                        pcm_track->SetDialNorm(input->dial_norm);
                    if (input->sequence_offset_set)
                        pcm_track->SetSequenceOffset(input->sequence_offset);
                    break;
                case AVID_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }

            // set input sample info and create buffer
            switch (input->essence_type)
            {
                case AVID_IEC_DV25:
                case AVID_DVBASED_DV25:
                case AVID_DV50:
                case AVID_DV100_1080I:
                case AVID_DV100_720P:
                case AVID_D10_30:
                case AVID_D10_40:
                case AVID_D10_50:
                case AVID_AVCI100_1080I:
                case AVID_AVCI100_1080P:
                case AVID_AVCI100_720P:
                case AVID_AVCI50_1080I:
                case AVID_AVCI50_1080P:
                case AVID_AVCI50_720P:
                case AVID_VC3_1080P_1235:
                case AVID_VC3_1080P_1237:
                case AVID_VC3_1080P_1238:
                case AVID_VC3_1080I_1241:
                case AVID_VC3_1080I_1242:
                case AVID_VC3_1080I_1243:
                case AVID_VC3_720P_1250:
                case AVID_VC3_720P_1251:
                case AVID_VC3_720P_1252:
                case AVID_VC3_1080P_1253:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader->GetFixedSampleSize() == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    break;
                case AVID_MPEG2LG_422P_HL:
                case AVID_MPEG2LG_MP_HL:
                case AVID_MPEG2LG_MP_H14:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MPEG2EssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case AVID_MJPEG_2_1:
                case AVID_MJPEG_3_1:
                case AVID_MJPEG_10_1:
                case AVID_MJPEG_20_1:
                case AVID_MJPEG_4_1M:
                case AVID_MJPEG_10_1M:
                case AVID_MJPEG_15_1S:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MJPEGEssenceParser(mjpeg_track->IsSingleField()));
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case AVID_PCM:
                    {
                        vector<uint32_t> shifted_sample_sequence = pcm_track->GetShiftedSampleSequence();
                        IM_ASSERT(shifted_sample_sequence.size() < sizeof(input->sample_sequence) / sizeof(uint32_t));
                        memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                               shifted_sample_sequence.size() * sizeof(uint32_t));
                        input->sample_sequence_size = shifted_sample_sequence.size();
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    }
                    break;
                case AVID_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }


            if (is_video)
                video_track_count++;
            else
                audio_track_count++;
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
        log_error("Ingex Media exception caught: %s\n", ex.what());
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

