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

#include <im/d10_mxf/D10File.h>
#include <im/d10_mxf/D10MPEGTrack.h>
#include <im/d10_mxf/D10PCMTrack.h>
#include <im/essence_parser/MPEG2EssenceParser.h>
#include <im/essence_parser/RawEssenceReader.h>
#include <im/Utils.h>
#include <im/MXFUtils.h>
#include "../AppUtils.h"
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



typedef struct
{
    D10EssenceType essence_type;

    D10Track *track;

    const char *filename;
    int64_t file_start_offset;
    int64_t file_max_length;

    RawEssenceReader *raw_reader;
    uint32_t sample_sequence[32];
    size_t sample_sequence_size;
    size_t sample_sequence_offset;

    // picture
    mxfRational aspect_ratio;
    bool aspect_ratio_set;
    uint8_t afd;

    // sound
    mxfRational sampling_rate;
    uint32_t audio_quant_bits;
    bool locked;
    bool locked_set;
    int8_t audio_ref_level;
    bool audio_ref_level_set;
    int8_t dial_norm;
    bool dial_norm_set;
    uint8_t audio_output_channel_index;
    bool audio_output_channel_index_set;
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

    input->raw_reader = new RawEssenceReader(raw_file);
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
}

static void clear_input(RawInput *input)
{
    delete input->raw_reader;
}

