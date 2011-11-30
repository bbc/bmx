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

#include <im/clip_writer/ClipWriter.h>
#include <im/as02/AS02PictureTrack.h>
#include <im/as02/AS02PCMTrack.h>
#include <im/avid_mxf/AvidPCMTrack.h>
#include <im/mxf_op1a/OP1APCMTrack.h>
#include <im/essence_parser/DVEssenceParser.h>
#include <im/essence_parser/MPEG2EssenceParser.h>
#include <im/essence_parser/AVCIRawEssenceReader.h>
#include <im/essence_parser/MJPEGEssenceParser.h>
#include <im/essence_parser/VC3EssenceParser.h>
#include <im/essence_parser/RawEssenceReader.h>
#include <im/URI.h>
#include <im/MXFUtils.h>
#include <im/XMLUtils.h>
#include "../AppUtils.h"
#include "../AS11Helper.h"
#include <im/IMException.h>
#include <im/Logging.h>

#include <mxf/mxf_avid.h>

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
    const char *position_str;
    AvidLocator locator;
} LocatorOption;

typedef struct
{
    ClipWriterType clip_type;
    EssenceTypeGroup essence_type_group;
    ClipWriterEssenceType essence_type;

    ClipWriterTrack *track;

    const char *filename;
    int64_t file_start_offset;
    int64_t file_max_length;

    int64_t output_start_offset;
    int64_t output_end_offset;

    RawEssenceReader *raw_reader;
    uint32_t sample_sequence[32];
    size_t sample_sequence_size;
    size_t sample_sequence_offset;

    // AS11
    uint32_t track_number;
    bool track_number_set;

    // picture
    Rational aspect_ratio;
    bool aspect_ratio_set;
    uint8_t afd;
    uint32_t component_depth;
    uint32_t input_height;

    // sound
    Rational sampling_rate;
    uint32_t audio_quant_bits;
    bool locked;
    bool locked_set;
    int8_t audio_ref_level;
    bool audio_ref_level_set;
    int8_t dial_norm;
    bool dial_norm_set;
} RawInput;

typedef struct
{
    const char *color_str;
    AvidRGBColor color;
} ColorMap;


static const ColorMap COLOR_MAP[] =
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

static const char DEFAULT_SHIM_NAME[]       = "Sample File";
static const char DEFAULT_SHIM_ID[]         = "http://bbc.co.uk/rd/as02/default-shim.txt";
static const char DEFAULT_SHIM_ANNOTATION[] = "Default AS-02 shim";


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

    if (input->essence_type == CW_AVCI100_1080I ||
        input->essence_type == CW_AVCI100_1080P ||
        input->essence_type == CW_AVCI100_720P ||
        input->essence_type == CW_AVCI50_1080I ||
        input->essence_type == CW_AVCI50_1080P ||
        input->essence_type == CW_AVCI50_720P)
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
    input->component_depth = 8;
    input->audio_quant_bits = 16;
}

static void clear_input(RawInput *input)
{
    delete input->raw_reader;
}

static string create_track_filename(const char *prefix, uint32_t track_number, ClipWriterEssenceType essence_type)
{
    bool is_picture = (essence_type != CW_PCM);

    char buffer[16];
    sprintf(buffer, "_%s%u.mxf", (is_picture ? "v" : "a"), track_number);

    string filename = prefix;
    return filename.append(buffer);
}

static bool parse_color(const char *color_str, AvidRGBColor *color)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(COLOR_MAP); i++) {
        if (strcmp(COLOR_MAP[i].color_str, color_str) == 0) {
            *color = COLOR_MAP[i].color;
            return true;
        }
    }

    return false;
}

static bool parse_mic_type(const char *mic_type_str, MICType *mic_type)
{
    if (strcmp(mic_type_str, "md5") == 0)
        *mic_type = MD5_MIC_TYPE;
    else if (strcmp(mic_type_str, "none") == 0)
        *mic_type = NONE_MIC_TYPE;
    else
        return false;

    return true;
}

static bool parse_clip_type(const char *clip_type_str, ClipWriterType *clip_type)
{
    if (strcmp(clip_type_str, "as02") == 0)
        *clip_type = CW_AS02_CLIP_TYPE;
    else if (strcmp(clip_type_str, "as11op1a") == 0)
        *clip_type = CW_AS11_OP1A_CLIP_TYPE;
    else if (strcmp(clip_type_str, "as11d10") == 0)
        *clip_type = CW_AS11_D10_CLIP_TYPE;
    else if (strcmp(clip_type_str, "op1a") == 0)
        *clip_type = CW_OP1A_CLIP_TYPE;
    else if (strcmp(clip_type_str, "avid") == 0)
        *clip_type = CW_AVID_CLIP_TYPE;
    else if (strcmp(clip_type_str, "d10") == 0)
        *clip_type = CW_D10_CLIP_TYPE;
    else
        return false;

    return true;
}

