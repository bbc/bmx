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
#include <bmx/essence_parser/DVEssenceParser.h>
#include <bmx/essence_parser/MPEG2EssenceParser.h>
#include <bmx/essence_parser/AVCEssenceParser.h>
#include <bmx/essence_parser/AVCIRawEssenceReader.h>
#include <bmx/essence_parser/MJPEGEssenceParser.h>
#include <bmx/essence_parser/VC3EssenceParser.h>
#include <bmx/essence_parser/RawEssenceReader.h>
#include <bmx/essence_parser/FileEssenceSource.h>
#include <bmx/essence_parser/KLVEssenceSource.h>
#include <bmx/essence_parser/MPEG2AspectRatioFilter.h>
#include <bmx/wave/WaveFileIO.h>
#include <bmx/wave/WaveReader.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/URI.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Version.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/apps/AS11Helper.h>
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
    AVCI_ESSENCE_GROUP,
} EssenceTypeGroup;


typedef struct
{
    const char *position_str;
    AvidLocator locator;
} LocatorOption;

typedef struct
{
    EssenceTypeGroup essence_type_group;
    bool avci_guess_interlaced;
    bool avci_guess_progressive;
    EssenceType essence_type;
    bool is_wave;

    ClipWriterTrack *track;

    const char *filename;
    int64_t file_start_offset;
    int64_t ess_max_length;
    bool parse_klv;
    mxfKey klv_key;
    uint32_t klv_track_num;

    int64_t output_start_offset;
    int64_t output_end_offset;

    RawEssenceReader *raw_reader;
    WaveReader *wave_reader;
    uint32_t wavepcm_track_index;
    uint32_t channel_count;

    uint32_t sample_sequence[32];
    size_t sample_sequence_size;
    size_t sample_sequence_offset;

    uint32_t track_number;
    bool track_number_set;

    EssenceFilter *filter;
    bool set_bs_aspect_ratio;

    // picture
    Rational aspect_ratio;
    bool aspect_ratio_set;
    uint8_t afd;
    uint32_t component_depth;
    uint32_t input_height;
    bool have_avci_header;
    bool d10_fixed_frame_size;

    // sound
    Rational sampling_rate;
    uint32_t audio_quant_bits;
    bool locked;
    bool locked_set;
    int8_t audio_ref_level;
    bool audio_ref_level_set;
    int8_t dial_norm;
    bool dial_norm_set;

    // ANC/VBI
    uint32_t anc_const_size;
    uint32_t vbi_const_size;
} RawInput;


static const char APP_NAME[]                = "raw2bmx";

static const char DEFAULT_SHIM_NAME[]       = "Sample File";
static const char DEFAULT_SHIM_ID[]         = "http://bbc.co.uk/rd/as02/default-shim.txt";
static const char DEFAULT_SHIM_ANNOTATION[] = "Default AS-02 shim";

static const char DEFAULT_BEXT_ORIGINATOR[] = "bmx";

static const Rational DEFAULT_SAMPLING_RATE = SAMPLING_RATE_48K;


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};