static string get_version_info()
{
    char buffer[256];
    sprintf(buffer, "raw2d10mxf, %s v%s (%s %s)", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
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
    fprintf(stderr, "  -f <rate>               Frame rate: 25 or 30 (30000/1001). Default 25\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames. Default is minimum available duration\n");
    fprintf(stderr, "  --seq-off <value>       Sound sample sequence offset. Default 0\n");
    fprintf(stderr, "Input Options (must precede the input to which it applies):\n");
    fprintf(stderr, "  -a <n:d>                Image aspect ratio. Either 4:3 or 16:9. Default parsed or 16:9\n");
    fprintf(stderr, "  --afd <value>           Active Format Descriptor code. Default not set\n");
    fprintf(stderr, "  -q <bps>                Audio quantization bits per sample. Either 16 or 24. Default 16\n");
    fprintf(stderr, "  --locked <bool>         Indicate whether the number of audio samples is locked to the video. Either true or false. Default not set\n");
    fprintf(stderr, "  --audio-ref <level>     Audio reference level, number of dBm for 0VU. Default not set\n");
    fprintf(stderr, "  --dial-norm <value>     Gain to be applied to normalize perceived loudness of the clip. Default not set\n");
    fprintf(stderr, "  --audio-ch <index>      Audio output channel index (0..7). Default is last input's index + 1\n");
    fprintf(stderr, "  --off <bytes>           Skip bytes at start of the next input/track's file\n");
    fprintf(stderr, "  --maxlen <bytes>        Maximum number of bytes to read from next input/track's file\n");
    fprintf(stderr, "Inputs:\n");
    fprintf(stderr, "  --d10 <name>            Raw D10 video input file\n");
    fprintf(stderr, "  --d10_30 <name>         Raw D10 30Mbps video input file\n");
    fprintf(stderr, "  --d10_40 <name>         Raw D10 40Mbps video input file\n");
    fprintf(stderr, "  --d10_50 <name>         Raw D10 50Mbps video input file\n");
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
    int64_t duration = -1;
    uint8_t sound_sequence_offset = 0;
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
                (value != 25 && value != 30))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            if (value == 25)
                frame_rate = FRAME_RATE_25;
            else
                frame_rate = FRAME_RATE_2997;
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
        else if (strcmp(argv[cmdln_index], "--seq-off") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &uvalue) != 1 || uvalue >= 255)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            sound_sequence_offset = uvalue;
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
        else if (strcmp(argv[cmdln_index], "--audio-ch") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &uvalue) != 1 ||
                uvalue >= 8)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.audio_output_channel_index = uvalue;
            input.audio_output_channel_index_set = true;
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
        else if (strcmp(argv[cmdln_index], "--d10_30") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = D10_MPEG_30;
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
            input.essence_type = D10_UNKNOWN_ESSENCE;
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
            input.essence_type = D10_MPEG_40;
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
            input.essence_type = D10_MPEG_50;
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
            input.essence_type = D10_PCM;
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

            if (!open_raw_reader(input))
                throw false;

            if (input->essence_type == D10_UNKNOWN_ESSENCE ||
                input->essence_type == D10_MPEG_30 ||
                input->essence_type == D10_MPEG_40 ||
                input->essence_type == D10_MPEG_50)
            {
                MPEG2EssenceParser *mpeg2_parser = new MPEG2EssenceParser();
                input->raw_reader->SetEssenceParser(mpeg2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    // default to D10 50Mbps if no essence samples
                    if (input->essence_type == D10_UNKNOWN_ESSENCE)
                        input->essence_type = D10_MPEG_50;
                } else {
                    input->raw_reader->SetFixedSampleSize(input->raw_reader->GetSampleDataSize());

                    mpeg2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                                 input->raw_reader->GetSampleDataSize());

                    if (input->essence_type == D10_UNKNOWN_ESSENCE) {
                        switch (mpeg2_parser->GetBitRate())
                        {
                            case 75000:
                                input->essence_type = D10_MPEG_30;
                                break;
                            case 100000:
                                input->essence_type = D10_MPEG_40;
                                break;
                            case 125000:
                                input->essence_type = D10_MPEG_50;
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
        }


        // complete commandline parsing

        start_timecode.Init(frame_rate, false);
        if (start_timecode_str && !parse_timecode(start_timecode_str, frame_rate, &start_timecode)) {
            usage(argv[0]);
            log_error("Invalid value '%s' for option '-t'\n", start_timecode_str);
            throw false;
        }


        // check frame/sampling rates
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->essence_type == D10_PCM) {
                if (!D10Track::IsSupported(input->essence_type, input->sampling_rate)) {
                    log_error("PCM sampling rate %d/%d not supported\n",
                              input->sampling_rate.numerator, input->sampling_rate.denominator);
                    throw false;
                }
            } else {
                if (!D10Track::IsSupported(input->essence_type, frame_rate)) {
                    log_error("Frame rate %d/%d not supported for format '%s'\n",
                              frame_rate.numerator, frame_rate.denominator,
                              MXFDescriptorHelper::EssenceTypeToString(D10Track::ConvertEssenceType(input->essence_type)).c_str());
                    throw false;
                }
            }
        }


        D10File *d10_file = D10File::OpenNew(output_filename, frame_rate);


        // initialize data structures and inputs

        d10_file->SetStartTimecode(start_timecode);
        if (clip_name)
            d10_file->SetClipName(clip_name);
        d10_file->SetSoundSequenceOffset(sound_sequence_offset);

        D10MPEGTrack *mpeg_track = 0;
        D10PCMTrack *pcm_track = 0;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

            // open raw file reader

            if (!open_raw_reader(input))
                throw false;


            // create track and configure

            input->track = d10_file->CreateTrack(input->essence_type);

            switch (input->essence_type)
            {
                case D10_MPEG_30:
                case D10_MPEG_40:
                case D10_MPEG_50:
                    mpeg_track = dynamic_cast<D10MPEGTrack*>(input->track);
                    mpeg_track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        mpeg_track->SetAFD(input->afd);
                    if (input->raw_reader->GetFixedSampleSize() != 0)
                        mpeg_track->SetSampleSize(input->raw_reader->GetFixedSampleSize());
                    break;
                case D10_PCM:
                    pcm_track = dynamic_cast<D10PCMTrack*>(input->track);
                    pcm_track->SetSamplingRate(input->sampling_rate);
                    pcm_track->SetQuantizationBits(input->audio_quant_bits);
                    if (input->locked_set)
                        pcm_track->SetLocked(input->locked);
                    if (input->audio_ref_level_set)
                        pcm_track->SetAudioRefLevel(input->audio_ref_level);
                    if (input->dial_norm_set)
                        pcm_track->SetDialNorm(input->dial_norm);
                    if (input->audio_output_channel_index_set)
                        pcm_track->SetOutputChannelIndex(input->audio_output_channel_index);
                    break;
                case D10_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }

            // set input sample info and create buffer
            switch (input->essence_type)
            {
                case D10_MPEG_30:
                case D10_MPEG_40:
                case D10_MPEG_50:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader->GetFixedSampleSize() == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize());
                    break;
                case D10_PCM:
                    {
                        vector<uint32_t> shifted_sample_sequence = pcm_track->GetShiftedSampleSequence();
                        IM_ASSERT(shifted_sample_sequence.size() < sizeof(input->sample_sequence) / sizeof(uint32_t));
                        memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                               shifted_sample_sequence.size() * sizeof(uint32_t));
                        input->sample_sequence_size = shifted_sample_sequence.size();
                        input->raw_reader->SetFixedSampleSize(pcm_track->GetSampleSize());
                    }
                    break;
                case D10_UNKNOWN_ESSENCE:
                    IM_ASSERT(false);
            }
        }

        d10_file->PrepareWrite();

        // write samples
        while (duration < 0 || d10_file->GetDuration() < duration) {
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

        d10_file->CompleteWrite();

        log_info("Duration: %s (%"PRId64" frames @%d/%d fps)\n",
                 get_generic_duration_string(d10_file->GetDuration(), d10_file->GetFrameRate()).c_str(),
                 d10_file->GetDuration(), d10_file->GetFrameRate().numerator, d10_file->GetFrameRate().denominator);


        // clean up
        for (i = 0; i < inputs.size(); i++)
            clear_input(&inputs[i]);
        delete d10_file;
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

