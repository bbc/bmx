/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif

#include <map>
#include <set>

#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/mxf_reader/MXFGroupReader.h>
#include <bmx/mxf_reader/MXFSequenceReader.h>
#include <bmx/mxf_reader/MXFFrameMetadata.h>
#include <bmx/mxf_reader/MXFTimedTextTrackReader.h>
#include <bmx/essence_parser/SoundConversion.h>
#include <bmx/st436/ST436Element.h>
#include <bmx/st436/RDD6Metadata.h>
#include <bmx/MD5.h>
#include <bmx/CRC32.h>
#include <bmx/MXFHTTPFile.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/URI.h>
#include <bmx/Version.h>
#include <bmx/apps/AppUtils.h>
#include <bmx/apps/AppMXFFileFactory.h>
#include <bmx/apps/AppTextInfoWriter.h>
#include <bmx/apps/AppXMLInfoWriter.h>
#include "AS11InfoOutput.h"
#include "AS10InfoOutput.h"
#include "APPInfoOutput.h"
#include "AvidInfoOutput.h"
#include "OutputFileManager.h"
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


#define DEFAULT_GF_RETRIES          10
#define DEFAULT_GF_RETRY_DELAY      1.0
#define DEFAULT_GF_RATE_AFTER_FAIL  1.5

#define DEFAULT_ST436_MANIFEST_COUNT    2

#define CHECK_FPRINTF(fname, pr)                                                                    \
    do {                                                                                            \
        if (pr < 0) {                                                                               \
            log_error("Failed to write to '%s': %s\n", fname, bmx_strerror(errno).c_str());         \
            throw false;                                                                            \
        }                                                                                           \
    } while (0)


typedef enum
{
    TEXT_INFO_FORMAT,
    XML_INFO_FORMAT,
} InfoFormat;

typedef enum
{
    CRC32_PASSED,
    CRC32_FAILED,
    CRC32_MISSING_DATA,
    CRC32_NOT_CHECKED,
} CRC32CheckResult;

typedef struct
{
    LogLevel level;
    string source;
    string message;
} LogMessage;

typedef struct
{
    vector<LogMessage> messages;
    vlog2_func vlog2;
} LogData;

typedef struct
{
    int64_t total_read;
    int64_t check_count;
    int64_t error_count;
} CRC32Data;


static LogData LOG_DATA;

static const char *APP_NAME                     = "mxf2raw";
static const char *XML_INFO_WRITER_NAMESPACE    = "http://bbc.co.uk/rd/bmx/201312";
static const char *XML_INFO_WRITER_VERSION      = "0.1"; // format <major>.<minor>

static const char* STDIN_FILENAME = "stdin:";

static const uint32_t DEFAULT_HTTP_MIN_READ = 64 * 1024;


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};

static const EnumInfo CRC32_CHECK_RESULT_EINFO[] =
{
    {CRC32_PASSED,          "Passed"},
    {CRC32_FAILED,          "Failed"},
    {CRC32_MISSING_DATA,    "Missing"},
    {CRC32_NOT_CHECKED,     "Not_Checked"},
    {0, 0}
};

static const EnumInfo SIGNAL_STANDARD_EINFO[] =
{
    {MXF_SIGNAL_STANDARD_NONE,          "None"},
    {MXF_SIGNAL_STANDARD_ITU601,        "ITU_601"},
    {MXF_SIGNAL_STANDARD_ITU1358,       "ITU_1358"},
    {MXF_SIGNAL_STANDARD_SMPTE347M,     "SMPTE_347"},
    {MXF_SIGNAL_STANDARD_SMPTE274M,     "SMPTE_274"},
    {MXF_SIGNAL_STANDARD_SMPTE296M,     "SMPTE_296"},
    {MXF_SIGNAL_STANDARD_SMPTE349M,     "SMPTE_349"},
    {MXF_SIGNAL_STANDARD_SMPTE428_1,    "SMPTE_428_1"},
    {0, 0}
};

static const EnumInfo FRAME_LAYOUT_EINFO[] =
{
    {MXF_FULL_FRAME,        "Full_Frame"},
    {MXF_SEPARATE_FIELDS,   "Separate_Fields"},
    {MXF_SINGLE_FIELD,      "Single_Field"},
    {MXF_MIXED_FIELDS,      "Mixed_Fields"},
    {MXF_SEGMENTED_FRAME,   "Segmented_Frame"},
    {0, 0}
};

static const EnumInfo COLOR_SITING_EINFO[] =
{
    {MXF_COLOR_SITING_COSITING,         "Cositing"},
    {MXF_COLOR_SITING_HORIZ_MIDPOINT,   "Horizontal_Midpoint"},
    {MXF_COLOR_SITING_THREE_TAP,        "Three_Tap"},
    {MXF_COLOR_SITING_QUINCUNX,         "Quincunx"},
    {MXF_COLOR_SITING_REC601,           "Rec_601"},
    {MXF_COLOR_SITING_LINE_ALTERN,      "Line_Alternating"},
    {MXF_COLOR_SITING_VERT_MIDPOINT,    "Vertical_Midpoint"},
    {MXF_COLOR_SITING_UNKNOWN,          "Unknown"},
    {0, 0}
};

static const EnumInfo ESSENCE_KIND_EINFO[] =
{
    {MXF_PICTURE_DDEF,      "Picture"},
    {MXF_SOUND_DDEF,        "Sound"},
    {MXF_TIMECODE_DDEF,     "Timecode"},
    {MXF_DATA_DDEF,         "Data"},
    {MXF_DM_DDEF,           "Descriptive_Metadata"},
    {0, 0}
};

static const EnumInfo VBI_WRAPPING_TYPE_EINFO[] =
{
    {VBI_FRAME,                 "VBI_Frame"},
    {VBI_FIELD1,                "VBI_Field_1"},
    {VBI_FIELD2,                "VBI_Field_2"},
    {VBI_PROGRESSIVE_FRAME,     "VBI_Progressive_Frame"},
    {0, 0}
};

static const EnumInfo VBI_SAMPLE_CODING_EINFO[] =
{
    {VBI_1_BIT_COMP_LUMA,           "VBI_1_Bit_Luma"},
    {VBI_1_BIT_COMP_COLOR,          "VBI_1_Bit_Color_Diff"},
    {VBI_1_BIT_COMP_LUMA_COLOR,     "VBI_1_Bit_Luma_Color_Diff"},
    {VBI_8_BIT_COMP_LUMA,           "VBI_8_Bit_Luma"},
    {VBI_8_BIT_COMP_COLOR,          "VBI_8_Bit_Color_Diff"},
    {VBI_8_BIT_COMP_LUMA_COLOR,     "VBI_8_Bit_Luma_Color_Diff"},
    {VBI_10_BIT_COMP_LUMA,          "VBI_10_Bit_Luma"},
    {VBI_10_BIT_COMP_COLOR,         "VBI_10_Bit_Color_Diff"},
    {VBI_10_BIT_COMP_LUMA_COLOR,    "VBI_10_Bit_Luma_Color_Diff"},
    {10,                            "Reserved"},
    {11,                            "Reserved"},
    {12,                            "Reserved"},
    {0, 0}
};

static const EnumInfo ANC_WRAPPING_TYPE_EINFO[] =
{
    {VANC_FRAME,                "VANC_Frame"},
    {VANC_FIELD1,               "VANC_Field_1"},
    {VANC_FIELD2,               "VANC_Field_2"},
    {VANC_PROGRESSIVE_FRAME,    "VANC_Progressive_Frame"},
    {HANC_FRAME,                "HANC_Frame"},
    {HANC_FIELD1,               "HANC_Field_1"},
    {HANC_FIELD2,               "HANC_Field_2"},
    {HANC_PROGRESSIVE_FRAME,    "HANC_Progressive_Frame"},
    {0, 0}
};

static const EnumInfo ANC_SAMPLE_CODING_EINFO[] =
{
    {1,                                 "Reserved"},
    {2,                                 "Reserved"},
    {3,                                 "Reserved"},
    {ANC_8_BIT_COMP_LUMA,               "ANC_8_Bit_Luma"},
    {ANC_8_BIT_COMP_COLOR,              "ANC_8_Bit_Color_Diff"},
    {ANC_8_BIT_COMP_LUMA_COLOR,         "ANC_8_Bit_Luma_Color_Diff"},
    {ANC_10_BIT_COMP_LUMA,              "ANC_10_Bit_Luma"},
    {ANC_10_BIT_COMP_COLOR,             "ANC_10_Bit_Color_Diff"},
    {ANC_10_BIT_COMP_LUMA_COLOR,        "ANC_10_Bit_Luma_Color_Diff"},
    {ANC_8_BIT_COMP_LUMA_ERROR,         "ANC_8_Bit_Luma_Parity_Error"},
    {ANC_8_BIT_COMP_COLOR_ERROR,        "ANC_8_Bit_Color_Diff_Parity_Error"},
    {ANC_8_BIT_COMP_LUMA_COLOR_ERROR,   "ANC_8_Bit_Luma_Color_Diff_Parity_Error"},
    {0, 0}
};

static const EnumInfo CHECKSUM_TYPE_EINFO[] =
{
    {CRC32_CHECKSUM,    "CRC32"},
    {MD5_CHECKSUM,      "MD5"},
    {SHA1_CHECKSUM,     "SHA1"},
    {0, 0}
};


static void forward_log_message(vlog2_func lgf, LogLevel level, const char *source, const char *format, ...)
{
    va_list p_arg;
    va_start(p_arg, format);
    lgf(level, source, format, p_arg);
    va_end(p_arg);
}

static void dump_log_messages()
{
    set_stderr_log_file();

    size_t i;
    for (i = 0; i < LOG_DATA.messages.size(); i++) {
        forward_log_message(bmx::vlog2, LOG_DATA.messages[i].level,
                            LOG_DATA.messages[i].source.empty() ? 0 : LOG_DATA.messages[i].source.c_str(),
                            "%s\n", LOG_DATA.messages[i].message.c_str());
    }
}

static void mxf2raw_vlog2(LogLevel level, const char *source, const char *format, va_list p_arg)
{
    if (level < LOG_LEVEL)
        return;

    char message[1024];
    bmx_vsnprintf(message, sizeof(message), format, p_arg);

    if (LOG_DATA.vlog2)
        forward_log_message(LOG_DATA.vlog2, level, source, "%s", message);

    // remove newline characters from the end of the message
    size_t end = strlen(message);
    while (end > 0 && (message[end - 1] == '\n' || message[end - 1] == '\r')) {
        message[end - 1] = 0;
        end--;
    }

    LogMessage log_message;
    log_message.level = level;
    if (source)
        log_message.source = source;
    log_message.message = message;
    LOG_DATA.messages.push_back(log_message);
}

static void mxf2raw_log(LogLevel level, const char *format, ...)
{
    va_list p_arg;

    va_start(p_arg, format);
    mxf2raw_vlog2(level, 0, format, p_arg);
    va_end(p_arg);
}

static void mxf2raw_vlog(LogLevel level, const char *format, va_list p_arg)
{
    mxf2raw_vlog2(level, 0, format, p_arg);
}

static const char* get_input_filename(const char *filename)
{
    if (filename[0] == 0)
        return STDIN_FILENAME;
    else
        return filename;
}

static const char* get_checksum_type_str(ChecksumType type)
{
    const EnumInfo *enum_info_ptr = CHECKSUM_TYPE_EINFO;
    while (enum_info_ptr->name) {
        if (enum_info_ptr->value == type)
            return enum_info_ptr->name;
        enum_info_ptr++;
    }

    BMX_ASSERT(false);
    return "";
}

static void calc_file_checksums(const vector<const char *> &filenames, const vector<ChecksumType> &checksum_types)
{
    size_t i;
    for (i = 0; i < filenames.size(); i++) {
        vector<string> checksum_strs;
        if (filenames[i][0] == 0) {
#if defined(_WIN32)
            int res = _setmode(_fileno(stdin), _O_BINARY);
            if (res == -1) {
                log_error("Failed to set 'stdin' to binary mode: %s\n", bmx_strerror(errno).c_str());
                throw false;
            }
#endif
            checksum_strs = Checksum::CalcFileChecksums(stdin, checksum_types);
        } else {
            checksum_strs = Checksum::CalcFileChecksums(filenames[i], checksum_types);
        }
        if (checksum_strs.empty()) {
            log_error("Failed to calculate checksum for file '%s'\n",
                      get_input_filename(filenames[i]));
            throw false;
        }

        if (checksum_types.size() == 1) {
            // matches output format produced by md5sum and sha1sum
            if (filenames[i] == 0)
                printf("%s  -\n", checksum_strs[0].c_str());
            else
                printf("%s  %s\n", checksum_strs[0].c_str(), filenames[i]);
        } else {
            size_t j;
            for (j = 0; j < checksum_types.size(); j++) {
                if (filenames[i] == 0)
                    printf("%s: %s  -\n", get_checksum_type_str(checksum_types[j]), checksum_strs[j].c_str());
                else
                    printf("%s: %s  %s\n", get_checksum_type_str(checksum_types[j]), checksum_strs[j].c_str(), filenames[i]);
            }
        }
    }
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

static string get_d10_sound_flags(uint8_t flags)
{
    char buf[10];

    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (flags & (1 << i))
            buf[7 - i] = '1';
        else
            buf[7 - i] = '0';
    }
    buf[i    ] = 'b';
    buf[i + 1] = 0;

    return buf;
}

