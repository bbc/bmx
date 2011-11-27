/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <im/mxf_reader/MXFFileReader.h>
#include <im/mxf_reader/MXFGroupReader.h>
#include <im/mxf_reader/MXFSequenceReader.h>
#include <im/essence_parser/SoundConversion.h>
#include <im/MD5.h>
#include <im/MXFUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



static char* get_label_string(mxfUL label, char *buf)
{
    sprintf(buf,
            "%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
            label.octet0,  label.octet1,  label.octet2,  label.octet3,
            label.octet4,  label.octet5,  label.octet6,  label.octet7,
            label.octet8,  label.octet9,  label.octet10, label.octet11,
            label.octet12, label.octet13, label.octet14, label.octet15);

    return buf;
}

static char* get_uuid_string(mxfUUID uuid, char *buf)
{
    return get_label_string(*(mxfUL*)&uuid, buf);
}

static char* get_umid_string(mxfUMID umid, char *buf)
{
    sprintf(buf,
            "%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x."
            "%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
            umid.octet0,  umid.octet1,  umid.octet2,  umid.octet3,
            umid.octet4,  umid.octet5,  umid.octet6,  umid.octet7,
            umid.octet8,  umid.octet9,  umid.octet10, umid.octet11,
            umid.octet12, umid.octet13, umid.octet14, umid.octet15,
            umid.octet16, umid.octet17, umid.octet18, umid.octet19,
            umid.octet20, umid.octet21, umid.octet22, umid.octet23,
            umid.octet24, umid.octet25, umid.octet26, umid.octet27,
            umid.octet28, umid.octet29, umid.octet30, umid.octet31);

    return buf;
}

static char* get_rational_string(mxfRational rat, char *buf)
{
    sprintf(buf, "%d/%d", rat.numerator, rat.denominator);
    return buf;
}

static char* get_timestamp_string(mxfTimestamp timestamp, char *buf)
{
    sprintf(buf, "%d-%02u-%02u %02u:%02u:%02u.%03u",
            timestamp.year, timestamp.month, timestamp.day,
            timestamp.hour, timestamp.min, timestamp.sec, timestamp.qmsec * 4);

    return buf;
}

static char* get_product_version_string(mxfProductVersion version, char *buf)
{
    const char *release_string;
    switch (version.release)
    {
        case 0:
            release_string = "unknown version";
            break;
        case 1:
            release_string = "released version";
            break;
        case 2:
            release_string = "development version";
            break;
        case 3:
            release_string = "released with patches version";
            break;
        case 4:
            release_string = "pre-release beta version";
            break;
        case 5:
            release_string = "private version";
            break;
        default:
            release_string = "unrecognized release number";
            break;
    }

    sprintf(buf, "%u.%u.%u.%u.%u (%s)",
            version.major, version.minor, version.patch, version.build, version.release, release_string);

    return buf;
}

static const char* get_signal_standard_string(uint8_t signal_standard)
{
    switch (signal_standard)
    {
        case MXF_SIGNAL_STANDARD_NONE:
            return "None";
        case MXF_SIGNAL_STANDARD_ITU601:
            return "ITU 601";
        case MXF_SIGNAL_STANDARD_ITU1358:
            return "ITU 1358";
        case MXF_SIGNAL_STANDARD_SMPTE347M:
            return "SMPTE 347M";
        case MXF_SIGNAL_STANDARD_SMPTE274M:
            return "SMPTE 274M";
        case MXF_SIGNAL_STANDARD_SMPTE296M:
            return "SMPTE 296M";
        case MXF_SIGNAL_STANDARD_SMPTE349M:
            return "SMPTE 349M";
        case MXF_SIGNAL_STANDARD_SMPTE428_1:
            return "SMPTE 428-1";
        default:
            break;
    }

    return "not recognized";
}

static const char* get_frame_layout_string(uint8_t frame_layout)
{
    switch (frame_layout)
    {
        case MXF_FULL_FRAME:
            return "Full Frame";
        case MXF_SEPARATE_FIELDS:
            return "Separate Fields";
        case MXF_SINGLE_FIELD:
            return "Single Field";
        case MXF_MIXED_FIELDS:
            return "Mixed Fields";
        case MXF_SEGMENTED_FRAME:
            return "Segmented Frame";
        default:
            break;
    }

    return "not recognized";
}

