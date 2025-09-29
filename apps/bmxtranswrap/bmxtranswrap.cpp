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
#include <map>
#include <algorithm>

#include "MXFInputTrack.h"
#include "../writers/OutputTrack.h"
#include "../writers/TrackMapper.h"
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/MXFGroupReader.h>
#include <bmx/mxf_reader/MXFSequenceReader.h>
#include <bmx/mxf_reader/MXFFrameMetadata.h>
#include <bmx/mxf_reader/MXFTimedTextTrackReader.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/essence_parser/MPEG2AspectRatioFilter.h>
#include <bmx/mxf_helper/RDD36MXFDescriptorHelper.h>
#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/wave/WaveFileIO.h>
#include <bmx/wave/WaveChunk.h>
#include <bmx/st436/ST436Element.h>
#include <bmx/st436/RDD6Metadata.h>
#include <bmx/URI.h>
#include <bmx/MXFHTTPFile.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Version.h>
#include <bmx/as11/AS11Labels.h>
#include <bmx/as10/AS10ShimNames.h>
#include <bmx/as10/AS10MPEG2Validator.h>
#include <bmx/as10/AS10RDD9Validator.h>
#include <bmx/apps/AppMCALabelHelper.h>
#include <bmx/apps/AppMXFFileFactory.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/apps/AS11Helper.h>
#include <bmx/apps/AS10Helper.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define DEFAULT_GF_RETRIES          10
#define DEFAULT_GF_RETRY_DELAY      1.0
#define DEFAULT_GF_RATE_AFTER_FAIL  1.5

#define DEFAULT_ST436_MANIFEST_COUNT    2


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


static const char APP_NAME[]                = "bmxtranswrap";

static const char DEFAULT_SHIM_NAME[]       = "Sample File";
static const char DEFAULT_SHIM_ID[]         = "http://bbc.co.uk/rd/as02/default-shim.txt";
static const char DEFAULT_SHIM_ANNOTATION[] = "Default AS-02 shim";

static const char DEFAULT_BEXT_ORIGINATOR[] = "bmx";

static const uint32_t DEFAULT_RW_INTL_SIZE  = (64 * 1024);

static const uint16_t DEFAULT_RDD6_LINES[2] = {9, 572};     /* ST 274, line 9 field 1 and 2 */
static const uint8_t DEFAULT_RDD6_SDID      = 4;            /* first channel pair is 5/6 */

static const uint32_t DEFAULT_HTTP_MIN_READ = 1024 * 1024;


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};



static bool regtest_output_track_map_comp(const TrackMapper::OutputTrackMap &left,
                                          const TrackMapper::OutputTrackMap &right)
{
    return left.data_def < right.data_def;
}

static bool filter_passing_anc_data_type(ANCDataType data_type, set<ANCDataType> &filter)
{
    set<ANCDataType>::const_iterator iter;
    for (iter = filter.begin(); iter != filter.end(); iter++) {
        if (*iter == ALL_ANC_DATA || *iter == data_type)
            return true;
    }

    return false;
}

static bool passing_anc_data_type(ANCDataType data_type, set<ANCDataType> &pass_filter, set<ANCDataType> &strip_filter)
{
    bool passing = false;
    if (!pass_filter.empty())
        passing = filter_passing_anc_data_type(data_type, pass_filter);
    else if (!strip_filter.empty())
        passing = true; // Default of pass filter changes to ALL_ANC_DATA if strip filter not empty

    if (passing && !strip_filter.empty())
        passing = !filter_passing_anc_data_type(data_type, strip_filter);

    return passing;
}

static bool single_filter_anc_manifest_element(const ANCManifestElement *element, set<ANCDataType> &filter)
{
    set<ANCDataType>::const_iterator iter;
    for (iter = filter.begin(); iter != filter.end(); iter++) {
        if (*iter == ALL_ANC_DATA) {
            return true;
        } else if (*iter == ST2020_ANC_DATA) {
            if (element->did == 0x45)
                return true;
        } else if (*iter == ST2016_ANC_DATA) {
            if (element->did  == 0x41 && (element->sdid == 0x05 || element->sdid == 0x06))
                return true;
        } else if (*iter == RDD8_SDP_ANC_DATA) {
            if (element->did  == 0x43 && element->sdid == 0x02)
                return true;
        } else if (*iter == ST12M_ANC_DATA) {
            if (element->did  == 0x60 && element->sdid == 0x60)
                return true;
        } else if (*iter == ST334_ANC_DATA) {
            if ((element->did == 0x61 && (element->sdid == 0x01 || element->sdid == 0x02)) ||
                (element->did == 0x62 && (element->sdid == 0x02)))
            {
                return true;
            }
        }
    }

    return false;
}

static bool filter_anc_manifest_element(const ANCManifestElement *element, set<ANCDataType> &pass_filter, set<ANCDataType> &strip_filter)
{
    bool pass = false;
    if (!pass_filter.empty())
        pass = single_filter_anc_manifest_element(element, pass_filter);
    else if (!strip_filter.empty())
        pass = true; // Default of pass filter changes to ALL_ANC_DATA if strip filter not empty

    if (pass && !strip_filter.empty())
        pass = !single_filter_anc_manifest_element(element, strip_filter);

    return pass;
}

static bool filter_anc_manifest(const MXFDataTrackInfo *data_info, set<ANCDataType> &pass_filter, set<ANCDataType> &strip_filter)
{
    size_t i;
    for (i = 0; i < data_info->anc_manifest.size(); i++) {
        if (filter_anc_manifest_element(&data_info->anc_manifest[i], pass_filter, strip_filter))
            return true;
    }

    return false;
}

static uint32_t calc_st2020_max_size(bool sample_coding_10bit, uint32_t line_count)
{
    // The maximum ANC packet size is limited by the Data Count (DC) byte and equals
    //     DID (1) + SDID (1) + DC (1) + UDW (255) + CS (1) = 259 samples
    // The ST 436 payload byte array data must be padded to a UInt32 boundary and therefore the
    //   max size is 260 samples for 8-bit sample coding. For 10-bit encoding 3 samples are stored
    //   in 4 bytes with 2 padding bits
    uint32_t max_array_size;
    if (sample_coding_10bit)
        max_array_size = (259 + 2) / 3 * 4;
    else
        max_array_size = 260;

    // A maximum of 2 ANC packets are required for an audio program metadata frame
    // The maximum number of packets per video frame is 2
    //     Method B: 2 packets if video frame rate <= 30 Hz; Method A: 2 packets if audio metadata exceeds max packet size
    // ST 436 ANC element starts with a 2 byte packet count followed by (14 byte header + array data) for each packet

    return 2 + line_count * 2 * (14 + max_array_size);
}

static uint32_t calc_st2020_max_size(const MXFDataTrackInfo *data_info)
{
    // The SDID identifies the first channel pair in the audio program
    // Assume the audio program count equals the number of unique SDID values
    // Also check for 10-bit sample coding
    bool sample_coding_10bit = false;
    set<uint8_t> sdids;
    size_t i;
    for (i = 0; i < data_info->anc_manifest.size(); i++) {
        if (data_info->anc_manifest[i].did == 0x45) {
            sdids.insert(data_info->anc_manifest[i].sdid);
            if (data_info->anc_manifest[i].sample_coding == ANC_10_BIT_COMP_LUMA ||
                data_info->anc_manifest[i].sample_coding == ANC_10_BIT_COMP_COLOR ||
                data_info->anc_manifest[i].sample_coding == ANC_10_BIT_COMP_LUMA_COLOR)
            {
                sample_coding_10bit = true;
            }
        }
    }
    if (sdids.empty()) {
        log_warn("Unable to calculate the maximum ANC frame element data size for ST 2020 because extracted manifest "
                 "includes no ST 2020 data");
        return 0;
    }

    return calc_st2020_max_size(sample_coding_10bit, (uint32_t)sdids.size());
}

static void construct_anc_rdd6_sub_frame(RDD6MetadataFrame *rdd6_frame,
                                         bool is_first_sub_frame,
                                         bmx::ByteArray *rdd6_buffer,
                                         uint8_t sdid, uint16_t line_number,
                                         bmx::ByteArray *anc_buffer)
{
    rdd6_buffer->SetSize(0);
    rdd6_frame->ConstructST2020(rdd6_buffer, sdid, is_first_sub_frame);

    ST436Element output_element(false);
    ST436Line line(false);
    line.wrapping_type         = VANC_FRAME;
    line.payload_sample_coding = ANC_8_BIT_COMP_LUMA;
    line.line_number           = line_number;
    line.payload_sample_count  = rdd6_buffer->GetSize();
    line.payload_data          = rdd6_buffer->GetBytes();
    line.payload_size          = rdd6_buffer->GetSize(); // alignment left to ST436Element::Construct
    output_element.lines.push_back(line);

    anc_buffer->SetSize(0);
    output_element.Construct(anc_buffer);
}

static void construct_anc_rdd6(RDD6MetadataFrame *rdd6_frame,
                               bmx::ByteArray *rdd6_first_buffer, bmx::ByteArray *rdd6_second_buffer,
                               uint8_t sdid, uint16_t *line_numbers,
                               bmx::ByteArray *anc_buffer)
{
    rdd6_first_buffer->SetSize(0);
    rdd6_second_buffer->SetSize(0);
    rdd6_frame->ConstructST2020(rdd6_first_buffer,  sdid, true);
    rdd6_frame->ConstructST2020(rdd6_second_buffer, sdid, false);

    ST436Element output_element(false);
    ST436Line line(false);
    line.wrapping_type         = VANC_FRAME;
    line.payload_sample_coding = ANC_8_BIT_COMP_LUMA;
    line.line_number           = line_numbers[0];
    line.payload_sample_count  = rdd6_first_buffer->GetSize();
    line.payload_data          = rdd6_first_buffer->GetBytes();
    line.payload_size          = rdd6_first_buffer->GetSize(); // alignment left to ST436Element::Construct
    output_element.lines.push_back(line);

    line.line_number           = line_numbers[1];
    line.payload_sample_count  = rdd6_second_buffer->GetSize();
    line.payload_data          = rdd6_second_buffer->GetBytes();
    line.payload_size          = rdd6_second_buffer->GetSize(); // alignment left to ST436Element::Construct
    output_element.lines.push_back(line);

    anc_buffer->SetSize(0);
    output_element.Construct(anc_buffer);
}

static uint32_t read_samples(MXFReader *reader, const vector<uint32_t> &sample_sequence,
                             uint32_t *sample_sequence_offset, uint32_t max_samples_per_read)
{
    uint32_t num_read;

    if (max_samples_per_read == 1) {
        uint32_t num_frame_samples = sample_sequence[*sample_sequence_offset];
        *sample_sequence_offset = (*sample_sequence_offset + 1) % sample_sequence.size();

        num_read = reader->Read(num_frame_samples);
        if (num_read != num_frame_samples)
            num_read = 0;
    } else {
        BMX_ASSERT(sample_sequence.size() == 1 && sample_sequence[0] == 1);
        num_read = reader->Read(max_samples_per_read);
    }

    return num_read;
}

static void write_anc_samples(OutputTrack *output_track, Frame *frame, set<ANCDataType> &pass_filter, set<ANCDataType> &strip_filter, bmx::ByteArray &anc_buffer)
{
    BMX_CHECK(frame->num_samples == 1);

    if (passing_anc_data_type(ALL_ANC_DATA, pass_filter, strip_filter)) {
        output_track->WriteSamples(0, (unsigned char*)frame->GetBytes(), frame->GetSize(), frame->num_samples);
        return;
    }

    ST436Element input_element(false);
    input_element.Parse(frame->GetBytes(), frame->GetSize());

    ST436Element output_element(false);
    size_t i;
    for (i = 0; i < input_element.lines.size(); i++) {
        ANCManifestElement manifest_element;
        manifest_element.Parse(&input_element.lines[i]);
        if (filter_anc_manifest_element(&manifest_element, pass_filter, strip_filter))
            output_element.lines.push_back(input_element.lines[i]);
    }

    anc_buffer.SetSize(0);
    output_element.Construct(&anc_buffer);

    output_track->WriteSamples(0, anc_buffer.GetBytes(), anc_buffer.GetSize(), 1);
}

static void disable_tracks(MXFReader *reader, const set<size_t> &track_indexes,
                           bool disable_audio, bool disable_video, bool disable_data)
{
    size_t i;
    for (i = 0; i < reader->GetNumTrackReaders(); i++) {
        if (track_indexes.count(i) ||
            (disable_audio && reader->GetTrackReader(i)->GetTrackInfo()->data_def == MXF_SOUND_DDEF) ||
            (disable_video && reader->GetTrackReader(i)->GetTrackInfo()->data_def == MXF_PICTURE_DDEF) ||
            (disable_data  && reader->GetTrackReader(i)->GetTrackInfo()->data_def == MXF_DATA_DDEF))
        {
            reader->GetTrackReader(i)->SetEnable(false);
        }
    }
}