static const char* get_did_type1_string(uint8_t did)
{
    static const struct
    {
        uint8_t did;
        const char *application;
    } did_type1_table[] =
    {
        {0x80, "[S291] Packet marked for deletion"},
        {0x84, "[S291] End packet deleted  (Deprecated; revision of ST291-2010)"},
        {0x88, "[S291] Start packet deleted (Deprecated; revision of ST291-2010)"},
        {0xa0, "[ST 299-2] Audio data in HANC space (3G) - Group 8 Control pkt"},
        {0xa1, "[ST 299-2] Audio data in HANC space (3G) - Group 7 Control pkt"},
        {0xa2, "[ST 299-2] Audio data in HANC space (3G) - Group 6 Control pkt"},
        {0xa3, "[ST 299-2] Audio data in HANC space (3G) - Group 5 Control pkt"},
        {0xa4, "[ST 299-2] Audio data in HANC space (3G) - Group 8"},
        {0xa5, "[ST 299-2] Audio data in HANC space (3G) - Group 7"},
        {0xa6, "[ST 299-2] Audio data in HANC space (3G) - Group 6"},
        {0xa7, "[ST 299-2] Audio data in HANC space (3G) - Group 5"},
        {0xe0, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xe1, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xe2, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xe3, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xe4, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xe5, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xe6, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xe7, "[ST 299-1] Audio data in HANC space (HDTV)"},
        {0xec, "[S272] Audio Data in HANC space (SDTV)"},
        {0xed, "[S272] Audio Data in HANC space (SDTV)"},
        {0xee, "[S272] Audio Data in HANC space (SDTV)"},
        {0xef, "[S272] Audio Data in HANC space (SDTV)"},
        {0xf0, "[S315] Camera position (HANC or VANC space)"},
        {0xf4, "[RP165] Error Detection and Handling (HANC space)"},
        {0xf8, "[S272] Audio Data in HANC space (SDTV)"},
        {0xf9, "[S272] Audio Data in HANC space (SDTV)"},
        {0xfa, "[S272] Audio Data in HANC space (SDTV)"},
        {0xfb, "[S272] Audio Data in HANC space (SDTV)"},
        {0xfc, "[S272] Audio Data in HANC space (SDTV)"},
        {0xfd, "[S272] Audio Data in HANC space (SDTV)"},
        {0xfe, "[S272] Audio Data in HANC space (SDTV)"},
        {0xff, "[S272] Audio Data in HANC space (SDTV)"}
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(did_type1_table); i++) {
        if (did_type1_table[i].did == did)
            return did_type1_table[i].application;
    }

    return 0;
}

static const char* get_did_type2_string(uint8_t did, uint8_t sdid)
{
    static const struct
    {
        uint8_t did;
        uint8_t sdid;
        const char *application;
    } did_type2_table[] =
    {
        {0x08, 0x08, "[S353] MPEG recoding data, VANC space"},
        {0x08, 0x0c, "[S353] MPEG recoding data, HANC space"},
        {0x40, 0x01, "[S305] SDTI transport in active frame space"},
        {0x40, 0x02, "[S348] HD-SDTI transport in active frame space"},
        {0x40, 0x04, "[S427] Link Encryption Message 1"},
        {0x40, 0x05, "[S427] Link Encryption Message 2"},
        {0x40, 0x06, "[S427] Link Encryption Metadata"},
        {0x41, 0x01, "[S352] Payload Identification , HANC space"},
        {0x41, 0x05, "[S2016-3] AFD and Bar Data"},
        {0x41, 0x06, "[S2016-4] Pan-Scan Data"},
        {0x41, 0x07, "[S2010] ANSI/SCTE 104 messages"},
        {0x41, 0x08, "[S2031] DVB/SCTE VBI data"},
        {0x41, 0x09, "[ST 2056 (pending approval)] MPEG TS packets in VANC"},
        {0x41, 0x0a, "[ST 2068 (Pending Approval)] Stereoscopic 3D Frame Compatible Packing and Signaling"},
        {0x43, 0x01, "[ITU-R BT.1685] Structure of inter-station control data conveyed by ancillary data packets"},
        {0x43, 0x02, "[RDD 8] (OP-47) Subtitling Distribution packet (SDP)"},
        {0x43, 0x03, "[RDD 8] (OP-47) Transport of ANC packet in an ANC Multipacket"},
        {0x43, 0x04, "[ARIB TR-B29] Metadata to monitor errors of audio and video signals on a broadcasting chain ARIB http://www.arib.or.jp/english/html/overview/archives/br/8-TR-B29v1_0-E1.pdf"},
        {0x43, 0x05, "[RDD18] Acquisition Metadata Sets for Video Camera Parameters"},
        {0x44, 0x04, "[RP214] KLV Metadata transport in VANC space"},
        {0x44, 0x14, "[RP214] KLV Metadata transport in HANC space"},
        {0x44, 0x44, "[RP223] Packing UMID and Program Identification Label Data into SMPTE 291M Ancillary Data Packets"},
        {0x45, 0x01, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x02, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x03, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x04, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x05, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x06, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x07, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x08, "[S2020-1] Compressed Audio Metadata"},
        {0x45, 0x09, "[S2020-1] Compressed Audio Metadata"},
        {0x46, 0x01, "[ST 2051] Two Frame Marker in HANC"},
        {0x50, 0x01, "[RDD 8] (OP-47) WSS data per RDD 8"},
        {0x51, 0x01, "[RP215] Film Codes in VANC space"},
        {0x60, 0x60, "[S12M-2] Ancillary Time Code"},
        {0x61, 0x01, "[S334-1] EIA 708B Data mapping into VANC space"},
        {0x61, 0x02, "[S334-1] EIA 608 Data mapping into VANC space"},
        {0x62, 0x01, "[RP207] Program Description in VANC space"},
        {0x62, 0x02, "[S334-1] Data broadcast (DTV) in VANC space"},
        {0x62, 0x03, "[RP208] VBI Data in VANC space"},
        {0x64, 0x64, "[RP196 (Withdrawn)] Time Code in HANC space (Deprecated; for reference only)"},
        {0x64, 0x7f, "[RP196 (Withdrawn)] VITC in HANC space (Deprecated; for reference only)"},
    };

    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(did_type2_table); i++) {
        if (did_type2_table[i].did == did && did_type2_table[i].sdid == sdid)
            return did_type2_table[i].application;
    }

    return 0;
}

static void write_op_label(AppInfoWriter *info_writer, const string &name, mxfUL op_label)
{
    static const mxfUL op_label_prefix = MXF_OP_L_LABEL(0, 0, 0, 0);

    char label_str[32];
    bmx_snprintf(label_str, sizeof(label_str), "%s", "Unknown");

    info_writer->StartAnnotations();
    if (memcmp(&op_label,        &op_label_prefix,        7) == 0 &&
        memcmp(&op_label.octet8, &op_label_prefix.octet8, 4) == 0)
    {
        if (op_label.octet12 == 0x10)
        {
            // OP-Atom
            bmx_snprintf(label_str, sizeof(label_str), "%s", "OPAtom");
            if (op_label.octet13 <= 0x03) {
                info_writer->WriteBoolItem("multi_source_clip",   (op_label.octet13 & 0x01) != 0);
                info_writer->WriteBoolItem("multi_essence_track", (op_label.octet13 & 0x02) != 0);
            }
        }
        else if (op_label.octet12 >= 0x01  && op_label.octet12 <= 0x03 &&
                 op_label.octet13 >= 0x01  && op_label.octet13 <= 0x03)
        {
            // OP-1A ... OP-3C
            bmx_snprintf(label_str, sizeof(label_str), "OP%c%c", '1' + op_label.octet12 - 1, 'A' + op_label.octet13 - 1);
            if ((op_label.octet14 & 0x01) && op_label.octet14 <= 0x0f &&
                 op_label.octet15 == 0x00)
            {
                info_writer->WriteBoolItem("external_essence", (op_label.octet14 & 0x02) != 0);
                info_writer->WriteBoolItem("non_stream",       (op_label.octet14 & 0x04) != 0);
                info_writer->WriteBoolItem("multi_track",      (op_label.octet14 & 0x08) != 0);
            }
        }
    }
    info_writer->WriteAUIDItem("label", op_label);
    info_writer->EndAnnotations();

    info_writer->WriteStringItem(name, label_str);
}

static void write_track_package_info(AppInfoWriter *info_writer, MXFTrackReader *track_reader, size_t index)
{
    MXFSequenceTrackReader *sequence_track = dynamic_cast<MXFSequenceTrackReader*>(track_reader);
    if (sequence_track) {
        info_writer->StartArrayItem("packages", sequence_track->GetNumSegments());
        size_t i;
        for (i = 0; i < sequence_track->GetNumSegments(); i++)
            write_track_package_info(info_writer, sequence_track->GetSegment(i), i);
        info_writer->EndArrayItem();
    } else {
        const MXFTrackInfo *track_info = track_reader->GetTrackInfo();
        MXFFileTrackReader *file_track = dynamic_cast<MXFFileTrackReader*>(track_reader);
        BMX_ASSERT(file_track);

        if (index == (size_t)(-1)) {
            info_writer->StartArrayItem("packages", 1);
            info_writer->StartArrayElement("package", 0);
        } else {
            info_writer->StartArrayElement("package", index);
        }

        info_writer->StartSection("material");
        info_writer->WriteUMIDItem("package_uid", track_info->material_package_uid);
        info_writer->WriteIntegerItem("track_id", track_info->material_track_id);
        info_writer->WriteIntegerItem("track_number", track_info->material_track_number);
        info_writer->EndSection();

        info_writer->StartSection("file_source");
        info_writer->WriteUMIDItem("package_uid", track_info->file_package_uid);
        info_writer->WriteIntegerItem("track_id", track_info->file_track_id);
        info_writer->WriteIntegerItem("track_number", track_info->file_track_number, true);
        if (BMX_REGRESSION_TEST)
            info_writer->WriteStringItem("file_uri", "regtest_file_uri");
        else
            info_writer->WriteStringItem("file_uri", file_track->GetFileReader()->GetAbsoluteURI().ToString());
        info_writer->EndSection();

        info_writer->EndArrayElement();
        if (index == (size_t)(-1))
            info_writer->EndArrayItem();
    }
}

static void write_crc32_check_data(AppInfoWriter *info_writer, const CRC32Data *crc32_data)
{
    CRC32CheckResult result = CRC32_PASSED;
    if (crc32_data->error_count != 0)
        result = CRC32_FAILED;
    else if (crc32_data->total_read > crc32_data->check_count)
        result = CRC32_MISSING_DATA;

    info_writer->WriteEnumStringItem("result", CRC32_CHECK_RESULT_EINFO, result);
    info_writer->WriteIntegerItem("error_count", crc32_data->error_count);
    info_writer->WriteIntegerItem("check_count", crc32_data->check_count);
    info_writer->WriteIntegerItem("total_read", crc32_data->total_read);
}

static void write_mca_label_info(AppInfoWriter *info_writer, MCALabelSubDescriptor *label)
{
    info_writer->WriteIDAUItem("link_id", label->getMCALinkID());
    info_writer->WriteAUIDItem("dict_id", label->getMCALabelDictionaryID());
    info_writer->WriteStringItem("tag_symbol", label->getMCATagSymbol());
    if (label->haveMCATagName())
        info_writer->WriteStringItem("tag_name", label->getMCATagName());
    if (label->haveRFC5646SpokenLanguage())
        info_writer->WriteStringItem("language", label->getRFC5646SpokenLanguage());
}