static const char* get_color_siting_string(uint8_t color_siting)
{
    switch (color_siting)
    {
        case MXF_COLOR_SITING_COSITING:
            return "Co-siting";
        case MXF_COLOR_SITING_HORIZ_MIDPOINT:
            return "Horizontal Mid-point";
        case MXF_COLOR_SITING_THREE_TAP:
            return "Three Tap";
        case MXF_COLOR_SITING_QUINCUNX:
            return "Quincunx";
        case MXF_COLOR_SITING_REC601:
            return "Rec 601";
        case MXF_COLOR_SITING_LINE_ALTERN:
            return "Line Alternating";
        case MXF_COLOR_SITING_VERT_MIDPOINT:
            return "Vertical Mid-point";
        case MXF_COLOR_SITING_UNKNOWN:
            return "Unknown";
        default:
            break;
    }

    return "not recognized";
}

static void print_track_info(const MXFTrackInfo *track_info)
{
    char string_buffer[128];

    const MXFPictureTrackInfo *picture_info = dynamic_cast<const MXFPictureTrackInfo*>(track_info);
    const MXFSoundTrackInfo *sound_info = dynamic_cast<const MXFSoundTrackInfo*>(track_info);

    printf("  Essence kind         : %s\n", (picture_info ? "Picture" : (sound_info ? "Sound" : "Unknown")));
    printf("  Essence type         : %s\n", MXFDescriptorHelper::EssenceTypeToString(track_info->essence_type).c_str());
    printf("  Essence label        : %s\n", get_label_string(track_info->essence_container_label, string_buffer));
    printf("  Material package uid : %s\n", get_umid_string(track_info->material_package_uid, string_buffer));
    printf("  Material track id    : %u\n", track_info->material_track_id);
    printf("  Material track number: %u\n", track_info->material_track_number);
    printf("  File package uid     : %s\n", get_umid_string(track_info->file_package_uid, string_buffer));
    printf("  File track id        : %u\n", track_info->file_track_id);
    printf("  File track number    : 0x%08x\n", track_info->file_track_number);
    printf("  Edit rate            : %s\n", get_rational_string(track_info->edit_rate, string_buffer));
    printf("  Duration             : %"PRId64"\n", track_info->duration);
    printf("  Lead filler offset   : %"PRId64"\n", track_info->lead_filler_offset);

    if (picture_info) {
        printf("  Picture coding label : %s\n", get_label_string(picture_info->picture_essence_coding_label, string_buffer));
        printf("  Signal standard      : %u (%s)\n", picture_info->signal_standard, get_signal_standard_string(picture_info->signal_standard));
        printf("  Frame layout         : %u (%s)\n", picture_info->frame_layout, get_frame_layout_string(picture_info->frame_layout));
        printf("  Color siting         : %u (%s)\n", picture_info->color_siting, get_color_siting_string(picture_info->color_siting));
        printf("  Stored dimensions    : %ux%u\n", picture_info->stored_width, picture_info->stored_height);
        printf("  Display dimensions   : %ux%u\n", picture_info->display_width, picture_info->display_height);
        printf("  Display x offset     : %u\n", picture_info->display_x_offset);
        printf("  Display y offset     : %u\n", picture_info->display_y_offset);
        printf("  Horiz subsampling    : %u\n", picture_info->horiz_subsampling);
        printf("  Vert subsampling     : %u\n", picture_info->vert_subsampling);
        printf("  Component depth      : %u\n", picture_info->component_depth);
        printf("  Aspect ratio         : %s\n", get_rational_string(picture_info->aspect_ratio, string_buffer));
        printf("  AFD                  : ");
        if (picture_info->afd)
            printf("%u\n", picture_info->afd);
        else
            printf("(not set)\n");
    } else if (sound_info) {
        printf("  Sampling rate        : %s\n", get_rational_string(sound_info->sampling_rate, string_buffer));
        printf("  Bits per sample      : %u\n", sound_info->bits_per_sample);
        printf("  Block align          : %u\n", sound_info->block_align);
        printf("  Channel count        : %u\n", sound_info->channel_count);
        printf("  Sequence offset      : %u\n", sound_info->sequence_offset);
        printf("  Locked               : %s\n", (sound_info->locked_set ? (sound_info->locked ? "true" : "false") : "(not set)"));
        printf("  Audio ref level      : ");
        if (sound_info->audio_ref_level_set)
            printf("%d\n", sound_info->audio_ref_level);
        else
            printf("(not set)\n");
        printf("  Dial norm            : ");
        if (sound_info->dial_norm_set)
            printf("%d\n", sound_info->dial_norm);
        else
            printf("(not set)\n");
    }
}

