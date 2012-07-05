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

#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/MXFGroupReader.h>
#include <bmx/mxf_reader/MXFSequenceReader.h>
#include <bmx/mxf_reader/MXFFrameMetadata.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/clip_writer/ClipWriter.h>
#include <bmx/as02/AS02PictureTrack.h>
#include <bmx/as02/AS02PCMTrack.h>
#include <bmx/avid_mxf/AvidPCMTrack.h>
#include <bmx/mxf_op1a/OP1APCMTrack.h>
#include <bmx/wave/WaveFileIO.h>
#include <bmx/URI.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/Version.h>
#include "../AppUtils.h"
#include "../AS11Helper.h"
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid.h>
#if defined(_WIN32)
#include <mxf/mxf_win32_file.h>
#endif
#include <mxf/mxf_rw_intl_file.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



typedef struct
{
    FrameBuffer *input_buffer;
    ClipWriterTrack *track;
    EssenceType essence_type;
    EssenceType input_essence_type;
    bool is_picture;
    uint32_t channel_count;
    uint32_t channel_index;
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

static const char DEFAULT_BEXT_ORIGINATOR[] = "bmx";

static const uint32_t DEFAULT_RW_INTL_SIZE  = (64 * 1024);


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};


class TranswrapFileFactory : public MXFFileFactory
{
public:
    TranswrapFileFactory(bool md5_wrap_input, bool rw_interleave, uint32_t rw_interleave_size, int input_flags)
    {
        mMD5WrapInput = md5_wrap_input;
        mInterleaver = 0;
        mInputFlags = input_flags;

        if (rw_interleave) {
            uint32_t cacheSize = 2 * 1024 * 1024;
            if (cacheSize < rw_interleave_size) {
                BMX_CHECK(rw_interleave_size < UINT32_MAX / 2);
                cacheSize = 2 * rw_interleave_size;
            }
            BMX_CHECK(mxf_create_rw_intl(rw_interleave_size, cacheSize, &mInterleaver));
        }
    }
    ~TranswrapFileFactory()
    {
        mxf_free_rw_intl(&mInterleaver);
    }

    bool OpenInput(string filename, File **file)
    {
        MXFFile *mxf_file = 0;

        try
        {
#if defined(_WIN32)
            BMX_CHECK(mxf_win32_file_open_read(filename.c_str(), mInputFlags, &mxf_file));
#else
            BMX_CHECK(mxf_disk_file_open_read(filename.c_str(), &mxf_file));
#endif

            if (mMD5WrapInput) {
                MXFMD5WrapperFile *md5_wrap_file = md5_wrap_mxf_file(mxf_file);
                mInputMD5WrapFiles.push_back(md5_wrap_file);
                mxf_file = md5_wrap_get_file(mInputMD5WrapFiles.back());
            }

            if (mInterleaver) {
                MXFFile *intl_mxf_file;
                BMX_CHECK(mxf_rw_intl_open(mInterleaver, mxf_file, 0, &intl_mxf_file));
                mxf_file = intl_mxf_file;
            }

            *file = new File(mxf_file);
            return true;
        }
        catch (...)
        {
            mxf_file_close(&mxf_file);
            return false;
        }
    }

public:
    MXFRWInterleaver* GetInterleaver() const { return mInterleaver; }

    vector<MXFMD5WrapperFile*>& GetInputMD5WrapFiles() { return mInputMD5WrapFiles; }

public:
    virtual File* OpenNew(string filename)
    {
        MXFFile *mxf_file;

#if defined(_WIN32)
        BMX_CHECK(mxf_win32_file_open_new(filename.c_str(), 0, &mxf_file));
#else
        BMX_CHECK(mxf_disk_file_open_new(filename.c_str(), &mxf_file));
#endif

        if (mInterleaver) {
            MXFFile *intl_mxf_file;
            BMX_CHECK(mxf_rw_intl_open(mInterleaver, mxf_file, 1, &intl_mxf_file));
            mxf_file = intl_mxf_file;
        }

        return new File(mxf_file);
    }