static void write_track_mca_label_info(AppInfoWriter *info_writer, MXFReader *reader,
                                       const MXFSoundTrackInfo *sound_info, bool mca_detail)
{
    const MXFMCALabelIndex *mca_label_index = reader->GetMCALabelIndex();
    map<uint32_t, vector<AudioChannelLabelSubDescriptor*> > c_labels;
    map<UUID, SoundfieldGroupLabelSubDescriptor*> sg_labels;
    map<UUID, GroupOfSoundfieldGroupsLabelSubDescriptor*> gosg_labels;
    size_t i;
    for (i = 0; i < sound_info->mca_labels.size(); i++) {
        AudioChannelLabelSubDescriptor *c_label = sound_info->mca_labels[i];
        if (c_label->haveMCAChannelID()) {
            c_labels[c_label->getMCAChannelID()].push_back(c_label);
        } else {
            BMX_CHECK(sound_info->channel_count == 1);
            c_labels[1].push_back(c_label);
        }

        if (c_label->haveSoundfieldGroupLinkID()) {
            UUID link_id = c_label->getSoundfieldGroupLinkID();
            MCALabelSubDescriptor *label = mca_label_index->FindLabel(link_id);
            SoundfieldGroupLabelSubDescriptor *sg_label = dynamic_cast<SoundfieldGroupLabelSubDescriptor*>(label);
            BMX_CHECK(sg_label);
            sg_labels[link_id] = sg_label;

            if (sg_label->haveGroupOfSoundfieldGroupsLinkID()) {
                vector<UUID> link_ids = sg_label->getGroupOfSoundfieldGroupsLinkID();
                size_t k;
                for (k = 0; k < link_ids.size(); k++) {
                    UUID &link_id = link_ids[k];
                    MCALabelSubDescriptor *label = mca_label_index->FindLabel(link_id);
                    GroupOfSoundfieldGroupsLabelSubDescriptor *gosg_label = dynamic_cast<GroupOfSoundfieldGroupsLabelSubDescriptor*>(label);
                    BMX_CHECK(gosg_label);
                    gosg_labels[link_id] = gosg_label;
                }
            }
        }
    }

    string c_summary;
    uint32_t c;
    for (c = 0; c < sound_info->channel_count; c++) {
        uint32_t channel_id = c + 1;
        if (c > 0)
            c_summary.append("; ");
        if (c_labels.count(channel_id)) {
            vector<AudioChannelLabelSubDescriptor*> &labels = c_labels[channel_id];
            size_t l;
            for (l = 0; l < labels.size(); l++) {
                AudioChannelLabelSubDescriptor *c_label = labels[l];
                if (l > 0)
                    c_summary.append(",");
                c_summary.append(c_label->getMCATagSymbol());
            }
        } else {
            c_summary.append("_");
        }
    }
    info_writer->WriteStringItem("channel_summary", c_summary);

    if (!sg_labels.empty()) {
        string sg_summary;
        map<UUID, SoundfieldGroupLabelSubDescriptor*>::iterator sg_iter;
        for (sg_iter = sg_labels.begin(); sg_iter != sg_labels.end(); sg_iter++) {
            SoundfieldGroupLabelSubDescriptor *sg_label = sg_iter->second;
            if (sg_iter != sg_labels.begin())
                sg_summary.append("; ");
            sg_summary.append(sg_label->getMCATagSymbol());
        }
        info_writer->WriteStringItem("sg_summary", sg_summary);
    }

    if (!gosg_labels.empty()) {
        string gosg_summary;
        map<UUID, GroupOfSoundfieldGroupsLabelSubDescriptor*>::iterator gosg_iter;
        for (gosg_iter = gosg_labels.begin(); gosg_iter != gosg_labels.end(); gosg_iter++) {
            GroupOfSoundfieldGroupsLabelSubDescriptor *gosg_label = gosg_iter->second;
            if (gosg_iter != gosg_labels.begin())
                gosg_summary.append("; ");
            gosg_summary.append(gosg_label->getMCATagSymbol());
        }
        info_writer->WriteStringItem("gosg_summary", gosg_summary);
    }


    if (mca_detail) {
        info_writer->StartArrayItem("channels", sound_info->channel_count);
        uint32_t c;
        for (c = 0; c < sound_info->channel_count; c++) {
            uint32_t channel_id = c + 1;
            info_writer->StartArrayElement("channel", c);
            info_writer->WriteIntegerItem("index", c);
            info_writer->WriteIntegerItem("id", channel_id);
            if (c_labels.count(channel_id)) {
                vector<AudioChannelLabelSubDescriptor*> &labels = c_labels[channel_id];
                info_writer->StartArrayItem("labels", labels.size());
                size_t l;
                for (l = 0; l < labels.size(); l++) {
                    AudioChannelLabelSubDescriptor *c_label = labels[l];
                    info_writer->StartArrayElement("channel_label", l);
                    write_mca_label_info(info_writer, c_label);
                    if (c_label->haveSoundfieldGroupLinkID()) {
                        UUID link_id = c_label->getSoundfieldGroupLinkID();
                        SoundfieldGroupLabelSubDescriptor *sg_label = sg_labels.at(link_id);
                        info_writer->StartAnnotations();
                        info_writer->WriteStringItem("tag_symbol", sg_label->getMCATagSymbol());
                        info_writer->EndAnnotations();
                        info_writer->WriteIDAUItem("sg_link_id", link_id);
                    }
                    info_writer->EndArrayElement();
                }
                info_writer->EndArrayItem();
            }
            info_writer->EndArrayElement();
        }
        info_writer->EndArrayItem();

        if (!sg_labels.empty()) {
            info_writer->StartArrayItem("soundfield_groups", sg_labels.size());
            size_t index;
            map<UUID, SoundfieldGroupLabelSubDescriptor*>::iterator iter;
            for (iter = sg_labels.begin(), index = 0; iter != sg_labels.end(); iter++, index++) {
                SoundfieldGroupLabelSubDescriptor *sg_label = iter->second;
                info_writer->StartArrayElement("soundfield_group", index);
                write_mca_label_info(info_writer, sg_label);
                if (sg_label->haveGroupOfSoundfieldGroupsLinkID()) {
                    vector<UUID> gosg_link_ids = sg_label->getGroupOfSoundfieldGroupsLinkID();
                    info_writer->StartArrayItem("gosg_link_ids", gosg_link_ids.size());
                    size_t l;
                    for (l = 0; l < gosg_link_ids.size(); l++) {
                        UUID &link_id = gosg_link_ids[l];
                        GroupOfSoundfieldGroupsLabelSubDescriptor *gosg_label = gosg_labels.at(link_id);
                        info_writer->StartArrayElement("gosg_link_id", l);
                        info_writer->StartAnnotations();
                        info_writer->WriteStringItem("tag_symbol", gosg_label->getMCATagSymbol());
                        info_writer->EndAnnotations();
                        info_writer->WriteIDAUItem("link_id", link_id);
                        info_writer->EndArrayElement();
                    }
                    info_writer->EndArrayItem();
                }
                info_writer->EndArrayElement();
            }
            info_writer->EndArrayItem();
        }

        if (!gosg_labels.empty()) {
            info_writer->StartArrayItem("group_of_soundfield_groups", gosg_labels.size());
            size_t index;
            map<UUID, GroupOfSoundfieldGroupsLabelSubDescriptor*>::iterator iter;
            for (iter = gosg_labels.begin(), index = 0; iter != gosg_labels.end(); iter++, index++) {
                GroupOfSoundfieldGroupsLabelSubDescriptor *gosg_label = iter->second;
                info_writer->StartArrayElement("group_of_soundfield_group", index);
                write_mca_label_info(info_writer, gosg_label);
                info_writer->EndArrayElement();
            }
            info_writer->EndArrayItem();
        }
    }
}

static void write_track_info(AppInfoWriter *info_writer, MXFReader *reader, MXFTrackReader *track_reader,
                             const vector<Checksum> *checksums, const CRC32Data *crc32_data, bool mca_detail)
{
    const MXFTrackInfo *track_info = track_reader->GetTrackInfo();
    const MXFPictureTrackInfo *picture_info = dynamic_cast<const MXFPictureTrackInfo*>(track_info);
    const MXFSoundTrackInfo *sound_info = dynamic_cast<const MXFSoundTrackInfo*>(track_info);
    const MXFDataTrackInfo *data_info = dynamic_cast<const MXFDataTrackInfo*>(track_info);

    int16_t precharge = 0;
    int16_t rollout = 0;
    if (reader->IsComplete() && track_reader->GetDuration() > 0) {
        precharge = track_reader->GetPrecharge(0, true);
        rollout = track_reader->GetRollout(track_reader->GetDuration() - 1, true);
    }

    info_writer->WriteEnumStringItem("essence_kind", ESSENCE_KIND_EINFO, track_info->data_def);
    info_writer->WriteStringItem("essence_type", essence_type_to_enum_string(track_info->essence_type));
    info_writer->WriteAUIDItem("ec_label", track_info->essence_container_label);
    info_writer->WriteRationalItem("edit_rate", track_info->edit_rate);
    info_writer->WriteDurationItem("duration", track_info->duration, track_info->edit_rate);
    if (track_info->lead_filler_offset != 0)
        info_writer->WritePositionItem("lead_filler_offset", track_info->lead_filler_offset, track_info->edit_rate);
    if (data_info && data_info->timed_text_manifest && data_info->timed_text_manifest->mStart != 0) {
        info_writer->WritePositionItem("timed_text_offset", data_info->timed_text_manifest->mStart,
                                       track_info->edit_rate);
    }
    if (precharge != 0)
        info_writer->WriteIntegerItem("precharge", precharge);
    if (rollout != 0)
        info_writer->WriteIntegerItem("rollout", rollout);
    if (checksums) {
        size_t i;
        for (i = 0; i < checksums->size(); i++) {
            info_writer->StartAnnotations();
            info_writer->WriteStringItem("type", get_checksum_type_str((*checksums)[i].GetType()));
            info_writer->EndAnnotations();
            info_writer->WriteStringItem("checksum", (*checksums)[i].GetDigestString());
        }
    }
    if (crc32_data) {
        info_writer->StartSection("crc32_check");
        write_crc32_check_data(info_writer, crc32_data);
        info_writer->EndSection();
    }

    write_track_package_info(info_writer, track_reader, (size_t)(-1));

    if (picture_info) {
        info_writer->StartSection("picture_descriptor");
        if (picture_info->picture_essence_coding_label != g_Null_UL)
            info_writer->WriteAUIDItem("coding_label", picture_info->picture_essence_coding_label);
        info_writer->WriteEnumItem("signal_standard", SIGNAL_STANDARD_EINFO, picture_info->signal_standard);
        info_writer->WriteEnumItem("frame_layout", FRAME_LAYOUT_EINFO, picture_info->frame_layout);
        info_writer->WriteIntegerItem("stored_width", picture_info->stored_width);
        info_writer->WriteIntegerItem("stored_height", picture_info->stored_height);
        info_writer->WriteIntegerItem("display_width", picture_info->display_width);
        info_writer->WriteIntegerItem("display_height", picture_info->display_height);
        if (BMX_OPT_PROP_IS_SET(picture_info->display_x_offset))
            info_writer->WriteIntegerItem("display_x_offset", picture_info->display_x_offset);
        if (BMX_OPT_PROP_IS_SET(picture_info->display_y_offset))
            info_writer->WriteIntegerItem("display_y_offset", picture_info->display_y_offset);
        info_writer->WriteRationalItem("aspect_ratio", picture_info->aspect_ratio);
        if (picture_info->afd)
            info_writer->WriteIntegerItem("afd", picture_info->afd);
        if (track_info->essence_type == AVCI200_1080I ||
            track_info->essence_type == AVCI200_1080P ||
            track_info->essence_type == AVCI200_720P ||
            track_info->essence_type == AVCI100_1080I ||
            track_info->essence_type == AVCI100_1080P ||
            track_info->essence_type == AVCI100_720P ||
            track_info->essence_type == AVCI50_1080I ||
            track_info->essence_type == AVCI50_1080P ||
            track_info->essence_type == AVCI50_720P)
        {
            info_writer->WriteBoolItem("avci_header", picture_info->have_avci_header);
        }
        if (picture_info->is_cdci) {
            info_writer->StartSection("cdci_descriptor");
            info_writer->WriteIntegerItem("component_depth", picture_info->component_depth);
            info_writer->WriteIntegerItem("horiz_subsamp", picture_info->horiz_subsampling);
            info_writer->WriteIntegerItem("vert_subsamp", picture_info->vert_subsampling);
            info_writer->WriteEnumItem("color_siting", COLOR_SITING_EINFO, picture_info->color_siting);
            info_writer->EndSection();
        }
        info_writer->EndSection();
    } else if (sound_info) {
        AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);
        if (text_writer)
            text_writer->PushItemValueIndent(strlen("d10_aes3_valid_flags "));

        info_writer->StartSection("sound_descriptor");
        info_writer->WriteRationalItem("sampling_rate", sound_info->sampling_rate);
        info_writer->WriteIntegerItem("bits_per_sample", sound_info->bits_per_sample);
        info_writer->WriteIntegerItem("block_align", sound_info->block_align);
        info_writer->WriteIntegerItem("channel_count", sound_info->channel_count);
        if (track_info->essence_type == D10_AES3_PCM) {
            info_writer->WriteStringItem("d10_aes3_valid_flags",
                                         get_d10_sound_flags(sound_info->d10_aes3_valid_flags));
        }
        if (sound_info->sequence_offset != 0)
            info_writer->WriteIntegerItem("sequence_offset", sound_info->sequence_offset);
        if (BMX_OPT_PROP_IS_SET(sound_info->locked))
            info_writer->WriteBoolItem("locked", sound_info->locked);
        if (BMX_OPT_PROP_IS_SET(sound_info->audio_ref_level))
            info_writer->WriteIntegerItem("audio_ref_level", sound_info->audio_ref_level);
        if (BMX_OPT_PROP_IS_SET(sound_info->dial_norm))
            info_writer->WriteIntegerItem("dial_norm", sound_info->dial_norm);
        if (sound_info->channel_assignment != g_Null_UL)
            info_writer->WriteAUIDItem("channel_assignment", sound_info->channel_assignment);
        if (!sound_info->mca_labels.empty()) {
            info_writer->StartSection("mca_labels");
            write_track_mca_label_info(info_writer, reader, sound_info, mca_detail);
            info_writer->EndSection();
        }
        info_writer->EndSection();

        if (text_writer)
            text_writer->PopItemValueIndent();
    } else if (data_info) {
        info_writer->StartSection("data_descriptor");
        if (!data_info->vbi_manifest.empty()) {
            info_writer->StartSection("vbi_descriptor");
            info_writer->StartArrayItem("manifest", data_info->vbi_manifest.size());
            size_t i;
            for (i = 0; i < data_info->vbi_manifest.size(); i++) {
                info_writer->StartArrayElement("element", i);
                info_writer->WriteIntegerItem("line_number", data_info->vbi_manifest[i].line_number);
                info_writer->WriteEnumItem("wrapping_type", VBI_WRAPPING_TYPE_EINFO,
                                           data_info->vbi_manifest[i].wrapping_type);
                info_writer->WriteEnumItem("sample_coding", VBI_SAMPLE_CODING_EINFO,
                                           data_info->vbi_manifest[i].sample_coding);
                info_writer->EndArrayElement();
            }
            info_writer->EndArrayItem();
            info_writer->EndSection();
        } else if (!data_info->anc_manifest.empty()) {
            info_writer->StartSection("anc_descriptor");
            info_writer->StartArrayItem("manifest", data_info->anc_manifest.size());
            size_t i;
            for (i = 0; i < data_info->anc_manifest.size(); i++) {
                info_writer->StartArrayElement("element", i);
                info_writer->WriteIntegerItem("line_number", data_info->anc_manifest[i].line_number);
                info_writer->WriteEnumItem("wrapping_type", ANC_WRAPPING_TYPE_EINFO,
                                           data_info->anc_manifest[i].wrapping_type);
                info_writer->WriteEnumItem("sample_coding", ANC_SAMPLE_CODING_EINFO,
                                           data_info->anc_manifest[i].sample_coding);
                if (data_info->anc_manifest[i].did) {
                    if ((data_info->anc_manifest[i].did & 0x80)) {
                        const char *description = get_did_type1_string(data_info->anc_manifest[i].did);
                        info_writer->StartSection("did_type_1");
                        info_writer->WriteIntegerItem("did", data_info->anc_manifest[i].did, true);
                        if (description)
                            info_writer->WriteStringItem("description", description);
                        info_writer->EndSection();
                    } else {
                        const char *description = get_did_type2_string(data_info->anc_manifest[i].did,
                                                                       data_info->anc_manifest[i].sdid);
                        info_writer->StartSection("did_type_2");
                        info_writer->WriteIntegerItem("did", data_info->anc_manifest[i].did, true);
                        info_writer->WriteIntegerItem("sdid", data_info->anc_manifest[i].sdid, true);
                        if (description)
                            info_writer->WriteStringItem("description", description);
                        info_writer->EndSection();
                    }
                }
                info_writer->EndArrayElement();
            }
            info_writer->EndArrayItem();
            info_writer->EndSection();
        } else if (data_info->timed_text_manifest) {
            info_writer->StartSection("timed_text_descriptor");
            info_writer->WriteStringItem("profile", data_info->timed_text_manifest->GetProfileDesignator());
            info_writer->WriteStringItem("encoding", data_info->timed_text_manifest->GetEncoding());
            if (data_info->timed_text_manifest->HaveResourceId()) {
                info_writer->WriteIDAUItem("resource_id", data_info->timed_text_manifest->GetResourceId());
            }
            if (data_info->timed_text_manifest->HaveLanguages()) {
                info_writer->WriteStringItem("languages", data_info->timed_text_manifest->GetLanguagesString());
            }
            if (!data_info->timed_text_manifest->GetAncillaryResources().empty()) {
                const vector<TimedTextAncillaryResource> &anc_resources =
                      data_info->timed_text_manifest->GetAncillaryResources();
                info_writer->StartArrayItem("ancillary_resources", anc_resources.size());
                size_t i;
                for (i = 0; i < anc_resources.size(); i++) {
                    info_writer->StartArrayElement("element", i);
                    info_writer->WriteIDAUItem("resource_id", anc_resources[i].resource_id);
                    info_writer->WriteStringItem("mime_type", anc_resources[i].mime_type);
                    info_writer->WriteIntegerItem("stream_id", anc_resources[i].stream_id);
                    info_writer->EndArrayElement();
                }
                info_writer->EndArrayItem();
            }
            info_writer->EndSection();
        }
        info_writer->EndSection();
    }
}

