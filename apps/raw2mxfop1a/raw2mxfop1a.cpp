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

#include <im/mxf_op1a/OP1AFile.h>
#include <im/mxf_op1a/OP1ADVTrack.h>
#include <im/mxf_op1a/OP1AD10Track.h>
#include <im/mxf_op1a/OP1AAVCITrack.h>
#include <im/mxf_op1a/OP1AMPEG2LGTrack.h>
#include <im/mxf_op1a/OP1AUncTrack.h>
#include <im/mxf_op1a/OP1AVC3Track.h>
#include <im/mxf_op1a/OP1APCMTrack.h>
#include <im/essence_parser/DVEssenceParser.h>
#include <im/essence_parser/MPEG2EssenceParser.h>
#include <im/essence_parser/AVCIRawEssenceReader.h>
#include <im/essence_parser/VC3EssenceParser.h>
#include <im/essence_parser/RawEssenceReader.h>
#include <im/Utils.h>
#include <im/MXFUtils.h>
#include "../AppUtils.h"
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



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
    OP1AEssenceType essence_type;

    OP1ATrack *track;

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

    // sound
    mxfRational sampling_rate;
    uint32_t audio_quant_bits;
    bool locked;
    bool locked_set;
    int8_t audio_ref_level;
    bool audio_ref_level_set;
    int8_t dial_norm;
    bool dial_norm_set;

    // MPEG-2 Long GOP
    bool image_type_set;
    uint8_t mpeg2lg_signal_standard;
    uint8_t mpeg2lg_frame_layout;
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

    if (input->essence_type == OP1A_AVCI100_1080I ||
        input->essence_type == OP1A_AVCI100_1080P ||
        input->essence_type == OP1A_AVCI100_720P ||
        input->essence_type == OP1A_AVCI50_1080I ||
        input->essence_type == OP1A_AVCI50_1080P ||
        input->essence_type == OP1A_AVCI50_720P)
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

