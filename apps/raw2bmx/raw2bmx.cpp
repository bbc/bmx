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

#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/as02/AS02PCMTrack.h>
#include <bmx/avid_mxf/AvidPCMTrack.h>
#include <bmx/mxf_op1a/OP1APCMTrack.h>
#include <bmx/essence_parser/DVEssenceParser.h>
#include <bmx/essence_parser/MPEG2EssenceParser.h>
#include <bmx/essence_parser/AVCIRawEssenceReader.h>
#include <bmx/essence_parser/MJPEGEssenceParser.h>
#include <bmx/essence_parser/VC3EssenceParser.h>
#include <bmx/essence_parser/RawEssenceReader.h>
#include <bmx/wave/WaveFileIO.h>
#include <bmx/wave/WaveReader.h>
#include <bmx/URI.h>
#include <bmx/MXFUtils.h>
#include <bmx/Version.h>
#include "../AppUtils.h"
#include "../AS11Helper.h"
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
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
    EssenceType essence_type;
    bool is_wave;

    ClipWriterTrack *track;

    const char *filename;
    int64_t file_start_offset;
    int64_t file_max_length;

    int64_t output_start_offset;
    int64_t output_end_offset;

    RawEssenceReader *raw_reader;
    WaveReader *wave_reader;
    uint32_t wave_track_index;

    uint32_t sample_sequence[32];
    size_t sample_sequence_size;
    size_t sample_sequence_offset;

    uint32_t track_number;
    bool track_number_set;

    // picture
    Rational aspect_ratio;
    bool aspect_ratio_set;
    uint8_t afd;
    uint32_t component_depth;
    uint32_t input_height;
    bool have_avci_header;

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


static const char DEFAULT_SHIM_NAME[]       = "Sample File";
static const char DEFAULT_SHIM_ID[]         = "http://bbc.co.uk/rd/as02/default-shim.txt";
static const char DEFAULT_SHIM_ANNOTATION[] = "Default AS-02 shim";

static const char DEFAULT_BEXT_ORIGINATOR[] = "bmx";

static const Rational DEFAULT_SAMPLING_RATE = SAMPLING_RATE_48K;


extern bool BMX_REGRESSION_TEST;