static void write_clip_info(AppInfoWriter *info_writer, MXFReader *reader,
                            const vector<vector<Checksum> > &track_checksums,
                            const vector<CRC32Data> &track_crc32_data,
                            bool mca_detail)
{
    Rational edit_rate = reader->GetEditRate();
    bool have_track_checksums = (track_checksums.size() == reader->GetNumTrackReaders());
    bool have_track_crc32_data = (track_crc32_data.size() == reader->GetNumTrackReaders());

    int16_t max_precharge = 0;
    int16_t max_rollout = 0;
    if (reader->IsComplete() && reader->GetDuration() > 0) {
        max_precharge = reader->GetMaxPrecharge(0, false);
        max_rollout = reader->GetMaxRollout(reader->GetDuration() - 1, false);
    }

    string clip_name = reader->GetMaterialPackageName();
    if (!clip_name.empty())
      info_writer->WriteStringItem("name", clip_name);

    info_writer->WriteRationalItem("edit_rate", edit_rate);
    info_writer->WriteDurationItem("duration", reader->GetDuration(), edit_rate);
    if (max_precharge != 0)
        info_writer->WriteIntegerItem("max_precharge", max_precharge);
    if (max_rollout != 0)
        info_writer->WriteIntegerItem("max_rollout", max_rollout);

    info_writer->StartSection("start_timecodes");
    if (reader->HaveMaterialTimecode()) {

        int64_t lead_filler_offset = reader->GetFixedLeadFillerOffset();
        info_writer->WriteTimecodeItem("material", reader->GetMaterialTimecode(lead_filler_offset));
        if (lead_filler_offset != 0) {
            info_writer->WriteTimecodeItem("material_origin",
                                           reader->GetMaterialTimecode(0));
        }
    }
    if (reader->HaveFileSourceTimecode())
        info_writer->WriteTimecodeItem("file_source", reader->GetFileSourceTimecode(0));
    if (reader->HavePhysicalSourceTimecode()) {
        if (!reader->GetPhysicalSourcePackageName().empty()) {
            info_writer->StartAnnotations();
            info_writer->WriteStringItem("name", reader->GetPhysicalSourcePackageName());
            info_writer->EndAnnotations();
        }
        info_writer->WriteTimecodeItem("physical_source", reader->GetPhysicalSourceTimecode(0));
    }
    info_writer->EndSection();

    if (reader->GetNumTrackReaders() > 0) {
        info_writer->StartArrayItem("tracks", reader->GetNumTrackReaders());
        uint32_t i;
        for (i = 0; i < reader->GetNumTrackReaders(); i++) {
            info_writer->StartArrayElement("track", i);
            write_track_info(info_writer, reader, reader->GetTrackReader(i),
                             (have_track_checksums ? &track_checksums[i] : 0),
                             (have_track_crc32_data ? &track_crc32_data[i] : 0),
                             mca_detail);
            info_writer->EndArrayElement();
        }
        info_writer->EndArrayItem();
    }
}

static void write_identification_info(AppInfoWriter *info_writer, Identification *identification)
{
    info_writer->WriteIDAUItem("generation_uid", identification->getThisGenerationUID());
    info_writer->WriteStringItem("company_name", identification->getCompanyName());
    info_writer->WriteStringItem("product_name", identification->getProductName());
    if (identification->haveProductVersion())
        info_writer->WriteProductVersionItem("product_version", identification->getProductVersion());
    info_writer->WriteStringItem("version_string", identification->getVersionString());
    info_writer->WriteIDAUItem("product_uid", identification->getProductUID());
    info_writer->WriteTimestampItem("modified_date", identification->getModificationDate());
    if (identification->haveToolkitVersion())
        info_writer->WriteProductVersionItem("toolkit_version", identification->getToolkitVersion());
    if (identification->havePlatform())
        info_writer->WriteStringItem("platform", identification->getPlatform());
}

static void write_package_info(AppInfoWriter *info_writer, GenericPackage *package)
{
    info_writer->WriteUMIDItem("package_uid", package->getPackageUID());
    if (package->haveName())
        info_writer->WriteStringItem("name", package->getName());
    info_writer->WriteTimestampItem("creation_date", package->getPackageCreationDate());
    info_writer->WriteTimestampItem("modified_date", package->getPackageModifiedDate());
}

static void write_file_checksum_info(AppInfoWriter *info_writer, MXFFileReader *file_reader,
                                     AppMXFFileFactory *file_factory)
{
    URI abs_uri = file_reader->GetAbsoluteURI();

    size_t i;
    for (i = 0; i < file_factory->GetNumInputChecksumFiles(); i++) {
        if (abs_uri == file_factory->GetInputChecksumAbsURI(i)) {
            vector<ChecksumType> checksum_types = file_factory->GetInputChecksumTypes(i);
            size_t j;
            for (j = 0; j < checksum_types.size(); j++) {
                info_writer->StartAnnotations();
                info_writer->WriteStringItem("type", get_checksum_type_str(checksum_types[j]));
                info_writer->EndAnnotations();
                info_writer->WriteStringItem("checksum",
                                             file_factory->GetInputChecksumDigestString(i, checksum_types[j]).c_str());
            }
            break;
        }
    }
}

static void write_file_info(AppInfoWriter *info_writer, MXFFileReader *file_reader, AppMXFFileFactory *file_factory)
{
    if (BMX_REGRESSION_TEST) {
        info_writer->WriteStringItem("file_uri", "regtest_file_uri");
    } else {
        info_writer->WriteStringItem("file_uri", file_reader->GetAbsoluteURI().ToString());
        write_file_checksum_info(info_writer, file_reader, file_factory);
    }

    info_writer->WriteVersionTypeItem("mxf_version", file_reader->GetMXFVersion());
    write_op_label(info_writer, "op_label", file_reader->GetOPLabel());
    if (file_reader->HaveInternalEssence())
        info_writer->WriteBoolItem("frame_wrapped", file_reader->IsFrameWrapped());

    vector<Identification*> identifications = file_reader->GetHeaderMetadata()->getPreface()->getIdentifications();
    info_writer->StartArrayItem("identifications", identifications.size());
    size_t i;
    for (i = 0; i < identifications.size(); i++) {
        info_writer->StartArrayElement("identification", i);
        write_identification_info(info_writer, identifications[i]);
        info_writer->EndArrayElement();
    }
    info_writer->EndArrayItem();

    vector<GenericPackage*> packages = file_reader->GetHeaderMetadata()->getPreface()->getContentStorage()->getPackages();

    size_t mp_count = 0;
    size_t fsp_count = 0;
    size_t psp_count = 0;
    for (i = 0; i < packages.size(); i++) {
        MaterialPackage *mp = dynamic_cast<MaterialPackage*>(packages[i]);
        SourcePackage *sp = dynamic_cast<SourcePackage*>(packages[i]);
        if (mp) {
            mp_count++;
        } else if (sp && sp->haveDescriptor()) {
            GenericDescriptor *descriptor = sp->getDescriptorLight();
            if (dynamic_cast<FileDescriptor*>(descriptor))
                fsp_count++;
            else if (descriptor && mxf_set_is_subclass_of(descriptor->getCMetadataSet(), &MXF_SET_K(PhysicalDescriptor)))
                psp_count++;
        }
    }

    if (mp_count > 0) {
        info_writer->StartArrayItem("material_packages", mp_count);
        size_t index = 0;
        for (i = 0; i < packages.size(); i++) {
            if (dynamic_cast<MaterialPackage*>(packages[i])) {
                info_writer->StartArrayElement("material_package", index);
                write_package_info(info_writer, packages[i]);
                info_writer->EndArrayElement();
                index++;
            }
        }
        info_writer->EndArrayItem();
    }

    if (fsp_count > 0) {
        info_writer->StartArrayItem("file_source_packages", fsp_count);
        size_t index = 0;
        for (i = 0; i < packages.size(); i++) {
            SourcePackage *sp = dynamic_cast<SourcePackage*>(packages[i]);
            if (sp && sp->haveDescriptor()) {
                GenericDescriptor *descriptor = sp->getDescriptorLight();
                if (dynamic_cast<FileDescriptor*>(descriptor)) {
                    info_writer->StartArrayElement("file_source_package", index);
                    write_package_info(info_writer, packages[i]);
                    info_writer->EndArrayElement();
                    index++;
                }
            }
        }
        info_writer->EndArrayItem();
    }

    if (psp_count > 0) {
        info_writer->StartArrayItem("physical_source_packages", psp_count);
        size_t index = 0;
        for (i = 0; i < packages.size(); i++) {
            SourcePackage *sp = dynamic_cast<SourcePackage*>(packages[i]);
            if (sp && sp->haveDescriptor()) {
                GenericDescriptor *descriptor = sp->getDescriptorLight();
                if (descriptor && mxf_set_is_subclass_of(descriptor->getCMetadataSet(), &MXF_SET_K(PhysicalDescriptor))) {
                    info_writer->StartArrayElement("physical_source_package", index);
                    write_package_info(info_writer, packages[i]);
                    info_writer->EndArrayElement();
                    index++;
                }
            }
        }
        info_writer->EndArrayItem();
    }
}

static void write_text_object_info(AppInfoWriter *info_writer, MXFTextObject *text_object)
{
    info_writer->WriteAUIDItem("scheme_id", text_object->GetSchemeId());
    info_writer->WriteStringItem("mime_type", text_object->GetMimeType());
    info_writer->WriteStringItem("lang_code", text_object->GetLanguageCode());
    info_writer->WriteStringItem("description", text_object->GetTextDataDescription());
    switch (text_object->GetEncoding())
    {
        case UTF8:
            info_writer->WriteStringItem("encoding", "UTF8");
            break;
        case UTF16:
            info_writer->WriteStringItem("encoding", "UTF16");
            break;
        default:
            break;
    }
    info_writer->WriteBoolItem("generic_stream", text_object->IsInGenericStream());
    info_writer->WriteUMIDItem("package_uid", text_object->GetPackageUID());
    info_writer->WriteIntegerItem("track_id", text_object->GetTrackId());
    info_writer->WriteIntegerItem("component_index", text_object->GetComponentIndex());
}

static void write_application_info(AppInfoWriter *info_writer)
{
    info_writer->WriteStringItem("name", APP_NAME);
    info_writer->WriteStringItem("lib_name", get_bmx_library_name());
    info_writer->WriteStringItem("lib_version", get_bmx_version_string());
    info_writer->WriteTimestampItem("build_timestamp", get_bmx_build_timestamp());
    if (!get_bmx_scm_version_string().empty())
        info_writer->WriteStringItem("scm_version", get_bmx_scm_version_string());
}