EssenceType process_assumed_essence_type(const MXFTrackInfo *input_track_info, EssenceType assume_d10_essence_type)
{
    // Map the essence type if generic MPEG video is assumed to be D-10
    if (assume_d10_essence_type != UNKNOWN_ESSENCE_TYPE &&
            (input_track_info->essence_type == PICTURE_ESSENCE ||
                input_track_info->essence_type == MPEG2LG_422P_ML_576I) &&
            (mxf_is_mpeg_video_ec(&input_track_info->essence_container_label, 1) ||
                mxf_is_mpeg_video_ec(&input_track_info->essence_container_label, 0)))
    {
        return assume_d10_essence_type;
    }
    else
    {
        return input_track_info->essence_type;
    }
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
    printf("Re-wrap from one MXF file to another MXF file\n");
    printf("\n");
    printf("Usage: %s <<Options>> [<<Input Options>> <mxf input>]+\n", strip_path(cmd).c_str());
    printf("   Use <mxf input> '-' for standard input\n");
    printf("Options (* means option is required):\n");
    printf("  -h | --help             Show usage and exit\n");
    printf("  -v | --version          Print version info\n");
    printf("  -p                      Print progress percentage to stdout\n");
    printf("  -l <file>               Log filename. Default log to stderr/stdout\n");
    printf(" --log-level <level>      Set the log level. 0=debug, 1=info, 2=warning, 3=error. Default is 1\n");
    printf("  -t <type>               Clip type: as02, as11op1a, as11d10, as11rdd9, op1a, avid, d10, rdd9, as10, wave, imf. Default is op1a\n");
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
    printf("  --input-file-md5        Calculate an MD5 checksum of the input file\n");
    printf("  -y <hh:mm:sscff>        Override input start timecode. Default 00:00:00:00\n");
    printf("                          The c character in the pattern should be ':' for non-drop frame; any other character indicates drop frame\n");
    printf("  --mtc                   Check first and use the input material package start timecode if present\n");
    printf("  --fstc                  Check first and use the file source package timecode if present\n");
    printf("  --pstc                  Check first and use the physical source package timecode if present\n");
    printf("  --tc-rate <rate>        Start timecode rate to use when input is audio only\n");
    printf("                          The <rate> is either 'num', 'num'/'den', 23976 (=24000/1001), 2997 (=30000/1001) or 5994 (=60000/1001)\n");
    printf("  --clip <name>           Set the clip name\n");
    printf("  --start <count>         Set the start in input edit rate units. Default is 0\n");
    printf("  --dur <count>           Set the duration in input edit rate units. Default is minimum input duration\n");
    printf("  --check-end             Check at the start that the last (start + duration - 1) edit unit can be read\n");
    printf("  --check-end-tolerance <frame> Allow output duration shorter than input declared duration\n");
    printf("  --check-complete        Check that the input file structure can be read and is complete\n");
    printf("  --group                 Use the group reader instead of the sequence reader\n");
    printf("                          Use this option if the files have different material packages\n");
    printf("                          but actually belong to the same virtual package / group\n");
    printf("  --no-reorder            Don't attempt to order the inputs in a sequence\n");
    printf("                          Use this option for files with broken timecode\n");
    printf("  --rt <factor>           Transwrap at realtime rate x <factor>, where <factor> is a floating point value\n");
    printf("                          <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
    printf("  --gf                    Support growing files. Retry reading a frame when it fails\n");
    printf("  --gf-retries <max>      Set the maximum times to retry reading a frame. The default is %u.\n", DEFAULT_GF_RETRIES);
    printf("  --gf-delay <sec>        Set the delay (in seconds) between a failure to read and a retry. The default is %f.\n", DEFAULT_GF_RETRY_DELAY);
    printf("  --gf-rate <factor>      Limit the read rate to realtime rate x <factor> after a read failure. The default is %f\n", DEFAULT_GF_RATE_AFTER_FAIL);
    printf("                          <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
    printf(" --disable-indexing-file   Use this option to stop the reader creating an index of the partitions and essence positions in the file up front\n");
    printf("                           This option can be used to avoid indexing files containing many partitions\n");
    if (mxf_http_is_supported()) {
        printf(" --http-min-read <bytes>\n");
        printf("                          Set the minimum number of bytes to read when accessing a file over HTTP. The default is %u.\n", DEFAULT_HTTP_MIN_READ);
        printf(" --http-disable-seek      Disable seeking when reading file over HTTP\n");
    }
    printf("  --no-precharge          Don't output clip/track with precharge. Adjust the start position and duration instead\n");
    printf("  --no-rollout            Don't output clip/track with rollout. Adjust the duration instead\n");
    printf("  --rw-intl               Interleave input reads with output writes\n");
    printf("  --rw-intl-size          The interleave size. Default is %u\n", DEFAULT_RW_INTL_SIZE);
    printf("                          Value must be a multiple of the system page size, %u\n", mxf_get_system_page_size());
#if defined(_WIN32)
    printf("  --seq-scan              Set the sequential scan hint for optimizing file caching whilst reading\n");
#if !defined(__MINGW32__)
    printf("  --mmap-file             Use memory-mapped file I/O for the MXF files\n");
    printf("                          Note: this may reduce file I/O performance and was found to be slower over network drives\n");
#endif
#endif
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
    printf("  -a <n:d>                Override or set the image aspect ratio\n");
    printf("  --bsar                  Set image aspect ratio in video bitstream. Currently supports D-10 essence types only\n");
    printf("  --vc2-mode <mode>       Set the mode that determines how the VC-2 data is wrapped\n");
    printf("                          <mode> is one of the following integer values:\n");
    printf("                            0: Passthrough input, but add a sequence header if not present, remove duplicate/redundant sequence headers\n");
    printf("                               and fix any incorrect parse info offsets and picture numbers\n");
    printf("                            1: (default) Same as 0, but remove auxiliary and padding data units and complete the sequence in each frame\n");
    printf("  --locked <bool>         Override or set flag indicating whether the number of audio samples is locked to the video. Either true or false\n");
    printf("  --audio-ref <level>     Override or set audio reference level, number of dBm for 0VU\n");
    printf("  --dial-norm <value>     Override or set gain to be applied to normalize perceived loudness of the clip\n");
    printf("  --ref-image-edit-rate <rate>     Override or set the Reference Image Edit Rate\n");
    printf("                                   The <rate> is either 'num', 'num'/'den', 23976 (=24000/1001), 2997 (=30000/1001) or 5994 (=60000/1001)\n");
    printf("  --ref-audio-align-level <value>  Override or set the Reference Audio Alignment Level\n");
    printf("  --signal-std  <value>   Override or set the video signal standard. The <value> is one of the following:\n");
    printf("                              'none', 'bt601', 'bt1358', 'st347', 'st274', 'st296', 'st349', 'st428'\n");
    printf("  --frame-layout <value>  Override or set the video frame layout. The <value> is one of the following:\n");
    printf("                              'fullframe', 'separatefield', 'singlefield', 'mixedfield', 'segmentedframe'\n");
    printf("  --field-dom <value>     Override or set which field is first in temporal order. The <value> is 1 or 2\n");
    printf("  --video-line-map <value>  Override or set the video line map. The <value> is 2 line numbers separated by a comma\n");
    printf("  --transfer-ch <value>   Override or set the transfer characteristic label\n");
    printf("                          The <value> is a SMPTE UL, formatted as a 'urn:smpte:ul:...' or one of the following:\n");
    printf("                              'bt470', 'bt709', 'st240', 'st274', 'bt1361', 'linear', 'dcdm',\n");
    printf("                              'iec61966', 'bt2020', 'st2084', 'hlg'\n");
    printf("  --coding-eq <value>     Override or set the coding equations label\n");
    printf("                          The <value> is a SMPTE UL, formatted as a 'urn:smpte:ul:...' or one of the following:\n");
    printf("                              'bt601', 'bt709', 'st240', 'ycgco', 'gbr', 'bt2020'\n");
    printf("  --color-prim <value>    Override or set the color primaries label\n");
    printf("                          The <value> is a SMPTE UL, formatted as a 'urn:smpte:ul:...' or one of the following:\n");
    printf("                              'st170', 'bt470', 'bt709', 'bt2020', 'dcdm', 'p3'\n");
    printf("  --color-siting <value>  Override or set the color siting. The <value> is one of the following:\n");
    printf("                              'cositing', 'horizmp', '3tap', 'quincunx', 'bt601', 'linealt', 'vertmp', 'unknown'\n");
    printf("                              (Note that 'bt601' is deprecated in SMPTE ST 377-1. Use 'cositing' instead)\n");
    printf("  --black-level <value>   Override or set the black reference level\n");
    printf("  --white-level <value>   Override or set the white reference level\n");
    printf("  --color-range <value>   Override or set the color range\n");
    printf("  --comp-max-ref <value>  Override or set the RGBA component maximum reference level\n");
    printf("  --comp-min-ref <value>  Override or set the RGBA component minimum reference level\n");
    printf("  --scan-dir <value>      Override or set the RGBA scanning direction\n");
    printf("  --display-primaries <value>    Override or set the mastering display primaries.\n");
    printf("                                 The <value> is an array of 6 unsigned integers separated by a ','.\n");
    printf("  --display-white-point <value>  Override or set the mastering display white point chromaticity.\n");
    printf("                                 The <value> is an array of 2 unsigned integers separated by a ','.\n");
    printf("  --display-max-luma <value>     Override or set the mastering display maximum luminance.\n");
    printf("  --display-min-luma <value>     Override or set the mastering display minimum luminance.\n");
    printf("  --rdd36-opaque              Override and treat RDD-36 4444 or 4444 XQ as opaque by omitting the Alpha Sample Depth property\n");
    printf("  --rdd36-comp-depth <value>  Override of set component depth for RDD-36. Defaults to 10 if not present in input file\n");
    printf("  --active-width          Override or set the Active Width of the active area rectangle\n");
    printf("  --active-height         Override or set the Active Height of the active area rectangle\n");
    printf("  --active-x-offset       Override or set the Active X Offset of the active area rectangle\n");
    printf("  --active-y-offset       Override or set the Active Y Offset of the active area rectangle\n");
    printf("  --display-f2-offset     Override or set the default Display F2 Offset if it is not extracted from the essence\n");
    printf("  --center-cut-4-3        Override or add the Alternative Center Cut 4:3\n");
    printf("  --center-cut-14-9       Override or add the Alternative Center Cut 14:9\n");
    printf("  --ignore-input-desc     Don't use input MXF file descriptor properties to fill in missing information\n");
    printf("  --track-map <expr>      Map input audio channels to output tracks\n");
    printf("                          The default is 'mono', except if --clip-wrap option is set for op1a it is 'singlemca'\n");
    printf("                          See below for details of the <expr> format\n");
    printf("  --dump-track-map        Dump the output audio track map to stderr.\n");
    printf("                          The dumps consists of a list output tracks, where each output track channel\n");
    printf("                          is shown as '<output track channel> <- <input channel>\n");
    printf("  --dump-track-map-exit   Same as --dump-track-map, but exit immediately afterwards\n");
    printf("  --assume-d10-30         Assume a generic MPEG video elementary stream is actually D-10 30\n");
    printf("  --assume-d10-40         Assume a generic MPEG video elementary stream is actually D-10 40\n");
    printf("  --assume-d10-50         Assume a generic MPEG video elementary stream is actually D-10 50\n");
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
    printf("    --part <interval>       Video essence partition interval in frames in input edit rate units, or (floating point) seconds with 's' suffix. Default single partition\n");
    printf("  rdd9:\n");
    printf("    --fixed-part <interval> Force each partition to have the exact same partition interval in frames, except the last partition\n");
    printf("                            New partitions are started if the frame count has been reached, even if the next partition does not begin with the start of a GOP\n");
    printf("\n");
    printf("  as11op1a/as11d10/as11rdd9:\n");
    printf("    --dm <fwork> <name> <value>    Set descriptive framework property. <fwork> is 'as11' or 'dpp'\n");
    printf("    --dm-file <fwork> <name>       Parse and set descriptive framework properties from text file <name>. <fwork> is 'as11' or 'dpp'\n");
    printf("    --seg <name>                   Parse and set segmentation data from text file <name>\n");
    printf("    --pass-dm                      Copy descriptive metadata from the input file. The metadata can be overidden by other options\n");
    printf("    --norm-pass-dm                 Same as --pass-dm, but also normalise strings to always have a null terminator. This is a workaround for products that fail to handle zero size string properties.\n");
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
    printf("  as02/as11op1a/as11d10/op1a/d10/rdd9/as10:\n");
    printf("    --afd <value>           Active Format Descriptor 4-bit code from table 1 in SMPTE ST 2016-1. Default is input file's value or not set\n");
    printf("\n");
    printf("  as11op1a/as11d10/op1a/d10/rdd9/as10:\n");
    printf("    --single-pass           Write file in a single pass\n");
    printf("                            Header and body partitions will be incomplete for as11op1a/op1a if the number if essence container bytes per edit unit is variable\n");
    printf("    --file-md5              Calculate an MD5 checksum of the output file. This requires writing in a single pass (--single-pass is assumed)\n");
    printf("\n");
    printf("  as11op1a/op1a/rdd9:\n");
    printf("    --pass-anc <filter>     Pass through ST 436 ANC data tracks\n");
    printf("                            <filter> is a comma separated list of ANC data types to pass through\n");
    printf("                            The following ANC data types are supported in <filter>:\n");
    printf("                                all      : pass through all ANC data\n");
    printf("                                st2020   : SMPTE ST 2020 / RDD 6 audio metadata\n");
    printf("                                st2016   : SMPTE ST 2016-3/ AFD, bar and pan-scan data\n");
    printf("                                sdp      : SMPTE RDD 8 / OP-47 Subtitling Distribution Packet data\n");
    printf("                                st12     : SMPTE ST 12 Ancillary timecode\n");
    printf("                                st334    : SMPTE ST 334-1 EIA 708B, EIA 608 and data broadcast (DTV)\n");
    printf("    --strip-anc <filter>    Don't pass through ST 436 ANC data tracks\n");
    printf("                            <filter> is a comma separated list of ANC data types to not pass through. The types are listed in the --pass-anc option\n");
    printf("                            This filter is applied after --pass-anc. The --pass-anc option will default to 'all' when --strip-anc is used\n");
    printf("    --pass-vbi              Pass through ST 436 VBI data tracks\n");
    printf("    --st436-mf <count>      Set the <count> of frames to examine for ST 436 ANC/VBI manifest info. Default is %u\n", DEFAULT_ST436_MANIFEST_COUNT);
    printf("                            The manifest is used at the start to determine whether an output ANC data track is created\n");
    printf("                            Set <count> to 0 to always create an ANC data track if the input has one\n");
    printf("    --anc-const <size>      Use to indicate that the ST 436 ANC frame element data, excl. key and length, has a constant <size>. A variable size is assumed by default\n");
    printf("    --anc-max <size>        Use to indicate that the ST 436 ANC frame element data, excl. key and length, has a maximum <size>. A variable size is assumed by default\n");
    printf("    --st2020-max            The ST 436 ANC maximum frame element data size for ST 2020 only is calculated from the extracted manifest. Option '--pass-anc st2020' is required.\n");
    printf("    --vbi-const <size>      Use to indicate that the ST 436 VBI frame element data, excl. key and length, has a constant <size>. A variable size is assumed by default\n");
    printf("    --vbi-max <size>        Use to indicate that the ST 436 VBI frame element data, excl. key and length, has a maximum <size>. A variable size is assumed by default\n");
    printf("    --rdd6 <file>           Add ST 436 ANC data track containing 'static' RDD-6 audio metadata from XML <file>\n");
    printf("                            The timecode fields are ignored, i.e. they are set to 'undefined' values in the RDD-6 metadata stream\n");
    printf("    --rdd6-lines <lines>    A single or pair of line numbers, using ',' as the separator, for carriage of the RDD-6 ANC data. The default is a pair of numbers, '%u,%u'\n", DEFAULT_RDD6_LINES[0], DEFAULT_RDD6_LINES[1]);
    printf("    --rdd6-sdid <sdid>      The SDID value indicating the first audio channel pair associated with the RDD-6 data. Default is %u\n", DEFAULT_RDD6_SDID);
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
    printf("    --system-item           Add system item\n");
    printf("    --primary-package       Set the header metadata set primary package property to the top-level file source package\n");
    printf("    --index-follows         The index partition follows the essence partition, even when it is CBE essence\n");
    printf("    --st379-2               Add ContainerConstraintsSubDescriptor to signal compliance with ST 379-2, MXF Constrained Generic Container\n");
    printf("                            The sub-descriptor will be added anyway if there is RDD 36 video present\n");
    printf("\n");
    printf("  op1a/rdd9:\n");
    printf("    --ard-zdf-hdf           Use the ARD ZDF HDF profile\n");
    printf("    --repeat-index          Repeat the index table segments in the footer partition\n");
    printf("\n");
    printf("  op1a:\n");
    printf("    --ard-zdf-xdf           Use the ARD ZDF XDF profile\n");
    printf("\n");
    printf("  op1a/d10:\n");
    printf("    --cbe-index-duration-0  Use duration=0 if index table is CBE\n");
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
    printf("  as11d10/d10:\n");
    printf("    --d10-mute <flags>      Indicate using a string of 8 '0' or '1' which sound channels should be muted. The lsb is the rightmost digit\n");
    printf("    --d10-invalid <flags>   Indicate using a string of 8 '0' or '1' which sound channels should be flagged invalid. The lsb is the rightmost digit\n");
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
    printf("    --comment <string>      Add 'Comments' user comment to the MaterialPackage\n");
    printf("    --desc <string>         Add 'Descript' user comment to the MaterialPackage\n");
    printf("    --tag <name> <value>    Add <name> user comment to the MaterialPackage. Option can be used multiple times\n");
    printf("    --locator <position> <comment> <color>\n");
    printf("                            Add locator at <position> with <comment> and <color>\n");
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
    printf("    --psp-uid <umid>        Set the tape/import Source Package UID\n");
    printf("                              tape: autogenerated by default\n");
    printf("                              import: single input Material Package UID or autogenerated by default\n");
    printf("    --psp-created <tstamp>  Set the tape/import Source Package creation date. Default is 'now'\n");
    printf("    --ess-marks             Convert XDCAM Essence Marks to locators\n");
    printf("    --allow-no-avci-head    Allow inputs with no AVCI header (512 bytes, sequence and picture parameter sets)\n");
    printf("    --avid-gf               Use the Avid growing file flavour\n");
    printf("    --avid-gf-dur <dur>     Set the duration which should be shown whilst the file is growing\n");
    printf("                            The default value is the output duration\n");
    printf("    --ignore-d10-aes3-flags   Ignore D10 AES3 audio validity flags and assume they are all valid\n");
    printf("                              This workarounds an issue with Avid transfer manager which sets channel flags 4 to 8 to invalid\n");
    printf("\n");
    printf("  op1a/avid:\n");
    printf("    --force-no-avci-head    Strip AVCI header (512 bytes, sequence and picture parameter sets) if present\n");
    printf("\n");
    printf("  wave:\n");
    printf("    --orig <name>                Set originator in the Wave bext chunk. Default '%s'\n", DEFAULT_BEXT_ORIGINATOR);
    printf("    --exclude-wave-chunks <ids>  Don't transfer non-builtin or <chna> Wave chunks with ID in the comma-separated list of chunk <ids>\n");
    printf("                                 Set <ids> to 'all' to exclude all Wave chunks\n");
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
    printf("Input options:\n");
    printf("  --disable-tracks <tracks> A comma separated list of track indexes and/or ranges to disable.\n");
    printf("                            A track is identified by the index reported by mxf2raw\n");
    printf("                            A range of track indexes is specified as '<first>-<last>', e.g. 0-3\n");
    printf("  --disable-audio           Disable audio tracks\n");
    printf("  --disable-video           Disable video tracks\n");
    printf("  --disable-data            Disable data tracks\n");
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
    Rational timecode_rate = FRAME_RATE_25;
    bool timecode_rate_set = false;
    vector<const char *> input_filenames;
    map<size_t, set<size_t> > disable_track_indexes;
    map<size_t, bool> disable_audio;
    map<size_t, bool> disable_video;
    map<size_t, bool> disable_data;
    const char *log_filename = 0;
    LogLevel log_level = INFO_LOG;
    ClipWriterType clip_type = CW_OP1A_CLIP_TYPE;
    ClipSubType clip_sub_type = NO_CLIP_SUB_TYPE;
    bool ard_zdf_hdf_profile = false;
    bool ard_zdf_xdf_profile = false;
    bool aes3 = false;
    bool kag_size_512 = false;
    bool op1a_system_item = false;
    bool op1a_primary_package = false;
    bool op1a_index_follows = false;
    bool st379_2 = false;
    AS10Shim as10_shim = AS10_UNKNOWN_SHIM;
    const char *output_name = "";
    map<EssenceType, string> filename_essence_type_names;
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    bool use_mtc = false;
    bool use_fstc = false;
    bool use_pstc = false;
    int64_t start = 0;
    bool start_set = false;
    int64_t duration = -1;
    bool check_end = false;
    int  check_end_tolerance = 0;
    bool check_complete = false;
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
    const char *mpeg_descr_defaults_name = 0;
    bool mpeg_descr_frame_checks = false;
    bool as10_loose_checks = false;
    int max_mpeg_check_same_warn_messages = 3;
    bool print_mpeg_checks = false;
    bool pass_dm = false;
    const char *segmentation_filename = 0;
    bool do_print_version = false;
    bool use_group_reader = false;
    bool keep_input_order = false;
    BMX_OPT_PROP_DECL_DEF(uint8_t, user_afd, 0);
    vector<AVCIHeaderInput> avci_header_inputs;
    bool show_progress = false;
    bool single_pass = false;
    bool output_file_md5 = false;
    BMX_OPT_PROP_DECL_DEF(Rational, user_aspect_ratio, ASPECT_RATIO_16_9);
    bool set_bs_aspect_ratio = false;
    BMX_OPT_PROP_DECL_DEF(bool, user_locked, false);
    BMX_OPT_PROP_DECL_DEF(int8_t, user_audio_ref_level, 0);
    BMX_OPT_PROP_DECL_DEF(int8_t, user_dial_norm, 0);
    BMX_OPT_PROP_DECL_DEF(MXFSignalStandard, user_signal_standard, MXF_SIGNAL_STANDARD_NONE);
    BMX_OPT_PROP_DECL_DEF(MXFFrameLayout, user_frame_layout, MXF_FULL_FRAME);
    BMX_OPT_PROP_DECL_DEF(uint8_t, user_field_dominance, 1);
    BMX_OPT_PROP_DECL_DEF(mxfVideoLineMap, user_video_line_map, g_Null_Video_Line_Map);
    BMX_OPT_PROP_DECL_DEF(mxfUL, user_transfer_ch, g_Null_UL);
    BMX_OPT_PROP_DECL_DEF(mxfUL, user_coding_equations, g_Null_UL);
    BMX_OPT_PROP_DECL_DEF(mxfUL, user_color_primaries, g_Null_UL);
    BMX_OPT_PROP_DECL_DEF(MXFColorSiting, user_color_siting, MXF_COLOR_SITING_UNKNOWN);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_black_ref_level, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_white_ref_level, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_color_range, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_comp_max_ref, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_comp_min_ref, 0);
    BMX_OPT_PROP_DECL_DEF(uint8_t, user_scan_dir, 0);
    BMX_OPT_PROP_DECL_DEF(mxfThreeColorPrimaries, user_display_primaries, g_Null_Three_Color_Primaries);
    BMX_OPT_PROP_DECL_DEF(mxfColorPrimary, user_display_white_point, g_Null_Color_Primary);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_display_max_luma, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_display_min_luma, 0);
    BMX_OPT_PROP_DECL_DEF(bool, user_rdd36_opaque, false);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_rdd36_component_depth, 10);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_active_width, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_active_height, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_active_x_offset, 0);
    BMX_OPT_PROP_DECL_DEF(uint32_t, user_active_y_offset, 0);
    BMX_OPT_PROP_DECL_DEF(int32_t, user_display_f2_offset, 0);
    BMX_OPT_PROP_DECL_DEF(bool, user_center_cut_4_3, false);
    BMX_OPT_PROP_DECL_DEF(bool, user_center_cut_14_9, false);
    BMX_OPT_PROP_DECL_DEF(mxfRational, user_ref_image_edit_rate, g_Null_Rational);
    BMX_OPT_PROP_DECL_DEF(int8_t, user_ref_audio_align_level, 0);
    bool ignore_input_desc = false;
    bool input_file_md5 = false;
    int input_file_flags = 0;
    bool no_precharge = false;
    bool no_rollout = false;
    bool rw_interleave = false;
    uint32_t rw_interleave_size = DEFAULT_RW_INTL_SIZE;
    uint32_t system_page_size = mxf_get_system_page_size();
    uint8_t d10_mute_sound_flags = 0;
    uint8_t d10_invalid_sound_flags = 0;
    const char *originator = DEFAULT_BEXT_ORIGINATOR;
    set<WaveChunkId> exclude_wave_chunks;
    bool exclude_all_wave_chunks = false;
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
    bool convert_ess_marks = false;
    bool allow_no_avci_head = false;
    bool force_no_avci_head = false;
    bool no_tc_track = false;
    bool min_part = false;
    bool body_part = false;
    bool repeat_index = false;
    bool cbe_index_duration_0 = false;
    bool op1a_clip_wrap = false;
    bool realtime = false;
    float rt_factor = 1.0;
    bool growing_file = false;
    unsigned int gf_retries = DEFAULT_GF_RETRIES;
    float gf_retry_delay = DEFAULT_GF_RETRY_DELAY;
    float gf_rate_after_fail = DEFAULT_GF_RATE_AFTER_FAIL;
    bool enable_indexing_file = true;
    bool product_info_set = false;
    string company_name;
    string product_name;
    mxfProductVersion product_version;
    string version_string;
    UUID product_uid;
    Timestamp creation_date;
    bool creation_date_set = false;
    bool ps_avcihead = false;
    bool replace_avid_avcihead = false;
    bool avid_gf = false;
    int64_t avid_gf_duration = -1;
    set<ANCDataType> pass_anc;
    set<ANCDataType> strip_anc;
    bool pass_vbi = false;
    uint32_t st436_manifest_count = DEFAULT_ST436_MANIFEST_COUNT;
    uint32_t anc_const_size = 0;
    uint32_t anc_max_size = 0;
    bool st2020_max_size = false;
    uint32_t vbi_const_size = 0;
    uint32_t vbi_max_size = 0;
    const char *rdd6_filename = 0;
    uint16_t rdd6_lines[2] = {DEFAULT_RDD6_LINES[0], DEFAULT_RDD6_LINES[1]};
    uint8_t rdd6_sdid = DEFAULT_RDD6_SDID;
    uint32_t http_min_read = DEFAULT_HTTP_MIN_READ;
    bool http_enable_seek = true;
    bool mp_track_num = false;