static uint32_t read_samples(RawInput *input, uint32_t max_samples_per_read)
{
    if (max_samples_per_read == 1) {
        uint32_t num_frame_samples = input->sample_sequence[input->sample_sequence_offset];
        input->sample_sequence_offset = (input->sample_sequence_offset + 1) % input->sample_sequence_size;

        if (input->raw_reader) {
            return (input->raw_reader->ReadSamples(num_frame_samples) == num_frame_samples ? 1 : 0);
        } else {
            Frame *last_frame = input->wave_reader->GetTrack(input->wave_track_index)->GetFrameBuffer()->GetLastFrame(false);
            if (last_frame)
                return (last_frame->num_samples == num_frame_samples ? 1 : 0);
            else
                return (input->wave_reader->Read(num_frame_samples) == num_frame_samples ? 1 : 0);
        }
    } else {
        BMX_ASSERT(input->sample_sequence_size == 1 && input->sample_sequence[0] == 1);
        if (input->raw_reader) {
            return input->raw_reader->ReadSamples(max_samples_per_read);
        } else {
            Frame *last_frame = input->wave_reader->GetTrack(input->wave_track_index)->GetFrameBuffer()->GetLastFrame(false);
            if (last_frame)
                return last_frame->num_samples;
            else
                return input->wave_reader->Read(max_samples_per_read);
        }
    }
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

    if (input->essence_type == AVCI100_1080I ||
        input->essence_type == AVCI100_1080P ||
        input->essence_type == AVCI100_720P ||
        input->essence_type == AVCI50_1080I ||
        input->essence_type == AVCI50_1080P ||
        input->essence_type == AVCI50_720P)
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

static bool open_wave_reader(RawInput *input)
{
    BMX_ASSERT(!input->wave_reader);

    WaveFileIO *wave_file = WaveFileIO::OpenRead(input->filename);
    if (!wave_file) {
        log_error("Failed to open wave file '%s'\n", input->filename);
        return false;
    }

    input->wave_reader = WaveReader::Open(wave_file, true);
    if (!input->wave_reader) {
        log_error("Failed to parse wave file '%s'\n", input->filename);
        return false;
    }

    return true;
}

static void init_input(RawInput *input)
{
    memset(input, 0, sizeof(*input));
    input->aspect_ratio = ASPECT_RATIO_16_9;
    input->aspect_ratio_set = false;
    input->sampling_rate = DEFAULT_SAMPLING_RATE;
    input->component_depth = 8;
    input->audio_quant_bits = 16;
}

static void copy_input(const RawInput *from, RawInput *to)
{
    memcpy(to, from, sizeof(*to));
    to->track = 0;
}

static void clear_input(RawInput *input)
{
    delete input->raw_reader;
    if (input->wave_track_index == 0)
        delete input->wave_reader;
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
    else if (strcmp(clip_type_str, "wave") == 0)
        *clip_type = CW_WAVE_CLIP_TYPE;
    else
        return false;

    return true;
}

static string get_version_info()
{
    char buffer[256];
    sprintf(buffer, "raw2bmx, %s v%s (%s %s)", get_bmx_library_name().c_str(),
                                               get_bmx_version_string().c_str(),
                                               __DATE__, __TIME__);
    return buffer;
}

static void usage(const char *cmd)
{
    fprintf(stderr, "%s\n", get_version_info().c_str());
    fprintf(stderr, "Usage: %s <<options>> [<<input options>> <input>]+\n", cmd);
    fprintf(stderr, "Options (* means option is required):\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -l <file>               Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, "  -t <type>               Clip type: as02, as11op1a, as11d10, op1a, avid, d10, wave. Default is as02\n");
    fprintf(stderr, "* -o <name>               as02: <name> is a bundle name\n");
    fprintf(stderr, "                          as11op1a/as11d10/op1a/d10/wave: <name> is a filename\n");
    fprintf(stderr, "                          avid: <name> is a filename prefix\n");
    fprintf(stderr, "  -f <rate>               Frame rate: 23976 (24000/1001), 24, 25, 2997 (30000/1001), 50 or 5994 (60000/1001). Default parsed or 25\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames in frame rate units. Default is minimum input duration\n");
    fprintf(stderr, "  --avcihead <format> <file> <offset>\n");
    fprintf(stderr, "                          Default AVC-Intra sequence header data (512 bytes) to use when the input file does have it\n");
    fprintf(stderr, "                          <format> is a comma separated list of one or more of the following integer values:\n");
    size_t i;
    for (i = 0; i < get_num_avci_header_formats(); i++)
        fprintf(stderr, "                              %2"PRIszt": %s\n", i, get_avci_header_format_string(i));
    fprintf(stderr, "                          or set <format> to 'all' for all formats listed above\n");
    fprintf(stderr, "                          The 512 bytes are extracted from <file> starting at <offset> bytes\n");
    fprintf(stderr, "                              and incrementing 512 bytes for each format in the list\n");
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
    fprintf(stderr, "  as11op1a/op1a:\n");
    fprintf(stderr, "    --single-pass           Write file in a single pass\n");
    fprintf(stderr, "                            The header and body partitions will be incomplete\n");
    fprintf(stderr, "    --file-md5              Calculate an MD5 checksum of the file. This requires writing in a single pass (--single-pass is assumed)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11d10/d10:\n");
    fprintf(stderr, "    --d10-mute <flags>      Indicate using a string of 8 '0' or '1' which sound channels should be muted. The lsb is the rightmost digit\n");
    fprintf(stderr, "    --d10-invalid <flags>   Indicate using a string of 8 '0' or '1' which sound channels should be flagged invalid. The lsb is the rightmost digit\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  avid:\n");
    fprintf(stderr, "    --project <name>        Set the Avid project name\n");
    fprintf(stderr, "    --tape <name>           Source tape name\n");
    fprintf(stderr, "    --comment <string>      Add 'Comments' user comment to the MaterialPackage\n");
    fprintf(stderr, "    --desc <string>         Add 'Descript' user comment to the MaterialPackage\n");
    fprintf(stderr, "    --tag <name> <value>    Add <name> user comment to the MaterialPackage. Option can be used multiple times\n");
    fprintf(stderr, "    --locator <position> <comment> <color>\n");
    fprintf(stderr, "                            Add locator at <position> (in frame rate units) with <comment> and <color>\n");
    fprintf(stderr, "                            <position> format is o?hh:mm:sscff, where the optional 'o' indicates it is an offset\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  wave:\n");
    fprintf(stderr, "    --orig <name>           Set originator in the output Wave bext chunk. Default '%s'\n", DEFAULT_BEXT_ORIGINATOR);
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input Options (must precede the input to which it applies):\n");
    fprintf(stderr, "  -a <n:d>                Image aspect ratio. Either 4:3 or 16:9. Default parsed or 16:9\n");
    fprintf(stderr, "  --afd <value>           Active Format Descriptor code. Default not set\n");
    fprintf(stderr, "  -c <depth>              Component depth for uncompressed/DV100 video. Either 8 or 10. Default parsed or 8\n");
    fprintf(stderr, "  --height                Height of input uncompressed video data. Default is the production aperture height, except for PAL (592) and NTSC (496)\n");
    fprintf(stderr, "  -s <bps>                Audio sampling rate numerator. Default %d\n", DEFAULT_SAMPLING_RATE.numerator);
    fprintf(stderr, "  -q <bps>                Audio quantization bits per sample. Either 16 or 24. Default 16\n");
    fprintf(stderr, "  --locked <bool>         Indicate whether the number of audio samples is locked to the video. Either true or false. Default not set\n");
    fprintf(stderr, "  --audio-ref <level>     Audio reference level, number of dBm for 0VU. Default not set\n");
    fprintf(stderr, "  --dial-norm <value>     Gain to be applied to normalize perceived loudness of the clip. Default not set\n");
    fprintf(stderr, "  --off <bytes>           Skip bytes at start of the next input/track's file\n");
    fprintf(stderr, "  --maxlen <bytes>        Maximum number of bytes to read from next input/track's file\n");
    fprintf(stderr, "  --track-num <num>       Set the output track number. Default track number equals last track number of same picture/sound type + 1\n");
    fprintf(stderr, "                          For as11d10/d10 the track number must be > 0 and <= 8 because the AES-3 channel index equals track number - 1\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02:\n");
    fprintf(stderr, "    --trk-out-start <offset>   Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    fprintf(stderr, "    --trk-out-end <offset>     Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
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
    fprintf(stderr, "  --wave <name>           Wave PCM audio input file\n");
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
    AS11Helper as11_helper;
    const char *segmentation_filename = 0;
    bool sequence_offset_set = false;
    uint8_t sequence_offset = 0;
    bool do_print_version = false;
    vector<AVCIHeaderInput> avci_header_inputs;
    bool single_pass = false;
    bool file_md5 = false;
    uint8_t d10_mute_sound_flags = 0;
    uint8_t d10_invalid_sound_flags = 0;
    const char *originator = DEFAULT_BEXT_ORIGINATOR;
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
            if (!parse_frame_rate(argv[cmdln_index + 1], &frame_rate))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
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
        else if (strcmp(argv[cmdln_index], "--avcihead") == 0)
        {
            if (cmdln_index + 3 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_avci_header(argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3],
                                   &avci_header_inputs))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid values '%s', '%s', '%s' for option '%s'\n",
                        argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3], argv[cmdln_index]);
                return 1;
            }
            cmdln_index += 3;
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
            if (!as11_helper.SetFrameworkProperty(argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3]))
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to set '%s' framework property '%s' to '%s'\n",
                        argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3]);
                return 1;
            }
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
            if (!as11_helper.ParseFrameworkFile(argv[cmdln_index + 1], argv[cmdln_index + 2]))
            {
                usage(argv[0]);
                fprintf(stderr, "Failed to parse '%s' framework file '%s'\n",
                        argv[cmdln_index + 1], argv[cmdln_index + 2]);
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
        else if (strcmp(argv[cmdln_index], "--single-pass") == 0)
        {
            single_pass = true;
        }
        else if (strcmp(argv[cmdln_index], "--file-md5") == 0)
        {
            file_md5 = true;
        }
        else if (strcmp(argv[cmdln_index], "--d10-mute") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_d10_sound_flags(argv[cmdln_index + 1], &d10_mute_sound_flags))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--d10-invalid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_d10_sound_flags(argv[cmdln_index + 1], &d10_invalid_sound_flags))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
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
        else if (strcmp(argv[cmdln_index], "--orig") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            originator = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--regtest") == 0)
        {
            BMX_REGRESSION_TEST = true;
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
        else if (strcmp(argv[cmdln_index], "-s") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &uvalue) != 1 && uvalue > 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.sampling_rate.numerator = uvalue;
            input.sampling_rate.denominator = 1;
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
            input.essence_type = IEC_DV25;
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
            input.essence_type = DVBASED_DV25;
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
            input.essence_type = DV50;
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
            input.essence_type = DV100_1080I;
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
            input.essence_type = DV100_720P;
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
            input.essence_type = D10_30;
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
            input.essence_type = D10_40;
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
            input.essence_type = D10_50;
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
            input.essence_type = AVCI100_1080I;
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
            input.essence_type = AVCI100_1080P;
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
            input.essence_type = AVCI100_720P;
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
            input.essence_type = AVCI50_1080I;
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
            input.essence_type = AVCI50_1080P;
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
            input.essence_type = AVCI50_720P;
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
            input.essence_type = UNC_SD;
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
            input.essence_type = UNC_HD_1080I;
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
            input.essence_type = UNC_HD_1080P;
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
            input.essence_type = UNC_HD_720P;
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
            input.essence_type = MPEG2LG_422P_HL_1080I;
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
            input.essence_type = MPEG2LG_422P_HL_1080P;
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
            input.essence_type = MPEG2LG_422P_HL_720P;
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
            input.essence_type = MPEG2LG_MP_HL_1080I;
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
            input.essence_type = MPEG2LG_MP_HL_1080P;
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
            input.essence_type = MPEG2LG_MP_HL_720P;
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
            input.essence_type = MPEG2LG_MP_H14_1080I;
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
            input.essence_type = MPEG2LG_MP_H14_1080P;
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
            input.essence_type = MJPEG_2_1;
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
            input.essence_type = MJPEG_3_1;
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
            input.essence_type = MJPEG_10_1;
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
            input.essence_type = MJPEG_20_1;
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
            input.essence_type = MJPEG_4_1M;
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
            input.essence_type = MJPEG_10_1M;
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
            input.essence_type = MJPEG_15_1S;
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
            input.essence_type = VC3_1080P_1235;
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
            input.essence_type = VC3_1080P_1237;
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
            input.essence_type = VC3_1080P_1238;
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
            input.essence_type = VC3_1080I_1241;
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
            input.essence_type = VC3_1080I_1242;
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
            input.essence_type = VC3_1080I_1243;
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
            input.essence_type = VC3_720P_1250;
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
            input.essence_type = VC3_720P_1251;
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
            input.essence_type = VC3_720P_1252;
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
            input.essence_type = VC3_1080P_1253;
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
            input.essence_type = WAVE_PCM;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--wave") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = WAVE_PCM;
            input.is_wave = true;
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

    if (BMX_REGRESSION_TEST) {
        mxf_set_regtest_funcs();
        mxf_avid_set_regtest_funcs();
    }

    if (do_print_version)
        log_info("%s\n", get_version_info().c_str());


    int cmd_result = 0;
    try
    {
        // update Avid uncompressed essence types if component depth equals 10
        if (clip_type == CW_AVID_CLIP_TYPE) {
            size_t i;
            for (i = 0; i < inputs.size(); i++) {
                if (inputs[i].component_depth == 10) {
                    switch (inputs[i].essence_type)
                    {
                        case UNC_SD:
                            inputs[i].essence_type = AVID_10BIT_UNC_SD;
                            break;
                        case UNC_HD_1080I:
                            inputs[i].essence_type = AVID_10BIT_UNC_HD_1080I;
                            break;
                        case UNC_HD_1080P:
                            inputs[i].essence_type = AVID_10BIT_UNC_HD_1080P;
                            break;
                        case UNC_HD_720P:
                            inputs[i].essence_type = AVID_10BIT_UNC_HD_720P;
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

            if (input->is_wave) {
                if (!open_wave_reader(input))
                    throw false;
                BMX_ASSERT(input->wave_reader->GetNumTracks() > 0);

                input->wave_track_index = 0;
                input->sampling_rate = input->wave_reader->GetSamplingRate();
                input->audio_quant_bits = input->wave_reader->GetQuantizationBits();

                if (input->wave_reader->GetNumTracks() > 1) {
                    vector<RawInput> additional_inputs;
                    uint32_t j;
                    for (j = 1; j < input->wave_reader->GetNumTracks(); j++) {
                        RawInput new_input;
                        copy_input(input, &new_input);
                        new_input.wave_track_index = j;
                        additional_inputs.push_back(new_input);
                    }
                    inputs.insert(inputs.begin() + i + 1, additional_inputs.begin(), additional_inputs.end());

                    i += additional_inputs.size();
                }

                continue;
            }


            // TODO: require parse friendly essence data in regression test for all formats
            if (BMX_REGRESSION_TEST) {
                if (input->essence_type_group != NO_ESSENCE_GROUP) {
                    log_error("Regression test requires specific input format type, eg. --iecdv25 rather than --dv\n");
                    throw false;
                }
                continue;
            }

            if (!open_raw_reader(input))
                throw false;

            if (input->essence_type_group == DV_ESSENCE_GROUP ||
                input->essence_type == IEC_DV25 ||
                input->essence_type == DVBASED_DV25 ||
                input->essence_type == DV50 ||
                input->essence_type == DV100_1080I ||
                input->essence_type == DV100_720P)
            {
                DVEssenceParser *dv_parser = new DVEssenceParser();
                input->raw_reader->SetEssenceParser(dv_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to IEC DV-25 if no essence samples
                    if (input->essence_type_group == DV_ESSENCE_GROUP)
                        input->essence_type = IEC_DV25;
                } else {
                    dv_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == DV_ESSENCE_GROUP) {
                        switch (dv_parser->GetEssenceType())
                        {
                            case DVEssenceParser::IEC_DV25:
                                input->essence_type = IEC_DV25;
                                break;
                            case DVEssenceParser::DVBASED_DV25:
                                input->essence_type = DVBASED_DV25;
                                break;
                            case DVEssenceParser::DV50:
                                input->essence_type = DV50;
                                break;
                            case DVEssenceParser::DV100_1080I:
                                input->essence_type = DV100_1080I;
                                break;
                            case DVEssenceParser::DV100_720P:
                                input->essence_type = DV100_720P;
                                break;
                            case DVEssenceParser::UNKNOWN_DV:
                                log_error("Unknown DV essence type\n");
                                throw false;
                        }
                    }

                    input->raw_reader->SetFixedSampleSize(dv_parser->GetFrameSize());

                    if (!frame_rate_set) {
                        if (dv_parser->Is50Hz()) {
                            if (input->essence_type == DV100_720P)
                                frame_rate = FRAME_RATE_50;
                            else
                                frame_rate = FRAME_RATE_25;
                        } else {
                            if (input->essence_type == DV100_720P)
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
                    // default to VC3_1080I_1242 if no essence samples
                    input->essence_type = VC3_1080I_1242;
                } else {
                    vc3_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    switch (vc3_parser->GetCompressionId())
                    {
                        case 1235:
                            input->essence_type = VC3_1080P_1235;
                            break;
                        case 1237:
                            input->essence_type = VC3_1080P_1237;
                            break;
                        case 1238:
                            input->essence_type = VC3_1080P_1238;
                            break;
                        case 1241:
                            input->essence_type = VC3_1080I_1241;
                            break;
                        case 1242:
                            input->essence_type = VC3_1080I_1242;
                            break;
                        case 1243:
                            input->essence_type = VC3_1080I_1243;
                            break;
                        case 1250:
                            input->essence_type = VC3_720P_1250;
                            break;
                        case 1251:
                            input->essence_type = VC3_720P_1251;
                            break;
                        case 1252:
                            input->essence_type = VC3_720P_1252;
                            break;
                        case 1253:
                            input->essence_type = VC3_1080P_1253;
                            break;
                        default:
                            log_error("Unknown VC3 essence type\n");
                            throw false;
                    }
                }
            }
            else if (input->essence_type_group == D10_ESSENCE_GROUP ||
                     input->essence_type == D10_30 ||
                     input->essence_type == D10_40 ||
                     input->essence_type == D10_50)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to D10 50Mbps if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = D10_50;
                } else {
                    input->raw_reader->SetFixedSampleSize(input->raw_reader->GetSampleDataSize());

                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == D10_ESSENCE_GROUP) {
                        switch (mpeg2_parser->GetBitRate())
                        {
                            case 75000:
                                input->essence_type = D10_30;
                                break;
                            case 100000:
                                input->essence_type = D10_40;
                                break;
                            case 125000:
                                input->essence_type = D10_50;
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
                     input->essence_type == MPEG2LG_422P_HL_1080I ||
                     input->essence_type == MPEG2LG_422P_HL_1080P ||
                     input->essence_type == MPEG2LG_422P_HL_720P ||
                     input->essence_type == MPEG2LG_MP_HL_1080I ||
                     input->essence_type == MPEG2LG_MP_HL_1080P ||
                     input->essence_type == MPEG2LG_MP_HL_720P ||
                     input->essence_type == MPEG2LG_MP_H14_1080I ||
                     input->essence_type == MPEG2LG_MP_H14_1080P)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to 422P@HL 1080i if no essence samples
                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP)
                        input->essence_type = MPEG2LG_422P_HL_1080I;
                } else {
                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP) {
                        if (mpeg2_parser->IsProgressive()) {
                            if (mpeg2_parser->GetVerticalSize() == 1080) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x82:
                                        input->essence_type = MPEG2LG_422P_HL_1080P;
                                        break;
                                    case 0x44:
                                        input->essence_type = MPEG2LG_MP_HL_1080P;
                                        break;
                                    case 0x46:
                                        input->essence_type = MPEG2LG_MP_H14_1080P;
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
                                        input->essence_type = MPEG2LG_422P_HL_720P;
                                        break;
                                    case 0x44:
                                        input->essence_type = MPEG2LG_MP_HL_720P;
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
                                    input->essence_type = MPEG2LG_422P_HL_1080I;
                                    break;
                                case 0x44:
                                    input->essence_type = MPEG2LG_MP_HL_1080I;
                                    break;
                                case 0x46:
                                    input->essence_type = MPEG2LG_MP_H14_1080I;
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
            !as11_helper.ParseSegmentationFile(segmentation_filename, frame_rate))
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
            if (input->essence_type == WAVE_PCM) {
                if (!ClipWriterTrack::IsSupported(clip_type, input->essence_type, input->sampling_rate)) {
                    log_error("PCM sampling rate %d/%d not supported\n",
                              input->sampling_rate.numerator, input->sampling_rate.denominator);
                    throw false;
                }
            } else {
                if (!ClipWriterTrack::IsSupported(clip_type, input->essence_type, frame_rate)) {
                    log_error("Essence type '%s' @%d/%d fps not supported for clip type '%s'\n",
                              essence_type_to_string(input->essence_type),
                              frame_rate.numerator, frame_rate.denominator,
                              ClipWriter::ClipWriterTypeToString(input->clip_type).c_str());
                    throw false;
                }
            }
        }


        // create clip

        int flavour = 0;
        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_OP1A_CLIP_TYPE) {
            flavour = OP1A_DEFAULT_FLAVOUR;
            if (file_md5)
                flavour |= OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= OP1A_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_AS11_D10_CLIP_TYPE || clip_type == CW_D10_CLIP_TYPE) {
            flavour = D10_DEFAULT_FLAVOUR;
            // single pass flavours not (yet) supported
        }
        DefaultMXFFileFactory file_factory;
        ClipWriter *clip = 0;
        switch (clip_type)
        {
            case CW_AS02_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS02Clip(output_name, true, frame_rate, &file_factory, false);
                break;
            case CW_AS11_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS11OP1AClip(flavour, file_factory.OpenNew(output_name), frame_rate);
                break;
            case CW_AS11_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS11D10Clip(flavour, file_factory.OpenNew(output_name), frame_rate);
                break;
            case CW_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewOP1AClip(flavour, file_factory.OpenNew(output_name), frame_rate);
                break;
            case CW_AVID_CLIP_TYPE:
                clip = ClipWriter::OpenNewAvidClip(frame_rate, &file_factory, false);
                break;
            case CW_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewD10Clip(flavour, file_factory.OpenNew(output_name), frame_rate);
                break;
            case CW_WAVE_CLIP_TYPE:
                clip = ClipWriter::OpenNewWaveClip(WaveFileIO::OpenNew(output_name));
                break;
            case CW_UNKNOWN_CLIP_TYPE:
                BMX_ASSERT(false);
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

            if (clip_type == CW_AS11_OP1A_CLIP_TYPE) {
                as11_clip->SetPartitionInterval(partition_interval);
                as11_clip->SetOutputStartOffset(output_start_offset);
                as11_clip->SetOutputEndOffset(- output_end_offset);
            }

            if (clip_type == CW_AS11_D10_CLIP_TYPE) {
                D10File *d10_clip = as11_clip->GetD10Clip();
                d10_clip->SetMuteSoundFlags(d10_mute_sound_flags);
                d10_clip->SetInvalidSoundFlags(d10_invalid_sound_flags);
            }

            if (!clip_name && as11_helper.HaveProgrammeTitle())
                as11_clip->SetClipName(as11_helper.GetProgrammeTitle());
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
                    if (inputs[i].essence_type == WAVE_PCM)
                        num_sound_tracks++;
                    else
                        num_picture_tracks++;
                }
                tape_package = avid_clip->CreateDefaultTapeSource(tape_name, num_picture_tracks, num_sound_tracks);

                tape_package_picture_refs = avid_clip->GetPictureSourceReferences(tape_package);
                BMX_ASSERT(tape_package_picture_refs.size() == num_picture_tracks);
                tape_package_sound_refs = avid_clip->GetSoundSourceReferences(tape_package);
                BMX_ASSERT(tape_package_sound_refs.size() == num_sound_tracks);
            }
        } else if (clip_type == CW_D10_CLIP_TYPE) {
            D10File *d10_clip = clip->GetD10Clip();

            d10_clip->SetMuteSoundFlags(d10_mute_sound_flags);
            d10_clip->SetInvalidSoundFlags(d10_invalid_sound_flags);
        } else if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();

            if (originator)
                wave_clip->GetBroadcastAudioExtension()->SetOriginator(originator);
        }


        // open raw inputs, create and initialize track properties

        unsigned char avci_header_data[AVCI_HEADER_SIZE];
        uint32_t picture_track_count = 0;
        uint32_t sound_track_count = 0;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

            // open raw file reader

            if (!input->is_wave && !open_raw_reader(input))
                throw false;

            bool is_picture = (input->essence_type != WAVE_PCM);


            // create track

            if (clip_type == CW_AVID_CLIP_TYPE) {
                string track_name = create_mxf_track_filename(
                                            output_name,
                                            is_picture ? picture_track_count + 1 : sound_track_count + 1,
                                            is_picture);
                input->track = clip->CreateTrack(input->essence_type, track_name.c_str());
            } else {
                input->track = clip->CreateTrack(input->essence_type);
            }


            // initialize track

            if (input->track_number_set)
                input->track->SetOutputTrackNumber(input->track_number);

            if (clip_type == CW_AS02_CLIP_TYPE) {
                AS02Track *as02_track = input->track->GetAS02Track();
                as02_track->SetMICType(mic_type);
                as02_track->SetMICScope(ess_component_mic_scope);
                as02_track->SetOutputStartOffset(input->output_start_offset);
                as02_track->SetOutputEndOffset(- input->output_end_offset);

                AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(as02_track);
                if (as02_pict_track)
                    as02_pict_track->SetPartitionInterval(partition_interval);
            } else if (clip_type == CW_AVID_CLIP_TYPE) {
                AvidTrack *avid_track = input->track->GetAvidTrack();
                AvidClip *avid_clip = clip->GetAvidClip();

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
                    BMX_ASSERT(source_refs.size() == 1);
                    avid_track->SetSourceRef(source_refs[0].first, source_refs[0].second);
                }
            }

            switch (input->essence_type)
            {
                case IEC_DV25:
                case DVBASED_DV25:
                case DV50:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case DV100_1080I:
                case DV100_720P:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetComponentDepth(input->component_depth);
                    break;
                case D10_30:
                case D10_40:
                case D10_50:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    if (input->raw_reader->GetFixedSampleSize() != 0)
                        input->track->SetSampleSize(input->raw_reader->GetFixedSampleSize());
                    break;
                case AVCI100_1080I:
                case AVCI100_1080P:
                case AVCI100_720P:
                case AVCI50_1080I:
                case AVCI50_1080P:
                case AVCI50_720P:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetAVCIMode(AVCI_ALL_FRAME_HEADER_MODE);
                    if (have_avci_header_data(input->essence_type, frame_rate, avci_header_inputs)) {
                        if (!read_avci_header_data(input->essence_type, frame_rate, avci_header_inputs,
                                                   avci_header_data, sizeof(avci_header_data)))
                        {
                            log_error("Failed to read AVC-Intra header data from input file for %s\n",
                                      essence_type_to_string(input->essence_type));
                            throw false;
                        }
                        input->track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                    }
                    break;
                case UNC_SD:
                case UNC_HD_1080I:
                case UNC_HD_1080P:
                case UNC_HD_720P:
                case AVID_10BIT_UNC_SD:
                case AVID_10BIT_UNC_HD_1080I:
                case AVID_10BIT_UNC_HD_1080P:
                case AVID_10BIT_UNC_HD_720P:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetComponentDepth(input->component_depth);
                    if (input->input_height > 0)
                        input->track->SetInputHeight(input->input_height);
                    break;
                case MPEG2LG_422P_HL_1080I:
                case MPEG2LG_422P_HL_1080P:
                case MPEG2LG_422P_HL_720P:
                case MPEG2LG_MP_HL_1080I:
                case MPEG2LG_MP_HL_1080P:
                case MPEG2LG_MP_HL_720P:
                case MPEG2LG_MP_H14_1080I:
                case MPEG2LG_MP_H14_1080P:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case MJPEG_2_1:
                case MJPEG_3_1:
                case MJPEG_10_1:
                case MJPEG_20_1:
                case MJPEG_4_1M:
                case MJPEG_10_1M:
                case MJPEG_15_1S:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    input->track->SetAspectRatio(input->aspect_ratio);
                    break;
                case VC3_1080P_1235:
                case VC3_1080P_1237:
                case VC3_1080P_1238:
                case VC3_1080I_1241:
                case VC3_1080I_1242:
                case VC3_1080I_1243:
                case VC3_720P_1250:
                case VC3_720P_1251:
                case VC3_720P_1252:
                case VC3_1080P_1253:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    break;
                case WAVE_PCM:
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
                case D10_AES3_PCM:
                case PICTURE_ESSENCE:
                case SOUND_ESSENCE:
                case UNKNOWN_ESSENCE_TYPE:
                    BMX_ASSERT(false);
            }


            // initialize raw input

            switch (input->essence_type)
            {
                case IEC_DV25:
                case DVBASED_DV25:
                case DV50:
                case DV100_1080I:
                case DV100_720P:
                case D10_30:
                case D10_40:
                case D10_50:
                case AVCI100_1080I:
                case AVCI100_1080P:
                case AVCI100_720P:
                case AVCI50_1080I:
                case AVCI50_1080P:
                case AVCI50_720P:
                case VC3_1080P_1235:
                case VC3_1080P_1237:
                case VC3_1080P_1238:
                case VC3_1080I_1241:
                case VC3_1080I_1242:
                case VC3_1080I_1243:
                case VC3_720P_1250:
                case VC3_720P_1251:
                case VC3_720P_1252:
                case VC3_1080P_1253:
                case UNC_SD:
                case UNC_HD_1080I:
                case UNC_HD_1080P:
                case UNC_HD_720P:
                case AVID_10BIT_UNC_SD:
                case AVID_10BIT_UNC_HD_1080I:
                case AVID_10BIT_UNC_HD_1080P:
                case AVID_10BIT_UNC_HD_720P:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader->GetFixedSampleSize() == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetInputSampleSize());
                    break;
                case MPEG2LG_422P_HL_1080I:
                case MPEG2LG_422P_HL_1080P:
                case MPEG2LG_422P_HL_720P:
                case MPEG2LG_MP_HL_1080I:
                case MPEG2LG_MP_HL_1080P:
                case MPEG2LG_MP_HL_720P:
                case MPEG2LG_MP_H14_1080I:
                case MPEG2LG_MP_H14_1080P:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MPEG2EssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case MJPEG_2_1:
                case MJPEG_3_1:
                case MJPEG_10_1:
                case MJPEG_20_1:
                case MJPEG_4_1M:
                case MJPEG_10_1M:
                case MJPEG_15_1S:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new MJPEGEssenceParser(input->track->IsSingleField()));
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case WAVE_PCM:
                {
                    vector<uint32_t> shifted_sample_sequence = input->track->GetShiftedSampleSequence();
                    BMX_ASSERT(shifted_sample_sequence.size() < ARRAY_SIZE(input->sample_sequence));
                    memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                           shifted_sample_sequence.size() * sizeof(uint32_t));
                    input->sample_sequence_size = shifted_sample_sequence.size();
                    if (input->raw_reader)
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    break;
                }
                case D10_AES3_PCM:
                case PICTURE_ESSENCE:
                case SOUND_ESSENCE:
                case UNKNOWN_ESSENCE_TYPE:
                    BMX_ASSERT(false);
            }

            if (is_picture)
                picture_track_count++;
            else
                sound_track_count++;
        }


        // add AS-11 descriptive metadata

        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE) {
            AS11Clip *as11_clip = clip->GetAS11Clip();
            as11_clip->PrepareHeaderMetadata();
            as11_helper.InsertFrameworks(as11_clip);
        }


        // read more than 1 sample to improve efficiency if the input is sound only and the output
        // doesn't require a sample sequence

        bool have_sound_sample_sequence = true;
        bool sound_only = true;
        uint32_t max_samples_per_read = 1;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->essence_type == WAVE_PCM) {
                if (input->sample_sequence_size == 1 && input->sample_sequence[0] == 1)
                    have_sound_sample_sequence = false;
            } else {
                sound_only = false;
            }
        }
        BMX_ASSERT(sound_only || have_sound_sample_sequence);
        if (sound_only && !have_sound_sample_sequence)
            max_samples_per_read = 1920;


        // create clip file(s) and write samples

        clip->PrepareWrite();

        // write samples
        int64_t total_read = 0;
        while (duration < 0 || total_read < duration) {
            // read samples into input buffers first to ensure the frame data is available for all tracks
            uint32_t min_num_samples = max_samples_per_read;
            uint32_t num_samples;
            for (i = 0; i < inputs.size(); i++) {
                num_samples = read_samples(&inputs[i], max_samples_per_read);
                if (num_samples < min_num_samples) {
                    min_num_samples = num_samples;
                    if (min_num_samples == 0)
                        break;
                }
            }
            if (min_num_samples == 0)
                break;
            BMX_ASSERT(min_num_samples == max_samples_per_read || max_samples_per_read > 1);
            total_read += min_num_samples;

            // write samples from input buffers
            for (i = 0; i < inputs.size(); i++) {
                RawInput *input = &inputs[i];
                if (input->raw_reader) {
                    num_samples = input->raw_reader->GetNumSamples();
                    if (max_samples_per_read > 1 && num_samples > min_num_samples)
                        num_samples = min_num_samples;
                    input->track->WriteSamples(input->raw_reader->GetSampleData(),
                                               input->raw_reader->GetSampleDataSize(),
                                               num_samples);
                } else {
                    Frame *frame = input->wave_reader->GetTrack(input->wave_track_index)->GetFrameBuffer()->GetLastFrame(true);
                    BMX_ASSERT(frame);
                    num_samples = frame->num_samples;
                    if (max_samples_per_read > 1 && num_samples > min_num_samples)
                        num_samples = min_num_samples;
                    input->track->WriteSamples(frame->GetBytes(),
                                               frame->GetSize(),
                                               num_samples);
                    delete frame;
                }
            }

            if (min_num_samples < max_samples_per_read)
                break;
        }


        // complete AS-11 descriptive metadata

        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE)
            as11_helper.Complete();


        // complete writing

        clip->CompleteWrite();

        log_info("Duration: %"PRId64" (%s)\n",
                 clip->GetDuration(),
                 get_generic_duration_string_2(clip->GetDuration(), clip->GetFrameRate()).c_str());


        if (file_md5) {
            if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_OP1A_CLIP_TYPE) {
                OP1AFile *op1a_clip = clip->GetOP1AClip();
                AS11Clip *as11_clip = clip->GetAS11Clip();
                if (as11_clip)
                    op1a_clip = as11_clip->GetOP1AClip();

                log_info("File MD5: %s\n", op1a_clip->GetMD5DigestStr().c_str());
            }
        }


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
    catch (const BMXException &ex)
    {
        log_error("BMX exception caught: %s\n", ex.what());
        cmd_result = 1;
    }
    catch (const bool &ex)
    {
        (void)ex;
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

