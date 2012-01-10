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

#define __STDC_FORMAT_MACROS 1

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <string>
#include <vector>

#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/MXFGroupReader.h>
#include <bmx/mxf_reader/MXFSequenceReader.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/as02/AS02PCMTrack.h>
#include <bmx/avid_mxf/AvidPCMTrack.h>
#include <bmx/mxf_op1a/OP1APCMTrack.h>
#include <bmx/URI.h>
#include <bmx/MXFUtils.h>
#include "../AppUtils.h"
#include "../AS11Helper.h"
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    FrameBuffer *input_buffer;
    ClipWriterTrack *track;
    bool is_picture;
    uint32_t channel_count;
    uint32_t channel_index;
    bool is_d10_aes3;
    uint32_t bits_per_sample;
    uint16_t block_align;
} OutputTrack;

typedef struct
{
    const char *position_str;
    AvidLocator locator;
} LocatorOption;


static const char DEFAULT_SHIM_NAME[]       = "Sample File";
static const char DEFAULT_SHIM_ID[]         = "http://bbc.co.uk/rd/as02/default-shim.txt";
static const char DEFAULT_SHIM_ANNOTATION[] = "Default AS-02 shim";


extern bool BMX_REGRESSION_TEST;



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
    sprintf(buffer, "bmxtranswrap, %s v%s (%s %s)", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
    return buffer;
}