#if defined(_WIN32) && !defined(__MINGW32__)
    bool use_mmap_file = false;
#endif
    vector<EmbedXMLInfo> embed_xml;
    EmbedXMLInfo next_embed_xml;
    bool ignore_d10_aes3_flags = false;
    TrackMapper track_mapper;
    bool track_map_set = false;
    bool dump_track_map = false;
    bool dump_track_map_exit = false;
    vector<pair<string, string> > track_mca_labels;
    bool use_avc_subdesc = false;
    UL audio_layout_mode_label = g_Null_UL;
    BMX_OPT_PROP_DECL_DEF(uint32_t, head_fill, 0);
    int vc2_mode_flags;
    mxfThreeColorPrimaries three_color_primaries;
    mxfColorPrimary color_primary;
    EssenceType assume_d10_essence_type = UNKNOWN_ESSENCE_TYPE;
    int value, num, den;
    unsigned int uvalue;
    int64_t i64value;
    Rational rate;
    bool msvc_block_limit;
    int cmdln_index;

    memset(&next_embed_xml, 0, sizeof(next_embed_xml));
    parse_vc2_mode("1", &vc2_mode_flags);
    parse_essence_type_names("video,audio,data,mixed", &filename_essence_type_names);

    if (argc == 1) {
        usage(argv[0]);
        return 0;
    }

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++)
    {
        msvc_block_limit = false;

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
        else if (strcmp(argv[cmdln_index], "-p") == 0)
        {
            show_progress = true;
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
        else if (strcmp(argv[cmdln_index], "--input-file-md5") == 0)
        {
            input_file_md5 = true;
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
        else if (strcmp(argv[cmdln_index], "--mtc") == 0)
        {
            use_mtc = true;
        }
        else if (strcmp(argv[cmdln_index], "--fstc") == 0)
        {
            use_fstc = true;
        }
        else if (strcmp(argv[cmdln_index], "--pstc") == 0)
        {
            use_pstc = true;
        }
        else if (strcmp(argv[cmdln_index], "--tc-rate") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_frame_rate(argv[cmdln_index + 1], &timecode_rate))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            timecode_rate_set = true;
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
        else if (strcmp(argv[cmdln_index], "--start") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &start) || start < 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            start_set = true;
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
        else if (strcmp(argv[cmdln_index], "--check-end") == 0)
        {
            check_end = true;
        }
        else if (strcmp(argv[cmdln_index], "--check-end-tolerance") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &value) ||
                value <= 0 )
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            check_end_tolerance = value;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--check-complete") == 0)
        {
            check_complete = true;
        }
        else if (strcmp(argv[cmdln_index], "--group") == 0)
        {
            use_group_reader = true;
        }
        else if (strcmp(argv[cmdln_index], "--no-reorder") == 0)
        {
            keep_input_order = true;
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
        else if (strcmp(argv[cmdln_index], "--gf") == 0)
        {
            growing_file = true;
        }
        else if (strcmp(argv[cmdln_index], "--gf-retries") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &gf_retries) || gf_retries == 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            growing_file = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--gf-delay") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_float(argv[cmdln_index + 1], &gf_retry_delay) || gf_retry_delay < 0.0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            growing_file = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--gf-rate") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_float(argv[cmdln_index + 1], &gf_rate_after_fail) || gf_rate_after_fail <= 0.0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            growing_file = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--disable-indexing-file") == 0)
        {
            enable_indexing_file = false;
        }
        else if (strcmp(argv[cmdln_index], "--http-min-read") == 0)
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
            http_min_read = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--http-disable-seek") == 0)
        {
            http_enable_seek = false;
        }
        else if (strcmp(argv[cmdln_index], "--no-precharge") == 0)
        {
            no_precharge = true;
        }
        else if (strcmp(argv[cmdln_index], "--no-rollout") == 0)
        {
            no_rollout = true;
        }
        else if (strcmp(argv[cmdln_index], "--rw-intl") == 0)
        {
            rw_interleave = true;
        }
        else if (strcmp(argv[cmdln_index], "--rw-intl-size") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue) ||
                (uvalue % system_page_size) != 0)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            rw_interleave_size = uvalue;
            cmdln_index++;
        }
#if defined(_WIN32)
        else if (strcmp(argv[cmdln_index], "--seq-scan") == 0)
        {
            input_file_flags |= MXF_WIN32_FLAG_SEQUENTIAL_SCAN;
        }
#if !defined(__MINGW32__)
        else if (strcmp(argv[cmdln_index], "--mmap-file") == 0)
        {
            use_mmap_file = true;
        }
