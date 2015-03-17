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

#include <string>
#include <vector>
#include <map>

#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/MXFGroupReader.h>
#include <bmx/mxf_reader/MXFSequenceReader.h>
#include <bmx/mxf_reader/MXFFrameMetadata.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/essence_parser/MPEG2AspectRatioFilter.h>
#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/wave/WaveFileIO.h>
#include <bmx/st436/ST436Element.h>
#include <bmx/st436/RDD6Metadata.h>
#include <bmx/URI.h>
#include <bmx/MXFHTTPFile.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Version.h>
#include <bmx/apps/AppMXFFileFactory.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/apps/AS11Helper.h>
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
    const MXFTrackInfo *input_track_info;
    FrameBuffer *input_buffer;
    ClipWriterTrack *track;
    EssenceType essence_type;
    MXFDataDefEnum data_def;
    uint32_t channel_count;
    uint32_t channel_index;
    uint32_t bits_per_sample;
    uint16_t block_align;
    int64_t rem_precharge;
    EssenceFilter *filter;
} OutputTrack;

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

static const uint32_t DEFAULT_HTTP_MIN_READ = 64 * 1024;


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};



static bool filter_anc_manifest_element(const ANCManifestElement *element, set<ANCDataType> &filter)
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

static bool filter_anc_manifest(const MXFDataTrackInfo *data_info, set<ANCDataType> &filter)
{
    size_t i;
    for (i = 0; i < data_info->anc_manifest.size(); i++) {
        if (filter_anc_manifest_element(&data_info->anc_manifest[i], filter))
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

static void write_samples(OutputTrack *output_track, unsigned char *data, uint32_t size, uint32_t num_samples)
{
    if (output_track->filter) {
        if (output_track->filter->SupportsInPlaceFilter()) {
            output_track->filter->Filter(data, size);
            output_track->track->WriteSamples(data, size, num_samples);
        } else {
            unsigned char *f_data = 0;
            try
            {
                uint32_t f_size;
                output_track->filter->Filter(data, size, &f_data, &f_size);
                output_track->track->WriteSamples(f_data, f_size, num_samples);
                delete [] f_data;
            }
            catch (...)
            {
                delete [] f_data;
                throw;
            }
        }
    } else {
        output_track->track->WriteSamples(data, size, num_samples);
    }
}

static void write_padding_samples(Frame *frame, OutputTrack *output_track, bmx::ByteArray *buffer)
{
    const MXFSoundTrackInfo *sound_info = (const MXFSoundTrackInfo *)output_track->input_track_info;

    BMX_ASSERT(sound_info->essence_type == WAVE_PCM);
    BMX_ASSERT(sound_info->edit_rate == sound_info->sampling_rate);
    BMX_ASSERT(frame->request_num_samples > 0);

    uint32_t size = frame->request_num_samples * output_track->block_align;
    buffer->Allocate(size);
    memset(buffer->GetBytes(), 0, size);

    write_samples(output_track, buffer->GetBytes(), size, frame->request_num_samples);
}

static void write_anc_samples(Frame *frame, OutputTrack *output_track, set<ANCDataType> &filter, bmx::ByteArray &anc_buffer)
{
    BMX_CHECK(frame->num_samples == 1);

    if (filter.empty() || (filter.size() == 1 && (*filter.begin()) == ALL_ANC_DATA)) {
        write_samples(output_track, (unsigned char*)frame->GetBytes(), frame->GetSize(), frame->num_samples);
        return;
    }

    ST436Element input_element(false);
    input_element.Parse(frame->GetBytes(), frame->GetSize());

    ST436Element output_element(false);
    size_t i;
    for (i = 0; i < input_element.lines.size(); i++) {
        ANCManifestElement manifest_element;
        manifest_element.Parse(&input_element.lines[i]);
        if (filter_anc_manifest_element(&manifest_element, filter))
            output_element.lines.push_back(input_element.lines[i]);
    }

    anc_buffer.SetSize(0);
    output_element.Construct(&anc_buffer);

    write_samples(output_track, anc_buffer.GetBytes(), anc_buffer.GetSize(), 1);
}

static void clear_output_track(OutputTrack *output_track)
{
    delete output_track->filter;
}

static void disable_tracks(MXFReader *reader, const set<uint32_t> &track_indexes)
{
    set<uint32_t>::const_iterator iter;
    for (iter = track_indexes.begin(); iter != track_indexes.end(); iter++) {
        if (*iter < reader->GetNumTrackReaders())
            reader->GetTrackReader(*iter)->SetEnable(false);
    }
}

static bool parse_track_indexes(const char *tracks_str, set<uint32_t> *track_indexes)
{
    unsigned int first_index, last_index;
    const char *tracks_str_ptr = tracks_str;
    while (tracks_str_ptr) {
        size_t result = sscanf(tracks_str_ptr, "%u-%u", &first_index, &last_index);
        if (result == 2) {
            if (first_index > last_index)
                return false;
            uint32_t index;
            for (index = first_index; index <= last_index; index++)
                track_indexes->insert(index);
        } else if (result == 1) {
            track_indexes->insert(first_index);
        } else {
            return false;
        }

        tracks_str_ptr = strchr(tracks_str_ptr, ',');
        if (tracks_str_ptr)
            tracks_str_ptr++;
    }

    return true;
}

static void usage(const char *cmd)
{
    fprintf(stderr, "%s\n", get_app_version_info(APP_NAME).c_str());
    fprintf(stderr, "Usage: %s <<options>> [<<input options>> <mxf input>]+\n", cmd);
    fprintf(stderr, "   Use <mxf input> '-' for standard input\n");
    fprintf(stderr, "Options (* means option is required):\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -p                      Print progress percentage to stdout\n");
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
    fprintf(stderr, "  --input-file-md5        Calculate an MD5 checksum of the input file\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Override input start timecode. Default 00:00:00:00\n");
    fprintf(stderr, "                          The c character in the pattern should be ':' for non-drop frame; any other character indicates drop frame\n");
    fprintf(stderr, "  --mtc                   Check first and use the input material package start timecode if present\n");
    fprintf(stderr, "  --fstc                  Check first and use the file source package timecode if present\n");
    fprintf(stderr, "  --pstc                  Check first and use the physical source package timecode if present\n");
    fprintf(stderr, "  --tc-rate <rate>        Start timecode rate to use when input is audio only\n");
    fprintf(stderr, "                          Values are 23976 (24000/1001), 24, 25 (default), 2997 (30000/1001), 30, 50, 5994 (60000/1001) or 60\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --start <frame>         Set the start frame in input edit rate units. Default is 0\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames in input edit rate units. Default is minimum input duration\n");
    fprintf(stderr, "  --check-end             Check at the start that the last (start + duration - 1) frame can be read\n");
    fprintf(stderr, "  --check-complete        Check that the input file is complete\n");
    fprintf(stderr, "  --group                 Use the group reader instead of the sequence reader\n");
    fprintf(stderr, "                          Use this option if the files have different material packages\n");
    fprintf(stderr, "                          but actually belong to the same virtual package / group\n");
    fprintf(stderr, "  --no-reorder            Don't attempt to order the inputs in a sequence\n");
    fprintf(stderr, "                          Use this option for files with broken timecode\n");
    fprintf(stderr, "  --rt <factor>           Transwrap at realtime rate x <factor>, where <factor> is a floating point value\n");
    fprintf(stderr, "                          <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
    fprintf(stderr, "  --gf                    Support growing files. Retry reading a frame when it fails\n");
    fprintf(stderr, "  --gf-retries <max>      Set the maximum times to retry reading a frame. The default is %u.\n", DEFAULT_GF_RETRIES);
    fprintf(stderr, "  --gf-delay <sec>        Set the delay (in seconds) between a failure to read and a retry. The default is %f.\n", DEFAULT_GF_RETRY_DELAY);
    fprintf(stderr, "  --gf-rate <factor>      Limit the read rate to realtime rate x <factor> after a read failure. The default is %f\n", DEFAULT_GF_RATE_AFTER_FAIL);
    fprintf(stderr, "                          <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
    if (mxf_http_is_supported()) {
        fprintf(stderr, " --http-min-read <bytes>\n");
        fprintf(stderr, "                          Set the minimum number of bytes to read when accessing a file over HTTP. The default is %u.\n", DEFAULT_HTTP_MIN_READ);
    }
    fprintf(stderr, "  --no-precharge          Don't output clip/track with precharge. Adjust the start position and duration instead\n");
    fprintf(stderr, "  --no-rollout            Don't output clip/track with rollout. Adjust the duration instead\n");
    fprintf(stderr, "  --rw-intl               Interleave input reads with output writes\n");
    fprintf(stderr, "  --rw-intl-size          The interleave size. Default is %u\n", DEFAULT_RW_INTL_SIZE);
    fprintf(stderr, "                          Value must be a multiple of the system page size, %u\n", mxf_get_system_page_size());
#if defined(_WIN32)
    fprintf(stderr, "  --seq-scan              Set the sequential scan hint for optimizing file caching whilst reading\n");
#endif
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
    fprintf(stderr, "  -a <n:d>                Override or set the image aspect ratio. Either 4:3 or 16:9.\n");
    fprintf(stderr, "  --bsar                  Set image aspect ratio in video bitstream. Currently supports D-10 essence types only\n");
    fprintf(stderr, "  --locked <bool>         Override or set flag indicating whether the number of audio samples is locked to the video. Either true or false\n");
    fprintf(stderr, "  --audio-ref <level>     Override or set audio reference level, number of dBm for 0VU\n");
    fprintf(stderr, "  --dial-norm <value>     Override or set gain to be applied to normalize perceived loudness of the clip\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02:\n");
    fprintf(stderr, "    --mic-type <type>       Media integrity check type: 'md5' or 'none'. Default 'md5'\n");
    fprintf(stderr, "    --mic-file              Calculate checksum for entire essence component file. Default is essence only\n");
    fprintf(stderr, "    --shim-name <name>      Set ShimName element value in shim.xml file to <name>. Default is '%s'\n", DEFAULT_SHIM_NAME);
    fprintf(stderr, "    --shim-id <id>          Set ShimID element value in shim.xml file to <id>. Default is '%s'\n", DEFAULT_SHIM_ID);
    fprintf(stderr, "    --shim-annot <str>      Set AnnotationText element value in shim.xml file to <str>. Default is '%s'\n", DEFAULT_SHIM_ANNOTATION);
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02/as11op1a/op1a/rdd9:\n");
    fprintf(stderr, "    --part <interval>       Video essence partition interval in frames in input edit rate units, or seconds with 's' suffix. Default single partition\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10:\n");
    fprintf(stderr, "    --dm <fwork> <name> <value>    Set descriptive framework property. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --dm-file <fwork> <name>       Parse and set descriptive framework properties from text file <name>. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --seg <name>                   Parse and set segmentation data from text file <name>\n");
    fprintf(stderr, "    --pass-dm                      Copy descriptive metadata from the input file. The metadata can be overidden by other options\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02/as11op1a/as11d10/op1a/d10/rdd9:\n");
    fprintf(stderr, "    --afd <value>           Active Format Descriptor 4-bit code from table 1 in SMPTE ST 2016-1. Default is input file's value or not set\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10/op1a/d10/rdd9:\n");
    fprintf(stderr, "    --single-pass           Write file in a single pass\n");
    fprintf(stderr, "                            Header and body partitions will be incomplete for as11op1a/op1a if the number if essence container bytes per edit unit is variable\n");
    fprintf(stderr, "    --file-md5              Calculate an MD5 checksum of the output file. This requires writing in a single pass (--single-pass is assumed)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/op1a:\n");
    fprintf(stderr, "    --pass-anc <filter>     Pass through ST 436 ANC data tracks\n");
    fprintf(stderr, "                            <filter> is a comma separated list of ANC data types to pass through\n");
    fprintf(stderr, "                            The following ANC data types are supported in <filter>:\n");
    fprintf(stderr, "                                all      : pass through all ANC data\n");
    fprintf(stderr, "                                st2020   : SMPTE ST 2020 / RDD 6 audio metadata\n");
    fprintf(stderr, "                                st2016   : SMPTE ST 2016-3/ AFD, bar and pan-scan data\n");
    fprintf(stderr, "                                sdp      : SMPTE RDD 8 / OP-47 Subtitling Distribution Packet data\n");
    fprintf(stderr, "                                st12     : SMPTE ST 12 Ancillary timecode\n");
    fprintf(stderr, "                                st334    : SMPTE ST 334-1 EIA 708B, EIA 608 and data broadcast (DTV)\n");
    fprintf(stderr, "    --pass-vbi              Pass through ST 436 VBI data tracks\n");
    fprintf(stderr, "    --st436-mf <count>      Set the <count> of frames to examine for ST 436 ANC/VBI manifest info. Default is %u\n", DEFAULT_ST436_MANIFEST_COUNT);
    fprintf(stderr, "                            The manifest is used at the start to determine whether an output ANC data track is created\n");
    fprintf(stderr, "                            Set <count> to 0 to always create an ANC data track if the input has one\n");
    fprintf(stderr, "    --anc-const <size>      Use to indicate that the ST 436 ANC frame element data, excl. key and length, has a constant <size>. A variable size is assumed by default\n");
    fprintf(stderr, "    --anc-max <size>        Use to indicate that the ST 436 ANC frame element data, excl. key and length, has a maximum <size>. A variable size is assumed by default\n");
    fprintf(stderr, "    --st2020-max            The ST 436 ANC maximum frame element data size for ST 2020 only is calculated from the extracted manifest. Option '--pass-anc st2020' is required.\n");
    fprintf(stderr, "    --vbi-const <size>      Use to indicate that the ST 436 VBI frame element data, excl. key and length, has a constant <size>. A variable size is assumed by default\n");
    fprintf(stderr, "    --vbi-max <size>        Use to indicate that the ST 436 VBI frame element data, excl. key and length, has a maximum <size>. A variable size is assumed by default\n");
    fprintf(stderr, "    --rdd6 <file>           Add ST 436 ANC data track containing 'static' RDD-6 audio metadata from XML <file>\n");
    fprintf(stderr, "                            The timecode fields are ignored, i.e. they are set to 'undefined' values in the RDD-6 metadata stream\n");
    fprintf(stderr, "    --rdd6-lines <lines>    The line numbers for carriage of the RDD-6 ANC data. <lines> is a pair of numbers separated by a ','. Default is '%u,%u'\n", DEFAULT_RDD6_LINES[0], DEFAULT_RDD6_LINES[1]);
    fprintf(stderr, "    --rdd6-sdid <sdid>      The SDID value indicating the first audio channel pair associated with the RDD-6 data. Default is %u\n", DEFAULT_RDD6_SDID);
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
    fprintf(stderr, "                            Add locator at <position> with <comment> and <color>\n");
    fprintf(stderr, "                            <position> format is o?hh:mm:sscff, where the optional 'o' indicates it is an offset\n");
    fprintf(stderr, "    --mp-uid <umid>         Set the Material Package UID. Autogenerated by default\n");
    fprintf(stderr, "    --mp-created <tstamp>   Set the Material Package creation date. Default is 'now'\n");
    fprintf(stderr, "    --psp-uid <umid>        Set the tape/import Source Package UID\n");
    fprintf(stderr, "                              tape: autogenerated by default\n");
    fprintf(stderr, "                              import: single input Material Package UID or autogenerated by default\n");
    fprintf(stderr, "    --psp-created <tstamp>  Set the tape/import Source Package creation date. Default is 'now'\n");
    fprintf(stderr, "    --ess-marks             Convert XDCAM Essence Marks to locators\n");
    fprintf(stderr, "    --allow-no-avci-head    Allow inputs with no AVCI header (512 bytes, sequence and picture parameter sets)\n");
    fprintf(stderr, "    --avid-gf               Use the Avid growing file flavour\n");
    fprintf(stderr, "    --avid-gf-dur <dur>     Set the duration which should be shown whilst the file is growing\n");
    fprintf(stderr, "                            The default value is the output duration\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  op1a/avid:\n");
    fprintf(stderr, "    --force-no-avci-head    Strip AVCI header (512 bytes, sequence and picture parameter sets) if present\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  wave:\n");
    fprintf(stderr, "    --orig <name>           Set originator in the Wave bext chunk. Default '%s'\n", DEFAULT_BEXT_ORIGINATOR);
    fprintf(stderr, "\n");
    fprintf(stderr, "Input options:\n");
    fprintf(stderr, "  --disable-tracks <tracks> A comma separated list of track indexes and/or ranges to disable.\n");
    fprintf(stderr, "                            A track is identified by the index reported by mxf2raw\n");
    fprintf(stderr, "                            A range of track indexes is specified as '<first>-<last>', e.g. 0-3\n");
    fprintf(stderr, "\n\n");
    fprintf(stderr, "Notes:\n");
    fprintf(stderr, " - <umid> format is 64 hexadecimal characters and any '.' and '-' characters are ignored\n");
    fprintf(stderr, " - <uuid> format is 32 hexadecimal characters and any '.' and '-' characters are ignored\n");
    fprintf(stderr, " - <tstamp> format is YYYY-MM-DDThh:mm:ss:qm where qm is in units of 1/250th second\n");
}

int main(int argc, const char** argv)
{
    Rational timecode_rate = FRAME_RATE_25;
    bool timecode_rate_set = false;
    vector<const char *> input_filenames;
    map<size_t, set<uint32_t> > disable_track_indexes;
    const char *log_filename = 0;
    LogLevel log_level = INFO_LOG;
    ClipWriterType clip_type = CW_OP1A_CLIP_TYPE;
    ClipSubType clip_sub_type = NO_CLIP_SUB_TYPE;
    bool ard_zdf_hdf_profile = false;
    const char *output_name = "";
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    bool use_mtc = false;
    bool use_fstc = false;
    bool use_pstc = false;
    int64_t start = 0;
    bool start_set = false;
    int64_t duration = -1;
    bool check_end = false;
    bool check_complete = false;
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
    bool pass_dm = false;
    const char *segmentation_filename = 0;
    bool do_print_version = false;
    bool use_group_reader = false;
    bool keep_input_order = false;
    uint8_t user_afd = 0;
    vector<AVCIHeaderInput> avci_header_inputs;
    bool show_progress = false;
    bool single_pass = false;
    bool output_file_md5 = false;
    Rational user_aspect_ratio = ASPECT_RATIO_16_9;
    bool user_aspect_ratio_set = false;
    bool set_bs_aspect_ratio = false;
    bool user_locked = false;
    bool user_locked_set = false;
    int8_t user_audio_ref_level = 0;
    bool user_audio_ref_level_set = false;
    int8_t user_dial_norm = 0;
    bool user_dial_norm_set = false;
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
    UMID mp_uid;
    bool mp_uid_set = false;
    Timestamp mp_created;
    bool mp_created_set = false;
    UMID psp_uid;
    bool psp_uid_set = false;
    Timestamp psp_created;
    bool psp_created_set = false;
    bool convert_ess_marks = false;
    bool allow_no_avci_head = false;
    bool force_no_avci_head = false;
    bool min_part = false;
    bool body_part = false;
    bool clip_wrap = false;
    bool realtime = false;
    float rt_factor = 1.0;
    bool growing_file = false;
    unsigned int gf_retries = DEFAULT_GF_RETRIES;
    float gf_retry_delay = DEFAULT_GF_RETRY_DELAY;
    float gf_rate_after_fail = DEFAULT_GF_RATE_AFTER_FAIL;
    bool product_info_set = false;
    string company_name;
    string product_name;
    mxfProductVersion product_version;
    string version_string;
    UUID product_uid;
    bool ps_avcihead = false;
    bool replace_avid_avcihead = false;
    bool avid_gf = false;
    int64_t avid_gf_duration = -1;
    set<ANCDataType> pass_anc;
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
        else if (strcmp(argv[cmdln_index], "-p") == 0)
        {
            show_progress = true;
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
        else if (strcmp(argv[cmdln_index], "--input-file-md5") == 0)
        {
            input_file_md5 = true;
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
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_frame_rate(argv[cmdln_index + 1], &timecode_rate))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            timecode_rate_set = true;
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
        else if (strcmp(argv[cmdln_index], "--start") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%"PRId64, &start) != 1 || start < 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            start_set = true;
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
        else if (strcmp(argv[cmdln_index], "--check-end") == 0)
        {
            check_end = true;
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
        else if (strcmp(argv[cmdln_index], "--gf") == 0)
        {
            growing_file = true;
        }
        else if (strcmp(argv[cmdln_index], "--gf-retries") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &gf_retries) != 1 || gf_retries == 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            growing_file = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--gf-delay") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%f", &gf_retry_delay) != 1 || gf_retry_delay < 0.0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            growing_file = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--gf-rate") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%f", &gf_rate_after_fail) != 1 || gf_rate_after_fail <= 0.0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            growing_file = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--http-min-read") == 0)
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
            http_min_read = (uint32_t)(uvalue);
            cmdln_index++;
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
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &uvalue) != 1 ||
                (uvalue % system_page_size) != 0)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
#endif
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
        else if (strcmp(argv[cmdln_index], "--replace-avid-avcihead") == 0)
        {
            replace_avid_avcihead = true;
        }
        else if (strcmp(argv[cmdln_index], "-a") == 0)
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
                user_aspect_ratio = ASPECT_RATIO_4_3;
            else
                user_aspect_ratio = ASPECT_RATIO_16_9;
            user_aspect_ratio_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--bsar") == 0)
        {
            set_bs_aspect_ratio = true;
        }
        else if (strcmp(argv[cmdln_index], "--locked") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_bool(argv[cmdln_index + 1], &user_locked))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            user_locked_set = true;
            cmdln_index++;
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
            user_audio_ref_level = value;
            user_audio_ref_level_set = true;
            cmdln_index++;
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
            user_dial_norm = value;
            user_dial_norm_set = true;
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
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument(s) for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            segmentation_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--pass-dm") == 0)
        {
            pass_dm = true;
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
            user_afd = value;
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
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_anc_data_types(argv[cmdln_index + 1], &pass_anc))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
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
            st436_manifest_count = (uint32_t)(uvalue);
            cmdln_index++;
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
            anc_const_size = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--anc-max") == 0)
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
            vbi_const_size = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--vbi-max") == 0)
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
            vbi_max_size = (uint32_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd6") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            rdd6_filename = argv[cmdln_index + 1];
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd6-lines") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_rdd6_lines(argv[cmdln_index + 1], rdd6_lines))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd6-sdid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (sscanf(argv[cmdln_index + 1], "%u", &uvalue) != 1 ||
                uvalue < 1 || uvalue > 9)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            rdd6_sdid = (uint8_t)(uvalue);
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--min-part") == 0)
        {
            min_part = true;
        }
        else if (strcmp(argv[cmdln_index], "--body-part") == 0)
        {
            body_part = true;
        }
        else if (strcmp(argv[cmdln_index], "--clip-wrap") == 0)
        {
            clip_wrap = true;
        }
        else if (strcmp(argv[cmdln_index], "--zero-mp-track-num") == 0)
        {
            zero_mp_track_num = true;
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
        else
        {
            break;
        }
    }

    if (st2020_max_size && (pass_anc.size() != 1 || *pass_anc.begin() != ST2020_ANC_DATA)) {
        usage(argv[0]);
        fprintf(stderr, "Option '--st2020-max' requires '--pass st2020'\n");
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
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_track_indexes(argv[cmdln_index + 1], &disable_track_indexes[input_filenames.size()]))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
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
                        usage(argv[0]);
                        fprintf(stderr, "Unknown argument '%s'\n", argv[cmdln_index]);
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
        usage(argv[0]);
        fprintf(stderr, "No input filenames\n");
        return 1;
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
        // open RDD-6 XML file

        RDD6MetadataSequence rdd6_static_sequence;
        RDD6MetadataFrame rdd6_frame;
        bmx::ByteArray rdd6_first_buffer, rdd6_second_buffer;
        bmx::ByteArray anc_buffer;
        uint32_t rdd6_const_size = 0;
        if (rdd6_filename) {
            if (clip_type != CW_OP1A_CLIP_TYPE) {
                log_error("RDD-6 file input only supported for '%s' and '%s' clip types\n",
                          clip_type_to_string(CW_OP1A_CLIP_TYPE, NO_CLIP_SUB_TYPE).c_str(),
                          clip_type_to_string(CW_OP1A_CLIP_TYPE, AS11_CLIP_SUB_TYPE).c_str());
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

            rdd6_frame.InitStaticSequence(&rdd6_static_sequence);

            rdd6_frame.UpdateStaticFrame(&rdd6_static_sequence);
            construct_anc_rdd6(&rdd6_frame, &rdd6_first_buffer, &rdd6_second_buffer, rdd6_sdid, rdd6_lines, &anc_buffer);
            rdd6_const_size = anc_buffer.GetSize();
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

        if (use_group_reader && input_filenames.size() > 1) {
            MXFGroupReader *group_reader = new MXFGroupReader();
            MXFFileReader::OpenResult result;
            size_t i;
            for (i = 0; i < input_filenames.size(); i++) {
                MXFFileReader *grp_file_reader = new MXFFileReader();
                grp_file_reader->SetFileFactory(&file_factory, false);
                grp_file_reader->GetPackageResolver()->SetFileFactory(&file_factory, false);
                grp_file_reader->SetST436ManifestFrameCount(st436_manifest_count);
                result = grp_file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", input_filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    throw false;
                }
                disable_tracks(grp_file_reader, disable_track_indexes[i]);
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
                result = seq_file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", input_filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    throw false;
                }
                disable_tracks(seq_file_reader, disable_track_indexes[i]);
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
            if (pass_dm && clip_sub_type == AS11_CLIP_SUB_TYPE)
                AS11Info::RegisterExtensions(file_reader->GetDataModel());
            result = file_reader->Open(input_filenames[0]);
            if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                log_error("Failed to open MXF file '%s': %s\n", input_filenames[0],
                          MXFFileReader::ResultToString(result).c_str());
                throw false;
            }
            disable_tracks(file_reader, disable_track_indexes[0]);

            reader = file_reader;
        }

        if (!reader->IsComplete()) {
            if (check_complete) {
                log_error("Input file is incomplete\n");
                throw false;
            }
            log_warn("Input file is incomplete\n");
        }

        reader->SetEmptyFrames(true);

        Rational frame_rate = reader->GetEditRate();


        // check whether the frame rate is a sound sampling rate
        // a frame rate that is a sound sampling rate will result in the timecode rate being used as
        // the clip frame rate and the timecode rate defaulting to 25fps if not set by the user

        bool is_sound_frame_rate = false;
        size_t i;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            const MXFSoundTrackInfo *input_sound_info =
                dynamic_cast<const MXFSoundTrackInfo*>(reader->GetTrackReader(i)->GetTrackInfo());

            if (input_sound_info && frame_rate == input_sound_info->sampling_rate) {
                is_sound_frame_rate = true;
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

            bool is_enabled = true;
            if (input_track_info->essence_type == WAVE_PCM)
            {
                Rational sampling_rate = input_sound_info->sampling_rate;
                if (!ClipWriterTrack::IsSupported(clip_type, WAVE_PCM, sampling_rate)) {
                    log_warn("Track %"PRIszt" essence type '%s' @%d/%d sps not supported by clip type '%s'\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             sampling_rate.numerator, sampling_rate.denominator,
                             clip_type_to_string(clip_type, clip_sub_type).c_str());
                    is_enabled = false;
                } else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32) {
                    log_warn("Track %"PRIszt" (%s) bits per sample %u not supported\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             input_sound_info->bits_per_sample);
                    is_enabled = false;
                } else if (input_sound_info->channel_count == 0) {
                    log_warn("Track %"PRIszt" (%s) has zero channel count\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type));
                    is_enabled = false;
                }
            }
            else if (input_track_info->essence_type == D10_AES3_PCM)
            {
                if (input_sound_info->sampling_rate != SAMPLING_RATE_48K)
                {
                    log_warn("Track %"PRIszt" essence type D-10 AES-3 audio sampling rate %d/%d not supported\n",
                             i,
                             input_sound_info->sampling_rate.numerator, input_sound_info->sampling_rate.denominator);
                    is_enabled = false;
                }
                else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32)
                {
                    log_warn("Track %"PRIszt" essence type D-10 AES-3 audio bits per sample %u not supported\n",
                             i,
                             input_sound_info->bits_per_sample);
                    is_enabled = false;
                }
            }
            else if (input_track_info->essence_type == UNKNOWN_ESSENCE_TYPE ||
                     input_track_info->essence_type == PICTURE_ESSENCE ||
                     input_track_info->essence_type == SOUND_ESSENCE ||
                     input_track_info->essence_type == DATA_ESSENCE)
            {
                log_warn("Track %"PRIszt" has unknown essence type\n", i);
                is_enabled = false;
            }
            else
            {
                if (input_track_info->edit_rate != frame_rate) {
                    log_warn("Track %"PRIszt" (essence type '%s') edit rate %d/%d does not equals clip edit rate %d/%d\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             input_track_info->edit_rate.numerator, input_track_info->edit_rate.denominator,
                             frame_rate.numerator, frame_rate.denominator);
                    is_enabled = false;
                } else if (!ClipWriterTrack::IsSupported(clip_type, input_track_info->essence_type, frame_rate)) {
                    log_warn("Track %"PRIszt" essence type '%s' @%d/%d fps not supported by clip type '%s'\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             frame_rate.numerator, frame_rate.denominator,
                             clip_type_to_string(clip_type, clip_sub_type).c_str());
                    is_enabled = false;
                } else if (input_track_info->essence_type == VBI_DATA) {
                    if (!pass_vbi) {
                        log_warn("Not passing through VBI data track %"PRIszt"\n", i);
                        is_enabled = false;
                    } else if (have_vbi_track) {
                        log_warn("Already have a VBI track; not passing through VBI data track %"PRIszt"\n", i);
                        is_enabled = false;
                    } else {
                        have_vbi_track = true;
                    }
                } else if (input_track_info->essence_type == ANC_DATA) {
                    if (rdd6_filename) {
                        log_warn("Mixing RDD-6 file input and MXF ANC data input not yet supported\n");
                        is_enabled = false;
                    } else if (pass_anc.empty()) {
                        log_warn("Not passing through ANC data track %"PRIszt"\n", i);
                        is_enabled = false;
                    } else if (have_anc_track) {
                        log_warn("Already have an ANC track; not passing through ANC data track %"PRIszt"\n", i);
                        is_enabled = false;
                    } else {
                        if (st436_manifest_count == 0 || filter_anc_manifest(input_data_info, pass_anc)) {
                            have_anc_track = true;
                        } else {
                            log_warn("No match found in ANC data manifest; not passing through ANC data track %"PRIszt"\n", i);
                            is_enabled = false;
                        }
                    }
                }

                if ((input_track_info->essence_type == AVCI200_1080I ||
                         input_track_info->essence_type == AVCI200_1080P ||
                         input_track_info->essence_type == AVCI200_720P ||
                         input_track_info->essence_type == AVCI100_1080I ||
                         input_track_info->essence_type == AVCI100_1080P ||
                         input_track_info->essence_type == AVCI100_720P ||
                         input_track_info->essence_type == AVCI50_1080I ||
                         input_track_info->essence_type == AVCI50_1080P ||
                         input_track_info->essence_type == AVCI50_720P) &&
                    !force_no_avci_head &&
                    !allow_no_avci_head &&
                    !track_reader->HaveAVCIHeader() &&
                    !(ps_avcihead &&
                        have_ps_avci_header_data(input_track_info->essence_type, input_track_info->edit_rate)) &&
                    !have_avci_header_data(input_track_info->essence_type, input_track_info->edit_rate,
                                           avci_header_inputs))
                {
                    log_warn("Track %"PRIszt" (essence type '%s') does not have sequence and picture parameter sets\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type));
                    is_enabled = false;
                }
            }

            if (!is_enabled) {
                log_warn("Ignoring track %"PRIszt" (essence type '%s')\n",
                          i, essence_type_to_string(input_track_info->essence_type));
            }

            track_reader->SetEnable(is_enabled);
        }

        if (!reader->IsEnabled()) {
            log_error("All tracks are disabled\n");
            throw false;
        }


        // check RDD-6 support

        if (rdd6_filename) {
            if (frame_rate != FRAME_RATE_25) {
                log_error("Frame rate %d/%d is currently not supported with RDD-6\n",
                          frame_rate.numerator, frame_rate.denominator);
                throw false;
            }
            for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                MXFTrackReader *track_reader = reader->GetTrackReader(i);
                if (!track_reader->IsEnabled())
                    continue;

                const MXFPictureTrackInfo *picture_info =
                    dynamic_cast<const MXFPictureTrackInfo*>(track_reader->GetTrackInfo());
                if (picture_info &&
                    picture_info->frame_layout != MXF_SEPARATE_FIELDS &&
                    picture_info->frame_layout != MXF_MIXED_FIELDS)
                {
                    log_error("Track %"PRIszt" frame layout %u is currently not supported with RDD-6\n",
                              i, picture_info->frame_layout);
                    throw false;
                }
            }
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
                log_error("Start position %"PRId64" is >= input duration %"PRId64"\n",
                          start, reader->GetDuration());
                throw false;
            }
            int64_t output_duration;
            if (duration >= 0) {
                output_duration = duration;
                if (read_start + output_duration > reader->GetDuration()) {
                    log_warn("Limiting duration %"PRId64" because it exceeds the available duration %"PRId64"\n",
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
                                 clip_type_to_string(clip_type, clip_sub_type).c_str());
                    }
                    log_info("Rollout resulted in %"PRId64" frame adjustment of duration\n",
                             output_duration - original_output_duration);
                }
            }

            reader->SetReadLimits(read_start + precharge, - precharge + output_duration + rollout, true);

            if (check_end && !reader->CheckReadLastFrame()) {
                log_error("Check for last frame failed\n");
                throw false;
            }
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
                if (!reader->HaveFixedLeadFillerOffset())
                    log_warn("No fixed lead filler offset\n");
                else
                    start_timecode.AddOffset(reader->GetFixedLeadFillerOffset(), frame_rate);
            }
        }


        // complete commandline parsing

        if (!timecode_rate_set && !is_sound_frame_rate)
            timecode_rate = frame_rate;
        if (start_timecode_str && !parse_timecode(start_timecode_str, timecode_rate, &start_timecode)) {
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
            Timecode position_start_timecode = start_timecode;
            if (position_start_timecode.IsInvalid())
                position_start_timecode.Init(timecode_rate, false);

            if (!parse_position(locators[i].position_str, position_start_timecode, timecode_rate,
                                &locators[i].locator.position))
            {
                usage(argv[0]);
                log_error("Invalid value '%s' for option '--locator'\n", locators[i].position_str);
                throw false;
            }
        }


        // copy across input file descriptive metadata

        if (pass_dm && clip_sub_type == AS11_CLIP_SUB_TYPE) {
            if (start != 0 || (duration >= 0 && duration < reader->GetDuration())) {
                log_error("Copying AS-11 descriptive metadata is currently only supported for complete file transwraps\n");
                throw false;
            }

            MXFFileReader *file_reader = dynamic_cast<MXFFileReader*>(reader);
            if (!file_reader) {
                log_error("Passing through AS-11 descriptive metadata is only supported for a single input file\n");
                throw false;
            }
            as11_helper.ReadSourceInfo(file_reader);
        }


        // create output clip and initialize

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
            if (output_file_md5)
                flavour |= OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= OP1A_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_D10_CLIP_TYPE) {
            flavour = D10_DEFAULT_FLAVOUR;
            if (output_file_md5)
                flavour |= D10_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= D10_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            if (ard_zdf_hdf_profile)
                flavour = RDD9_ARD_ZDF_HDF_PROFILE_FLAVOUR;
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
        Rational clip_frame_rate = (is_sound_frame_rate ? timecode_rate : frame_rate);
        switch (clip_type)
        {
            case CW_AS02_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS02Clip(output_name, true, clip_frame_rate, &file_factory, false);
                break;
            case CW_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewOP1AClip(flavour, file_factory.OpenNew(output_name), clip_frame_rate);
                break;
            case CW_AVID_CLIP_TYPE:
                clip = ClipWriter::OpenNewAvidClip(flavour, clip_frame_rate, &file_factory, false);
                break;
            case CW_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewD10Clip(flavour, file_factory.OpenNew(output_name), clip_frame_rate);
                break;
            case CW_RDD9_CLIP_TYPE:
                clip = ClipWriter::OpenNewRDD9Clip(flavour, file_factory.OpenNew(output_name), clip_frame_rate);
                break;
            case CW_WAVE_CLIP_TYPE:
                clip = ClipWriter::OpenNewWaveClip(WaveFileIO::OpenNew(output_name));
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

            if (flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)
                op1a_clip->SetInputDuration(reader->GetReadDuration());

            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                op1a_clip->ReserveHeaderMetadataSpace(16384); // min is 8192

            if (clip_sub_type != AS11_CLIP_SUB_TYPE)
                op1a_clip->SetClipWrapped(clip_wrap);
            if (partition_interval_set)
                op1a_clip->SetPartitionInterval(partition_interval);
            op1a_clip->SetOutputStartOffset(- precharge);
            op1a_clip->SetOutputEndOffset(- rollout);
        } else if (clip_type == CW_AVID_CLIP_TYPE) {
            AvidClip *avid_clip = clip->GetAvidClip();

            if (avid_gf) {
                if (avid_gf_duration < 0)
                    avid_clip->SetGrowingDuration(reader->GetReadDuration());
                else
                    avid_clip->SetGrowingDuration(avid_gf_duration);
            }

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

            d10_clip->SetMuteSoundFlags(d10_mute_sound_flags);
            d10_clip->SetInvalidSoundFlags(d10_invalid_sound_flags);

            if (flavour & D10_SINGLE_PASS_WRITE_FLAVOUR)
                d10_clip->SetInputDuration(reader->GetReadDuration());

            if (clip_sub_type == AS11_CLIP_SUB_TYPE)
                d10_clip->ReserveHeaderMetadataSpace(16384); // min is 8192
        } else if (clip_type == CW_RDD9_CLIP_TYPE) {
            RDD9File *rdd9_clip = clip->GetRDD9Clip();

            if (partition_interval_set)
                rdd9_clip->SetPartitionInterval(partition_interval);
            rdd9_clip->SetOutputStartOffset(- precharge);
            rdd9_clip->SetOutputEndOffset(- rollout);
        } else if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();

            if (originator)
                wave_clip->GetBroadcastAudioExtension()->SetOriginator(originator);
        }


        // create output tracks and initialize

        vector<OutputTrack> output_tracks;
        size_t input_to_output_count = 0;

        unsigned char avci_header_data[AVCI_HEADER_SIZE];
        map<MXFDataDefEnum, uint32_t> track_count;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *input_track_reader = reader->GetTrackReader(i);
            if (!input_track_reader->IsEnabled())
                continue;

            const MXFTrackInfo *input_track_info = input_track_reader->GetTrackInfo();
            const MXFPictureTrackInfo *input_picture_info = dynamic_cast<const MXFPictureTrackInfo*>(input_track_info);
            const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);

            uint8_t afd = user_afd;
            if (!afd && input_picture_info)
                afd = input_picture_info->afd;


            // create track(s)

            uint32_t output_track_count = 1;
            if (input_sound_info)
                output_track_count = input_sound_info->channel_count;
            BMX_CHECK(output_track_count > 0);

            uint32_t c;
            for (c = 0; c < output_track_count; c++) {
                OutputTrack output_track;
                memset(&output_track, 0, sizeof(output_track));

                output_track.input_track_info    = input_track_info;
                output_track.input_buffer        = input_track_reader->GetFrameBuffer();
                output_track.data_def            = input_track_info->data_def;
                output_track.channel_count       = output_track_count;
                output_track.channel_index       = c;
                if (input_track_info->essence_type == D10_AES3_PCM)
                    output_track.essence_type    = WAVE_PCM;
                else
                    output_track.essence_type    = input_track_info->essence_type;
                if (input_sound_info) {
                    output_track.bits_per_sample = input_sound_info->bits_per_sample;
                    output_track.block_align     = (input_sound_info->bits_per_sample + 7) / 8;
                } else {
                    output_track.bits_per_sample = 0;
                    output_track.block_align     = 0;
                }
                output_track.rem_precharge       = 0;

                if (clip_type == CW_AVID_CLIP_TYPE) {
                    string track_name = create_mxf_track_filename(output_name,
                                                                  track_count[input_track_info->data_def] + 1,
                                                                  input_track_info->data_def);
                    output_track.track = clip->CreateTrack(output_track.essence_type, track_name.c_str());
                } else {
                    output_track.track = clip->CreateTrack(output_track.essence_type);
                }


                // initialize track

                // TODO: track number setting and check AES-3 channel validity

                if (clip_type == CW_AS02_CLIP_TYPE) {
                    AS02Track *as02_track = output_track.track->GetAS02Track();
                    as02_track->SetMICType(mic_type);
                    as02_track->SetMICScope(ess_component_mic_scope);

                    if (partition_interval_set) {
                        AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(as02_track);
                        if (as02_pict_track)
                            as02_pict_track->SetPartitionInterval(partition_interval);
                    }
                } else if (clip_type == CW_AVID_CLIP_TYPE) {
                    AvidTrack *avid_track = output_track.track->GetAvidTrack();

                    if (avid_track->SupportOutputStartOffset())
                        avid_track->SetOutputStartOffset(- precharge);
                    else
                        output_track.rem_precharge = - precharge; // skip precharge frames

                    if (physical_package) {
                        if (input_track_info->data_def == MXF_PICTURE_DDEF) {
                            avid_track->SetSourceRef(physical_package_picture_refs[track_count[input_track_info->data_def]].first,
                                                     physical_package_picture_refs[track_count[input_track_info->data_def]].second);
                        } else if (input_track_info->data_def == MXF_SOUND_DDEF) {
                            avid_track->SetSourceRef(physical_package_sound_refs[track_count[input_track_info->data_def]].first,
                                                     physical_package_sound_refs[track_count[input_track_info->data_def]].second);
                        }
                    }
                }

                switch (output_track.essence_type)
                {
                    case IEC_DV25:
                    case DVBASED_DV25:
                    case DV50:
                        if (user_aspect_ratio_set)
                            output_track.track->SetAspectRatio(user_aspect_ratio);
                        else
                            output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        break;
                    case DV100_1080I:
                    case DV100_720P:
                        if (user_aspect_ratio_set)
                            output_track.track->SetAspectRatio(user_aspect_ratio);
                        else
                            output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        output_track.track->SetComponentDepth(input_picture_info->component_depth);
                        break;
                    case D10_30:
                    case D10_40:
                    case D10_50:
                        if (user_aspect_ratio_set) {
                            output_track.track->SetAspectRatio(user_aspect_ratio);
                            if (set_bs_aspect_ratio)
                                output_track.filter = new MPEG2AspectRatioFilter(user_aspect_ratio);
                        } else {
                            output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                            if (set_bs_aspect_ratio)
                                output_track.filter = new MPEG2AspectRatioFilter(input_picture_info->aspect_ratio);
                        }
                        if (afd)
                            output_track.track->SetAFD(afd);
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
                            output_track.track->SetAFD(afd);
                        if (force_no_avci_head) {
                            output_track.track->SetAVCIMode(AVCI_NO_FRAME_HEADER_MODE);
                        } else {
                            if (allow_no_avci_head)
                                output_track.track->SetAVCIMode(AVCI_NO_OR_ALL_FRAME_HEADER_MODE);
                            else
                                output_track.track->SetAVCIMode(AVCI_ALL_FRAME_HEADER_MODE);

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
                                output_track.track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                                output_track.track->SetReplaceAVCIHeader(true);
                            }
                            else if (input_track_reader->HaveAVCIHeader())
                            {
                                output_track.track->SetAVCIHeader(input_track_reader->GetAVCIHeader(), AVCI_HEADER_SIZE);
                            }
                            else if (ps_avcihead && get_ps_avci_header_data(input_track_info->essence_type,
                                                                            input_picture_info->edit_rate,
                                                                            avci_header_data, sizeof(avci_header_data)))
                            {
                                output_track.track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                            }
                            else if (read_avci_header_data(input_track_info->essence_type,
                                                           input_picture_info->edit_rate, avci_header_inputs,
                                                           avci_header_data, sizeof(avci_header_data)))
                            {
                                output_track.track->SetAVCIHeader(avci_header_data, sizeof(avci_header_data));
                            }
                            else if (!allow_no_avci_head)
                            {
                                log_error("Failed to read AVC-Intra header data from input file for %s\n",
                                          essence_type_to_string(input_track_info->essence_type));
                                throw false;
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
                        if (user_aspect_ratio_set)
                            output_track.track->SetAspectRatio(user_aspect_ratio);
                        else
                            output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        output_track.track->SetComponentDepth(input_picture_info->component_depth);
                        output_track.track->SetInputHeight(input_picture_info->stored_height);
                        break;
                    case AVID_ALPHA_SD:
                    case AVID_ALPHA_HD_1080I:
                    case AVID_ALPHA_HD_1080P:
                    case AVID_ALPHA_HD_720P:
                        if (user_aspect_ratio_set)
                            output_track.track->SetAspectRatio(user_aspect_ratio);
                        else
                            output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        output_track.track->SetInputHeight(input_picture_info->stored_height);
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
                        if (afd)
                            output_track.track->SetAFD(afd);
                        break;
                    case MJPEG_2_1:
                    case MJPEG_3_1:
                    case MJPEG_10_1:
                    case MJPEG_20_1:
                    case MJPEG_4_1M:
                    case MJPEG_10_1M:
                    case MJPEG_15_1S:
                        if (afd)
                            output_track.track->SetAFD(afd);
                        if (user_aspect_ratio_set)
                            output_track.track->SetAspectRatio(user_aspect_ratio);
                        else
                            output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
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
                            output_track.track->SetAFD(afd);
                        break;
                    case WAVE_PCM:
                        output_track.track->SetSamplingRate(input_sound_info->sampling_rate);
                        output_track.track->SetQuantizationBits(input_sound_info->bits_per_sample);
                        output_track.track->SetChannelCount(1);
                        if (user_locked_set)
                            output_track.track->SetLocked(user_locked);
                        else if (input_sound_info->locked_set)
                            output_track.track->SetLocked(input_sound_info->locked);
                        if (user_audio_ref_level_set)
                            output_track.track->SetAudioRefLevel(user_audio_ref_level);
                        else if (input_sound_info->audio_ref_level_set)
                            output_track.track->SetAudioRefLevel(input_sound_info->audio_ref_level);
                        if (user_dial_norm_set)
                            output_track.track->SetDialNorm(user_dial_norm);
                        else if (input_sound_info->dial_norm_set)
                            output_track.track->SetDialNorm(input_sound_info->dial_norm);
                        if (clip_type == CW_D10_CLIP_TYPE || input_sound_info->sequence_offset)
                            output_track.track->SetSequenceOffset(input_sound_info->sequence_offset);
                        break;
                    case ANC_DATA:
                        if (anc_const_size) {
                            output_track.track->SetConstantDataSize(anc_const_size);
                        } else if (anc_max_size) {
                            output_track.track->SetMaxDataSize(anc_max_size);
                        } else if (st2020_max_size) {
                            output_track.track->SetMaxDataSize(calc_st2020_max_size(
                                dynamic_cast<const MXFDataTrackInfo*>(output_track.input_track_info)));
                        }
                        break;
                    case VBI_DATA:
                        if (vbi_const_size)
                            output_track.track->SetConstantDataSize(vbi_const_size);
                        else if (vbi_max_size)
                            output_track.track->SetMaxDataSize(vbi_max_size);
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

                output_tracks.push_back(output_track);
                input_to_output_count++;

                track_count[input_track_info->data_def]++;
            }
        }

        // add RDD-6 ANC data track

        if (rdd6_filename) {
            OutputTrack output_track;
            memset(&output_track, 0, sizeof(output_track));
            output_track.essence_type   = ANC_DATA;
            output_track.data_def       = MXF_DATA_DDEF;
            output_track.channel_count  = 1;
            output_track.track          = clip->CreateTrack(ANC_DATA);

            if (anc_const_size)
                output_track.track->SetConstantDataSize(anc_const_size);
            else if (anc_max_size)
                output_track.track->SetMaxDataSize(anc_max_size);
            else if (st2020_max_size)
                output_track.track->SetMaxDataSize(calc_st2020_max_size(false, 1));
            else if (rdd6_const_size)
                output_track.track->SetConstantDataSize(rdd6_const_size);

            output_tracks.push_back(output_track);
        }

        BMX_ASSERT((rdd6_filename  && input_to_output_count + 1 == output_tracks.size()) ||
                   (!rdd6_filename && input_to_output_count     == output_tracks.size()));


        // add AS-11 descriptive metadata

        if (clip_sub_type == AS11_CLIP_SUB_TYPE) {
            as11_helper.InsertFrameworks(clip);

            if ((clip_type == CW_OP1A_CLIP_TYPE && (flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)) ||
                (clip_type == CW_D10_CLIP_TYPE  && (flavour & D10_SINGLE_PASS_WRITE_FLAVOUR)))
            {
                as11_helper.Complete();
            }
        }


        // force file md5 update now that reading/seeking will be forwards only

        if (input_file_md5)
            file_factory.ForceInputChecksumUpdate();


        // set the sample sequence
        // read more than 1 sample to improve efficiency if the input is sound only and the output
        // doesn't require a sample sequence

        BMX_ASSERT(!output_tracks.empty());
        BMX_CHECK(!is_sound_frame_rate || output_tracks[0].essence_type == WAVE_PCM);
        vector<uint32_t> sample_sequence;
        uint32_t sample_sequence_offset = 0;
        uint32_t max_samples_per_read = 1;
        if (is_sound_frame_rate) {
            // read sample sequence required for output frame rate
            sample_sequence = output_tracks[0].track->GetShiftedSampleSequence();
            if (sample_sequence.size() == 1 && sample_sequence[0] == 1)
                max_samples_per_read = 1920; // improve efficiency and read multiple samples
        } else {
            // read 1 input frame
            sample_sequence.push_back(1);
        }
        BMX_ASSERT(max_samples_per_read == 1 || (precharge == 0 && rollout == 0));


        // realtime transwrapping

        uint32_t rt_start = 0;
        if (realtime)
            rt_start = get_tick_count();


        // growing input files

        unsigned int gf_retry_count = 0;
        bool gf_read_failure = false;
        int64_t gf_failure_num_read = 0;
        uint32_t gf_failure_start = 0;


        // create clip file(s) and write samples

        clip->PrepareWrite();

        float next_progress_update;
        init_progress(&next_progress_update);

        int64_t read_duration = reader->GetReadDuration();
        int64_t total_read = 0;
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

            // check whether sufficient frame data is available
            // if the frame is empty then check zero PCM sample padding is possible
            for (i = 0; i < input_to_output_count; i++) {
                OutputTrack &output_track = output_tracks[i];
                Frame *frame = output_track.input_buffer->GetLastFrame(false);
                BMX_ASSERT(frame);
                if (!frame->IsComplete()) {
                    if (!frame->IsEmpty()) {
                        log_warn("Partially complete frames not yet supported\n");
                        break;
                    }
                    if (output_track.input_track_info->essence_type != WAVE_PCM ||
                        output_track.input_track_info->edit_rate !=
                            ((MXFSoundTrackInfo*)output_track.input_track_info)->sampling_rate)
                    {
                        log_warn("Failed to provide padding data for empty frame\n");
                        break;
                    }
                }
                BMX_ASSERT(frame->IsEmpty() || frame->IsComplete());
            }
            if (i < input_to_output_count)
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

            bool take_frame;
            for (i = 0; i < input_to_output_count; i++) {
                OutputTrack &output_track = output_tracks[i];

                if (output_track.channel_index + 1 >= output_track.channel_count)
                    take_frame = true;
                else
                    take_frame = false;

                Frame *frame = output_track.input_buffer->GetLastFrame(take_frame);
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

                uint32_t num_samples;
                if (output_track.rem_precharge > 0)
                {
                    output_track.rem_precharge -= num_read;
                }
                else if (!frame->IsComplete())
                {
                    write_padding_samples(frame, &output_track, &sound_buffer);
                }
                else if (output_track.channel_count > 1 ||
                         output_track.input_track_info->essence_type == D10_AES3_PCM)
                {
                    sound_buffer.Allocate(frame->GetSize()); // more than enough
                    if (output_track.input_track_info->essence_type == D10_AES3_PCM) {
                        convert_aes3_to_pcm(frame->GetBytes(), frame->GetSize(),
                                            output_track.bits_per_sample, output_track.channel_index,
                                            sound_buffer.GetBytes(), sound_buffer.GetAllocatedSize());
                        num_samples = get_aes3_sample_count(frame->GetBytes(), frame->GetSize());
                    } else {
                        deinterleave_audio(frame->GetBytes(), frame->GetSize(),
                                           output_track.bits_per_sample,
                                           output_track.channel_count, output_track.channel_index,
                                           sound_buffer.GetBytes(), sound_buffer.GetAllocatedSize());
                        num_samples = frame->GetSize() / (output_track.channel_count * output_track.block_align);
                    }
                    write_samples(&output_track,
                                  sound_buffer.GetBytes(),
                                  num_samples * output_track.block_align,
                                  num_samples);
                }
                else if (output_track.input_track_info->essence_type == ANC_DATA)
                {
                    write_anc_samples(frame, &output_track, pass_anc, anc_buffer);
                }
                else
                {
                    if (output_track.data_def == MXF_SOUND_DDEF)
                        num_samples = frame->GetSize() / output_track.block_align;
                    else
                        num_samples = frame->num_samples;
                    write_samples(&output_track, (unsigned char*)frame->GetBytes(), frame->GetSize(), num_samples);
                }

                if (take_frame)
                    delete frame;
            }

            if (rdd6_filename) {
                rdd6_frame.UpdateStaticFrame(&rdd6_static_sequence);

                construct_anc_rdd6(&rdd6_frame, &rdd6_first_buffer, &rdd6_second_buffer, rdd6_sdid, rdd6_lines, &anc_buffer);
                output_tracks.back().track->WriteSamples(anc_buffer.GetBytes(), anc_buffer.GetSize(), 1);

                rdd6_static_sequence.UpdateForNextStaticFrame();
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

        if (show_progress)
            print_progress(total_read, read_duration, 0);


        // set precharge and rollout for non-interleaved clip types

        if (clip_type == CW_AS02_CLIP_TYPE && (precharge || rollout)) {
            for (i = 0; i < input_to_output_count; i++) {
                OutputTrack &output_track = output_tracks[i];
                AS02Track *as02_track = output_track.track->GetAS02Track();
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
                ((clip_type == CW_OP1A_CLIP_TYPE && !(flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)) ||
                 (clip_type == CW_D10_CLIP_TYPE  && !(flavour & D10_SINGLE_PASS_WRITE_FLAVOUR))))
        {
            as11_helper.Complete();
        }


        // complete writing

        clip->CompleteWrite();

        log_info("Duration: %"PRId64" (%s)\n",
                 clip->GetDuration(),
                 get_generic_duration_string_2(clip->GetDuration(), clip->GetFrameRate()).c_str());


        if (read_duration >= 0 && total_read != read_duration) {
            bmx::log(reader->IsComplete() ? ERROR_LOG : WARN_LOG,
                     "Read less (%"PRId64") samples than expected (%"PRId64")\n", total_read, read_duration);
            if (reader->IsComplete())
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
            clear_output_track(&output_tracks[i]);
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