static void write_log_messages(AppInfoWriter *info_writer)
{
    static const char *level_names[] = {"debug", "info", "warning", "error"};

    AppTextInfoWriter *text_writer = dynamic_cast<AppTextInfoWriter*>(info_writer);
    if (text_writer)
        text_writer->PushItemValueIndent(strlen("warning "));

    size_t i;
    for (i = 0; i < LOG_DATA.messages.size(); i++) {
        if (!LOG_DATA.messages[i].source.empty()) {
            info_writer->StartAnnotations();
            info_writer->WriteStringItem("source", LOG_DATA.messages[i].source);
            info_writer->EndAnnotations();
        }
        const char *level_name = level_names[0];
        if (LOG_DATA.messages[i].level >= 0 &&
            (size_t)LOG_DATA.messages[i].level <= BMX_ARRAY_SIZE(level_names))
        {
            level_name = level_names[LOG_DATA.messages[i].level];
        }
        info_writer->WriteStringItem(level_name, LOG_DATA.messages[i].message);
    }

    if (text_writer)
        text_writer->PopItemValueIndent();
}

static void write_data(FILE *file, const string &filename, const unsigned char *data, uint32_t size,
                       bool wrap_klv, const mxfKey *key)
{
#define CHECK_WRITE(dt, sz)                                                                                     \
    if (fwrite(dt, 1, sz, file) != sz) {                                                                        \
        log_error("Failed to write to raw file '%s': %s\n", filename.c_str(), bmx_strerror(errno).c_str());     \
        throw false;                                                                                            \
    }

    if (wrap_klv) {
        // write KL with 8-byte Length
        unsigned char len_bytes[8] = {0x87};
        mxf_set_uint32(size, &len_bytes[4]);

        CHECK_WRITE((const unsigned char*)&key->octet0, 16)
        CHECK_WRITE(len_bytes, 8)
    }

    CHECK_WRITE(data, size)
}

static bool update_rdd6_xml(Frame *frame, RDD6MetadataFrame *rdd6_frame, vector<string> *cumulative_desc_chars,
                            vector<bool> *have_start, vector<bool> *have_end, bool *done)
{
    ST436Element st436_element(false);
    st436_element.Parse(frame->GetBytes(), frame->GetSize());

    vector<const ST436Line*> rdd6_lines;
    size_t i;
    for (i = 0; i < st436_element.lines.size(); i++) {
        ANCManifestElement man_element;
        man_element.Parse(&st436_element.lines[i]);
        if (man_element.sample_coding == ANC_8_BIT_COMP_LUMA &&
            man_element.did == 0x45 &&
            (man_element.sdid >= 0x01 && man_element.sdid <= 0x09))
        {
            rdd6_lines.push_back(&st436_element.lines[i]);
        }
    }
    if (rdd6_lines.empty())
        return true;

    RDD6MetadataFrame next_rdd6_frame;
    RDD6MetadataFrame *parse_rdd6_frame;
    if (rdd6_frame->first_sub_frame && rdd6_frame->second_sub_frame) {
        parse_rdd6_frame = &next_rdd6_frame;
    } else {
        if (rdd6_lines.size() == 1) {
            bool is_first_sub_frame;
            if (!rdd6_frame->GetST2020SubFrameIndex(rdd6_lines[0]->payload_data, rdd6_lines[0]->payload_size,
                                                    &is_first_sub_frame))
            {
                log_warn("ST-436 ANC data contains 1 RDD-6 line but could not parse a sub-frame\n");
                return false;
            }
        }
        parse_rdd6_frame = rdd6_frame;
    }

    if (rdd6_lines.size() >= 2) {
        if (rdd6_lines.size() > 2)
            log_warn("ST-436 ANC data contains %" PRIszt " RDD-6 lines; only using the first 2\n", rdd6_lines.size());
        parse_rdd6_frame->ParseST2020(rdd6_lines[0]->payload_data, rdd6_lines[0]->payload_size,
                                      rdd6_lines[1]->payload_data, rdd6_lines[1]->payload_size);
        if (parse_rdd6_frame == rdd6_frame)
            parse_rdd6_frame->BufferPayloads(); // buffer because referenced frame data will de deleted
        if (!parse_rdd6_frame->IsComplete()) {
            log_warn("RDD-6 frame data is incomplete\n");
            return false;
        }
    } else {
        parse_rdd6_frame->ParseST2020(rdd6_lines[0]->payload_data, rdd6_lines[0]->payload_size,
                                      0, 0);
        if (parse_rdd6_frame == rdd6_frame)
            parse_rdd6_frame->BufferPayloads(); // buffer because referenced frame data will de deleted
    }

    if (rdd6_lines.size() == 1 && parse_rdd6_frame->second_sub_frame)
        return true; // description chars are in the first sub frame only

    bool all_done = true;
    vector<uint8_t> desc_chars;
    parse_rdd6_frame->GetDescriptionTextChars(&desc_chars);
    if (cumulative_desc_chars->size() != desc_chars.size()) {
        if (!cumulative_desc_chars->empty()) {
            log_warn("Cannot extract description text chars because RDD-6 program config has changed");
            return false;
        }
        cumulative_desc_chars->resize(desc_chars.size());
        have_start->resize(desc_chars.size(), false);
        have_end->resize(desc_chars.size(), false);
    }
    for (i = 0; i < desc_chars.size(); i++) {
        if (desc_chars[i] && !(*have_end)[i]) {
            (*cumulative_desc_chars)[i].append(1, (char)desc_chars[i]);

            if (!(*have_start)[i] && desc_chars[i] == RDD6_START_TEXT)
                (*have_start)[i] = true;
            else if ((*have_start)[i] && desc_chars[i] == RDD6_END_TEXT)
                (*have_end)[i] = true;
        }
        if (!(*have_end)[i])
            all_done = false;
    }

    *done = all_done;

    return true;
}

static void write_timecodes(MXFReader *reader, Frame *frame, FILE *tc_file)
{
    static const char *null_timecode_str = "__:__:__:__";

    bool have_sys_item_creation_tc = false;
    Timecode sys_item_creation_tc;
    bool have_sys_item_user_tc = false;
    Timecode sys_item_user_tc;
    const SS1TimecodeArray *ss1_timecodes = 0;

    const vector<FrameMetadata*> *metadata = frame->GetMetadata(SDTI_CP_SYSTEM_METADATA_FMETA_ID);
    if (metadata && !metadata->empty()) {
        const SDTICPSystemMetadata *sdticp_meta = dynamic_cast<const SDTICPSystemMetadata*>((*metadata)[0]);

        if (sdticp_meta->mHaveCreationTimecode) {
            sys_item_creation_tc = decode_smpte_timecode(sdticp_meta->mCPRate,
                                                         sdticp_meta->mCreationTimecode.bytes,
                                                         BMX_ARRAY_SIZE(sdticp_meta->mCreationTimecode.bytes));
            have_sys_item_creation_tc = true;
        }
        if (sdticp_meta->mHaveUserTimecode) {
            sys_item_user_tc = decode_smpte_timecode(sdticp_meta->mCPRate,
                                                     sdticp_meta->mUserTimecode.bytes,
                                                     BMX_ARRAY_SIZE(sdticp_meta->mUserTimecode.bytes));
            have_sys_item_user_tc = true;
        }
    }
    metadata = frame->GetMetadata(SYSTEM_SCHEME_1_FMETA_ID);
    if (metadata) {
        size_t i;
        for (i = 0; i < metadata->size(); i++) {
            const SystemScheme1Metadata *ss1_meta = dynamic_cast<const SystemScheme1Metadata*>((*metadata)[i]);
            if (ss1_meta->GetType() == SystemScheme1Metadata::TIMECODE_ARRAY) {
                ss1_timecodes = dynamic_cast<const SS1TimecodeArray*>(ss1_meta);
                break;
            }
        }
    }

    Timecode timecode;
    // position 'timecode'
    fprintf(tc_file, "%s", get_duration_string(frame->position, frame->edit_rate).c_str());
    // material package timecode
    if (reader->HaveMaterialTimecode()) {
        timecode = reader->GetMaterialTimecode(frame->position);
        fprintf(tc_file, " %s", get_timecode_string(timecode).c_str());
    } else {
        fprintf(tc_file, " %s", null_timecode_str);
    }
    // file source package timecode
    if (reader->HaveFileSourceTimecode()) {
        timecode = reader->GetFileSourceTimecode(frame->position);
        fprintf(tc_file, " %s", get_timecode_string(timecode).c_str());
    } else {
        fprintf(tc_file, " %s", null_timecode_str);
    }
    // physical source package timecode
    if (reader->HavePhysicalSourceTimecode()) {
        timecode = reader->GetPhysicalSourceTimecode(frame->position);
        fprintf(tc_file, " %s", get_timecode_string(timecode).c_str());
    } else {
        fprintf(tc_file, " %s", null_timecode_str);
    }
    // Avid auxiliary timecodes
    size_t i;
    for (i = 0; i < 5; i++) {
        if (reader->HaveAvidAuxTimecode(i)) {
            timecode = reader->GetAvidAuxTimecode(i, frame->position);
            fprintf(tc_file, " %s", get_timecode_string(timecode).c_str());
        } else {
            fprintf(tc_file, " %s", null_timecode_str);
        }
    }
    // system item user date/time timecode
    if (have_sys_item_user_tc)
        fprintf(tc_file, " %s", get_timecode_string(sys_item_user_tc).c_str());
    else
        fprintf(tc_file, " %s", null_timecode_str);
    // system item creation date/time timecode
    if (have_sys_item_creation_tc)
        fprintf(tc_file, " %s", get_timecode_string(sys_item_creation_tc).c_str());
    else
        fprintf(tc_file, " %s", null_timecode_str);
    // system scheme 1 timecode array
    for (i = 0; i < 4; i++) {
        if (ss1_timecodes && i < ss1_timecodes->mS12MTimecodes.size()) {
            timecode = decode_smpte_timecode(frame->edit_rate,
                                             ss1_timecodes->mS12MTimecodes[i].bytes,
                                             BMX_ARRAY_SIZE(ss1_timecodes->mS12MTimecodes[i].bytes));
            fprintf(tc_file, " %s", get_timecode_string(timecode).c_str());
        } else {
            fprintf(tc_file, " %s", null_timecode_str);
        }
    }

    fprintf(tc_file, "\n");
}

static string create_text_object_filename(string prefix, bool is_xml, size_t index)
{
    const char *suffix = ".txt";
    if (is_xml)
        suffix = ".xml";

    char buffer[32];
    bmx_snprintf(buffer, sizeof(buffer), "_%" PRIszt "%s", index, suffix);

    return prefix + buffer;
}

static bool parse_rdd6_frames(const char *frames_str, int64_t *min, int64_t *max)
{
    if (sscanf(frames_str, "%" PRId64 "-%" PRId64, min, max) == 2) {
        return *min <= *max;
    } else if (sscanf(frames_str, "%" PRId64, min) == 1) {
        *max = *min;
        return *min >= 0;
    } else {
        return false;
    }
}

static bool parse_app_events_mask(const char *mask_str, int *mask_out)
{
    const char *ptr = mask_str;
    int mask = 0;

    while (*ptr) {
        if (*ptr == 'd' || *ptr == 'D')
            mask |= DIGIBETA_DROPOUT_MASK;
        else if (*ptr == 'p' || *ptr == 'P')
            mask |= PSE_FAILURE_MASK;
        else if (*ptr == 't' || *ptr == 'T')
            mask |= TIMECODE_BREAK_MASK;
        else if (*ptr == 'v' || *ptr == 'V')
            mask |= VTR_ERROR_MASK;
        else
            return false;

        ptr++;
    }

    *mask_out = mask;
    return true;
}

static bool parse_wrap_klv_mask(const char *mask_str, set<MXFDataDefEnum> *mask)
{
    const char *ptr = mask_str;

    while (*ptr) {
        if (*ptr == 'v' || *ptr == 'V')
            mask->insert(MXF_PICTURE_DDEF);
        else if (*ptr == 'a' || *ptr == 'A')
            mask->insert(MXF_SOUND_DDEF);
        else if (*ptr == 'd' || *ptr == 'D')
            mask->insert(MXF_DATA_DDEF);
        else
            return false;

        ptr++;
    }

    return true;
}

static bool parse_info_format(const char *format_str, InfoFormat *format)
{
    if (strcmp(format_str, "text") == 0)
        *format = TEXT_INFO_FORMAT;
    else if (strcmp(format_str, "xml") == 0)
        *format = XML_INFO_FORMAT;
    else
        return false;

    return true;
}