static void print_identification_info(Identification *identification)
{
    char string_buffer[128];

    printf("      Generation UID    : %s\n", get_uuid_string(identification->getThisGenerationUID(), string_buffer));
    printf("      Company name      : %s\n", identification->getCompanyName().c_str());
    printf("      Product name      : %s\n", identification->getProductName().c_str());
    printf("      Product version   : ");
    if (identification->haveProductVersion())
        printf("%s\n", get_product_version_string(identification->getProductVersion(), string_buffer));
    else
        printf("(not set)\n");
    printf("      Version string    : %s\n", identification->getVersionString().c_str());
    printf("      Product UID       : %s\n", get_uuid_string(identification->getProductUID(), string_buffer));
    printf("      Modification date : %s\n", get_timestamp_string(identification->getModificationDate(), string_buffer));
    printf("      Toolkit version   : ");
    if (identification->haveToolkitVersion())
        printf("%s\n", get_product_version_string(identification->getToolkitVersion(), string_buffer));
    else
        printf("(not set)\n");
    printf("      Platform          : ");
    if (identification->havePlatform())
        printf("%s\n", identification->getPlatform().c_str());
    else
        printf("(not set)\n");
}

static string timecode_to_string(Timecode timecode)
{
    char buffer[64];
    sprintf(buffer, "%02d:%02d:%02d%c%02d (@ %dfps)",
            timecode.GetHour(), timecode.GetMin(), timecode.GetSec(),
            timecode.IsDropFrame() ? ';' : ':', timecode.GetFrame(),
            timecode.GetRoundedTCBase());
    return buffer;
}

static string create_raw_filename(string prefix, bool is_video, uint32_t index, int32_t child_index)
{
    char buffer[64];
    if (child_index >= 0)
        sprintf(buffer, "_%s%u_%d.raw", is_video ? "v" : "a", index, child_index);
    else
        sprintf(buffer, "_%s%u.raw", is_video ? "v" : "a", index);

    return prefix + buffer;
}

