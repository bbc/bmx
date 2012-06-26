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
#include <cstdarg>
#include <cctype>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <vector>
#include <string>
#include <exception>

using namespace std;



#define MOV_CHECK(cmd) \
    if (!(cmd)) { \
        throw MOVException("%s failed at line %d", #cmd, __LINE__); \
    }

#define MKTAG(cs) ((((uint32_t)cs[0])<<24)|(((uint32_t)cs[1])<<16)|(((uint32_t)cs[2])<<8)|(((uint32_t)cs[3])))

#define ATOM_INDENT             "    "
#define ATOM_VALUE_INDENT       "  "

#define CURRENT_ATOM            g_atoms.back()
#define HAVE_PREV_ATOM          (g_atoms.size() >= 2)
#define PREV_ATOM               g_atoms[g_atoms.size() - 2]

#define DUMP_FUNC_MAP_SIZE      (sizeof(dump_func_map) / sizeof(DumpFuncMap))



typedef struct
{
    uint64_t size;
    char type[4];
    uint64_t rem_size;
    uint64_t offset;
} MOVAtom;

typedef struct
{
    char type[4];
    void (*dump)();
} DumpFuncMap;


static const uint32_t MHLR_COMPONENT_TYPE     = MKTAG("mhlr");
static const uint32_t VIDE_COMPONENT_SUB_TYPE = MKTAG("vide");
static const uint32_t SOUN_COMPONENT_SUB_TYPE = MKTAG("soun");
static const uint32_t TMCD_COMPONENT_SUB_TYPE = MKTAG("tmcd");


static bool g_file_size_64bit = false;
static FILE *g_mov_file = 0;
static vector<MOVAtom> g_atoms;
static uint64_t g_file_offset;
static vector<string> g_meta_keys;
static uint32_t g_movie_timescale;
static uint32_t g_component_type = 0;
static uint32_t g_component_sub_type = 0;


class MOVException : public std::exception
{
public:
    MOVException()
    : exception()
    {
    }

    MOVException(const char *format, ...)
    : exception()
    {
        char message[1024];

        va_list varg;
        va_start(varg, format);
#if defined(_MSC_VER)
        int res = _vsnprintf(message, sizeof(message), format, varg);
        if (res == -1 && errno == EINVAL)
            message[0] = 0;
        else
            message[sizeof(message) - 1] = 0;
#else
        if (vsnprintf(message, sizeof(message), format, varg) < 0)
            message[0] = 0;
#endif
        va_end(varg);

        mMessage = message;
    }

    MOVException(const string &message)
    : exception()
    {
        mMessage = message;
    }

    ~MOVException() throw()
    {
    }

    const char* what() const throw()
    {
        return mMessage.c_str();
    }

protected:
    string mMessage;
};



static bool equals_type(const char *left, const char *right)
{
    return memcmp(left, right, 4) == 0;
}

static void update_atom_read(uint64_t num_read)
{
    MOV_CHECK(!g_atoms.empty());
    MOV_CHECK(num_read <= CURRENT_ATOM.rem_size);
    CURRENT_ATOM.rem_size -= num_read;

    g_file_offset += num_read;
}

static void skip_bytes(uint64_t num_bytes)
{
    MOV_CHECK(fseeko(g_mov_file, num_bytes, SEEK_CUR) == 0);
    update_atom_read(num_bytes);
}

static void push_atom()
{
    MOVAtom atom;
    memset(&atom, 0, sizeof(atom));
    g_atoms.push_back(atom);
}

static void pop_atom()
{
    MOV_CHECK(!g_atoms.empty());
    MOV_CHECK(CURRENT_ATOM.rem_size == 0);

    if (g_atoms.size() > 1)
        PREV_ATOM.rem_size -= CURRENT_ATOM.size;

    g_atoms.pop_back();
}

static bool read_bytes(unsigned char *bytes, uint32_t size)
{
    if (fread(bytes, size, 1, g_mov_file) != 1) {
        if (ferror(g_mov_file))
            throw MOVException("Failed to read bytes: %s", strerror(errno));
        return false;
    }

    update_atom_read(size);
    return true;
}

static bool read_uint64(uint64_t *value)
{
    unsigned char bytes[8];
    if (!read_bytes(bytes, 8))
        return false;

    *value = (((uint64_t)bytes[0]) << 56) |
             (((uint64_t)bytes[1]) << 48) |
             (((uint64_t)bytes[2]) << 40) |
             (((uint64_t)bytes[3]) << 32) |
             (((uint64_t)bytes[4]) << 24) |
             (((uint64_t)bytes[5]) << 16) |
             (((uint64_t)bytes[6]) << 8) |
               (uint64_t)bytes[7];

    return true;
}

static bool read_int64(int64_t *value)
{
    uint64_t uvalue;
    if (!read_uint64(&uvalue))
        return false;

    *value = (int64_t)uvalue;
    return true;
}

static bool read_uint32(uint32_t *value)
{
    unsigned char bytes[4];
    if (!read_bytes(bytes, 4))
        return false;

    *value = (((uint32_t)bytes[0]) << 24) |
             (((uint32_t)bytes[1]) << 16) |
             (((uint32_t)bytes[2]) << 8) |
               (uint32_t)bytes[3];

    return true;
}

static bool read_int32(int32_t *value)
{
    uint32_t uvalue;
    if (!read_uint32(&uvalue))
        return false;

    *value = (int32_t)uvalue;
    return true;
}

static bool read_uint24(uint32_t *value)
{
    unsigned char bytes[3];
    if (!read_bytes(bytes, 3))
        return false;

    *value = (((uint32_t)bytes[0]) << 16) |
             (((uint32_t)bytes[1]) << 8) |
               (uint32_t)bytes[2];

    return true;
}

static bool read_int24(int32_t *value)
{
    uint32_t uvalue;
    if (!read_uint24(&uvalue))
        return false;

    *value = (int32_t)uvalue;
    return true;
}

static bool read_uint16(uint16_t *value)
{
    unsigned char bytes[2];
    if (!read_bytes(bytes, 2))
        return false;

    *value = (((uint16_t)bytes[0]) << 8) |
               (uint16_t)bytes[1];

    return true;
}

static bool read_int16(int16_t *value)
{
    uint16_t uvalue;
    if (!read_uint16(&uvalue))
        return false;

    *value = (int16_t)uvalue;
    return true;
}

static bool read_uint8(uint8_t *value)
{
    unsigned char bytes[1];
    if (!read_bytes(bytes, 1))
        return false;

    *value = bytes[0];

    return true;
}

static bool read_int8(int8_t *value)
{
    uint8_t uvalue;
    if (!read_uint8(&uvalue))
        return false;

    *value = (int8_t)uvalue;
    return true;
}

static bool read_type(char *type)
{
    uint32_t value;
    if (!read_uint32(&value))
        return false;

    type[0] = (char)((value >> 24) & 0xff);
    type[1] = (char)((value >> 16) & 0xff);
    type[2] = (char)((value >> 8) & 0xff);
    type[3] = (char)(value & 0xff);

    return true;
}

static bool read_atom_start()
{
    MOVAtom &atom = CURRENT_ATOM;
    atom.size = 8;
    atom.rem_size = 8;
    atom.offset = g_file_offset;

    uint32_t uint32_size;
    uint64_t uint64_size;

    if (!read_uint32(&uint32_size))
        return false; // end-of-file

    MOV_CHECK(read_type(atom.type));
    MOV_CHECK(uint32_size == 1 || uint32_size >= 8);
    if (uint32_size == 1) {
        // extended size
        atom.size += 8;
        atom.rem_size += 8;
        MOV_CHECK(read_uint64(&uint64_size));
    } else {
        uint64_size = uint32_size;
    }

    atom.rem_size = uint64_size - atom.size;
    atom.size = uint64_size;

    return true;
}

static bool read_matrix(uint32_t *value)
{
    int i;
    for (i = 0; i < 9; i++)
        MOV_CHECK(read_uint32(&value[i]));

    return true;
}


static void indent_atom_header()
{
    size_t i;
    for (i = 1; i < g_atoms.size(); i++)
        printf(ATOM_INDENT);
}

static void indent(int extra_amount = 0)
{
    int i;
    for (i = 1; i < (int)g_atoms.size(); i++)
        printf(ATOM_INDENT);
    printf(ATOM_VALUE_INDENT);
    for (i = 0; i < extra_amount; i++)
        printf(" ");
}

static void dump_uint64_index(uint64_t count, uint64_t index)
{
    if (count < 0xffff)
        printf("%04"PRIx64, index);
    else if (count < 0xffffff)
        printf("%06"PRIx64, index);
    else if (count < 0xffffffff)
        printf("%08"PRIx64, index);
    else
        printf("%016"PRIx64, index);
}

static void dump_uint32_index(uint32_t count, uint32_t index)
{
    if (count < 0xffff)
        printf("%04x", index);
    else if (count < 0xffffff)
        printf("%06x", index);
    else
        printf("%08x", index);
}

static void dump_inline_bytes(unsigned char *bytes, uint32_t size)
{
    printf("(size %u) ", size);

    uint32_t i;
    for (i = 0; i < size; i++)
        printf(" %02x", bytes[i]);

    printf("  |");
    for (i = 0; i < size; i++) {
        if (isprint(bytes[i]))
            printf("%c", bytes[i]);
        else
            printf(".");
    }
    printf("|");
}

static void dump_bytes_line(uint64_t size, uint64_t offset, unsigned char *line, uint32_t line_size)
{
    dump_uint64_index(size, offset);
    printf("  ");

    uint32_t i;
    for (i = 0; i < line_size; i++) {
        if (i == 8)
            printf(" ");
        printf(" %02x", line[i]);
    }
    for (; i < 16; i++) {
        if (i == 8)
            printf(" ");
        printf("   ");
    }

    printf("  |");
    for (i = 0; i < line_size; i++) {
        if (isprint(line[i]))
            printf("%c", line[i]);
        else
            printf(".");
    }
    printf("|");
}

static void dump_bytes(uint64_t size, int extra_indent_amount = 0)
{
    if (size == 0)
        return;


    indent(extra_indent_amount);

    unsigned char buffer[16];
    uint64_t total_read = 0;
    uint32_t num_read;
    while (total_read < size) {
        num_read = 16;
        if (total_read + num_read > size)
            num_read = size - total_read;
        MOV_CHECK(read_bytes(buffer, num_read));

        if (total_read > 0) {
            printf("\n");
            indent(extra_indent_amount);
        }

        dump_bytes_line(size, total_read, buffer, num_read);

        total_read += num_read;
    }

    printf("\n");
}

static void dump_bytes(unsigned char *bytes, uint64_t size, int extra_indent_amount = 0)
{
    indent(extra_indent_amount);

    uint64_t num_lines = size / 16;
    uint64_t i;
    for (i = 0; i < num_lines; i++) {
        if (i > 0) {
            printf("\n");
            indent(extra_indent_amount);
        }

        dump_bytes_line(size, i * 16, &bytes[i * 16], 16);
    }

    if (num_lines > 0)
        printf("\n");

    if ((size % 16) > 0) {
        if (num_lines > 0)
            indent(extra_indent_amount);
        dump_bytes_line(size, num_lines * 16, &bytes[num_lines * 16], (size % 16));
        printf("\n");
    }
}

static void dump_string(uint32_t size, int extra_indent_amount = 0)
{
    if (size == 0) {
        printf("\n");
        return;
    }

    if (size > 256) {
        printf("\n");
        indent(extra_indent_amount);
        dump_bytes(size, extra_indent_amount);
        return;
    }

    unsigned char buffer[256];
    MOV_CHECK(read_bytes(buffer, size));

    uint32_t i;
    for (i = 0; i < size; i++) {
        if (!isprint(buffer[i]))
            break;
    }
    if (i < size) {
        for (; i < size; i++) {
            if (buffer[i] != '\0')
                break;
        }
        if (i < size) {
            printf("\n");
            dump_bytes(buffer, size, extra_indent_amount);
            return;
        }
    }

    printf("'");
    for (i = 0; i < size; i++) {
        if (buffer[i] == '\0')
            break;

        printf("%c", buffer[i]);
    }
    printf("'");
    if (i < size) {
        printf(" +");
        for (; i < size; i++)
            printf(" 0x00");
    }
    printf("\n");
}

static void dump_type(const char *type)
{
    size_t i;
    for (i = 0; i < 4; i++)
        printf("%c", type[i]);
}

static void dump_uint32_tag(uint32_t value)
{
    int i;
    for (i = 3; i >= 0; i--)
        printf("%c", (value >> (8 * i)) & 0xff);
}

static void dump_file_size(uint64_t value)
{
    if (g_file_size_64bit)
        printf("%20"PRIu64" (0x%016"PRIx64")", value, value);
    else
        printf("%10u (0x%08x)", (uint32_t)value, (uint32_t)value);
}

static void dump_uint64_size(uint64_t value)
{
    printf("%20"PRIu64" (0x%016"PRIx64")", value, value);
}

static void dump_uint32_size(uint32_t value)
{
    printf("%10u (0x%08x)", value, value);
}

#if 0
static void dump_uint64(uint64_t value, bool hex)
{
    if (hex)
        printf("0x%016"PRIx64, value);
    else
        printf("%20"PRIu64, value);
}
#endif

static void dump_uint32(uint32_t value, bool hex)
{
    if (hex)
        printf("0x%08x", value);
    else
        printf("%10u", value);
}

static void dump_int32(uint32_t value)
{
    printf("%10d", value);
}

static void dump_uint16(uint16_t value, bool hex)
{
    if (hex)
        printf("0x%04x", value);
    else
        printf("%5u", value);
}

static void dump_uint32_chars(uint32_t value)
{
    int i;
    for (i = 3; i >= 0; i--) {
        unsigned char c = (value >> (8 * i)) & 0xff;
        if (isprint(c))
            printf("%c", c);
        else
            printf(".");
    }
    printf(" (");
    for (i = 3; i >= 0; i--) {
        unsigned char c = (value >> (8 * i)) & 0xff;
        if (i != 3)
            printf(" ");
        printf("%02x", c);
    }
    printf(")");
}

static void dump_uint32_fp(uint32_t value, uint8_t bits_left)
{
    printf("%f", value / (double)(1 << (32 - bits_left)));
}

static void dump_uint16_fp(uint16_t value, uint8_t bits_left)
{
    printf("%f", value / (double)(1 << (16 - bits_left)));
}

static void dump_int16_fp(int16_t value, uint8_t bits_left)
{
    printf("%f", value / (double)(1 << (16 - bits_left)));
}

static void dump_timestamp(uint64_t value)
{
    // 2082844800 = difference between Unix epoch (1970-01-01) and Apple epoch (1904-01-01)
    time_t unix_secs = (time_t)(value - 2082844800);
    struct tm *utc;
    utc = gmtime(&unix_secs);
    if (utc == 0) {
        printf("%"PRIu64" seconds since 1904-01-01", value);
    } else {
        printf("%04d-%02d-%02dT%02d:%02d:%02dZ (%"PRIu64" sec since 1904-01-01)",
               utc->tm_year + 1900, utc->tm_mon, utc->tm_mday,
               utc->tm_hour, utc->tm_min, utc->tm_sec,
               value);
    }
}

static void dump_matrix(uint32_t *matrix, int extra_indent_amount = 0)
{
    // matrix:
    //    a  b  u
    //    c  d  v
    //    tx ty w
    //
    // order is: a, b, u, c, d, v, tx, ty, w
    // all are fixed point 16.16, except u, v and w which are 2.30, hence w = 0x40000000 (1.0)

    int i, j;
    for (i = 0; i < 3; i++) {
        indent(extra_indent_amount);
        for (j = 0; j < 3; j++) {
            if (j != 0)
                printf(" ");
            if (j == 2)
                dump_uint32_fp(matrix[i * 3 + j], 2);
            else
                dump_uint32_fp(matrix[i * 3 + j], 16);
        }
        printf("\n");
    }
}

static void dump_color(uint16_t red, uint16_t green, uint16_t blue)
{
    printf("RGB(0x%04x,0x%04x,0x%04x)", red, green, blue);
}

static void dump_atom_header()
{
    indent_atom_header();
    dump_type(CURRENT_ATOM.type);
    printf(": s=");
    dump_file_size(CURRENT_ATOM.size);
    printf(", o=");
    dump_file_size(CURRENT_ATOM.offset);
    printf("\n");
}

static void dump_atom()
{
    dump_atom_header();

    if (CURRENT_ATOM.rem_size > 0)
        dump_bytes(CURRENT_ATOM.rem_size);
}

static void dump_child_atom(const DumpFuncMap *dump_func_map, size_t dump_func_map_size)
{
    size_t i;
    for (i = 0; i < dump_func_map_size; i++) {
        if (dump_func_map[i].type[0] == '\0' || // any type
            memcmp(dump_func_map[i].type, CURRENT_ATOM.type, 4) == 0)
        {
            dump_func_map[i].dump();
            if (CURRENT_ATOM.rem_size > 0) {
                indent();
                printf("remainder...: %"PRIu64" unparsed bytes\n", CURRENT_ATOM.rem_size);
                dump_bytes(CURRENT_ATOM.rem_size, 2);
            }
            break;
        }
    }
    if (i >= dump_func_map_size)
        dump_atom();
}

static void dump_container_atom(const DumpFuncMap *dump_func_map, size_t dump_func_map_size)
{
    dump_atom_header();

    while (CURRENT_ATOM.rem_size > 0) {
        push_atom();

        if (!read_atom_start())
            break;

        dump_child_atom(dump_func_map, dump_func_map_size);

        pop_atom();
    }
}

static void dump_full_atom_header(uint8_t *version, uint32_t *flags, bool newline_flags = true)
{
    dump_atom_header();

    MOV_CHECK(read_uint8(version));
    indent();
    printf("version: %u\n", (*version));

    MOV_CHECK(read_uint24(flags));
    indent();
    printf("flags: 0x%06x", (*flags));
    if (newline_flags)
        printf("\n");
}

static void dump_unknown_version(uint8_t version)
{
    indent();
    printf("remainder...: unknown version %u, %"PRIu64" unparsed bytes\n", version, CURRENT_ATOM.rem_size);
    dump_bytes(CURRENT_ATOM.rem_size, 2);
}


static void dump_ftyp_atom()
{
    dump_atom_header();

    uint32_t major_brand;
    MOV_CHECK(read_uint32(&major_brand));
    indent();
    printf("major_brand: ");
    dump_uint32_chars(major_brand);
    printf("\n");

    uint32_t minor_version;
    MOV_CHECK(read_uint32(&minor_version));
    indent();
    printf("minor_version: ");
    dump_uint32(minor_version, true);
    printf("\n");

    bool first = true;
    uint32_t compatible_brand;
    indent();
    printf("compatible_brands: ");
    while (CURRENT_ATOM.rem_size >= 4) {
        MOV_CHECK(read_uint32(&compatible_brand));
        if (!first)
            printf(", ");
        else
            first = false;
        dump_uint32_chars(compatible_brand);
    }
    printf("\n");
}

static void dump_mdat_atom()
{
    dump_atom_header();

    if (CURRENT_ATOM.rem_size > 0) {
        indent();
        printf("...skipped %"PRIu64" bytes\n", CURRENT_ATOM.rem_size);
        skip_bytes(CURRENT_ATOM.rem_size);
    }
}

static void dump_free_atom()
{
    dump_atom_header();

    if (CURRENT_ATOM.rem_size > 0) {
        indent();
        printf("...skipped %"PRIu64" bytes\n", CURRENT_ATOM.rem_size);
        skip_bytes(CURRENT_ATOM.rem_size);
    }
}

static void dump_skip_atom()
{
    dump_atom_header();

    if (CURRENT_ATOM.rem_size > 0) {
        indent();
        printf("...skipped %"PRIu64" bytes\n", CURRENT_ATOM.rem_size);
        skip_bytes(CURRENT_ATOM.rem_size);
    }
}

static void dump_dref_child_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags, false);
    if ((flags & 0x000001))
        printf(" (self reference)\n");
    else
        printf("\n");

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    if (equals_type(CURRENT_ATOM.type, "url ")) {
        indent();
        printf("data: (%"PRIu64" bytes) url: ", CURRENT_ATOM.rem_size);
        dump_string(CURRENT_ATOM.rem_size);
        return;
    } else {
        indent();
        printf("data: (%"PRIu64" bytes)\n", CURRENT_ATOM.rem_size);
        dump_bytes(CURRENT_ATOM.rem_size, 2);
        return;
    }
}

