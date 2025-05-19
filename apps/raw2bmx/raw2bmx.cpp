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
#define __STDC_LIMIT_MACROS

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <limits.h>

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#include "RawInputTrack.h"
#include "../writers/OutputTrack.h"
#include "../writers/TrackMapper.h"
#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/essence_parser/DVEssenceParser.h>
#include <bmx/essence_parser/MPEG2EssenceParser.h>
#include <bmx/essence_parser/AVCEssenceParser.h>
#include <bmx/essence_parser/AVCIRawEssenceReader.h>
#include <bmx/essence_parser/MJPEGEssenceParser.h>
#include <bmx/essence_parser/RDD36EssenceParser.h>
#include <bmx/essence_parser/J2CEssenceParser.h>
#include <bmx/essence_parser/VC3EssenceParser.h>
#include <bmx/essence_parser/VC2EssenceParser.h>
#include <bmx/essence_parser/JXSEssenceParser.h>
#include <bmx/essence_parser/RawEssenceReader.h>
#include <bmx/essence_parser/FileEssenceSource.h>
#include <bmx/essence_parser/KLVEssenceSource.h>
#include <bmx/essence_parser/KLVEssenceReader.h>
#include <bmx/essence_parser/FilePatternEssenceSource.h>
#include <bmx/essence_parser/D10RawEssenceReader.h>
#include <bmx/essence_parser/MPEG2AspectRatioFilter.h>
#include <bmx/mxf_helper/RDD36MXFDescriptorHelper.h>
#include <bmx/wave/WaveFileIO.h>
#include <bmx/wave/WaveFileChunk.h>
#include <bmx/wave/WaveReader.h>
#include <bmx/wave/WaveCHNA.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/URI.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Version.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/apps/TimedTextManifestParser.h>
#include <bmx/apps/ADMCHNATextFileHelper.h>
#include <bmx/as11/AS11Labels.h>
#include <bmx/as10/AS10ShimNames.h>
#include <bmx/as10/AS10MPEG2Validator.h>
#include <bmx/as10/AS10RDD9Validator.h>
#include <bmx/apps/AS11Helper.h>
#include <bmx/apps/AS10Helper.h>
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
    AVC_ESSENCE_GROUP,
    JXS_ESSENCE_GROUP,
} EssenceTypeGroup;


typedef struct
{
    UL scheme_id;
    const char *lang;
    const char *filename;
} EmbedXMLInfo;

typedef struct
{
    const char *position_str;
    AvidLocator locator;
} LocatorOption;

typedef struct
{
    const char *filename;
    WaveChunkId id;
} WaveChunkData;

struct RawInput
{
    bool disabled;

    EssenceTypeGroup essence_type_group;
    bool avci_guess_interlaced;
    bool avci_guess_progressive;
    EssenceType essence_type;
    bool is_wave;

    const char *filename;
    int64_t file_start_offset;
    int64_t ess_max_length;
    bool parse_klv;
    mxfKey klv_key;
    uint32_t klv_track_num;
    const char *file_pattern;
    bool fill_pattern_gaps;

    int64_t output_start_offset;
    int64_t output_end_offset;

    RawEssenceReader *raw_reader;
    WaveReader *wave_reader;
    KLVEssenceReader *klv_reader;
    uint32_t channel_count;
    TimedTextManifestParser *timed_text_manifest;

    uint32_t sample_sequence[32];
    size_t sample_sequence_size;
    size_t sample_sequence_offset;

    BMX_OPT_PROP_DECL(uint32_t, track_number);

    EssenceFilter *filter;
    bool set_bs_aspect_ratio;

    map<string, WaveChunkRef> *wave_chunk_refs;
    bool no_chna_chunk;

    // picture
    BMX_OPT_PROP_DECL(Rational, aspect_ratio);
    BMX_OPT_PROP_DECL(uint8_t, afd);
    BMX_OPT_PROP_DECL(uint32_t, component_depth);
    uint32_t input_height;
    bool have_avci_header;
    bool d10_fixed_frame_size;
    BMX_OPT_PROP_DECL(MXFSignalStandard, signal_standard);
    BMX_OPT_PROP_DECL(MXFFrameLayout, frame_layout);
    BMX_OPT_PROP_DECL(uint8_t, field_dominance);
    BMX_OPT_PROP_DECL(mxfVideoLineMap, video_line_map);
    BMX_OPT_PROP_DECL(mxfUL, transfer_ch);
    BMX_OPT_PROP_DECL(mxfUL, coding_equations);
    BMX_OPT_PROP_DECL(mxfUL, color_primaries);
    BMX_OPT_PROP_DECL(MXFColorSiting, color_siting);
    BMX_OPT_PROP_DECL(uint32_t, black_ref_level);
    BMX_OPT_PROP_DECL(uint32_t, white_ref_level);
    BMX_OPT_PROP_DECL(uint32_t, color_range);
    BMX_OPT_PROP_DECL(uint32_t, comp_max_ref);
    BMX_OPT_PROP_DECL(uint32_t, comp_min_ref);
    BMX_OPT_PROP_DECL(uint8_t, scan_dir);
    BMX_OPT_PROP_DECL(mxfThreeColorPrimaries, display_primaries);
    BMX_OPT_PROP_DECL(mxfColorPrimary, display_white_point);
    BMX_OPT_PROP_DECL(uint32_t, display_max_luma);
    BMX_OPT_PROP_DECL(uint32_t, display_min_luma);
    int vc2_mode_flags;
    BMX_OPT_PROP_DECL(bool, rdd36_opaque);
    BMX_OPT_PROP_DECL(uint32_t, active_width);
    BMX_OPT_PROP_DECL(uint32_t, active_height);
    BMX_OPT_PROP_DECL(uint32_t, active_x_offset);
    BMX_OPT_PROP_DECL(uint32_t, active_y_offset);
    BMX_OPT_PROP_DECL(int32_t, display_f2_offset);
    BMX_OPT_PROP_DECL(bool, center_cut_4_3);
    BMX_OPT_PROP_DECL(bool, center_cut_14_9);

    // sound
    Rational sampling_rate;
    uint32_t bits_per_sample;
    BMX_OPT_PROP_DECL(bool, locked);
    BMX_OPT_PROP_DECL(int8_t, audio_ref_level);
    BMX_OPT_PROP_DECL(int8_t, dial_norm);
    BMX_OPT_PROP_DECL(mxfRational, ref_image_edit_rate);
    BMX_OPT_PROP_DECL(int8_t, ref_audio_align_level);

    // ANC/VBI
    uint32_t anc_const_size;
    uint32_t vbi_const_size;
};

typedef struct RawInput RawInput;


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


static bool regtest_output_track_map_comp(const TrackMapper::OutputTrackMap &left,
                                          const TrackMapper::OutputTrackMap &right)
{
    return left.data_def < right.data_def;
}

static uint32_t read_samples(RawInput *input, uint32_t max_samples_per_read)
{
    // flush last frame from wave reader buffer
    if (input->wave_reader) {
        uint32_t i;
        for (i = 0; i < input->wave_reader->GetNumTracks(); i++)
            delete input->wave_reader->GetTrack(i)->GetFrameBuffer()->GetLastFrame(true);
    }

    if (max_samples_per_read == 1) {
        uint32_t num_frame_samples    = input->sample_sequence[input->sample_sequence_offset];
        input->sample_sequence_offset = (input->sample_sequence_offset + 1) % input->sample_sequence_size;

        if (input->raw_reader) {
            return (input->raw_reader->ReadSamples(num_frame_samples) == num_frame_samples ? 1 : 0);
        } else if (input->klv_reader) {
            BMX_ASSERT(num_frame_samples == 1);
            return (input->klv_reader->ReadValue() ? 1 : 0);
        } else {
            return (input->wave_reader->Read(num_frame_samples) == num_frame_samples ? 1 : 0);
        }
    } else {
        BMX_ASSERT(input->sample_sequence_size == 1 && input->sample_sequence[0] == 1);
        if (input->raw_reader)
            return input->raw_reader->ReadSamples(max_samples_per_read);
        else if (input->klv_reader)
            return (input->klv_reader->ReadValue() ? 1 : 0);
        else
            return input->wave_reader->Read(max_samples_per_read);
    }
}

static bool open_raw_reader(RawInput *input)
{
    if (input->raw_reader) {
        input->raw_reader->Reset();
        return true;
    }

    EssenceSource *essence_source;

    BMX_ASSERT(input->filename || input->file_pattern);
    if (input->filename) {
        FileEssenceSource *file_source = new FileEssenceSource();
        if (!file_source->Open(input->filename, input->file_start_offset)) {
            log_error("Failed to open input file '%s' at start offset %" PRId64 ": %s\n",
                    input->filename, input->file_start_offset, file_source->GetStrError().c_str());
            delete file_source;
            return false;
        }
        essence_source = file_source;
    } else {
        FilePatternEssenceSource *file_pattern_source = new FilePatternEssenceSource(input->fill_pattern_gaps);
        if (!file_pattern_source->Open(input->file_pattern, input->file_start_offset)) {
            log_error("Failed to open file pattern '%s' at start offset %" PRId64 ": %s\n",
                    input->file_pattern, input->file_start_offset, file_pattern_source->GetStrError().c_str());
            delete file_pattern_source;
            return false;
        }
        essence_source = file_pattern_source;
    }

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
    else if (input->essence_type_group == D10_ESSENCE_GROUP ||
             input->essence_type == D10_30 ||
             input->essence_type == D10_40 ||
             input->essence_type == D10_50)
    {
        input->raw_reader = new D10RawEssenceReader(essence_source);
    }
    else if (input->parse_klv &&
             (input->essence_type == ANC_DATA ||
              input->essence_type == VBI_DATA))
    {
        input->klv_reader = new KLVEssenceReader(dynamic_cast<KLVEssenceSource*>(essence_source));
    }
    else
    {
        input->raw_reader = new RawEssenceReader(essence_source);
    }
    if (input->ess_max_length > 0)
        input->raw_reader->SetMaxReadLength(input->ess_max_length);

    return true;
}

static void open_wave_reader(RawInput *input)
{
    BMX_ASSERT(!input->wave_reader);

    input->wave_reader = WaveReader::Open(WaveFileIO::OpenRead(input->filename), true);
    if (!input->wave_reader)
        BMX_EXCEPTION(("Failed to parse wave file '%s'", input->filename));
}

static void init_input(RawInput *input)
{
    memset(input, 0, sizeof(*input));
    input->timed_text_manifest = new TimedTextManifestParser();
    input->wave_chunk_refs = new map<string, WaveChunkRef>();
    BMX_OPT_PROP_DEFAULT(input->aspect_ratio, ASPECT_RATIO_16_9);
    BMX_OPT_PROP_DEFAULT(input->afd, 0);
    BMX_OPT_PROP_DEFAULT(input->signal_standard, MXF_SIGNAL_STANDARD_NONE);
    BMX_OPT_PROP_DEFAULT(input->frame_layout, MXF_FULL_FRAME);
    BMX_OPT_PROP_DEFAULT(input->field_dominance, 1);
    BMX_OPT_PROP_DEFAULT(input->video_line_map, g_Null_Video_Line_Map);
    BMX_OPT_PROP_DEFAULT(input->transfer_ch, g_Null_UL);
    BMX_OPT_PROP_DEFAULT(input->coding_equations, g_Null_UL);
    BMX_OPT_PROP_DEFAULT(input->color_primaries, g_Null_UL);
    BMX_OPT_PROP_DEFAULT(input->color_siting, MXF_COLOR_SITING_UNKNOWN);
    BMX_OPT_PROP_DEFAULT(input->black_ref_level, 0);
    BMX_OPT_PROP_DEFAULT(input->white_ref_level, 0);
    BMX_OPT_PROP_DEFAULT(input->color_range, 0);
    BMX_OPT_PROP_DEFAULT(input->comp_max_ref, 0);
    BMX_OPT_PROP_DEFAULT(input->comp_min_ref, 0);
    BMX_OPT_PROP_DEFAULT(input->scan_dir, 0);
    BMX_OPT_PROP_DEFAULT(input->display_primaries, g_Null_Three_Color_Primaries);
    BMX_OPT_PROP_DEFAULT(input->display_white_point, g_Null_Color_Primary);
    BMX_OPT_PROP_DEFAULT(input->display_max_luma, 0);
    BMX_OPT_PROP_DEFAULT(input->display_min_luma, 0);
    parse_vc2_mode("1", &input->vc2_mode_flags);
    BMX_OPT_PROP_DEFAULT(input->rdd36_opaque, false);
    BMX_OPT_PROP_DEFAULT(input->active_width, 0);
    BMX_OPT_PROP_DEFAULT(input->active_height, 0);
    BMX_OPT_PROP_DEFAULT(input->active_x_offset, 0);
    BMX_OPT_PROP_DEFAULT(input->active_y_offset, 0);
    BMX_OPT_PROP_DEFAULT(input->display_f2_offset, 0);
    BMX_OPT_PROP_DEFAULT(input->center_cut_4_3, false);
    BMX_OPT_PROP_DEFAULT(input->center_cut_14_9, false);
    input->sampling_rate = DEFAULT_SAMPLING_RATE;
    BMX_OPT_PROP_DEFAULT(input->component_depth, 8);
    input->bits_per_sample = 16;
    input->channel_count = 1;
    BMX_OPT_PROP_DEFAULT(input->ref_image_edit_rate, g_Null_Rational);
    BMX_OPT_PROP_DEFAULT(input->ref_audio_align_level, 0);
}