#endif
#endif
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
        else if (strcmp(argv[cmdln_index], "--replace-avid-avcihead") == 0)
        {
            replace_avid_avcihead = true;
        }
        else if (strcmp(argv[cmdln_index], "-a") == 0)
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
            user_aspect_ratio.numerator   = num;
            user_aspect_ratio.denominator = den;
            BMX_OPT_PROP_MARK(user_aspect_ratio, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--bsar") == 0)
        {
            set_bs_aspect_ratio = true;
        }
        else if (strcmp(argv[cmdln_index], "--vc2-mode") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_vc2_mode(argv[cmdln_index + 1], &vc2_mode_flags))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--locked") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_bool(argv[cmdln_index + 1], &user_locked))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_locked, true);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_audio_ref_level, value);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_dial_norm, value);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_ref_image_edit_rate, rate);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_ref_audio_align_level, value);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--signal-std") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_signal_standard(argv[cmdln_index + 1], &user_signal_standard)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_signal_standard, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--frame-layout") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_frame_layout(argv[cmdln_index + 1], &user_frame_layout)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_frame_layout, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--field-dom") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_field_dominance(argv[cmdln_index + 1], &user_field_dominance)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_field_dominance, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--video-line-map") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_video_line_map(argv[cmdln_index + 1], &user_video_line_map)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_video_line_map, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--transfer-ch") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_transfer_ch(argv[cmdln_index + 1], &user_transfer_ch)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_transfer_ch, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--coding-eq") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_coding_equations(argv[cmdln_index + 1], &user_coding_equations)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_coding_equations, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--color-prim") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_color_primaries(argv[cmdln_index + 1], &user_color_primaries)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_color_primaries, true);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--color-siting") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_color_siting(argv[cmdln_index + 1], &user_color_siting)) {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            BMX_OPT_PROP_MARK(user_color_siting, true);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_black_ref_level, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_white_ref_level, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_color_range, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_comp_max_ref, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_comp_min_ref, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_scan_dir, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_display_primaries, three_color_primaries);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_display_white_point, color_primary);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_display_max_luma, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_display_min_luma, uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd36-opaque") == 0)
        {
            BMX_OPT_PROP_SET(user_rdd36_opaque, true);
        }
        else if (strcmp(argv[cmdln_index], "--rdd36-comp-depth") == 0)
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
            BMX_OPT_PROP_SET(user_rdd36_component_depth, value);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_active_width, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_active_height, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_active_x_offset, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_active_y_offset, uvalue);
            cmdln_index++;
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
            BMX_OPT_PROP_SET(user_display_f2_offset, value);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--center-cut-4-3") == 0)
        {
            BMX_OPT_PROP_SET(user_center_cut_4_3, true);
        }
        else if (strcmp(argv[cmdln_index], "--center-cut-14-9") == 0)
        {
            BMX_OPT_PROP_SET(user_center_cut_14_9, true);
        }
        else if (strcmp(argv[cmdln_index], "--ignore-input-desc") == 0)
        {
            ignore_input_desc = true;
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
        else if (strcmp(argv[cmdln_index], "--pass-dm") == 0)
        {
            pass_dm = true;
        }
        else if (strcmp(argv[cmdln_index], "--norm-pass-dm") == 0)
        {
            pass_dm = true;
            as11_helper.SetNormaliseStrings(true);
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
            BMX_OPT_PROP_SET(user_afd, value);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--single-pass") == 0)
        {
            single_pass = true;
        }
        else if (strcmp(argv[cmdln_index], "--file-md5") == 0)
        {
            output_file_md5 = true;
        }
        else if (strcmp(argv[cmdln_index], "--pass-anc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_anc_data_types(argv[cmdln_index + 1], &pass_anc))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--strip-anc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_anc_data_types(argv[cmdln_index + 1], &strip_anc))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--pass-vbi") == 0)
        {
            pass_vbi = true;
        }
        else if (strcmp(argv[cmdln_index], "--st436-mf") == 0)
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
            st436_manifest_count = (uint32_t)(uvalue);
            cmdln_index++;
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
            anc_const_size = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--anc-max") == 0)
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
            anc_max_size = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--st2020-max") == 0)
        {
            st2020_max_size = true;
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
            vbi_const_size = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vbi-max") == 0)
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
            vbi_max_size = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd6") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            rdd6_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd6-lines") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_rdd6_lines(argv[cmdln_index + 1], rdd6_lines))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd6-sdid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_int(argv[cmdln_index + 1], &uvalue) ||
                uvalue < 1 || uvalue > 9)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            rdd6_sdid = (uint8_t)(uvalue);
            cmdln_index++;
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
        else if (strcmp(argv[cmdln_index], "--cbe-index-duration-0") == 0)
        {
            cbe_index_duration_0 = true;
        }
        else if (strcmp(argv[cmdln_index], "--clip-wrap") == 0)
        {
            op1a_clip_wrap = true;
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
        else if (strcmp(argv[cmdln_index], "--system-item") == 0)
        {
            op1a_system_item = true;
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
        else if (strcmp(argv[cmdln_index], "--ess-marks") == 0)
        {
            convert_ess_marks = true;
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
        else if (strcmp(argv[cmdln_index], "--ignore-d10-aes3-flags") == 0)
        {
            ignore_d10_aes3_flags = true;
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
        else if (strcmp(argv[cmdln_index], "--exclude-wave-chunks") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_wave_chunk_ids(argv[cmdln_index + 1], &exclude_wave_chunks, &exclude_all_wave_chunks))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
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
        else if (strcmp(argv[cmdln_index], "--assume-d10-30") == 0)
        {
            assume_d10_essence_type = D10_30;
        }
        else if (strcmp(argv[cmdln_index], "--assume-d10-40") == 0)
        {
            assume_d10_essence_type = D10_40;
        }
        else if (strcmp(argv[cmdln_index], "--assume-d10-50") == 0)
        {
            assume_d10_essence_type = D10_50;
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
        else if (strcmp(argv[cmdln_index], "--regtest") == 0)
        {
            BMX_REGRESSION_TEST = true;
        }
        else
        {
            break;
        }
    }

    if (st2020_max_size && !passing_anc_data_type(ST2020_ANC_DATA, pass_anc, strip_anc)) {
        usage_ref(argv[0]);
        fprintf(stderr, "Option '--st2020-max' requires something equivalent to '--pass st2020'\n");
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

    for (; cmdln_index < argc; cmdln_index++) {
        if (strcmp(argv[cmdln_index], "--disable-tracks") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Missing argument for Input Option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_track_indexes(argv[cmdln_index + 1], &disable_track_indexes[input_filenames.size()]))
            {
                usage_ref(argv[0]);
                fprintf(stderr, "Invalid value '%s' for Input Option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--disable-audio") == 0)
        {
            disable_audio[input_filenames.size()] = true;
        }
        else if (strcmp(argv[cmdln_index], "--disable-video") == 0)
        {
            disable_video[input_filenames.size()] = true;
        }
        else if (strcmp(argv[cmdln_index], "--disable-data") == 0)
        {
            disable_data[input_filenames.size()] = true;
        }
        else
        {
            if (strcmp(argv[cmdln_index], "-") == 0) {
                // standard input
                input_filenames.push_back("");
            } else {
                if (mxf_http_is_url(argv[cmdln_index])) {
                    if (!mxf_http_is_supported()) {
                        fprintf(stderr, "HTTP file access is not supported in this build\n");
                        return 1;
                    }
                } else if (!check_file_exists(argv[cmdln_index])) {
                    if (argv[cmdln_index][0] == '-') {
                        usage_ref(argv[0]);
                        fprintf(stderr, "Unknown Input Option '%s'\n", argv[cmdln_index]);
                    } else {
                        fprintf(stderr, "Failed to open input filename '%s'\n", argv[cmdln_index]);
                    }
                    return 1;
                }
                input_filenames.push_back(argv[cmdln_index]);
            }
        }
    }

    if (input_filenames.empty()) {
        usage_ref(argv[0]);
        fprintf(stderr, "No input filenames\n");
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

    if (clip_sub_type == AS10_CLIP_SUB_TYPE) {
        const char *as10_shim_name = as10_helper.GetShimName();
        if (!as10_shim_name) {
            usage_ref(argv[0]);
            fprintf(stderr, "Set required 'ShimName' property for as10 output\n");
            return 1;
        }
        as10_shim = get_as10_shim(as10_shim_name);
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


        // open an MXFReader using the input filenames

        AppMXFFileFactory file_factory;
        MXFReader *reader = 0;
        MXFFileReader *file_reader = 0;

        if (input_file_md5)
            file_factory.AddInputChecksumType(MD5_CHECKSUM);
        file_factory.SetInputFlags(input_file_flags);
        if (rw_interleave)
            file_factory.SetRWInterleave(rw_interleave_size);
        file_factory.SetHTTPMinReadSize(http_min_read);
        file_factory.SetHTTPEnableSeek(http_enable_seek);
#if defined(_WIN32) && !defined(__MINGW32__)
        file_factory.SetUseMMapFile(use_mmap_file);
#endif

        if (use_group_reader && input_filenames.size() > 1) {
            MXFGroupReader *group_reader = new MXFGroupReader();
            MXFFileReader::OpenResult result;
            size_t i;
            for (i = 0; i < input_filenames.size(); i++) {
                MXFFileReader *grp_file_reader = new MXFFileReader();
                grp_file_reader->SetFileFactory(&file_factory, false);
                grp_file_reader->GetPackageResolver()->SetFileFactory(&file_factory, false);
                grp_file_reader->SetST436ManifestFrameCount(st436_manifest_count);
                grp_file_reader->SetEnableIndexFile(enable_indexing_file);
                result = grp_file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", input_filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    throw false;
                }
                disable_tracks(grp_file_reader, disable_track_indexes[i],
                               disable_audio[i], disable_video[i], disable_data[i]);
                group_reader->AddReader(grp_file_reader);
            }
            if (!group_reader->Finalize())
                throw false;

            reader = group_reader;
        } else if (input_filenames.size() > 1) {
            MXFSequenceReader *seq_reader = new MXFSequenceReader();
            MXFFileReader::OpenResult result;
            size_t i;
            for (i = 0; i < input_filenames.size(); i++) {
                MXFFileReader *seq_file_reader = new MXFFileReader();
                seq_file_reader->SetFileFactory(&file_factory, false);
                seq_file_reader->GetPackageResolver()->SetFileFactory(&file_factory, false);
                seq_file_reader->SetST436ManifestFrameCount(st436_manifest_count);
                seq_file_reader->SetEnableIndexFile(enable_indexing_file);
                result = seq_file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", input_filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    throw false;
                }
                disable_tracks(seq_file_reader, disable_track_indexes[i],
                               disable_audio[i], disable_video[i], disable_data[i]);
                seq_reader->AddReader(seq_file_reader);
            }
            if (!seq_reader->Finalize(false, keep_input_order))
                throw false;

            reader = seq_reader;
        } else {
            MXFFileReader::OpenResult result;
            file_reader = new MXFFileReader();
            file_reader->SetFileFactory(&file_factory, false);
            file_reader->GetPackageResolver()->SetFileFactory(&file_factory, false);
            file_reader->SetST436ManifestFrameCount(st436_manifest_count);
            file_reader->SetEnableIndexFile(enable_indexing_file);
            if (pass_dm && clip_sub_type == AS11_CLIP_SUB_TYPE)
                AS11Info::RegisterExtensions(file_reader->GetHeaderMetadata());
            if (pass_dm && clip_sub_type == AS10_CLIP_SUB_TYPE)
                AS10Info::RegisterExtensions(file_reader->GetHeaderMetadata());
            result = file_reader->Open(input_filenames[0]);
            if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                log_error("Failed to open MXF file '%s': %s\n", input_filenames[0],
                          MXFFileReader::ResultToString(result).c_str());
                throw false;
            }
            disable_tracks(file_reader, disable_track_indexes[0],
                           disable_audio[0], disable_video[0], disable_data[0]);

            reader = file_reader;
        }

        if (!reader->IsComplete()) {
            if (check_complete) {
                log_error("Input file is incomplete\n");
                throw false;
            }
            if (reader->IsSeekable() && enable_indexing_file)
                log_warn("Input file is incomplete\n");
            else
                log_debug("Input file is incomplete, probably because the file is not seekable\n");
        }

        reader->SetEmptyFrames(true);

        Rational frame_rate = reader->GetEditRate();


        // open RDD-6 XML file

        RDD6MetadataSequence rdd6_static_sequence;
        RDD6MetadataFrame rdd6_frame;
        bmx::ByteArray rdd6_first_buffer, rdd6_second_buffer;
        bmx::ByteArray anc_buffer;
        uint32_t rdd6_const_size = 0;
        bool rdd6_pair_in_frame = true;
        if (rdd6_filename) {
            if (clip_type != CW_OP1A_CLIP_TYPE && clip_type != CW_RDD9_CLIP_TYPE) {
                log_error("RDD-6 file input only supported for OP1a and RDD 9 clip types and sub-types\n");
                throw false;
            }

            if (!rdd6_frame.ParseXML(rdd6_filename)) {
                log_error("Failed to read and parse RDD-6 file '%s'\n", rdd6_filename);
                throw false;
            }
            if (!rdd6_frame.first_sub_frame)
                log_error("Missing first sub-frame in RDD-6 file\n");
            if (!rdd6_frame.second_sub_frame)
                log_error("Missing second sub-frame in RDD-6 file\n");
            if (!rdd6_frame.first_sub_frame || !rdd6_frame.second_sub_frame)
                throw false;

            if ((int64_t)frame_rate.numerator > 30 * (int64_t)frame_rate.denominator)
                rdd6_pair_in_frame = false;

            rdd6_frame.InitStaticSequence(&rdd6_static_sequence);

            if (rdd6_pair_in_frame) {
                rdd6_frame.UpdateStaticFrame(&rdd6_static_sequence);
                construct_anc_rdd6(&rdd6_frame, &rdd6_first_buffer, &rdd6_second_buffer, rdd6_sdid, rdd6_lines, &anc_buffer);
                rdd6_const_size = anc_buffer.GetSize();
            } // else the individual RDD-6 sub-frames have different sizes
        }


        // check whether the frame rate is a sound sampling rate
        // a frame rate that is a sound sampling rate will result in the timecode rate being used as
        // the clip frame rate and the timecode rate defaulting to 25Hz if not set by the user

        bool input_edit_rate_is_sampling_rate = false;
        size_t i;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            const MXFSoundTrackInfo *input_sound_info =
                dynamic_cast<const MXFSoundTrackInfo*>(reader->GetTrackReader(i)->GetTrackInfo());

            if (input_sound_info && frame_rate == input_sound_info->sampling_rate) {
                input_edit_rate_is_sampling_rate = true;
                break;
            }
        }


        // check support for tracks and disable unsupported track types

        bool have_vbi_track = false, have_anc_track = false;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *track_reader = reader->GetTrackReader(i);
            if (!track_reader->IsEnabled())
                continue;

            const MXFTrackInfo *input_track_info = track_reader->GetTrackInfo();

            const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);
            const MXFDataTrackInfo *input_data_info = dynamic_cast<const MXFDataTrackInfo*>(input_track_info);

            // Map generic MPEG video to D-10 if the --assume-d10-* options were used
            EssenceType input_essence_type = process_assumed_essence_type(input_track_info, assume_d10_essence_type);
            if (input_essence_type != input_track_info->essence_type) {
                log_warn("Assuming input track essence '%s' is '%s'\n",
                         essence_type_to_string(input_track_info->essence_type),
                         essence_type_to_string(input_essence_type));
            }

            bool is_enabled = true;
            if (input_essence_type == WAVE_PCM)
            {
                Rational sampling_rate = input_sound_info->sampling_rate;
                if (!ClipWriterTrack::IsSupported(clip_type, WAVE_PCM, sampling_rate)) {
                    log_warn("Track %" PRIszt " essence type '%s' @%d/%d sps not supported by clip type '%s'\n",
                             i,
                             essence_type_to_string(input_essence_type),
                             sampling_rate.numerator, sampling_rate.denominator,
                             clip_type_to_string(clip_type, clip_sub_type));
                    is_enabled = false;
                } else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32) {
                    log_warn("Track %" PRIszt " (%s) bits per sample %u not supported\n",
                             i,
                             essence_type_to_string(input_essence_type),
                             input_sound_info->bits_per_sample);
                    is_enabled = false;
                } else if (input_sound_info->channel_count == 0) {
                    log_warn("Track %" PRIszt " (%s) has zero channel count\n",
                             i,
                             essence_type_to_string(input_essence_type));
                    is_enabled = false;
                }
            }
            else if (input_essence_type == D10_AES3_PCM)
            {
                if (input_sound_info->sampling_rate != SAMPLING_RATE_48K)
                {
                    log_warn("Track %" PRIszt " essence type D-10 AES-3 audio sampling rate %d/%d not supported\n",
                             i,
                             input_sound_info->sampling_rate.numerator, input_sound_info->sampling_rate.denominator);
                    is_enabled = false;
                }
                else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32)
                {
                    log_warn("Track %" PRIszt " essence type D-10 AES-3 audio bits per sample %u not supported\n",
                             i,
                             input_sound_info->bits_per_sample);
                    is_enabled = false;
                }
            }
            else if (input_essence_type == UNKNOWN_ESSENCE_TYPE ||
                     input_essence_type == PICTURE_ESSENCE ||
                     input_essence_type == SOUND_ESSENCE ||
                     input_essence_type == DATA_ESSENCE)
            {
                log_warn("Track %" PRIszt " has unknown essence type\n", i);
                is_enabled = false;
            }
            else
            {
                if (input_track_info->edit_rate != frame_rate) {
                    log_warn("Track %" PRIszt " (essence type '%s') edit rate %d/%d does not equals clip edit rate %d/%d\n",
                             i,
                             essence_type_to_string(input_essence_type),
                             input_track_info->edit_rate.numerator, input_track_info->edit_rate.denominator,
                             frame_rate.numerator, frame_rate.denominator);
                    is_enabled = false;
                } else if (!ClipWriterTrack::IsSupported(clip_type, input_essence_type, frame_rate)) {
                    log_warn("Track %" PRIszt " essence type '%s' @%d/%d Hz not supported by clip type '%s'\n",
                             i,
                             essence_type_to_string(input_essence_type),
                             frame_rate.numerator, frame_rate.denominator,
                             clip_type_to_string(clip_type, clip_sub_type));
                    is_enabled = false;
                } else if (input_essence_type == VBI_DATA) {
                    if (!pass_vbi) {
                        log_warn("Not passing through VBI data track %" PRIszt "\n", i);
                        is_enabled = false;
                    } else if (have_vbi_track) {
                        log_warn("Already have a VBI track; not passing through VBI data track %" PRIszt "\n", i);
                        is_enabled = false;
                    } else {
                        have_vbi_track = true;
                    }
                } else if (input_essence_type == ANC_DATA) {
                    if (rdd6_filename) {
                        log_warn("Mixing RDD-6 file input and MXF ANC data input not yet supported\n");
                        is_enabled = false;
                    } else if (pass_anc.empty() && strip_anc.empty()) {
                        log_warn("Not passing through ANC data track %" PRIszt "\n", i);
                        is_enabled = false;
                    } else if (have_anc_track) {
                        log_warn("Already have an ANC track; not passing through ANC data track %" PRIszt "\n", i);
                        is_enabled = false;
                    } else {
                        if (st436_manifest_count == 0 || filter_anc_manifest(input_data_info, pass_anc, strip_anc)) {
                            have_anc_track = true;
                        } else {
                            log_warn("No match found in ANC data manifest; not passing through ANC data track %" PRIszt "\n", i);
                            is_enabled = false;
                        }
                    }
                }

                if ((input_essence_type == AVCI200_1080I ||
                         input_essence_type == AVCI200_1080P ||
                         input_essence_type == AVCI200_720P ||
                         input_essence_type == AVCI100_1080I ||
                         input_essence_type == AVCI100_1080P ||
                         input_essence_type == AVCI100_720P ||
                         input_essence_type == AVCI50_1080I ||
                         input_essence_type == AVCI50_1080P ||
                         input_essence_type == AVCI50_720P) &&
                    !force_no_avci_head &&
                    !allow_no_avci_head &&
                    !track_reader->HaveAVCIHeader() &&
                    !(ps_avcihead &&
                        have_ps_avci_header_data(input_essence_type, input_track_info->edit_rate)) &&
                    !have_avci_header_data(input_essence_type, input_track_info->edit_rate,
                                           avci_header_inputs))
                {
                    log_warn("Track %" PRIszt " (essence type '%s') does not have sequence and picture parameter sets\n",
                             i,
                             essence_type_to_string(input_essence_type));
                    is_enabled = false;
                }
            }

            if (!is_enabled) {
                log_warn("Ignoring track %" PRIszt " (essence type '%s')\n",
                          i, essence_type_to_string(input_essence_type));
            }

            track_reader->SetEnable(is_enabled);
        }

        if (!reader->IsEnabled()) {
            log_error("All tracks are disabled\n");
            throw false;
        }


        // set read limits

        int64_t read_start = 0;
        int16_t precharge = 0;
        int16_t rollout = 0;
        if (!reader->IsComplete()) {
            if (start_set || duration >= 0) {
                log_error("The --start and --dur options are not yet supported for incomplete files\n");
                throw false;
            }
            if (check_end) {
                log_error("Checking last frame is present (--check-end) is not supported for incomplete files\n");
                throw false;
            }
        } else {
            read_start = start;
            if (read_start >= reader->GetDuration()) {
                log_error("Start position %" PRId64 " is >= input duration %" PRId64 "\n",
                          start, reader->GetDuration());
                throw false;
            }
            int64_t output_duration;
            if (duration >= 0) {
                output_duration = duration;
                if (read_start + output_duration > reader->GetDuration()) {
                    log_warn("Limiting duration %" PRId64 " because it exceeds the available duration %" PRId64 "\n",
                              duration, reader->GetDuration() - read_start);
                    output_duration = reader->GetDuration() - read_start;
                }
            } else {
                output_duration = reader->GetDuration() - read_start;
            }

            if (output_duration > 0) {
                precharge = reader->GetMaxPrecharge(read_start, false);
                rollout = reader->GetMaxRollout(read_start + output_duration - 1, false);

                if (precharge != 0 && no_precharge) {
                    log_info("Precharge resulted in %d frame adjustment of start position and duration\n",
                             precharge);
                    output_duration += (- precharge);
                    read_start += precharge;
                    precharge = 0;
                }
                if (rollout != 0 && (no_rollout || clip_type == CW_AVID_CLIP_TYPE)) {
                    int64_t original_output_duration = output_duration;
                    while (rollout != 0) {
                        output_duration += rollout;
                        rollout = reader->GetMaxRollout(read_start + output_duration - 1, false);
                    }
                    if (!no_rollout) {
                        log_warn("'%s' clip type does not support rollout\n",
                                 clip_type_to_string(clip_type, clip_sub_type));
                    }
                    log_info("Rollout resulted in %" PRId64 " frame adjustment of duration\n",
                             output_duration - original_output_duration);
                }
            }

            if (check_end) {
                reader->SetReadLimits(read_start + precharge, max<int64_t>(0, -precharge + output_duration + rollout - check_end_tolerance), true);
                if (!reader->CheckReadLastFrame()) {
                    log_error("Check for last frame failed\n");
                    throw false;
                }
            }

            if (!check_end || check_end_tolerance != 0)
                reader->SetReadLimits(read_start + precharge, - precharge + output_duration + rollout, true);
        }


        // get input start timecode

        if (!start_timecode_str) {
            if (use_mtc && reader->HaveMaterialTimecode())
                start_timecode = reader->GetMaterialTimecode(read_start);
            if (use_fstc && start_timecode.IsInvalid() && reader->HaveFileSourceTimecode())
                start_timecode = reader->GetFileSourceTimecode(read_start);
            if (use_pstc && start_timecode.IsInvalid() && reader->HavePhysicalSourceTimecode())
                start_timecode = reader->GetPhysicalSourceTimecode(read_start);
            if (start_timecode.IsInvalid() && reader->HavePlayoutTimecode())
                start_timecode = reader->GetPlayoutTimecode(read_start);

            if (!start_timecode.IsInvalid()) {
                // adjust start timecode to be at the point after the leading filler segments
                // this corresponds to the zero position in the MXF reader
                start_timecode.AddOffset(reader->GetFixedLeadFillerOffset(), frame_rate);
            }
        }


        // complete commandline parsing

        if (!timecode_rate_set && !input_edit_rate_is_sampling_rate)
            timecode_rate = frame_rate;
        if (start_timecode_str && !parse_timecode(start_timecode_str, timecode_rate, &start_timecode)) {
            usage_ref(argv[0]);
            log_error("Invalid value '%s' for Option '-t'\n", start_timecode_str);
            throw false;
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
            Timecode position_start_timecode = start_timecode;
            if (position_start_timecode.IsInvalid())
                position_start_timecode.Init(timecode_rate, false);

            if (!parse_position(locators[i].position_str, position_start_timecode, timecode_rate,
                                &locators[i].locator.position))
            {
                usage_ref(argv[0]);
                log_error("Invalid value '%s' for Option '--locator'\n", locators[i].position_str);
                throw false;
            }
        }

        bool have_sound = false;
        bool sound_only_container = true;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *track_reader = reader->GetTrackReader(i);
            if (!track_reader->IsEnabled())
                continue;

            if (track_reader->GetTrackInfo()->essence_type == WAVE_PCM) {
                have_sound = true;
            } else if (track_reader->GetTrackInfo()->essence_type != TIMED_TEXT) {  // timed text is in a separate container
                sound_only_container = false;
                break;
            }
        }
        if (sound_only_container && !have_sound)
            sound_only_container = false;

        // Set --clip-wrap if the output is IMF with sound only
        if (clip_type == CW_OP1A_CLIP_TYPE &&
            clip_sub_type == IMF_CLIP_SUB_TYPE &&
            sound_only_container)
        {
            op1a_clip_wrap = true;
        }

        // Default --track-map to "singlemca" if clip wrapping
        if (!track_map_set && op1a_clip_wrap)
            track_mapper.ParseMapDef("singlemca");


        // copy across input file descriptive metadata

        if (pass_dm && (clip_sub_type == AS11_CLIP_SUB_TYPE ||
                        clip_sub_type == AS10_CLIP_SUB_TYPE ))
        {
            if (clip_sub_type == AS11_CLIP_SUB_TYPE &&
                (start != 0 || (duration >= 0 && duration < reader->GetDuration())))
            {
                log_error("Copying AS-11 descriptive metadata is currently only supported for complete file transwraps\n");
                throw false;
            }

            MXFFileReader *file_reader = dynamic_cast<MXFFileReader*>(reader);
            if (!file_reader) {
                log_error("Passing through AS-10/AS-11 descriptive metadata is only supported for a single input file\n");
                throw false;
            }
            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                as11_helper.ReadSourceInfo(file_reader);
            else
                as10_helper.ReadSourceInfo(file_reader);
        }


        // check if the file only contains timed text tracks as in that case the input duration
        // needs to be copied to the output

        bool timed_text_only = true;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *track_reader = reader->GetTrackReader(i);
            if (track_reader->IsEnabled() && track_reader->GetTrackInfo()->essence_type != TIMED_TEXT) {
                timed_text_only = false;
                break;
            }
        }


        // map input to output tracks

        // map WAVE PCM tracks
        vector<TrackMapper::InputTrackInfo> unused_input_tracks;
        vector<TrackMapper::InputTrackInfo> mapper_input_tracks;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *input_track_reader = reader->GetTrackReader(i);
            if (!input_track_reader->IsEnabled())
                continue;

            const MXFTrackInfo *input_track_info = input_track_reader->GetTrackInfo();
            const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);

            TrackMapper::InputTrackInfo mapper_input_track;
            if (input_track_info->essence_type == WAVE_PCM ||
                input_track_info->essence_type == D10_AES3_PCM)
            {
                mapper_input_track.external_index  = (uint32_t)i;
                mapper_input_track.essence_type    = WAVE_PCM; // D10_AES3_PCM gets converted to WAVE_PCM
                mapper_input_track.data_def        = input_track_info->data_def;
                mapper_input_track.bits_per_sample = input_sound_info->bits_per_sample;
                mapper_input_track.channel_count   = input_sound_info->channel_count;
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
            MXFTrackReader *input_track_reader = reader->GetTrackReader(unused_input_tracks[i].external_index);
            const MXFTrackInfo *input_track_info = input_track_reader->GetTrackInfo();
            log_info("Track %" PRIszt " is not mapped (essence type '%s')\n",
                      unused_input_tracks[i].external_index, essence_type_to_string(input_track_info->essence_type));
            input_track_reader->SetEnable(false);
        }

        // insert non-WAVE PCM tracks to output mapping
        uint32_t input_track_index = (uint32_t)mapper_input_tracks.size();
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *input_track_reader = reader->GetTrackReader(i);
            if (!input_track_reader->IsEnabled())
                continue;

            const MXFTrackInfo *input_track_info = input_track_reader->GetTrackInfo();
            if (input_track_info->essence_type != WAVE_PCM &&
                input_track_info->essence_type != D10_AES3_PCM)
            {
                // Map generic MPEG video to D-10 if the --assume-d10-* options were used
                EssenceType input_essence_type = process_assumed_essence_type(input_track_info, assume_d10_essence_type);

                TrackMapper::OutputTrackMap track_map;
                track_map.essence_type = input_essence_type;
                track_map.data_def     = input_track_info->data_def;

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
        if (!reader->IsEnabled()) {
            log_error("No input tracks mapped to output\n");
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


        // create output clip and initialize

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
                if (op1a_system_item)
                    flavour |= OP1A_SYSTEM_ITEM_FLAVOUR;
                if (min_part)
                    flavour |= OP1A_MIN_PARTITIONS_FLAVOUR;
                else if (body_part)
                    flavour |= OP1A_BODY_PARTITIONS_FLAVOUR;
            }
            if (output_file_md5)
                flavour |= OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= OP1A_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_D10_CLIP_TYPE) {
            flavour = D10_DEFAULT_FLAVOUR;
            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                flavour |= D10_AS11_FLAVOUR;
            if (output_file_md5)
                flavour |= D10_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= D10_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            if (ard_zdf_hdf_profile)
                flavour = RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR;
            else if (clip_sub_type == AS10_CLIP_SUB_TYPE)
                flavour = RDD9_AS10_FLAVOUR;
            else if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                flavour = RDD9_AS11_FLAVOUR;
            if (output_file_md5)
                flavour |= RDD9_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= RDD9_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_AVID_CLIP_TYPE) {
            flavour = AVID_DEFAULT_FLAVOUR;
            if (avid_gf)
                flavour |= AVID_GROWING_FILE_FLAVOUR;
        }
        ClipWriter *clip = 0;
        Rational clip_frame_rate = (input_edit_rate_is_sampling_rate ? timecode_rate : frame_rate);
        switch (clip_type)
        {
            case CW_AS02_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS02Clip(complete_output_name, true, clip_frame_rate, &file_factory, false);
                break;
            case CW_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewOP1AClip(flavour, file_factory.OpenNew(complete_output_name), clip_frame_rate);
                break;
            case CW_AVID_CLIP_TYPE:
                clip = ClipWriter::OpenNewAvidClip(flavour, clip_frame_rate, &file_factory, false);
                break;
            case CW_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewD10Clip(flavour, file_factory.OpenNew(complete_output_name), clip_frame_rate);
                break;
            case CW_RDD9_CLIP_TYPE:
                clip = ClipWriter::OpenNewRDD9Clip(flavour, file_factory.OpenNew(complete_output_name), clip_frame_rate);
                break;
            case CW_WAVE_CLIP_TYPE:
                clip = ClipWriter::OpenNewWaveClip(WaveFileIO::OpenNew(complete_output_name));
                break;
            case CW_UNKNOWN_CLIP_TYPE:
                BMX_ASSERT(false);
                break;
        }

        SourcePackage *physical_package = 0;
        vector<pair<mxfUMID, uint32_t> > physical_package_picture_refs;
        vector<pair<mxfUMID, uint32_t> > physical_package_sound_refs;

        if (!start_timecode.IsInvalid())
            clip->SetStartTimecode(start_timecode);
        if (clip_name)
            clip->SetClipName(clip_name);
        else if (clip_sub_type == AS11_CLIP_SUB_TYPE && as11_helper.HaveProgrammeTitle())
            clip->SetClipName(as11_helper.GetProgrammeTitle());
        else if (clip_sub_type == AS10_CLIP_SUB_TYPE && as10_helper.HaveMainTitle())
            clip->SetClipName(as10_helper.GetMainTitle());
        clip->SetProductInfo(company_name, product_name, product_version, version_string, product_uid);
        if (creation_date_set)
            clip->SetCreationDate(creation_date);
        if (cbe_index_duration_0)
            clip->ForceWriteCBEDuration0(true);

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

            if ((flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR) || timed_text_only)
                op1a_clip->SetInputDuration(reader->GetReadDuration());

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
            op1a_clip->SetOutputStartOffset(- precharge);
            op1a_clip->SetOutputEndOffset(- rollout);
            if (no_tc_track)
                op1a_clip->SetAddTimecodeTrack(false);
            if (op1a_primary_package)
                op1a_clip->SetPrimaryPackage(true);
        } else if (clip_type == CW_AVID_CLIP_TYPE) {
            AvidClip *avid_clip = clip->GetAvidClip();

            if (avid_gf) {
                if (avid_gf_duration < 0)
                    avid_clip->SetGrowingDuration(reader->GetReadDuration());
                else
                    avid_clip->SetGrowingDuration(avid_gf_duration);
            }

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
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    MXFTrackReader *input_track_reader = reader->GetTrackReader(i);
                    if (!input_track_reader->IsEnabled())
                        continue;

                    const MXFTrackInfo *input_track_info = input_track_reader->GetTrackInfo();
                    if (input_track_info->data_def != MXF_PICTURE_DDEF && input_track_info->data_def != MXF_SOUND_DDEF)
                        continue;

                    const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);

                    if (input_sound_info)
                        num_sound_tracks += input_sound_info->channel_count;
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
                    if (reader->GetMaterialPackageUID() != g_Null_UMID)
                        physical_package->setPackageUID(reader->GetMaterialPackageUID());
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

            if (flavour & D10_SINGLE_PASS_WRITE_FLAVOUR)
                d10_clip->SetInputDuration(reader->GetReadDuration());

            if (BMX_OPT_PROP_IS_SET(head_fill))
                d10_clip->ReserveHeaderMetadataSpace(head_fill);
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            RDD9File *rdd9_clip = clip->GetRDD9Clip();

            if (BMX_OPT_PROP_IS_SET(head_fill))
                rdd9_clip->ReserveHeaderMetadataSpace(head_fill);

            if (clip_sub_type == AS10_CLIP_SUB_TYPE)
              rdd9_clip->SetValidator(new AS10RDD9Validator(as10_shim, as10_loose_checks));

            if (repeat_index)
                rdd9_clip->SetRepeatIndexTable(true);

            if (partition_interval_set)
                rdd9_clip->SetPartitionInterval(partition_interval);
            rdd9_clip->SetFixedPartitionInterval(fixed_partition_interval);
            rdd9_clip->SetOutputStartOffset(- precharge);
            rdd9_clip->SetOutputEndOffset(- rollout);

            if (mp_uid_set)
                rdd9_clip->SetMaterialPackageUID(mp_uid);
            if (fp_uid_set)
                rdd9_clip->SetFileSourcePackageUID(fp_uid);
        } else if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();

            if (originator)
                wave_clip->GetBroadcastAudioExtension()->SetOriginator(originator);
        }


        // create the output tracks
        map<uint32_t, MXFInputTrack*> created_input_tracks;
        vector<OutputTrack*> output_tracks;
        vector<MXFInputTrack*> input_tracks;
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
                    MXFTrackReader *input_track_reader = reader->GetTrackReader(channel_map.input_external_index);
                    MXFInputTrack *input_track;
                    if (created_input_tracks.count(channel_map.input_external_index)) {
                        input_track = created_input_tracks[channel_map.input_external_index];
                    } else {
                        input_track = new MXFInputTrack(input_track_reader);
                        input_tracks.push_back(input_track);
                        created_input_tracks[channel_map.input_external_index] = input_track;
                    }

                    // copy across sound info to OutputTrack
                    if (!output_track->HaveInputTrack()) {
                        const MXFTrackInfo *input_track_info = input_track_reader->GetTrackInfo();
                        const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);
                        if (input_sound_info) {
                            OutputTrackSoundInfo *output_sound_info = output_track->GetSoundInfo();
                            output_sound_info->sampling_rate   = input_sound_info->sampling_rate;
                            output_sound_info->bits_per_sample = input_sound_info->bits_per_sample;
                            output_sound_info->sequence_offset = input_sound_info->sequence_offset;
                            BMX_OPT_PROP_COPY(output_sound_info->locked,          input_sound_info->locked);
                            BMX_OPT_PROP_COPY(output_sound_info->audio_ref_level, input_sound_info->audio_ref_level);
                            BMX_OPT_PROP_COPY(output_sound_info->dial_norm,       input_sound_info->dial_norm);
                        }
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


        // initialise output tracks

        unsigned char avci_header_data[AVCI_HEADER_SIZE];
        bool update_header = false;
        for (i = 0; i < output_tracks.size(); i++) {
            OutputTrack *output_track = output_tracks[i];

            ClipWriterTrack *clip_track = output_track->GetClipTrack();
            EssenceType output_essence_type = clip_track->GetEssenceType();
            MXFDataDefEnum output_data_def = convert_essence_type_to_data_def(output_essence_type);

            MXFInputTrack *input_track = 0;
            MXFTrackReader *input_track_reader = 0;
            const MXFTrackInfo *input_track_info = 0;
            const MXFPictureTrackInfo *input_picture_info = 0;
            if (output_track->HaveInputTrack()) {
                input_track = dynamic_cast<MXFInputTrack*>(output_track->GetFirstInputTrack());
                input_track_reader = input_track->GetTrackReader();
                input_track_info = input_track_reader->GetTrackInfo();
                input_picture_info = dynamic_cast<const MXFPictureTrackInfo*>(input_track_info);
            } else {
                BMX_ASSERT(output_essence_type == WAVE_PCM);
            }
            const OutputTrackSoundInfo *output_sound_info = output_track->GetSoundInfo();

            uint8_t afd = 0;
            if (BMX_OPT_PROP_IS_SET(user_afd))
                afd = user_afd;
            else if (input_picture_info)
                afd = input_picture_info->afd;

            // TODO: track number setting and check AES-3 channel validity

            if (clip_type == CW_AS02_CLIP_TYPE) {
                AS02Track *as02_track = clip_track->GetAS02Track();
                as02_track->SetMICType(mic_type);
                as02_track->SetMICScope(ess_component_mic_scope);

                if (partition_interval_set) {
                    AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(as02_track);
                    if (as02_pict_track)
                        as02_pict_track->SetPartitionInterval(partition_interval);
                }
            } else if (clip_type == CW_AVID_CLIP_TYPE) {
                AvidTrack *avid_track = clip_track->GetAvidTrack();

                if (avid_track->SupportOutputStartOffset())
                    avid_track->SetOutputStartOffset(- precharge);
                else
                    output_track->SetSkipPrecharge(- precharge); // skip precharge frames

                if (physical_package) {
                    if (output_data_def == MXF_PICTURE_DDEF) {
                        avid_track->SetSourceRef(physical_package_picture_refs[output_track->GetPhysSrcTrackIndex()].first,
                                                 physical_package_picture_refs[output_track->GetPhysSrcTrackIndex()].second);
                    } else if (output_data_def == MXF_SOUND_DDEF) {
                        avid_track->SetSourceRef(physical_package_sound_refs[output_track->GetPhysSrcTrackIndex()].first,
                                                 physical_package_sound_refs[output_track->GetPhysSrcTrackIndex()].second);
                    }
                }
            }

            BMX_ASSERT(input_track || output_essence_type == WAVE_PCM);
            switch (output_essence_type)
            {
                case IEC_DV25:
                case DVBASED_DV25:
                case DV50:
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    else
                        clip_track->SetAspectRatio(input_picture_info->aspect_ratio);
                    if (afd)
                        clip_track->SetAFD(afd);
                    break;
                case DV100_1080I:
                case DV100_720P:
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    else
                        clip_track->SetAspectRatio(input_picture_info->aspect_ratio);
                    if (afd)
                        clip_track->SetAFD(afd);
                    clip_track->SetComponentDepth(input_picture_info->component_depth);
                    break;
                case D10_30:
                case D10_40:
                case D10_50:
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio)) {
                        clip_track->SetAspectRatio(user_aspect_ratio);
                        if (set_bs_aspect_ratio)
                            output_track->SetFilter(new MPEG2AspectRatioFilter(user_aspect_ratio));
                    } else {
                        clip_track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (set_bs_aspect_ratio)
                            output_track->SetFilter(new MPEG2AspectRatioFilter(input_picture_info->aspect_ratio));
                    }
                    if (afd)
                        clip_track->SetAFD(afd);
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
                    if (afd)
                        clip_track->SetAFD(afd);
                    clip_track->SetUseAVCSubDescriptor(use_avc_subdesc);
                    if (force_no_avci_head) {
                        clip_track->SetAVCIMode(AVCI_NO_FRAME_HEADER_MODE);
                    } else {
                        if (allow_no_avci_head)
                            clip_track->SetAVCIMode(AVCI_NO_OR_ALL_FRAME_HEADER_MODE);
                        else
                            clip_track->SetAVCIMode(AVCI_ALL_FRAME_HEADER_MODE);

                        if (replace_avid_avcihead)
                        {
                            if (!get_ps_avci_header_data(input_track_info->essence_type,
                                                         input_picture_info->edit_rate,
                                                         avci_header_data, sizeof(avci_header_data)))
                            {
                                log_error("No replacement Panasonic AVCI header data available for input %s\n",
                                          essence_type_to_string(input_track_info->essence_type));
                                throw false;
                            }
                            if (input_track_reader->HaveAVCIHeader()) {
                                bool missing_stop_bit;
                                bool other_differences;
                                check_avid_avci_stop_bit(input_track_reader->GetAVCIHeader(), avci_header_data,
                                                         AVCI_HEADER_SIZE, &missing_stop_bit, &other_differences);
                                if (other_differences) {
                                    log_warn("Difference between input and Panasonic AVCI header is not just a "
                                             "missing stop bit\n");
                                    log_warn("AVCI header replacement may result in invalid or broken bitstream\n");
                                } else if (missing_stop_bit) {
                                    log_info("Found missing stop bit in input AVCI header\n");
                                } else {
                                    log_info("No missing stop bit found in input AVCI header\n");
                                }
                            }
                            clip_track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                            clip_track->SetReplaceAVCIHeader(true);
                        }
                        else if (input_track_reader->HaveAVCIHeader())
                        {
                            clip_track->SetAVCIHeader(input_track_reader->GetAVCIHeader(), AVCI_HEADER_SIZE);
                        }
                        else if (ps_avcihead && get_ps_avci_header_data(input_track_info->essence_type,
                                                                        input_picture_info->edit_rate,
                                                                        avci_header_data, sizeof(avci_header_data)))
                        {
                            clip_track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                        }
                        else if (read_avci_header_data(input_track_info->essence_type,
                                                       input_picture_info->edit_rate, avci_header_inputs,
                                                       avci_header_data, sizeof(avci_header_data)))
                        {
                            clip_track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                        }
                        else if (!allow_no_avci_head)
                        {
                            log_error("Failed to read AVC-Intra header data from input file for %s\n",
                                      essence_type_to_string(input_track_info->essence_type));
                            throw false;
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
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    if (afd)
                        clip_track->SetAFD(afd);
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
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    else
                        clip_track->SetAspectRatio(input_picture_info->aspect_ratio);
                    if (afd)
                        clip_track->SetAFD(afd);
                    clip_track->SetComponentDepth(input_picture_info->component_depth);
                    clip_track->SetInputHeight(input_picture_info->stored_height);
                    break;
                case AVID_ALPHA_SD:
                case AVID_ALPHA_HD_1080I:
                case AVID_ALPHA_HD_1080P:
                case AVID_ALPHA_HD_720P:
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    else
                        clip_track->SetAspectRatio(input_picture_info->aspect_ratio);
                    if (afd)
                        clip_track->SetAFD(afd);
                    clip_track->SetInputHeight(input_picture_info->stored_height);
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
                    if (afd)
                        clip_track->SetAFD(afd);
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
                    if (afd)
                        clip_track->SetAFD(afd);
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    else
                        clip_track->SetAspectRatio(input_picture_info->aspect_ratio);
                    break;
                case RDD36_422_PROXY:
                case RDD36_422_LT:
                case RDD36_422:
                case RDD36_422_HQ:
                case RDD36_4444:
                case RDD36_4444_XQ:
                    if (afd)
                        clip_track->SetAFD(afd);
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    if (BMX_OPT_PROP_IS_SET(user_rdd36_component_depth))
                        clip_track->SetComponentDepth(user_rdd36_component_depth);
                    else if (input_picture_info->component_depth > 0)
                        clip_track->SetComponentDepth(input_picture_info->component_depth);
                    break;
                case JPEG2000_CDCI:
                case JPEG2000_RGBA:
                    if (afd)
                        clip_track->SetAFD(afd);
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    break;
                case JPEGXS_CDCI:
                case JPEGXS_RGBA:
                    if (afd)
                        clip_track->SetAFD(afd);
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    break;
                case VC2:
                    if (afd)
                        clip_track->SetAFD(afd);
                    if (BMX_OPT_PROP_IS_SET(user_aspect_ratio))
                        clip_track->SetAspectRatio(user_aspect_ratio);
                    else
                        clip_track->SetAspectRatio(input_picture_info->aspect_ratio);
                    clip_track->SetVC2ModeFlags(vc2_mode_flags);
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
                    if (afd)
                        clip_track->SetAFD(afd);
                    break;
                case WAVE_PCM:
                    clip_track->SetSamplingRate(output_sound_info->sampling_rate);
                    clip_track->SetQuantizationBits(output_sound_info->bits_per_sample);
                    clip_track->SetChannelCount(output_track->GetChannelCount());
                    if (BMX_OPT_PROP_IS_SET(user_locked))
                        clip_track->SetLocked(user_locked);
                    else if (BMX_OPT_PROP_IS_SET(output_sound_info->locked))
                        clip_track->SetLocked(output_sound_info->locked);
                    if (BMX_OPT_PROP_IS_SET(user_audio_ref_level))
                        clip_track->SetAudioRefLevel(user_audio_ref_level);
                    else if (BMX_OPT_PROP_IS_SET(output_sound_info->audio_ref_level))
                        clip_track->SetAudioRefLevel(output_sound_info->audio_ref_level);
                    if (BMX_OPT_PROP_IS_SET(user_dial_norm))
                        clip_track->SetDialNorm(user_dial_norm);
                    else if (BMX_OPT_PROP_IS_SET(output_sound_info->dial_norm))
                        clip_track->SetDialNorm(output_sound_info->dial_norm);
                    if (clip_type == CW_D10_CLIP_TYPE || output_sound_info->sequence_offset)
                        clip_track->SetSequenceOffset(output_sound_info->sequence_offset);
                    if (audio_layout_mode_label != g_Null_UL)
                        clip_track->SetChannelAssignment(audio_layout_mode_label);
                    break;
                case ANC_DATA:
                    if (anc_const_size) {
                        clip_track->SetConstantDataSize(anc_const_size);
                    } else if (anc_max_size) {
                        clip_track->SetMaxDataSize(anc_max_size);
                    } else if (st2020_max_size) {
                        clip_track->SetMaxDataSize(calc_st2020_max_size(
                            dynamic_cast<const MXFDataTrackInfo*>(input_track_info)));
                    }
                    break;
                case VBI_DATA:
                    if (vbi_const_size)
                        clip_track->SetConstantDataSize(vbi_const_size);
                    else if (vbi_max_size)
                        clip_track->SetMaxDataSize(vbi_max_size);
                    break;
                case TIMED_TEXT:
                {
                    const MXFDataTrackInfo *input_data_info = dynamic_cast<const MXFDataTrackInfo*>(input_track_info);
                    MXFTimedTextTrackReader *tt_track_reader =
                            dynamic_cast<MXFTimedTextTrackReader*>(input_track_reader);
                    TimedTextManifest timed_text_manifest = *input_data_info->timed_text_manifest;
                    if (read_start > 0) {
                        // adjust the timed text offset with the sub-clip start offset
                        if (read_start > timed_text_manifest.mStart) {
                            log_error("Cannot start the sub-clip %" PRId64 " after the Timed Text zero point %" PRId64 "\n",
                                      read_start, timed_text_manifest.mStart);
                            throw false;
                        }
                        timed_text_manifest.mStart -= read_start;
                    }
                    clip_track->SetTimedTextSource(&timed_text_manifest);
                    clip_track->SetTimedTextResourceProvider(tt_track_reader->CreateResourceProvider());
                    break;
                }
                case D10_AES3_PCM:
                case MGA:
                case MGA_SADM:
                case PICTURE_ESSENCE:
                case SOUND_ESSENCE:
                case DATA_ESSENCE:
                case UNKNOWN_ESSENCE_TYPE:
                    BMX_ASSERT(false);
            }

            PictureMXFDescriptorHelper *pict_helper =
                    dynamic_cast<PictureMXFDescriptorHelper*>(clip_track->GetMXFDescriptorHelper());
            SoundMXFDescriptorHelper *sound_helper =
                    dynamic_cast<SoundMXFDescriptorHelper*>(clip_track->GetMXFDescriptorHelper());

            if (pict_helper) {
                if (BMX_OPT_PROP_IS_SET(user_signal_standard))
                    pict_helper->SetSignalStandard(user_signal_standard);
                if (BMX_OPT_PROP_IS_SET(user_frame_layout))
                    pict_helper->SetFrameLayout(user_frame_layout);
                if (BMX_OPT_PROP_IS_SET(user_field_dominance))
                    pict_helper->SetFieldDominance(user_field_dominance);
                if (BMX_OPT_PROP_IS_SET(user_video_line_map))
                    pict_helper->SetVideoLineMap(user_video_line_map);
                if (BMX_OPT_PROP_IS_SET(user_transfer_ch))
                    pict_helper->SetTransferCharacteristic(user_transfer_ch);
                if (BMX_OPT_PROP_IS_SET(user_coding_equations))
                    pict_helper->SetCodingEquations(user_coding_equations);
                if (BMX_OPT_PROP_IS_SET(user_color_primaries))
                    pict_helper->SetColorPrimaries(user_color_primaries);
                if (BMX_OPT_PROP_IS_SET(user_color_siting))
                    pict_helper->SetColorSiting(user_color_siting);
                if (BMX_OPT_PROP_IS_SET(user_black_ref_level))
                    pict_helper->SetBlackRefLevel(user_black_ref_level);
                if (BMX_OPT_PROP_IS_SET(user_white_ref_level))
                    pict_helper->SetWhiteRefLevel(user_white_ref_level);
                if (BMX_OPT_PROP_IS_SET(user_color_range))
                    pict_helper->SetColorRange(user_color_range);
                if (BMX_OPT_PROP_IS_SET(user_comp_max_ref))
                    pict_helper->SetComponentMaxRef(user_comp_max_ref);
                if (BMX_OPT_PROP_IS_SET(user_comp_min_ref))
                    pict_helper->SetComponentMinRef(user_comp_min_ref);
                if (BMX_OPT_PROP_IS_SET(user_scan_dir))
                    pict_helper->SetScanningDirection(user_scan_dir);
                if (BMX_OPT_PROP_IS_SET(user_display_primaries))
                    pict_helper->SetMasteringDisplayPrimaries(user_display_primaries);
                if (BMX_OPT_PROP_IS_SET(user_display_white_point))
                    pict_helper->SetMasteringDisplayWhitePointChromaticity(user_display_white_point);
                if (BMX_OPT_PROP_IS_SET(user_display_max_luma))
                    pict_helper->SetMasteringDisplayMaximumLuminance(user_display_max_luma);
                if (BMX_OPT_PROP_IS_SET(user_display_min_luma))
                    pict_helper->SetMasteringDisplayMinimumLuminance(user_display_min_luma);
                if (BMX_OPT_PROP_IS_SET(user_active_width))
                    pict_helper->SetActiveWidth(user_active_width);
                if (BMX_OPT_PROP_IS_SET(user_active_height))
                    pict_helper->SetActiveHeight(user_active_height);
                if (BMX_OPT_PROP_IS_SET(user_active_x_offset))
                    pict_helper->SetActiveXOffset(user_active_x_offset);
                if (BMX_OPT_PROP_IS_SET(user_active_y_offset))
                    pict_helper->SetActiveYOffset(user_active_y_offset);
                if (BMX_OPT_PROP_IS_SET(user_display_f2_offset))
                    pict_helper->SetDisplayF2Offset(user_display_f2_offset);
                if ((BMX_OPT_PROP_IS_SET(user_center_cut_4_3) && user_center_cut_4_3) ||
                    (BMX_OPT_PROP_IS_SET(user_center_cut_14_9) && user_center_cut_14_9))
                {
                    vector<mxfUL> cuts;
                    if (BMX_OPT_PROP_IS_SET(user_center_cut_4_3) && user_center_cut_4_3)
                        cuts.push_back(CENTER_CUT_4_3);
                    if (BMX_OPT_PROP_IS_SET(user_center_cut_14_9) && user_center_cut_14_9)
                        cuts.push_back(CENTER_CUT_14_9);

                    pict_helper->SetAlternativeCenterCuts(cuts);
                }

                RDD36MXFDescriptorHelper *rdd36_helper = dynamic_cast<RDD36MXFDescriptorHelper*>(pict_helper);
                if (rdd36_helper) {
                    if (BMX_OPT_PROP_IS_SET(user_rdd36_opaque))
                        rdd36_helper->SetIsOpaque(user_rdd36_opaque);
                }
            } else if (sound_helper) {
                if (BMX_OPT_PROP_IS_SET(user_ref_image_edit_rate))
                    sound_helper->SetReferenceImageEditRate(user_ref_image_edit_rate);
                if (BMX_OPT_PROP_IS_SET(user_ref_audio_align_level))
                    sound_helper->SetReferenceAudioAlignmentLevel(user_ref_audio_align_level);
            }
        }

        // add RDD-6 ANC data track for input RDD-6 XML file

        if (rdd6_filename) {
            OutputTrack *output_track = new OutputTrack(clip->CreateTrack(ANC_DATA));
            ClipWriterTrack *clip_track = output_track->GetClipTrack();

            if (anc_const_size)
                clip_track->SetConstantDataSize(anc_const_size);
            else if (anc_max_size)
                clip_track->SetMaxDataSize(anc_max_size);
            else if (st2020_max_size)
                clip_track->SetMaxDataSize(calc_st2020_max_size(false, 1));
            else if (rdd6_const_size)
                clip_track->SetConstantDataSize(rdd6_const_size);

            output_tracks.push_back(output_track);
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


        // Add wave chunks

        if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();

            // Ensure that the start channels are up-to-date for each track
            wave_clip->UpdateChannelCounts();

            // Use the generic stream identifier to ensure chunks are included only once
            set<uint32_t> unique_chunk_stream_ids;

            // Loop over the wave output tracks
            for (size_t i = 0; i < output_tracks.size(); i++) {
                OutputTrack *output_track = output_tracks[i];

                WaveTrackWriter *wave_track = output_track->GetClipTrack()->GetWaveTrack();

                // Loop over the output track channels, where each has a mapping from an input track channel
                const map<uint32_t, OutputTrack::InputMap> &input_maps = output_track->GetInputMaps();
                map<uint32_t, OutputTrack::InputMap>::const_iterator iter;
                for (iter = input_maps.begin(); iter != input_maps.end(); iter++) {
                    uint32_t output_channel_index = iter->first;
                    uint32_t input_channel_index = iter->second.input_channel_index;

                    InputTrack *input_track = iter->second.input_track;
                    const MXFTrackReader *input_track_reader = dynamic_cast<MXFInputTrack*>(input_track)->GetTrackReader();

                    // Add all referenced chunks if not already present
                    for (size_t k = 0; k < input_track_reader->GetNumWaveChunks(); k++) {
                        MXFWaveChunk *wave_chunk = input_track_reader->GetWaveChunk(k);

                        // Don't write chunks in the exclusion list
                        if (exclude_all_wave_chunks || exclude_wave_chunks.count(wave_chunk->Id()) > 0)
                            continue;

                        if (!unique_chunk_stream_ids.count(wave_chunk->GetStreamId())) {
                            if (wave_clip->HaveChunk(wave_chunk->Id())) {
                                // E.g. this can happen if multiple <axml> chunks exist that came from different Wave files
                                // and the <axml> chunks may or may not be identical or equivalent.
                                // The assumption is that only 1 chunk with a given ID can exist in a BWF64 / Wave file.
                                // The exception is probably <JUNK>, but that shouldn't be transferred between Wave or MXF.
                                log_warn("Replaced chunk <%s> with another with the same ID in the output\n",
                                         get_wave_chunk_id_str(wave_chunk->Id()).c_str());
                            }
                            wave_clip->AddChunk(wave_chunk, false);
                            unique_chunk_stream_ids.insert(wave_chunk->GetStreamId());
                        }
                    }

                    if (!exclude_all_wave_chunks && exclude_wave_chunks.count(WAVE_CHUNK_ID("chna")) == 0) {
                        // Add audio IDs for the ADM <chna> chunk
                        vector<WaveCHNA::AudioID> audio_ids = input_track_reader->GetCHNAAudioIDs(input_channel_index);
                        for (size_t k = 0; k < audio_ids.size(); k++) {
                            WaveCHNA::AudioID &audio_id = audio_ids[k];

                            // Change the input channel track_index to the output channel track_index
                            // + 1 because the <chna> track_index starts at 1
                            audio_id.track_index = output_channel_index + 1;
                            wave_track->AddADMAudioID(audio_id);
                        }
                    }
                }
            }
        }


        // prepare the clip's header metadata and update file descriptors from input where supported

        clip->PrepareHeaderMetadata();

        if (!ignore_input_desc) {
            for (i = 0; i < output_tracks.size(); i++) {
                OutputTrack *output_track = output_tracks[i];
                if (output_track->HaveInputTrack()) {
                    InputTrack *first_track = output_track->GetFirstInputTrack();
                    const MXFTrackReader *input_track_reader = dynamic_cast<MXFInputTrack*>(first_track)->GetTrackReader();
                    MXFDescriptorHelper *desc_helper = output_track->GetClipTrack()->GetMXFDescriptorHelper();
                    if (input_track_reader && desc_helper) {
                        // Note: D10 PCM tracks won't have a FileDescriptor set (file_desc == 0 here)
                        // because a separate sound descriptor is created when preparing the header metadata.
                        // The structure of the D10 classes would need to be changed to support the update here,
                        // e.g. require a track map to be used to create a single D10PCMTrack rather than have
                        // The D10File accept creation of multiple tracks.
                        FileDescriptor *file_desc = desc_helper->GetFileDescriptor();

                        FileDescriptor *file_desc_in = input_track_reader->GetFileDescriptor();
                        if (file_desc && file_desc_in)
                            desc_helper->UpdateFileDescriptor(file_desc_in);
                    }
                }
            }
        }


        // add AS-10/11 descriptive metadata

        if (clip_sub_type == AS11_CLIP_SUB_TYPE) {
            as11_helper.AddMetadata(clip);

            if ((clip_type == CW_OP1A_CLIP_TYPE && (flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)) ||
                (clip_type == CW_D10_CLIP_TYPE  && (flavour & D10_SINGLE_PASS_WRITE_FLAVOUR)))
            {
                as11_helper.Complete();
            }
        } else if (clip_sub_type == AS10_CLIP_SUB_TYPE) {
            as10_helper.AddMetadata(clip);
        }


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


        // force file md5 update now that reading/seeking will be forwards only

        if (input_file_md5)
            file_factory.ForceInputChecksumUpdate();


        // set the sample sequence
        // read more than 1 sample to improve efficiency if the input is sound only and the output
        // doesn't require a sample sequence

        BMX_ASSERT(!output_tracks.empty());
        BMX_CHECK(!input_edit_rate_is_sampling_rate || output_tracks[0]->GetClipTrack()->GetEssenceType() == WAVE_PCM);
        vector<uint32_t> sample_sequence;
        uint32_t sample_sequence_offset = 0;
        uint32_t max_samples_per_read = 1;
        if (input_edit_rate_is_sampling_rate) {
            // the input edit rate is the sound sampling rate, which means the output is sound only as well
            if ((clip_type == CW_OP1A_CLIP_TYPE && clip->GetOP1AClip()->IsClipWrapped()) ||
                 clip_type == CW_AS02_CLIP_TYPE ||
                 clip_type == CW_WAVE_CLIP_TYPE)
            {
                // Don't restrict the output sound to frames if it's clip wrapped
                sample_sequence.push_back(1);
            }
            else
            {
                // set the sample sequence required for frame-wrapped output
                sample_sequence = output_tracks[0]->GetClipTrack()->GetShiftedSampleSequence();
            }

            // set max_samples_per_read > 1 if no sample sequence is needed for the output
            // reading multiple samples is more efficient in that case
            if (sample_sequence.size() == 1 && sample_sequence[0] == 1)
                max_samples_per_read = 1920;
        } else {
            // read 1 input frame
            sample_sequence.push_back(1);
        }
        BMX_ASSERT(max_samples_per_read == 1 || (precharge == 0 && rollout == 0));


        // realtime transwrapping

        uint64_t rt_start = 0;
        if (realtime)
            rt_start = get_tick_count();


        // growing input files

        unsigned int gf_retry_count = 0;
        bool gf_read_failure = false;
        int64_t gf_failure_num_read = 0;
        uint64_t gf_failure_start = 0;


        // create clip file(s) and write samples

        clip->PrepareWrite();

        float next_progress_update;
        init_progress(&next_progress_update);

        int64_t read_duration = reader->GetReadDuration();
        int64_t total_read = 0;
        bool even_frame = true;
        int64_t duration_at_precharge_end = -1;
        int64_t duration_at_rollout_start = -1;
        int64_t container_duration;
        int64_t prev_container_duration = -1;
        bmx::ByteArray sound_buffer;
        while (read_duration < 0 || total_read < read_duration) {
            uint32_t num_read = read_samples(reader, sample_sequence, &sample_sequence_offset, max_samples_per_read);
            if (num_read == 0) {
                if (!growing_file || !reader->ReadError() || gf_retry_count >= gf_retries)
                    break;
                gf_retry_count++;
                gf_read_failure = true;
                if (gf_retry_delay > 0.0) {
                    rt_sleep(1.0f / gf_retry_delay, get_tick_count(), frame_rate,
                             frame_rate.numerator / frame_rate.denominator);
                }
                continue;
            }
            if (growing_file && gf_retry_count > 0) {
                gf_failure_num_read = total_read;
                gf_failure_start    = get_tick_count();
                gf_retry_count      = 0;
            }

            // check whether any incomplete frames (where requested samples < read samples) are supported
            bool add_pcm_padding = false;
            for (i = 0; i < input_tracks.size(); i++) {
                MXFInputTrack *input_track = input_tracks[i];
                if (input_track->GetTrackInfo()->essence_type == TIMED_TEXT) {
                    // timed text is handled elsewhere
                    continue;
                }

                Frame *frame = input_track->GetFrameBuffer()->GetLastFrame(false);
                BMX_ASSERT(frame);

                // If a single output sample (edit unit) is read from the input and it is incomplete then
                // check if padding can be added
                if (max_samples_per_read == 1 && !frame->IsComplete()) {
                    const MXFTrackInfo *input_track_info = input_track->GetTrackInfo();
                    // only support padding with PCM samples and where the input edit rate equals audio sampling rate
                    if (input_track_info->essence_type != WAVE_PCM ||
                        input_track_info->edit_rate != ((MXFSoundTrackInfo*)input_track_info)->sampling_rate)
                    {
                        log_warn("Unable to provide PCM padding data for incomplete frame\n");
                        break;
                    }

                    // transferring partial frame data is only supported for the WAVE clip type
                    if (!frame->IsEmpty() && clip_type != CW_WAVE_CLIP_TYPE) {
                        log_warn("Transferring partial PCM frame data is only supported for %s\n",
                                 clip_type_to_string(CW_WAVE_CLIP_TYPE, NO_CLIP_SUB_TYPE));
                        break;
                    }

                    // only pad partial frames if not outputting to WAVE
                    if (clip_type != CW_WAVE_CLIP_TYPE)
                        add_pcm_padding = true;
                }
            }
            if (i < input_tracks.size())
                break;

            if (clip_type == CW_AS02_CLIP_TYPE && (precharge || rollout)) {
                container_duration = clip->GetDuration();
                if (total_read == - precharge)
                    duration_at_precharge_end = container_duration;
                if (total_read == read_duration - rollout) {
                    duration_at_rollout_start = container_duration;
                    // roundup for rollout
                    if (container_duration == prev_container_duration)
                        duration_at_rollout_start++;
                }
                prev_container_duration = container_duration;
            }

            uint32_t first_sound_num_samples = 0;
            for (i = 0; i < input_tracks.size(); i++) {
                MXFInputTrack *input_track = input_tracks[i];
                if (input_track->GetTrackInfo()->essence_type == TIMED_TEXT) {
                    // timed text is handled elsewhere
                    continue;
                }

                Frame *frame = input_track->GetFrameBuffer()->GetLastFrame(true);
                BMX_ASSERT(frame);

                if (clip_type == CW_AVID_CLIP_TYPE && convert_ess_marks) {
                    const vector<FrameMetadata*> *metadata = frame->GetMetadata(SDTI_CP_PACKAGE_METADATA_FMETA_ID);
                    if (metadata && !metadata->empty()) {
                        const SDTICPPackageMetadata *pkg_metadata =
                            dynamic_cast<const SDTICPPackageMetadata*>((*metadata)[0]);
                        if (!pkg_metadata->mEssenceMark.empty()) {
                            AvidLocator locator;
                            locator.position = frame->position - (read_start + precharge);
                            if (pkg_metadata->mEssenceMark[0] == '_')
                                locator.color = COLOR_RED;
                            else
                                locator.color = COLOR_GREEN;
                            locator.comment = pkg_metadata->mEssenceMark;
                            clip->GetAvidClip()->AddLocator(locator);
                        }
                    }
                }

                size_t k;
                for (k = 0; k < input_track->GetOutputTrackCount(); k++) {
                    OutputTrack *output_track = input_track->GetOutputTrack(k);
                    uint32_t output_channel_index = input_track->GetOutputChannelIndex(k);
                    uint32_t input_channel_index = input_track->GetInputChannelIndex(k);

                    const MXFTrackInfo *input_track_info = input_track->GetTrackInfo();
                    const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);

                    uint32_t bits_per_sample = 0;
                    uint16_t channel_block_align = 0;
                    if (input_sound_info) {
                        bits_per_sample     = input_sound_info->bits_per_sample;
                        channel_block_align = (bits_per_sample + 7) / 8;
                    }

                    uint32_t num_samples = 0;
                    if (output_track->HaveSkipPrecharge())
                    {
                        output_track->SkipPrecharge(num_read);
                    }
                    else if (!frame->IsEmpty())
                    {
                        if ((input_sound_info && input_sound_info->channel_count > 1) ||
                                input_track_info->essence_type == D10_AES3_PCM)
                        {
                            sound_buffer.Allocate(frame->GetSize()); // more than enough
                            if (input_track_info->essence_type == D10_AES3_PCM) {
                                convert_aes3_to_pcm(frame->GetBytes(), frame->GetSize(), ignore_d10_aes3_flags,
                                                    bits_per_sample, input_channel_index,
                                                    sound_buffer.GetBytes(), sound_buffer.GetAllocatedSize());
                                num_samples = get_aes3_sample_count(frame->GetBytes(), frame->GetSize());
                            } else {
                                deinterleave_audio(frame->GetBytes(), frame->GetSize(),
                                                   bits_per_sample,
                                                   input_sound_info->channel_count, input_channel_index,
                                                   sound_buffer.GetBytes(), sound_buffer.GetAllocatedSize());
                                num_samples = frame->GetSize() / (input_sound_info->channel_count * channel_block_align);
                            }

                            output_track->WriteSamples(output_channel_index,
                                                       sound_buffer.GetBytes(),
                                                       num_samples * channel_block_align,
                                                       num_samples);
                        }
                        else if (input_track_info->essence_type == ANC_DATA)
                        {
                            write_anc_samples(output_track, frame, pass_anc, strip_anc, anc_buffer);
                        }
                        else
                        {
                            if (input_sound_info)
                                num_samples = frame->GetSize() / channel_block_align;
                            else
                                num_samples = frame->num_samples;
                            output_track->WriteSamples(output_channel_index,
                                                       (unsigned char*)frame->GetBytes(),
                                                       frame->GetSize(),
                                                       num_samples);
                        }
                    }

                    if (add_pcm_padding && !frame->IsComplete()) {
                        BMX_ASSERT(input_track_info->essence_type == WAVE_PCM);
                        BMX_ASSERT(input_sound_info->edit_rate == input_sound_info->sampling_rate);
                        num_samples = frame->request_num_samples - frame->num_samples;
                        output_track->WritePaddingSamples(output_channel_index, num_samples);
                    }

                    if (input_sound_info && first_sound_num_samples == 0 && num_samples > 0)
                        first_sound_num_samples = num_samples;
                }

                delete frame;
            }

            // write samples for silence tracks
            for (i = 0; i < output_tracks.size(); i++) {
                OutputTrack *output_track = output_tracks[i];
                if (output_track->IsSilenceTrack()) {
                    if (output_track->HaveSkipPrecharge())
                        output_track->SkipPrecharge(num_read);
                    else
                        output_track->WriteSilenceSamples(first_sound_num_samples);
                }
            }


            if (rdd6_filename) {
                // expecting last track to be RDD-6 from an XML file
                BMX_ASSERT(!output_tracks.back()->HaveInputTrack() &&
                           !output_tracks.back()->IsSilenceTrack());

                if (rdd6_pair_in_frame || even_frame)
                    rdd6_frame.UpdateStaticFrame(&rdd6_static_sequence);

                if (rdd6_pair_in_frame) {
                    construct_anc_rdd6(&rdd6_frame, &rdd6_first_buffer, &rdd6_second_buffer, rdd6_sdid, rdd6_lines, &anc_buffer);
                } else {
                    if (even_frame)
                        construct_anc_rdd6_sub_frame(&rdd6_frame, true, &rdd6_first_buffer, rdd6_sdid, rdd6_lines[0], &anc_buffer);
                    else
                        construct_anc_rdd6_sub_frame(&rdd6_frame, false, &rdd6_second_buffer, rdd6_sdid, rdd6_lines[1], &anc_buffer);
                }
                output_tracks.back()->WriteSamples(0, anc_buffer.GetBytes(), anc_buffer.GetSize(), 1);

                if (rdd6_pair_in_frame || !even_frame)
                    rdd6_static_sequence.UpdateForNextStaticFrame();
                even_frame = !even_frame;
            }

            if (update_header) {
                clip->UpdateHeaderMetadata();
                update_header = false;
            }

            total_read += num_read;


            if (show_progress)
                print_progress(total_read, read_duration, &next_progress_update);

            if (max_samples_per_read > 1 && num_read < max_samples_per_read)
                break;

            if (gf_read_failure)
                rt_sleep(gf_rate_after_fail, gf_failure_start, frame_rate, total_read - gf_failure_num_read);
            else if (realtime)
                rt_sleep(rt_factor, rt_start, frame_rate, total_read);
        }
        if (reader->ReadError()) {
            bmx::log(reader->IsComplete() ? ERROR_LOG : WARN_LOG,
                     "A read error occurred: %s\n", reader->ReadErrorMessage().c_str());
            if (gf_retry_count >= gf_retries)
                log_warn("Reached maximum growing file retries, %u\n", gf_retries);
            if (reader->IsComplete())
                cmd_result = 1;
        }

        if (timed_text_only) {
            total_read = read_duration;
        }

        if (show_progress)
            print_progress(total_read, read_duration, 0);


        // set precharge and rollout for non-interleaved clip types

        if (clip_type == CW_AS02_CLIP_TYPE && (precharge || rollout)) {
            for (i = 0; i < output_tracks.size(); i++) {
                OutputTrack *output_track = output_tracks[i];
                AS02Track *as02_track = output_track->GetClipTrack()->GetAS02Track();
                int64_t container_duration = as02_track->GetContainerDuration();

                if (duration_at_precharge_end >= 0)
                    as02_track->SetOutputStartOffset(as02_track->ConvertClipDuration(duration_at_precharge_end));
                if (duration_at_rollout_start >= 0) {
                    int64_t end_offset = as02_track->ConvertClipDuration(duration_at_rollout_start) - container_duration;
                    if (end_offset < 0)
                        as02_track->SetOutputEndOffset(end_offset);
                    // note that end_offset could be > 0 if rounded up and there was a last incomplete frame
                }
            }
        }


        // complete AS-11 descriptive metadata

        if (clip_sub_type == AS11_CLIP_SUB_TYPE &&
                ((clip_type != CW_OP1A_CLIP_TYPE && clip_type != CW_D10_CLIP_TYPE) ||
                 (clip_type == CW_OP1A_CLIP_TYPE && !(flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)) ||
                 (clip_type == CW_D10_CLIP_TYPE  && !(flavour & D10_SINGLE_PASS_WRITE_FLAVOUR))))
        {
            as11_helper.Complete();
        }
        else if (clip_sub_type == AS10_CLIP_SUB_TYPE)
        {
            as10_helper.Complete();
        }

        // complete writing

        clip->CompleteWrite();

        log_info("Duration: %" PRId64 " (%s)\n",
                 clip->GetDuration(),
                 get_generic_duration_string_2(clip->GetDuration(), clip->GetFrameRate()).c_str());


        if (read_duration >= 0 && total_read != read_duration) {
            bool isError = reader->IsComplete() && total_read < read_duration - check_end_tolerance;

            bmx::log(isError ? ERROR_LOG : WARN_LOG,
                     "Read fewer samples (%" PRId64 ") than expected (%" PRId64 ")\n", total_read, read_duration);

            if (!op1a_clip_wrap &&
                input_edit_rate_is_sampling_rate &&  // reading sound samples rather than frames
                clip_type == CW_OP1A_CLIP_TYPE &&
                clip->GetOP1AClip()->IsFrameWrapped())
            {
                bmx::log(isError ? ERROR_LOG : WARN_LOG,
                         "Use the --clip-wrap option to transfer all audio samples\n");
            }

            if (isError)
                cmd_result = 1;
        }


        // output file md5

        if (output_file_md5) {
            if (clip_type == CW_OP1A_CLIP_TYPE) {
                OP1AFile *op1a_clip = clip->GetOP1AClip();

                log_info("Output file MD5: %s\n", op1a_clip->GetMD5DigestStr().c_str());
            } else if (clip_type == CW_D10_CLIP_TYPE) {
                D10File *d10_clip = clip->GetD10Clip();

                log_info("Output file MD5: %s\n", d10_clip->GetMD5DigestStr().c_str());
            } else if (clip_type == CW_RDD9_CLIP_TYPE) {
                RDD9File *rdd9_clip = clip->GetRDD9Clip();

                log_info("Output file MD5: %s\n", rdd9_clip->GetMD5DigestStr().c_str());
            }
        }


        // input file md5

        if (input_file_md5) {
            file_factory.FinalizeInputChecksum();
            size_t i;
            for (i = 0; i < file_factory.GetNumInputChecksumFiles(); i++) {
                log_info("Input file MD5: %s %s\n",
                         file_factory.GetInputChecksumFilename(i).c_str(),
                         file_factory.GetInputChecksumDigestString(i, MD5_CHECKSUM).c_str());
            }
        }


        delete reader;
        delete clip;
        for (i = 0; i < output_tracks.size(); i++)
            delete output_tracks[i];
        for (i = 0; i < input_tracks.size(); i++)
            delete input_tracks[i];
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
        cmd_result = (ex ? 0 : 1);
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