static void dump_dref_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        push_atom();

        if (!read_atom_start())
            break;

        dump_dref_child_atom();

        pop_atom();
    }
}

static void dump_stts_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("       count   duration\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint32_t sample_count;
        MOV_CHECK(read_uint32(&sample_count));
        uint32_t sample_duration;
        MOV_CHECK(read_uint32(&sample_duration));

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        dump_uint32(sample_count, true);
        printf(" ");
        dump_uint32(sample_duration, true);
        printf("\n");
    }
}

static void dump_ctts_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("       count     offset\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint32_t sample_count;
        MOV_CHECK(read_uint32(&sample_count));
        int32_t sample_offset;
        MOV_CHECK(read_int32(&sample_offset));

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        dump_uint32(sample_count, true);
        printf(" ");
        dump_int32(sample_offset);
        printf("\n");
    }
}

static void dump_cslg_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    int32_t dts_shift;
    MOV_CHECK(read_int32(&dts_shift));
    indent();
    printf("dts_shift: %d\n", dts_shift);

    int32_t min_cts;
    MOV_CHECK(read_int32(&min_cts));
    indent();
    printf("min_cts: %d\n", min_cts);

    int32_t max_cts;
    MOV_CHECK(read_int32(&max_cts));
    indent();
    printf("max_cts: %d\n", max_cts);

    int32_t pts_start;
    MOV_CHECK(read_int32(&pts_start));
    indent();
    printf("pts_start: %d\n", pts_start);

    int32_t pts_end;
    MOV_CHECK(read_int32(&pts_end));
    indent();
    printf("pts_end: %d\n", pts_end);
}