static string get_version_info()
{
    char buffer[256];
    sprintf(buffer, "raw2mxfop1a, %s v%s (%s %s)", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
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
    fprintf(stderr, "  -f <rate>               Frame rate: 25, 30 (30000/1001), 50 or 60 (60000/1001). Default parsed or 25\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --part <interval>       Essence partition interval in frames, or seconds with 's' suffix. Default single partition\n");
    fprintf(stderr, "  --out-start <offset>    Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    fprintf(stderr, "  --out-end <offset>      Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    fprintf(stderr, "  --seq-off <value>       Sound sample sequence offset. Default not set (0)\n");
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
    fprintf(stderr, "Inputs:\n");
    fprintf(stderr, "  --dv <name>             Raw DV video input file\n");
    fprintf(stderr, "  --iecdv25 <name>        Raw IEC DV25 video input file\n");
    fprintf(stderr, "  --dvbased25 <name>      Raw DV-Based DV25 video input file\n");
    fprintf(stderr, "  --dv50 <name>           Raw DV50 video input file\n");
    fprintf(stderr, "  --dv100_1080i <name>    Raw DV100 1080i video input file\n");
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
    fprintf(stderr, "  --mpeg2lg_422p_hl <name>  Raw MPEG-2 Long GOP 422P@HL (eg. XDCAM HD422) video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl <name>    Raw MPEG-2 Long GOP MP@HL (eg. XDCAM EX) video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_h14 <name>   Raw MPEG-2 Long GOP MP@H14 (eg. XDCAM HD / HDV) video input file\n");
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
    const char *output_filename = 0;
    vector<RawInput> inputs;
    RawInput input;
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    mxfRational frame_rate = FRAME_RATE_25;
    bool frame_rate_set = false;
    const char *clip_name = 0;
    const char *partition_interval_str = 0;
    int64_t partition_interval = 0;
    int64_t output_start_offset = 0;
    int64_t output_end_offset = 0;
    bool sequence_offset_set = false;
    uint8_t sequence_offset = 0;
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
    for (; cmdln_index < argc - 1; cmdln_index++)
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
            input.essence_type = OP1A_IEC_DV25;
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
            input.essence_type = OP1A_DVBASED_DV25;
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
            input.essence_type = OP1A_DV50;
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
            input.essence_type = OP1A_DV100_1080I;
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
            input.essence_type = OP1A_DV100_720P;
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
            input.essence_type = OP1A_D10_30;
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
            input.essence_type = OP1A_D10_40;
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
            input.essence_type = OP1A_D10_50;
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
            input.essence_type = OP1A_AVCI100_1080I;
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
            input.essence_type = OP1A_AVCI100_1080P;
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
            input.essence_type = OP1A_AVCI100_720P;
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
            input.essence_type = OP1A_AVCI50_1080I;
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
            input.essence_type = OP1A_AVCI50_1080P;
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
            input.essence_type = OP1A_AVCI50_720P;
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
            input.essence_type = OP1A_UNC_SD;
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
            input.essence_type = OP1A_UNC_HD_1080I;
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
            input.essence_type = OP1A_UNC_HD_1080P;
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
            input.essence_type = OP1A_UNC_HD_720P;
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
            input.essence_type = OP1A_MPEG2LG_422P_HL;
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
            input.essence_type = OP1A_MPEG2LG_MP_HL;
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
            input.essence_type = OP1A_MPEG2LG_MP_H14;
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
            input.essence_type = OP1A_VC3_1080P_1235;
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
            input.essence_type = OP1A_VC3_1080P_1237;
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
            input.essence_type = OP1A_VC3_1080P_1238;
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
            input.essence_type = OP1A_VC3_1080I_1241;
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
            input.essence_type = OP1A_VC3_1080I_1242;
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
            input.essence_type = OP1A_VC3_1080I_1243;
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
            input.essence_type = OP1A_VC3_720P_1250;
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
            input.essence_type = OP1A_VC3_720P_1251;
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
            input.essence_type = OP1A_VC3_720P_1252;
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
            input.essence_type = OP1A_VC3_1080P_1253;
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
            input.essence_type = OP1A_PCM;
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
                input->essence_type == OP1A_IEC_DV25 ||
                input->essence_type == OP1A_DVBASED_DV25 ||
                input->essence_type == OP1A_DV50 ||
                input->essence_type == OP1A_DV100_1080I ||
                input->essence_type == OP1A_DV100_720P)
            {
                DVEssenceParser *dv_parser = new DVEssenceParser();
                input->raw_reader->SetEssenceParser(dv_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to IEC DV-25 if no essence samples
                    if (input->essence_type_group == DV_ESSENCE_GROUP)
                        input->essence_type = OP1A_IEC_DV25;
                } else {
                    dv_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == DV_ESSENCE_GROUP) {
                        switch (dv_parser->GetEssenceType())
                        {
                            case DVEssenceParser::IEC_DV25:
                                input->essence_type = OP1A_IEC_DV25;
                                break;
                            case DVEssenceParser::DVBASED_DV25:
                                input->essence_type = OP1A_DVBASED_DV25;
                                break;
                            case DVEssenceParser::DV50:
                                input->essence_type = OP1A_DV50;
                                break;
                            case DVEssenceParser::DV100_1080I:
                                input->essence_type = OP1A_DV100_1080I;
                                break;
                            case DVEssenceParser::DV100_720P:
                                input->essence_type = OP1A_DV100_720P;
                                break;
                            case DVEssenceParser::UNKNOWN_DV:
                                log_error("Unknown DV essence type\n");
                                throw false;
                        }
                    }

                    input->raw_reader->SetFixedSampleSize(dv_parser->GetFrameSize());

                    if (!frame_rate_set) {
                        if (dv_parser->Is50Hz()) {
                            if (input->essence_type == OP1A_DV100_720P)
                                frame_rate = FRAME_RATE_50;
                            else
                                frame_rate = FRAME_RATE_25;
                        } else {
                            if (input->essence_type == OP1A_DV100_720P)
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
                    // default to OP1A_VC3_1080I_1242 if no essence samples
                    input->essence_type = OP1A_VC3_1080I_1242;
                } else {
                    vc3_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    switch (vc3_parser->GetCompressionId())
                    {
                        case 1235:
                            input->essence_type = OP1A_VC3_1080P_1235;
                            break;
                        case 1237:
                            input->essence_type = OP1A_VC3_1080P_1237;
                            break;
                        case 1238:
                            input->essence_type = OP1A_VC3_1080P_1238;
                            break;
                        case 1241:
                            input->essence_type = OP1A_VC3_1080I_1241;
                            break;
                        case 1242:
                            input->essence_type = OP1A_VC3_1080I_1242;
                            break;
                        case 1243:
                            input->essence_type = OP1A_VC3_1080I_1243;
                            break;
                        case 1250:
                            input->essence_type = OP1A_VC3_720P_1250;
                            break;
                        case 1251:
                            input->essence_type = OP1A_VC3_720P_1251;
                            break;
                        case 1252:
                            input->essence_type = OP1A_VC3_720P_1252;
                            break;
                        case 1253:
                            input->essence_type = OP1A_VC3_1080P_1253;
                            break;
                        default:
                            log_error("Unknown VC3 essence type\n");
                            throw false;
                    }
                }
            }
            else if (input->essence_type_group == D10_ESSENCE_GROUP ||
                     input->essence_type == OP1A_D10_30 ||
                     input->essence_type == OP1A_D10_40 ||
                     input->essence_type == OP1A_D10_50)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to D10 50Mbps if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = OP1A_D10_50;
                } else {
                    input->raw_reader->SetFixedSampleSize(input->raw_reader->GetSampleDataSize());

                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == D10_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetBitRate())
                        {
                            case 75000:
                                input->essence_type = OP1A_D10_30;
                                break;
                            case 100000:
                                input->essence_type = OP1A_D10_40;
                                break;
                            case 125000:
                                input->essence_type = OP1A_D10_50;
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
                     input->essence_type == OP1A_MPEG2LG_422P_HL ||
                     input->essence_type == OP1A_MPEG2LG_MP_HL ||
                     input->essence_type == OP1A_MPEG2LG_MP_H14)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to 422P@HL if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = OP1A_MPEG2LG_422P_HL;
                } else {
                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetProfileAndLevel())
                        {
                            case 0x82:
                                input->essence_type = OP1A_MPEG2LG_422P_HL;
                                break;
                            case 0x44:
                                input->essence_type = OP1A_MPEG2LG_MP_HL;
                                break;
                            case 0x46:
                                input->essence_type = OP1A_MPEG2LG_MP_H14;
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

        start_timecode.Init(get_rounded_tc_base(frame_rate), false);
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


        // check frame/sampling rates
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->essence_type == OP1A_PCM) {
                if (!OP1ATrack::IsSupported(input->essence_type, false, input->sampling_rate)) {
                    log_error("PCM sampling rate %d/%d not supported\n",
                              input->sampling_rate.numerator, input->sampling_rate.denominator);
                    throw false;
                }
            } else {
                bool is_mpeg2lg_720p = false;
                if (input->mpeg2lg_signal_standard == MXF_SIGNAL_STANDARD_SMPTE296M &&
                    (input->essence_type == OP1A_MPEG2LG_422P_HL ||
                        input->essence_type == OP1A_MPEG2LG_MP_HL))
                {
                    is_mpeg2lg_720p = true;
                }

                if (!OP1ATrack::IsSupported(input->essence_type, is_mpeg2lg_720p, frame_rate)) {
                    log_error("Frame rate %d/%d not supported for format '%s'\n",
                              frame_rate.numerator, frame_rate.denominator,
                              MXFDescriptorHelper::EssenceTypeToString(OP1ATrack::ConvertEssenceType(input->essence_type)).c_str());
                    throw false;
                }
            }
        }


        OP1AFile *mxf1a_file = OP1AFile::OpenNew(OP1A_DEFAULT_FLAVOUR, output_filename, frame_rate);


        // initialize data structures and inputs

        mxf1a_file->SetStartTimecode(start_timecode);
        if (clip_name)
            mxf1a_file->SetClipName(clip_name);
        mxf1a_file->SetPartitionInterval(partition_interval);
        mxf1a_file->SetOutputStartOffset(output_start_offset);
        mxf1a_file->SetOutputEndOffset(- output_end_offset);

        OP1ADVTrack *dv_track = 0;
        OP1AD10Track *d10_track = 0;
        OP1AAVCITrack *avci_track = 0;
        OP1AMPEG2LGTrack *mpeg2lg_track = 0;
        OP1AUncTrack *unc_track = 0;
        OP1AVC3Track *vc3_track = 0;
        OP1APCMTrack *pcm_track = 0;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

            // open raw file reader

            if (!open_raw_reader(input))
                throw false;


            // create track and configure

            input->track = mxf1a_file->CreateTrack(input->essence_type);

            if (input->track_number_set)
                input->track->SetMaterialTrackNumber(input->track_number);

            switch (input->essence_type)
            {
                case OP1A_IEC_DV25:
                case OP1A_DVBASED_DV25:
                case OP1A_DV50:
                    dv_track = dynamic_cast<OP1ADVTrack*>(input->track);
                    dv_track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        dv_track->SetAFD(input->afd);
                    break;
                case OP1A_DV100_1080I:
                case OP1A_DV100_720P:
                    dv_track = dynamic_cast<OP1ADVTrack*>(input->track);
                    dv_track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        dv_track->SetAFD(input->afd);
                    dv_track->SetComponentDepth(input->component_depth);
                    break;
                case OP1A_D10_30:
                case OP1A_D10_40:
                case OP1A_D10_50:
                    d10_track = dynamic_cast<OP1AD10Track*>(input->track);
                    d10_track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        d10_track->SetAFD(input->afd);
                    if (input->raw_reader->GetFixedSampleSize() != 0)
                        d10_track->SetSampleSize(input->raw_reader->GetFixedSampleSize());
                    break;
                case OP1A_AVCI100_1080I:
                case OP1A_AVCI100_1080P:
                case OP1A_AVCI100_720P:
                case OP1A_AVCI50_1080I:
                case OP1A_AVCI50_1080P:
                case OP1A_AVCI50_720P:
                    avci_track = dynamic_cast<OP1AAVCITrack*>(input->track);
                    if (input->afd)
                        avci_track->SetAFD(input->afd);
                    break;
                case OP1A_UNC_SD:
                case OP1A_UNC_HD_1080I:
                case OP1A_UNC_HD_1080P:
                case OP1A_UNC_HD_720P:
                    // TODO: fix HD aspect to 16:9?
                    unc_track = dynamic_cast<OP1AUncTrack*>(input->track);
                    unc_track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        unc_track->SetAFD(input->afd);
                    unc_track->SetComponentDepth(input->component_depth);
                    break;
                case OP1A_MPEG2LG_422P_HL:
                case OP1A_MPEG2LG_MP_HL:
                case OP1A_MPEG2LG_MP_H14:
                    mpeg2lg_track = dynamic_cast<OP1AMPEG2LGTrack*>(input->track);
                    if (input->afd)
                        mpeg2lg_track->SetAFD(input->afd);
                    mpeg2lg_track->SetSignalStandard(input->mpeg2lg_signal_standard);
                    mpeg2lg_track->SetFrameLayout(input->mpeg2lg_frame_layout);
                    break;
                case OP1A_VC3_1080P_1235:
                case OP1A_VC3_1080P_1237:
                case OP1A_VC3_1080P_1238:
                case OP1A_VC3_1080I_1241:
                case OP1A_VC3_1080I_1242:
                case OP1A_VC3_1080I_1243:
                case OP1A_VC3_720P_1250:
                case OP1A_VC3_720P_1251:
                case OP1A_VC3_720P_1252:
                case OP1A_VC3_1080P_1253:
                    vc3_track = dynamic_cast<OP1AVC3Track*>(input->track);
                    if (input->afd)
                        vc3_track->SetAFD(input->afd);
                    break;
                case OP1A_PCM:
                    pcm_track = dynamic_cast<OP1APCMTrack*>(input->track);
                    pcm_track->SetSamplingRate(input->sampling_rate);
                    pcm_track->SetQuantizationBits(input->audio_quant_bits);
                    if (input->locked_set)
                        pcm_track->SetLocked(input->locked);
                    if (input->audio_ref_level_set)
                        pcm_track->SetAudioRefLevel(input->audio_ref_level);
                    if (input->dial_norm_set)
                        pcm_track->SetDialNorm(input->dial_norm);
                    if (sequence_offset_set)
                        pcm_track->SetSequenceOffset(sequence_offset);
                    break;
                case OP1A_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }

            // set input sample info and create buffer
            switch (input->essence_type)
            {
                case OP1A_IEC_DV25:
                case OP1A_DVBASED_DV25:
                case OP1A_DV50:
                case OP1A_DV100_1080I:
                case OP1A_DV100_720P:
                case OP1A_D10_30:
                case OP1A_D10_40:
                case OP1A_D10_50:
                case OP1A_AVCI100_1080I:
                case OP1A_AVCI100_1080P:
                case OP1A_AVCI100_720P:
                case OP1A_AVCI50_1080I:
                case OP1A_AVCI50_1080P:
                case OP1A_AVCI50_720P:
                case OP1A_UNC_SD:
                case OP1A_UNC_HD_1080I:
                case OP1A_UNC_HD_1080P:
                case OP1A_UNC_HD_720P:
                case OP1A_VC3_1080P_1235:
                case OP1A_VC3_1080P_1237:
                case OP1A_VC3_1080P_1238:
                case OP1A_VC3_1080I_1241:
                case OP1A_VC3_1080I_1242:
                case OP1A_VC3_1080I_1243:
                case OP1A_VC3_720P_1250:
                case OP1A_VC3_720P_1251:
                case OP1A_VC3_720P_1252:
                case OP1A_VC3_1080P_1253:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader->GetFixedSampleSize() == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    break;
                case OP1A_MPEG2LG_422P_HL:
                case OP1A_MPEG2LG_MP_HL:
                case OP1A_MPEG2LG_MP_H14:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MPEG2EssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case OP1A_PCM:
                    {
                        vector<uint32_t> shifted_sample_sequence = pcm_track->GetShiftedSampleSequence();
                        IM_ASSERT(shifted_sample_sequence.size() < sizeof(input->sample_sequence) / sizeof(uint32_t));
                        memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                               shifted_sample_sequence.size() * sizeof(uint32_t));
                        input->sample_sequence_size = shifted_sample_sequence.size();
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    }
                    break;
                case OP1A_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }
        }

        mxf1a_file->PrepareWrite();

        // write samples
        while (duration < 0 || mxf1a_file->GetDuration() < duration) {
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

        mxf1a_file->CompleteWrite();

        log_info("Duration: %s (%"PRId64" frames @%d/%d fps)\n",
                 get_generic_duration_string(mxf1a_file->GetDuration(), mxf1a_file->GetFrameRate()).c_str(),
                 mxf1a_file->GetDuration(), mxf1a_file->GetFrameRate().numerator, mxf1a_file->GetFrameRate().denominator);


        // clean up
        for (i = 0; i < inputs.size(); i++)
            clear_input(&inputs[i]);
        delete mxf1a_file;
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