static uint32_t read_samples(RawInput *input, uint32_t max_samples_per_read)
{
    if (max_samples_per_read == 1) {
        uint32_t num_frame_samples = input->sample_sequence[input->sample_sequence_offset];
        input->sample_sequence_offset = (input->sample_sequence_offset + 1) % input->sample_sequence_size;

        if (input->raw_reader) {
            if (input->essence_type == WAVE_PCM && input->wavepcm_track_index > 0)
                return (input->raw_reader->GetNumSamples() == num_frame_samples ? 1 : 0);
            else
                return (input->raw_reader->ReadSamples(num_frame_samples) == num_frame_samples ? 1 : 0);
        } else {
            Frame *last_frame = input->wave_reader->GetTrack(input->wavepcm_track_index)->GetFrameBuffer()->GetLastFrame(false);
            if (last_frame)
                return (last_frame->num_samples == num_frame_samples ? 1 : 0);
            else
                return (input->wave_reader->Read(num_frame_samples) == num_frame_samples ? 1 : 0);
        }
    } else {
        BMX_ASSERT(input->sample_sequence_size == 1 && input->sample_sequence[0] == 1);
        if (input->raw_reader) {
            if (input->essence_type == WAVE_PCM && input->wavepcm_track_index > 0)
                return input->raw_reader->GetNumSamples();
            else
                return input->raw_reader->ReadSamples(max_samples_per_read);
        } else {
            Frame *last_frame = input->wave_reader->GetTrack(input->wavepcm_track_index)->GetFrameBuffer()->GetLastFrame(false);
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

    FileEssenceSource *file_source = new FileEssenceSource();
    if (!file_source->Open(input->filename, input->file_start_offset)) {
        log_error("Failed to open input file '%s' at start offset %"PRId64": %s\n",
                  input->filename, input->file_start_offset, file_source->GetStrError().c_str());
        delete file_source;
        return false;
    }

    EssenceSource *essence_source = file_source;
    if (input->parse_klv) {
        KLVEssenceSource *klv_source;
        if (input->klv_track_num)
            klv_source = new KLVEssenceSource(essence_source, input->klv_track_num);
        else
            klv_source = new KLVEssenceSource(essence_source, &input->klv_key);
        essence_source = klv_source;
    }

    if (input->essence_type == AVCI200_1080I ||
        input->essence_type == AVCI200_1080P ||
        input->essence_type == AVCI200_720P ||
        input->essence_type == AVCI100_1080I ||
        input->essence_type == AVCI100_1080P ||
        input->essence_type == AVCI100_720P ||
        input->essence_type == AVCI50_1080I ||
        input->essence_type == AVCI50_1080P ||
        input->essence_type == AVCI50_720P)
    {
        input->raw_reader = new AVCIRawEssenceReader(essence_source);
    }
    else
    {
        input->raw_reader = new RawEssenceReader(essence_source);
    }
    if (input->ess_max_length > 0)
        input->raw_reader->SetMaxReadLength(input->ess_max_length);

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

static void write_samples(RawInput *input, unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (input->filter) {
        if (input->filter->SupportsInPlaceFilter()) {
            input->filter->Filter(data, size);
            input->track->WriteSamples(data, size, num_samples);
        } else {
            unsigned char *f_data = 0;
            try
            {
                uint32_t f_size;
                input->filter->Filter(data, size, &f_data, &f_size);
                input->track->WriteSamples(f_data, f_size, num_samples);
                delete [] f_data;
            }
            catch (...)
            {
                delete [] f_data;
                throw;
            }
        }
    } else {
        input->track->WriteSamples(data, size, num_samples);
    }
}

static void init_input(RawInput *input)
{
    memset(input, 0, sizeof(*input));
    input->aspect_ratio = ASPECT_RATIO_16_9;
    input->aspect_ratio_set = false;
    input->sampling_rate = DEFAULT_SAMPLING_RATE;
    input->component_depth = 8;
    input->audio_quant_bits = 16;
    input->channel_count = 1;
}

static void copy_input(const RawInput *from, RawInput *to)
{
    memcpy(to, from, sizeof(*to));
    to->track = 0;
}

static void clear_input(RawInput *input)
{
    if (input->essence_type != WAVE_PCM || input->wavepcm_track_index == 0) {
        delete input->raw_reader;
        delete input->wave_reader;
    }
    delete input->filter;
}

static bool parse_avci_guess(const char *str, bool *interlaced, bool *progressive)
{
    if (strcmp(str, "i") == 0) {
        *interlaced  = true;
        *progressive = false;
        return true;
    } else if (strcmp(str, "p") == 0) {
        *interlaced  = false;
        *progressive = true;
        return true;
    }

    return false;
}

static void usage(const char *cmd)
{
    fprintf(stderr, "%s\n", get_app_version_info(APP_NAME).c_str());
    fprintf(stderr, "Usage: %s <<options>> [<<input options>> <input>]+\n", cmd);
    fprintf(stderr, "Options (* means option is required):\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -l <file>               Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, " --log-level <level>      Set the log level. 0=debug, 1=info, 2=warning, 3=error. Default is 1\n");
    fprintf(stderr, "  -t <type>               Clip type: as02, as11op1a, as11d10, op1a, avid, d10, rdd9, wave. Default is op1a\n");
    fprintf(stderr, "* -o <name>               as02: <name> is a bundle name\n");
    fprintf(stderr, "                          as11op1a/as11d10/op1a/d10/rdd9/wave: <name> is a filename\n");
    fprintf(stderr, "                          avid: <name> is a filename prefix\n");
    fprintf(stderr, "  --prod-info <cname>\n");
    fprintf(stderr, "              <pname>\n");
    fprintf(stderr, "              <ver>\n");
    fprintf(stderr, "              <verstr>\n");
    fprintf(stderr, "              <uid>\n");
    fprintf(stderr, "                          Set the product info in the MXF Identification set\n");
    fprintf(stderr, "                          <cname> is a string and is the Company Name property\n");
    fprintf(stderr, "                          <pname> is a string and is the Product Name property\n");
    fprintf(stderr, "                          <ver> has format '<major>.<minor>.<patch>.<build>.<release>' and is the Product Version property. Set to '0.0.0.0.0' to omit it\n");
    fprintf(stderr, "                          <verstr> is a string and is the Version String property\n");
    fprintf(stderr, "                          <uid> is a UUID (see Notes at the end) and is the Product UID property\n");
    fprintf(stderr, "  -f <rate>               Frame rate: 23976 (24000/1001), 24, 25, 2997 (30000/1001), 30, 50, 5994 (60000/1001) or 60. Default parsed or 25\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Start timecode. Default 00:00:00:00\n");
    fprintf(stderr, "                          The c character in the pattern should be ':' for non-drop frame; any other character indicates drop frame\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames in frame rate units. Default is minimum input duration\n");
    fprintf(stderr, "  --rt <factor>           Wrap at realtime rate x <factor>, where <factor> is a floating point value\n");
    fprintf(stderr, "                          <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
    fprintf(stderr, "  --avcihead <format> <file> <offset>\n");
    fprintf(stderr, "                          Default AVC-Intra sequence header data (512 bytes) to use when the input file does not have it\n");
    fprintf(stderr, "                          <format> is a comma separated list of one or more of the following integer values:\n");
    size_t i;
    for (i = 0; i < get_num_avci_header_formats(); i++)
        fprintf(stderr, "                              %2"PRIszt": %s\n", i, get_avci_header_format_string(i));
    fprintf(stderr, "                          or set <format> to 'all' for all formats listed above\n");
    fprintf(stderr, "                          The 512 bytes are extracted from <file> starting at <offset> bytes\n");
    fprintf(stderr, "                          and incrementing 512 bytes for each format in the list\n");
    fprintf(stderr, "  --ps-avcihead           Panasonic AVC-Intra sequence header data for Panasonic-compatible files that don't include the header data\n");
    fprintf(stderr, "                          These formats are supported:\n");
    for (i = 0; i < get_num_ps_avci_header_formats(); i++) {
        if (i == 0)
            fprintf(stderr, "                              ");
        else if (i % 4 == 0)
            fprintf(stderr, ",\n                              ");
        else
            fprintf(stderr, ", ");
        fprintf(stderr, "%s", get_ps_avci_header_format_string(i));
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02:\n");
    fprintf(stderr, "    --mic-type <type>       Media integrity check type: 'md5' or 'none'. Default 'md5'\n");
    fprintf(stderr, "    --mic-file              Calculate checksum for entire essence component file. Default is essence only\n");
    fprintf(stderr, "    --shim-name <name>      Set ShimName element value in shim.xml file to <name>. Default is '%s'\n", DEFAULT_SHIM_NAME);
    fprintf(stderr, "    --shim-id <id>          Set ShimID element value in shim.xml file to <id>. Default is '%s'\n", DEFAULT_SHIM_ID);
    fprintf(stderr, "    --shim-annot <str>      Set AnnotationText element value in shim.xml file to <str>. Default is '%s'\n", DEFAULT_SHIM_ANNOTATION);
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02/as11op1a/op1a/rdd9:\n");
    fprintf(stderr, "    --part <interval>       Video essence partition interval in frames, or seconds with 's' suffix. Default single partition\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10:\n");
    fprintf(stderr, "    --dm <fwork> <name> <value>    Set descriptive framework property. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --dm-file <fwork> <name>       Parse and set descriptive framework properties from text file <name>. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --seg <name>                   Parse and set segmentation data from text file <name>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/op1a/rdd9:\n");
    fprintf(stderr, "    --out-start <offset>    Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    fprintf(stderr, "    --out-end <offset>      Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10/op1a/d10/rdd9:\n");
    fprintf(stderr, "    --seq-off <value>       Sound sample sequence offset. Default 0 for as11d10/d10 and not set (0) for as11op1a/op1a\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/op1a/rdd9:\n");
    fprintf(stderr, "    --single-pass           Write file in a single pass\n");
    fprintf(stderr, "                            The header and body partitions will be incomplete\n");
    fprintf(stderr, "    --file-md5              Calculate an MD5 checksum of the file. This requires writing in a single pass (--single-pass is assumed)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  op1a:\n");
    fprintf(stderr, "    --min-part              Only use a header and footer MXF file partition. Use this for applications that don't support\n");
    fprintf(stderr, "                            separate partitions for header metadata, index tables, essence container data and footer\n");
    fprintf(stderr, "    --body-part             Create separate body partitions for essence data\n");
    fprintf(stderr, "                            and don't create separate body partitions for index table segments\n");
    fprintf(stderr, "    --clip-wrap             Use clip wrapping for a single sound track\n");
    fprintf(stderr, "    --zero-mp-track-num     Always set the Track Number property in the Material Package tracks to 0\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  op1a/rdd9:\n");
    fprintf(stderr, "    --ard-zdf-hdf           Use the ARD ZDF HDF profile\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11d10/d10:\n");
    fprintf(stderr, "    --d10-mute <flags>      Indicate using a string of 8 '0' or '1' which sound channels should be muted. The lsb is the rightmost digit\n");
    fprintf(stderr, "    --d10-invalid <flags>   Indicate using a string of 8 '0' or '1' which sound channels should be flagged invalid. The lsb is the rightmost digit\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  avid:\n");
    fprintf(stderr, "    --project <name>        Set the Avid project name\n");
    fprintf(stderr, "    --tape <name>           Source tape name\n");
    fprintf(stderr, "    --import <name>         Source import name. <name> is one of the following:\n");
    fprintf(stderr, "                              - a file URL starting with 'file://'\n");
    fprintf(stderr, "                              - an absolute Windows (starts with '[A-Z]:') or *nix (starts with '/') filename\n");
    fprintf(stderr, "                              - a relative filename, which will be converted to an absolute filename\n");
    fprintf(stderr, "    --comment <string>      Add 'Comments' user comment to the MaterialPackage\n");
    fprintf(stderr, "    --desc <string>         Add 'Descript' user comment to the MaterialPackage\n");
    fprintf(stderr, "    --tag <name> <value>    Add <name> user comment to the MaterialPackage. Option can be used multiple times\n");
    fprintf(stderr, "    --locator <position> <comment> <color>\n");
    fprintf(stderr, "                            Add locator at <position> (in frame rate units) with <comment> and <color>\n");
    fprintf(stderr, "                            <position> format is o?hh:mm:sscff, where the optional 'o' indicates it is an offset\n");
    fprintf(stderr, "    --mp-uid <umid>         Set the Material Package UID. Autogenerated by default\n");
    fprintf(stderr, "    --mp-created <tstamp>   Set the Material Package creation date. Default is 'now'\n");
    fprintf(stderr, "    --psp-uid <umid>        Set the tape/import Source Package UID. Autogenerated by default\n");
    fprintf(stderr, "    --psp-created <tstamp>  Set the tape/import Source Package creation date. Default is 'now'\n");
    fprintf(stderr, "    --allow-no-avci-head    Allow inputs with no AVCI header (512 bytes, sequence and picture parameter sets)\n");
    fprintf(stderr, "    --avid-gf               Use the Avid growing file flavour\n");
    fprintf(stderr, "    --avid-gf-dur <dur>     Set the duration which should be shown whilst the file is growing\n");
    fprintf(stderr, "                            Avid will show 'Capture in Progress' when this option is used\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  op1a/avid:\n");
    fprintf(stderr, "    --force-no-avci-head    Strip AVCI header (512 bytes, sequence and picture parameter sets) if present\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  wave:\n");
    fprintf(stderr, "    --orig <name>           Set originator in the output Wave bext chunk. Default '%s'\n", DEFAULT_BEXT_ORIGINATOR);
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input Options (must precede the input to which it applies):\n");
    fprintf(stderr, "  -a <n:d>                Image aspect ratio. Either 4:3 or 16:9. Default parsed or 16:9\n");
    fprintf(stderr, "  --bsar                  Set image aspect ratio in video bitstream. Currently supports D-10 essence types only\n");
    fprintf(stderr, "  --afd <value>           Active Format Descriptor 4-bit code from table 1 in SMPTE ST 2016-1. Default not set\n");
    fprintf(stderr, "  -c <depth>              Component depth for uncompressed/DV100 video. Either 8 or 10. Default parsed or 8\n");
    fprintf(stderr, "  --height                Height of input uncompressed video data. Default is the production aperture height, except for PAL (592) and NTSC (496)\n");
    fprintf(stderr, "  -s <bps>                Audio sampling rate numerator for raw pcm. Default %d\n", DEFAULT_SAMPLING_RATE.numerator);
    fprintf(stderr, "  -q <bps>                Audio quantization bits per sample for raw pcm. Either 16 or 24. Default 16\n");
    fprintf(stderr, "  --audio-chan <count>    Audio channel count for raw pcm. Default 1\n");
    fprintf(stderr, "  --locked <bool>         Indicate whether the number of audio samples is locked to the video. Either true or false. Default not set\n");
    fprintf(stderr, "  --audio-ref <level>     Audio reference level, number of dBm for 0VU. Default not set\n");
    fprintf(stderr, "  --dial-norm <value>     Gain to be applied to normalize perceived loudness of the clip. Default not set\n");
    fprintf(stderr, "  --anc-const <size>      Set the constant ANC data frame <size>\n");
    fprintf(stderr, "  --vbi-const <size>      Set the constant VBI data frame <size>\n");
    fprintf(stderr, "  --off <bytes>           Skip <bytes> at the start of the next input/track's file\n");
    fprintf(stderr, "  --maxlen <bytes>        Maximum number of essence data bytes to read from next input/track's file\n");
    fprintf(stderr, "  --klv <key>             Essence data is read from the KLV data in the next input/track's file\n");
    fprintf(stderr, "                          <key> should have one of the following values:\n");
    fprintf(stderr, "                            - 's', which means the first 16 bytes, at file position 0 or --off byte offset, are taken to be the Key\n");
    fprintf(stderr, "                            - optional '0x' followed by 8 hexadecimal characters which represents the 4-byte track number part of a generic container essence Key\n");
    fprintf(stderr, "                            - 32 hexadecimal characters representing a 16-byte Key\n");
    fprintf(stderr, "  --track-num <num>       Set the output track number. Default track number equals last track number of same picture/sound type + 1\n");
    fprintf(stderr, "                          For as11d10/d10 the track number must be > 0 and <= 8 because the AES-3 channel index equals track number - 1\n");
    fprintf(stderr, "  --avci-guess <i/p>      Guess interlaced ('i') or progressive ('p') AVC-Intra when using the --avci option with 1080p25/i50 or 1080p30/i60\n");
    fprintf(stderr, "                          The default guess uses the H.264 frame_mbs_only_flag field\n");
    fprintf(stderr, "  --fixed-size            Set to indicate that the d10 frames have a fixed size and therefore do not need to be parsed after the first frame\n");
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
    fprintf(stderr, "  --avci <name>           Raw AVC-Intra video input file. See also --avci-guess option\n");
    fprintf(stderr, "  --avci200_1080i <name>  Raw AVC-Intra 200 1080i video input file\n");
    fprintf(stderr, "  --avci200_1080p <name>  Raw AVC-Intra 200 1080p video input file\n");
    fprintf(stderr, "  --avci200_720p <name>   Raw AVC-Intra 200 720p video input file\n");
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
    fprintf(stderr, "  --unc_3840 <name>       Raw uncompressed UHD 3840x2160 UYVY 422 video input file\n");
    fprintf(stderr, "  --avid_alpha <name>               Raw Avid alpha component SD video input file\n");
    fprintf(stderr, "  --avid_alpha_1080i <name>         Raw Avid alpha component HD 1080i video input file\n");
    fprintf(stderr, "  --avid_alpha_1080p <name>         Raw Avid alpha component HD 1080p video input file\n");
    fprintf(stderr, "  --avid_alpha_720p <name>          Raw Avid alpha component HD 720p video input file\n");
    fprintf(stderr, "  --mpeg2lg <name>                  Raw MPEG-2 Long GOP video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl_1080i <name>    Raw MPEG-2 Long GOP 422P@HL 1080i video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl_1080p <name>    Raw MPEG-2 Long GOP 422P@HL 1080p video input file\n");
    fprintf(stderr, "  --mpeg2lg_422p_hl_720p <name>     Raw MPEG-2 Long GOP 422P@HL 720p video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_1920_1080i <name> Raw MPEG-2 Long GOP MP@HL 1920x1080i video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_1920_1080p <name> Raw MPEG-2 Long GOP MP@HL 1920x1080p video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_1440_1080i <name> Raw MPEG-2 Long GOP MP@HL 1440x1080i video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_1440_1080p <name> Raw MPEG-2 Long GOP MP@HL 1440x1080p video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_hl_720p <name>       Raw MPEG-2 Long GOP MP@HL 720p video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_h14_1080i <name>     Raw MPEG-2 Long GOP MP@H14 1080i video input file\n");
    fprintf(stderr, "  --mpeg2lg_mp_h14_1080p <name>     Raw MPEG-2 Long GOP MP@H14 1080p video input file\n");
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
    fprintf(stderr, "  --vc3_1080i_1244 <name> Raw VC3/DNxHD 1920x1080i 145/120 Mbps input file\n");
    fprintf(stderr, "  --vc3_720p_1250 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps 10bit input file\n");
    fprintf(stderr, "  --vc3_720p_1251 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps input file\n");
    fprintf(stderr, "  --vc3_720p_1252 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps input file\n");
    fprintf(stderr, "  --vc3_1080p_1253 <name> Raw VC3/DNxHD 1920x1080p 45/36 Mbps input file\n");
    fprintf(stderr, "  --vc3_720p_1258 <name>  Raw VC3/DNxHD 1280x720p 45 Mbps input file\n");
    fprintf(stderr, "  --vc3_1080p_1259 <name> Raw VC3/DNxHD 1920x1080p 85 Mbps input file\n");
    fprintf(stderr, "  --vc3_1080i_1260 <name> Raw VC3/DNxHD 1920x1080i 85 Mbps input file\n");
    fprintf(stderr, "  --pcm <name>            Raw PCM audio input file\n");
    fprintf(stderr, "  --wave <name>           Wave PCM audio input file\n");
    fprintf(stderr, "  --anc <name>            Raw ST 436 Ancillary data. Currently requires the --anc-const option\n");
    fprintf(stderr, "  --vbi <name>            Raw ST 436 Vertical Blanking Interval data. Currently requires the --vbi-const option\n");
    fprintf(stderr, "\n\n");
    fprintf(stderr, "Notes:\n");
    fprintf(stderr, " - <umid> format is 64 hexadecimal characters and any '.' and '-' characters are ignored\n");
    fprintf(stderr, " - <uuid> format is 32 hexadecimal characters and any '.' and '-' characters are ignored\n");
    fprintf(stderr, " - <tstamp> format is YYYY-MM-DDThh:mm:ss:qm where qm is in units of 1/250th second\n");
}

int main(int argc, const char** argv)
{
    const char *log_filename = 0;
    LogLevel log_level = INFO_LOG;
    ClipWriterType clip_type = CW_OP1A_CLIP_TYPE;
    ClipSubType clip_sub_type = NO_CLIP_SUB_TYPE;
    bool ard_zdf_hdf_profile = false;
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
    bool partition_interval_set = false;
    const char *shim_name = 0;
    const char *shim_id = 0;
    const char *shim_annot = 0;
    const char *project_name = 0;
    const char *tape_name = 0;
    const char *import_name = 0;
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
    UMID mp_uid;
    bool mp_uid_set = false;
    Timestamp mp_created;
    bool mp_created_set = false;
    UMID psp_uid;
    bool psp_uid_set = false;
    Timestamp psp_created;
    bool psp_created_set = false;
    bool min_part = false;
    bool body_part = false;
    bool clip_wrap = false;
    bool allow_no_avci_head = false;
    bool force_no_avci_head = false;
    bool realtime = false;
    float rt_factor = 1.0;
    bool product_info_set = false;
    string company_name;
    string product_name;
    mxfProductVersion product_version;
    string version_string;
    UUID product_uid;
    bool ps_avcihead = false;
    bool avid_gf = false;
    int64_t avid_gf_duration = -1;
    int64_t regtest_end = -1;
    bool have_anc = false;
    bool have_vbi = false;
    bool zero_mp_track_num = false;
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
                printf("%s\n", get_app_version_info(APP_NAME).c_str());
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
        else if (strcmp(argv[cmdln_index], "--log-level") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_log_level(argv[cmdln_index + 1], &log_level))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
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
            if (!parse_clip_type(argv[cmdln_index + 1], &clip_type, &clip_sub_type))
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
        else if (strcmp(argv[cmdln_index], "--prod-info") == 0)
        {
            if (cmdln_index + 5 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing arguments for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_product_info(&argv[cmdln_index + 1], 5, &company_name, &product_name, &product_version,
                                    &version_string, &product_uid))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid values '%s' etc. for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            product_info_set = true;
            cmdln_index += 5;
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
        else if (strcmp(argv[cmdln_index], "--rt") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%f", &rt_factor) != 1 || rt_factor <= 0.0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            realtime = true;
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
        else if (strcmp(argv[cmdln_index], "--ps-avcihead") == 0)
        {
            ps_avcihead = true;
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
        else if (strcmp(argv[cmdln_index], "--min-part") == 0)
        {
            min_part = true;
        }
        else if (strcmp(argv[cmdln_index], "--body-part") == 0)
        {
            body_part = true;
        }
        else if (strcmp(argv[cmdln_index], "--zero-mp-track-num") == 0)
        {
            zero_mp_track_num = true;
        }
        else if (strcmp(argv[cmdln_index], "--clip-wrap") == 0)
        {
            clip_wrap = true;
        }
        else if (strcmp(argv[cmdln_index], "--ard-zdf-hdf") == 0)
        {
            ard_zdf_hdf_profile = true;
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
        else if (strcmp(argv[cmdln_index], "--import") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            import_name = argv[cmdln_index + 1];
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
        else if (strcmp(argv[cmdln_index], "--mp-uid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_umid(argv[cmdln_index + 1], &mp_uid))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            mp_uid_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mp-created") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdln_index + 1], &mp_created))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            mp_created_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--psp-uid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_umid(argv[cmdln_index + 1], &psp_uid))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            psp_uid_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--psp-created") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdln_index + 1], &psp_created))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            psp_created_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--allow-no-avci-head") == 0)
        {
            allow_no_avci_head = true;
        }
        else if (strcmp(argv[cmdln_index], "--avid-gf") == 0)
        {
            avid_gf = true;
        }
        else if (strcmp(argv[cmdln_index], "--avid-gf-dur") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &avid_gf_duration) != 1 || avid_gf_duration < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            avid_gf = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--force-no-avci-head") == 0)
        {
            force_no_avci_head = true;
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
        else if (strcmp(argv[cmdln_index], "--regtest-end") == 0)
        {
            BMX_REGRESSION_TEST = true;
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &regtest_end) != 1 || regtest_end < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
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
        else if (strcmp(argv[cmdln_index], "--bsar") == 0)
        {
            input.set_bs_aspect_ratio = true;
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
        else if (strcmp(argv[cmdln_index], "--audio-chan") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &value) != 1 ||
                value == 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.channel_count = value;
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
        else if (strcmp(argv[cmdln_index], "--anc-const") == 0)
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
            input.anc_const_size = (uint32_t)(uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--vbi-const") == 0)
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
            input.vbi_const_size = (uint32_t)(uvalue);
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
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &input.ess_max_length) != 1 ||
                input.ess_max_length <= 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--klv") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_klv_opt(argv[cmdln_index + 1], &input.klv_key, &input.klv_track_num))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.parse_klv = true;
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
        else if (strcmp(argv[cmdln_index], "--avci-guess") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_avci_guess(argv[cmdln_index + 1],
                                  &input.avci_guess_interlaced,
                                  &input.avci_guess_progressive))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--fixed-size") == 0)
        {
            input.d10_fixed_frame_size = true;
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = D10_50;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type_group = AVCI_ESSENCE_GROUP;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if ((strcmp(argv[cmdln_index], "--avci100_1080i") == 0) || (strcmp(argv[cmdln_index], "--avci200_1080i") == 0))
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (strcmp(argv[cmdln_index], "--avci100_1080i") == 0)
                input.essence_type = AVCI100_1080I;
            else
                input.essence_type = AVCI200_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if ((strcmp(argv[cmdln_index], "--avci100_1080p") == 0) || (strcmp(argv[cmdln_index], "--avci200_1080p") == 0))
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (strcmp(argv[cmdln_index], "--avci100_1080p") == 0)
                input.essence_type = AVCI100_1080P;
            else
                input.essence_type = AVCI200_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if ((strcmp(argv[cmdln_index], "--avci100_720p") == 0) || (strcmp(argv[cmdln_index], "--avci200_720p") == 0))
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (strcmp(argv[cmdln_index], "--avci100_720p") == 0)
                input.essence_type = AVCI100_720P;
            else
                input.essence_type = AVCI200_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avci50_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = UNC_HD_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--unc_3840") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = UNC_UHD_3840;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avid_alpha") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_ALPHA_SD;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avid_alpha_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_ALPHA_HD_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avid_alpha_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_ALPHA_HD_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avid_alpha_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVID_ALPHA_HD_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MPEG2LG_422P_HL_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_1920_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MPEG2LG_MP_HL_1920_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_1920_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MPEG2LG_MP_HL_1920_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_1440_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MPEG2LG_MP_HL_1440_1080I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_1440_1080p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MPEG2LG_MP_HL_1440_1080P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_hl_720p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VC3_1080I_1243;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080i_1244") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VC3_1080I_1244;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_720p_1250") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VC3_1080P_1253;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_720p_1258") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VC3_720P_1258;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080p_1259") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VC3_1080P_1259;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3_1080i_1260") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VC3_1080I_1260;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--pcm") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
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
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = WAVE_PCM;
            input.is_wave = true;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--anc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (have_anc)
            {
                usage(argv[0]);
                fprintf(stderr, "Multiple '%s' inputs are not permitted\n", argv[cmdln_index]);
                return 1;
            }
            if (input.anc_const_size == 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing or zero '--anc-const' option for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = ANC_DATA;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            have_anc = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vbi") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (have_vbi)
            {
                usage(argv[0]);
                fprintf(stderr, "Multiple '%s' inputs are not permitted\n", argv[cmdln_index]);
                return 1;
            }
            if (input.vbi_const_size == 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing or zero '--vbi-const' option for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VBI_DATA;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            have_vbi = true;
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

    if (clip_type != CW_AVID_CLIP_TYPE)
        allow_no_avci_head = false;
    if (clip_type != CW_AVID_CLIP_TYPE && clip_type != CW_OP1A_CLIP_TYPE)
        force_no_avci_head = false;

    if (!product_info_set) {
        company_name    = get_bmx_company_name();
        product_name    = get_bmx_library_name();
        product_version = get_bmx_mxf_product_version();
        version_string  = get_bmx_mxf_version_string();
        product_uid     = get_bmx_product_uid();
    }

    LOG_LEVEL = log_level;
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
        log_info("%s\n", get_app_version_info(APP_NAME).c_str());


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

            if (input->essence_type == WAVE_PCM) {
                if (input->is_wave) {
                    if (!open_wave_reader(input))
                        throw false;
                    BMX_ASSERT(input->wave_reader->GetNumTracks() > 0);

                    input->wavepcm_track_index = 0;
                    input->sampling_rate = input->wave_reader->GetSamplingRate();
                    input->audio_quant_bits = input->wave_reader->GetQuantizationBits();
                    input->channel_count = input->wave_reader->GetNumTracks();

                    if (input->wave_reader->GetNumTracks() > 1) {
                        vector<RawInput> additional_inputs;
                        uint32_t j;
                        for (j = 1; j < input->wave_reader->GetNumTracks(); j++) {
                            RawInput new_input;
                            copy_input(input, &new_input);
                            new_input.wavepcm_track_index = j;
                            additional_inputs.push_back(new_input);
                        }
                        inputs.insert(inputs.begin() + i + 1, additional_inputs.begin(), additional_inputs.end());

                        i += additional_inputs.size();
                    }
                } else {
                    if (!open_raw_reader(input))
                        throw false;

                    vector<RawInput> additional_inputs;
                    uint32_t j;
                    for (j = 1; j < input->channel_count; j++) {
                        RawInput new_input;
                        copy_input(input, &new_input);
                        new_input.wavepcm_track_index = j;
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
                        case 1244:
                            input->essence_type = VC3_1080I_1244;
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
                        case 1258:
                            input->essence_type = VC3_720P_1258;
                            break;
                        case 1259:
                            input->essence_type = VC3_1080P_1259;
                            break;
                        case 1260:
                            input->essence_type = VC3_1080I_1260;
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
                    if (input->d10_fixed_frame_size)
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
                     input->essence_type == MPEG2LG_MP_HL_1920_1080I ||
                     input->essence_type == MPEG2LG_MP_HL_1920_1080P ||
                     input->essence_type == MPEG2LG_MP_HL_1440_1080I ||
                     input->essence_type == MPEG2LG_MP_HL_1440_1080P ||
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
                            if (mpeg2_parser->GetVerticalSize() != 1080 && mpeg2_parser->GetVerticalSize() != 720) {
                                log_error("Unexpected MPEG-2 Long GOP vertical size %u; expecting 1080 or 720\n",
                                          mpeg2_parser->GetVerticalSize());
                                throw false;
                            }
                            if (mpeg2_parser->GetHorizontalSize() == 1920) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x82:
                                        input->essence_type = MPEG2LG_422P_HL_1080P;
                                        break;
                                    case 0x44:
                                        input->essence_type = MPEG2LG_MP_HL_1920_1080P;
                                        break;
                                    default:
                                        log_error("Unexpected MPEG-2 Long GOP profile and level %u\n",
                                                  mpeg2_parser->GetProfileAndLevel());
                                        throw false;
                                }
                            } else if (mpeg2_parser->GetHorizontalSize() == 1440) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x44:
                                        input->essence_type = MPEG2LG_MP_HL_1440_1080P;
                                        break;
                                    case 0x46:
                                        input->essence_type = MPEG2LG_MP_H14_1080P;
                                        break;
                                    default:
                                        log_error("Unexpected MPEG-2 Long GOP profile and level %u\n",
                                                  mpeg2_parser->GetProfileAndLevel());
                                        throw false;
                                }
                            } else if (mpeg2_parser->GetHorizontalSize() == 1280) {
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
                                log_error("Unexpected MPEG-2 Long GOP horizontal size %u; expected 1920 or 1440 or 1280\n",
                                          mpeg2_parser->GetHorizontalSize());
                                throw false;
                            }
                        } else {
                            if (mpeg2_parser->GetVerticalSize() != 1080) {
                                log_error("Unexpected MPEG-2 Long GOP vertical size %u; expecting 1080 \n",
                                          mpeg2_parser->GetVerticalSize());
                                throw false;
                            }
                            if (mpeg2_parser->GetHorizontalSize() == 1920) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x82:
                                        input->essence_type = MPEG2LG_422P_HL_1080I;
                                        break;
                                    case 0x44:
                                        input->essence_type = MPEG2LG_MP_HL_1920_1080I;
                                        break;
                                    default:
                                        log_error("Unknown MPEG-2 Long GOP profile and level %u\n",
                                                  mpeg2_parser->GetProfileAndLevel());
                                        throw false;
                                }
                            } else if (mpeg2_parser->GetHorizontalSize() == 1440) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x44:
                                        input->essence_type = MPEG2LG_MP_HL_1440_1080I;
                                        break;
                                    case 0x46:
                                        input->essence_type = MPEG2LG_MP_H14_1080I;
                                        break;
                                    default:
                                        log_error("Unknown MPEG-2 Long GOP profile and level %u\n",
                                                  mpeg2_parser->GetProfileAndLevel());
                                        throw false;
                                }
                            } else {
                                log_error("Unexpected MPEG-2 Long GOP horizontal size %u; expected 1920 or 1440\n",
                                          mpeg2_parser->GetHorizontalSize());
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
            else if (input->essence_type_group == AVCI_ESSENCE_GROUP)
            {
                AVCEssenceParser *avc_parser = new AVCEssenceParser();
                input->raw_reader->SetEssenceParser(avc_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    log_error("Failed to read and parse first AVC-I frame to determine essence type\n");
                    throw false;
                }

                avc_parser->ParseFrameInfo(input->raw_reader->GetSampleData(),
                                           input->raw_reader->GetSampleDataSize());
                input->essence_type = avc_parser->GetAVCIEssenceType(input->raw_reader->GetSampleDataSize(),
                                                                     input->avci_guess_interlaced,
                                                                     input->avci_guess_progressive);
                if (input->essence_type == UNKNOWN_ESSENCE_TYPE) {
                    log_error("AVC-I essence type not recognised\n");
                    throw false;
                }

                // re-open the raw reader to use the special AVCI reader
                delete input->raw_reader;
                input->raw_reader = 0;
                if (!open_raw_reader(input))
                    throw false;
            }
        }


        // complete commandline parsing

        start_timecode.Init(frame_rate, false);
        if (start_timecode_str && !parse_timecode(start_timecode_str, frame_rate, &start_timecode)) {
            usage(argv[0]);
            log_error("Invalid value '%s' for option '-t'\n", start_timecode_str);
            throw false;
        }

        if (partition_interval_str) {
            if (!parse_partition_interval(partition_interval_str, frame_rate, &partition_interval))
            {
                usage(argv[0]);
                log_error("Invalid value '%s' for option '--part'\n", partition_interval_str);
                throw false;
            }
            partition_interval_set = true;
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

        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->set_bs_aspect_ratio) {
                if (!(input->essence_type == D10_30 ||
                      input->essence_type == D10_40 ||
                      input->essence_type == D10_50))
                {
                    usage(argv[0]);
                    log_error("Setting aspect ratio in the bitstream is only supported for D-10 essence types\n");
                    throw false;
                }
                input->filter = new MPEG2AspectRatioFilter(input->aspect_ratio);
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
                              clip_type_to_string(clip_type, clip_sub_type).c_str());
                    throw false;
                }
            }
        }


        // create clip

        int flavour = 0;
        if (clip_type == CW_OP1A_CLIP_TYPE) {
            flavour = OP1A_DEFAULT_FLAVOUR;
            if (ard_zdf_hdf_profile) {
                flavour |= OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR;
            } else if (clip_sub_type != AS11_CLIP_SUB_TYPE) {
                if (min_part)
                    flavour |= OP1A_MIN_PARTITIONS_FLAVOUR;
                else if (body_part)
                    flavour |= OP1A_BODY_PARTITIONS_FLAVOUR;
                if (zero_mp_track_num)
                    flavour |= OP1A_ZERO_MP_TRACK_NUMBER_FLAVOUR;
            }
            if (file_md5)
                flavour |= OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= OP1A_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_D10_CLIP_TYPE) {
            flavour = D10_DEFAULT_FLAVOUR;
            // single pass flavours not (yet) supported
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            if (ard_zdf_hdf_profile)
                flavour = RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR;
            if (file_md5)
                flavour |= RDD9_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= RDD9_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_AVID_CLIP_TYPE) {
            flavour = AVID_DEFAULT_FLAVOUR;
            if (avid_gf)
                flavour |= AVID_GROWING_FILE_FLAVOUR;
        }
        DefaultMXFFileFactory file_factory;
        ClipWriter *clip = 0;
        switch (clip_type)
        {
            case CW_AS02_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS02Clip(output_name, true, frame_rate, &file_factory, false);
                break;
            case CW_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewOP1AClip(flavour, file_factory.OpenNew(output_name), frame_rate);
                break;
            case CW_AVID_CLIP_TYPE:
                clip = ClipWriter::OpenNewAvidClip(flavour, frame_rate, &file_factory, false);
                break;
            case CW_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewD10Clip(flavour, file_factory.OpenNew(output_name), frame_rate);
                break;
            case CW_RDD9_CLIP_TYPE:
                clip = ClipWriter::OpenNewRDD9Clip(flavour, file_factory.OpenNew(output_name), frame_rate);
                break;
            case CW_WAVE_CLIP_TYPE:
                clip = ClipWriter::OpenNewWaveClip(WaveFileIO::OpenNew(output_name));
                break;
            case CW_UNKNOWN_CLIP_TYPE:
                BMX_ASSERT(false);
                break;
        }


        // initialize clip properties

        SourcePackage *physical_package = 0;
        vector<pair<mxfUMID, uint32_t> > physical_package_picture_refs;
        vector<pair<mxfUMID, uint32_t> > physical_package_sound_refs;

        clip->SetStartTimecode(start_timecode);
        if (clip_name)
            clip->SetClipName(clip_name);
        else if (clip_sub_type == AS11_CLIP_SUB_TYPE && as11_helper.HaveProgrammeTitle())
            clip->SetClipName(as11_helper.GetProgrammeTitle());
        clip->SetProductInfo(company_name, product_name, product_version, version_string, product_uid);

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
        } else if (clip_type == CW_OP1A_CLIP_TYPE) {
            OP1AFile *op1a_clip = clip->GetOP1AClip();

            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                op1a_clip->ReserveHeaderMetadataSpace(16384); // min is 8192

            if (clip_sub_type != AS11_CLIP_SUB_TYPE)
                op1a_clip->SetClipWrapped(clip_wrap);
            if (partition_interval_set)
                op1a_clip->SetPartitionInterval(partition_interval);
            op1a_clip->SetOutputStartOffset(output_start_offset);
            op1a_clip->SetOutputEndOffset(- output_end_offset);
        } else if (clip_type == CW_AVID_CLIP_TYPE) {
            AvidClip *avid_clip = clip->GetAvidClip();

            if (avid_gf && avid_gf_duration >= 0)
                avid_clip->SetGrowingDuration(avid_gf_duration);

            if (!clip_name)
                avid_clip->SetClipName(output_name);

            if (project_name)
                avid_clip->SetProjectName(project_name);

            for (i = 0; i < locators.size(); i++)
                avid_clip->AddLocator(locators[i].locator);

            map<string, string>::const_iterator iter;
            for (iter = user_comments.begin(); iter != user_comments.end(); iter++)
                avid_clip->SetUserComment(iter->first, iter->second);

            if (mp_uid_set)
                avid_clip->SetMaterialPackageUID(mp_uid);

            if (mp_created_set)
                avid_clip->SetMaterialPackageCreationDate(mp_created);

            if (tape_name || import_name) {
                uint32_t num_picture_tracks = 0;
                uint32_t num_sound_tracks = 0;
                for (i = 0; i < inputs.size(); i++) {
                    if (inputs[i].essence_type == WAVE_PCM)
                        num_sound_tracks++;
                    else
                        num_picture_tracks++;
                }
                if (tape_name) {
                    physical_package = avid_clip->CreateDefaultTapeSource(tape_name,
                                                                          num_picture_tracks, num_sound_tracks);
                } else {
                    URI uri;
                    if (!parse_avid_import_name(import_name, &uri)) {
                        log_error("Failed to parse import name '%s'\n", import_name);
                        throw false;
                    }
                    physical_package = avid_clip->CreateDefaultImportSource(uri.ToString(), uri.GetLastSegment(),
                                                                            num_picture_tracks, num_sound_tracks);
                }
                if (psp_uid_set)
                    physical_package->setPackageUID(psp_uid);
                if (psp_created_set) {
                    physical_package->setPackageCreationDate(psp_created);
                    physical_package->setPackageModifiedDate(psp_created);
                }

                physical_package_picture_refs = avid_clip->GetSourceReferences(physical_package, MXF_PICTURE_DDEF);
                BMX_ASSERT(physical_package_picture_refs.size() == num_picture_tracks);
                physical_package_sound_refs = avid_clip->GetSourceReferences(physical_package, MXF_SOUND_DDEF);
                BMX_ASSERT(physical_package_sound_refs.size() == num_sound_tracks);
            }
        } else if (clip_type == CW_D10_CLIP_TYPE) {
            D10File *d10_clip = clip->GetD10Clip();

            d10_clip->SetMuteSoundFlags(d10_mute_sound_flags);
            d10_clip->SetInvalidSoundFlags(d10_invalid_sound_flags);

            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                d10_clip->ReserveHeaderMetadataSpace(16384); // min is 8192
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            RDD9File *rdd9_clip = clip->GetRDD9Clip();

            if (partition_interval_set)
                rdd9_clip->SetPartitionInterval(partition_interval);
            rdd9_clip->SetOutputStartOffset(output_start_offset);
            rdd9_clip->SetOutputEndOffset(- output_end_offset);
        } else if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();

            if (originator)
                wave_clip->GetBroadcastAudioExtension()->SetOriginator(originator);
        }


        // open raw inputs, create and initialize track properties

        unsigned char avci_header_data[AVCI_HEADER_SIZE];
        map<MXFDataDefEnum, uint32_t> track_count;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];

            // open raw file reader

            if (!input->is_wave && !open_raw_reader(input))
                throw false;

            MXFDataDefEnum data_def = (input->essence_type == WAVE_PCM ? MXF_SOUND_DDEF : MXF_PICTURE_DDEF);


            // create track

            if (clip_type == CW_AVID_CLIP_TYPE) {
                string track_name = create_mxf_track_filename(output_name, track_count[data_def] + 1, data_def);
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

                if (partition_interval_set) {
                    AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(as02_track);
                    if (as02_pict_track)
                        as02_pict_track->SetPartitionInterval(partition_interval);
                }
            } else if (clip_type == CW_AVID_CLIP_TYPE) {
                AvidTrack *avid_track = input->track->GetAvidTrack();

                if (physical_package) {
                    if (data_def == MXF_PICTURE_DDEF) {
                        avid_track->SetSourceRef(physical_package_picture_refs[track_count[data_def]].first,
                                                 physical_package_picture_refs[track_count[data_def]].second);
                    } else if (data_def == MXF_SOUND_DDEF) {
                        avid_track->SetSourceRef(physical_package_sound_refs[track_count[data_def]].first,
                                                 physical_package_sound_refs[track_count[data_def]].second);
                    }
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
                    break;
                case AVCI200_1080I:
                case AVCI200_1080P:
                case AVCI200_720P:
                case AVCI100_1080I:
                case AVCI100_1080P:
                case AVCI100_720P:
                case AVCI50_1080I:
                case AVCI50_1080P:
                case AVCI50_720P:
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    if (force_no_avci_head) {
                        input->track->SetAVCIMode(AVCI_NO_FRAME_HEADER_MODE);
                    } else {
                        if (allow_no_avci_head)
                            input->track->SetAVCIMode(AVCI_NO_OR_ALL_FRAME_HEADER_MODE);
                        else
                            input->track->SetAVCIMode(AVCI_ALL_FRAME_HEADER_MODE);

                        if (ps_avcihead && get_ps_avci_header_data(input->essence_type, frame_rate,
                                                                   avci_header_data, sizeof(avci_header_data)))
                        {
                            input->track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                        }
                        else if (have_avci_header_data(input->essence_type, frame_rate, avci_header_inputs))
                        {
                            if (!read_avci_header_data(input->essence_type, frame_rate, avci_header_inputs,
                                                       avci_header_data, sizeof(avci_header_data)))
                            {
                                log_error("Failed to read AVC-Intra header data from input file for %s\n",
                                          essence_type_to_string(input->essence_type));
                                throw false;
                            }
                            input->track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                        }
                    }
                    break;
                case UNC_SD:
                case UNC_HD_1080I:
                case UNC_HD_1080P:
                case UNC_HD_720P:
                case UNC_UHD_3840:
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
                case AVID_ALPHA_SD:
                case AVID_ALPHA_HD_1080I:
                case AVID_ALPHA_HD_1080P:
                case AVID_ALPHA_HD_720P:
                    input->track->SetAspectRatio(input->aspect_ratio);
                    if (input->afd)
                        input->track->SetAFD(input->afd);
                    if (input->input_height > 0)
                        input->track->SetInputHeight(input->input_height);
                    break;
                case MPEG2LG_422P_HL_1080I:
                case MPEG2LG_422P_HL_1080P:
                case MPEG2LG_422P_HL_720P:
                case MPEG2LG_MP_HL_1920_1080I:
                case MPEG2LG_MP_HL_1920_1080P:
                case MPEG2LG_MP_HL_1440_1080I:
                case MPEG2LG_MP_HL_1440_1080P:
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
                case VC3_1080I_1244:
                case VC3_720P_1250:
                case VC3_720P_1251:
                case VC3_720P_1252:
                case VC3_1080P_1253:
                case VC3_720P_1258:
                case VC3_1080P_1259:
                case VC3_1080I_1260:
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
                case ANC_DATA:
                    input->track->SetConstantDataSize(input->anc_const_size);
                    break;
                case VBI_DATA:
                    input->track->SetConstantDataSize(input->vbi_const_size);
                    break;
                case AVC_HIGH_10_INTRA_UNCS:
                case AVC_HIGH_422_INTRA_UNCS:
                case D10_AES3_PCM:
                case PICTURE_ESSENCE:
                case SOUND_ESSENCE:
                case DATA_ESSENCE:
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
                case AVCI200_1080I:
                case AVCI200_1080P:
                case AVCI200_720P:
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
                case VC3_1080I_1244:
                case VC3_720P_1250:
                case VC3_720P_1251:
                case VC3_720P_1252:
                case VC3_1080P_1253:
                case VC3_720P_1258:
                case VC3_1080P_1259:
                case VC3_1080I_1260:
                case UNC_SD:
                case UNC_HD_1080I:
                case UNC_HD_1080P:
                case UNC_HD_720P:
                case UNC_UHD_3840:
                case AVID_10BIT_UNC_SD:
                case AVID_10BIT_UNC_HD_1080I:
                case AVID_10BIT_UNC_HD_1080P:
                case AVID_10BIT_UNC_HD_720P:
                case AVID_ALPHA_SD:
                case AVID_ALPHA_HD_1080I:
                case AVID_ALPHA_HD_1080P:
                case AVID_ALPHA_HD_720P:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader->GetFixedSampleSize() == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetInputSampleSize());
                    break;
                case D10_30:
                case D10_40:
                case D10_50:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if ((BMX_REGRESSION_TEST || input->d10_fixed_frame_size) &&
                        input->raw_reader->GetFixedSampleSize() == 0)
                    {
                        input->raw_reader->SetFixedSampleSize(input->track->GetInputSampleSize());
                    }
                    break;
                case MPEG2LG_422P_HL_1080I:
                case MPEG2LG_422P_HL_1080P:
                case MPEG2LG_422P_HL_720P:
                case MPEG2LG_MP_HL_1920_1080I:
                case MPEG2LG_MP_HL_1920_1080P:
                case MPEG2LG_MP_HL_1440_1080I:
                case MPEG2LG_MP_HL_1440_1080P:
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
                    BMX_ASSERT(shifted_sample_sequence.size() < BMX_ARRAY_SIZE(input->sample_sequence));
                    memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                           shifted_sample_sequence.size() * sizeof(uint32_t));
                    input->sample_sequence_size = shifted_sample_sequence.size();
                    if (input->raw_reader && input->wavepcm_track_index == 0)
                        input->raw_reader->SetFixedSampleSize(input->track->GetSampleSize() * input->channel_count);
                    break;
                }
                case ANC_DATA:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetFixedSampleSize(input->anc_const_size);
                    break;
                case VBI_DATA:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetFixedSampleSize(input->vbi_const_size);
                    break;
                case AVC_HIGH_10_INTRA_UNCS:
                case AVC_HIGH_422_INTRA_UNCS:
                case D10_AES3_PCM:
                case PICTURE_ESSENCE:
                case SOUND_ESSENCE:
                case DATA_ESSENCE:
                case UNKNOWN_ESSENCE_TYPE:
                    BMX_ASSERT(false);
            }

            track_count[data_def]++;
        }


        // add AS-11 descriptive metadata

        if (clip_sub_type == AS11_CLIP_SUB_TYPE)
            as11_helper.InsertFrameworks(clip);


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


        // realtime wrapping

        uint32_t rt_start = 0;
        if (realtime)
            rt_start = get_tick_count();


        // create clip file(s) and write samples

        clip->PrepareWrite();

        // write samples
        bmx::ByteArray pcm_buffer;
        int64_t total_read = 0;
        while (duration < 0 || total_read < duration) {
            // break if reached end of partial file regression test
            if (regtest_end >= 0 && total_read >= regtest_end)
                break;

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
                    if (input->essence_type == WAVE_PCM && input->channel_count > 1) {
                        pcm_buffer.Allocate(input->raw_reader->GetSampleDataSize() / input->channel_count);
                        deinterleave_audio(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize(),
                                           input->audio_quant_bits, input->channel_count, input->wavepcm_track_index,
                                           pcm_buffer.GetBytes(), pcm_buffer.GetAllocatedSize());
                        pcm_buffer.SetSize(input->raw_reader->GetSampleDataSize() / input->channel_count);
                        write_samples(input, pcm_buffer.GetBytes(), pcm_buffer.GetSize(), num_samples);
                    } else {
                        write_samples(input, input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize(),
                                      num_samples);
                    }
                } else {
                    Frame *frame = input->wave_reader->GetTrack(input->wavepcm_track_index)->GetFrameBuffer()->GetLastFrame(true);
                    BMX_ASSERT(frame);
                    num_samples = frame->num_samples;
                    if (max_samples_per_read > 1 && num_samples > min_num_samples)
                        num_samples = min_num_samples;
                    write_samples(input, (unsigned char*)frame->GetBytes(), frame->GetSize(), num_samples);
                    delete frame;
                }
            }

            if (min_num_samples < max_samples_per_read)
                break;

            if (realtime)
                rt_sleep(rt_factor, rt_start, frame_rate, total_read);
        }


        if (regtest_end < 0) { // only complete if not regression testing partial files

            // complete AS-11 descriptive metadata

            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                as11_helper.Complete();


            // complete writing

            clip->CompleteWrite();

            log_info("Duration: %"PRId64" (%s)\n",
                     clip->GetDuration(),
                     get_generic_duration_string_2(clip->GetDuration(), clip->GetFrameRate()).c_str());


            if (file_md5) {
                if (clip_type == CW_OP1A_CLIP_TYPE) {
                    OP1AFile *op1a_clip = clip->GetOP1AClip();

                    log_info("File MD5: %s\n", op1a_clip->GetMD5DigestStr().c_str());
                } else if (clip_type == CW_RDD9_CLIP_TYPE) {
                    RDD9File *rdd9_clip = clip->GetRDD9Clip();

                    log_info("File MD5: %s\n", rdd9_clip->GetMD5DigestStr().c_str());
                }
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