static void dump_stss_stps_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("      sample\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint32_t sample;
        MOV_CHECK(read_uint32(&sample));

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        dump_uint32(sample, true);
        printf("\n");
    }
}

static void dump_sdtp_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries = CURRENT_ATOM.rem_size;
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("      reserved  depends  dependent  redundancy\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint8_t sample;
        MOV_CHECK(read_uint8(&sample));

        // NOTE: not sure whether this is correct for qt
        // sample files had reserved == 1 for I and P-frames, and depends_on only 2 when I-frame
        uint8_t reserved = (sample & 0xc0) >> 6;
        uint8_t depends_on = (sample & 0x30) >> 4;
        uint8_t dependent_on = (sample & 0x0c) >> 2;
        uint8_t has_redundancy = (sample & 0x03);

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        printf("           %d        %d          %d           %d\n",
               reserved, depends_on, dependent_on, has_redundancy);
    }
}

static void dump_stsc_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("  first chunk  samples-per-chunk         descr. id\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint32_t first_chunk;
        MOV_CHECK(read_uint32(&first_chunk));
        uint32_t samples_per_chunk;
        MOV_CHECK(read_uint32(&samples_per_chunk));
        uint32_t sample_description_id;
        MOV_CHECK(read_uint32(&sample_description_id));

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        printf(" ");
        dump_uint32(first_chunk, true);
        printf("         ");
        dump_uint32(samples_per_chunk, true);
        printf("        ");
        dump_uint32(sample_description_id, false);
        printf("\n");
    }
}