static void usage(const char *cmd)
{
    fprintf(stderr, "%s\n", get_app_version_info(APP_NAME).c_str());
    fprintf(stderr, "Usage: %s <<options>> [<<input options>> <filename>]+\n", cmd);
    fprintf(stderr, "   Use <filename> '-' for standard input\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -h | --help           Show usage and exit\n");
    fprintf(stderr, " -v | --version        Print version info to stderr\n");
    fprintf(stderr, " -l <file>             Log filename. Default log to stderr\n");
    fprintf(stderr, " --log-level <level>   Set the log level. 0=debug, 1=info, 2=warning, 3=error. Default is 1\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " --file-chksum-only <type>\n");
    fprintf(stderr, "                       Calculate checksum of the file(s) and exit\n");
    fprintf(stderr, "                       <type> is one of the following: 'crc32', 'md5', 'sha1'\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " --group               Use the group reader instead of the sequence reader\n");
    fprintf(stderr, "                       Use this option if the files have different material packages\n");
    fprintf(stderr, "                       but actually belong to the same virtual package / group\n");
    fprintf(stderr, " --no-reorder          Don't attempt to re-order the inputs, based on timecode, when constructing a sequence\n");
    fprintf(stderr, "                       Use this option for files with broken timecode\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " --check-end           Check that the last frame (start + duration - 1) can be read when opening the files\n");
    fprintf(stderr, " --check-complete      Check that the input files are complete\n");
    fprintf(stderr, " --check-app-issues    Check that there are no known issues with the APP (Archive Preservation Project) file\n");
    fprintf(stderr, " --check-app-crc32     Check APP essence CRC-32 data\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " -i | --info           Extract input information. Default output is to stdout\n");
    fprintf(stderr, " --info-format <fmt>   Input info format. 'text' or 'xml'. Default 'text'\n");
    fprintf(stderr, " --info-file <name>    Input info output file <name>\n");
    fprintf(stderr, " --track-chksum <type> Calculate checksum of the track essence data\n");
    fprintf(stderr, "                       <type> is one of the following: 'crc32', 'md5', 'sha1'\n");
    fprintf(stderr, " --file-chksum <type>  Calculate checksum of the input file(s)\n");
    fprintf(stderr, "                       <type> is one of the following: 'crc32', 'md5', 'sha1'\n");
    fprintf(stderr, " --as11                Extract AS-11 and UK DPP metadata\n");
    fprintf(stderr, " --as10                Extract AS-10 metadata\n");
    fprintf(stderr, " --app                 Extract APP metadata\n");
    fprintf(stderr, " --app-events <mask>   Extract APP events metadata\n");
    fprintf(stderr, "                       <mask> is a sequence of event types (e.g. dtv) identified using the following characters:\n");
    fprintf(stderr, "                           d=digibeta dropout, p=PSE failure, t=timecode break, v=VTR error\n");
    fprintf(stderr, " --no-app-events-tc    Don't extract timecodes from the essence container to associate with the APP events metadata\n");
    fprintf(stderr, " --app-crc32 <fname>   Extract APP CRC-32 frame data to <fname>\n");
    fprintf(stderr, " --app-tc <fname>      Extract APP timecodes to <fname>\n");
    fprintf(stderr, " --all-tc <fname>      Extract header and content package metadata timecodes to <fname>\n");
    fprintf(stderr, "                       The list of timecodes extracted for each frame is as follows:\n");
    fprintf(stderr, "                         frame position, material package, file source package, physical source package,\n");
    fprintf(stderr, "                         5 x Avid auxiliary, system item user, system item creation,\n");
    fprintf(stderr, "                         4 x system scheme 1 timecode array\n");
    fprintf(stderr, "                       The header metadata timecodes are limited to the set extracted by bmx and what bmx accepts\n");
    fprintf(stderr, "                       If the timecode property is not present then __:__:__:__ is printed\n");
    fprintf(stderr, " --avid                Extract Avid metadata\n");
    fprintf(stderr, " --st436-mf <count>    Set the <count> of frames to examine for ST 436 ANC/VBI manifest info. Default is %u\n", DEFAULT_ST436_MANIFEST_COUNT);
    fprintf(stderr, " --rdd6 <frames> <filename>\n");
    fprintf(stderr, "                       Extract RDD-6 audio metadata from <frames> to XML <filename>.\n");
    fprintf(stderr, "                       <frames> can either be a single frame or a range using '-' as a separator\n");
    fprintf(stderr, "                       RDD-6 metadata is extracted from the first frame and program description text is accumulated using the other frames\n");
    fprintf(stderr, "                       Not all frames will be required if a complete program text has been extracted\n");
    fprintf(stderr, " --mca-detail          Show detailed MCA channel label information\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " -p | --ess-out <prefix>\n");
    fprintf(stderr, "                       Extract essence to files starting with <prefix> and suffix '.raw'\n");
    fprintf(stderr, " --wrap-klv <mask>     Wrap essence frames in KLV using the input Key and an 8-byte Length\n");
    fprintf(stderr, "                       The filename suffix is '.klv' rather than '.raw'\n");
    fprintf(stderr, "                       <mask> is a sequence of characters which identify which data types to wrap\n");
    fprintf(stderr, "                           v=video, a=audio, d=data\n");
    fprintf(stderr, " --read-ess            Read the essence data, even when no other option requires it\n");
    fprintf(stderr, " --deint               De-interleave multi-channel / AES-3 sound\n");
    fprintf(stderr, " --start <frame>       Set the start frame to read. Default is 0\n");
    fprintf(stderr, " --dur <frame>         Set the duration in frames. Default is minimum avaliable duration\n");
    fprintf(stderr, " --nopc                Don't include pre-charge frames\n");
    fprintf(stderr, " --noro                Don't include roll-out frames\n");
    fprintf(stderr, " --rt <factor>         Read at realtime rate x <factor>, where <factor> is a floating point value\n");
    fprintf(stderr, "                       <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
#if defined(_WIN32)
    fprintf(stderr, " --no-seq-scan         Do not set the sequential scan hint for optimizing file caching\n");
#if !defined(__MINGW32__)
    fprintf(stderr, " --mmap-file           Use memory-mapped file I/O for the MXF files\n");
    fprintf(stderr, "                       Note: this may reduce file I/O performance and was found to be slower over network drives\n");
#endif
#endif
    fprintf(stderr, " --gf                  Support growing files. Retry reading a frame when it fails\n");
    fprintf(stderr, " --gf-retries <max>    Set the maximum times to retry reading a frame. The default is %u.\n", DEFAULT_GF_RETRIES);
    fprintf(stderr, " --gf-delay <sec>      Set the delay (in seconds) between a failure to read and a retry. The default is %f.\n", DEFAULT_GF_RETRY_DELAY);
    fprintf(stderr, " --gf-rate <factor>    Limit the read rate to realtime rate x <factor> after a read failure. The default is %f\n", DEFAULT_GF_RATE_AFTER_FAIL);
    fprintf(stderr, "                       <factor> value 1.0 results in realtime rate, value < 1.0 slower and > 1.0 faster\n");
    if (mxf_http_is_supported()) {
        fprintf(stderr, " --http-min-read <bytes>\n");
        fprintf(stderr, "                       Set the minimum number of bytes to read when accessing a file over HTTP. The default is %u.\n", DEFAULT_HTTP_MIN_READ);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, " --text-out <prefix>   Extract text based objects to files starting with <prefix>\n");
    fprintf(stderr, "                       and suffix '.xml' if it is XML and otherwise '.txt'\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input options:\n");
    fprintf(stderr, " --disable-tracks <tracks> A comma separated list of track indexes and/or ranges to disable when reading essence data.\n");
    fprintf(stderr, "                           A track is identified by the index reported by mxf2raw\n");
    fprintf(stderr, "                           A range of track indexes is specified as '<first>-<last>', e.g. 0-3\n");
    fprintf(stderr, " --disable-audio       Disable audio tracks when reading essence data\n");
    fprintf(stderr, " --disable-video       Disable video tracks when reading essence data\n");
    fprintf(stderr, " --disable-data        Disable data tracks when reading essence data\n");
    fprintf(stderr, "\n");
}

int main(int argc, const char** argv)
{
    std::vector<const char *> input_filenames;
    const char *log_filename = 0;
    LogLevel log_level = INFO_LOG;
    set<ChecksumType> file_checksum_only_types;
    bool use_group_reader = false;
    bool keep_input_order = false;
    bool check_end = false;
    bool check_complete = false;
    bool check_app_issues = false;
    bool do_write_info = false;
    InfoFormat info_format = TEXT_INFO_FORMAT;
    const char *info_filename = 0;
    set<ChecksumType> track_checksum_types;
    set<ChecksumType> file_checksum_types;
    bool do_as11_info = false;
    bool do_as10_info = false;
    bool do_app_info = false;
    int app_events_mask = 0;
    bool extract_app_events_tc = true;
    bool check_app_crc32 = false;
    const char *app_crc32_filename = 0;
    const char *app_tc_filename = 0;
    const char *all_tc_filename = 0;
    bool do_avid_info = false;
    uint32_t st436_manifest_count = DEFAULT_ST436_MANIFEST_COUNT;
    const char *rdd6_filename = 0;
    int64_t rdd6_frame_min = 0;
    int64_t rdd6_frame_max = 0;
    bool do_ess_read = false;
    const char *ess_output_prefix = 0;
    set<MXFDataDefEnum> wrap_klv_mask;
    map<size_t, set<size_t> > disable_track_indexes;
    map<size_t, bool> disable_audio;
    map<size_t, bool> disable_video;
    map<size_t, bool> disable_data;
    bool deinterleave = false;
    int64_t start = 0;
    bool start_set = false;
    int64_t duration = -1;
    bool no_precharge = false;
    bool no_rollout = false;
#if defined(_WIN32)
    int file_flags = MXF_WIN32_FLAG_SEQUENTIAL_SCAN;
#else
    int file_flags = 0;
#endif
    bool realtime = false;
    float rt_factor = 1.0;
    bool growing_file = false;
    unsigned int gf_retries = DEFAULT_GF_RETRIES;
    float gf_retry_delay = DEFAULT_GF_RETRY_DELAY;
    float gf_rate_after_fail = DEFAULT_GF_RATE_AFTER_FAIL;
    uint32_t http_min_read = DEFAULT_HTTP_MIN_READ;
    ChecksumType checkum_type;
#if defined(_WIN32) && !defined(__MINGW32__)
    bool use_mmap_file = false;
#endif
    const char *text_output_prefix = 0;
    bool mca_detail = false;
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
            fprintf(stderr, "%s\n", get_app_version_info(APP_NAME).c_str());
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
        else if (strcmp(argv[cmdln_index], "--file-chksum-only") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_checksum_type(argv[cmdln_index + 1], &checkum_type))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            file_checksum_only_types.insert(checkum_type);
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
        else if (strcmp(argv[cmdln_index], "--check-end") == 0)
        {
            check_end = true;
        }
        else if (strcmp(argv[cmdln_index], "--check-complete") == 0)
        {
            check_complete = true;
        }
        else if (strcmp(argv[cmdln_index], "--check-app-issues") == 0)
        {
            check_app_issues = true;
        }
        else if (strcmp(argv[cmdln_index], "--check-app-crc32") == 0)
        {
            check_app_crc32 = true;
            do_ess_read = true;
            do_write_info = true;
        }
        else if (strcmp(argv[cmdln_index], "-i") == 0 ||
                 strcmp(argv[cmdln_index], "--info") == 0)
        {
            do_write_info = true;
        }
        else if (strcmp(argv[cmdln_index], "--info-format") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_info_format(argv[cmdln_index + 1], &info_format))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            do_write_info = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--info-file") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            info_filename = argv[cmdln_index + 1];
            do_write_info = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--track-chksum") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_checksum_type(argv[cmdln_index + 1], &checkum_type))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            track_checksum_types.insert(checkum_type);
            do_write_info = true;
            do_ess_read = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--file-chksum") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_checksum_type(argv[cmdln_index + 1], &checkum_type))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            file_checksum_types.insert(checkum_type);
            do_write_info = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--as11") == 0)
        {
            do_as11_info = true;
            do_write_info = true;
        }
        else if (strcmp(argv[cmdln_index], "--as10") == 0)
        {
            do_as10_info = true;
            do_write_info = true;
        }
        else if (strcmp(argv[cmdln_index], "--app") == 0)
        {
            do_app_info = true;
            do_write_info = true;
        }
        else if (strcmp(argv[cmdln_index], "--app-events") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_app_events_mask(argv[cmdln_index + 1], &app_events_mask))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            do_app_info = true;
            do_write_info = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--no-app-events-tc") == 0)
        {
            extract_app_events_tc = false;
        }
        else if (strcmp(argv[cmdln_index], "--app-crc32") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            app_crc32_filename = argv[cmdln_index + 1];
            do_ess_read = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--app-tc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            app_tc_filename = argv[cmdln_index + 1];
            do_ess_read = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--all-tc") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            all_tc_filename = argv[cmdln_index + 1];
            do_ess_read = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--avid") == 0)
        {
            do_avid_info = true;
            do_write_info = true;
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
            do_write_info = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--rdd6") == 0)
        {
            if (cmdln_index + 2 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (parse_rdd6_frames(argv[cmdln_index + 1], &rdd6_frame_min, &rdd6_frame_max) != 1)
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            rdd6_filename = argv[cmdln_index + 2];
            cmdln_index += 2;
        }
        else if (strcmp(argv[cmdln_index], "--mca-detail") == 0)
        {
            mca_detail = true;
            do_write_info = true;
        }
        else if (strcmp(argv[cmdln_index], "-p") == 0 ||
                 strcmp(argv[cmdln_index], "--ess-out") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            ess_output_prefix = argv[cmdln_index + 1];
            do_ess_read = true;
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--wrap-klv") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            if (!parse_wrap_klv_mask(argv[cmdln_index + 1], &wrap_klv_mask))
            {
                usage(argv[0]);
                fprintf(stderr, "Invalid value '%s' for option '%s'\n", argv[cmdln_index + 1], argv[cmdln_index]);
                return 1;
            }
            cmdln_index++;
        }
        else if (strcmp(argv[cmdln_index], "--read-ess") == 0)
        {
            do_ess_read = true;
        }
        else if (strcmp(argv[cmdln_index], "--deint") == 0)
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
            if (sscanf(argv[cmdln_index + 1], "%" PRId64, &start) != 1 || start < 0)
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
            if (sscanf(argv[cmdln_index + 1], "%" PRId64, &duration) != 1 || duration < 0)
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
#if defined(_WIN32)
        else if (strcmp(argv[cmdln_index], "--no-seq-scan") == 0)
        {
            file_flags &= ~MXF_WIN32_FLAG_SEQUENTIAL_SCAN;
        }
#if !defined(__MINGW32__)
        else if (strcmp(argv[cmdln_index], "--mmap-file") == 0)
        {
            use_mmap_file = true;
        }
#endif
#endif
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
        else if (strcmp(argv[cmdln_index], "--text-out") == 0)
        {
            if (cmdln_index + 1 >= argc)
            {
                usage(argv[0]);
                fprintf(stderr, "Missing argument for option '%s'\n", argv[cmdln_index]);
                return 1;
            }
            text_output_prefix = argv[cmdln_index + 1];
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
        else if (strcmp(argv[cmdln_index], "--regtest") == 0)
        {
            BMX_REGRESSION_TEST = true;
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

    if (cmdln_index == 1) {
        // default to outputting info if no options are given
        do_write_info = true;
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


    LOG_LEVEL = log_level;
    if (log_filename && !open_log_file(log_filename))
        return 1;
    if (do_write_info) {
        // intercept log messages for adding to structured info output
        if (log_filename)
            LOG_DATA.vlog2 = bmx::vlog2;
        else
            LOG_DATA.vlog2 = 0;
        bmx::log   = mxf2raw_log;
        bmx::vlog  = mxf2raw_vlog;
        bmx::vlog2 = mxf2raw_vlog2;
    }

    connect_libmxf_logging();


    int cmd_result = 0;

    if (!file_checksum_only_types.empty()) {
        try
        {
            vector<ChecksumType> types_vec;
            types_vec.insert(types_vec.begin(), file_checksum_only_types.begin(), file_checksum_only_types.end());

            calc_file_checksums(input_filenames, types_vec);
        }
        catch (const BMXException &ex)
        {
            log_error("BMX exception caught: %s\n", ex.what());
            cmd_result = 1;
        }
        catch (const bool &ex)
        {
            if (ex)
                cmd_result = 0;
            else
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


    try
    {
        bool complete_result = true;
        bool last_frame_result = true;
        bool app_issues_result = true;
        CRC32CheckResult app_crc32_result = CRC32_NOT_CHECKED;


        FILE *info_file = stdout;
        if (info_filename) {
            info_file = fopen(info_filename, "wb");
            if (!info_file) {
                log_error("Failed to open info file '%s': %s\n",
                          info_filename, bmx_strerror(errno).c_str());
                throw false;
            }
        }


        AppMXFFileFactory file_factory;
        APPInfoOutput app_output;
        MXFReader *reader = 0;
        MXFFileReader *file_reader = 0;

        if (!file_checksum_types.empty())
            file_factory.SetInputChecksumTypes(file_checksum_types);
        file_factory.SetInputFlags(file_flags);
        file_factory.SetHTTPMinReadSize(http_min_read);
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
                result = grp_file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", get_input_filename(input_filenames[i]),
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
                result = seq_file_reader->Open(input_filenames[i]);
                if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                    log_error("Failed to open MXF file '%s': %s\n", get_input_filename(input_filenames[i]),
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
            if (do_as11_info)
                as11_register_extensions(file_reader);
            if (do_as10_info)
                as10_register_extensions(file_reader);
            if (do_app_info || check_app_issues)
                app_output.RegisterExtensions(file_reader);
            // avid extensions are already registered by the MXFReader
            result = file_reader->Open(input_filenames[0]);
            if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
                log_error("Failed to open MXF file '%s': %s\n", get_input_filename(input_filenames[0]),
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
                complete_result = false;
                cmd_result = 1;
            }
            if (reader->IsSeekable())
                log_warn("Input file is incomplete\n");
            else
                log_debug("Input file is not seekable\n");
        }

        mxfRational edit_rate = reader->GetEditRate();


        // check ANC track is present if RDD-6 extraction requested
        bool have_anc_track = false;
        if (rdd6_filename) {
            size_t i;
            for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                if (reader->GetTrackReader(i)->GetTrackInfo()->essence_type == ANC_DATA) {
                    have_anc_track = true;
                    break;
                }
            }
            if (!have_anc_track)
                log_warn("No ANC data track present\n");
        }


        // set read limits and checks
        int64_t output_duration;
        int64_t max_precharge = 0;
        int64_t max_rollout = 0;
        if (!reader->IsComplete()) {
            if (start_set || duration >= 0) {
                log_error("The --start and --dur options are not yet supported for incomplete files\n");
                throw false;
            }
            if (check_end) {
                log_error("Checking last frame is present (--check-end) is not supported for incomplete files\n");
                last_frame_result = false;
                cmd_result = 1;
            }
            output_duration = reader->GetDuration();
        } else {
            int64_t input_duration = reader->GetDuration();
            if (start > input_duration) {
                log_error("Start position %" PRId64 " is beyond available frames %" PRId64 "\n", start, input_duration);
                throw false;
            }
            output_duration = input_duration - start;
            if (duration >= 0) {
                if (duration <= output_duration) {
                    output_duration = duration;
                } else {
                    log_warn("Output duration %" PRId64 " not possible. Set to %" PRId64 " instead\n",
                             duration, output_duration);
                }
            }

            if (!no_precharge)
                max_precharge = reader->GetMaxPrecharge(start, false);
            if (!no_rollout)
                max_rollout = reader->GetMaxRollout(start + output_duration - 1, false);

            reader->SetReadLimits(start + max_precharge, - max_precharge + output_duration + max_rollout, true);

            if (check_end && !reader->CheckReadLastFrame()) {
                log_error("Check for last frame failed\n");
                last_frame_result = false;
                cmd_result = 1;
            }
        }

        if (file_reader) {
            if (do_app_info)
                app_output.ExtractInfo(app_events_mask);

            if (check_app_issues && !app_output.CheckIssues()) {
                log_error("APP file has issues\n");
                app_issues_result = false;
                cmd_result = 1;
            }
        }


        // force file checksum update now that reading/seeking will generally be forwards only
        if (!file_checksum_types.empty())
            file_factory.ForceInputChecksumUpdate();


        // read data

        int64_t last_rdd6_frame = -1;
        bool rdd6_failed = false;
        bool rdd6_done = false;
        RDD6MetadataFrame rdd6_frame;
        vector<string> rdd6_desc_chars;
        vector<bool> rdd6_have_start;
        vector<bool> rdd6_have_end;
        vector<vector<Checksum> > track_checksums;
        vector<CRC32Data> track_crc32_data;
        OutputFileManager output_file_manager;

        if (do_ess_read) {

            // track checksum calculation initialization
            if (!track_checksum_types.empty()) {
                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    track_checksums.push_back(vector<Checksum>());
                    set<ChecksumType>::const_iterator iter;
                    for (iter = track_checksum_types.begin(); iter != track_checksum_types.end(); iter++)
                        track_checksums[i].push_back(Checksum(*iter));
                }
            }

            // APP crc32 check initialization
            if (check_app_crc32) {
                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    CRC32Data data = {0, 0, 0};
                    track_crc32_data.push_back(data);
                }
            }

            // open APP crc32 output file
            FILE *app_crc32_file = 0;
            if (file_reader && app_crc32_filename) {
                app_crc32_file = fopen(app_crc32_filename, "wb");
                if (!app_crc32_file) {
                    log_error("Failed to open APP CRC-32 file '%s': %s\n",
                              app_crc32_filename, bmx_strerror(errno).c_str());
                    throw false;
                }
            }

            // open APP timecode output file
            FILE *app_tc_file = 0;
            if (file_reader && app_tc_filename) {
                app_tc_file = fopen(app_tc_filename, "wb");
                if (!app_tc_file) {
                    log_error("Failed to open APP timecode file '%s': %s\n",
                              app_tc_filename, bmx_strerror(errno).c_str());
                    throw false;
                }
            }

            // open all timecode output file
            FILE *all_tc_file = 0;
            if (all_tc_filename) {
                all_tc_file = fopen(all_tc_filename, "wb");
                if (!all_tc_file) {
                    log_error("Failed to open all timecode file '%s': %s\n",
                              all_tc_filename, bmx_strerror(errno).c_str());
                    throw false;
                }
                fprintf(all_tc_file,
                        "# %9s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|%11s|\n",
                        "position",
                        "material p", "file src p", "phys src p",
                        "aux 1", "aux 2", "aux 3", "aux 4", "aux 5",
                        "sys user", "sys create",
                        "ss1 1", "ss1 2", "ss1 3", "ss1 4");
            }

            // open raw files and check if have video
            bool have_video = false;
            if (ess_output_prefix) {
                output_file_manager.SetPrefix(ess_output_prefix);
                output_file_manager.SetSoundDeinterleave(deinterleave);

                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    if (!reader->GetTrackReader(i)->IsEnabled())
                        continue;

                    const MXFTrackInfo *track_info = reader->GetTrackReader(i)->GetTrackInfo();
                    output_file_manager.AddTrackFile(i, track_info,
                                                     (wrap_klv_mask.find(track_info->data_def) != wrap_klv_mask.end()));

                    if (track_info->data_def == MXF_PICTURE_DDEF)
                        have_video = true;
                }
            }

            // choose number of samples to read in one go
            uint32_t max_samples_per_read = 1;
            if (!have_video && edit_rate == SAMPLING_RATE_48K)
                max_samples_per_read = 1920;

            // realtime reading
            uint32_t rt_start = 0;
            if (realtime)
                rt_start = get_tick_count();

            // growing file
            unsigned int gf_retry_count = 0;
            bool gf_read_failure = false;
            int64_t gf_failure_num_read = 0;
            uint32_t gf_failure_start = 0;

            // read data
            bmx::ByteArray sound_buffer;
            int64_t total_num_read = 0;
            while (true)
            {
                uint32_t num_read = reader->Read(max_samples_per_read);
                if (num_read == 0) {
                    if (!growing_file || !reader->ReadError() || gf_retry_count >= gf_retries)
                        break;
                    gf_retry_count++;
                    gf_read_failure = true;
                    if (gf_retry_delay > 0.0) {
                        rt_sleep(1.0f / gf_retry_delay, get_tick_count(), edit_rate,
                                 edit_rate.numerator / edit_rate.denominator);
                    }
                    continue;
                }
                if (growing_file && gf_retry_count > 0) {
                    gf_failure_num_read = total_num_read;
                    gf_failure_start    = get_tick_count();
                    gf_retry_count      = 0;
                }
                total_num_read += num_read;

                vector<uint64_t> crc32_data(reader->GetNumTrackReaders(), UINT64_MAX);
                bool have_app_tc = false;
                bool written_timecodes = false;
                size_t i;
                for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                    MXFTrackInfo *track_info = reader->GetTrackReader(i)->GetTrackInfo();
                    while (true) {
                        Frame *frame = reader->GetTrackReader(i)->GetFrameBuffer()->GetLastFrame(true);
                        if (!frame)
                            break;
                        if (frame->IsEmpty()) {
                            delete frame;
                            continue;
                        }

                        if (!track_checksums.empty()) {
                            size_t m;
                            for (m = 0; m < track_checksums[i].size(); m++)
                                track_checksums[i][m].Update(frame->GetBytes(), frame->GetSize());
                        }

                        if (check_app_crc32 || app_crc32_file) {
                            const vector<FrameMetadata*> *metadata = frame->GetMetadata(SYSTEM_SCHEME_1_FMETA_ID);
                            if (metadata) {
                                size_t m;
                                for (m = 0; m < metadata->size(); m++) {
                                    const SystemScheme1Metadata *ss1_meta =
                                        dynamic_cast<const SystemScheme1Metadata*>((*metadata)[m]);
                                    if (ss1_meta->GetType() != SystemScheme1Metadata::APP_CHECKSUM)
                                        continue;

                                    const SS1APPChecksum *checksum = dynamic_cast<const SS1APPChecksum*>(ss1_meta);

                                    if (app_crc32_file)
                                        crc32_data[i] = checksum->mCRC32;

                                    if (check_app_crc32) {
                                        uint32_t crc32;
                                        crc32_init(&crc32);
                                        crc32_update(&crc32, frame->GetBytes(), frame->GetSize());
                                        crc32_final(&crc32);

                                        if (crc32 != checksum->mCRC32)
                                            track_crc32_data[i].error_count++;
                                        track_crc32_data[i].check_count++;
                                    }

                                    break;
                                }
                            }
                            if (check_app_crc32)
                                track_crc32_data[i].total_read++;
                        }

                        if (!have_app_tc && file_reader &&
                            ((app_events_mask && extract_app_events_tc) || app_tc_file))
                        {
                            const vector<FrameMetadata*> *metadata = frame->GetMetadata(SYSTEM_SCHEME_1_FMETA_ID);
                            if (metadata) {
                                size_t i;
                                for (i = 0; i < metadata->size(); i++) {
                                    const SystemScheme1Metadata *ss1_meta =
                                        dynamic_cast<const SystemScheme1Metadata*>((*metadata)[i]);
                                    if (ss1_meta->GetType() != SystemScheme1Metadata::TIMECODE_ARRAY)
                                        continue;

                                    const SS1TimecodeArray *tc_array = dynamic_cast<const SS1TimecodeArray*>(ss1_meta);
                                    if (app_events_mask && extract_app_events_tc) {
                                        app_output.AddEventTimecodes(frame->position, tc_array->GetVITC(),
                                                                     tc_array->GetLTC());
                                    }
                                    if (app_tc_file) {
                                        Timecode ctc(edit_rate, false, frame->position);
                                        CHECK_FPRINTF(app_tc_filename,
                                                      fprintf(app_tc_file, "C%s V%s L%s\n",
                                                              get_timecode_string(ctc).c_str(),
                                                              get_timecode_string(tc_array->GetVITC()).c_str(),
                                                              get_timecode_string(tc_array->GetLTC()).c_str()));
                                    }

                                    have_app_tc = true;
                                    break;
                                }
                            }
                        }

                        if (all_tc_file && !written_timecodes) {
                            write_timecodes(reader, frame, all_tc_file);
                            written_timecodes = true;
                        }

                        if (ess_output_prefix) {
                            FILE *file;
                            string filename;
                            const MXFSoundTrackInfo *sound_info = dynamic_cast<const MXFSoundTrackInfo*>(track_info);
                            if (sound_info && deinterleave && sound_info->channel_count > 1) {
                                sound_buffer.Allocate(frame->GetSize()); // more than enough
                                uint32_t c;
                                for (c = 0; c < sound_info->channel_count; c++) {
                                    if (sound_info->essence_type == D10_AES3_PCM) {
                                        convert_aes3_to_pcm(frame->GetBytes(), frame->GetSize(), false,
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
                                    output_file_manager.GetTrackFile(i, c, &file, &filename);
                                    write_data(file, filename,
                                               sound_buffer.GetBytes(), sound_buffer.GetSize(),
                                               (wrap_klv_mask.find(track_info->data_def) != wrap_klv_mask.end()),
                                               &frame->element_key);
                                }
                            } else if (track_info->essence_type != TIMED_TEXT) {  // timed text is written at the end
                                output_file_manager.GetTrackFile(i, &file, &filename);
                                write_data(file, filename,
                                           frame->GetBytes(), frame->GetSize(),
                                           (wrap_klv_mask.find(track_info->data_def) != wrap_klv_mask.end()),
                                           &frame->element_key);
                            }
                        }

                        if (track_info->essence_type == ANC_DATA && rdd6_filename && !rdd6_failed && !rdd6_done) {
                            if ((last_rdd6_frame  < 0 && frame->position == rdd6_frame_min) ||
                                (last_rdd6_frame >= 0 && frame->position <= rdd6_frame_max &&
                                    frame->position == last_rdd6_frame + 1))
                            {
                                if (update_rdd6_xml(frame, &rdd6_frame, &rdd6_desc_chars, &rdd6_have_start,
                                                    &rdd6_have_end, &rdd6_done))
                                {
                                    last_rdd6_frame = frame->position;
                                }
                                else
                                {
                                    rdd6_failed = true;
                                }
                            }
                        }

                        delete frame;
                    }
                }

                if (app_crc32_file) {
                    CHECK_FPRINTF(app_crc32_filename,
                                  fprintf(app_crc32_file, "%" PRId64, total_num_read - num_read));
                    size_t i;
                    for (i = 0; i < crc32_data.size(); i++) {
                        if (crc32_data[i] == UINT64_MAX) {
                            CHECK_FPRINTF(app_crc32_filename,
                                          fprintf(app_crc32_file, " ????"));
                        } else {
                            CHECK_FPRINTF(app_crc32_filename,
                                          fprintf(app_crc32_file, " %04x", (uint32_t)crc32_data[i]));
                        }
                    }
                    CHECK_FPRINTF(app_crc32_filename,
                                  fprintf(app_crc32_file, "\n"));
                }

                if (gf_read_failure)
                    rt_sleep(gf_rate_after_fail, gf_failure_start, edit_rate, total_num_read - gf_failure_num_read);
                else if (realtime)
                    rt_sleep(rt_factor, rt_start, edit_rate, total_num_read);
            }
            if (reader->ReadError()) {
                bmx::log(reader->IsComplete() ? ERROR_LOG : WARN_LOG,
                         "A read error occurred: %s\n", reader->ReadErrorMessage().c_str());
                if (gf_retry_count >= gf_retries)
                    log_warn("Reached maximum growing file retries, %u\n", gf_retries);
                if (reader->IsComplete())
                    cmd_result = 1;
            }

            if (!track_checksums.empty()) {
                size_t i, m;
                for (i = 0; i < track_checksums.size(); i++) {
                    for (m = 0; m < track_checksums[i].size(); m++)
                        track_checksums[i][m].Final();
                }
            }

            if (check_app_crc32) {
                app_crc32_result = CRC32_PASSED;

                bool file_missing_crc32 = true;
                size_t i;
                for (i = 0; i < track_crc32_data.size(); i++) {
                    if (track_crc32_data[i].check_count > 0) {
                        file_missing_crc32 = false;
                        break;
                    }
                }
                if (file_missing_crc32) {
                    app_crc32_result = CRC32_MISSING_DATA;
                } else {
                    for (i = 0; i < track_crc32_data.size(); i++) {
                        if (track_crc32_data[i].error_count > 0) {
                            log_error("Track %" PRIszt " has %" PRId64 " CRC-32 errors\n",
                                      i, track_crc32_data[i].error_count);
                            app_crc32_result = CRC32_FAILED;
                            cmd_result = 1;
                        }
                        if (track_crc32_data[i].total_read > track_crc32_data[i].check_count) {
                            if (track_crc32_data[i].check_count == 0) {
                                log_warn("Track %" PRIszt " does not contain CRC-32 data\n", i);
                            } else {
                                log_warn("Track %" PRIszt " is missing CRC-32 data in %" PRId64 " frames\n",
                                          i, track_crc32_data[i].total_read - track_crc32_data[i].check_count);
                            }
                            app_crc32_result = CRC32_MISSING_DATA;
                        }
                    }
                }
            }

            log_info("Read %" PRId64 " samples (%s)\n",
                     total_num_read,
                     get_generic_duration_string_2(total_num_read, edit_rate).c_str());


            // clean-up
            if (app_tc_file)
                fclose(app_tc_file);
            if (app_crc32_file)
                fclose(app_crc32_file);
            if (all_tc_file)
                fclose(all_tc_file);
        }

        if (have_anc_track && rdd6_filename && !rdd6_failed && last_rdd6_frame != rdd6_frame_max && !rdd6_done) {
            reader->ClearFrameBuffers(true);
            if (reader->IsComplete())
                reader->SetReadLimits();
            if (last_rdd6_frame < 0)
                reader->Seek(rdd6_frame_min);
            else
                reader->Seek(last_rdd6_frame + 1);
            while (!rdd6_failed && !rdd6_done && last_rdd6_frame < rdd6_frame_max &&
                   reader->GetPosition() <= rdd6_frame_max && reader->Read(1))
            {
                bool have_anc_data = false;
                uint32_t i;
                for (i = 0; i < reader->GetNumTrackReaders() && !have_anc_data; i++) {
                    while (!rdd6_failed && last_rdd6_frame < rdd6_frame_max) {
                        Frame *frame = reader->GetTrackReader(i)->GetFrameBuffer()->GetLastFrame(true);
                        if (!frame)
                            break;
                        if (frame->IsEmpty()) {
                            delete frame;
                            continue;
                        }

                        if (reader->GetTrackReader(i)->GetTrackInfo()->essence_type == ANC_DATA) {
                            if (update_rdd6_xml(frame, &rdd6_frame, &rdd6_desc_chars, &rdd6_have_start,
                                                &rdd6_have_end, &rdd6_done))
                            {
                                last_rdd6_frame = frame->position;
                            }
                            else
                            {
                                rdd6_failed = true;
                            }
                            have_anc_data = true;
                        }

                        delete frame;
                    }
                }
            }
        }
        if (rdd6_filename) {
            if (rdd6_frame.IsEmpty()) {
                log_warn("Failed to extract RDD-6 frame data to XML file\n");
            } else {
                if (!rdd6_failed && last_rdd6_frame > rdd6_frame_min)
                    rdd6_frame.SetCumulativeDescriptionTextChars(rdd6_desc_chars);
                rdd6_frame.UnparseXML(rdd6_filename);
            }
        }

        if (file_reader && app_events_mask && extract_app_events_tc) {
            if (file_reader->IsComplete())
                file_reader->SetReadLimits();
            // enabling just 1 track is sufficient to get the timecode metadata
            bool have_track = false;
            size_t i;
            for (i = file_reader->GetNumTrackReaders(); i > 0; i--) {
                if (!have_track && file_reader->GetTrackReader(i - 1)->IsEnabled())
                    have_track = true;
                else
                    file_reader->GetTrackReader(i - 1)->SetEnable(false);
            }
            app_output.CompleteEventTimecodes();
        }

        // extract text objects
        if (text_output_prefix) {
            size_t i;
            for (i = 0; i < reader->GetNumTextObjects(); i++) {
                MXFTextObject *text_object = reader->GetTextObject(i);
                string text_filename = create_text_object_filename(text_output_prefix, text_object->IsXML(), i);
                FILE *text_file = fopen(text_filename.c_str(), "wb");
                if (!text_file) {
                    log_error("Failed to open text output file '%s': %s\n",
                              text_filename.c_str(), bmx_strerror(errno).c_str());
                    throw false;
                }
                text_object->Read(text_file);
                fclose(text_file);
            }
        }

        // extract timed text
        if (ess_output_prefix) {
            size_t i;
            for (i = 0; i < reader->GetNumTrackReaders(); i++) {
                MXFTrackReader *track_reader = reader->GetTrackReader(i);
                if (track_reader->GetTrackInfo()->essence_type != TIMED_TEXT) {
                    continue;
                }

                MXFTimedTextTrackReader *tt_track_reader = dynamic_cast<MXFTimedTextTrackReader*>(track_reader);
                if (!tt_track_reader) {
                    log_warn("Extracting timed text from a sequence is not supported\n");
                    continue;
                }

                FILE *file;
                string filename;
                output_file_manager.GetTrackFile(i, &file, &filename);
                tt_track_reader->ReadTimedText(file, 0, 0);

                TimedTextManifest *manifest = tt_track_reader->GetManifest();
                vector<TimedTextAncillaryResource> &anc_resources = manifest->GetAncillaryResources();
                size_t k;
                for (k = 0; k < anc_resources.size(); k++) {
                    output_file_manager.GetTrackFile(i, anc_resources[k].stream_id, &file, &filename);
                    tt_track_reader->ReadAncillaryResourceByStreamId(anc_resources[k].stream_id, file, 0, 0);
                }
            }
        }


        file_factory.FinalizeInputChecksum();


        if (do_write_info) {
            AppInfoWriter *info_writer;
            if (info_format == XML_INFO_FORMAT) {
                AppXMLInfoWriter *xml_info_writer = new AppXMLInfoWriter(info_file);
                xml_info_writer->SetNamespace(XML_INFO_WRITER_NAMESPACE, "");
                xml_info_writer->SetVersion(XML_INFO_WRITER_VERSION);
                info_writer = xml_info_writer;
            } else {
                info_writer = new AppTextInfoWriter(info_file);
            }

            info_writer->RegisterCCName("cdci_descriptor",  "CDCIDescriptor");
            info_writer->RegisterCCName("anc_descriptor",   "ANCDescriptor");
            info_writer->RegisterCCName("vbi_descriptor",   "VBIDescriptor");
            info_writer->RegisterCCName("crc32_check",      "CRC32Check");
            info_writer->RegisterCCName("did_type_1",       "DIDType1");
            info_writer->RegisterCCName("did_type_2",       "DIDType2");
            info_writer->SetClipEditRate(edit_rate);

            info_writer->StartAnnotations();
            info_writer->WriteTimestampItem("created", generate_timestamp_now());
            info_writer->EndAnnotations();
            info_writer->Start("bmx");

            info_writer->StartSection("application");
            write_application_info(info_writer);
            info_writer->EndSection();

            vector<size_t> file_ids = reader->GetFileIds(false);
            info_writer->StartArrayItem("files", file_ids.size());
            size_t i;
            for (i = 0; i < file_ids.size(); i++) {
                info_writer->StartArrayElement("file", i);
                write_file_info(info_writer, reader->GetFileReader(file_ids[i]), &file_factory);
                info_writer->EndArrayElement();
            }
            info_writer->EndArrayItem();

            info_writer->StartSection("clip");
            write_clip_info(info_writer, reader, track_checksums, track_crc32_data, mca_detail);
            if (reader->GetNumTextObjects() > 0) {
                info_writer->StartArrayItem("text_objects", reader->GetNumTextObjects());
                size_t i;
                for (i = 0; i < reader->GetNumTextObjects(); i++) {
                    info_writer->StartArrayElement("text_object", i);
                    write_text_object_info(info_writer, reader->GetTextObject(i));
                    info_writer->EndArrayElement();
                }
                info_writer->EndArrayItem();
            }
            if (file_reader) {
                if (do_as11_info)
                    as11_write_info(info_writer, file_reader);
                if (do_as10_info)
                    as10_write_info(info_writer, file_reader);
                if (do_avid_info)
                    avid_write_info(info_writer, file_reader);
                if (do_app_info)
                    app_output.WriteInfo(info_writer, app_events_mask != 0);
            } else {
                if (do_as11_info)
                    log_warn("AS-11 info only supported for a single input file\n");
                if (do_as10_info)
                    log_warn("AS-10 info only supported for a single input file\n");
                if (do_avid_info)
                    log_warn("Avid info only supported for a single input file\n");
                if (do_app_info)
                    log_warn("APP info only supported for a single input file\n");
            }
            info_writer->EndSection();

            if (check_complete || check_end || (file_reader && check_app_issues) || check_app_crc32) {
                info_writer->StartSection("checks");
                if (check_complete)
                    info_writer->WriteBoolItem("is_complete", complete_result);
                if (check_end)
                    info_writer->WriteBoolItem("last_frame", last_frame_result);
                if (file_reader && check_app_issues)
                    info_writer->WriteBoolItem("no_app_issues", app_issues_result);
                if (check_app_crc32)
                    info_writer->WriteEnumStringItem("app_crc32", CRC32_CHECK_RESULT_EINFO, app_crc32_result);
                info_writer->EndSection();
            }

            if (!LOG_DATA.messages.empty()) {
                info_writer->StartArrayItem("log_messages", LOG_DATA.messages.size());
                write_log_messages(info_writer);
                info_writer->EndArrayItem();
            }

            info_writer->End();

            delete info_writer;
        }


        delete reader;
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
        if (ex)
            cmd_result = 0;
        else
            cmd_result = 1;
    }
    catch (...)
    {
        log_error("Unknown exception caught\n");
        cmd_result = 1;
    }

    if (log_filename)
        close_log_file();
    else if (cmd_result != 0 && !LOG_DATA.messages.empty())
        dump_log_messages();


    return cmd_result;
}