static void clear_input(RawInput *input)
{
    delete input->raw_reader;
    delete input->wave_reader;
    delete input->klv_reader;
    delete input->filter;
    delete input->timed_text_manifest;
    delete input->wave_chunk_refs;
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

static void usage_ref(const char *cmd)
{
    fprintf(stderr, "%s\n", get_app_version_info(APP_NAME).c_str());
    fprintf(stderr, "Run '%s -h' to show detailed commandline usage.\n", strip_path(cmd).c_str());
    fprintf(stderr, "\n");
}

static void usage(const char *cmd)
{
    printf("%s\n", get_app_version_info(APP_NAME).c_str());
    printf("Create MXF files from raw essence files\n");
    printf("\n");
    printf("Usage: %s <<Options>> [<<Input Options>> <Input>]+\n", strip_path(cmd).c_str());
    printf("Options (* means option is required):\n");
    printf("  -h | --help             Show usage and exit\n");
    printf("  -v | --version          Print version info\n");
    printf("  -l <file>               Log filename. Default log to stderr/stdout\n");
    printf(" --log-level <level>      Set the log level. 0=debug, 1=info, 2=warning, 3=error. Default is 1\n");
    printf("  -t <type>               Clip type: as02, as11op1a, as11d10, op1a, avid, d10, rdd9, as10, wave, imf. Default is op1a\n");
    printf("                          Note that an 'op1a' or 'as11op1a' output file type could be signalled as other operational patterns if there is a Timed Text track\n");
    printf("* -o <name>               as02: <name> is a bundle name\n");
    printf("                          as11op1a/as11d10/op1a/d10/rdd9/as10/wave: <name> is a filename or filename pattern (see Notes at the end)\n");
    printf("                          avid: <name> is a filename prefix\n");
    printf("  --ess-type-names <names>  A comma separated list of 4 names for video, audio, data or mixed essence types\n");
    printf("                            The names can be used to replace {type} in output filename patterns\n");
    printf("                            The default names are: video,audio,data,mixed\n");
    printf("  --prod-info <cname>\n");
    printf("              <pname>\n");
    printf("              <ver>\n");
    printf("              <verstr>\n");
    printf("              <uid>\n");
    printf("                          Set the product info in the MXF Identification set\n");
    printf("                          <cname> is a string and is the Company Name property\n");
    printf("                          <pname> is a string and is the Product Name property\n");
    printf("                          <ver> has format '<major>.<minor>.<patch>.<build>.<release>' and is the Product Version property. Set to '0.0.0.0.0' to omit it\n");
    printf("                          <verstr> is a string and is the Version String property\n");
    printf("                          <uid> is a UUID (see Notes at the end) and is the Product UID property\n");
    printf("  --create-date <tstamp>  Set the creation date in the MXF Identification set. Default is 'now'\n");
    printf("  -f <rate>               Set the frame rate, overriding any frame rate parsed from the essence data\n");
    printf("                          The <rate> is either 'num', 'num'/'den', 23976 (=24000/1001), 2997 (=30000/1001) or 5994 (=60000/1001)\n");
    printf("  --dflt-rate <rate>      Set the default frame rate which is used when no rate is provided by the essence data. Without this option the default is 25\n");
    printf("                          The <rate> is either 'num', 'num'/'den', 23976 (=24000/1001), 2997 (=30000/1001) or 5994 (=60000/1001)\n");
    printf("  -y <hh:mm:sscff>        Start timecode. Default 00:00:00:00\n");
    printf("                          The c character in the pattern should be ':' for non-drop frame; any other character indicates drop frame\n");
    printf("  --clip <name>           Set the clip name\n");
    printf("  --dur <frame>           Set the duration in frames in frame rate units. Default is minimum input duration\n");
    printf("  --rt <factor>           Wrap at realtime rate x <factor>, where <factor> is a floating point value\n");
    printf("                          <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
    printf("  --avcihead <format> <file> <offset>\n");
    printf("                          Default AVC-Intra sequence header data (512 bytes) to use when the input file does not have it\n");
    printf("                          <format> is a comma separated list of one or more of the following integer values:\n");
    size_t i;
    for (i = 0; i < get_num_avci_header_formats(); i++)
        printf("                              %2" PRIszt ": %s\n", i, get_avci_header_format_string(i));
    printf("                          or set <format> to 'all' for all formats listed above\n");
    printf("                          The 512 bytes are extracted from <file> starting at <offset> bytes\n");
    printf("                          and incrementing 512 bytes for each format in the list\n");
    printf("  --ps-avcihead           Panasonic AVC-Intra sequence header data for Panasonic-compatible files that don't include the header data\n");
    printf("                          These formats are supported:\n");
    for (i = 0; i < get_num_ps_avci_header_formats(); i++) {
        if (i == 0)
            printf("                              ");
        else if (i % 4 == 0)
            printf(",\n                              ");
        else
            printf(", ");
        printf("%s", get_ps_avci_header_format_string(i));
    }
    printf("\n");
    printf("  --track-map <expr>      Map input audio channels to output tracks\n");
    printf("                          The default is 'mono', except if --clip-wrap option is set for op1a it is 'singlemca'\n");
    printf("                          See below for details of the <expr> format\n");
    printf("  --dump-track-map        Dump the output audio track map to stderr.\n");
    printf("                          The dumps consists of a list output tracks, where each output track channel\n");
    printf("                          is shown as '<output track channel> <- <input channel>\n");
    printf("  --dump-track-map-exit   Same as --dump-track-map, but exit immediately afterwards\n");
    printf("\n");
    printf("  as11op1a/as11d10/as11rdd9/op1a/rdd9/d10:\n");
    printf("    --head-fill <bytes>     Reserve minimum <bytes> at the end of the header metadata using a KLV Fill\n");
    printf("                            Add a 'K' suffix for kibibytes and 'M' for mibibytes\n");
    printf("\n");
    printf("  as02:\n");
    printf("    --mic-type <type>       Media integrity check type: 'md5' or 'none'. Default 'md5'\n");
    printf("    --mic-file              Calculate checksum for entire essence component file. Default is essence only\n");
    printf("    --shim-name <name>      Set ShimName element value in shim.xml file to <name>. Default is '%s'\n", DEFAULT_SHIM_NAME);
    printf("    --shim-id <id>          Set ShimID element value in shim.xml file to <id>. Default is '%s'\n", DEFAULT_SHIM_ID);
    printf("    --shim-annot <str>      Set AnnotationText element value in shim.xml file to <str>. Default is '%s'\n", DEFAULT_SHIM_ANNOTATION);
    printf("\n");
    printf("  as02/as11op1a/op1a/rdd9/as10:\n");
    printf("    --part <interval>       Video essence partition interval in frames, or (floating point) seconds with 's' suffix. Default single partition\n");
    printf("  rdd9:\n");
    printf("    --fixed-part <interval> Force each partition to have the exact same partition interval in frames, except the last partition\n");
    printf("                            New partitions are started if the frame count has been reached, even if the next partition does not begin with the start of a GOP\n");
    printf("\n");
    printf("  as11op1a/as11d10:\n");
    printf("    --dm <fwork> <name> <value>    Set descriptive framework property. <fwork> is 'as11' or 'dpp'\n");
    printf("    --dm-file <fwork> <name>       Parse and set descriptive framework properties from text file <name>. <fwork> is 'as11' or 'dpp'\n");
    printf("    --seg <name>                   Parse and set segmentation data from text file <name>\n");
    printf("    --spec-id <id>                 Set the AS-11 specification identifier labels associated with <id>\n");
    printf("                                   The <id> is one of the following:\n");
    printf("                                       as11-x1 : AMWA AS-11 X1, delivery of finished UHD programs to Digital Production Partnership (DPP) broadcasters\n");
    printf("                                       as11-x2 : AMWA AS-11 X2, delivery of finished HD AVC Intra programs to a broadcaster or publisher\n");
    printf("                                       as11-x3 : AMWA AS-11 X3, delivery of finished HD AVC Long GOP programs to a broadcaster or publisher\n");
    printf("                                       as11-x4 : AMWA AS-11 X4, delivery of finished HD AVC Long GOP programs to a broadcaster or publisher\n");
    printf("                                       as11-x5 : AMWA AS-11 X5, delivery of finished UHD TV Commericals and Promotions to UK Digital Production Partnership (DPP) broadcasters\n");
    printf("                                       as11-x6 : AMWA AS-11 X6, delivery of finished HD TV Commercials and Promotions to UK Digital Production Partnership (DPP) broadcasters\n");
    printf("                                       as11-x7 : AMWA AS-11 X7, delivery of finished SD D10 programs to a broadcaster or publisher\n");
    printf("                                       as11-x8 : AMWA AS-11 X8, delivery of finished HD (MPEG-2) programs to North American Broadcasters Association (NABA) broadcasters\n");
    printf("                                       as11-x9 : AMWA AS-11 X9, delivery of finished HD TV Programmes (AVC) to North American Broadcasters Association (NABA) broadcasters\n");
    printf("\n");
    printf("  as11op1a/op1a/rdd9/as10:\n");
    printf("    --out-start <offset>    Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    printf("    --out-end <offset>      Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    printf("\n");
    printf("  as11op1a/as11d10/op1a/d10/rdd9/as10:\n");
    printf("    --seq-off <value>       Sound sample sequence offset. Default 0 for as11d10/d10 and not set (0) for as11op1a/op1a\n");
    printf("\n");
    printf("  as11op1a/op1a/rdd9/as10:\n");
    printf("    --single-pass           Write file in a single pass\n");
    printf("                            The header and body partitions will be incomplete\n");
    printf("    --file-md5              Calculate an MD5 checksum of the file. This requires writing in a single pass (--single-pass is assumed)\n");
    printf("\n");
    printf("  op1a:\n");
    printf("    --no-tc-track           Don't create a timecode track in either the material or file source package\n");
    printf("    --min-part              Only use a header and footer MXF file partition. Use this for applications that don't support\n");
    printf("                            separate partitions for header metadata, index tables, essence container data and footer\n");
    printf("    --body-part             Create separate body partitions for essence data\n");
    printf("                            and don't create separate body partitions for index table segments\n");
    printf("    --clip-wrap             Use clip wrapping for a single sound track\n");
    printf("    --mp-track-num          Use the material package track number property to define a track order. By default the track number is set to 0\n");
    printf("    --aes-3                 Use AES-3 audio mapping\n");
    printf("    --kag-size-512          Set KAG size to 512, instead of 1\n");
    printf("    --primary-package       Set the header metadata set primary package property to the top-level file source package\n");
    printf("    --index-follows         The index partition follows the essence partition, even when it is CBE essence\n");
    printf("    --st379-2               Add ContainerConstraintsSubDescriptor to signal compliance with ST 379-2, MXF Constrained Generic Container\n");
    printf("                            The sub-descriptor will be added anyway if there is RDD 36 video present\n");
    printf("\n");
    printf("  op1a/rdd9/d10:\n");
    printf("    --xml-scheme-id <id>    Set the XML payload scheme identifier associated with the following --embed-xml option.\n");
    printf("                            The <id> is one of the following:\n");
    printf("                                * a SMPTE UL, formatted as a 'urn:smpte:ul:...',\n");
    printf("                                * a UUID, formatted as a 'urn:uuid:...'or as 32 hexadecimal characters using a '.' or '-' seperator,\n");
    printf("                                * 'as11', which corresponds to urn:smpte:ul:060e2b34.04010101.0d010801.04010000\n");
    printf("                            A default BMX scheme identifier is used if this option is not provided\n");
    printf("    --xml-lang <tag>        Set the RFC 5646 language tag associated with the the following --embed-xml option.\n");
    printf("                            Defaults to the xml:lang attribute in the root element or empty string if not present\n");
    printf("    --embed-xml <filename>  Embed the XML from <filename> using the approach specified in SMPTE RP 2057\n");
    printf("                            If the XML size is less than 64KB and uses UTF-8 or UTF-16 encoding (declared in\n");
    printf("                            the XML prolog) then the XML data is included in the header metadata. Otherwise\n");
    printf("                            a Generic Stream partition is used to hold the XML data.\n");
    printf("\n");
    printf("  op1a/rdd9:\n");
    printf("    --ard-zdf-hdf           Use the ARD ZDF HDF profile\n");
    printf("    --repeat-index          Repeat the index table segments in the footer partition\n");
    printf("\n");
    printf("  op1a:\n");
    printf("    --ard-zdf-xdf           Use the ARD ZDF XDF profile\n");
    printf("\n");
    printf("  as11d10/d10:\n");
    printf("    --d10-mute <flags>      Indicate using a string of 8 '0' or '1' which sound channels should be muted. The lsb is the rightmost digit\n");
    printf("    --d10-invalid <flags>   Indicate using a string of 8 '0' or '1' which sound channels should be flagged invalid. The lsb is the rightmost digit\n");
    printf("\n");
    printf("  as10:\n");
    printf("    --shim-name <name>      Shim name for AS10 (used for setting 'ShimName' metadata and setting video/sound parameters' checks)\n");
    printf("                            list of known shims: %s\n", get_as10_shim_names().c_str());
    printf("    --dm-file as10 <name>   Parse and set descriptive framework properties from text file <name>\n");
    printf("                            N.B. 'ShimName' is the only mandatary property of AS10 metadata set\n");
    printf("    --dm as10 <name> <value>    Set descriptive framework property\n");
    printf("    --pass-dm               Copy descriptive metadata from the input file. The metadata can be overidden by other options\n");
    printf("    --mpeg-checks [<name>]  Enable AS-10 compliancy checks. The file <name> is optional and contains expected descriptor values\n");
    printf("    --loose-checks          Don't stop processing on detected compliancy violations\n");
    printf("    --print-checks          Print default values of mpeg descriptors and report on descriptors either found in mpeg headers or copied from mxf headers\n");
    printf("    --max-same-warnings <value>  Max same violations warnings logged, default 3\n");
    printf("\n");
    printf("  as11op1a/as11d10/op1a/d10/rdd9/as10:\n");
    printf("    --mp-uid <umid>         Set the Material Package UID. Autogenerated by default\n");
    printf("    --fp-uid <umid>         Set the File Package UID. Autogenerated by default\n");
    printf("\n");
    printf("  avid:\n");
    printf("    --project <name>        Set the Avid project name\n");
    printf("    --tape <name>           Source tape name\n");
    printf("    --import <name>         Source import name. <name> is one of the following:\n");
    printf("                              - a file URL starting with 'file://'\n");
    printf("                              - an absolute Windows (starts with '[A-Z]:') or *nix (starts with '/') filename\n");
    printf("                              - a relative filename, which will be converted to an absolute filename\n");
    printf("    --aux <hh:mm:sscff>     Set up to 5 auxiliary start timecodes. Multiple timecodes are separated by commas e.g. --aux 15:02:15:23,09:37:08:10\n");
    printf("                            The c character in the pattern should be ':' for non-drop frame; any other character indicates drop frame\n");
    printf("    --comment <string>      Add 'Comments' user comment to the MaterialPackage\n");
    printf("    --desc <string>         Add 'Descript' user comment to the MaterialPackage\n");
    printf("    --tag <name> <value>    Add <name> user comment to the MaterialPackage. Option can be used multiple times\n");
    printf("    --locator <position> <comment> <color>\n");
    printf("                            Add locator at <position> (in frame rate units) with <comment> and <color>\n");
    printf("                            <position> format is o?hh:mm:sscff, where the optional 'o' indicates it is an offset\n");
    printf("    --umid-type <type>      Set the UMID type that is generated for the Package UID properties.\n");
    printf("                            The default <type> is 'aafsdk'.\n");
    printf("                            The <type> is one of the following:\n");
    printf("                              uuid       : UUID generation method\n");
    printf("                              aafsdk     : same method as implemented in the AAF SDK\n");
    printf("                                           This type is required to be compatible with some older Avid product versions\n");
    printf("                                           Note: this is not guaranteed to create a unique UMID when used in multiple processes\n");
    printf("                              old-aafsdk : same method as implemented in revision 1.47 of AAF/ref-impl/src/impl/AAFUtils.c in the AAF SDK\n");
    printf("                                           Note: this is not guaranteed to create a unique UMID when used in multiple processes\n");
    printf("    --mp-uid <umid>         Set the Material Package UID. Autogenerated by default\n");
    printf("    --mp-created <tstamp>   Set the Material Package creation date. Default is 'now'\n");
    printf("    --psp-uid <umid>        Set the tape/import Source Package UID. Autogenerated by default\n");
    printf("    --psp-created <tstamp>  Set the tape/import Source Package creation date. Default is 'now'\n");
    printf("    --allow-no-avci-head    Allow inputs with no AVCI header (512 bytes, sequence and picture parameter sets)\n");
    printf("    --avid-gf               Use the Avid growing file flavour\n");
    printf("    --avid-gf-dur <dur>     Set the duration which should be shown whilst the file is growing\n");
    printf("                            Avid will show 'Capture in Progress' when this option is used\n");
    printf("\n");
    printf("  op1a/avid:\n");
    printf("    --force-no-avci-head    Strip AVCI header (512 bytes, sequence and picture parameter sets) if present\n");
    printf("\n");
    printf("  wave:\n");
    printf("    --orig <name>                      Set originator in the output Wave bext chunk. Default '%s'\n", DEFAULT_BEXT_ORIGINATOR);
    printf("    --wave-chunk-data <file> <id>      Add a chunk to the output Wave file that has chunk data (not including id and size) from <file> and chunk <id>\n");
    printf("                                       This chunk will override any non-builtin and <chna> chunk originating from the input Wave files\n");
    printf("    --chna-audio-ids <file>            Add a <chna> chunk to the output Wave file which is defined in the text <file>\n");
    printf("                                       This chunk will override any <chna> chunk originating from the input Wave files\n");
    printf("\n");
    printf("  as02/op1a/as11op1a:\n");
    printf("    --use-avc-subdesc       Use the AVC sub-descriptor rather than the MPEG video descriptor for AVC-Intra tracks\n");
    printf("\n");
    printf("  op1a/as11op1a/rdd9:\n");
    printf("    --audio-layout <label>  Set the Wave essence descriptor channel assignment label which identifies the audio layout mode in operation\n");
    printf("                            The <label> is one of the following:\n");
    printf("                                * a SMPTE UL, formatted as a 'urn:smpte:ul:...',\n");
    printf("                                * 'as11-mode-0', which corresponds to urn:smpte:ul:060e2b34.04010101.0d010801.02010000,\n");
    printf("                                * 'as11-mode-1', which corresponds to urn:smpte:ul:060e2b34.04010101.0d010801.02020000,\n");
    printf("                                * 'as11-mode-2', which corresponds to urn:smpte:ul:060e2b34.04010101.0d010801.02030000\n");
    printf("                                * 'imf', which corresponds to urn:smpte:ul:060e2b34.0401010d.04020210.04010000\n");
    printf("                                * 'adm', which corresponds to urn:smpte:ul:060e2b34.0401010d.04020210.05010000\n");
    printf("    --track-mca-labels <scheme> <file>  Insert audio labels defined in <file>. The 'as11' <scheme> will add an override and otherwise <scheme> is ignored\n");
    printf("                                        The format of <file> is described in bmx/docs/mca_labels_format.md\n");
    printf("                                        All tag symbols registered in the bmx code are available for use\n");
    printf("                                        The 'as11' <scheme> will change the label associated with the 'chVIN' tag symbol to use the 'Visually Impaired Narrative' tag name, i.e. without a '-'\n");
    printf("\n");
    printf("\n");
    printf("Input Options (must precede the input to which it applies):\n");
    printf("  -a <n:d>                Image aspect ratio. Default parsed from essence or 16:9\n");
    printf("  --bsar                  Set image aspect ratio in video bitstream. Currently supports D-10 essence types only\n");
    printf("  --afd <value>           Active Format Descriptor 4-bit code from table 1 in SMPTE ST 2016-1. Default not set\n");
    printf("  -c <depth>              Component depth for uncompressed/DV100/RDD-36 video. Either 8 or 10. Default parsed, 8 for uncompressed/DV100 and 10 for RDD-36\n");
    printf("  --height <value>        Height of input uncompressed video data. Default is the production aperture height, except for PAL (592) and NTSC (496)\n");
    printf("  --signal-std  <value>   Set the video signal standard. The <value> is one of the following:\n");
    printf("                              'none', 'bt601', 'bt1358', 'st347', 'st274', 'st296', 'st349', 'st428'\n");
    printf("  --frame-layout <value>  Set the video frame layout. The <value> is one of the following:\n");
    printf("                              'fullframe', 'separatefield', 'singlefield', 'mixedfield', 'segmentedframe'\n");
    printf("  --field-dom <value>     Set which field is first in temporal order. The <value> is 1 or 2\n");
    printf("  --video-line-map <value>  Override or set the video line map. The <value> is 2 line numbers separated by a comma\n");
    printf("  --transfer-ch <value>   Set the transfer characteristic label\n");
    printf("                          The <value> is a SMPTE UL, formatted as a 'urn:smpte:ul:...' or one of the following:\n");
    printf("                              'bt470', 'bt709', 'st240', 'st274', 'bt1361', 'linear', 'dcdm',\n");
    printf("                              'iec61966', 'bt2020', 'st2084', 'hlg'\n");
    printf("  --coding-eq <value>     Set the coding equations label\n");
    printf("                          The <value> is a SMPTE UL, formatted as a 'urn:smpte:ul:...' or one of the following:\n");
    printf("                              'bt601', 'bt709', 'st240', 'ycgco', 'gbr', 'bt2020'\n");
    printf("  --color-prim <value>    Set the color primaries label\n");
    printf("                          The <value> is a SMPTE UL, formatted as a 'urn:smpte:ul:...' or one of the following:\n");
    printf("                              'st170', 'bt470', 'bt709', 'bt2020', 'dcdm', 'p3'\n");
    printf("  --color-siting <value>  Set the color siting. The <value> is one of the following:\n");
    printf("                              'cositing', 'horizmp', '3tap', 'quincunx', 'bt601', 'linealt', 'vertmp', 'unknown'\n");
    printf("                              (Note that 'bt601' is deprecated in SMPTE ST 377-1. Use 'cositing' instead)\n");
    printf("  --black-level <value>   Set the CDCI black reference level\n");
    printf("  --white-level <value>   Set the CDCI white reference level\n");
    printf("  --color-range <value>   Set the CDCI color range\n");
    printf("  --comp-max-ref <value>  Set the RGBA component maximum reference level\n");
    printf("  --comp-min-ref <value>  Set the RGBA component minimum reference level\n");
    printf("  --scan-dir <value>      Set the RGBA scanning direction\n");
    printf("  --display-primaries <value>    Set the mastering display primaries.\n");
    printf("                                 The <value> is an array of 6 unsigned integers separated by a ','.\n");
    printf("  --display-white-point <value>  Set the mastering display white point chromaticity.\n");
    printf("                                 The <value> is an array of 2 unsigned integers separated by a ','.\n");
    printf("  --display-max-luma <value>     Set the mastering display maximum luminance.\n");
    printf("  --display-min-luma <value>     Set the mastering display minimum luminance.\n");
    printf("  --rdd36-opaque          Treat RDD-36 4444 or 4444 XQ as opaque by omitting the Alpha Sample Depth property\n");
    printf("  --active-width          Set the Active Width of the active area rectangle\n");
    printf("  --active-height         Set the Active Height of the active area rectangle\n");
    printf("  --active-x-offset       Set the Active X Offset of the active area rectangle\n");
    printf("  --active-y-offset       Set the Active Y Offset of the active area rectangle\n");
    printf("  --display-f2-offset     Set the default Display F2 Offset if none is extracted from the essence\n");
    printf("  --center-cut-4-3        Add the Alternative Center Cut 4:3\n");
    printf("  --center-cut-14-9       Add the Alternative Center Cut 14:9\n");
    printf("  -s <bps>                Audio sampling rate numerator for raw pcm. Default %d\n", DEFAULT_SAMPLING_RATE.numerator);
    printf("  -q <bps>                Audio quantization bits per sample for raw pcm. Either 16 or 24. Default 16\n");
    printf("  --audio-chan <count>    Audio channel count for raw pcm. Default 1\n");
    printf("  --locked <bool>         Indicate whether the number of audio samples is locked to the video. Either true or false. Default not set\n");
    printf("  --audio-ref <level>     Audio reference level, number of dBm for 0VU. Default not set\n");
    printf("  --dial-norm <value>     Gain to be applied to normalize perceived loudness of the clip. Default not set\n");
    printf("  --ref-image-edit-rate <rate>     Override or set the Reference Image Edit Rate\n");
    printf("                                   The <rate> is either 'num', 'num'/'den', 23976 (=24000/1001), 2997 (=30000/1001) or 5994 (=60000/1001)\n");
    printf("  --ref-audio-align-level <value>  Override or set the Reference Audio Alignment Level\n");
    printf("  --anc-const <size>      Set the constant ANC data frame <size>\n");
    printf("  --vbi-const <size>      Set the constant VBI data frame <size>\n");
    printf("  --off <bytes>           Skip <bytes> at the start of the next input/track's file\n");
    printf("  --maxlen <bytes>        Maximum number of essence data bytes to read from next input/track's file\n");
    printf("  --klv <key>             Essence data is read from the KLV data in the next input/track's file\n");
    printf("                          <key> should have one of the following values:\n");
    printf("                            - 's', which means the first 16 bytes, at file position 0 or --off byte offset, are taken to be the Key\n");
    printf("                            - optional '0x' followed by 8 hexadecimal characters which represents the 4-byte track number part of a generic container essence Key\n");
    printf("                            - 32 hexadecimal characters representing a 16-byte Key\n");
    printf("  --fill-pattern-gaps     Fill gaps in a numbered sequence pattern of raw files by repeating the contents of the file at the start of a gap\n");
    printf("  --track-num <num>       Set the output track number. Default track number equals last track number of same picture/sound type + 1\n");
    printf("                          For as11d10/d10 the track number must be > 0 and <= 8 because the AES-3 channel index equals track number - 1\n");
    printf("  --avci-guess <i/p>      Guess interlaced ('i') or progressive ('p') AVC-Intra when using the --avci option with 1080p25/i50 or 1080p30/i60\n");
    printf("                          The default guess uses the H.264 frame_mbs_only_flag field\n");
    printf("  --fixed-size            Set to indicate that the d10 frames have a fixed size and therefore do not need to be parsed after the first frame\n");
    printf("  --vc2-mode <mode>       Set the mode that determines how the VC-2 data is wrapped\n");
    printf("                          <mode> is one of the following integer values:\n");
    printf("                            0: Passthrough input, but add a sequence header if not present, remove duplicate/redundant sequence headers\n");
    printf("                               and fix any incorrect parse info offsets and picture numbers\n");
    printf("                            1: (default) Same as 0, but remove auxiliary and padding data units and complete the sequence in each frame\n");
    printf("  --wave-chunks <ids>                List of non-builtin WAVE chunk IDs to transfer across from the input\n");
    printf("                                     The <ids> is a comma separated list of chunk IDs. Spaces are automatically appended to IDs if they are less than 4 letters\n");
    printf("                                     The built-in WAVE chunk IDs are: %s\n", WaveReader::GetBuiltinChunkListString().c_str());
    printf("  --adm-wave-chunk <id>[,<label>]*   Identifies a non-builtin ADM WAVE chunk to transfer across from the input\n");
    printf("                                     ADM Soundfield Groups can be reference this chunk\n");
    printf("                                     A comma separated list of profile and level labels can be provided as well. A <label> can be:\n");
    printf("                                       * a SMPTE UL, formatted as a 'urn:smpte:ul:...',\n");
    printf("                                       * 'adm_itu2076', which corresponds to urn:smpte:ul:060e2b34.0401010d.04020211.01010000\n");
    printf("  --no-chna-chunk                    Don't transfer the ADM <chna> chunk from the input\n");
    printf("\n");
    printf("  as02:\n");
    printf("    --trk-out-start <offset>   Offset to start of first output frame, eg. pre-charge in MPEG-2 Long GOP\n");
    printf("    --trk-out-end <offset>     Offset (positive value) from last frame to last output frame, eg. rollout in MPEG-2 Long GOP\n");
    printf("\n");
    printf("\n");
    printf("Inputs:\n");
    printf("  --dv <name>             Raw DV video input file\n");
    printf("  --iecdv25 <name>        Raw IEC DV25 video input file\n");
    printf("  --dvbased25 <name>      Raw DV-Based DV25 video input file\n");
    printf("  --dv50 <name>           Raw DV50 video input file\n");
    printf("  --dv100_1080i <name>    Raw DV100 1080i video input file\n");
    printf("  --dv100_1080p <name>    Raw DV100 1080p video input file\n");
    printf("  --dv100_720p <name>     Raw DV100 720p video input file\n");
    printf("  --d10 <name>            Raw D10 video input file\n");
    printf("  --d10_30 <name>         Raw D10 30Mbps video input file\n");
    printf("  --d10_40 <name>         Raw D10 40Mbps video input file\n");
    printf("  --d10_50 <name>         Raw D10 50Mbps video input file\n");
    printf("  --avci <name>           Raw AVC-Intra video input file. See also --avci-guess option\n");
    printf("  --avci200_1080i <name>  Raw AVC-Intra 200 1080i video input file\n");
    printf("  --avci200_1080p <name>  Raw AVC-Intra 200 1080p video input file\n");
    printf("  --avci200_720p <name>   Raw AVC-Intra 200 720p video input file\n");
    printf("  --avci100_1080i <name>  Raw AVC-Intra 100 1080i video input file\n");
    printf("  --avci100_1080p <name>  Raw AVC-Intra 100 1080p video input file\n");
    printf("  --avci100_720p <name>   Raw AVC-Intra 100 720p video input file\n");
    printf("  --avci50_1080i <name>   Raw AVC-Intra 50 1080i video input file\n");
    printf("  --avci50_1080p <name>   Raw AVC-Intra 50 1080p video input file\n");
    printf("  --avci50_720p <name>    Raw AVC-Intra 50 720p video input file\n");
    printf("  --avc <name>                 Raw AVC video input file\n");
    printf("  --avc_baseline <name>        Raw AVC Baseline profile video input file\n");
    printf("  --avc_constr_baseline <name> Raw AVC Constrained Baseline profile video input file\n");
    printf("  --avc_main <name>            Raw AVC Main profile video input file\n");
    printf("  --avc_extended <name>        Raw AVC Extended profile video input file\n");
    printf("  --avc_high <name>            Raw AVC High profile video input file\n");
    printf("  --avc_high_10 <name>         Raw AVC High 10 profile video input file\n");
    printf("  --avc_high_422 <name>        Raw AVC High 422 profile video input file\n");
    printf("  --avc_high_444 <name>        Raw AVC High 444 profile video input file\n");
    printf("  --avc_high_10_intra <name>   Raw AVC High 10 Intra profile video input file\n");
    printf("  --avc_high_422_intra <name>  Raw AVC High 422 Intra profile video input file\n");
    printf("  --avc_high_444_intra <name>  Raw AVC High 444 Intra profile video input file\n");
    printf("  --avc_cavlc_444 <name>       Raw AVC CAVLC 444 profile video input file\n");
    printf("  --unc <name>            Raw uncompressed SD UYVY 422 video input file\n");
    printf("  --unc_1080i <name>      Raw uncompressed HD 1080i UYVY 422 video input file\n");
    printf("  --unc_1080p <name>      Raw uncompressed HD 1080p UYVY 422 video input file\n");
    printf("  --unc_720p <name>       Raw uncompressed HD 720p UYVY 422 video input file\n");
    printf("  --unc_3840 <name>       Raw uncompressed UHD 3840x2160 UYVY 422 video input file\n");
    printf("  --avid_alpha <name>               Raw Avid alpha component SD video input file\n");
    printf("  --avid_alpha_1080i <name>         Raw Avid alpha component HD 1080i video input file\n");
    printf("  --avid_alpha_1080p <name>         Raw Avid alpha component HD 1080p video input file\n");
    printf("  --avid_alpha_720p <name>          Raw Avid alpha component HD 720p video input file\n");
    printf("  --mpeg2lg <name>                  Raw MPEG-2 Long GOP video input file\n");
    printf("  --mpeg2lg_422p_ml_576i <name>     Raw MPEG-2 Long GOP 422P@ML 576i video input file\n");
    printf("  --mpeg2lg_mp_ml_576i <name>       Raw MPEG-2 Long GOP MP@ML 576i video input file\n");
    printf("  --mpeg2lg_422p_hl_1080i <name>    Raw MPEG-2 Long GOP 422P@HL 1080i video input file\n");
    printf("  --mpeg2lg_422p_hl_1080p <name>    Raw MPEG-2 Long GOP 422P@HL 1080p video input file\n");
    printf("  --mpeg2lg_422p_hl_720p <name>     Raw MPEG-2 Long GOP 422P@HL 720p video input file\n");
    printf("  --mpeg2lg_mp_hl_1920_1080i <name> Raw MPEG-2 Long GOP MP@HL 1920x1080i video input file\n");
    printf("  --mpeg2lg_mp_hl_1920_1080p <name> Raw MPEG-2 Long GOP MP@HL 1920x1080p video input file\n");
    printf("  --mpeg2lg_mp_hl_1440_1080i <name> Raw MPEG-2 Long GOP MP@HL 1440x1080i video input file\n");
    printf("  --mpeg2lg_mp_hl_1440_1080p <name> Raw MPEG-2 Long GOP MP@HL 1440x1080p video input file\n");
    printf("  --mpeg2lg_mp_hl_720p <name>       Raw MPEG-2 Long GOP MP@HL 720p video input file\n");
    printf("  --mpeg2lg_mp_h14_1080i <name>     Raw MPEG-2 Long GOP MP@H14 1080i video input file\n");
    printf("  --mpeg2lg_mp_h14_1080p <name>     Raw MPEG-2 Long GOP MP@H14 1080p video input file\n");
    printf("  --mjpeg21 <name>        Raw Avid MJPEG 2:1 video input file\n");
    printf("  --mjpeg31 <name>        Raw Avid MJPEG 3:1 video input file\n");
    printf("  --mjpeg101 <name>       Raw Avid MJPEG 10:1 video input file\n");
    printf("  --mjpeg201 <name>       Raw Avid MJPEG 20:1 video input file\n");
    printf("  --mjpeg41m <name>       Raw Avid MJPEG 4:1m video input file\n");
    printf("  --mjpeg101m <name>      Raw Avid MJPEG 10:1m video input file\n");
    printf("  --mjpeg151s <name>      Raw Avid MJPEG 15:1s video input file\n");
    printf("  --rdd36_422_proxy <name>   Raw SMPTE RDD-36 (ProRes) 4:2:2 Proxy profile input file\n");
    printf("  --rdd36_422_lt <name>      Raw SMPTE RDD-36 (ProRes) 4:2:2 LT profile input file\n");
    printf("  --rdd36_422 <name>         Raw SMPTE RDD-36 (ProRes) 4:2:2 profile input file\n");
    printf("  --rdd36_422_hq <name>      Raw SMPTE RDD-36 (ProRes) 4:2:2 HQ profile input file\n");
    printf("  --rdd36_4444 <name>        Raw SMPTE RDD-36 (ProRes) 4:4:4:4 profile input file\n");
    printf("  --rdd36_4444_xq <name>     Raw SMPTE RDD-36 (ProRes) 4:4:4:4 XQ profile input file\n");
    printf("  --j2c_cdci <name|file pattern>  Raw JPEG 2000 (ISO/IEC 15444-1) codestream representing components (e.g. YCbCr) described by a MXF CDCI descriptor\n");
    printf("                                If a '%%' is in the value then it is assumed to be a <file pattern> for a sequence of files, each containing a single frame\n");
    printf("                                See Notes below for a detailed description of <file pattern>\n");
    printf("  --j2c_rgba <name|file pattern>  Raw JPEG 2000 (ISO/IEC 15444-1) codestream representing components (e.g. RGB) described by a MXF RGBA descriptor\n");
    printf("                                If a '%%' is in the value then it is assumed to be a <file pattern> for a sequence of files, each containing a single frame\n");
    printf("                                See Notes below for a detailed description of <file pattern>\n");
    printf("  --jxs_cdci <name|file pattern>  Raw JPEG XS codestream representing components (e.g. YCbCr) described by a MXF CDCI descriptor\n");
    printf("                                If a '%%' is in the value then it is assumed to be a <file pattern> for a sequence of files, each containing a single frame\n");
    printf("                                See Notes below for a detailed description of <file pattern>\n");
    printf("  --jxs_rgba <name|file pattern>  Raw JPEG XS codestream representing components (e.g. RGB) described by a MXF RGBA descriptor\n");
    printf("                                If a '%%' is in the value then it is assumed to be a <file pattern> for a sequence of files, each containing a single frame\n");
    printf("                                See Notes below for a detailed description of <file pattern>\n");
    printf("  --vc2 <name>            Raw VC2 input file\n");
    printf("  --vc3 <name>            Raw VC3/DNxHD input file\n");
    printf("  --vc3_1080p_1235 <name> Raw VC3/DNxHD 1920x1080p 220/185/175 Mbps 10bit input file\n");
    printf("  --vc3_1080p_1237 <name> Raw VC3/DNxHD 1920x1080p 145/120/115 Mbps input file\n");
    printf("  --vc3_1080p_1238 <name> Raw VC3/DNxHD 1920x1080p 220/185/175 Mbps input file\n");
    printf("  --vc3_1080i_1241 <name> Raw VC3/DNxHD 1920x1080i 220/185 Mbps 10bit input file\n");
    printf("  --vc3_1080i_1242 <name> Raw VC3/DNxHD 1920x1080i 145/120 Mbps input file\n");
    printf("  --vc3_1080i_1243 <name> Raw VC3/DNxHD 1920x1080i 220/185 Mbps input file\n");
    printf("  --vc3_1080i_1244 <name> Raw VC3/DNxHD 1920x1080i 145/120 Mbps input file\n");
    printf("  --vc3_720p_1250 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps 10bit input file\n");
    printf("  --vc3_720p_1251 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps input file\n");
    printf("  --vc3_720p_1252 <name>  Raw VC3/DNxHD 1280x720p 220/185/110/90 Mbps input file\n");
    printf("  --vc3_1080p_1253 <name> Raw VC3/DNxHD 1920x1080p 45/36 Mbps input file\n");
    printf("  --vc3_720p_1258 <name>  Raw VC3/DNxHD 1280x720p 45 Mbps input file\n");
    printf("  --vc3_1080p_1259 <name> Raw VC3/DNxHD 1920x1080p 85 Mbps input file\n");
    printf("  --vc3_1080i_1260 <name> Raw VC3/DNxHD 1920x1080i 85 Mbps input file\n");
    printf("  --pcm <name>            Raw PCM audio input file\n");
    printf("  --wave <name>           Wave PCM audio input file\n");
    printf("  --anc <name>            Raw ST 436 Ancillary data. Requires the --anc-const option or frame wrapped in KLV and the --klv option\n");
    printf("  --vbi <name>            Raw ST 436 Vertical Blanking Interval data. Requires the --vbi-const option or frame wrapped in KLV and the --klv option\n");
    printf("  --tt <manifest>         Manifest file containing Timed Text metadata\n");
    printf("\n\n");
    printf("Notes:\n");
    printf(" - filename pattern: Clip types producing a single output file may use a filename pattern with variables that get substituted\n");
    printf("                     Pattern variables start with a '{' and end with a '}'. E.g. 'output_{type}_{fp_uuid}.mxf'\n");
    printf("                     The following variables are available:\n");
    printf("                       - {type}: Is replaced with the name for video, audio, data or mixed essence types\n");
    printf("                                 See the --ess-type-names option for the default names and use the option to change them\n");
    printf("                       - {mp_uuid}: The UUID material number in a material package UMID\n");
    printf("                                    The clip writers will by default generate UMIDs with UUID material numbers\n");
    printf("                       - {mp_umid}: The material package UMID\n");
    printf("                       - {fp_uuid}: The UUID material number in a file source package UMID\n");
    printf("                                    The clip writers will by default generate UMIDs with UUID material numbers\n");
    printf("                       - {fp_umid}: The file source package UMID\n");
    printf("                     At least one letter in a variable name can also be in uppercase, which will result in\n");
    printf("                     the corresponding substituted value being in uppercase.\n");
    printf(" - <umid> format is 64 hexadecimal characters and any '.' and '-' characters are ignored\n");
    printf(" - <uuid> format is 32 hexadecimal characters and any '.' and '-' characters are ignored\n");
    printf(" - <tstamp> format is YYYY-MM-DDThh:mm:ss:qm where qm is in units of 1/250th second\n");
    printf(" - <file pattern>: Each file name must include an integer frame number in the sequence.\n");
    printf("                 <file pattern> is a file path that must contain a single '%%d' where the frame number appears in the name.\n");
    printf("                 <file pattern> must not contain any other '%%' characters.\n");
    printf("                 If --fill-pattern-gaps option is used then gaps in frame numbers are filled by repeating the previous frame.\n");
    printf("                 Example: 'inputs/frame_%%d.j2c' for input files `inputs/frame_00001.j2c' etc..\n");
    printf("\n");
    printf(" The track mapping <expr> format is one of the following:\n");
    printf("     'mono'     : each input audio channel is mapped to a single-channel output track\n");
    printf("     'stereo'   : input audio channels are paired to stereo-channel output tracks\n");
    printf("                  A silence channel is added to the last output track if the channel count is odd\n");
    printf("     'singlemca': all input audio channels are mapped to a single multi-channel output track\n");
    printf("     <pattern>  : a pattern defining how input channels map to output track channels - see below\n");
    printf("\n");
    printf(" The track mapping <pattern> specifies how input audio channels are mapped to output track channels\n");
    printf(" A <pattern> consists of a list of <group>s separated by a ';'.\n");
    printf(" A <group> starts with an optional 'm', followed by a list of <element> separated by a ','.\n");
    printf("   An 'm' indicates that each channel in the <group> is mapped to separate single-channel output track.\n");
    printf("   If an 'm' is not present then the channels are mapped to a single output track.\n");
    printf(" A <element> is either a <channel>, <range>, <silence> or <remainder>.\n");
    printf(" A <channel> is an input channel number starting from 0.\n");
    printf("   The input channel number is the number derived from the input track order reported by mxf2raw for the\n");
    printf("   input files in the same order and including any --disable input options. Each channel in a track\n");
    printf("   contributes 1 to the overall channel number.\n");
    printf(" A <range> is 2 <channel>s separated by a '-' and includes all channels starting with the first number\n");
    printf("   and ending with the last number.\n");
    printf(" A <silence> is 's' followed by a <count> and results in <count> silence channels added to the output track.\n");
    printf(" A <remainder> is 'x', and results in all remaining channels being added to the output track.\n");
    printf("\n");
    printf("Here are some <pattern> examples:\n");
    printf("    'mx'     : equivalent to 'mono'\n");
    printf("    '0,1;x'  : the first 2 channels mapped to a stereo output track and all remaining channels mapped\n");
    printf("               to a single multi-channel track\n");
    printf("    '2-7;0,1': 6 channel output track followed by a 2 channel output track\n");
    printf("    '0,1,s2' : 2 input channels plus 2 silence channels mapped to a single output track\n");
}

int main(int argc, const char** argv)
{
    const char *log_filename = 0;
    LogLevel log_level = INFO_LOG;
    ClipWriterType clip_type = CW_OP1A_CLIP_TYPE;
    ClipSubType clip_sub_type = NO_CLIP_SUB_TYPE;
    bool ard_zdf_hdf_profile = false;
    bool ard_zdf_xdf_profile = false;
    bool aes3 = false;
    bool kag_size_512 = false;
    bool op1a_primary_package = false;
    bool op1a_index_follows = false;
    bool st379_2 = false;
    AS10Shim as10_shim = AS10_UNKNOWN_SHIM;
    const char *mpeg_descr_defaults_name = 0;
    bool mpeg_descr_frame_checks = true;
    int max_mpeg_check_same_warn_messages = 3;
    bool print_mpeg_checks = false;
    bool as10_loose_checks = true;
    const char *output_name = "";
    map<EssenceType, string> filename_essence_type_names;
    vector<RawInput> inputs;
    RawInput input;
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    vector<Timecode> auxiliary_timecodes;
    vector<string> auxiliary_timecode_strings;
    Rational default_frame_rate = FRAME_RATE_25;
    Rational frame_rate = ZERO_RATIONAL;
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
    bool fixed_partition_interval = false;
    const char *shim_name = 0;
    const char *shim_id = 0;
    const char *shim_annot = 0;
    const char *project_name = 0;
    const char *tape_name = 0;
    const char *import_name = 0;
    map<string, string> user_comments;
    vector<LocatorOption> locators;
    AS11Helper as11_helper;
    AS10Helper as10_helper;
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
    AvidUMIDType avid_umid_type = AAFSDK_UMID_TYPE;
    UMID mp_uid = g_Null_UMID;
    bool mp_uid_set = false;
    Timestamp mp_created;
    bool mp_created_set = false;
    UMID fp_uid = g_Null_UMID;
    bool fp_uid_set = false;
    UMID psp_uid = g_Null_UMID;
    bool psp_uid_set = false;
    Timestamp psp_created;
    bool psp_created_set = false;
    bool no_tc_track = false;
    bool min_part = false;
    bool body_part = false;
    bool repeat_index = false;
    bool op1a_clip_wrap = false;
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
    Timestamp creation_date;
    bool creation_date_set = false;
    bool ps_avcihead = false;
    bool avid_gf = false;
    int64_t avid_gf_duration = -1;
    int64_t regtest_end = -1;
    bool have_anc = false;
    bool have_vbi = false;
    bool mp_track_num = false;
    vector<EmbedXMLInfo> embed_xml;
    EmbedXMLInfo next_embed_xml;
    TrackMapper track_mapper;
    bool track_map_set = false;
    bool dump_track_map = false;
    bool dump_track_map_exit = false;
    vector<pair<string, string> > track_mca_labels;
    bool use_avc_subdesc = false;
    UL audio_layout_mode_label = g_Null_UL;
    BMX_OPT_PROP_DECL_DEF(uint32_t, head_fill, 0);
    mxfThreeColorPrimaries three_color_primaries;
    mxfColorPrimary color_primary;
    vector<WaveChunkData> wave_chunk_datas;
    const char *chna_audio_ids_file = 0;
    bool real_essence_regtest = false;
    int value, num, den;
    unsigned int uvalue;
    int64_t i64value;
    mxfRational rate;
    bool msvc_block_limit;
    int cmdln_index;

    memset(&next_embed_xml, 0, sizeof(next_embed_xml));
    parse_essence_type_names("video,audio,data,mixed", &filename_essence_type_names);

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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            log_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--log-level") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_log_level(argv[cmdln_index + 1], &log_level))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-t") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_clip_type(argv[cmdln_index + 1], &clip_type, &clip_sub_type))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-o") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            output_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--ess-type-names") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_essence_type_names(argv[cmdln_index + 1], &filename_essence_type_names))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--prod-info") == 0)
        {
            if (cmdln_index + 5 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing arguments for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_product_info(&argv[cmdln_index + 1], 5, &company_name, &product_name, &product_version,
                                    &version_string, &product_uid))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid values '%s' etc. for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            product_info_set = true;
            cmdln_index += 5;
        }
        else if (strcmp(argv[cmdln_index], "--create-date") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdln_index + 1], &creation_date))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            creation_date_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-f") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_frame_rate(argv[cmdln_index + 1], &frame_rate))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            frame_rate_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dflt-rate") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_frame_rate(argv[cmdln_index + 1], &default_frame_rate))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-y") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            start_timecode_str = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--clip") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            clip_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dur") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &duration) || duration < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rt") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_float(argv[cmdln_index + 1], &rt_factor) || rt_factor <= 0.0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            realtime = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avcihead") == 0)
        {
            if (cmdln_index + 3 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_avci_header(argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3],
                                   &avci_header_inputs))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid values '%s', '%s', '%s' for Option '%s'\n",
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_mic_type(argv[cmdln_index + 1], &mic_type))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mic-file") == 0)
        {
            ess_component_mic_scope = ENTIRE_FILE_MIC_SCOPE;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg-checks") == 0)
        {
            mpeg_descr_frame_checks = true;
            if (cmdln_index + 1 < argc)
            {
                mpeg_descr_defaults_name = argv[cmdln_index + 1];
                if (*mpeg_descr_defaults_name == '-') // optional <name> is not present
                    mpeg_descr_defaults_name = NULL;
                else
                    cmdln_index++;
            }
        }
        else if (strcmp(argv[cmdln_index], "--loose-checks") == 0)
        {
            as10_loose_checks = true;
        }
        else if (strcmp(argv[cmdln_index], "--print-checks") == 0)
        {
            print_mpeg_checks = true;
        }
        else if (strcmp(argv[cmdln_index], "--max-same-warnings") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &max_mpeg_check_same_warn_messages) ||
                max_mpeg_check_same_warn_messages < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--shim-name") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            shim_name = argv[cmdln_index + 1];
            string shim_name_upper = shim_name;
            transform(shim_name_upper.begin(), shim_name_upper.end(), shim_name_upper.begin(), ::toupper);
            as10_helper.SetFrameworkProperty("as10", "ShimName", shim_name_upper.c_str());
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--shim-id") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            shim_id = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--shim-annot") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            shim_annot = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--part") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            partition_interval_str = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--fixed-part") == 0)
        {
            fixed_partition_interval = true;
        }
        else if (strcmp(argv[cmdln_index], "--dm") == 0)
        {
            if (cmdln_index + 3 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument(s) for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (as10_helper.SupportFrameworkType(argv[cmdln_index + 1]) &&
                !as10_helper.SetFrameworkProperty(argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3]))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Failed to set '%s' framework property '%s' to '%s'\n",
                        argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3]);
                return 1;
            }
            if (as11_helper.SupportFrameworkType(argv[cmdln_index + 1]) &&
                !as11_helper.SetFrameworkProperty(argv[cmdln_index + 1], argv[cmdln_index + 2], argv[cmdln_index + 3]))
            {
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument(s) for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (as10_helper.SupportFrameworkType(argv[cmdln_index + 1]) &&
                !as10_helper.ParseFrameworkFile(argv[cmdln_index + 1], argv[cmdln_index + 2]))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Failed to parse '%s' framework file '%s'\n",
                        argv[cmdln_index + 1], argv[cmdln_index + 2]);
                return 1;
            }
            if (as11_helper.SupportFrameworkType(argv[cmdln_index + 1]) &&
                !as11_helper.ParseFrameworkFile(argv[cmdln_index + 1], argv[cmdln_index + 2]))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Failed to parse '%s' framework file '%s'\n",
                        argv[cmdln_index + 1], argv[cmdln_index + 2]);
                return 1;
            }
            cmdln_index += 2;
        }
        else if (strcmp(argv[cmdln_index], "--seg") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            segmentation_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--spec-id") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!as11_helper.ParseSpecificationId(argv[cmdln_index + 1]))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--out-start") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &output_start_offset) ||
                output_start_offset < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--out-end") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &output_end_offset) ||
                output_end_offset < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--seq-off") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                value < 0 || value > 255)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
        else if (strcmp(argv[cmdln_index], "--no-tc-track") == 0)
        {
            no_tc_track = true;
        }
        else if (strcmp(argv[cmdln_index], "--min-part") == 0)
        {
            min_part = true;
        }
        else if (strcmp(argv[cmdln_index], "--body-part") == 0)
        {
            body_part = true;
        }
        else if (strcmp(argv[cmdln_index], "--repeat-index") == 0)
        {
            repeat_index = true;
        }
        else if (strcmp(argv[cmdln_index], "--mp-track-num") == 0)
        {
            mp_track_num = true;
        }
        else if (strcmp(argv[cmdln_index], "--aes-3") == 0)
        {
            aes3 = true;
        }
        else if (strcmp(argv[cmdln_index], "--kag-size-512") == 0)
        {
            kag_size_512 = true;
        }
        else if (strcmp(argv[cmdln_index], "--primary-package") == 0)
        {
            op1a_primary_package = true;
        }
        else if (strcmp(argv[cmdln_index], "--index-follows") == 0)
        {
            op1a_index_follows = true;
        }
        else if (strcmp(argv[cmdln_index], "--st379-2") == 0)
        {
            st379_2 = true;
        }
        else if (strcmp(argv[cmdln_index], "--xml-scheme-id") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!AS11Helper::ParseXMLSchemeId(argv[cmdln_index + 1], &next_embed_xml.scheme_id) &&
                !parse_mxf_auid(argv[cmdln_index + 1], &next_embed_xml.scheme_id))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--xml-lang") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            next_embed_xml.lang = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--embed-xml") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            next_embed_xml.filename = argv[cmdln_index + 1];
            embed_xml.push_back(next_embed_xml);
            memset(&next_embed_xml, 0, sizeof(next_embed_xml));
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--clip-wrap") == 0)
        {
            op1a_clip_wrap = true;
        }
        else if (strcmp(argv[cmdln_index], "--ard-zdf-hdf") == 0)
        {
            ard_zdf_hdf_profile = true;
        }
        else if (strcmp(argv[cmdln_index], "--ard-zdf-xdf") == 0)
        {
            ard_zdf_xdf_profile = true;
        }
        else if (strcmp(argv[cmdln_index], "--d10-mute") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_d10_sound_flags(argv[cmdln_index + 1], &d10_mute_sound_flags))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--d10-invalid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_d10_sound_flags(argv[cmdln_index + 1], &d10_invalid_sound_flags))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--project") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            project_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--tape") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            tape_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--import") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            import_name = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--aux") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            istringstream aux_ss(argv[cmdln_index + 1]);
            string curr_aux;
            while (getline(aux_ss, curr_aux, ','))
            {
                auxiliary_timecode_strings.push_back(curr_aux);
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--comment") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            user_comments["Comments"] = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--desc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument(s) for %s\n", argv[cmdln_index]);
                return 1;
            }
            LocatorOption locator;
            locator.position_str = argv[cmdln_index + 1];
            locator.locator.comment = argv[cmdln_index + 2];
            if (!parse_color(argv[cmdln_index + 3], &locator.locator.color))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Failed to read --locator <color> value '%s'\n", argv[cmdln_index + 3]);
                return 1;
            }
            locators.push_back(locator);
            cmdln_index += 3;
        }
        else if (strcmp(argv[cmdln_index], "--umid-type") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_avid_umid_type(argv[cmdln_index + 1], &avid_umid_type))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mp-uid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_umid(argv[cmdln_index + 1], &mp_uid))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            mp_uid_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mp-created") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdln_index + 1], &mp_created))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            mp_created_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--fp-uid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_umid(argv[cmdln_index + 1], &fp_uid))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            fp_uid_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--psp-uid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_umid(argv[cmdln_index + 1], &psp_uid))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            psp_uid_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--psp-created") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdln_index + 1], &psp_created))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &avid_gf_duration) || avid_gf_duration < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            originator = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--wave-chunk-data") == 0)
        {
            if (cmdln_index + 2 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (argv[cmdln_index + 2][0] == 0 || strlen(argv[cmdln_index + 2]) > 4) {
                usage(argv[0]);
                fprintf(stderr, "Invalid Wave chunk id '%s' for option '%s'\n", argv[cmdln_index + 2], argv[cmdln_index]);
                return 1;
            }

            string id_str = argv[cmdln_index + 2];
            id_str.resize(4, ' ');

            WaveChunkData wave_chunk_data;
            wave_chunk_data.filename = argv[cmdln_index + 1];
            wave_chunk_data.id = WAVE_CHUNK_ID(id_str);
            wave_chunk_datas.push_back(wave_chunk_data);

            cmdln_index += 2;
        }
        else if (strcmp(argv[cmdln_index], "--chna-audio-ids") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            chna_audio_ids_file = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--track-map") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!track_mapper.ParseMapDef(argv[cmdln_index + 1]))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            track_map_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--dump-track-map") == 0)
        {
            dump_track_map = true;
        }
        else if (strcmp(argv[cmdln_index], "--dump-track-map-exit") == 0)
        {
            dump_track_map = true;
            dump_track_map_exit = true;
        }
        else if (strcmp(argv[cmdln_index], "--head-fill") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_bytes_size(argv[cmdln_index + 1], &i64value) || i64value > UINT32_MAX)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(head_fill, (uint32_t)i64value);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--track-mca-labels") == 0)
        {
            if (cmdln_index + 3 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument(s) for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            track_mca_labels.push_back(make_pair(argv[cmdln_index + 1], argv[cmdln_index + 2]));
            cmdln_index += 2;
        }
        else if (strcmp(argv[cmdln_index], "--use-avc-subdesc") == 0)
        {
            use_avc_subdesc = true;
        }
        else if (strcmp(argv[cmdln_index], "--audio-layout") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!AppMCALabelHelper::ParseAudioLayoutMode(argv[cmdln_index + 1], &audio_layout_mode_label) &&
                !parse_mxf_auid(argv[cmdln_index + 1], &audio_layout_mode_label))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &regtest_end) || regtest_end < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--regtest-real") == 0)
        {
            BMX_REGRESSION_TEST = true;
            real_essence_regtest = true;
        }
        else
        {
            break;
        }
    }

    init_input(&input);
    for (; cmdln_index < argc; cmdln_index++)
    {
        msvc_block_limit = false;

        if (strcmp(argv[cmdln_index], "-a") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int_pair(argv[cmdln_index + 1], ':', &num, &den) || num <= 0 || den <= 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.aspect_ratio.numerator   = num;
            input.aspect_ratio.denominator = den;
            BMX_OPT_PROP_MARK(input.aspect_ratio, true);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                value <= 0 || value > 255)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.afd, value);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "-c") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                (value != 8 && value != 10))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.component_depth, value);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--height") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) || value == 0) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.input_height = value;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--signal-std") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_signal_standard(argv[cmdln_index + 1], &input.signal_standard)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.signal_standard, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--frame-layout") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_frame_layout(argv[cmdln_index + 1], &input.frame_layout)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.frame_layout, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--field-dom") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_field_dominance(argv[cmdln_index + 1], &input.field_dominance)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.field_dominance, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--video-line-map") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_video_line_map(argv[cmdln_index + 1], &input.video_line_map)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.video_line_map, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--transfer-ch") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_transfer_ch(argv[cmdln_index + 1], &input.transfer_ch)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.transfer_ch, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--coding-eq") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_coding_equations(argv[cmdln_index + 1], &input.coding_equations)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.coding_equations, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--color-prim") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_color_primaries(argv[cmdln_index + 1], &input.color_primaries)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.color_primaries, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--color-siting") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_color_siting(argv[cmdln_index + 1], &input.color_siting)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.color_siting, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--black-level") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.black_ref_level, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--white-level") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.white_ref_level, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--color-range") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.color_range, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--comp-max-ref") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.comp_max_ref, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--comp-min-ref") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.comp_min_ref, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--scan-dir") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.scan_dir, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--display-primaries") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_three_color_primaries(argv[cmdln_index + 1], &three_color_primaries)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.display_primaries, three_color_primaries);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--display-white-point") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_color_primary(argv[cmdln_index + 1], &color_primary)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.display_white_point, color_primary);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--display-max-luma") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.display_max_luma, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--display-min-luma") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.display_min_luma, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--rdd36-opaque") == 0)
        {
            BMX_OPT_PROP_SET(input.rdd36_opaque, true);
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--active-width") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.active_width, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--active-height") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.active_height, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--active-x-offset") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.active_x_offset, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--active-y-offset") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.active_y_offset, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--display-f2-offset") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.display_f2_offset, value);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--center-cut-4-3") == 0)
        {
            BMX_OPT_PROP_SET(input.center_cut_4_3, true);
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--center-cut-14-9") == 0)
        {
            BMX_OPT_PROP_SET(input.center_cut_14_9, true);
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "-s") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue) && uvalue > 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                (value != 16 && value != 24))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.bits_per_sample = value;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--audio-chan") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                value == 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_bool(argv[cmdln_index + 1], &input.locked))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(input.locked, true);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--audio-ref") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                value < -128 || value > 127)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.audio_ref_level, value);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--dial-norm") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                value < -128 || value > 127)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.dial_norm, value);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--ref-image-edit-rate") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_frame_rate(argv[cmdln_index + 1], &rate))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.ref_image_edit_rate, rate);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--ref-audio-align-level") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.ref_audio_align_level, value);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--anc-const") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &input.file_start_offset) ||
                input.file_start_offset < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--maxlen") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &input.ess_max_length) ||
                input.ess_max_length <= 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--klv") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_klv_opt(argv[cmdln_index + 1], &input.klv_key, &input.klv_track_num))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            input.parse_klv = true;
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--fill-pattern-gaps") == 0)
        {
            input.fill_pattern_gaps = true;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--track-num") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_SET(input.track_number, uvalue);
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--avci-guess") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_avci_guess(argv[cmdln_index + 1],
                                  &input.avci_guess_interlaced,
                                  &input.avci_guess_progressive))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
        else if (strcmp(argv[cmdln_index], "--vc2-mode") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_vc2_mode(argv[cmdln_index + 1], &input.vc2_mode_flags))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--wave-chunks") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_wave_chunk_refs(argv[cmdln_index + 1], input.wave_chunk_refs))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--adm-wave-chunk") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_adm_wave_chunk_ref(argv[cmdln_index + 1], input.wave_chunk_refs))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--no-chna-chunk") == 0)
        {
            input.no_chna_chunk = true;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--trk-out-start") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &input.output_start_offset) ||
                input.output_start_offset < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else if (strcmp(argv[cmdln_index], "--trk-out-end") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &input.output_end_offset) ||
                input.output_end_offset < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
            continue; // skip input reset at the end
        }
        else
        {
            // break if/else here to workaround Visual C++ error
            // C1061: compiler limit : blocks nested too deeply
            msvc_block_limit = true;
        }

        if (!msvc_block_limit)
        {
            // do nothing - wasn't the Visual C++ C1061 workaround
        }
        else if (strcmp(argv[cmdln_index], "--dv") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVCI50_720P;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type_group = AVC_ESSENCE_GROUP;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_baseline") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_BASELINE;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_constr_baseline") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_CONSTRAINED_BASELINE;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_main") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_MAIN;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_extended") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_EXTENDED;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_high") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_HIGH;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_high_10") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_HIGH_10;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_high_422") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_HIGH_422;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_high_444") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_HIGH_444;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_high_10_intra") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_HIGH_10_INTRA;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_high_422_intra") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_HIGH_422_INTRA;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_high_444_intra") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_HIGH_444_INTRA;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avc_cavlc_444") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = AVC_CAVLC_444_INTRA;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--unc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type_group = MPEG2LG_ESSENCE_GROUP;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_422p_ml_576i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MPEG2LG_422P_ML_576I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_mp_ml_576i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MPEG2LG_MP_ML_576I;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--mpeg2lg_422p_hl_1080i") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = MJPEG_15_1S;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--jxs_rgba") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }           
            input.essence_type = JPEGXS_RGBA;
            if (strchr(argv[cmdln_index + 1], '%') != 0)
                input.file_pattern = argv[cmdln_index + 1];
            else
                input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--jxs_cdci") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = JPEGXS_CDCI;
            if (strchr(argv[cmdln_index + 1], '%') != 0)
                input.file_pattern = argv[cmdln_index + 1];
            else
                input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd36_422_proxy") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = RDD36_422_PROXY;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd36_422_lt") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = RDD36_422_LT;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd36_422") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = RDD36_422;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd36_422_hq") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = RDD36_422_HQ;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd36_4444") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = RDD36_4444;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        // Allowing --rdd36_4444_xq as defined in the cli help as well as --rdd36_4444_hq
        // which was defined first to keep backwards compatibility with older scripts.
        else if (strcmp(argv[cmdln_index], "--rdd36_4444_xq") == 0 ||
                 strcmp(argv[cmdln_index], "--rdd36_4444_hq") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = RDD36_4444_XQ;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--j2c_cdci") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = JPEG2000_CDCI;
            if (strchr(argv[cmdln_index + 1], '%') != 0)
                input.file_pattern = argv[cmdln_index + 1];
            else
                input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--j2c_rgba") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = JPEG2000_RGBA;
            if (strchr(argv[cmdln_index + 1], '%') != 0)
               input.file_pattern = argv[cmdln_index + 1];
            else
               input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc2") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VC2;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vc3") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (have_anc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Multiple '%s' inputs are not permitted\n", argv[cmdln_index]);
                return 1;
            }
            if (input.anc_const_size == 0 && !input.parse_klv)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing or zero '--anc-const' or '--klv' options for input '%s'\n", argv[cmdln_index]);
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
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (have_vbi)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Multiple '%s' inputs are not permitted\n", argv[cmdln_index]);
                return 1;
            }
            if (input.vbi_const_size == 0 && !input.parse_klv)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing or zero '--vbi-const' option for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = VBI_DATA;
            input.filename = argv[cmdln_index + 1];
            inputs.push_back(input);
            have_vbi = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--tt") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for input '%s'\n", argv[cmdln_index]);
                return 1;
            }
            input.essence_type = TIMED_TEXT;
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

    // Clear the unused last initialised input
    clear_input(&input);

    if (cmdln_index != argc) {
        usage_ref(argv[0]);
        fprintf(stderr, "Unknown Input Option '%s'\n", argv[cmdln_index]);
        return 1;
    }

    if (inputs.empty()) {
        usage_ref(argv[0]);
        fprintf(stderr, "No raw inputs given\n");
        return 1;
    }

    if (clip_type == CW_AS02_CLIP_TYPE || clip_type == CW_AVID_CLIP_TYPE) {
        if (uses_filename_pattern_variables(output_name)) {
            usage_ref(argv[0]);
            fprintf(stderr, "Clip type '%s' does not support output filename pattern variables\n",
                    clip_type_to_string(clip_type, clip_sub_type));
            return 1;
        }
    }

    if (!frame_rate_set)
        frame_rate = default_frame_rate;

    if (clip_sub_type == AS10_CLIP_SUB_TYPE) {
        const char *as10_shim_name = as10_helper.GetShimName();
        if (!as10_shim_name) {
            usage_ref(argv[0]);
            fprintf(stderr, "Set required 'ShimName' property for as10 output");
            return 1;
        }
        as10_shim = get_as10_shim(as10_shim_name);
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

    if (op1a_clip_wrap && (clip_type != CW_OP1A_CLIP_TYPE || clip_sub_type == AS11_CLIP_SUB_TYPE)) {
        fprintf(stderr, "Ignoring unsupported --clip-wrap option\n");
        op1a_clip_wrap = false;
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
    } else {
        set_avid_umid_type(avid_umid_type);
    }

    if (do_print_version)
        log_info("%s\n", get_app_version_info(APP_NAME).c_str());


    int cmd_result = 0;
    try
    {
        // check the XML files exist
        if (clip_type == CW_OP1A_CLIP_TYPE ||
            clip_type == CW_RDD9_CLIP_TYPE ||
            clip_type == CW_D10_CLIP_TYPE)
        {
            size_t i;
            for (i = 0; i < embed_xml.size(); i++) {
                const EmbedXMLInfo &info = embed_xml[i];
                if (!check_file_exists(info.filename)) {
                    log_error("XML file '%s' does not exist\n", info.filename);
                    throw false;
                }
            }
        } else if (!embed_xml.empty()) {
            log_warn("Embedding XML is not supported for clip type %s\n",
                     clip_type_to_string(clip_type, clip_sub_type));
        }

        // update Avid uncompressed essence types if component depth equals 10
        if (clip_type == CW_AVID_CLIP_TYPE) {
            size_t i;
            for (i = 0; i < inputs.size(); i++) {
                RawInput *input = &inputs[i];
                if (!input->disabled && input->component_depth == 10) {
                    switch (input->essence_type)
                    {
                        case UNC_SD:
                            input->essence_type = AVID_10BIT_UNC_SD;
                            break;
                        case UNC_HD_1080I:
                            input->essence_type = AVID_10BIT_UNC_HD_1080I;
                            break;
                        case UNC_HD_1080P:
                            input->essence_type = AVID_10BIT_UNC_HD_1080P;
                            break;
                        case UNC_HD_720P:
                            input->essence_type = AVID_10BIT_UNC_HD_720P;
                            break;
                        default:
                            break;
                    }
                }
            }
        }


        // change default component depth for RDD-36
        size_t i;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if ((input->essence_type == RDD36_422_PROXY ||
                    input->essence_type == RDD36_422_LT ||
                    input->essence_type == RDD36_422 ||
                    input->essence_type == RDD36_422_HQ ||
                    input->essence_type == RDD36_4444 ||
                    input->essence_type == RDD36_4444_XQ) &&
                !BMX_OPT_PROP_IS_SET(input->component_depth))
            {
                BMX_OPT_PROP_SET(input->component_depth, 10);
            }
        }


        // extract essence info
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->disabled)
                continue;

            if (input->essence_type == WAVE_PCM) {
                if (input->is_wave) {
                    open_wave_reader(input);
                    BMX_ASSERT(input->wave_reader->GetNumTracks() > 0);

                    input->sampling_rate   = input->wave_reader->GetSamplingRate();
                    input->bits_per_sample = input->wave_reader->GetQuantizationBits();
                    input->channel_count   = input->wave_reader->GetNumTracks();
                } else {
                    if (!open_raw_reader(input))
                        throw false;
                }
                continue;
            }

            // TODO: more parse friendly regression test essence data
            if (BMX_REGRESSION_TEST &&
                !real_essence_regtest &&
                input->essence_type != RDD36_422 &&
                input->essence_type != RDD36_4444)
            {
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

                    if (!BMX_OPT_PROP_IS_SET(input->aspect_ratio))
                        BMX_OPT_PROP_SET(input->aspect_ratio, dv_parser->GetAspectRatio());
                }
            }
            else if (input->essence_type == RDD36_422_PROXY ||
                     input->essence_type == RDD36_422_LT ||
                     input->essence_type == RDD36_422 ||
                     input->essence_type == RDD36_422_HQ ||
                     input->essence_type == RDD36_4444 ||
                     input->essence_type == RDD36_4444_XQ)
            {
                RDD36EssenceParser *rdd36_parser = new RDD36EssenceParser();
                input->raw_reader->SetEssenceParser(rdd36_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() != 0) {
                    rdd36_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    if (!frame_rate_set) {
                        if (rdd36_parser->HaveFrameRate()) {
                            frame_rate = rdd36_parser->GetFrameRate();
                        } else {
                            log_warn("No frame rate set for '%s'; using default frame rate %d/%d\n",
                                     essence_type_to_string(input->essence_type),
                                     frame_rate.numerator, frame_rate.denominator);
                        }
                    }

                    // other parameters derived from the bitstream metadata will be set in the RDD36MXFDescriptorHelper
                }
            }
            else if (input->essence_type == VC2)
            {
                VC2EssenceParser *vc2_parser = new VC2EssenceParser();
                input->raw_reader->SetEssenceParser(vc2_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() > 0) {
                    vc2_parser->ParseFrameInfo(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize());

                    if (!frame_rate_set)
                        frame_rate = vc2_parser->GetFrameRate();
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

                    if (!BMX_OPT_PROP_IS_SET(input->aspect_ratio))
                        BMX_OPT_PROP_SET(input->aspect_ratio, mpeg2_parser->GetAspectRatio());
                }
            }
            else if (input->essence_type_group == MPEG2LG_ESSENCE_GROUP ||
                     input->essence_type == MPEG2LG_422P_ML_576I ||
                     input->essence_type == MPEG2LG_MP_ML_576I ||
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
                            if (mpeg2_parser->GetVerticalSize() != 1080
                                && mpeg2_parser->GetVerticalSize() != 576) {
                                log_error("Unexpected MPEG-2 Long GOP vertical size %u; expecting 1080 or 576 \n",
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
                            } else if (mpeg2_parser->GetHorizontalSize() == 720) {
                                switch (mpeg2_parser->GetProfileAndLevel())
                                {
                                    case 0x85:
                                        input->essence_type = MPEG2LG_422P_ML_576I;
                                        break;
                                    case 0x48:
                                        input->essence_type = MPEG2LG_MP_ML_576I;
                                        break;
                                    default:
                                        log_error("Unexpected MPEG-2 Long GOP profile and level %u\n",
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

                    if (!BMX_OPT_PROP_IS_SET(input->aspect_ratio))
                        BMX_OPT_PROP_SET(input->aspect_ratio, mpeg2_parser->GetAspectRatio());
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

                avc_parser->ParseFrameInfo2(input->raw_reader->GetSampleData(),
                                            ParsedFrameSize(input->raw_reader->GetSampleDataSize()));
                input->essence_type = avc_parser->GetAVCIEssenceType(input->raw_reader->GetSampleDataSize(),
                                                                     input->avci_guess_interlaced,
                                                                     input->avci_guess_progressive);
                if (input->essence_type == UNKNOWN_ESSENCE_TYPE) {
                    log_error("AVC-I essence type not recognised\n");
                    throw false;
                }

                if (!frame_rate_set) {
                    if (avc_parser->HaveFrameRate()) {
                        frame_rate = avc_parser->GetFrameRate();
                    } else {
                        log_warn("No frame rate set for '%s'; using default frame rate %d/%d\n",
                                 essence_type_to_string(input->essence_type),
                                 frame_rate.numerator, frame_rate.denominator);
                    }
                }

                // re-open the raw reader to use the special AVCI reader
                delete input->raw_reader;
                input->raw_reader = 0;
                if (!open_raw_reader(input))
                    throw false;
            }
            else if (input->essence_type_group == AVC_ESSENCE_GROUP ||
                     input->essence_type == AVC_BASELINE ||
                     input->essence_type == AVC_CONSTRAINED_BASELINE ||
                     input->essence_type == AVC_MAIN ||
                     input->essence_type == AVC_EXTENDED ||
                     input->essence_type == AVC_HIGH ||
                     input->essence_type == AVC_HIGH_10 ||
                     input->essence_type == AVC_HIGH_422 ||
                     input->essence_type == AVC_HIGH_444 ||
                     input->essence_type == AVC_HIGH_10_INTRA ||
                     input->essence_type == AVC_HIGH_422_INTRA ||
                     input->essence_type == AVC_HIGH_444_INTRA ||
                     input->essence_type == AVC_CAVLC_444_INTRA)
            {
                AVCEssenceParser *avc_parser = new AVCEssenceParser();
                input->raw_reader->SetEssenceParser(avc_parser);

                input->raw_reader->ReadSamples(1);
                if (input->raw_reader->GetNumSamples() == 0) {
                    if (input->essence_type_group == AVC_ESSENCE_GROUP)
                        input->essence_type = AVC_HIGH_422;
                } else {
                    avc_parser->ParseFrameInfo2(input->raw_reader->GetSampleData(),
                                                ParsedFrameSize(input->raw_reader->GetSampleDataSize()));
                    if (input->essence_type_group != AVC_ESSENCE_GROUP &&
                        avc_parser->GetEssenceType() != input->essence_type)
                    {
                        log_warn("Using parsed AVC essence type '%s' which does not match commandline option essence type '%s'\n",
                                 essence_type_to_string(avc_parser->GetEssenceType()),
                                 essence_type_to_string(input->essence_type));
                    }
                    input->essence_type = avc_parser->GetEssenceType();

                    if (!frame_rate_set) {
                        if (avc_parser->HaveFrameRate()) {
                            frame_rate = avc_parser->GetFrameRate();
                        } else {
                            log_warn("No frame rate set for '%s'; using default frame rate %d/%d\n",
                                    essence_type_to_string(input->essence_type),
                                    frame_rate.numerator, frame_rate.denominator);
                        }
                    }

                    // TODO: other parameters?
                }
            }
        }


        // complete commandline parsing

        start_timecode.Init(frame_rate, false);
        if (start_timecode_str && !parse_timecode(start_timecode_str, frame_rate, &start_timecode)) {
            usage_ref(argv[0]);
            log_error("Invalid value '%s' for Option '-t'\n", start_timecode_str);
            throw false;
        }

        if (auxiliary_timecode_strings.size() > 5)
        {
            log_error("Avid only supports up to 5 auxilliary timecode tracks.\n");
            throw false;
        }
        for (size_t i = 0; i < auxiliary_timecode_strings.size(); i++)
        {
            Timecode auxiliary_timecode;
            if (!parse_timecode(auxiliary_timecode_strings[i].c_str(), frame_rate, &auxiliary_timecode)) {
                usage_ref(argv[0]);
                log_error("Invalid value '%s' for Option '--aux'\n", auxiliary_timecode_strings[i].c_str());
                throw false;
            }
            auxiliary_timecodes.push_back(auxiliary_timecode);
        }

        if (partition_interval_str) {
            if (!parse_partition_interval(partition_interval_str, frame_rate, &partition_interval))
            {
                usage_ref(argv[0]);
                log_error("Invalid value '%s' for Option '--part'\n", partition_interval_str);
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
                usage_ref(argv[0]);
                log_error("Invalid value '%s' for Option '--locator'\n", locators[i].position_str);
                throw false;
            }
        }

        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->disabled || input->essence_type != TIMED_TEXT)
                continue;

            if (!input->timed_text_manifest->Parse(input->filename, start_timecode, frame_rate)) {
                log_error("Failed to parse timed text manifest\n");
                throw false;
            }
            if (!input->timed_text_manifest->CheckCanReadTTFile()) {
                log_error("Timed text file '%s' referenced by manifest can't be opened for reading\n",
                          input->timed_text_manifest->GetTTFilename().c_str());
                throw false;
            }
        }

        // Determine if the input is sound only
        bool have_sound = false;
        bool sound_only_container = true;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->disabled)
                continue;

            if (input->essence_type == WAVE_PCM) {
                have_sound = true;
            } else if (input->essence_type != TIMED_TEXT) {  // timed text is in a separate container
                sound_only_container = false;
                break;
            }
        }
        if (sound_only_container && !have_sound)
            sound_only_container = false;

        // Set --clip-wrap if output is IMF with sound only
        if (clip_type == CW_OP1A_CLIP_TYPE &&
            clip_sub_type == IMF_CLIP_SUB_TYPE &&
            sound_only_container)
        {
            op1a_clip_wrap = true;
        }

        // Default --track-map to "singlemca" if clip wrapping
        if (!track_map_set && op1a_clip_wrap)
            track_mapper.ParseMapDef("singlemca");


        // check support for essence type and frame/sampling rates

        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->disabled)
                continue;

            if (input->essence_type == WAVE_PCM) {
                if (!ClipWriterTrack::IsSupported(clip_type, input->essence_type, input->sampling_rate)) {
                    log_error("PCM sampling rate %d/%d not supported\n",
                              input->sampling_rate.numerator, input->sampling_rate.denominator);
                    throw false;
                }
            } else {
                if (!ClipWriterTrack::IsSupported(clip_type, input->essence_type, frame_rate)) {
                    log_error("Essence type '%s' @%d/%d Hz not supported for clip type '%s'\n",
                              essence_type_to_string(input->essence_type),
                              frame_rate.numerator, frame_rate.denominator,
                              clip_type_to_string(clip_type, clip_sub_type));
                    throw false;
                }
            }
        }


        // check whether all tracks are timed text and therefore that duration must be set

        bool have_samples_to_write = false;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (!input->disabled && input->essence_type != TIMED_TEXT) {
                have_samples_to_write = true;
                break;
            }
        }
        if (!have_samples_to_write && duration < 0) {
            log_error("--dur option is required if all tracks are timed text\n");
            throw false;
        }


        // map input to output tracks

        // map WAVE PCM tracks
        vector<TrackMapper::InputTrackInfo> unused_input_tracks;
        vector<TrackMapper::InputTrackInfo> mapper_input_tracks;
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->disabled)
                continue;

            TrackMapper::InputTrackInfo mapper_input_track;
            if (input->essence_type == WAVE_PCM) {
                mapper_input_track.external_index  = (uint32_t)i;
                mapper_input_track.essence_type    = input->essence_type;
                mapper_input_track.data_def        = convert_essence_type_to_data_def(input->essence_type);
                mapper_input_track.bits_per_sample = input->bits_per_sample;
                mapper_input_track.channel_count   = input->channel_count;
                mapper_input_tracks.push_back(mapper_input_track);
            }
        }
        vector<TrackMapper::OutputTrackMap> output_track_maps =
            track_mapper.MapTracks(mapper_input_tracks, &unused_input_tracks);

        if (dump_track_map) {
            track_mapper.DumpOutputTrackMap(stderr, mapper_input_tracks, output_track_maps);
            if (dump_track_map_exit)
                throw true;
        }

        // TODO: a non-mono audio mapping requires changes to the Avid physical source package track layout and
        // also depends on support in Avid products
        if (clip_type == CW_AVID_CLIP_TYPE && !TrackMapper::IsMonoOutputTrackMap(output_track_maps)) {
            log_error("Avid clip type only supports mono audio track mapping\n");
            throw false;
        }

        for (i = 0; i < unused_input_tracks.size(); i++) {
            RawInput *input = &inputs[unused_input_tracks[i].external_index];
            log_info("Input %" PRIszt " is not mapped (essence type '%s')\n",
                      unused_input_tracks[i].external_index, essence_type_to_string(input->essence_type));
            input->disabled = true;
        }

        // insert non-WAVE PCM tracks to output mapping
        bool have_enabled_input = false;
        uint32_t input_track_index = (uint32_t)mapper_input_tracks.size();
        for (i = 0; i < inputs.size(); i++) {
            RawInput *input = &inputs[i];
            if (input->disabled)
                continue;

            have_enabled_input = true;

            if (input->essence_type != WAVE_PCM) {
                TrackMapper::OutputTrackMap track_map;
                track_map.essence_type = input->essence_type;
                track_map.data_def     = convert_essence_type_to_data_def(input->essence_type);

                TrackMapper::TrackChannelMap channel_map;
                channel_map.have_input           = true;
                channel_map.input_external_index = (uint32_t)i;
                channel_map.input_index          = input_track_index;
                channel_map.input_channel_index  = 0;
                channel_map.output_channel_index = 0;
                track_map.channel_maps.push_back(channel_map);

                output_track_maps.push_back(track_map);
                input_track_index++;
            }
        }
        if (output_track_maps.empty()) {
            log_error("No output tracks are mapped\n");
            throw false;
        }
        if (!have_enabled_input) {
            log_error("No raw input mapped to output\n");
            throw false;
        }

        // the order determines the regression test's MXF identifiers values and so the
        // output_track_maps are ordered to ensure the regression test isn't effected
        // It also helps analysing MXF dumps as the tracks will be in a consistent order
        std::stable_sort(output_track_maps.begin(), output_track_maps.end(), regtest_output_track_map_comp);


        // complete output filename using pattern variables

        string complete_output_name = output_name;
        if (uses_filename_pattern_variables(output_name)) {
            // set MXF package UIDs
            if (!mp_uid_set) {
                if (clip_type == CW_OP1A_CLIP_TYPE) {
                    mp_uid = OP1AFile::CreatePackageUID();
                    mp_uid_set = true;
                } else if (clip_type == CW_D10_CLIP_TYPE) {
                    mp_uid = D10File::CreatePackageUID();
                    mp_uid_set = true;
                } else if (clip_type == CW_RDD9_CLIP_TYPE) {
                    mp_uid = RDD9File::CreatePackageUID();
                    mp_uid_set = true;
                }
            }
            if (!fp_uid_set) {
                if (clip_type == CW_OP1A_CLIP_TYPE) {
                    fp_uid = OP1AFile::CreatePackageUID();
                    fp_uid_set = true;
                } else if (clip_type == CW_D10_CLIP_TYPE) {
                    fp_uid = D10File::CreatePackageUID();
                    fp_uid_set = true;
                } else if (clip_type == CW_RDD9_CLIP_TYPE) {
                    fp_uid = RDD9File::CreatePackageUID();
                    fp_uid_set = true;
                }
            }

            // determine generic essence type or UNKNOWN_ESSENCE_TYPE if mixed
            EssenceType generic_essence_type = UNKNOWN_ESSENCE_TYPE;
            for (size_t i = 0; i < output_track_maps.size(); i++) {
                EssenceType track_generic_essence_type = get_generic_essence_type(output_track_maps[i].essence_type);
                if (i == 0) {
                    generic_essence_type = track_generic_essence_type;
                } else if (generic_essence_type != track_generic_essence_type) {
                    generic_essence_type = UNKNOWN_ESSENCE_TYPE;
                    break;
                }
            }

            complete_output_name = create_filename_from_pattern(output_name, generic_essence_type,
                                                                filename_essence_type_names,
                                                                mp_uid, fp_uid);
            log_info("Output filename set to '%s'\n", complete_output_name.c_str());
        }

        if (complete_output_name.empty()) {
            log_error("No output name given; use the '-o' option\n");
            throw false;
        }


        // create clip

        int flavour = 0;
        if (clip_type == CW_OP1A_CLIP_TYPE) {
            flavour = OP1A_DEFAULT_FLAVOUR;
            if (ard_zdf_xdf_profile) {
                flavour |= OP1A_ARD_ZDF_XDF_PROFILE_FLAVOUR;
            } else if (ard_zdf_hdf_profile) {
                flavour |= OP1A_ARD_ZDF_HDF_PROFILE_FLAVOUR;
            } else if (clip_sub_type == AS11_CLIP_SUB_TYPE) {
                if (as11_helper.HaveAS11CoreFramework()) // AS11 Core Framework has the Audio Track Layout property
                    flavour |= OP1A_MP_TRACK_NUMBER_FLAVOUR;
                flavour |= OP1A_AS11_FLAVOUR;
            } else if (clip_sub_type == IMF_CLIP_SUB_TYPE) {
                flavour |= OP1A_IMF_FLAVOUR;
            } else {
                if (mp_track_num)
                    flavour |= OP1A_MP_TRACK_NUMBER_FLAVOUR;
                if (aes3)
                    flavour |= OP1A_AES_FLAVOUR;
                if (kag_size_512)
                    flavour |= OP1A_512_KAG_FLAVOUR;
                if (min_part)
                    flavour |= OP1A_MIN_PARTITIONS_FLAVOUR;
                else if (body_part)
                    flavour |= OP1A_BODY_PARTITIONS_FLAVOUR;
            }
            if (file_md5)
                flavour |= OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= OP1A_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_D10_CLIP_TYPE) {
            flavour = D10_DEFAULT_FLAVOUR;
            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                flavour |= D10_AS11_FLAVOUR;
            // single pass flavours not (yet) supported
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            if (ard_zdf_hdf_profile)
                flavour = RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR;
            else if (clip_sub_type == AS10_CLIP_SUB_TYPE)
                flavour = RDD9_AS10_FLAVOUR;
            else if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                flavour = RDD9_AS11_FLAVOUR;
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
                clip = ClipWriter::OpenNewAS02Clip(complete_output_name, true, frame_rate, &file_factory, false);
                break;
            case CW_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewOP1AClip(flavour, file_factory.OpenNew(complete_output_name), frame_rate);
                break;
            case CW_AVID_CLIP_TYPE:
                clip = ClipWriter::OpenNewAvidClip(flavour, frame_rate, &file_factory, false);
                break;
            case CW_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewD10Clip(flavour, file_factory.OpenNew(complete_output_name), frame_rate);
                break;
            case CW_RDD9_CLIP_TYPE:
                clip = ClipWriter::OpenNewRDD9Clip(flavour, file_factory.OpenNew(complete_output_name), frame_rate);
                break;
            case CW_WAVE_CLIP_TYPE:
                clip = ClipWriter::OpenNewWaveClip(WaveFileIO::OpenNew(complete_output_name));
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
        if (auxiliary_timecodes.size() > 0)
            clip->SetAuxiliaryTimecodes(auxiliary_timecodes);
        if (clip_name)
            clip->SetClipName(clip_name);
        else if (clip_sub_type == AS11_CLIP_SUB_TYPE && as11_helper.HaveProgrammeTitle())
            clip->SetClipName(as11_helper.GetProgrammeTitle());
        else if (clip_sub_type == AS10_CLIP_SUB_TYPE && as10_helper.HaveMainTitle())
            clip->SetClipName(as10_helper.GetMainTitle());
        clip->SetProductInfo(company_name, product_name, product_version, version_string, product_uid);
        if (creation_date_set)
            clip->SetCreationDate(creation_date);

        if (clip_type == CW_AS02_CLIP_TYPE) {
            AS02Clip *as02_clip = clip->GetAS02Clip();
            AS02Bundle *bundle = as02_clip->GetBundle();

            if (BMX_OPT_PROP_IS_SET(head_fill))
                as02_clip->ReserveHeaderMetadataSpace(head_fill);

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

            if (BMX_OPT_PROP_IS_SET(head_fill))
                op1a_clip->ReserveHeaderMetadataSpace(head_fill);

            if (repeat_index)
                op1a_clip->SetRepeatIndexTable(true);
            if (op1a_index_follows)
                op1a_clip->SetIndexFollowsEssence(true);

            if (st379_2)
                op1a_clip->SetSignalST3792(true);

            if (mp_uid_set)
                op1a_clip->SetMaterialPackageUID(mp_uid);
            if (fp_uid_set)
                op1a_clip->SetFileSourcePackageUID(fp_uid);

            if (op1a_clip_wrap)
                op1a_clip->SetClipWrapped(true);
            if (partition_interval_set)
                op1a_clip->SetPartitionInterval(partition_interval);
            op1a_clip->SetOutputStartOffset(output_start_offset);
            op1a_clip->SetOutputEndOffset(- output_end_offset);
            if (no_tc_track)
                op1a_clip->SetAddTimecodeTrack(false);
            if (op1a_primary_package)
                op1a_clip->SetPrimaryPackage(true);

            if (!have_samples_to_write)
                op1a_clip->SetInputDuration(duration);
        } else if (clip_type == CW_AVID_CLIP_TYPE) {
            AvidClip *avid_clip = clip->GetAvidClip();

            if (avid_gf && avid_gf_duration >= 0)
                avid_clip->SetGrowingDuration(avid_gf_duration);

            if (!clip_name)
                avid_clip->SetClipName(complete_output_name);

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
                    RawInput *input = &inputs[i];
                    if (!input->disabled) {
                        if (input->essence_type == WAVE_PCM)
                            num_sound_tracks += input->channel_count;
                        else
                            num_picture_tracks++;
                    }
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

            if (mp_uid_set)
                d10_clip->SetMaterialPackageUID(mp_uid);
            if (fp_uid_set)
                d10_clip->SetFileSourcePackageUID(fp_uid);

            d10_clip->SetMuteSoundFlags(d10_mute_sound_flags);
            d10_clip->SetInvalidSoundFlags(d10_invalid_sound_flags);

            if (BMX_OPT_PROP_IS_SET(head_fill))
                d10_clip->ReserveHeaderMetadataSpace(head_fill);
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            RDD9File *rdd9_clip = clip->GetRDD9Clip();

            if (repeat_index)
                rdd9_clip->SetRepeatIndexTable(true);

            if (partition_interval_set)
                rdd9_clip->SetPartitionInterval(partition_interval);
            rdd9_clip->SetFixedPartitionInterval(fixed_partition_interval);
            rdd9_clip->SetOutputStartOffset(output_start_offset);
            rdd9_clip->SetOutputEndOffset(- output_end_offset);

            if (mp_uid_set)
                rdd9_clip->SetMaterialPackageUID(mp_uid);
            if (fp_uid_set)
                rdd9_clip->SetFileSourcePackageUID(fp_uid);

            if (BMX_OPT_PROP_IS_SET(head_fill))
                rdd9_clip->ReserveHeaderMetadataSpace(head_fill);

            if (clip_sub_type == AS10_CLIP_SUB_TYPE)
                rdd9_clip->SetValidator(new AS10RDD9Validator(as10_shim, as10_loose_checks));
        } else if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();

            if (originator)
                wave_clip->GetBroadcastAudioExtension()->SetOriginator(originator);
        }


        // create the output tracks
        map<uint32_t, RawInputTrack*> created_input_tracks;
        vector<OutputTrack*> output_tracks;
        vector<RawInputTrack*> input_tracks;
        map<MXFDataDefEnum, uint32_t> phys_src_track_indexes;
        for (i = 0; i < output_track_maps.size(); i++) {
            const TrackMapper::OutputTrackMap &output_track_map = output_track_maps[i];

            OutputTrack *output_track;
            if (clip_type == CW_AVID_CLIP_TYPE) {
                // each channel is mapped to a separate physical source package track
                MXFDataDefEnum data_def = (MXFDataDefEnum)output_track_map.data_def;
                string track_name = create_mxf_track_filename(complete_output_name.c_str(),
                                                              phys_src_track_indexes[data_def] + 1,
                                                              data_def);
                output_track = new OutputTrack(clip->CreateTrack(output_track_map.essence_type, track_name.c_str()));
                output_track->SetPhysSrcTrackIndex(phys_src_track_indexes[data_def]);

                phys_src_track_indexes[data_def]++;
            } else {
                output_track = new OutputTrack(clip->CreateTrack(output_track_map.essence_type));
            }

            size_t k;
            for (k = 0; k < output_track_map.channel_maps.size(); k++) {
                const TrackMapper::TrackChannelMap &channel_map = output_track_map.channel_maps[k];

                if (channel_map.have_input) {
                    RawInput *input = &inputs[channel_map.input_external_index];
                    RawInputTrack *input_track;
                    if (created_input_tracks.count(channel_map.input_external_index)) {
                        input_track = created_input_tracks[channel_map.input_external_index];
                    } else {
                        input_track = new RawInputTrack(input, (MXFDataDefEnum)output_track_map.data_def);
                        input_tracks.push_back(input_track);
                        created_input_tracks[channel_map.input_external_index] = input_track;
                    }

                    // copy across sound info to OutputTrack
                    if (!output_track->HaveInputTrack() &&
                        convert_essence_type_to_data_def(input->essence_type) == MXF_SOUND_DDEF)
                    {
                        OutputTrackSoundInfo *output_sound_info = output_track->GetSoundInfo();
                        output_sound_info->sampling_rate   = input->sampling_rate;
                        output_sound_info->bits_per_sample = input->bits_per_sample;
                        output_sound_info->sequence_offset = 0;
                        BMX_OPT_PROP_COPY(output_sound_info->locked,                input->locked);
                        BMX_OPT_PROP_COPY(output_sound_info->audio_ref_level,       input->audio_ref_level);
                        BMX_OPT_PROP_COPY(output_sound_info->dial_norm,             input->dial_norm);
                        BMX_OPT_PROP_COPY(output_sound_info->ref_image_edit_rate,   input->ref_image_edit_rate);
                        BMX_OPT_PROP_COPY(output_sound_info->ref_audio_align_level, input->ref_audio_align_level);
                    }

                    output_track->AddInput(input_track, channel_map.input_channel_index, channel_map.output_channel_index);
                    input_track->AddOutput(output_track, channel_map.output_channel_index, channel_map.input_channel_index);
                } else {
                    output_track->AddSilenceChannel(channel_map.output_channel_index);
                }
            }

            output_tracks.push_back(output_track);
        }


        // initialise silence output track info using the first non-silent sound track info

        OutputTrackSoundInfo *donor_sound_info = 0;
        for (i = 0; i < output_tracks.size(); i++) {
            OutputTrack *output_track = output_tracks[i];
            if (!output_track->IsSilenceTrack() && output_track->GetSoundInfo()) {
                donor_sound_info = output_track->GetSoundInfo();
                break;
            }
        }
        for (i = 0; i < output_tracks.size(); i++) {
            OutputTrack *output_track = output_tracks[i];
            if (output_track->IsSilenceTrack()) {
                if (!donor_sound_info) {
                    log_error("All sound tracks containing silence is currently not supported\n");
                    throw false;
                }
                output_track->GetSoundInfo()->Copy(*donor_sound_info);
            }
        }


        // initialize output clip tracks

        unsigned char avci_header_data[AVCI_HEADER_SIZE];
        bool update_header = false;
        for (i = 0; i < output_tracks.size(); i++) {
            OutputTrack *output_track = output_tracks[i];

            ClipWriterTrack *clip_track = output_track->GetClipTrack();

            RawInputTrack *input_track = 0;
            RawInput *input = 0;
            EssenceType essence_type;
            if (output_track->HaveInputTrack()) {
                input_track = dynamic_cast<RawInputTrack*>(output_track->GetFirstInputTrack());
                input = input_track->GetRawInput();
                essence_type = input->essence_type;
            } else {
                essence_type = clip_track->GetEssenceType();
            }
            MXFDataDefEnum data_def = convert_essence_type_to_data_def(essence_type);
            const OutputTrackSoundInfo *output_sound_info = output_track->GetSoundInfo();

            if (input && BMX_OPT_PROP_IS_SET(input->track_number))
                clip_track->SetOutputTrackNumber(input->track_number);

            if (clip_type == CW_AS02_CLIP_TYPE) {
                AS02Track *as02_track = clip_track->GetAS02Track();
                as02_track->SetMICType(mic_type);
                as02_track->SetMICScope(ess_component_mic_scope);
                if (input) {
                    as02_track->SetOutputStartOffset(input->output_start_offset);
                    as02_track->SetOutputEndOffset(- input->output_end_offset);
                }

                if (partition_interval_set) {
                    AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(as02_track);
                    if (as02_pict_track)
                        as02_pict_track->SetPartitionInterval(partition_interval);
                }
            } else if (clip_type == CW_AVID_CLIP_TYPE) {
                AvidTrack *avid_track = clip_track->GetAvidTrack();

                if (physical_package) {
                    if (data_def == MXF_PICTURE_DDEF) {
                        avid_track->SetSourceRef(physical_package_picture_refs[output_track->GetPhysSrcTrackIndex()].first,
                                                 physical_package_picture_refs[output_track->GetPhysSrcTrackIndex()].second);
                    } else if (data_def == MXF_SOUND_DDEF) {
                        avid_track->SetSourceRef(physical_package_sound_refs[output_track->GetPhysSrcTrackIndex()].first,
                                                 physical_package_sound_refs[output_track->GetPhysSrcTrackIndex()].second);
                    }
                }
            }

            BMX_ASSERT(input || essence_type == WAVE_PCM);
            switch (essence_type)
            {
                case IEC_DV25:
                case DVBASED_DV25:
                case DV50:
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    break;
                case DV100_1080I:
                case DV100_720P:
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    clip_track->SetComponentDepth(input->component_depth);
                    break;
                case D10_30:
                case D10_40:
                case D10_50:
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (input->set_bs_aspect_ratio)
                        output_track->SetFilter(new MPEG2AspectRatioFilter(input->aspect_ratio));
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
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
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    clip_track->SetUseAVCSubDescriptor(use_avc_subdesc);
                    if (force_no_avci_head) {
                        clip_track->SetAVCIMode(AVCI_NO_FRAME_HEADER_MODE);
                    } else {
                        if (allow_no_avci_head)
                            clip_track->SetAVCIMode(AVCI_NO_OR_ALL_FRAME_HEADER_MODE);
                        else
                            clip_track->SetAVCIMode(AVCI_ALL_FRAME_HEADER_MODE);

                        if (ps_avcihead && get_ps_avci_header_data(input->essence_type, frame_rate,
                                                                   avci_header_data, sizeof(avci_header_data)))
                        {
                            clip_track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
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
                            clip_track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                        }
                    }
                    break;
                case AVC_BASELINE:
                case AVC_CONSTRAINED_BASELINE:
                case AVC_MAIN:
                case AVC_EXTENDED:
                case AVC_HIGH:
                case AVC_HIGH_10:
                case AVC_HIGH_422:
                case AVC_HIGH_444:
                case AVC_HIGH_10_INTRA:
                case AVC_HIGH_422_INTRA:
                case AVC_HIGH_444_INTRA:
                case AVC_CAVLC_444_INTRA:
                    if (BMX_OPT_PROP_IS_SET(input->aspect_ratio))
                        clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    update_header = true;
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
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    clip_track->SetComponentDepth(input->component_depth);
                    if (input->input_height > 0)
                        clip_track->SetInputHeight(input->input_height);
                    break;
                case AVID_ALPHA_SD:
                case AVID_ALPHA_HD_1080I:
                case AVID_ALPHA_HD_1080P:
                case AVID_ALPHA_HD_720P:
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    if (input->input_height > 0)
                        clip_track->SetInputHeight(input->input_height);
                    break;
                case MPEG2LG_422P_ML_576I:
                case MPEG2LG_MP_ML_576I:
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
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    if (mpeg_descr_frame_checks && (flavour & RDD9_AS10_FLAVOUR)) {
                        RDD9MPEG2LGTrack *rdd9_mpeglgtrack = dynamic_cast<RDD9MPEG2LGTrack*>(clip_track->GetRDD9Track());
                        if (rdd9_mpeglgtrack) {
                            rdd9_mpeglgtrack->SetValidator(new AS10MPEG2Validator(as10_shim, mpeg_descr_defaults_name,
                                                                                  max_mpeg_check_same_warn_messages,
                                                                                  print_mpeg_checks,
                                                                                  as10_loose_checks));
                        }
                    }
                    break;
                case MJPEG_2_1:
                case MJPEG_3_1:
                case MJPEG_10_1:
                case MJPEG_20_1:
                case MJPEG_4_1M:
                case MJPEG_10_1M:
                case MJPEG_15_1S:
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    break;
                case RDD36_422_PROXY:
                case RDD36_422_LT:
                case RDD36_422:
                case RDD36_422_HQ:
                case RDD36_4444:
                case RDD36_4444_XQ:
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    clip_track->SetComponentDepth(input->component_depth);
                    break;
                case JPEG2000_CDCI:
                case JPEG2000_RGBA:
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    break;
                case JPEGXS_CDCI:
                case JPEGXS_RGBA:
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    break;
                case VC2:
                    clip_track->SetAspectRatio(input->aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    clip_track->SetVC2ModeFlags(input->vc2_mode_flags);
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
                    if (BMX_OPT_PROP_IS_SET(input->afd))
                        clip_track->SetAFD(input->afd);
                    break;
                case WAVE_PCM:
                    clip_track->SetSamplingRate(output_sound_info->sampling_rate);
                    clip_track->SetQuantizationBits(output_sound_info->bits_per_sample);
                    clip_track->SetChannelCount(output_track->GetChannelCount());
                    if (BMX_OPT_PROP_IS_SET(output_sound_info->locked))
                        clip_track->SetLocked(output_sound_info->locked);
                    if (BMX_OPT_PROP_IS_SET(output_sound_info->audio_ref_level))
                        clip_track->SetAudioRefLevel(output_sound_info->audio_ref_level);
                    if (BMX_OPT_PROP_IS_SET(output_sound_info->dial_norm))
                        clip_track->SetDialNorm(output_sound_info->dial_norm);
                    // force D10 sequence offset to 0 and default to 0 for other clip types
                    if (clip_type == CW_D10_CLIP_TYPE || sequence_offset_set)
                        clip_track->SetSequenceOffset(sequence_offset);
                    if (audio_layout_mode_label != g_Null_UL)
                        clip_track->SetChannelAssignment(audio_layout_mode_label);
                    break;
                case ANC_DATA:
                    if (input->anc_const_size)
                        clip_track->SetConstantDataSize(input->anc_const_size);
                    break;
                case VBI_DATA:
                    if (input->vbi_const_size)
                        clip_track->SetConstantDataSize(input->vbi_const_size);
                    break;
                case TIMED_TEXT:
                    clip_track->SetTimedTextSource(input->timed_text_manifest);
                    break;
                default:
                    BMX_ASSERT(false);
            }

            PictureMXFDescriptorHelper *pict_helper =
                    dynamic_cast<PictureMXFDescriptorHelper*>(clip_track->GetMXFDescriptorHelper());
            SoundMXFDescriptorHelper *sound_helper =
                    dynamic_cast<SoundMXFDescriptorHelper*>(clip_track->GetMXFDescriptorHelper());

            if (pict_helper) {
                if (BMX_OPT_PROP_IS_SET(input->signal_standard))
                    pict_helper->SetSignalStandard(input->signal_standard);
                if (BMX_OPT_PROP_IS_SET(input->frame_layout))
                    pict_helper->SetFrameLayout(input->frame_layout);
                if (BMX_OPT_PROP_IS_SET(input->field_dominance))
                    pict_helper->SetFieldDominance(input->field_dominance);
                if (BMX_OPT_PROP_IS_SET(input->video_line_map))
                    pict_helper->SetVideoLineMap(input->video_line_map);
                if (BMX_OPT_PROP_IS_SET(input->transfer_ch))
                    pict_helper->SetTransferCharacteristic(input->transfer_ch);
                if (BMX_OPT_PROP_IS_SET(input->coding_equations))
                    pict_helper->SetCodingEquations(input->coding_equations);
                if (BMX_OPT_PROP_IS_SET(input->color_primaries))
                    pict_helper->SetColorPrimaries(input->color_primaries);
                if (BMX_OPT_PROP_IS_SET(input->color_siting))
                    pict_helper->SetColorSiting(input->color_siting);
                if (BMX_OPT_PROP_IS_SET(input->black_ref_level))
                    pict_helper->SetBlackRefLevel(input->black_ref_level);
                if (BMX_OPT_PROP_IS_SET(input->white_ref_level))
                    pict_helper->SetWhiteRefLevel(input->white_ref_level);
                if (BMX_OPT_PROP_IS_SET(input->color_range))
                    pict_helper->SetColorRange(input->color_range);
                if (BMX_OPT_PROP_IS_SET(input->comp_max_ref))
                    pict_helper->SetComponentMaxRef(input->comp_max_ref);
                if (BMX_OPT_PROP_IS_SET(input->comp_min_ref))
                    pict_helper->SetComponentMinRef(input->comp_min_ref);
                if (BMX_OPT_PROP_IS_SET(input->scan_dir))
                    pict_helper->SetScanningDirection(input->scan_dir);
                if (BMX_OPT_PROP_IS_SET(input->display_primaries))
                    pict_helper->SetMasteringDisplayPrimaries(input->display_primaries);
                if (BMX_OPT_PROP_IS_SET(input->display_white_point))
                    pict_helper->SetMasteringDisplayWhitePointChromaticity(input->display_white_point);
                if (BMX_OPT_PROP_IS_SET(input->display_max_luma))
                    pict_helper->SetMasteringDisplayMaximumLuminance(input->display_max_luma);
                if (BMX_OPT_PROP_IS_SET(input->display_min_luma))
                    pict_helper->SetMasteringDisplayMinimumLuminance(input->display_min_luma);
                if (BMX_OPT_PROP_IS_SET(input->active_width))
                    pict_helper->SetActiveWidth(input->active_width);
                if (BMX_OPT_PROP_IS_SET(input->active_height))
                    pict_helper->SetActiveHeight(input->active_height);
                if (BMX_OPT_PROP_IS_SET(input->active_x_offset))
                    pict_helper->SetActiveXOffset(input->active_x_offset);
                if (BMX_OPT_PROP_IS_SET(input->active_y_offset))
                    pict_helper->SetActiveYOffset(input->active_y_offset);
                if (BMX_OPT_PROP_IS_SET(input->display_f2_offset))
                    pict_helper->SetDisplayF2Offset(input->display_f2_offset);
                if ((BMX_OPT_PROP_IS_SET(input->center_cut_4_3) && input->center_cut_4_3) ||
                    (BMX_OPT_PROP_IS_SET(input->center_cut_14_9) && input->center_cut_14_9))
                {
                    vector<mxfUL> cuts;
                    if (BMX_OPT_PROP_IS_SET(input->center_cut_4_3) && input->center_cut_4_3)
                        cuts.push_back(CENTER_CUT_4_3);
                    if (BMX_OPT_PROP_IS_SET(input->center_cut_14_9) && input->center_cut_14_9)
                        cuts.push_back(CENTER_CUT_14_9);

                    pict_helper->SetAlternativeCenterCuts(cuts);
                }

                RDD36MXFDescriptorHelper *rdd36_helper = dynamic_cast<RDD36MXFDescriptorHelper*>(pict_helper);
                if (rdd36_helper) {
                    if (BMX_OPT_PROP_IS_SET(input->rdd36_opaque))
                        rdd36_helper->SetIsOpaque(input->rdd36_opaque);
                }
            } else if (sound_helper) {
                if (BMX_OPT_PROP_IS_SET(output_sound_info->ref_image_edit_rate))
                    sound_helper->SetReferenceImageEditRate(output_sound_info->ref_image_edit_rate);
                if (BMX_OPT_PROP_IS_SET(output_sound_info->ref_audio_align_level))
                    sound_helper->SetReferenceAudioAlignmentLevel(output_sound_info->ref_audio_align_level);
            }
        }


        // open raw readers and initialise inputs

        for (i = 0; i < input_tracks.size(); i++) {
            RawInputTrack *input_track = input_tracks[i];
            RawInput *input = input_track->GetRawInput();

            if (!input->is_wave && !open_raw_reader(input))
                throw false;

            ClipWriterTrack *clip_track = input_track->GetOutputTrack(0)->GetClipTrack();

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
                        input->raw_reader->SetFixedSampleSize(clip_track->GetInputSampleSize());
                    break;
                case AVC_BASELINE:
                case AVC_CONSTRAINED_BASELINE:
                case AVC_MAIN:
                case AVC_EXTENDED:
                case AVC_HIGH:
                case AVC_HIGH_10:
                case AVC_HIGH_422:
                case AVC_HIGH_444:
                case AVC_HIGH_10_INTRA:
                case AVC_HIGH_422_INTRA:
                case AVC_HIGH_444_INTRA:
                case AVC_CAVLC_444_INTRA:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new AVCEssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case D10_30:
                case D10_40:
                case D10_50:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if ((BMX_REGRESSION_TEST || input->d10_fixed_frame_size) &&
                        input->raw_reader->GetFixedSampleSize() == 0)
                    {
                        input->raw_reader->SetFixedSampleSize(clip_track->GetInputSampleSize());
                    }
                    break;
                case MPEG2LG_422P_ML_576I:
                case MPEG2LG_MP_ML_576I:
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
                case RDD36_422_PROXY:
                case RDD36_422_LT:
                case RDD36_422:
                case RDD36_422_HQ:
                case RDD36_4444:
                case RDD36_4444_XQ:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new RDD36EssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(100000000);
                    break;
                case JPEG2000_CDCI:
                case JPEG2000_RGBA:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new J2CEssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(100000000);
                    break;
                case JPEGXS_CDCI:
                case JPEGXS_RGBA:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new JXSEssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(100000000);
                    input->raw_reader->SetMaxReadLength(30000000);
                    input->raw_reader->SetFrameStartSize(30000000); // 4k, 3bpp, RGB
                    input->raw_reader->SetReadBlockSize(30000000);
                    break;
                case VC2:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    input->raw_reader->SetEssenceParser(new VC2EssenceParser());
                    input->raw_reader->SetCheckMaxSampleSize(100000000);
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
                    input->raw_reader->SetEssenceParser(new MJPEGEssenceParser(clip_track->IsSingleField()));
                    input->raw_reader->SetCheckMaxSampleSize(50000000);
                    break;
                case WAVE_PCM:
                {
                    if (sound_only_container &&
                        (
                            (clip_type == CW_OP1A_CLIP_TYPE && clip->GetOP1AClip()->IsClipWrapped()) ||
                             clip_type == CW_AS02_CLIP_TYPE ||
                             clip_type == CW_WAVE_CLIP_TYPE
                        ))
                    {
                        // Don't restrict the sound to frames if it's sound only and clip wrapped
                        input->sample_sequence[0] = 1;
                        input->sample_sequence_size = 1;
                    }
                    else
                    {
                        vector<uint32_t> shifted_sample_sequence = clip_track->GetShiftedSampleSequence();
                        BMX_ASSERT(shifted_sample_sequence.size() < BMX_ARRAY_SIZE(input->sample_sequence));
                        memcpy(input->sample_sequence, &shifted_sample_sequence[0],
                            shifted_sample_sequence.size() * sizeof(uint32_t));
                        input->sample_sequence_size = shifted_sample_sequence.size();
                    }
                    if (input->raw_reader) {
                        uint32_t sample_size = ((input->bits_per_sample + 7) / 8) * input->channel_count;
                        input->raw_reader->SetFixedSampleSize(sample_size);
                    }
                    break;
                }
                case ANC_DATA:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader && input->anc_const_size)
                        input->raw_reader->SetFixedSampleSize(input->anc_const_size);
                    break;
                case VBI_DATA:
                    input->sample_sequence[0] = 1;
                    input->sample_sequence_size = 1;
                    if (input->raw_reader && input->vbi_const_size)
                        input->raw_reader->SetFixedSampleSize(input->vbi_const_size);
                    break;
                case TIMED_TEXT:
                    break;
                default:
                    BMX_ASSERT(false);
            }
        }


        // embed XML

        if (clip_type == CW_OP1A_CLIP_TYPE ||
            clip_type == CW_RDD9_CLIP_TYPE ||
            clip_type == CW_D10_CLIP_TYPE)
        {
            for (i = 0; i < embed_xml.size(); i++) {
                const EmbedXMLInfo &info = embed_xml[i];
                ClipWriterTrack *xml_track = clip->CreateXMLTrack();
                if (info.scheme_id != g_Null_UL)
                    xml_track->SetXMLSchemeId(info.scheme_id);
                if (info.lang)
                  xml_track->SetXMLLanguageCode(info.lang);
                xml_track->SetXMLSource(info.filename);
            }
        }


        // Add Wave chunk data, descriptors and references

        if (clip_type == CW_OP1A_CLIP_TYPE || clip_type == CW_WAVE_CLIP_TYPE) {
            // Get the output tracks that reference wave file inputs
            set<RawInput*> wave_inputs;
            vector<pair<OutputTrack*, RawInput*> > output_tracks_using_wave;
            for (size_t i = 0; i < output_tracks.size(); i++) {
                OutputTrack *output_track = output_tracks[i];
                if (!output_track->HaveInputTrack())
                    continue;

                // Loop over the input track channels to find wave file inputs
                const map<uint32_t, OutputTrack::InputMap> &input_maps = output_track->GetInputMaps();
                map<uint32_t, OutputTrack::InputMap>::const_iterator iter;
                for (iter = input_maps.begin(); iter != input_maps.end(); iter++) {
                    RawInput *input = dynamic_cast<RawInputTrack*>(iter->second.input_track)->GetRawInput();
                    if (input->essence_type != WAVE_PCM || !input->wave_reader)
                        continue;

                    wave_inputs.insert(input);
                    output_tracks_using_wave.push_back(make_pair(output_track, input));
                }
            }

            if (!wave_inputs.empty()) {
                // These refs are only used in MXF. It is ignored for wave
                map<RawInput*, vector<uint32_t> > input_chunk_refs;

                // Add the (ADM) wave chunk data from the input wave file to the output clip
                set<RawInput*>::const_iterator iter;
                for (iter = wave_inputs.begin(); iter != wave_inputs.end(); iter++) {
                    RawInput *input = *iter;
                    if (input->wave_chunk_refs->empty()) {
                        continue;
                    }
                    WaveReader *wave_reader = input->wave_reader;

                    map<string, WaveChunkRef>::const_iterator refs_iter;
                    for (refs_iter = input->wave_chunk_refs->begin(); refs_iter != input->wave_chunk_refs->end(); refs_iter++) {
                        WaveChunkId chunk_id = WAVE_CHUNK_ID(refs_iter->first.c_str());
                        const WaveChunkRef &ref = refs_iter->second;

                        WaveFileChunk *chunk = wave_reader->GetAdditionalChunk(chunk_id);
                        if (chunk) {
                            if (clip_type == CW_WAVE_CLIP_TYPE) {
                                WaveWriter *wave_clip = clip->GetWaveClip();
                                if (wave_clip->HaveChunk(chunk->Id())) {
                                    // E.g. this can happen if multiple <axml> chunks exist that came from different Wave files
                                    // and the <axml> chunks may or may not be identical or equivalent.
                                    // The assumption is that only 1 chunk with a given ID can exist in a BWF64 / Wave file.
                                    // The exception is probably <JUNK>, but that shouldn't be transferred between Wave or MXF.
                                    log_warn("Replaced chunk <%s> with another with the same ID in the output\n",
                                             get_wave_chunk_id_str(chunk->Id()).c_str());
                                }
                            }

                            uint32_t stream_id;
                            if (ref.is_adm)
                                stream_id = clip->AddADMWaveChunk(chunk, false, ref.profile_and_level_uls);
                            else
                                stream_id = clip->AddWaveChunk(chunk, false);
                            input_chunk_refs[input].push_back(stream_id);
                        } else {
                            if (WaveReader::IsBuiltinChunk(chunk_id)) {
                                log_warn("WAVE chunk '%s' is a builtin\n", refs_iter->first.c_str());
                            } else {
                                log_warn("WAVE chunk '%s' does not exist in the input\n", refs_iter->first.c_str());
                            }
                        }
                    }
                }

                // Add references to the wave chunk streams to the MXF output clip tracks
                if (clip_type != CW_WAVE_CLIP_TYPE) {
                    for (size_t i = 0; i < output_tracks_using_wave.size(); i++) {
                        OutputTrack *output_track = output_tracks_using_wave[i].first;
                        RawInput *wave_input = output_tracks_using_wave[i].second;

                        ClipWriterTrack *clip_track = output_track->GetClipTrack();

                        const vector<uint32_t> &chunk_refs = input_chunk_refs[wave_input];
                        for (size_t k = 0; k < chunk_refs.size(); k++) {
                            clip_track->AddWaveChunkReference(chunk_refs[k]);
                        }
                    }
                }
            }
        }


        // Map <chna> chunks from the input wave file to MXF descriptors

        if (clip_type == CW_OP1A_CLIP_TYPE || clip_type == CW_WAVE_CLIP_TYPE) {
            if (clip_type == CW_WAVE_CLIP_TYPE) {
                // Ensure that the start channels are up-to-date for each wave output track
                WaveWriter *wave_clip = clip->GetWaveClip();
                wave_clip->UpdateChannelCounts();
            }

            // Loop over the output tracks
            set<RawInput*> checked_wave_inputs;
            vector<pair<OutputTrack*, RawInput*> > output_tracks_using_chna;
            for (i = 0; i < output_tracks.size(); i++) {
                OutputTrack *output_track = output_tracks[i];
                if (!output_track->HaveInputTrack())
                    continue;

                ClipWriterTrack *clip_track = output_track->GetClipTrack();

                // Loop over the input track channels for this output track, adding the input <chna> audio IDs
                const map<uint32_t, OutputTrack::InputMap> &input_maps = output_track->GetInputMaps();
                map<uint32_t, OutputTrack::InputMap>::const_iterator iter;
                for (iter = input_maps.begin(); iter != input_maps.end(); iter++) {
                    RawInput *input = dynamic_cast<RawInputTrack*>(iter->second.input_track)->GetRawInput();
                    if (input->essence_type != WAVE_PCM || !input->wave_reader)
                        continue;

                    uint32_t output_channel_index = iter->first;
                    uint32_t input_channel_index = iter->second.input_channel_index;

                    // Check that the input channel has a <chna> audio ID and that is should be read
                    if (input->no_chna_chunk)
                        continue;
                    WaveCHNA *input_chna = input->wave_reader->GetCHNA();
                    if (!input_chna)
                        continue;

                    // Warn if there are no wave chunks transferred whilst there are known ADM chunks in the input
                    if (!checked_wave_inputs.count(input)) {
                        if (input->wave_chunk_refs->empty()) {
                            string available_adm_chunks;
                            const char *adm_chunks[] = {"axml", "bxml", "sxml"};
                            for (size_t i = 0; i < BMX_ARRAY_SIZE(adm_chunks); i++) {
                                if (input->wave_reader->GetAdditionalChunk(WAVE_CHUNK_ID(adm_chunks[i]))) {
                                    if (!available_adm_chunks.empty())
                                        available_adm_chunks += ", ";
                                    available_adm_chunks += "<" + string(adm_chunks[i]) + ">";
                                }
                            }
                            if (!available_adm_chunks.empty()) {
                                log_warn("Mapping <chna> chunk with no ADM chunks, but %s is available. "
                                            "Use --wave-chunks or --adm-wave-chunk options to add them\n",
                                            available_adm_chunks.c_str());
                            }
                        }
                        checked_wave_inputs.insert(input);
                    }

                    // + 1 below because ADM track (channel) indexes start at 1
                    vector<WaveCHNA::AudioID> audio_ids = input_chna->GetAudioIDs(input_channel_index + 1);
                    for (size_t k = 0; k < audio_ids.size(); k++) {
                        WaveCHNA::AudioID &audio_id = audio_ids[k];

                        // + 1 below because ADM track (channel) indexes start at 1
                        audio_id.track_index = output_channel_index + 1;
                        clip_track->AddADMAudioID(audio_id);
                    }
                }
            }
        }


        // Add Wave chunks provided via commandline args.
        // These chunks will override any chunks added above

        if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();
            size_t i;
            for (i = 0; i < wave_chunk_datas.size(); i++) {
                const WaveChunkData &wave_chunk_data = wave_chunk_datas[i];

                // Use the WaveFileIO class as a generic BMXIO class
                BMXIO *file = WaveFileIO::OpenRead(wave_chunk_data.filename);

                WaveFileChunk *chunk = new WaveFileChunk(wave_chunk_data.id, file, true, 0, (uint32_t)file->Size());
                wave_clip->AddChunk(chunk, true);
            }

            if (chna_audio_ids_file) {
                WaveCHNA *chna = parse_chna_text_file(chna_audio_ids_file);
                wave_clip->AddCHNA(chna, true);
            }
        }


        // prepare the clip's header metadata

        clip->PrepareHeaderMetadata();


        // add AS-11 descriptive metadata

        if (clip_sub_type == AS11_CLIP_SUB_TYPE)
            as11_helper.AddMetadata(clip);
        else if (clip_sub_type == AS10_CLIP_SUB_TYPE)
            as10_helper.AddMetadata(clip);


        // insert MCA labels
        // Note: this must happen after processing wave chunks because the ADM soundfield group label requires
        // the MXF stream ID for a given wave chunk ID

        for (i = 0; i < track_mca_labels.size(); i++) {
            const string &scheme = track_mca_labels[i].first;
            const string &labels_filename = track_mca_labels[i].second;

            AppMCALabelHelper label_helper(scheme == "as11");
            if (!label_helper.ParseTrackLabels(labels_filename)) {
                log_error("Failed to parse audio labels file '%s'\n", labels_filename.c_str());
                throw false;
            }
            label_helper.InsertTrackLabels(clip);
        }


        // read more than 1 sample to improve efficiency if the input is sound only and the output
        // doesn't require a sample sequence

        uint32_t max_samples_per_read = 1;
        if (sound_only_container) {
            bool all_have_sound_sample_sequence = true;
            for (i = 0; i < inputs.size(); i++) {
                RawInput *input = &inputs[i];
                if (input->disabled)
                    continue;

                if (input->essence_type == WAVE_PCM &&
                    input->sample_sequence_size == 1 &&
                    input->sample_sequence[0] == 1)
                {
                    all_have_sound_sample_sequence = false;
                    break;
                }
            }
            if (!all_have_sound_sample_sequence)
                max_samples_per_read = 1920;
        }


        // realtime wrapping

        uint64_t rt_start = 0;
        if (realtime)
            rt_start = get_tick_count();


        // create clip file(s) and write samples

        clip->PrepareWrite();

        // write samples
        bmx::ByteArray pcm_buffer;
        int64_t total_read = 0;
        while (have_samples_to_write && (duration < 0 || total_read < duration)) {
            // break if reached end of partial file regression test
            if (regtest_end >= 0 && total_read >= regtest_end)
                break;

            // read samples into input buffers first to ensure the frame data is available for all tracks
            uint32_t min_num_samples = max_samples_per_read;
            uint32_t num_samples;
            for (i = 0; i < inputs.size(); i++) {
                RawInput *input = &inputs[i];
                if (!input->disabled && input->essence_type != TIMED_TEXT) {
                    num_samples = read_samples(input, max_samples_per_read);
                    if (num_samples < min_num_samples) {
                        min_num_samples = num_samples;
                        if (min_num_samples == 0)
                            break;
                    }
                }
            }
            if (min_num_samples == 0)
                break;
            BMX_ASSERT(min_num_samples == max_samples_per_read || max_samples_per_read > 1);
            total_read += min_num_samples;

            // write samples from input buffers
            uint32_t first_sound_num_samples = 0;
            for (i = 0; i < input_tracks.size(); i++) {
                RawInputTrack *input_track = input_tracks[i];
                RawInput *input = input_track->GetRawInput();
                if (input->essence_type == TIMED_TEXT) {
                    continue;
                }

                size_t k;
                for (k = 0; k < input_track->GetOutputTrackCount(); k++) {
                    OutputTrack *output_track = input_track->GetOutputTrack(k);
                    uint32_t output_channel_index = input_track->GetOutputChannelIndex(k);
                    uint32_t input_channel_index = input_track->GetInputChannelIndex(k);

                    if (input->raw_reader) {
                        num_samples = input->raw_reader->GetNumSamples();
                        if (max_samples_per_read > 1 && num_samples > min_num_samples)
                            num_samples = min_num_samples;
                        if (input->essence_type == WAVE_PCM && input->channel_count > 1) {
                            pcm_buffer.Allocate(input->raw_reader->GetSampleDataSize() / input->channel_count);
                            deinterleave_audio(input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize(),
                                               input->bits_per_sample, input->channel_count, input_channel_index,
                                               pcm_buffer.GetBytes(), pcm_buffer.GetAllocatedSize());
                            pcm_buffer.SetSize(input->raw_reader->GetSampleDataSize() / input->channel_count);
                            output_track->WriteSamples(output_channel_index,
                                                       pcm_buffer.GetBytes(), pcm_buffer.GetSize(),
                                                       num_samples);
                        } else {
                            output_track->WriteSamples(output_channel_index,
                                                       input->raw_reader->GetSampleData(), input->raw_reader->GetSampleDataSize(),
                                                       num_samples);
                        }
                    } else if (input->klv_reader) {
                        num_samples = input->klv_reader->GetValueSize() ? 1 : 0;
                        if (num_samples) {
                            output_track->WriteSamples(
                                output_channel_index,
                                input->klv_reader->GetValue(),
                                input->klv_reader->GetValueSize(),
                                num_samples
                            );
                        }
                    } else {
                        Frame *frame = input->wave_reader->GetTrack(input_channel_index)->GetFrameBuffer()->GetLastFrame(false);
                        BMX_ASSERT(frame);
                        num_samples = frame->num_samples;
                        if (max_samples_per_read > 1 && num_samples > min_num_samples)
                            num_samples = min_num_samples;
                        output_track->WriteSamples(output_channel_index,
                                                   (unsigned char*)frame->GetBytes(), frame->GetSize(),
                                                   num_samples);
                        // frames popped and deleted in read_samples()
                    }

                    if (convert_essence_type_to_data_def(input->essence_type) == MXF_SOUND_DDEF &&
                        first_sound_num_samples == 0 && num_samples > 0)
                    {
                        first_sound_num_samples = num_samples;
                    }
                }
            }

            // write samples for silence tracks
            for (i = 0; i < output_tracks.size(); i++) {
                OutputTrack *output_track = output_tracks[i];
                if (output_track->IsSilenceTrack())
                    output_track->WriteSilenceSamples(first_sound_num_samples);
            }

            if (update_header) {
                clip->UpdateHeaderMetadata();
                update_header = false;
            }

            if (min_num_samples < max_samples_per_read)
                break;

            if (realtime)
                rt_sleep(rt_factor, rt_start, frame_rate, total_read);
        }


        if (regtest_end < 0) { // only complete if not regression testing partial files

            // complete AS-10/11 descriptive metadata

            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                as11_helper.Complete();
            else if (clip_sub_type == AS10_CLIP_SUB_TYPE)
                as10_helper.Complete();


            // complete writing

            clip->CompleteWrite();

            log_info("Duration: %" PRId64 " (%s)\n",
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
        for (i = 0; i < output_tracks.size(); i++)
            delete output_tracks[i];
        for (i = 0; i < input_tracks.size(); i++)
            delete input_tracks[i];
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