static void dump_stsz_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t sample_size;
    MOV_CHECK(read_uint32(&sample_size));
    indent();
    printf("sample_size: %d\n", sample_size);

    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    if (CURRENT_ATOM.rem_size == 0) {
        if (num_entries > 0) {
            indent(4);
            printf("...none\n");
        }
        MOV_CHECK(sample_size > 0);
        return;
    }

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("         size\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        int32_t size;
        MOV_CHECK(read_int32(&size));

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        printf(" ");
        dump_uint32(size, true);
        printf("\n");
    }
}

static void dump_stco_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("      offset (hex offset)\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint32_t offset;
        MOV_CHECK(read_uint32(&offset));

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        dump_uint32_size(offset);
        printf("\n");
    }
}

static void dump_co64_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("entries (");
    dump_uint32(num_entries, false);
    printf("):\n");

    indent(4);
    if (num_entries < 0xffff)
        printf("   i");
    else if (num_entries < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("                offset         (hex offset)\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint64_t offset;
        MOV_CHECK(read_uint64(&offset));

        indent(4);
        dump_uint32_index(num_entries, i);
        printf("  ");

        dump_uint64_size(offset);
        printf("\n");
    }
}

static void dump_hdlr_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t component_type;
    MOV_CHECK(read_uint32(&component_type));
    indent();
    printf("component_type: ");
    dump_uint32_tag(component_type);
    printf("\n");

    uint32_t component_sub_type;
    MOV_CHECK(read_uint32(&component_sub_type));
    indent();
    printf("component_sub_type: ");
    dump_uint32_tag(component_sub_type);
    printf("\n");

    if (HAVE_PREV_ATOM && strncmp(PREV_ATOM.type, "mdia", 4) == 0) {
        g_component_type = component_type;
        g_component_sub_type = component_sub_type;
    }

    uint32_t component_manufacturer;
    MOV_CHECK(read_uint32(&component_manufacturer));
    indent();
    printf("component_manufacturer: %u\n", component_manufacturer);

    uint32_t component_flags;
    MOV_CHECK(read_uint32(&component_flags));
    indent();
    printf("component_flags: 0x%08x\n", component_flags);

    uint32_t component_flags_mask;
    MOV_CHECK(read_uint32(&component_flags_mask));
    indent();
    printf("component_flags_mask: 0x%08x\n", component_flags_mask);

    uint8_t component_name_len;
    MOV_CHECK(read_uint8(&component_name_len));
    indent();
    printf("component_name: ");
    if (component_name_len == 0) {
        printf("\n");
    } else {
        dump_string(component_name_len, 2);
    }
}

static void dump_stsd_tmcd_name_atom()
{
    dump_atom_header();

    uint16_t len;
    MOV_CHECK(read_uint16(&len));
    uint16_t language_code;
    MOV_CHECK(read_uint16(&language_code));

    indent();
    printf("value: (len=%d,lang=0x%04x) ", len, language_code);
    dump_string(len, 2);
}

static void dump_colr_atom()
{
    dump_atom_header();

    uint32_t color_param_type;
    MOV_CHECK(read_uint32(&color_param_type));
    indent();
    printf("color_param_type: ");
    dump_uint32_tag(color_param_type);
    printf("\n");

    if (color_param_type == MKTAG("nclc")) {
        uint16_t primaries;
        MOV_CHECK(read_uint16(&primaries));
        indent();
        printf("primaries: %u\n", primaries);

        uint16_t transfer_func;
        MOV_CHECK(read_uint16(&transfer_func));
        indent();
        printf("transfer_func: %u\n", transfer_func);

        uint16_t matrix;
        MOV_CHECK(read_uint16(&matrix));
        indent();
        printf("matrix: %u\n", matrix);
    }
}

static void dump_fiel_atom()
{
    dump_atom_header();

    uint8_t fields;
    MOV_CHECK(read_uint8(&fields));
    indent();
    printf("fields: %u", fields);
    if (fields == 1)
        printf(" (progressive)\n");
    else if (fields == 2)
        printf(" (interlaced)\n");
    else
        printf("\n");

    uint8_t detail;
    MOV_CHECK(read_uint8(&detail));
    indent();
    printf("detail: %u\n", detail);
}

static void dump_pasp_atom()
{
    dump_atom_header();

    int32_t h_spacing;
    MOV_CHECK(read_int32(&h_spacing));
    indent();
    printf("h_spacing: %d\n", h_spacing);

    int32_t v_spacing;
    MOV_CHECK(read_int32(&v_spacing));
    indent();
    printf("v_spacing: %d\n", v_spacing);
}

static void dump_clap_atom()
{
    dump_atom_header();

    int32_t clean_ap_width_num, clean_ap_width_den;
    MOV_CHECK(read_int32(&clean_ap_width_num));
    MOV_CHECK(read_int32(&clean_ap_width_den));
    indent();
    printf("clean_aperture_width: %d/%d\n", clean_ap_width_num, clean_ap_width_den);

    int32_t clean_ap_height_num, clean_ap_height_den;
    MOV_CHECK(read_int32(&clean_ap_height_num));
    MOV_CHECK(read_int32(&clean_ap_height_den));
    indent();
    printf("clean_aperture_height: %d/%d\n", clean_ap_height_num, clean_ap_height_den);

    int32_t horiz_offset_num, horiz_offset_den;
    MOV_CHECK(read_int32(&horiz_offset_num));
    MOV_CHECK(read_int32(&horiz_offset_den));
    indent();
    printf("horiz_offset: %d/%d\n", horiz_offset_num, horiz_offset_den);

    int32_t vert_offset_num, vert_offset_den;
    MOV_CHECK(read_int32(&vert_offset_num));
    MOV_CHECK(read_int32(&vert_offset_den));
    indent();
    printf("vert_offset: %d/%d\n", vert_offset_num, vert_offset_den);
}