static string get_version_info()
{
    char buffer[256];
    sprintf(buffer, "raw2bmx, %s v%s (%s %s)", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
    return buffer;
}

static void usage(const char* cmd)
{
    fprintf(stderr, "%s\n", get_version_info().c_str());
    fprintf(stderr, "Usage: %s <<options>> [<<input options>> <input>]+\n", cmd);
    fprintf(stderr, "Options (* means option is required):\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -l <file>               Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, "  -t <type>               Clip type: as02, as11op1a, as11d10, op1a, avid, d10. Default is as02\n");
    fprintf(stderr, "* -o <name>               as02: <name> is a bundle name\n");
    fprintf(stderr, "                          as11op1a/as11d10/op1a/d10: <name> is a filename\n");
    fprintf(stderr, "                          avid: <name> is a filename prefix\n");
    fprintf(stderr, "  -f <rate>               Frame rate: 25, 30 (30000/1001), 50 or 60 (60000/1001). Default parsed or 25\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames. Default is minimum input duration\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02:\n");
    fprintf(stderr, "    --mic-type <type>       Media integrity check type: 'md5' or 'none'. Default 'md5'\n");
    fprintf(stderr, "    --mic-file              Calculate checksum for entire essence component file. Default is essence only\n");
    fprintf(stderr, "    --shim-name <name>      Set ShimName element value in shim.xml file to <name>. Default is '%s'\n", DEFAULT_SHIM_NAME);
    fprintf(stderr, "    --shim-id <id>          Set ShimID element value in shim.xml file to <id>. Default is '%s'\n", DEFAULT_SHIM_ID);
    fprintf(stderr, "    --shim-annot <str>      Set AnnotationText element value in shim.xml file to <str>. Default is '%s'\n", DEFAULT_SHIM_ANNOTATION);
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02/as11op1a/op1a:\n");
    fprintf(stderr, "    --part <interval>       Video essence partition interval in frames, or seconds with 's' suffix. Default single partition\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10:\n");
    fprintf(stderr, "    --dm <fwork> <name> <value>    Set descriptive framework property. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --dm-file <fwork> <name>       Parse and set descriptive framework properties from text file <name>. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --seg <name>                   Parse and set segmentation data from text file <name>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/op1a:\n");
    fprintf(stderr, "    --out-start <offset>    Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    fprintf(stderr, "    --out-end <offset>      Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10/op1a/d10:\n");
    fprintf(stderr, "    --seq-off <value>       Sound sample sequence offset. Default 0 for as11d10/d10 and not set (0) for as11op1a/op1a\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  avid:\n");
    fprintf(stderr, "    --project <name>        Set the Avid project name\n");
    fprintf(stderr, "    --tape <name>           Source tape name\n");
    fprintf(stderr, "    --comment <string>      Add 'Comments' user comment to the MaterialPackage\n");
    fprintf(stderr, "    --desc <string>         Add 'Descript' user comment to the MaterialPackage\n");
    fprintf(stderr, "    --tag <name> <value>    Add <name> user comment to the MaterialPackage. Option can be used multiple times\n");
    fprintf(stderr, "    --locator <position> <comment> <color>\n");
    fprintf(stderr, "                            Add locator at <position> with <comment> and <color>\n");
    fprintf(stderr, "                            <position> format is o?hh:mm:sscff, where the optional 'o' indicates it is an offset\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input Options (must precede the input to which it applies):\n");
    fprintf(stderr, "  -a <n:d>                Image aspect ratio. Either 4:3 or 16:9. Default parsed or 16:9\n");
    fprintf(stderr, "  --afd <value>           Active Format Descriptor code. Default not set\n");
    fprintf(stderr, "  -c <depth>              Component depth for uncompressed/DV100 video. Either 8 or 10. Default parsed or 8\n");
    fprintf(stderr, "  --height                Height of input uncompressed video data. Default is the production aperture height, except for PAL (592) and NTSC (496)\n");
    fprintf(stderr, "  -q <bps>                Audio quantization bits per sample. Either 16 or 24. Default 16\n");
    fprintf(stderr, "  --locked <bool>         Indicate whether the number of audio samples is locked to the video. Either true or false. Default not set\n");
    fprintf(stderr, "  --audio-ref <level>     Audio reference level, number of dBm for 0VU. Default not set\n");
    fprintf(stderr, "  --dial-norm <value>     Gain to be applied to normalize perceived loudness of the clip. Default not set\n");
    fprintf(stderr, "  --off <bytes>           Skip bytes at start of the next input/track's file\n");
    fprintf(stderr, "  --maxlen <bytes>        Maximum number of bytes to read from next input/track's file\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02:\n");
    fprintf(stderr, "    --trk-out-start <offset>   Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02/avid:\n");
    fprintf(stderr, "    --trk-out-end <offset>     Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10/op1a:\n");
    fprintf(stderr, "    --track-num <num>          Set the track number. Default starts at 1 or equals last track of same picture/sound type + 1\n");
    fprintf(stderr, "                               For as11d10/d10 the track number determines the channel index, which equals track number - 1\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
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
    fprintf(stderr, "  --mpeg2lg <name>                Raw MPEG-2 Long GOP video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl_1080i <name>  Raw MPEG-2 Long GOP 422P@HL 1080i video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl_1080p <name>  Raw MPEG-2 Long GOP 422P@HL 1080p video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl_720p <name>   Raw MPEG-2 Long GOP 422P@HL 720p video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_1080i <name>    Raw MPEG-2 Long GOP MP@HL 1080i video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_1080p <name>    Raw MPEG-2 Long GOP MP@HL 1080p video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_720p <name>     Raw MPEG-2 Long GOP MP@HL 720p video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_h14_1080i <name>   Raw MPEG-2 Long GOP MP@H14 1080i video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_h14_1080p <name>   Raw MPEG-2 Long GOP MP@H14 1080p video input file\n");
    fprintf(stderr, "  --mjpeg21 <name>        Raw Avid MJPEG 2:1 video input file\n");
    fprintf(stderr, "  --mjpeg31 <name>        Raw Avid MJPEG 3:1 video input file\n");
    fprintf(stderr, "  --mjpeg101 <name>       Raw Avid MJPEG 10:1 video input file\n");
    fprintf(stderr, "  --mjpeg201 <name>       Raw Avid MJPEG 20:1 video input file\n");
    fprintf(stderr, "  --mjpeg41m <name>       Raw Avid MJPEG 4:1m video input file\n");
    fprintf(stderr, "  --mjpeg101m <name>      Raw Avid MJPEG 10:1m video input file\n");
    fprintf(stderr, "  --mjpeg151s <name>      Raw Avid MJPEG 15:1s video input file\n");
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
    ClipWriterType clip_type = CW_AS02_CLIP_TYPE;
    const char *output_name = "";
    vector<RawInput> inputs;
    RawInput input;
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    Rational frame_rate = FRAME_RATE_25;
    bool frame_rate_set = false;
    int64_t duration = -1;
    int64_t output_start_offset = 0;
    int64_t output_end_offset = 0;
    const char *clip_name = 0;
    MICType mic_type = MD5_MIC_TYPE;
    MICScope ess_component_mic_scope = ESSENCE_ONLY_MIC_SCOPE;
    const char *partition_interval_str = 0;
    int64_t partition_interval = 0;
    const char *shim_name = 0;
    const char *shim_id = 0;
    const char *shim_annot = 0;
    const char *project_name = 0;
    const char *tape_name = 0;
    map<string, string> user_comments;
    vector<LocatorOption> locators;
    vector<FrameworkProperty> framework_properties;
    const char *segmentation_filename = 0;
    vector<AS11TCSegment> segments;
    bool filler_complete_segments = false;
    bool sequence_offset_set = false;
    uint8_t sequence_offset = 0;
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
        else if (strcmp(argv[cmdln_index], "-o") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            output_name = argv[cmdln_index + 1];
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
        else if (strcmp(argv[cmdln_index], "--mic-type") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_mic_type(argv[cmdln_index + 1], &mic_type))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mic-file") == 0)
        {
            ess_component_mic_scope = ENTIRE_FILE_MIC_SCOPE;
        }
        else if (strcmp(argv[cmdln_index], "--shim-name") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            shim_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--shim-id") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            shim_id = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--shim-annot") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            shim_annot = argv[cmdln_index + 1];
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
        else if (strcmp(argv[cmdln_index], "--height") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &value) != 1 || value == 0) {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.input_height = value;
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
        else if (strcmp(argv[cmdln_index], "--trk-out-start") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &input.output_start_offset) != 1 ||
                input.output_start_offset < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--trk-out-end") == 0)
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
            input.essence_type = CW_IEC_DV25;
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
            input.essence_type = CW_DVBASED_DV25;
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
            input.essence_type = CW_DV50;
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
            input.essence_type = CW_DV100_1080I;
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
            input.essence_type = CW_DV100_1080P;
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
            input.essence_type = CW_DV100_720P;
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
            input.essence_type = CW_D10_30;
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
            input.essence_type = CW_D10_40;
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
            input.essence_type = CW_D10_50;
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
            input.essence_type = CW_AVCI100_1080I;
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
            input.essence_type = CW_AVCI100_1080P;
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
            input.essence_type = CW_AVCI100_720P;
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
            input.essence_type = CW_AVCI50_1080I;
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
            input.essence_type = CW_AVCI50_1080P;
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
            input.essence_type = CW_AVCI50_720P;
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
            input.essence_type = CW_UNC_SD;
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
            input.essence_type = CW_UNC_HD_1080I;
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
            input.essence_type = CW_UNC_HD_1080P;
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
            input.essence_type = CW_UNC_HD_720P;
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
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_422p_hl_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_422P_HL_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_422p_hl_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_422P_HL_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_422p_hl_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_422P_HL_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_MP_HL_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_MP_HL_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_MP_HL_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_h14_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_MP_H14_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_h14_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = CW_MPEG2LG_MP_H14_1080P;
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
            input.essence_type = CW_MJPEG_2_1;
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
            input.essence_type = CW_MJPEG_3_1;
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
            input.essence_type = CW_MJPEG_10_1;
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
            input.essence_type = CW_MJPEG_20_1;
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
            input.essence_type = CW_MJPEG_4_1M;
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
            input.essence_type = CW_MJPEG_10_1M;
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
            input.essence_type = CW_MJPEG_15_1S;
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
            input.essence_type = CW_VC3_1080P_1235;
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
            input.essence_type = CW_VC3_1080P_1237;
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
            input.essence_type = CW_VC3_1080P_1238;
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
            input.essence_type = CW_VC3_1080I_1241;
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
            input.essence_type = CW_VC3_1080I_1242;
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
            input.essence_type = CW_VC3_1080I_1243;
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
            input.essence_type = CW_VC3_720P_1250;
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
            input.essence_type = CW_VC3_720P_1251;
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
            input.essence_type = CW_VC3_720P_1252;
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
            input.essence_type = CW_VC3_1080P_1253;
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
            input.essence_type = CW_PCM;
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

    if (!output_name) {
        usage(argv[0]);
        fprintf(stderr, "No output name given\n");
        return 1;
    }

    if (inputs.empty()) {
        usage(argv[0]);
        fprintf(stderr, "No raw inputs given\n");
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
        // TODO: move to AS02 classes and remove XMLUtils include
        XMLInitializer xml_initializer;


        // update Avid uncompressed essence types if component depth equals 10
        if (clip_type == CW_AVID_CLIP_TYPE) {
            size_t i;
            for (i = 0; i < inputs.size(); i++) {
                if (inputs[i].component_depth == 10) {
                    switch (inputs[i].essence_type)
                    {
                        case CW_UNC_SD:
                            inputs[i].essence_type = CW_AVID_10BIT_UNC_SD;
                            break;
                        case CW_UNC_HD_1080I:
                            inputs[i].essence_type = CW_AVID_10BIT_UNC_HD_1080I;
                            break;
                        case CW_UNC_HD_1080P:
                            inputs[i].essence_type = CW_AVID_10BIT_UNC_HD_1080P;
                            break;
                        case CW_UNC_HD_720P:
                            inputs[i].essence_type = CW_AVID_10BIT_UNC_HD_720P;
                            break;
                        default:
                            break;
                    }
                }
            }
        }


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
                input->essence_type == CW_IEC_DV25 ||
                input->essence_type == CW_DVBASED_DV25 ||
                input->essence_type == CW_DV50 ||
                input->essence_type == CW_DV100_1080I ||
                input->essence_type == CW_DV100_1080P ||
                input->essence_type == CW_DV100_720P)
            {
                DVEssenceParser *dv_parser = new DVEssenceParser();
                input->raw_reader->SetEssenceParser(dv_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to IEC DV-25 if no essence samples
                    if (input->essence_type_group == DV_ESSENCE_GROUP)
                        input->essence_type = CW_IEC_DV25;
                } else {
                    dv_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == DV_ESSENCE_GROUP) {
                        switch (dv_parser->GetEssenceType())
                        {
                            case DVEssenceParser::IEC_DV25:
                                input->essence_type = CW_IEC_DV25;
                                break;
                            case DVEssenceParser::DVBASED_DV25:
                                input->essence_type = CW_DVBASED_DV25;
                                break;
                            case DVEssenceParser::DV50:
                                input->essence_type = CW_DV50;
                                break;
                            case DVEssenceParser::DV100_1080I:
                                input->essence_type = CW_DV100_1080I;
                                break;
                            case DVEssenceParser::DV100_720P:
                                input->essence_type = CW_DV100_720P;
                                break;
                            case DVEssenceParser::UNKNOWN_DV:
                                log_error("Unknown DV essence type\n");
                                throw false;
                        }
                    }

                    input->raw_reader->SetFixedSampleSize(dv_parser->GetFrameSize());

                    if (!frame_rate_set) {
                        if (dv_parser->Is50Hz()) {
                            if (input->essence_type == CW_DV100_720P)
                                frame_rate = FRAME_RATE_50;
                            else
                                frame_rate = FRAME_RATE_25;
                        } else {
                            if (input->essence_type == CW_DV100_720P)
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
                    // default to CW_VC3_1080I_1242 if no essence samples
                    input->essence_type = CW_VC3_1080I_1242;
                } else {
                    vc3_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    switch (vc3_parser->GetCompressionId())
                    {
                        case 1235:
                            input->essence_type = CW_VC3_1080P_1235;
                            break;
                        case 1237:
                            input->essence_type = CW_VC3_1080P_1237;
                            break;
                        case 1238:
                            input->essence_type = CW_VC3_1080P_1238;
                            break;
                        case 1241:
                            input->essence_type = CW_VC3_1080I_1241;
                            break;
                        case 1242:
                            input->essence_type = CW_VC3_1080I_1242;
                            break;
                        case 1243:
                            input->essence_type = CW_VC3_1080I_1243;
                            break;
                        case 1250:
                            input->essence_type = CW_VC3_720P_1250;
                            break;
                        case 1251:
                            input->essence_type = CW_VC3_720P_1251;
                            break;
                        case 1252:
                            input->essence_type = CW_VC3_720P_1252;
                            break;
                        case 1253:
                            input->essence_type = CW_VC3_1080P_1253;
                            break;
                        default:
                            log_error("Unknown VC3 essence type\n");
                            throw false;
                    }
                }
            }
            else if (input->essence_type_group == D10_ESSENCE_GROUP ||
                     input->essence_type == CW_D10_30 ||
                     input->essence_type == CW_D10_40 ||
                     input->essence_type == CW_D10_50)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to D10 50Mbps if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = CW_D10_50;
                } else {
                    input->raw_reader->SetFixedSampleSize(input->raw_reader->GetSampleDataSize());

                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == D10_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetBitRate())
                        {
                            case 75000:
                                input->essence_type = CW_D10_30;
                                break;
                            case 100000:
                                input->essence_type = CW_D10_40;
                                break;
                            case 125000:
                                input->essence_type = CW_D10_50;
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
                     input->essence_type == CW_MPEG2LG_422P_HL_1080I ||
                     input->essence_type == CW_MPEG2LG_422P_HL_1080P ||
                     input->essence_type == CW_MPEG2LG_422P_HL_720P ||
                     input->essence_type == CW_MPEG2LG_MP_HL_1080I ||
                     input->essence_type == CW_MPEG2LG_MP_HL_1080P ||
                     input->essence_type == CW_MPEG2LG_MP_HL_720P ||
                     input->essence_type == CW_MPEG2LG_MP_H14_1080I ||
                     input->essence_type == CW_MPEG2LG_MP_H14_1080P)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to 422P@HL 1080i if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = CW_MPEG2LG_422P_HL_1080I;
                } else {
                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP) {
                        if (mpeg2_parser->IsProgressive()) {
                            if (mpeg2_parser->GetVerticalSize() == 1080) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x82:
                                        input->essence_type = CW_MPEG2LG_422P_HL_1080P;
                                        break;
                                    case 0x44:
                                        input->essence_type = CW_MPEG2LG_MP_HL_1080P;
                                        break;
                                    case 0x46:
                                        input->essence_type = CW_MPEG2LG_MP_H14_1080P;
                                        break;
                                    default:
                                        log_error("Unexpected MPEG-2 Long GOP profile and level %u\n",
                                                  mpeg2_parser->GetProfileAndLevel());
                                        throw false;
                                }
                            } else if (mpeg2_parser->GetVerticalSize() == 720) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x82:
                                        input->essence_type = CW_MPEG2LG_422P_HL_720P;
                                        break;
                                    case 0x44:
                                        input->essence_type = CW_MPEG2LG_MP_HL_720P;
                                        break;
                                    default:
                                        log_error("Unexpected MPEG-2 Long GOP profile and level %u\n",
                                                  mpeg2_parser->GetProfileAndLevel());
                                        throw false;
                                }
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
                            switch (mpeg2_parser->GetProfileAndLevel())
                            {
                                case 0x82:
                                    input->essence_type = CW_MPEG2LG_422P_HL_1080I;
                                    break;
                                case 0x44:
                                    input->essence_type = CW_MPEG2LG_MP_HL_1080I;
                                    break;
                                case 0x46:
                                    input->essence_type = CW_MPEG2LG_MP_H14_1080I;
                                    break;
                                default:
                                    log_error("Unknown MPEG-2 Long GOP profile and level %u\n",
                                              mpeg2_parser->GetProfileAndLevel());
                                    throw false;
                            }
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

        for (i = 0; i < locators.size(); i++) {
            if (!parse_position(locators[i].position_str, start_timecode, frame_rate, &locators[i].locator.position)) {
                usage(argv[0]);
                log_error("Invalid value '%s' for option '--locator'\n", locators[i].position_str);
                throw false;
            }
        }


        // check support for essence type and frame/sampling rates
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->essence_type == CW_PCM) {
                if (!ClipWriterTrack::IsSupported(clip_type, input->essence_type, false, input->sampling_rate)) {
                    log_error("PCM sampling rate %d/%d not supported\n",
                              input->sampling_rate.numerator, input->sampling_rate.denominator);
                    throw false;
                }
            } else {
                bool is_mpeg2lg_720p = false;
                if (input->essence_type == CW_MPEG2LG_422P_HL_720P ||
                    input->essence_type == CW_MPEG2LG_MP_HL_720P)
                {
                    is_mpeg2lg_720p = true;
                }

                if (!ClipWriterTrack::IsSupported(clip_type, input->essence_type, is_mpeg2lg_720p, frame_rate)) {
                    log_error("Essence type '%s' @%d/%d fps not supported for clip type '%s'\n",
                              ClipWriterTrack::EssenceTypeToString(input->essence_type).c_str(),
                              frame_rate.numerator, frame_rate.denominator,
                              ClipWriter::ClipWriterTypeToString(input->clip_type).c_str());
                    throw false;
                }
            }
        }


        // create clip
        ClipWriter *clip = 0;
        switch (clip_type)
        {
            case CW_AS02_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS02Clip(output_name, true, frame_rate);
                break;
            case CW_AS11_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS11OP1AClip(output_name, frame_rate);
                break;
            case CW_AS11_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS11D10Clip(output_name, frame_rate);
                break;
            case CW_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewOP1AClip(OP1A_DEFAULT_FLAVOUR, output_name, frame_rate);
                break;
            case CW_AVID_CLIP_TYPE:
                clip = ClipWriter::OpenNewAvidClip(frame_rate);
                break;
            case CW_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewD10Clip(output_name, frame_rate);
                break;
            case CW_UNKNOWN_CLIP_TYPE:
                IM_ASSERT(false);
                break;
        }


        // initialize clip properties

        SourcePackage *tape_package = 0;
        vector<pair<mxfUMID, uint32_t> > tape_package_picture_refs;
        vector<pair<mxfUMID, uint32_t> > tape_package_sound_refs;

        clip->SetStartTimecode(start_timecode);
        if (clip_name)
            clip->SetClipName(clip_name);

        if (clip_type == CW_AS02_CLIP_TYPE) {
            AS02Clip *as02_clip = clip->GetAS02Clip();
            AS02Bundle *bundle = as02_clip->GetBundle();

            bundle->GetManifest()->SetDefaultMICType(mic_type);
            bundle->GetManifest()->SetDefaultMICScope(ENTIRE_FILE_MIC_SCOPE);

            if (shim_name)
                bundle->GetShim()->SetName(shim_name);
            else
                bundle->GetShim()->SetName(DEFAULT_SHIM_NAME);
            if (shim_id)
                bundle->GetShim()->SetId(shim_id);
            else
                bundle->GetShim()->SetId(DEFAULT_SHIM_ID);
            if (shim_annot)
                bundle->GetShim()->AppendAnnotation(shim_annot);
            else if (!shim_id)
                bundle->GetShim()->AppendAnnotation(DEFAULT_SHIM_ANNOTATION);
        } else if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE) {
            AS11Clip *as11_clip = clip->GetAS11Clip();

            as11_clip->SetPartitionInterval(partition_interval);
            as11_clip->SetOutputStartOffset(output_start_offset);
            as11_clip->SetOutputEndOffset(- output_end_offset);

            if (!clip_name) {
                for (i = 0; i < framework_properties.size(); i++) {
                    if (framework_properties[i].type == AS11_CORE_FRAMEWORK_TYPE &&
                        framework_properties[i].name == "ProgrammeTitle")
                    {
                        as11_clip->SetClipName(framework_properties[i].value);
                        break;
                    }
                }
            }
        } else if (clip_type == CW_OP1A_CLIP_TYPE) {
            OP1AFile *op1a_clip = clip->GetOP1AClip();

            op1a_clip->SetPartitionInterval(partition_interval);
            op1a_clip->SetOutputStartOffset(output_start_offset);
            op1a_clip->SetOutputEndOffset(- output_end_offset);
        } else if (clip_type == CW_AVID_CLIP_TYPE) {
            AvidClip *avid_clip = clip->GetAvidClip();

            if (!clip_name)
                avid_clip->SetClipName(output_name);

            if (project_name)
                avid_clip->SetProjectName(project_name);

            for (i = 0; i < locators.size(); i++)
                avid_clip->AddLocator(locators[i].locator);

            map<string, string>::const_iterator iter;
            for (iter = user_comments.begin(); iter != user_comments.end(); iter++)
                avid_clip->SetUserComment(iter->first, iter->second);

            if (tape_name) {
                uint32_t num_picture_tracks = 0;
                uint32_t num_sound_tracks = 0;
                for (i = 0; i < inputs.size(); i++) {
                    if (inputs[i].essence_type == CW_PCM)
                        num_sound_tracks++;
                    else
                        num_picture_tracks++;
                }
                tape_package = avid_clip->CreateDefaultTapeSource(tape_name, num_picture_tracks, num_sound_tracks);

                tape_package_picture_refs = avid_clip->GetPictureSourceReferences(tape_package);
                IM_ASSERT(tape_package_picture_refs.size() == num_picture_tracks);
                tape_package_sound_refs = avid_clip->GetSoundSourceReferences(tape_package);
                IM_ASSERT(tape_package_sound_refs.size() == num_sound_tracks);
            }
        }


        // open raw inputs, create and initialize track properties

        uint32_t picture_track_count = 0;
        uint32_t sound_track_count = 0;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

            // open raw file reader

            if (!open_raw_reader(input))
                throw false;

            bool is_picture = (input->essence_type != CW_PCM);


            // create track

            if (clip_type == CW_AVID_CLIP_TYPE) {
                string track_name = create_track_filename(output_name,
                                                          is_picture ? picture_track_count + 1 : sound_track_count + 1,
                                                          input->essence_type);
                input->track = clip->CreateTrack(input->essence_type, track_name.c_str());
            } else {
                input->track = clip->CreateTrack(input->essence_type);
            }


            // initialize track

            if (clip_type == CW_AS02_CLIP_TYPE) {
                AS02Track *as02_track = input->track->GetAS02Track();
                as02_track->SetMICType(mic_type);
                as02_track->SetMICScope(ess_component_mic_scope);
                as02_track->SetOutputStartOffset(input->output_start_offset);
                as02_track->SetOutputEndOffset(- input->output_end_offset);

                AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(input->track->GetAS02Track());
                if (as02_pict_track)
                    as02_pict_track->SetPartitionInterval(partition_interval);
            } else if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE) {
                AS11Track *as11_track = input->track->GetAS11Track();
                if (input->track_number_set)
                    as11_track->SetTrackNumber(input->track_number);
            } else if (clip_type == CW_OP1A_CLIP_TYPE) {
                OP1ATrack *op1a_track = input->track->GetOP1ATrack();
                if (input->track_number_set)
                    op1a_track->SetMaterialTrackNumber(input->track_number);
            } else if (clip_type == CW_AVID_CLIP_TYPE) {
                AvidTrack *avid_track = input->track->GetAvidTrack();
                AvidClip *avid_clip = clip->GetAvidClip();

                avid_track->SetOutputEndOffset(- input->output_end_offset);

                if (tape_package) {
                    if (is_picture) {
                        avid_track->SetSourceRef(tape_package_picture_refs[picture_track_count].first,
                                                 tape_package_picture_refs[picture_track_count].second);
                    } else {
                        avid_track->SetSourceRef(tape_package_sound_refs[sound_track_count].first,
                                                 tape_package_sound_refs[sound_track_count].second);
                    }
                } else {
                    vector<pair<mxfUMID, uint32_t> > source_refs;
                    string name = strip_path(input->filename);
                    URI uri;
                    uri.ParseFilename(input->filename);
                    if (is_picture) {
                        SourcePackage *import_package = avid_clip->CreateDefaultImportSource(uri.ToString(), name, 1, 0);
                        source_refs = avid_clip->GetPictureSourceReferences(import_package);
                    } else {
                        SourcePackage *import_package = avid_clip->CreateDefaultImportSource(uri.ToString(), name, 0, 1);
                        source_refs = avid_clip->GetSoundSourceReferences(import_package);
                    }
                    IM_ASSERT(source_refs.size() == 1);
                    avid_track->SetSourceRef(source_refs[0].first, source_refs[0].second);
                }
            } else if (clip_type == CW_D10_CLIP_TYPE) {
                D10PCMTrack *d10_pcm_track = dynamic_cast<D10PCMTrack*>(input->track->GetD10Track());
                if (d10_pcm_track && input->track_number_set)
                    d10_pcm_track->SetOutputChannelIndex(input->track_number_set);
            }

            switch (input->essence_type)
            {
                case CW_IEC_DV25:
                case CW_DVBASED_DV25:
                case CW_DV50:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case CW_DV100_1080I:
                case CW_DV100_1080P:
                case CW_DV100_720P:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetComponentDepth(input->component_depth);
                    break;
                case CW_D10_30:
                case CW_D10_40:
                case CW_D10_50:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    if (input->raw_reader->GetFixedSampleSize() != 0)
                        input->track->SetSampleSize(input->raw_reader->GetFixedSampleSize());
                    break;
                case CW_AVCI100_1080I:
                case CW_AVCI100_1080P:
                case CW_AVCI100_720P:
                case CW_AVCI50_1080I:
                case CW_AVCI50_1080P:
                case CW_AVCI50_720P:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetAVCIMode(AVCI_ALL_FRAME_HEADER_MODE);
                    break;
                case CW_UNC_SD:
                case CW_UNC_HD_1080I:
                case CW_UNC_HD_1080P:
                case CW_UNC_HD_720P:
                case CW_AVID_10BIT_UNC_SD:
                case CW_AVID_10BIT_UNC_HD_1080I:
                case CW_AVID_10BIT_UNC_HD_1080P:
                case CW_AVID_10BIT_UNC_HD_720P:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetComponentDepth(input->component_depth);
                    if (input->input_height > 0)
                        input->track->SetInputHeight(input->input_height);
                    break;
                case CW_MPEG2LG_422P_HL_1080I:
                case CW_MPEG2LG_422P_HL_1080P:
                case CW_MPEG2LG_422P_HL_720P:
                case CW_MPEG2LG_MP_HL_1080I:
                case CW_MPEG2LG_MP_HL_1080P:
                case CW_MPEG2LG_MP_HL_720P:
                case CW_MPEG2LG_MP_H14_1080I:
                case CW_MPEG2LG_MP_H14_1080P:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case CW_MJPEG_2_1:
                case CW_MJPEG_3_1:
                case CW_MJPEG_10_1:
                case CW_MJPEG_20_1:
                case CW_MJPEG_4_1M:
                case CW_MJPEG_10_1M:
                case CW_MJPEG_15_1S:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetAspectRatio(input->aspect_ratio);
                    break;
                case CW_VC3_1080P_1235:
                case CW_VC3_1080P_1237:
                case CW_VC3_1080P_1238:
                case CW_VC3_1080I_1241:
                case CW_VC3_1080I_1242:
                case CW_VC3_1080I_1243:
                case CW_VC3_720P_1250:
                case CW_VC3_720P_1251:
                case CW_VC3_720P_1252:
                case CW_VC3_1080P_1253:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case CW_PCM:
                    input->track->SetSamplingRate(input->sampling_rate);
                    input->track->SetQuantizationBits(input->audio_quant_bits);
                    if (input->locked_set)
                        input->track->SetLocked(input->locked);
                    if (input->audio_ref_level_set)
                        input->track->SetAudioRefLevel(input->audio_ref_level);
                    if (input->dial_norm_set)
                        input->track->SetDialNorm(input->dial_norm);
                    // force D10 sequence offset to 0 and default to 0 for other clip types
                    if (clip_type == CW_D10_CLIP_TYPE || sequence_offset_set)
                        input->track->SetSequenceOffset(sequence_offset);
                    break;
                case CW_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }


            // initialize raw input

            switch (input->essence_type)
            {
                case CW_IEC_DV25:
                case CW_DVBASED_DV25:
                case CW_DV50:
                case CW_DV100_1080I:
                case CW_DV100_1080P:
                case CW_DV100_720P:
                case CW_D10_30:
                case CW_D10_40:
                case CW_D10_50:
                case CW_AVCI100_1080I:
                case CW_AVCI100_1080P:
                case CW_AVCI100_720P:
                case CW_AVCI50_1080I:
                case CW_AVCI50_1080P:
                case CW_AVCI50_720P:
                case CW_VC3_1080P_1235:
                case CW_VC3_1080P_1237:
                case CW_VC3_1080P_1238:
                case CW_VC3_1080I_1241:
                case CW_VC3_1080I_1242:
                case CW_VC3_1080I_1243:
                case CW_VC3_720P_1250:
                case CW_VC3_720P_1251:
                case CW_VC3_720P_1252:
                case CW_VC3_1080P_1253:
                case CW_UNC_SD:
                case CW_UNC_HD_1080I:
                case CW_UNC_HD_1080P:
                case CW_UNC_HD_720P:
                case CW_AVID_10BIT_UNC_SD:
                case CW_AVID_10BIT_UNC_HD_1080I:
                case CW_AVID_10BIT_UNC_HD_1080P:
                case CW_AVID_10BIT_UNC_HD_720P:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader->GetFixedSampleSize() == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetInputSampleSize());
                    break;
                case CW_MPEG2LG_422P_HL_1080I:
                case CW_MPEG2LG_422P_HL_1080P:
                case CW_MPEG2LG_422P_HL_720P:
                case CW_MPEG2LG_MP_HL_1080I:
                case CW_MPEG2LG_MP_HL_1080P:
                case CW_MPEG2LG_MP_HL_720P:
                case CW_MPEG2LG_MP_H14_1080I:
                case CW_MPEG2LG_MP_H14_1080P:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MPEG2EssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case CW_MJPEG_2_1:
                case CW_MJPEG_3_1:
                case CW_MJPEG_10_1:
                case CW_MJPEG_20_1:
                case CW_MJPEG_4_1M:
                case CW_MJPEG_10_1M:
                case CW_MJPEG_15_1S:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MJPEGEssenceParser(input->track->IsSingleField()));
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case CW_PCM:
                {
                    vector<uint32_t> shifted_sample_sequence = input->track->GetShiftedSampleSequence();
                    IM_ASSERT(shifted_sample_sequence.size() < sizeof(input->sample_sequence) / sizeof(uint32_t));
                    memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                           shifted_sample_sequence.size() * sizeof(uint32_t));
                    input->sample_sequence_size = shifted_sample_sequence.size();
                    input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    break;
                }
                case CW_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }

            if (is_picture)
                picture_track_count++;
            else
                sound_track_count++;
        }


        // add AS-11 descriptive metadata

        bool have_dpp_properties = false;
        bool have_dpp_total_number_of_parts = false;
        bool have_dpp_total_programme_duration = false;
        FrameworkHelper *dpp_framework_h = 0;

        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE) {
            AS11Clip *as11_clip = clip->GetAS11Clip();

            as11_clip->PrepareHeaderMetadata();
            AS11DMS::RegisterExtensions(as11_clip->GetDataModel());
            UKDPPDMS::RegisterExtensions(as11_clip->GetDataModel());

            FrameworkHelper as11_framework_h(as11_clip->GetDataModel(), new AS11CoreFramework(as11_clip->GetHeaderMetadata()));
            dpp_framework_h = new FrameworkHelper(as11_clip->GetDataModel(), new UKDPPFramework(as11_clip->GetHeaderMetadata()));

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
                        if (!dpp_framework_h->SetProperty(framework_properties[i].name, framework_properties[i].value)) {
                            log_warn("Failed to set UKDPPFramework property '%s' to '%s'\n",
                                     framework_properties[i].name.c_str(), framework_properties[i].value.c_str());
                        } else {
                            have_dpp_properties = true;
                        }
                        break;
                }
            }

            as11_clip->InsertAS11CoreFramework(dynamic_cast<AS11CoreFramework*>(as11_framework_h.GetFramework()));
            IM_CHECK_M(as11_framework_h.GetFramework()->validate(true), ("AS11 Framework validation failed"));

            if (!segments.empty())
                as11_clip->InsertTCSegmentation(segments);

            if (have_dpp_properties) {
                as11_clip->InsertUKDPPFramework(dynamic_cast<UKDPPFramework*>(dpp_framework_h->GetFramework()));

                // make sure UKDPPTotalNumberOfParts and UKDPPTotalProgrammeDuration are set (default 0) for validation
                UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(dpp_framework_h->GetFramework());
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
                IM_CHECK(mxf_remove_set(as11_clip->GetHeaderMetadata()->getCHeaderMetadata(),
                                        dpp_framework_h->GetFramework()->getCMetadataSet()));
            }
        }


        // create clip file(s) and write samples

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


        // complete AS-11 descriptive metadata

        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE) {
            AS11Clip *as11_clip = clip->GetAS11Clip();

            if (!segments.empty())
                as11_clip->CompleteSegmentation(filler_complete_segments);

            if (have_dpp_properties) {
                // calculate or check total number of parts and programme duration
                UKDPPFramework *dpp_framework = dynamic_cast<UKDPPFramework*>(dpp_framework_h->GetFramework());
                if (have_dpp_total_number_of_parts) {
                    if (as11_clip->GetTotalSegments() != dpp_framework->GetTotalNumberOfParts()) {
                        log_warn("UKDPPTotalNumberOfParts value %u does not equal actual total part count %u\n",
                                 dpp_framework->GetTotalNumberOfParts(), as11_clip->GetTotalSegments());
                    }
                } else {
                    dpp_framework->SetTotalNumberOfParts(as11_clip->GetTotalSegments());
                }
                if (have_dpp_total_programme_duration) {
                    if (as11_clip->GetTotalSegmentDuration() != dpp_framework->GetTotalProgrammeDuration()) {
                        log_warn("UKDPPTotalProgrammeDuration value %u does not equal actual total part duration %u\n",
                                 dpp_framework->GetTotalProgrammeDuration(), as11_clip->GetTotalSegmentDuration());
                    }
                } else {
                    dpp_framework->SetTotalProgrammeDuration(as11_clip->GetTotalSegmentDuration());
                }
            }
        }


        // complete writing

        clip->CompleteWrite();

        log_info("Duration: %s (%"PRId64" frames @%d/%d fps)\n",
                 get_generic_duration_string(clip->GetDuration(), clip->GetFrameRate()).c_str(),
                 clip->GetDuration(), clip->GetFrameRate().numerator, clip->GetFrameRate().denominator);


        // clean up
        for (i = 0; i < inputs.size(); i++)
            clear_input(&inputs[i]);
        delete clip;
        delete dpp_framework_h;
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