static string get_version_info()
{
    char buffer[256];
    sprintf(buffer, "mxf2raw, %s v%s (%s %s)", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
    return buffer;
}

static void usage(const char* cmd)
{
    fprintf(stderr, "%s\n", get_version_info().c_str());
    fprintf(stderr, "Usage: %s <<options>> <filename>+\n", cmd);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -h | --help           Show usage and exit\n");
    fprintf(stderr, " -v | --version        Print version info\n");
    fprintf(stderr, " -l <file>             Log filename. Default log to stderr/stdout\n");
    fprintf(stderr, " -p <prefix>           Raw output filename prefix\n");
    fprintf(stderr, " -r                    Read frames even if -p or --md5 not used\n");
    fprintf(stderr, " -i                    Print file information to stdout\n");
    fprintf(stderr, " -d                    De-interleave multi-channel / AES-3 sound\n");
    fprintf(stderr, " --start <frame>       Set the start frame. Default is 0\n");
    fprintf(stderr, " --dur <frame>         Set the duration in frames. Default is minimum avaliable duration\n");
    fprintf(stderr, " --nopc                Don't include pre-charge frames\n");
    fprintf(stderr, " --noro                Don't include roll-out frames\n");
    fprintf(stderr, " --md5                 Calculate md5 checksum of essence data\n");
    fprintf(stderr, " --group               Use the group reader instead of the sequence reader\n");
    fprintf(stderr, "                       Use this option if the files have different material packages\n");
    fprintf(stderr, "                       but actually belong to the same virtual package / group\n");
    fprintf(stderr, " --no-reorder          Don't attempt to order the inputs in a sequence\n");
    fprintf(stderr, "                       Use this option for files with broken timecode\n");
}

int main(int argc, const char** argv)
{
    const char *log_filename = 0;
    std::vector<const char *> filenames;
    const char *prefix = 0;
    bool do_read = false;
    bool do_print_info = false;
    bool deinterleave = false;
    int64_t start = 0;
    int64_t duration = -1;
    bool no_precharge = false;
    bool no_rollout = false;
    bool calc_md5 = false;
    bool use_group_reader = false;
    bool keep_input_order = false;
    bool do_print_version = false;
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
        else if (strcmp(argv[cmdln_index], "-p") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            prefix = argv[cmdln_index + 1];
            do_read = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "-r") == 0)
        {
            do_read = true;
        }
        else if (strcmp(argv[cmdln_index], "-i") == 0)
        {
            do_print_info = true;
        }
        else if (strcmp(argv[cmdln_index], "-d") == 0)
        {
            deinterleave = true;
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
        else if (strcmp(argv[cmdln_index], "--nopc") == 0)
        {
            no_precharge = true;
        }
        else if (strcmp(argv[cmdln_index], "--noro") == 0)
        {
            no_rollout = true;
        }
        else if (strcmp(argv[cmdln_index], "--md5") == 0)
        {
            calc_md5 = true;
            do_read = true;
        }
        else if (strcmp(argv[cmdln_index], "--group") == 0)
        {
            use_group_reader = true;
        }
        else if (strcmp(argv[cmdln_index], "--no-reorder") == 0)
        {
            keep_input_order = true;
        }
        else
        {
            break;
        }
    }

    if (cmdln_index + 1 > argc) {
        usage(argv[0]);
        fprintf(stderr, "Missing parameters\n");
        return 1;
    } else if (cmdln_index >= argc) {
        usage(argv[0]);
        fprintf(stderr, "No <filename> given\n");
        return 1;
    }

    for (; cmdln_index < argc; cmdln_index++)
        filenames.push_back(argv[cmdln_index]);


    if (log_filename) {
        if (!open_log_file(log_filename))
            return 1;
    }

    connect_libmxf_logging();

    if (do_print_version)
        log_info("%s\n", get_version_info().c_str());


    int cmd_result = 0;
    try
    {
        MXFReader *reader;
        MXFFileReader *file_reader = 0;
        if (use_group_reader) {
            MXFGroupReader *group_reader = new MXFGroupReader();
            size_t i;
            for (i = 0; i < filenames.size(); i++) {
                MXFFileReader *file_reader;
                MXFFileReader::OpenResult result = MXFFileReader::Open(filenames[i], 0, false, &file_reader);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    throw false;
                }

                group_reader->AddReader(file_reader);
            }
            group_reader->Finalize();

            reader = group_reader;
        } else if (filenames.size() > 1) {
            MXFSequenceReader *seq_reader = new MXFSequenceReader();
            size_t i;
            for (i = 0; i < filenames.size(); i++) {
                MXFFileReader *file_reader;
                MXFFileReader::OpenResult result = MXFFileReader::Open(filenames[i], 0, false, &file_reader);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", filenames[i],
                              MXFFileReader::ResultToString(result).c_str());
                    throw false;
                }

                seq_reader->AddReader(file_reader);
            }
            seq_reader->Finalize(false, keep_input_order);

            reader = seq_reader;
        } else {
            MXFFileReader::OpenResult result = MXFFileReader::Open(filenames[0], 0, false, &file_reader);
            if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                log_error("Failed to open MXF file '%s': %s\n", filenames[0],
                          MXFFileReader::ResultToString(result).c_str());
                throw false;
            }

            reader = file_reader;
        }


        mxfRational sample_rate = reader->GetSampleRate();


        // set read limits
        int64_t input_duration = reader->GetDuration();
        if (start > input_duration) {
            log_error("Start position %"PRId64" is beyond available frames %"PRId64"\n", start, input_duration);
            throw false;
        }
        int64_t output_duration = input_duration - start;
        if (duration >= 0) {
            if (duration <= output_duration) {
                output_duration = duration;
            } else {
                log_warn("Output duration %"PRId64" not possible. Set to %"PRId64" instead\n",
                         duration, output_duration);
            }
        }

        int64_t max_precharge = 0;
        if (!no_precharge)
            max_precharge = reader->GetMaxPrecharge(start, true);
        int64_t max_rollout = 0;
        if (!no_rollout)
            max_rollout = reader->GetMaxRollout(start + output_duration - 1, true);

        reader->SetReadLimits(start + max_precharge, start + output_duration + max_rollout, true);

        int64_t lead_filler_offset = 0;
        if (!reader->HaveFixedLeadFillerOffset())
            log_warn("No fixed lead filler offset\n");
        else
            lead_filler_offset = reader->GetFixedLeadFillerOffset();


        // print info

        if (do_print_info) {
            if (file_reader) {
                printf("MXF File Information:\n");
                printf("  Filename                   : %s\n", filenames[0]);
                printf("  Material package name      : %s\n", reader->GetMaterialPackageName().c_str());
                if (reader->HaveMaterialTimecode()) {
                        printf("  Material start timecode    : %s\n",
                           timecode_to_string(reader->GetMaterialTimecode(0)).c_str());
                    if (lead_filler_offset != 0) {
                        printf("  Material start timecode including lead filler offset: %s\n",
                               timecode_to_string(reader->GetMaterialTimecode(lead_filler_offset)).c_str());
                    }
                }
                if (reader->HaveFileSourceTimecode())
                    printf("  File src start timecode    : %s\n", timecode_to_string(reader->GetFileSourceTimecode(0)).c_str());
                if (reader->HavePhysicalSourceTimecode()) {
                    printf("  Physical src start timecode: %s\n",
                           timecode_to_string(reader->GetPhysicalSourceTimecode(0)).c_str());
                    printf("  Physical src package name  : %s\n", reader->GetPhysicalSourcePackageName().c_str());
                }
                printf("  Precharge                  : %"PRId64"\n", max_precharge);
                printf("  Duration                   : %"PRId64"\n", output_duration);
                printf("  Rollout                    : %"PRId64"\n", max_rollout);

                printf("  Identifications            :\n");
                vector<Identification*> identifications = file_reader->GetHeaderMetadata()->getPreface()->getIdentifications();
                size_t i;
                for (i = 0; i < identifications.size(); i++) {
                    printf("    Identification %zu:\n", i);
                    print_identification_info(identifications[i]);
                }
            }

            size_t i;
            for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                printf("Track %zu:\n", i);
                print_track_info(reader->GetTrackReader(i)->GetTrackInfo());
            }
            printf("\n");
        }


        // read data

        if (do_read) {
            // log info
            if (prefix)
                log_info("Output prefix: %s\n", prefix);
            if (log_filename || !do_print_info) {
                if (file_reader) {
                    log_info("Input filename: %s\n", filenames[0]);
                    log_info("Material package name: %s\n", reader->GetMaterialPackageName().c_str());
                }
                log_info("Precharge: %"PRId64"\n", max_precharge);
                log_info("Duration: %"PRId64"\n", output_duration);
                log_info("Rollout: %"PRId64"\n", max_rollout);
                log_info("Edit rate: %d/%d\n", sample_rate.numerator, sample_rate.denominator);
            }

            // md5 calculation initialization
            vector<MD5Context> md5_contexts;
            if (calc_md5) {
                md5_contexts.resize(reader->GetNumTrackReaders());
                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++)
                    md5_init(&md5_contexts[i]);
            }

            // open raw files and check if have video
            bool have_video = false;
            vector<FILE*> raw_files;
            vector<string> raw_filenames;
            if (prefix) {
                int video_count = 0, audio_count = 0;
                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    const MXFTrackInfo *track_info = reader->GetTrackReader(i)->GetTrackInfo();
                    if (track_info->is_picture) {
                        string raw_filename = create_raw_filename(prefix, true, video_count, -1);
                        FILE *raw_file = fopen(raw_filename.c_str(), "wb");
                        if (!raw_file) {
                            log_error("Failed to open raw file '%s': %s\n",
                                      raw_filename.c_str(), strerror(errno));
                            throw false;
                        }
                        raw_files.push_back(raw_file);
                        raw_filenames.push_back(raw_filename);
                        video_count++;
                        have_video = true;
                    } else if (track_info->is_sound) {
                        const MXFSoundTrackInfo *sound_info = dynamic_cast<const MXFSoundTrackInfo*>(track_info);
                        if (deinterleave && sound_info->channel_count > 1) {
                            uint32_t c;
                            for (c = 0; c < sound_info->channel_count; c++) {
                                string raw_filename = create_raw_filename(prefix, false, audio_count, c);
                                FILE *raw_file = fopen(raw_filename.c_str(), "wb");
                                if (!raw_file) {
                                    log_error("Failed to open raw file '%s': %s\n",
                                              raw_filename.c_str(), strerror(errno));
                                    throw false;
                                }
                                raw_files.push_back(raw_file);
                                raw_filenames.push_back(raw_filename);
                            }
                        } else {
                            string raw_filename = create_raw_filename(prefix, false, audio_count, -1);
                            FILE *raw_file = fopen(raw_filename.c_str(), "wb");
                            if (!raw_file) {
                                log_error("Failed to open raw file '%s': %s\n",
                                          raw_filename.c_str(), strerror(errno));
                                throw false;
                            }
                            raw_files.push_back(raw_file);
                            raw_filenames.push_back(raw_filename);
                        }
                        audio_count++;
                    }
                }
            }

            // choose number of samples to read in one go
            uint32_t max_samples_per_read = 1;
            if (!have_video && sample_rate.numerator == 48000 && sample_rate.denominator == 1)
                max_samples_per_read = 1920;

            // read data
            im::ByteArray sound_buffer;
            int64_t total_num_read = 0;
            int64_t frame_position = 0;
            while (true)
            {
                frame_position = reader->GetPosition();
                uint32_t num_read = reader->Read(max_samples_per_read);
                if (num_read == 0)
                    break;
                total_num_read += num_read;

                int file_count = 0;
                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    while (true) {
                        Frame *frame = reader->GetTrackReader(i)->GetFrameBuffer()->GetLastFrame(true);
                        if (!frame)
                            break;
                        if (frame->IsEmpty()) {
                            delete frame;
                            continue;
                        }

                        if (calc_md5)
                            md5_update(&md5_contexts[i], frame->GetBytes(), frame->GetSize());

                        if (prefix) {
                            const MXFSoundTrackInfo *sound_info =
                                dynamic_cast<const MXFSoundTrackInfo*>(reader->GetTrackReader(i)->GetTrackInfo());
                            if (sound_info && deinterleave && sound_info->channel_count > 1) {
                                sound_buffer.Allocate(frame->GetSize()); // more than enough
                                uint32_t c;
                                for (c = 0; c < sound_info->channel_count; c++) {
                                    if (sound_info->IsD10Audio()) {
                                        convert_aes3_to_pcm(frame->GetBytes(), frame->GetSize(),
                                                            sound_info->bits_per_sample, c,
                                                            sound_buffer.GetBytes(), sound_buffer.GetAllocatedSize());
                                        sound_buffer.SetSize(sound_info->block_align / sound_info->channel_count *
                                                                get_aes3_sample_count(frame->GetBytes(), frame->GetSize()));
                                    } else {
                                        deinterleave_audio(frame->GetBytes(), frame->GetSize(),
                                                           sound_info->bits_per_sample, sound_info->channel_count, c,
                                                           sound_buffer.GetBytes(), sound_buffer.GetAllocatedSize());
                                        sound_buffer.SetSize(frame->GetSize() / sound_info->channel_count);
                                    }
                                    uint32_t num_written = fwrite(sound_buffer.GetBytes(), sound_buffer.GetSize(), 1,
                                                                  raw_files[file_count]);
                                    if (num_written != 1) {
                                        log_error("Failed to write to raw file '%s': %s\n",
                                                  raw_filenames[file_count].c_str(), strerror(errno));
                                        throw false;
                                    }
                                    file_count++;
                                }
                            } else {
                                uint32_t num_written = fwrite(frame->GetBytes(), frame->GetSize(), 1,
                                                              raw_files[file_count]);
                                if (num_written != 1) {
                                    log_error("Failed to write to raw file '%s': %s\n",
                                              raw_filenames[file_count].c_str(), strerror(errno));
                                    throw false;
                                }
                                file_count++;
                            }
                        } else {
                            const MXFSoundTrackInfo *sound_info =
                                dynamic_cast<const MXFSoundTrackInfo*>(reader->GetTrackReader(i)->GetTrackInfo());
                            if (sound_info && deinterleave && sound_info->channel_count > 1)
                                file_count += sound_info->channel_count;
                            else
                                file_count++;
                        }

                        delete frame;
                    }
                }
            }

            log_info("Read %"PRId64" samples\n", total_num_read);

            if (calc_md5) {
                unsigned char digest[16];
                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    md5_final(digest, &md5_contexts[i]);
                    log_info("MD5 Track %zu: %s\n", i, md5_digest_str(digest).c_str());
                }
            }


            // clean-up
            size_t i;
            for (i = 0; i < raw_files.size(); i++)
                fclose(raw_files[i]);
        }


        // clean-up
        delete reader;
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