static void dump_stbl_vide()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'c','o','l','r'}, dump_colr_atom},
        {{'f','i','e','l'}, dump_fiel_atom},
        {{'p','a','s','p'}, dump_pasp_atom},
        {{'c','l','a','p'}, dump_clap_atom},
    };

    uint16_t version;
    MOV_CHECK(read_uint16(&version));
    indent(2);
    printf("version: %u\n", version);

    uint16_t revision;
    MOV_CHECK(read_uint16(&revision));
    indent(2);
    printf("revision: 0x%04x\n", revision);

    uint32_t vendor;
    MOV_CHECK(read_uint32(&vendor));
    indent(2);
    printf("vendor: ");
    dump_uint32_chars(vendor);
    printf("\n");

    uint32_t temporal_quality;
    MOV_CHECK(read_uint32(&temporal_quality));
    indent(2);
    printf("temporal_quality: 0x%08x\n", temporal_quality);

    uint32_t spatial_quality;
    MOV_CHECK(read_uint32(&spatial_quality));
    indent(2);
    printf("spatial_quality: 0x%08x\n", spatial_quality);

    uint16_t width;
    MOV_CHECK(read_uint16(&width));
    indent(2);
    printf("width: %u\n", width);

    uint16_t height;
    MOV_CHECK(read_uint16(&height));
    indent(2);
    printf("height: %u\n", height);

    uint32_t horizontal_resolution;
    MOV_CHECK(read_uint32(&horizontal_resolution));
    indent(2);
    printf("horizontal_resolution: ");
    dump_uint32_fp(horizontal_resolution, 16);
    printf("\n");

    uint32_t vertical_resolution;
    MOV_CHECK(read_uint32(&vertical_resolution));
    indent(2);
    printf("vertical_resolution: ");
    dump_uint32_fp(vertical_resolution, 16);
    printf("\n");

    uint32_t data_size;
    MOV_CHECK(read_uint32(&data_size));
    indent(2);
    printf("data_size: %u\n", data_size);

    uint16_t frame_count;
    MOV_CHECK(read_uint16(&frame_count));
    indent(2);
    printf("frame_count: %u\n", frame_count);

    uint8_t compressor_name_len;
    MOV_CHECK(read_uint8(&compressor_name_len));
    MOV_CHECK(compressor_name_len - 1 <= 32);
    indent(2);
    printf("compressor_name: ");
    dump_string(31, 4);

    uint16_t depth;
    MOV_CHECK(read_uint16(&depth));
    indent(2);
    printf("depth: %u\n", depth);

    uint16_t color_table_id;
    MOV_CHECK(read_uint16(&color_table_id));
    indent(2);
    printf("color_table_id: 0x%04x\n", color_table_id);

    // extensions
    while (CURRENT_ATOM.rem_size > 8) {
        push_atom();

        if (!read_atom_start())
            break;

        dump_child_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);

        pop_atom();
    }
}

static void dump_stbl_soun()
{
    if (CURRENT_ATOM.rem_size > 0)
        dump_bytes(CURRENT_ATOM.rem_size);
}

static void dump_stbl_tmcd()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'n','a','m','e'}, dump_stsd_tmcd_name_atom},
    };

    uint32_t reserved1;
    MOV_CHECK(read_uint32(&reserved1));
    indent(2);
    printf("reserved: 0x%08x\n", reserved1);

    uint32_t flags;
    MOV_CHECK(read_uint32(&flags));
    indent(2);
    printf("flags: 0x%08x\n", flags);

    uint32_t timescale;
    MOV_CHECK(read_uint32(&timescale));
    indent(2);
    printf("timescale: %u\n", timescale);

    int32_t frame_duration;
    MOV_CHECK(read_int32(&frame_duration));
    indent(2);
    printf("frame_duration: %d (%f sec)\n", frame_duration, frame_duration / (double)(timescale));

    uint8_t number_of_frames;
    MOV_CHECK(read_uint8(&number_of_frames));
    indent(2);
    printf("number_of_frames: %u\n", number_of_frames);

    uint8_t reserved2;
    MOV_CHECK(read_uint8(&reserved2));
    indent(2);
    printf("reserved: 0x%02x\n", reserved2);

    // extensions
    while (CURRENT_ATOM.rem_size > 8) {
        push_atom();

        if (!read_atom_start())
            break;

        dump_child_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);

        pop_atom();
    }
}

static void dump_stsd_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t num_entries;
    MOV_CHECK(read_uint32(&num_entries));
    indent();
    printf("sample_descriptions: (");
    dump_uint32(num_entries, true);
    printf(")\n");

    uint32_t i;
    for (i = 0; i < num_entries; i++) {
        uint32_t size;
        MOV_CHECK(read_uint32(&size));
        indent(2);
        printf("size: %08x\n", size);
        MOV_CHECK(size >= 16);

        uint32_t data_format;
        MOV_CHECK(read_uint32(&data_format));
        indent(2);
        printf("data_format: ");
        dump_uint32_chars(data_format);
        printf("\n");

        unsigned char reserved[6];
        MOV_CHECK(read_bytes(reserved, 6));
        indent(2);
        printf("reserved: ");
        dump_inline_bytes(reserved, 6);
        printf("\n");

        uint16_t data_ref_index;
        MOV_CHECK(read_uint16(&data_ref_index));
        indent(2);
        printf("data_ref_index: 0x%04x\n", data_ref_index);

        if (g_component_type == MHLR_COMPONENT_TYPE && g_component_sub_type == VIDE_COMPONENT_SUB_TYPE) {
            dump_stbl_vide();
        } else if (g_component_type == MHLR_COMPONENT_TYPE && g_component_sub_type == SOUN_COMPONENT_SUB_TYPE) {
            dump_stbl_soun();
        } else if (g_component_type == MHLR_COMPONENT_TYPE && g_component_sub_type == TMCD_COMPONENT_SUB_TYPE) {
            dump_stbl_tmcd();
        } else {
            indent(2);
            printf("remainder...: %u unparsed bytes\n", size - 16);
            dump_bytes(size - 16, 4);
        }
    }
}

static void dump_stbl_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'s','t','s','d'}, dump_stsd_atom},
        {{'s','t','t','s'}, dump_stts_atom},
        {{'c','t','t','s'}, dump_ctts_atom},
        {{'c','s','l','g'}, dump_cslg_atom},
        {{'s','t','s','s'}, dump_stss_stps_atom},
        {{'s','t','p','s'}, dump_stss_stps_atom},
        {{'s','d','t','p'}, dump_sdtp_atom},
        {{'s','t','s','c'}, dump_stsc_atom},
        {{'s','t','s','z'}, dump_stsz_atom},
        {{'s','t','c','o'}, dump_stco_atom},
        {{'c','o','6','4'}, dump_co64_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_dinf_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'d','r','e','f'}, dump_dref_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_vmhd_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags, false);
    if ((flags & 0x0001))
        printf(" (no lean ahead)");
    printf("\n");

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint16_t graphics_mode;
    MOV_CHECK(read_uint16(&graphics_mode));
    indent();
    printf("graphics_mode: %02x\n", graphics_mode);

    uint16_t opcolor_r, opcolor_g, opcolor_b;
    MOV_CHECK(read_uint16(&opcolor_r));
    MOV_CHECK(read_uint16(&opcolor_g));
    MOV_CHECK(read_uint16(&opcolor_b));
    indent();
    printf("opcolor: ");
    dump_color(opcolor_r, opcolor_g, opcolor_b);
    printf("\n");
}

static void dump_smhd_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    int16_t balance;
    MOV_CHECK(read_int16(&balance));
    indent();
    printf("balance: ");
    dump_int16_fp(balance, 8);
    printf("\n");

    uint16_t reserved;
    MOV_CHECK(read_uint16(&reserved));
    indent();
    printf("reserved: ");
    dump_uint16(reserved, true);
    printf("\n");
}

static void dump_gmin_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags, false);
    if ((flags & 0x0001))
        printf(" (no lean ahead)");
    printf("\n");

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint16_t graphics_mode;
    MOV_CHECK(read_uint16(&graphics_mode));
    indent();
    printf("graphics_mode: %02x\n", graphics_mode);

    uint16_t opcolor_r, opcolor_g, opcolor_b;
    MOV_CHECK(read_uint16(&opcolor_r));
    MOV_CHECK(read_uint16(&opcolor_g));
    MOV_CHECK(read_uint16(&opcolor_b));
    indent();
    printf("opcolor: ");
    dump_color(opcolor_r, opcolor_g, opcolor_b);
    printf("\n");

    int16_t balance;
    MOV_CHECK(read_int16(&balance));
    indent();
    printf("balance: ");
    dump_int16_fp(balance, 8);
    printf("\n");

    uint16_t reserved;
    MOV_CHECK(read_uint16(&reserved));
    indent();
    printf("reserved: ");
    dump_uint16(reserved, true);
    printf("\n");
}