static void usage(const char *cmd)
{
    fprintf(stderr, "%s\n", get_version_info().c_str());
    fprintf(stderr, "Usage: %s <<options>> [<mxf input>]+\n", cmd);
    fprintf(stderr, "Options (* means option is required):\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -l <file>               Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, "  -t <type>               Clip type: as02, as11op1a, as11d10, op1a, avid, d10. Default is as02\n");
    fprintf(stderr, "* -o <name>               as02: <name> is a bundle name\n");
    fprintf(stderr, "                          as11op1a/as11d10/op1a/d10: <name> is a filename\n");
    fprintf(stderr, "                          avid: <name> is a filename prefix\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Override input start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --tc-rate <rate>        Start timecode rate to use when input is audio only\n");
    fprintf(stderr, "                          Values are 25 (default), 30 (30000/1001), 50 or 60 (60000/1001)\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --start <frame>         Set the start frame. Default is 0\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames. Default is minimum input duration\n");
    fprintf(stderr, "  --group                 Use the group reader instead of the sequence reader\n");
    fprintf(stderr, "                          Use this option if the files have different material packages\n");
    fprintf(stderr, "                          but actually belong to the same virtual package / group\n");
    fprintf(stderr, "  --no-reorder            Don't attempt to order the inputs in a sequence\n");
    fprintf(stderr, "                          Use this option for files with broken timecode\n");
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
    fprintf(stderr, "  as02/as11op1a/as11d10/op1a/d10:\n");
    fprintf(stderr, "    --afd <value>           Active Format Descriptor code. Default is input file's value or not set\n");
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
}

int main(int argc, const char** argv)
{
    Rational timecode_rate = FRAME_RATE_25;
    bool timecode_rate_set = false;
    vector<const char *> input_filenames;
    const char *log_filename = 0;
    ClipWriterType clip_type = CW_AS02_CLIP_TYPE;
    const char *output_name = "";
    Timecode start_timecode;
    const char *start_timecode_str = 0;
    int64_t start = 0;
    int64_t duration = -1;
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
    bool do_print_version = false;
    bool use_group_reader = false;
    bool keep_input_order = false;
    uint8_t user_afd = 0;
    int value;
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
        else if (strcmp(argv[cmdln_index], "--tc-rate") == 0)
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
            if (value == 25)
                timecode_rate = FRAME_RATE_25;
            else if (value == 30)
                timecode_rate = FRAME_RATE_2997;
            else if (value == 50)
                timecode_rate = FRAME_RATE_50;
            else
                timecode_rate = FRAME_RATE_5994;
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
        else if (strcmp(argv[cmdln_index], "--group") == 0)
        {
            use_group_reader = true;
        }
        else if (strcmp(argv[cmdln_index], "--no-reorder") == 0)
        {
            keep_input_order = true;
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
            BMX_REGRESSION_TEST = true;
        }
        else
        {
            break;
        }
    }

    for (; cmdln_index < argc; cmdln_index++)
        input_filenames.push_back(argv[cmdln_index]);

    if (input_filenames.empty()) {
        usage(argv[0]);
        fprintf(stderr, "No input filenames\n");
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
        // open an MXFReader using the input filenames

        MXFReader *reader;
        if (use_group_reader) {
            MXFGroupReader *group_reader = new MXFGroupReader();
            size_t i;
            for (i = 0; i < input_filenames.size(); i++) {
                MXFFileReader *file_reader = new MXFFileReader();
                MXFFileReader::OpenResult result = file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", input_filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    delete file_reader;
                    throw false;
                }

                group_reader->AddReader(file_reader);
            }
            if (!group_reader->Finalize())
                throw false;

            reader = group_reader;
        } else if (input_filenames.size() > 1) {
            MXFSequenceReader *seq_reader = new MXFSequenceReader();
            size_t i;
            for (i = 0; i < input_filenames.size(); i++) {
                MXFFileReader *file_reader = new MXFFileReader();
                MXFFileReader::OpenResult result = file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", input_filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    delete file_reader;
                    throw false;
                }

                seq_reader->AddReader(file_reader);
            }
            if (!seq_reader->Finalize(false, keep_input_order))
                throw false;

            reader = seq_reader;
        } else {
            MXFFileReader *file_reader = new MXFFileReader();
            MXFFileReader::OpenResult result = file_reader->Open(input_filenames[0]);
            if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                log_error("Failed to open MXF file '%s': %s\n", input_filenames[0],
                          MXFFileReader::ResultToString(result).c_str());
                delete file_reader;
                throw false;
            }

            reader = file_reader;
        }


        Rational frame_rate = reader->GetSampleRate();


        // complete commandline parsing

        if (!timecode_rate_set) {
            size_t i;
            for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                if (reader->GetTrackReader(i)->GetTrackInfo()->is_picture) {
                    timecode_rate = frame_rate;
                    break;
                }
            }
        }
        if (start_timecode_str && !parse_timecode(start_timecode_str, timecode_rate, &start_timecode)) {
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

        size_t i;
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


        // check support for tracks and disable unsupported track types

        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *track_reader = reader->GetTrackReader(i);
            const MXFTrackInfo *input_track_info = track_reader->GetTrackInfo();

            const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);
            const MXFPictureTrackInfo *input_picture_info = dynamic_cast<const MXFPictureTrackInfo*>(input_track_info);

            bool is_supported = true;
            if (input_track_info->essence_type == UNKNOWN_ESSENCE_TYPE ||
                input_track_info->essence_type == PICTURE_ESSENCE ||
                input_track_info->essence_type == SOUND_ESSENCE)
            {
                log_warn("Track %zu has unknown essence type\n", i);
                is_supported = false;
            }
            else if (input_track_info->essence_type == WAVE_PCM)
            {
                Rational sampling_rate = input_sound_info->sampling_rate;
                if (!ClipWriterTrack::IsSupported(clip_type, WAVE_PCM, sampling_rate)) {
                    log_warn("Track %zu essence type '%s' @%d/%d sps not supported by clip type '%s'\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             sampling_rate.numerator, sampling_rate.denominator,
                             ClipWriter::ClipWriterTypeToString(clip_type).c_str());
                    is_supported = false;
                } else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32) {
                    log_warn("Track %zu (%s) bits per sample %u not supported\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             input_sound_info->bits_per_sample);
                    is_supported = false;
                }
            }
            else if (input_track_info->IsD10Audio())
            {
                if (input_sound_info->sampling_rate.numerator != 48000 ||
                    input_sound_info->sampling_rate.denominator != 1)
                {
                    log_warn("Track %zu essence type D-10 AES-3 audio sampling rate %d/%d not supported\n",
                             i,
                             input_sound_info->sampling_rate.numerator, input_sound_info->sampling_rate.denominator);
                    is_supported = false;
                }
                else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32)
                {
                    log_warn("Track %zu essence type D-10 AES-3 audio bits per sample %u not supported\n",
                             i,
                             input_sound_info->bits_per_sample);
                    is_supported = false;
                }
            }
            else
            {
                if (input_track_info->edit_rate != frame_rate) {
                    log_warn("Track %zu (essence type '%s') edit rate %d/%d does not equals clip edit rate %d/%d\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             input_track_info->edit_rate.numerator, input_track_info->edit_rate.denominator,
                             frame_rate.numerator, frame_rate.denominator,
                             ClipWriter::ClipWriterTypeToString(clip_type).c_str());
                    is_supported = false;
                } else if (!ClipWriterTrack::IsSupported(clip_type, input_track_info->essence_type, frame_rate)) {
                    log_warn("Track %zu essence type '%s' @%d/%d fps not supported by clip type '%s'\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             frame_rate.numerator, frame_rate.denominator,
                             ClipWriter::ClipWriterTypeToString(clip_type).c_str());
                    is_supported = false;
                }

                if ((input_track_info->essence_type == D10_30 ||
                        input_track_info->essence_type == D10_40 ||
                        input_track_info->essence_type == D10_50) &&
                    input_picture_info->d10_frame_size == 0)
                {
                    log_warn("Track %zu (essence type '%s') has zero frame size\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type));
                    is_supported = false;
                }
                else if ((input_track_info->essence_type == AVCI100_1080I ||
                               input_track_info->essence_type == AVCI100_1080P ||
                               input_track_info->essence_type == AVCI100_720P ||
                               input_track_info->essence_type == AVCI50_1080I ||
                               input_track_info->essence_type == AVCI50_1080P ||
                               input_track_info->essence_type == AVCI50_720P) &&
                         !track_reader->HaveAVCIHeader())
                {
                    log_warn("Track %zu (essence type '%s') does not have sequence and picture parameter sets\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type));
                    is_supported = false;
                }
            }

            track_reader->SetEnable(is_supported);
        }

        if (!reader->IsEnabled()) {
            log_error("All tracks are disabled\n");
            throw false;
        }


        // choose number of samples to read in one go

        uint32_t max_samples_per_read = 1;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            const MXFSoundTrackInfo *input_sound_info =
                dynamic_cast<const MXFSoundTrackInfo*>(reader->GetTrackReader(i)->GetTrackInfo());

            if (input_sound_info && frame_rate == input_sound_info->sampling_rate) {
                max_samples_per_read = 1920;
                break;
            }
        }


        // set read limits

        int64_t read_start = start;
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

        int16_t precharge = reader->GetMaxPrecharge(read_start, true);
        int16_t rollout = reader->GetMaxRollout(read_start + output_duration - 1, true);

        reader->SetReadLimits(read_start + precharge, read_start + output_duration + rollout, true);


        // create output clip and initialize

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
                BMX_ASSERT(false);
                break;
        }

        SourcePackage *tape_package = 0;
        vector<pair<mxfUMID, uint32_t> > tape_package_picture_refs;
        vector<pair<mxfUMID, uint32_t> > tape_package_sound_refs;

        if (!start_timecode_str) {
            if (reader->HavePlayoutTimecode()) {
                start_timecode = reader->GetPlayoutTimecode(start);

                // adjust start timecode to be at the point after the leading filler segments
                // this corresponds to the zero position in the MXF reader
                if (!reader->HaveFixedLeadFillerOffset())
                    log_warn("No fixed lead filler offset\n");
                else
                    start_timecode.AddOffset(reader->GetFixedLeadFillerOffset(), frame_rate);

                // the Avid or AS11 D-10 clip types don't support precharge and so the timecode
                // needs to be adjusted back to the precharge starting point
                if (clip_type == CW_AVID_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE)
                    start_timecode.AddOffset(precharge, frame_rate);
            }
        }

        if (!start_timecode.IsInvalid())
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
                as11_clip->SetOutputStartOffset(- precharge);
                as11_clip->SetOutputEndOffset(- rollout);
            }

            if (!clip_name && as11_helper.HaveProgrammeTitle())
                as11_clip->SetClipName(as11_helper.GetProgrammeTitle());
        } else if (clip_type == CW_OP1A_CLIP_TYPE) {
            OP1AFile *op1a_clip = clip->GetOP1AClip();

            op1a_clip->SetPartitionInterval(partition_interval);
            op1a_clip->SetOutputStartOffset(- precharge);
            op1a_clip->SetOutputEndOffset(- rollout);
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
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    MXFTrackReader *input_track_reader = reader->GetTrackReader(i);
                    if (!input_track_reader->IsEnabled())
                        continue;

                    if (input_track_reader->GetTrackInfo()->is_picture)
                        num_picture_tracks++;
                    else
                        num_sound_tracks++;
                }
                tape_package = avid_clip->CreateDefaultTapeSource(tape_name, num_picture_tracks, num_sound_tracks);

                tape_package_picture_refs = avid_clip->GetPictureSourceReferences(tape_package);
                BMX_ASSERT(tape_package_picture_refs.size() == num_picture_tracks);
                tape_package_sound_refs = avid_clip->GetSoundSourceReferences(tape_package);
                BMX_ASSERT(tape_package_sound_refs.size() == num_sound_tracks);
            }
        }


        // create output tracks and initialize

        vector<OutputTrack> output_tracks;

        uint32_t picture_track_count = 0;
        uint32_t sound_track_count = 0;
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
                output_track.input_buffer  = input_track_reader->GetFrameBuffer();
                output_track.is_picture    = input_track_info->is_picture;
                output_track.channel_count = output_track_count;
                output_track.channel_index = c;
                output_track.is_d10_aes3   = input_track_info->IsD10Audio();
                if (input_sound_info) {
                    output_track.bits_per_sample = input_sound_info->bits_per_sample;
                    output_track.block_align     = (input_sound_info->bits_per_sample + 7) / 8;
                } else {
                    output_track.bits_per_sample = 0;
                    output_track.block_align     = 0;
                }

                if (clip_type == CW_AVID_CLIP_TYPE) {
                    string track_name = create_mxf_track_filename(
                                            output_name,
                                            input_track_info->is_picture ? picture_track_count + 1 :
                                                                           sound_track_count + 1,
                                            input_track_info->is_picture);
                    output_track.track = clip->CreateTrack(input_track_info->essence_type, track_name.c_str());
                } else {
                    output_track.track = clip->CreateTrack(input_track_info->essence_type);
                }


                // initialize track

                // TODO: track number setting and check AES-3 channel validity

                if (clip_type == CW_AS02_CLIP_TYPE) {
                    AS02Track *as02_track = output_track.track->GetAS02Track();
                    as02_track->SetMICType(mic_type);
                    as02_track->SetMICScope(ess_component_mic_scope);

                    // TODO: need precharge/rollout at track rates
                    // TODO: check whether precharge/rollout is available on all tracks etc.
                    if (precharge != 0)
                        log_warn("TODO: precharge in AS-02 track\n");
                    //as02_track->SetOutputStartOffset(- precharge);
                    if (rollout != 0)
                        log_warn("TODO: rollout in AS-02 track\n");
                    //as02_track->SetOutputEndOffset(- rollout);

                    AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(as02_track);
                    if (as02_pict_track)
                        as02_pict_track->SetPartitionInterval(partition_interval);
                } else if (clip_type == CW_AVID_CLIP_TYPE) {
                    AvidTrack *avid_track = output_track.track->GetAvidTrack();

                    // TODO: need precharge/rollout at track rates
                    // TODO: check whether precharge/rollout is available on all tracks etc.
                    if (rollout != 0)
                        log_warn("TODO: rollout in Avid track\n");
                    //avid_track->SetOutputEndOffset(- rollout);

                    if (tape_package) {
                        if (input_track_info->is_picture) {
                            avid_track->SetSourceRef(tape_package_picture_refs[picture_track_count].first,
                                                     tape_package_picture_refs[picture_track_count].second);
                        } else {
                            avid_track->SetSourceRef(tape_package_sound_refs[sound_track_count].first,
                                                     tape_package_sound_refs[sound_track_count].second);
                        }
                    }
                    // TODO: Import source package
                }

                switch (input_track_info->essence_type)
                {
                    case IEC_DV25:
                    case DVBASED_DV25:
                    case DV50:
                        output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        break;
                    case DV100_1080I:
                    case DV100_720P:
                        output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        output_track.track->SetComponentDepth(input_picture_info->component_depth);
                        break;
                    case D10_30:
                    case D10_40:
                    case D10_50:
                        output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        BMX_ASSERT(input_picture_info->d10_frame_size != 0);
                        output_track.track->SetSampleSize(input_picture_info->d10_frame_size);
                        break;
                    case AVCI100_1080I:
                    case AVCI100_1080P:
                    case AVCI100_720P:
                    case AVCI50_1080I:
                    case AVCI50_1080P:
                    case AVCI50_720P:
                        if (afd)
                            output_track.track->SetAFD(afd);
                        output_track.track->SetAVCIMode(AVCI_ALL_FRAME_HEADER_MODE);
                        BMX_ASSERT(input_track_reader->HaveAVCIHeader());
                        output_track.track->SetAVCIHeader(input_track_reader->GetAVCIHeader(), AVCI_HEADER_SIZE);
                        break;
                    case UNC_SD:
                    case UNC_HD_1080I:
                    case UNC_HD_1080P:
                    case UNC_HD_720P:
                    case AVID_10BIT_UNC_SD:
                    case AVID_10BIT_UNC_HD_1080I:
                    case AVID_10BIT_UNC_HD_1080P:
                    case AVID_10BIT_UNC_HD_720P:
                        output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        output_track.track->SetComponentDepth(input_picture_info->component_depth);
                        output_track.track->SetInputHeight(input_picture_info->stored_height);
                        break;
                    case MPEG2LG_422P_HL_1080I:
                    case MPEG2LG_422P_HL_1080P:
                    case MPEG2LG_422P_HL_720P:
                    case MPEG2LG_MP_HL_1080I:
                    case MPEG2LG_MP_HL_1080P:
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
                        output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
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
                        if (afd)
                            output_track.track->SetAFD(afd);
                        break;
                    case WAVE_PCM:
                        output_track.track->SetSamplingRate(input_sound_info->sampling_rate);
                        output_track.track->SetQuantizationBits(input_sound_info->bits_per_sample);
                        output_track.track->SetChannelCount(1);
                        if (input_sound_info->locked_set)
                            output_track.track->SetLocked(input_sound_info->locked);
                        if (input_sound_info->audio_ref_level_set)
                            output_track.track->SetAudioRefLevel(input_sound_info->audio_ref_level);
                        if (input_sound_info->dial_norm_set)
                            output_track.track->SetDialNorm(input_sound_info->dial_norm);
                        if (clip_type == CW_D10_CLIP_TYPE || input_sound_info->sequence_offset)
                            output_track.track->SetSequenceOffset(input_sound_info->sequence_offset);
                        break;
                    case PICTURE_ESSENCE:
                    case SOUND_ESSENCE:
                    case UNKNOWN_ESSENCE_TYPE:
                        BMX_ASSERT(false);
                }

                output_tracks.push_back(output_track);


                if (input_track_info->is_picture)
                    picture_track_count++;
                else
                    sound_track_count++;
            }
        }



        // add AS-11 descriptive metadata

        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE) {
            AS11Clip *as11_clip = clip->GetAS11Clip();
            as11_clip->PrepareHeaderMetadata();
            as11_helper.InsertFrameworks(as11_clip);
        }


        // create clip file(s) and write samples

        clip->PrepareWrite();

        bmx::ByteArray sound_buffer;
        while (output_duration < 0 || clip->GetDuration() < output_duration) {
            uint32_t num_read = reader->Read(max_samples_per_read);
            if (num_read == 0)
                break;

            bool take_frame;
            for (i = 0; i < output_tracks.size(); i++) {
                OutputTrack &output_track = output_tracks[i];

                if (output_track.channel_index + 1 >= output_track.channel_count)
                    take_frame = true;
                else
                    take_frame = false;

                Frame *frame = output_track.input_buffer->GetLastFrame(take_frame);
                if (!frame || frame->IsEmpty()) {
                    if (frame && take_frame)
                        delete frame;
                    continue;
                }

                uint32_t num_samples;
                if (output_track.channel_count > 1 || output_track.is_d10_aes3) {
                    sound_buffer.Allocate(frame->GetSize()); // more than enough
                    if (output_track.is_d10_aes3) {
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
                    output_track.track->WriteSamples(sound_buffer.GetBytes(),
                                                     num_samples * output_track.block_align,
                                                     num_samples);
                } else {
                    if (output_track.is_picture)
                        num_samples = frame->num_samples;
                    else
                        num_samples = frame->GetSize() / output_track.block_align;
                    output_track.track->WriteSamples(frame->GetBytes(), frame->GetSize(), num_samples);
                }

                if (take_frame)
                    delete frame;
            }
        }


        // complete AS-11 descriptive metadata

        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_AS11_D10_CLIP_TYPE)
            as11_helper.Complete();


        // complete writing

        clip->CompleteWrite();

        log_info("Duration: %"PRId64" (%s)\n",
                 clip->GetDuration(),
                 get_generic_duration_string_2(clip->GetDuration(), clip->GetFrameRate()).c_str());


        if (clip->GetDuration() != output_duration) {
            log_error("Clip duration does not equal expected duration, %"PRId64" (%s)\n",
                      output_duration,
                      get_generic_duration_string_2(output_duration, clip->GetFrameRate()).c_str());
            throw false;
        }


        delete reader;
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