    virtual File* OpenRead(string filename)
    {
        MXFFile *mxf_file;

#if defined(_WIN32)
        BMX_CHECK(mxf_win32_file_open_read(filename.c_str(), 0, &mxf_file));
#else
        BMX_CHECK(mxf_disk_file_open_read(filename.c_str(), &mxf_file));
#endif

        if (mInterleaver) {
            MXFFile *intl_mxf_file;
            BMX_CHECK(mxf_rw_intl_open(mInterleaver, mxf_file, 0, &intl_mxf_file));
            mxf_file = intl_mxf_file;
        }

        return new File(mxf_file);
    }

    virtual File* OpenModify(string filename)
    {
        MXFFile *mxf_file;

#if defined(_WIN32)
        BMX_CHECK(mxf_win32_file_open_modify(filename.c_str(), 0, &mxf_file));
#else
        BMX_CHECK(mxf_disk_file_open_modify(filename.c_str(), &mxf_file));
#endif

        if (mInterleaver) {
            MXFFile *intl_mxf_file;
            BMX_CHECK(mxf_rw_intl_open(mInterleaver, mxf_file, 1, &intl_mxf_file));
            mxf_file = intl_mxf_file;
        }

        return new File(mxf_file);
    }

private:
    bool mMD5WrapInput;
    vector<MXFMD5WrapperFile*> mInputMD5WrapFiles;
    MXFRWInterleaver *mInterleaver;
    int mInputFlags;
};



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
    sprintf(buffer, "bmxtranswrap, %s v%s, %s %s (scm %s)",
            get_bmx_library_name().c_str(),
            get_bmx_version_string().c_str(),
            __DATE__, __TIME__,
            get_bmx_scm_version_string().c_str());
    return buffer;
}