static void dump_tcmi_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags, false);
    if ((flags & 0x0001))
        printf(" (no lean ahead)");
    printf("\n");

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint16_t text_font;
    MOV_CHECK(read_uint16(&text_font));
    indent();
    printf("text_font: %02x\n", text_font);

    uint16_t text_face;
    MOV_CHECK(read_uint16(&text_face));
    indent();
    printf("text_face: %02x\n", text_face);

    uint32_t text_size;
    MOV_CHECK(read_uint32(&text_size));
    indent();
    printf("text_size: ");
    dump_uint32_fp(text_size, 16);
    printf("\n");

    uint16_t text_color_r, text_color_g, text_color_b;
    MOV_CHECK(read_uint16(&text_color_r));
    MOV_CHECK(read_uint16(&text_color_g));
    MOV_CHECK(read_uint16(&text_color_b));
    indent();
    printf("text_color: ");
    dump_color(text_color_r, text_color_g, text_color_b);
    printf("\n");

    uint16_t bg_color_r, bg_color_g, bg_color_b;
    MOV_CHECK(read_uint16(&bg_color_r));
    MOV_CHECK(read_uint16(&bg_color_g));
    MOV_CHECK(read_uint16(&bg_color_b));
    indent();
    printf("background_color: ");
    dump_color(bg_color_r, bg_color_g, bg_color_b);
    printf("\n");

    uint8_t font_name_size;
    MOV_CHECK(read_uint8(&font_name_size));
    indent();
    printf("font_name: ");
    dump_string(font_name_size, 2);
}

static void dump_tmcd_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'t','c','m','i'}, dump_tcmi_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_gmhd_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'g','m','i','n'}, dump_gmin_atom},
        {{'t','m','c','d'}, dump_tmcd_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_minf_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'v','m','h','d'}, dump_vmhd_atom},
        {{'s','m','h','d'}, dump_smhd_atom},
        {{'g','m','h','d'}, dump_gmhd_atom},
        {{'h','d','l','r'}, dump_hdlr_atom},
        {{'d','i','n','f'}, dump_dinf_atom},
        {{'s','t','b','l'}, dump_stbl_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_mdhd_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00 && version != 0x01) {
        dump_unknown_version(version);
        return;
    }


    if (version == 0x00) {
        uint32_t creation_time;
        MOV_CHECK(read_uint32(&creation_time));
        indent();
        printf("creation_time: ");
        dump_timestamp(creation_time);
        printf("\n");

        uint32_t modification_time;
        MOV_CHECK(read_uint32(&modification_time));
        indent();
        printf("modification_time: ");
        dump_timestamp(modification_time);
        printf("\n");
    } else {
        uint64_t creation_time;
        MOV_CHECK(read_uint64(&creation_time));
        indent();
        printf("creation_time: ");
        dump_timestamp(creation_time);
        printf("\n");

        uint64_t modification_time;
        MOV_CHECK(read_uint64(&modification_time));
        indent();
        printf("modification_time: ");
        dump_timestamp(modification_time);
        printf("\n");
    }

    uint32_t timescale;
    MOV_CHECK(read_uint32(&timescale));
    indent();
    printf("timescale: %u\n", timescale);

    if (version == 0x00) {
        int32_t duration;
        MOV_CHECK(read_int32(&duration));
        indent();
        printf("duration: %d (%f sec)\n", duration, duration / (double)(timescale));
    } else {
        int64_t duration;
        MOV_CHECK(read_int64(&duration));
        indent();
        printf("duration: %"PRId64" (%f sec)\n", duration, duration / (double)(timescale));
    }

    uint16_t language;
    MOV_CHECK(read_uint16(&language));
    indent();
    printf("language: %u\n", language);

    uint16_t quality;
    MOV_CHECK(read_uint16(&quality));
    indent();
    printf("quality: %u\n", quality);
}

static void dump_mdia_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'m','d','h','d'}, dump_mdhd_atom},
        {{'h','d','l','r'}, dump_hdlr_atom},
        {{'m','i','n','f'}, dump_minf_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_keys_atom()
{
    static uint32_t mdta_key_namespace = MKTAG("mdta");

    g_meta_keys.clear();

    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t count;
    MOV_CHECK(read_uint32(&count));

    indent();
    printf("key_values (");
    dump_uint32(count, false);
    printf("):\n");

    unsigned char key_value_buffer[129];
    uint32_t total_count = 0;
    for (total_count = 0; total_count < count; total_count++) {
        indent(2);

        uint32_t key_size;
        uint32_t key_value_size;
        MOV_CHECK(read_uint32(&key_size));
        MOV_CHECK(key_size >= 8);
        uint32_t key_namespace;
        MOV_CHECK(read_uint32(&key_namespace));

        printf("%4u  ", total_count + 1);
        dump_uint32(key_size, true);

        key_value_size = key_size - 8;

        if (key_namespace == mdta_key_namespace && key_value_size < sizeof(key_value_buffer)) {
            MOV_CHECK(read_bytes(key_value_buffer, key_value_size));

            size_t i;
            for (i = 0; i < key_value_size; i++) {
                if (!isprint(key_value_buffer[i]))
                    break;
            }
            if (i >= key_value_size) {
                key_value_buffer[key_value_size] = '\0';
                printf("  mdta  '%s'\n", (char*)key_value_buffer);
                g_meta_keys.push_back((char*)key_value_buffer);
            } else {
                printf("  ");
                dump_uint32_chars(key_namespace);
                printf("\n");
                dump_bytes(key_value_buffer, key_value_size, 4);
                g_meta_keys.push_back("");
            }
        } else {
            printf("  ");
            dump_uint32_chars(key_namespace);
            printf("\n");
            dump_bytes(key_value_size, 4);
            g_meta_keys.push_back("");
        }
    }
}

static void dump_ilst_data_atom()
{
    dump_atom_header();

    uint8_t type_field_1;
    uint32_t type_field_2;
    MOV_CHECK(read_uint8(&type_field_1));
    MOV_CHECK(read_uint24(&type_field_2));

    indent();
    printf("type 1: %u\n", type_field_1);
    indent();
    printf("type 2: %u\n", type_field_2);

    uint16_t locale;
    uint16_t country;
    MOV_CHECK(read_uint16(&locale));
    MOV_CHECK(read_uint16(&country));

    indent();
    printf("locale: %u\n", locale);
    indent();
    printf("country: %u\n", country);

    indent();
    if (type_field_1 == 0 &&
        (type_field_2 == 1 ||    // utf-8
         type_field_2 == 21 ||   // be signed integer
         type_field_2 == 22))    // be unsigned integer
    {
        if (type_field_2 == 21) {
            if (CURRENT_ATOM.rem_size == 8) {
                int64_t value;
                MOV_CHECK(read_int64(&value));
                printf("value (int64): %"PRId64"\n", value);
            } else if (CURRENT_ATOM.rem_size == 4) {
                int32_t value;
                MOV_CHECK(read_int32(&value));
                printf("value (int32): %d\n", value);
            } else if (CURRENT_ATOM.rem_size == 3) {
                int32_t value;
                MOV_CHECK(read_int24(&value));
                printf("value (int24): %d\n", value);
            } else if (CURRENT_ATOM.rem_size == 2) {
                int16_t value;
                MOV_CHECK(read_int16(&value));
                printf("value (int16): %d\n", value);
            } else if (CURRENT_ATOM.rem_size == 1) {
                int8_t value;
                MOV_CHECK(read_int8(&value));
                printf("value (int8): %d\n", value);
            } else {
                printf("value:\n");
                dump_bytes(CURRENT_ATOM.rem_size, 4);
            }
        } else if (type_field_2 == 22) {
            if (CURRENT_ATOM.rem_size == 8) {
                uint64_t value;
                MOV_CHECK(read_uint64(&value));
                printf("value (uint64): %"PRIu64"\n", value);
            } else if (CURRENT_ATOM.rem_size == 4) {
                uint32_t value;
                MOV_CHECK(read_uint32(&value));
                printf("value (uint32): %u\n", value);
            } else if (CURRENT_ATOM.rem_size == 3) {
                uint32_t value;
                MOV_CHECK(read_uint24(&value));
                printf("value (uint24): %u\n", value);
            } else if (CURRENT_ATOM.rem_size == 2) {
                uint16_t value;
                MOV_CHECK(read_uint16(&value));
                printf("value (uint16): %u\n", value);
            } else if (CURRENT_ATOM.rem_size == 1) {
                uint8_t value;
                MOV_CHECK(read_uint8(&value));
                printf("value (uint8): %u\n", value);
            } else {
                printf("value:\n");
                dump_bytes(CURRENT_ATOM.rem_size, 4);
            }
        } else {    // type_field_2 == 1
            uint64_t utf8_value_size = CURRENT_ATOM.rem_size;
            unsigned char utf8_value_buffer[129];
            if (utf8_value_size == 0) {
                printf("value: ''\n");
            } else if (utf8_value_size < sizeof(utf8_value_buffer)) {
                MOV_CHECK(read_bytes(utf8_value_buffer, utf8_value_size));

                size_t i;
                for (i = 0; i < utf8_value_size; i++) {
                    if (!isprint(utf8_value_buffer[i]))
                        break;
                }
                if (i >= utf8_value_size) {
                    utf8_value_buffer[utf8_value_size] = '\0';
                    printf("value: '%s'\n", (char*)utf8_value_buffer);
                } else {
                    printf("value:\n");
                    dump_bytes(utf8_value_buffer, utf8_value_size, 4);
                }
            } else {
                printf("value:\n");
                dump_bytes(utf8_value_size, 4);
            }
        }
    }
    else
    {
        printf("value:\n");
        dump_bytes(CURRENT_ATOM.rem_size, 4);
    }
}

static void dump_ilst_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'d','a','t','a'}, dump_ilst_data_atom},
    };

    dump_atom_header();

    while (CURRENT_ATOM.rem_size > 0) {
        uint32_t element_size;
        MOV_CHECK(read_uint32(&element_size));
        uint32_t key_index;
        MOV_CHECK(read_uint32(&key_index));
        MOV_CHECK(key_index >= 1 && (g_meta_keys.empty() || key_index <= g_meta_keys.size()));

        indent();
        printf("size: ");
        dump_uint32_size(element_size);
        printf("\n");
        indent();
        printf("key_index: %u", key_index);
        if (!g_meta_keys.empty()) {
            if (!g_meta_keys[key_index - 1].empty())
                printf(" ('%s')", g_meta_keys[key_index - 1].c_str());
        }
        printf("\n");


        push_atom();

        if (!read_atom_start())
            break;

        dump_child_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);

        pop_atom();
    }
}

static void dump_clefprofenof_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t fp_width;
    MOV_CHECK(read_uint32(&fp_width));
    indent();
    printf("width: ");
    dump_uint32_fp(fp_width, 16);
    printf("\n");

    uint32_t fp_height;
    MOV_CHECK(read_uint32(&fp_height));
    indent();
    printf("height: ");
    dump_uint32_fp(fp_height, 16);
    printf("\n");
}

static void dump_tapt_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'c','l','e','f'}, dump_clefprofenof_atom},
        {{'p','r','o','f'}, dump_clefprofenof_atom},
        {{'e','n','o','f'}, dump_clefprofenof_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_tref_child_atom()
{
    dump_atom_header();

    uint32_t count = CURRENT_ATOM.rem_size / 4;

    indent();
    printf("track_ids (");
    dump_uint32(count, false);
    printf("):\n");

    indent(4);
    if (count < 0xffff)
        printf("   i");
    else if (count < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("          id\n");


    uint32_t i;
    for (i = 0; i < count; i++) {
        uint32_t track_id;
        MOV_CHECK(read_uint32(&track_id));

        indent(4);
        dump_uint32_index(count, i);

        printf("  ");
        dump_uint32(track_id, true);
    }
    printf("\n");
}

static void dump_tref_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'\0','\0','\0','\0'}, dump_tref_child_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_elst_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00) {
        dump_unknown_version(version);
        return;
    }


    uint32_t count;
    MOV_CHECK(read_uint32(&count));

    indent();
    printf("edit_list_table (");
    dump_uint32(count, false);
    printf("):\n");

    indent(4);
    if (count < 0xffff)
        printf("   i");
    else if (count < 0xffffff)
        printf("     i");
    else
        printf("       i");
    printf("    duration       time          rate\n");


    uint32_t i = 0;
    for (i = 0; i < count; i++) {
        int32_t track_duration;
        MOV_CHECK(read_int32(&track_duration));
        int32_t media_time;
        MOV_CHECK(read_int32(&media_time));
        uint32_t media_rate;
        MOV_CHECK(read_uint32(&media_rate));

        indent(4);
        dump_uint32_index(count, i);

        printf("  ");
        dump_int32(track_duration);

        printf(" ");
        dump_int32(media_time);

        printf("      ");
        dump_uint32_fp(media_rate, 16);
        printf("\n");
    }
}

static void dump_edts_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'e','l','s','t'}, dump_elst_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_meta_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'k','e','y','s'}, dump_keys_atom},
        {{'i','l','s','t'}, dump_ilst_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_tkhd_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00 && version != 0x01) {
        dump_unknown_version(version);
        return;
    }


    if (version == 0x00) {
        uint32_t creation_time;
        MOV_CHECK(read_uint32(&creation_time));
        indent();
        printf("creation_time: ");
        dump_timestamp(creation_time);
        printf("\n");

        uint32_t modification_time;
        MOV_CHECK(read_uint32(&modification_time));
        indent();
        printf("modification_time: ");
        dump_timestamp(modification_time);
        printf("\n");
    } else {
        uint64_t creation_time;
        MOV_CHECK(read_uint64(&creation_time));
        indent();
        printf("creation_time: ");
        dump_timestamp(creation_time);
        printf("\n");

        uint64_t modification_time;
        MOV_CHECK(read_uint64(&modification_time));
        indent();
        printf("modification_time: ");
        dump_timestamp(modification_time);
        printf("\n");
    }

    uint32_t track_id;
    MOV_CHECK(read_uint32(&track_id));
    indent();
    printf("track_id: %u\n", track_id);

    uint32_t reserved_uint32;
    MOV_CHECK(read_uint32(&reserved_uint32));
    indent();
    printf("reserved: ");
    dump_uint32(reserved_uint32, true);
    printf("\n");

    if (version == 0x00) {
        int32_t duration;
        MOV_CHECK(read_int32(&duration));
        indent();
        printf("duration: %d (%f sec)\n", duration, duration / (double)(g_movie_timescale));
    } else {
        int64_t duration;
        MOV_CHECK(read_int64(&duration));
        indent();
        printf("duration: %"PRId64" (%f sec)\n", duration, duration / (double)(g_movie_timescale));
    }

    unsigned char reserved_bytes[8];
    MOV_CHECK(read_bytes(reserved_bytes, 8));
    indent();
    printf("reserved: ");
    dump_inline_bytes(reserved_bytes, 8);
    printf("\n");

    uint16_t layer;
    MOV_CHECK(read_uint16(&layer));
    indent();
    printf("layer: %u\n", layer);

    uint16_t alternate_group;
    MOV_CHECK(read_uint16(&alternate_group));
    indent();
    printf("alternate_group: %u\n", alternate_group);

    uint16_t volume;
    MOV_CHECK(read_uint16(&volume));
    indent();
    printf("volume: ");
    dump_uint16_fp(volume, 8);
    printf("\n");

    uint16_t reserved_uint16;
    MOV_CHECK(read_uint16(&reserved_uint16));
    indent();
    printf("reserved: ");
    dump_uint16(reserved_uint16, true);
    printf("\n");

    uint32_t matrix[9];
    MOV_CHECK(read_matrix(matrix));
    indent();
    printf("matrix: \n");
    dump_matrix(matrix, 2);

    uint32_t track_width;
    MOV_CHECK(read_uint32(&track_width));
    indent();
    printf("track_width: ");
    dump_uint32_fp(track_width, 16);
    printf("\n");

    uint32_t track_height;
    MOV_CHECK(read_uint32(&track_height));
    indent();
    printf("track_height: ");
    dump_uint32_fp(track_height, 16);
    printf("\n");
}