static void usage(const char *cmd)
{
    fprintf(stderr, "%s\n", get_version_info().c_str());
    fprintf(stderr, "Usage: %s <<options>> [<mxf input>]+\n", cmd);
    fprintf(stderr, "Options (* means option is required):\n");
    fprintf(stderr, "  -h | --help             Show usage and exit\n");
    fprintf(stderr, "  -v | --version          Print version info\n");
    fprintf(stderr, "  -p                      Print progress percentage to stdout\n");
    fprintf(stderr, "  -l <file>               Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, "  -t <type>               Clip type: as02, as11op1a, as11d10, op1a, avid, d10, wave. Default is as02\n");
    fprintf(stderr, "* -o <name>               as02: <name> is a bundle name\n");
    fprintf(stderr, "                          as11op1a/as11d10/op1a/d10/wave: <name> is a filename\n");
    fprintf(stderr, "                          avid: <name> is a filename prefix\n");
    fprintf(stderr, "  --input-file-md5        Calculate an MD5 checksum of the input file\n");
    fprintf(stderr, "  -y <hh:mm:sscff>        Override input start timecode. Is drop frame when c is not ':'. Default 00:00:00:00\n");
    fprintf(stderr, "  --tc-rate <rate>        Start timecode rate to use when input is audio only\n");
    fprintf(stderr, "                          Values are 23976 (24000/1001), 24, 25 (default), 2997 (30000/1001), 50 or 5994 (60000/1001)\n");
    fprintf(stderr, "  --clip <name>           Set the clip name\n");
    fprintf(stderr, "  --start <frame>         Set the start frame in input edit rate units. Default is 0\n");
    fprintf(stderr, "  --dur <frame>           Set the duration in frames in input edit rate units. Default is minimum input duration\n");
    fprintf(stderr, "  --group                 Use the group reader instead of the sequence reader\n");
    fprintf(stderr, "                          Use this option if the files have different material packages\n");
    fprintf(stderr, "                          but actually belong to the same virtual package / group\n");
    fprintf(stderr, "  --no-reorder            Don't attempt to order the inputs in a sequence\n");
    fprintf(stderr, "                          Use this option for files with broken timecode\n");
    fprintf(stderr, "  --no-precharge          Don't output clip/track with precharge. Adjust the start position and duration instead\n");
    fprintf(stderr, "  --no-rollout            Don't output clip/track with rollout. Adjust the duration instead\n");
    fprintf(stderr, "  --no-d10-depad          Don't reduce the size of the D-10 frame to the maximum size\n");
    fprintf(stderr, "                          Use this option if there are non-zero bytes beyond the maximum size\n");
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
    fprintf(stderr, "                              and incrementing 512 bytes for each format in the list\n");
    fprintf(stderr, "  -a <n:d>                Override or set the image aspect ratio. Either 4:3 or 16:9.\n");
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
    fprintf(stderr, "  as02/as11op1a/op1a:\n");
    fprintf(stderr, "    --part <interval>       Video essence partition interval in frames in input edit rate units, or seconds with 's' suffix. Default single partition\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10:\n");
    fprintf(stderr, "    --dm <fwork> <name> <value>    Set descriptive framework property. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --dm-file <fwork> <name>       Parse and set descriptive framework properties from text file <name>. <fwork> is 'as11' or 'dpp'\n");
    fprintf(stderr, "    --seg <name>                   Parse and set segmentation data from text file <name>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as02/as11op1a/as11d10/op1a/d10:\n");
    fprintf(stderr, "    --afd <value>           Active Format Descriptor 4-bit code from table 1 in SMPTE ST 2016-1. Default is input file's value or not set\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  as11op1a/as11d10/op1a/d10:\n");
    fprintf(stderr, "    --single-pass           Write file in a single pass\n");
    fprintf(stderr, "                            Header and body partitions will be incomplete for as11op1a/op1a if the number if essence container bytes per edit unit is variable\n");
    fprintf(stderr, "    --file-md5              Calculate an MD5 checksum of the output file. This requires writing in a single pass (--single-pass is assumed)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  op1a:\n");
    fprintf(stderr, "    --min-part              Only use a header and footer MXF file partition. Use this for applications that don't support\n");
    fprintf(stderr, "                                separate partitions for header metadata, index tables, essence container data and footer\n");
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
    fprintf(stderr, "    --tp-uid <umid>         Set the tape Source Package UID. Autogenerated by default\n");
    fprintf(stderr, "    --tp-created <tstamp>   Set the tape Source Package creation date. Default is 'now'\n");
    fprintf(stderr, "    --ess-marks             Convert XDCAM Essence Marks to locators\n");
    fprintf(stderr, "    --allow-no-avci-head    Allow inputs with no AVCI header (512 bytes, sequence and picture parameter sets)\n");
    fprintf(stderr, "    --force-no-avci-head    Strip AVCI header (512 bytes, sequence and picture parameter sets) if present\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  wave:\n");
    fprintf(stderr, "    --orig <name>           Set originator in the Wave bext chunk. Default '%s'\n", DEFAULT_BEXT_ORIGINATOR);
    fprintf(stderr, "\n\n");
    fprintf(stderr, "Notes:\n");
    fprintf(stderr, " - <umid> format is [0-9a-fA-F]{64}, a sequence of 32 hexadecimal bytes\n");
    fprintf(stderr, " - <tstamp> format is YYYY-MM-DDThh:mm:ss:qm where qm is in units of 1/250th second\n");
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
    const char *import_name = 0;
    map<string, string> user_comments;
    vector<LocatorOption> locators;
    AS11Helper as11_helper;
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
    UMID tp_uid;
    bool tp_uid_set = false;
    Timestamp tp_created;
    bool tp_created_set = false;
    bool no_d10_depad = false;
    bool convert_ess_marks = false;
    bool allow_no_avci_head = false;
    bool force_no_avci_head = false;
    bool min_part = false;
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
        else if (strcmp(argv[cmdln_index], "--no-precharge") == 0)
        {
            no_precharge = true;
        }
        else if (strcmp(argv[cmdln_index], "--no-rollout") == 0)
        {
            no_rollout = true;
        }
        else if (strcmp(argv[cmdln_index], "--no-d10-depad") == 0)
        {
            no_d10_depad = true;
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
        else if (strcmp(argv[cmdln_index], "--min-part") == 0)
        {
            min_part = true;
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
        else if (strcmp(argv[cmdln_index], "--tp-uid") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_umid(argv[cmdln_index + 1], &tp_uid))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            tp_uid_set = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--tp-created") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_timestamp(argv[cmdln_index + 1], &tp_created))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            tp_created_set = true;
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

    if (clip_type != CW_AVID_CLIP_TYPE) {
        allow_no_avci_head = false;
        force_no_avci_head = false;
    }


    for (; cmdln_index < argc; cmdln_index++) {
        if (!check_file_exists(argv[cmdln_index])) {
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

#define OPEN_FILE(sreader, index)                                                   \
    MXFFileReader::OpenResult result;                                               \
    File *file = 0;                                                                 \
    if (file_factory.OpenInput(input_filenames[index], &file))                      \
        result = sreader->Open(file, input_filenames[index]);                       \
    else                                                                            \
        result = MXFFileReader::MXF_RESULT_OPEN_FAIL;                               \
    if (result != MXFFileReader::MXF_RESULT_SUCCESS) {                              \
        log_error("Failed to open MXF file '%s': %s\n", input_filenames[index],     \
                  MXFFileReader::ResultToString(result).c_str());                   \
        delete sreader;                                                             \
        throw false;                                                                \
    }

        TranswrapFileFactory file_factory(input_file_md5, rw_interleave, rw_interleave_size, input_file_flags);
        MXFReader *reader;
        if (use_group_reader) {
            MXFGroupReader *group_reader = new MXFGroupReader();
            size_t i;
            for (i = 0; i < input_filenames.size(); i++) {
                MXFFileReader *file_reader = new MXFFileReader();
                OPEN_FILE(file_reader, i)
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
                OPEN_FILE(file_reader, i)
                seq_reader->AddReader(file_reader);
            }
            if (!seq_reader->Finalize(false, keep_input_order))
                throw false;

            reader = seq_reader;
        } else {
            MXFFileReader *file_reader = new MXFFileReader();
            OPEN_FILE(file_reader, 0)

            reader = file_reader;
        }

        Rational frame_rate = reader->GetSampleRate();


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

        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            MXFTrackReader *track_reader = reader->GetTrackReader(i);
            const MXFTrackInfo *input_track_info = track_reader->GetTrackInfo();

            const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);
            const MXFPictureTrackInfo *input_picture_info = dynamic_cast<const MXFPictureTrackInfo*>(input_track_info);

            bool is_supported = true;
            if (input_track_info->essence_type == WAVE_PCM)
            {
                Rational sampling_rate = input_sound_info->sampling_rate;
                if (!ClipWriterTrack::IsSupported(clip_type, WAVE_PCM, sampling_rate)) {
                    log_warn("Track %"PRIszt" essence type '%s' @%d/%d sps not supported by clip type '%s'\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             sampling_rate.numerator, sampling_rate.denominator,
                             ClipWriter::ClipWriterTypeToString(clip_type).c_str());
                    is_supported = false;
                } else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32) {
                    log_warn("Track %"PRIszt" (%s) bits per sample %u not supported\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             input_sound_info->bits_per_sample);
                    is_supported = false;
                } else if (input_sound_info->channel_count == 0) {
                    log_warn("Track %"PRIszt" (%s) has zero channel count\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type));
                    is_supported = false;
                }
            }
            else if (input_track_info->essence_type == D10_AES3_PCM)
            {
                if (input_sound_info->sampling_rate != SAMPLING_RATE_48K)
                {
                    log_warn("Track %"PRIszt" essence type D-10 AES-3 audio sampling rate %d/%d not supported\n",
                             i,
                             input_sound_info->sampling_rate.numerator, input_sound_info->sampling_rate.denominator);
                    is_supported = false;
                }
                else if (input_sound_info->bits_per_sample == 0 || input_sound_info->bits_per_sample > 32)
                {
                    log_warn("Track %"PRIszt" essence type D-10 AES-3 audio bits per sample %u not supported\n",
                             i,
                             input_sound_info->bits_per_sample);
                    is_supported = false;
                }
            }
            else if (input_track_info->essence_type == UNKNOWN_ESSENCE_TYPE ||
                     input_track_info->essence_type == PICTURE_ESSENCE ||
                     input_track_info->essence_type == SOUND_ESSENCE)
            {
                log_warn("Track %"PRIszt" has unknown essence type\n", i);
                is_supported = false;
            }
            else
            {
                if (input_track_info->edit_rate != frame_rate) {
                    log_warn("Track %"PRIszt" (essence type '%s') edit rate %d/%d does not equals clip edit rate %d/%d\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type),
                             input_track_info->edit_rate.numerator, input_track_info->edit_rate.denominator,
                             frame_rate.numerator, frame_rate.denominator);
                    is_supported = false;
                } else if (!ClipWriterTrack::IsSupported(clip_type, input_track_info->essence_type, frame_rate)) {
                    log_warn("Track %"PRIszt" essence type '%s' @%d/%d fps not supported by clip type '%s'\n",
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
                    log_warn("Track %"PRIszt" (essence type '%s') has zero frame size\n",
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
                         !force_no_avci_head &&
                         !allow_no_avci_head &&
                         !track_reader->HaveAVCIHeader() &&
                         !have_avci_header_data(input_track_info->essence_type, input_track_info->edit_rate,
                                                avci_header_inputs))
                {
                    log_warn("Track %"PRIszt" (essence type '%s') does not have sequence and picture parameter sets\n",
                             i,
                             essence_type_to_string(input_track_info->essence_type));
                    is_supported = false;
                }
            }

            if (!is_supported) {
                log_warn("Ignoring unsupported track %"PRIszt" (essence type '%s')\n",
                          i, essence_type_to_string(input_track_info->essence_type));
            }

            track_reader->SetEnable(is_supported);
        }

        if (!reader->IsEnabled()) {
            log_error("All tracks are disabled\n");
            throw false;
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

        int16_t precharge = 0;
        int16_t rollout = 0;
        if (output_duration > 0) {
            precharge = reader->GetMaxPrecharge(read_start, true);
            rollout = reader->GetMaxRollout(read_start + output_duration - 1, true);

            if (precharge != 0 && (no_precharge || clip_type == CW_AVID_CLIP_TYPE)) {
                if (!no_precharge) {
                    log_warn("'%s' clip type does not support precharge\n",
                             ClipWriter::ClipWriterTypeToString(clip_type).c_str());
                }
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
                    rollout = reader->GetMaxRollout(read_start + output_duration - 1, true);
                }
                if (!no_rollout) {
                    log_warn("'%s' clip type does not support rollout\n",
                             ClipWriter::ClipWriterTypeToString(clip_type).c_str());
                }
                log_info("Rollout resulted in %"PRId64" frame adjustment of duration\n",
                         output_duration - original_output_duration);
            }
        }

        reader->SetReadLimits(read_start + precharge, - precharge + output_duration + rollout, true);


        // get input start timecode

        if (!start_timecode_str && reader->HavePlayoutTimecode()) {
            start_timecode = reader->GetPlayoutTimecode(read_start);

            // adjust start timecode to be at the point after the leading filler segments
            // this corresponds to the zero position in the MXF reader
            if (!reader->HaveFixedLeadFillerOffset())
                log_warn("No fixed lead filler offset\n");
            else
                start_timecode.AddOffset(reader->GetFixedLeadFillerOffset(), frame_rate);
        }


        // complete commandline parsing

        if (!timecode_rate_set && !is_sound_frame_rate)
            timecode_rate = frame_rate;
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


        // create output clip and initialize

        int flavour = 0;
        if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_OP1A_CLIP_TYPE) {
            flavour = OP1A_DEFAULT_FLAVOUR;
            if (clip_type == CW_OP1A_CLIP_TYPE && min_part)
                flavour |= OP1A_MIN_PARTITIONS_FLAVOUR;
            if (output_file_md5)
                flavour |= OP1A_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= OP1A_SINGLE_PASS_WRITE_FLAVOUR;
        } else if (clip_type == CW_AS11_D10_CLIP_TYPE || clip_type == CW_D10_CLIP_TYPE) {
            flavour = D10_DEFAULT_FLAVOUR;
            if (output_file_md5)
                flavour |= D10_SINGLE_PASS_MD5_WRITE_FLAVOUR;
            else if (single_pass)
                flavour |= D10_SINGLE_PASS_WRITE_FLAVOUR;
        }
        ClipWriter *clip = 0;
        Rational clip_frame_rate = (is_sound_frame_rate ? timecode_rate : frame_rate);
        switch (clip_type)
        {
            case CW_AS02_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS02Clip(output_name, true, clip_frame_rate, &file_factory, false);
                break;
            case CW_AS11_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS11OP1AClip(flavour, file_factory.OpenNew(output_name), clip_frame_rate);
                break;
            case CW_AS11_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewAS11D10Clip(flavour, file_factory.OpenNew(output_name), clip_frame_rate);
                break;
            case CW_OP1A_CLIP_TYPE:
                clip = ClipWriter::OpenNewOP1AClip(flavour, file_factory.OpenNew(output_name), clip_frame_rate);
                break;
            case CW_AVID_CLIP_TYPE:
                clip = ClipWriter::OpenNewAvidClip(clip_frame_rate, &file_factory, false);
                break;
            case CW_D10_CLIP_TYPE:
                clip = ClipWriter::OpenNewD10Clip(flavour, file_factory.OpenNew(output_name), clip_frame_rate);
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

            if (clip_type == CW_AS11_D10_CLIP_TYPE) {
                D10File *d10_clip = as11_clip->GetD10Clip();
                d10_clip->SetMuteSoundFlags(d10_mute_sound_flags);
                d10_clip->SetInvalidSoundFlags(d10_invalid_sound_flags);
            }

            if (clip_type == CW_AS11_OP1A_CLIP_TYPE && (flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR))
                as11_clip->GetOP1AClip()->SetInputDuration(reader->GetReadDuration());
            else if (clip_type == CW_AS11_D10_CLIP_TYPE && (flavour & D10_SINGLE_PASS_WRITE_FLAVOUR))
                as11_clip->GetD10Clip()->SetInputDuration(reader->GetReadDuration());

            if (!clip_name && as11_helper.HaveProgrammeTitle())
                as11_clip->SetClipName(as11_helper.GetProgrammeTitle());
        } else if (clip_type == CW_OP1A_CLIP_TYPE) {
            OP1AFile *op1a_clip = clip->GetOP1AClip();

            if (flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)
                op1a_clip->SetInputDuration(reader->GetReadDuration());

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
                    const MXFSoundTrackInfo *input_sound_info = dynamic_cast<const MXFSoundTrackInfo*>(input_track_info);

                    if (input_sound_info)
                        num_sound_tracks += input_sound_info->channel_count;
                    else
                        num_picture_tracks++;
                }
                if (tape_name) {
                    physical_package = avid_clip->CreateDefaultTapeSource(tape_name,
                                                                          num_picture_tracks, num_sound_tracks);

                    if (tp_uid_set)
                        physical_package->setPackageUID(tp_uid);
                    if (tp_created_set) {
                        physical_package->setPackageCreationDate(tp_created);
                        physical_package->setPackageModifiedDate(tp_created);
                    }
                } else {
                    URI uri;
                    if (!parse_avid_import_name(import_name, &uri)) {
                        log_error("Failed to parse import name '%s'\n", import_name);
                        throw false;
                    }
                    physical_package = avid_clip->CreateDefaultImportSource(uri.ToString(), uri.GetLastSegment(),
                                                                            num_picture_tracks, num_sound_tracks,
                                                                            false);
                    if (reader->GetMaterialPackageUID() != g_Null_UMID)
                        physical_package->setPackageUID(reader->GetMaterialPackageUID());
                }
                physical_package_picture_refs = avid_clip->GetPictureSourceReferences(physical_package);
                BMX_ASSERT(physical_package_picture_refs.size() == num_picture_tracks);
                physical_package_sound_refs = avid_clip->GetSoundSourceReferences(physical_package);
                BMX_ASSERT(physical_package_sound_refs.size() == num_sound_tracks);
            }

        } else if (clip_type == CW_D10_CLIP_TYPE) {
            D10File *d10_clip = clip->GetD10Clip();

            d10_clip->SetMuteSoundFlags(d10_mute_sound_flags);
            d10_clip->SetInvalidSoundFlags(d10_invalid_sound_flags);

            if (flavour & D10_SINGLE_PASS_WRITE_FLAVOUR)
                d10_clip->SetInputDuration(reader->GetReadDuration());
        } else if (clip_type == CW_WAVE_CLIP_TYPE) {
            WaveWriter *wave_clip = clip->GetWaveClip();

            if (originator)
                wave_clip->GetBroadcastAudioExtension()->SetOriginator(originator);
        }


        // create output tracks and initialize

        vector<OutputTrack> output_tracks;

        unsigned char avci_header_data[AVCI_HEADER_SIZE];
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
                output_track.input_buffer        = input_track_reader->GetFrameBuffer();
                output_track.is_picture          = input_track_info->is_picture;
                output_track.channel_count       = output_track_count;
                output_track.channel_index       = c;
                output_track.input_essence_type  = input_track_info->essence_type;
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

                if (clip_type == CW_AVID_CLIP_TYPE) {
                    string track_name = create_mxf_track_filename(
                                            output_name,
                                            input_track_info->is_picture ? picture_track_count + 1 :
                                                                           sound_track_count + 1,
                                            input_track_info->is_picture);
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

                    AS02PictureTrack *as02_pict_track = dynamic_cast<AS02PictureTrack*>(as02_track);
                    if (as02_pict_track)
                        as02_pict_track->SetPartitionInterval(partition_interval);
                } else if (clip_type == CW_AVID_CLIP_TYPE) {
                    AvidTrack *avid_track = output_track.track->GetAvidTrack();

                    if (physical_package) {
                        if (input_track_info->is_picture) {
                            avid_track->SetSourceRef(physical_package_picture_refs[picture_track_count].first,
                                                     physical_package_picture_refs[picture_track_count].second);
                        } else {
                            avid_track->SetSourceRef(physical_package_sound_refs[sound_track_count].first,
                                                     physical_package_sound_refs[sound_track_count].second);
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
                        if (user_aspect_ratio_set)
                            output_track.track->SetAspectRatio(user_aspect_ratio);
                        else
                            output_track.track->SetAspectRatio(input_picture_info->aspect_ratio);
                        if (afd)
                            output_track.track->SetAFD(afd);
                        BMX_ASSERT(input_picture_info->d10_frame_size != 0);
                        output_track.track->SetSampleSize(input_picture_info->d10_frame_size, !no_d10_depad);
                        break;
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

                            if (input_track_reader->HaveAVCIHeader())
                            {
                                output_track.track->SetAVCIHeader(input_track_reader->GetAVCIHeader(), AVCI_HEADER_SIZE);
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
                    case D10_AES3_PCM:
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

            if ((clip_type == CW_AS11_OP1A_CLIP_TYPE && (flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)) ||
                (clip_type == CW_AS11_D10_CLIP_TYPE  && (flavour & D10_SINGLE_PASS_WRITE_FLAVOUR)))
            {
                as11_helper.Complete();
            }
        }


        // force file md5 update now that reading/seeking will be forwards only

        if (input_file_md5) {
            size_t i;
            for (i = 0; i < file_factory.GetInputMD5WrapFiles().size(); i++)
                md5_wrap_force_update(file_factory.GetInputMD5WrapFiles()[i]);
        }


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
        int64_t last_ess_mark_pos = INT64_MIN;
        bmx::ByteArray sound_buffer;
        while (total_read < read_duration) {
            uint32_t num_read = read_samples(reader, sample_sequence, &sample_sequence_offset, max_samples_per_read);
            if (num_read == 0)
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

                if (clip_type == CW_AVID_CLIP_TYPE && convert_ess_marks && frame->position > last_ess_mark_pos) {
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
                            last_ess_mark_pos = frame->position;
                        }
                    }
                }

                uint32_t num_samples;
                if (output_track.channel_count > 1 || output_track.input_essence_type == D10_AES3_PCM) {
                    sound_buffer.Allocate(frame->GetSize()); // more than enough
                    if (output_track.input_essence_type == D10_AES3_PCM) {
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

            total_read += num_read;

            if (show_progress)
                print_progress(total_read, read_duration, &next_progress_update);

            if (max_samples_per_read > 1 && num_read < max_samples_per_read)
                break;
        }
        if (show_progress)
            print_progress(total_read, read_duration, 0);


        // set precharge and rollout for non-interleaved clip types

        if (clip_type == CW_AS02_CLIP_TYPE && (precharge || rollout)) {
            for (i = 0; i < output_tracks.size(); i++) {
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

        if ((clip_type == CW_AS11_OP1A_CLIP_TYPE && !(flavour & OP1A_SINGLE_PASS_WRITE_FLAVOUR)) ||
            (clip_type == CW_AS11_D10_CLIP_TYPE  && !(flavour & D10_SINGLE_PASS_WRITE_FLAVOUR)))
        {
            as11_helper.Complete();
        }


        // complete writing

        clip->CompleteWrite();

        log_info("Duration: %"PRId64" (%s)\n",
                 clip->GetDuration(),
                 get_generic_duration_string_2(clip->GetDuration(), clip->GetFrameRate()).c_str());


        if (total_read != read_duration) {
            log_error("Read less (%"PRId64") samples than expected (%"PRId64")\n",
                      total_read, read_duration);
            throw false;
        }


        // output file md5

        if (output_file_md5) {
            if (clip_type == CW_AS11_OP1A_CLIP_TYPE || clip_type == CW_OP1A_CLIP_TYPE) {
                OP1AFile *op1a_clip = clip->GetOP1AClip();
                AS11Clip *as11_clip = clip->GetAS11Clip();
                if (as11_clip)
                    op1a_clip = as11_clip->GetOP1AClip();

                log_info("Output file MD5: %s\n", op1a_clip->GetMD5DigestStr().c_str());
            } else if (clip_type == CW_AS11_D10_CLIP_TYPE || clip_type == CW_D10_CLIP_TYPE) {
                D10File *d10_clip = clip->GetD10Clip();
                AS11Clip *as11_clip = clip->GetAS11Clip();
                if (as11_clip)
                    d10_clip = as11_clip->GetD10Clip();

                log_info("Output file MD5: %s\n", d10_clip->GetMD5DigestStr().c_str());
            }
        }


        // input file md5

        if (input_file_md5) {
            BMX_ASSERT(input_filenames.size() == file_factory.GetInputMD5WrapFiles().size());
            unsigned char digest[16];
            size_t i;
            for (i = 0; i < file_factory.GetInputMD5WrapFiles().size(); i++) {
                md5_wrap_finalize(file_factory.GetInputMD5WrapFiles()[i], digest);
                log_info("Input file MD5: %s %s\n", input_filenames[i], md5_digest_str(digest).c_str());
            }
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