static void dump_trak_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'t','k','h','d'}, dump_tkhd_atom},
        {{'t','a','p','t'}, dump_tapt_atom},
        {{'e','d','t','s'}, dump_edts_atom},
        {{'t','r','e','f'}, dump_tref_atom},
        {{'m','d','i','a'}, dump_mdia_atom},
        {{'m','e','t','a'}, dump_meta_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_mvhd_atom()
{
    uint8_t version;
    uint32_t flags;
    dump_full_atom_header(&version, &flags);

    if (version != 0x00 && version != 0x01) {
        dump_unknown_version(version);
        return;
    }


    if (version == 0x00) {
        uint32_t creation_time;
        MOV_CHECK(read_uint32(&creation_time));
        indent();
        printf("creation_time: ");
        dump_timestamp(creation_time);
        printf("\n");

        uint32_t modification_time;
        MOV_CHECK(read_uint32(&modification_time));
        indent();
        printf("modification_time: ");
        dump_timestamp(modification_time);
        printf("\n");
    } else {
        uint64_t creation_time;
        MOV_CHECK(read_uint64(&creation_time));
        indent();
        printf("creation_time: ");
        dump_timestamp(creation_time);
        printf("\n");

        uint64_t modification_time;
        MOV_CHECK(read_uint64(&modification_time));
        indent();
        printf("modification_time: ");
        dump_timestamp(modification_time);
        printf("\n");
    }

    MOV_CHECK(read_uint32(&g_movie_timescale));
    indent();
    printf("timescale: %u\n", g_movie_timescale);

    if (version == 0x00) {
        int32_t duration;
        MOV_CHECK(read_int32(&duration));
        indent();
        printf("duration: %d (%f sec)\n", duration, duration / (double)(g_movie_timescale));
    } else {
        int64_t duration;
        MOV_CHECK(read_int64(&duration));
        indent();
        printf("duration: %"PRId64" (%f sec)\n", duration, duration / (double)(g_movie_timescale));
    }

    uint32_t preferred_rate;
    MOV_CHECK(read_uint32(&preferred_rate));
    indent();
    printf("preferred_rate: ");
    dump_uint32_fp(preferred_rate, 16);
    printf("\n");

    uint16_t preferred_volume;
    MOV_CHECK(read_uint16(&preferred_volume));
    indent();
    printf("preferred_volume: ");
    dump_uint16_fp(preferred_volume, 8);
    printf("\n");

    unsigned char bytes[10];
    MOV_CHECK(read_bytes(bytes, 10));
    indent();
    printf("reserved: ");
    dump_inline_bytes(bytes, 10);
    printf("\n");

    uint32_t matrix[9];
    MOV_CHECK(read_matrix(matrix));
    indent();
    printf("matrix: \n");
    dump_matrix(matrix, 2);


    // NOTE/TODO: spec. does not clearly state the xxx_time values are in timescale units

    uint32_t preview_time;
    MOV_CHECK(read_uint32(&preview_time));
    indent();
    printf("preview_time: %u\n", preview_time);

    uint32_t preview_duration;
    MOV_CHECK(read_uint32(&preview_duration));
    indent();
    printf("preview_duration: %u (%f sec)\n", preview_duration, preview_duration / (double)g_movie_timescale);

    uint32_t poster_time;
    MOV_CHECK(read_uint32(&poster_time));
    indent();
    printf("poster_time: %u\n", poster_time);

    uint32_t selection_time;
    MOV_CHECK(read_uint32(&selection_time));
    indent();
    printf("selection_time: %u\n", selection_time);

    uint32_t selection_duration;
    MOV_CHECK(read_uint32(&selection_duration));
    indent();
    printf("selection_duration: %u (%f sec)\n", selection_duration, selection_duration / (double)g_movie_timescale);

    uint32_t current_time;
    MOV_CHECK(read_uint32(&current_time));
    indent();
    printf("current_time: %u\n", current_time);

    uint32_t next_track_id;
    MOV_CHECK(read_uint32(&next_track_id));
    indent();
    printf("next_track_id: %u\n", next_track_id);
}

static void dump_moov_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'m','v','h','d'}, dump_mvhd_atom},
        {{'t','r','a','k'}, dump_trak_atom},
        {{'m','e','t','a'}, dump_meta_atom},
    };

    dump_container_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}

static void dump_top_atom()
{
    static const DumpFuncMap dump_func_map[] =
    {
        {{'f','t','y','p'}, dump_ftyp_atom},
        {{'m','d','a','t'}, dump_mdat_atom},
        {{'f','r','e','e'}, dump_free_atom},
        {{'s','k','i','p'}, dump_skip_atom},
        {{'m','o','o','v'}, dump_moov_atom},
    };

    dump_child_atom(dump_func_map, DUMP_FUNC_MAP_SIZE);
}


static void dump_file()
{
    while (true) {
        push_atom();

        if (!read_atom_start())
            break;

        dump_top_atom();

        pop_atom();
    }
}

static void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s [options] <quicktime filename>\n", cmd);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -h | --help       Print this usage message and exit\n");
}

int main(int argc, const char **argv)
{
    const char *filename;
    int cmdln_index;

    // parse commandline arguments

    for (cmdln_index = 1; cmdln_index < argc; cmdln_index++) {
        if (strcmp(argv[cmdln_index], "-h") == 0 ||
            strcmp(argv[cmdln_index], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            break;
        }
    }

    if (cmdln_index + 1 < argc) {
        usage(argv[0]);
        fprintf(stderr, "Unknown argument '%s'\n", argv[cmdln_index]);
        return 1;
    }
    if (cmdln_index + 1 > argc) {
        if (argc == 1) {
            usage(argv[0]);
            return 0;
        }
        usage(argv[0]);
        fprintf(stderr, "Missing quicktime filename\n");
        return 1;
    }

    filename = argv[cmdln_index];


    // open file

    g_mov_file = fopen(filename, "rb");
    if (!g_mov_file) {
        fprintf(stderr, "Failed to open quicktime file '%s': %s\n", filename, strerror(errno));
        return 1;
    }

    // check whether file has 64-bit size

    struct stat stat_buf;
    if (stat(filename, &stat_buf) != 0) {
        fprintf(stderr, "Failed to stat quicktime file '%s': %s\n", filename, strerror(errno));
        return 1;
    }
    g_file_size_64bit = (stat_buf.st_size > 0xffffffff);


    // dump file

    try
    {
        dump_file();
    }
    catch (exception &ex)
    {
        fprintf(stderr, "Exception: %s\n", ex.what());
        return 1;
    }
    catch (...)
    {
        fprintf(stderr, "Unexpected exception\n");
        return 1;
    }

    if (g_mov_file)
        fclose(g_mov_file);

    return 0;
}

