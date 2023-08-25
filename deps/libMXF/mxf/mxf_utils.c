/*
 * General purpose utilities
 *
 * Copyright (C) 2006, British Broadcasting Corporation
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


#if defined(_WIN32)

#include <sys/timeb.h>
#include <time.h>
#include <windows.h>

#else

#include <uuid/uuid.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#endif


#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>


typedef enum
{
    COMMON_INDICATOR,
    FRAME_ONLY_NO_INDICATOR,
    CLIP_ONLY_NO_INDICATOR,
    UNC_INDICATOR,
    D10_D11_INDICATOR,
    AES_BWF_INDICATOR,
} WrappingIndicatorType;

typedef struct
{
    mxfUL label;
    uint8_t indicator_byte_offset;
    WrappingIndicatorType indicator_type;
} ECLabelInfo;

#define GC_PREFIX   0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01

static const ECLabelInfo EC_LABEL_INFO[] =
{
    // SMPTE D-10 Mappings
    {{GC_PREFIX, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x01, 0x00, 0x00}, 15, D10_D11_INDICATOR},
    // DV-DIF Mappings
    {{GC_PREFIX, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x02, 0x00, 0x00}, 15, COMMON_INDICATOR},
    // SMPTE D-11 Mappings
    {{GC_PREFIX, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x03, 0x00, 0x00}, 15, D10_D11_INDICATOR},
    // MPEG ES
    {{GC_PREFIX, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x04, 0x00, 0x00}, 15, COMMON_INDICATOR},
    // Uncompressed Pictures
    {{GC_PREFIX, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x05, 0x00, 0x00}, 15, UNC_INDICATOR},
    // AES-BWF
    {{GC_PREFIX, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x06, 0x00, 0x00}, 14, AES_BWF_INDICATOR},
    // MPEG PES
    {{GC_PREFIX, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x07, 0x00, 0x00}, 15, COMMON_INDICATOR},
    // MPEG PS
    {{GC_PREFIX, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x08, 0x00, 0x00}, 15, COMMON_INDICATOR},
    // MPEG TS
    {{GC_PREFIX, 0x02, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x09, 0x00, 0x00}, 15, COMMON_INDICATOR},
    // A-law
    {{GC_PREFIX, 0x03, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0a, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // Encrypted Data Mappings
    {{GC_PREFIX, 0x07, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0b, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // JPEG-2000 Picture Mappings
    {{GC_PREFIX, 0x07, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0c, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // Generic VBI Data Mapping Undefined Payload
    {{GC_PREFIX, 0x09, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0d, 0x00, 0x00},  0, FRAME_ONLY_NO_INDICATOR},
    // Generic ANC Data Mapping Undefined Payload
    {{GC_PREFIX, 0x09, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0e, 0x00, 0x00},  0, FRAME_ONLY_NO_INDICATOR},
    // AVC NAL Unit Stream
    {{GC_PREFIX, 0x0a, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x0f, 0x00, 0x00}, 15, COMMON_INDICATOR},
    // AVC Byte Stream
    {{GC_PREFIX, 0x0a, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x10, 0x00, 0x00}, 15, COMMON_INDICATOR},
    // VC-3 Pictures
    {{GC_PREFIX, 0x0a, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x11, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // VC-1 Pictures
    {{GC_PREFIX, 0x0a, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x12, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // D-Cinema Timed Text Stream
    {{GC_PREFIX, 0x0a, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x13, 0x01, 0x01},  0, CLIP_ONLY_NO_INDICATOR},
    // D-Cinema Aux Data Essence
    {{GC_PREFIX, 0x0a, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x13, 0x02, 0x01},  0, FRAME_ONLY_NO_INDICATOR},
    // TIFF/EP
    {{GC_PREFIX, 0x0b, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x14, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // VC-2 Pictures
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x15, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // AAC ADIF
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x16, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // AAC ADTS
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x17, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // AAC LATM LOAS
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x18, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // ACES Pictures
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x19, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // DMCVT Data Coding
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x1a, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // VC-5 Pictures
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x1b, 0x00, 0x00}, 14, COMMON_INDICATOR},
    // Pro Res Picture (RDD 44)
    {{GC_PREFIX, 0x0d, 0x0d, 0x01, 0x03, 0x01, 0x02, 0x1c, 0x00, 0x00}, 14, COMMON_INDICATOR},
};


mxf_generate_uuid_func     mxf_generate_uuid     = mxf_default_generate_uuid;
mxf_get_timestamp_now_func mxf_get_timestamp_now = mxf_default_get_timestamp_now;
mxf_generate_umid_func     mxf_generate_umid     = mxf_default_generate_umid;
mxf_generate_key_func      mxf_generate_key      = mxf_default_generate_key;



static size_t utf8_code_len(const char *u8_code)
{
    if ((unsigned char)u8_code[0] < 0x80)
    {
        return 1;
    }
    else if (((unsigned char)u8_code[0] & 0xe0) == 0xc0 &&
             ((unsigned char)u8_code[1] & 0xc0) == 0x80)
    {
        return 2;
    }
    else if (((unsigned char)u8_code[0] & 0xf0) == 0xe0 &&
             ((unsigned char)u8_code[1] & 0xc0) == 0x80 &&
             ((unsigned char)u8_code[2] & 0xc0) == 0x80)
    {
        return 3;
    }
    else if (((unsigned char)u8_code[0] & 0xf8) == 0xf0 &&
             ((unsigned char)u8_code[1] & 0xc0) == 0x80 &&
             ((unsigned char)u8_code[2] & 0xc0) == 0x80 &&
             ((unsigned char)u8_code[3] & 0xc0) == 0x80)
    {
        return 4;
    }

    return -1;
}

static size_t utf16_code_len(const mxfUTF16Char *u16_code)
{
    if (u16_code[0] < 0xd800 || u16_code[0] > 0xdfff)
    {
        return 1;
    }
    else if (((u16_code[0] & 0xfc00) == 0xd800) &&
             ((u16_code[1] & 0xfc00) == 0xdc00))
    {
        return 2;
    }

    return -1;
}

static size_t utf8_code_len_from_utf16(const mxfUTF16Char *u16_code)
{
    if (u16_code[0] < 0x80)
    {
        return 1;
    }
    else if (u16_code[0] < 0x800)
    {
        return 2;
    }
    else if (u16_code[0] < 0xd800 || u16_code[0] > 0xdfff)
    {
        return 3;
    }
    else if (((u16_code[0] & 0xfc00) == 0xd800) &&
             ((u16_code[1] & 0xfc00) == 0xdc00))
    {
        return 4;
    }

    return -1;
}

static size_t utf16_code_len_from_utf8(const char *u8_code)
{
    size_t u8_len = utf8_code_len(u8_code);
    if (u8_len <= 3)
        return 1;
    else if (u8_len == 4)
        return 2;

    return -1;
}

static size_t utf8_code_to_utf16(mxfUTF16Char *u16_code, size_t avail_u16_len, const char *u8_code,
                                 size_t *u8_len, size_t *u16_len)
{
    size_t len16 = utf16_code_len_from_utf8(u8_code);
    size_t len8 = utf8_code_len(u8_code);
    if (len16 == (size_t)(-1) || len8 == (size_t)(-1) || len16 > avail_u16_len)
        return (size_t)(-1);

    if (len8 == 1)
    {
        u16_code[0] = (mxfUTF16Char)u8_code[0];
    }
    else if (len8 == 2)
    {
        u16_code[0] = ((((mxfUTF16Char)u8_code[0]) & 0x1f) << 6) |
                       (((mxfUTF16Char)u8_code[1]) & 0x3f);
    }
    else if (len8 == 3)
    {
        u16_code[0] = ((((mxfUTF16Char)u8_code[0]) & 0x0f) << 12) |
                      ((((mxfUTF16Char)u8_code[1]) & 0x3f) << 6) |
                       (((mxfUTF16Char)u8_code[2]) & 0x3f);
    }
    else
    {
        uint32_t c = ((((mxfUTF16Char)u8_code[0]) & 0x07) << 18) |
                     ((((mxfUTF16Char)u8_code[1]) & 0x3f) << 12) |
                     ((((mxfUTF16Char)u8_code[2]) & 0x3f) << 6) |
                      (((mxfUTF16Char)u8_code[3]) & 0x3f);
        c -= 0x10000;
        u16_code[0] = (mxfUTF16Char)(0xd800 | ((c >> 10) & 0x03ff));
        u16_code[1] = (mxfUTF16Char)(0xdc00 |  (c        & 0x03ff));
    }

    *u16_len = len16;
    *u8_len = len8;
    return len16;
}

static size_t utf16_code_to_utf8(char *u8_code, size_t avail_u8_len, const mxfUTF16Char *u16_code,
                                 size_t *u16_len, size_t *u8_len)
{
    size_t len8 = utf8_code_len_from_utf16(u16_code);
    size_t len16 = utf16_code_len(u16_code);
    if (len8 == (size_t)(-1) || len16 == (size_t)(-1) || len8 > avail_u8_len)
        return (size_t)(-1);

    if (len8 == 1)
    {
        u8_code[0] = (char)(u16_code[0]);
    }
    else if (len8 == 2)
    {
        u8_code[0] = (char)(0xc0 | (u16_code[0] >> 6));
        u8_code[1] = (char)(0x80 | (u16_code[0] & 0x3f));
    }
    else if (len8 == 3)
    {
        u8_code[0] = (char)(0xe0 |  (u16_code[0] >> 12));
        u8_code[1] = (char)(0x80 | ((u16_code[0] >> 6) & 0x3f));
        u8_code[2] = (char)(0x80 |  (u16_code[0]       & 0x3f));
    }
    else
    {
        uint32_t c = ((u16_code[0] & 0x03ff) << 10) |
                      (u16_code[1] & 0x03ff);
        c += 0x10000;
        u8_code[0] = (char)(0xf0 | ((c >> 18) & 0x07));
        u8_code[1] = (char)(0x80 | ((c >> 12) & 0x3f));
        u8_code[2] = (char)(0x80 | ((c >> 6)  & 0x3f));
        u8_code[3] = (char)(0x80 |  (c        & 0x3f));
    }

    *u8_len = len8;
    *u16_len = len16;
    return len8;
}

static size_t utf16_strlen_from_utf8(const char *u8_str)
{
    size_t len = 0;
    const char *u8_str_ptr = u8_str;
    while (*u8_str_ptr) {
        size_t u8_code_len = utf8_code_len(u8_str_ptr);
        size_t u16_code_len = utf16_code_len_from_utf8(u8_str_ptr);
        if (u8_code_len == (size_t)(-1) || u16_code_len == (size_t)(-1))
            return (size_t)(-1);

        u8_str_ptr += u8_code_len;
        len += u16_code_len;
    }

    return len;
}

static size_t utf8_strlen_from_utf16(const mxfUTF16Char *u16_str)
{
    size_t len = 0;
    const mxfUTF16Char *u16_str_ptr = u16_str;
    while (*u16_str_ptr) {
        size_t u8_code_len = utf8_code_len_from_utf16(u16_str_ptr);
        size_t u16_code_len = utf16_code_len(u16_str_ptr);
        if (u8_code_len == (size_t)(-1) || u16_code_len == (size_t)(-1))
            return (size_t)(-1);

        u16_str_ptr += u16_code_len;
        len += u8_code_len;
    }

    return len;
}



void mxf_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    mxf_vsnprintf(str, size, format, ap);
    va_end(ap);
}

void mxf_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
#if defined(_MSC_VER)
    int res = _vsnprintf(str, size, format, ap);
    if (str && size > 0) {
        if (res == -1 && errno == EINVAL)
            str[0] = 0;
        else
            str[size - 1] = 0;
    }
#else
    if (vsnprintf(str, size, format, ap) < 0 && str && size > 0)
        str[0] = 0;
#endif
}

char* mxf_strerror(int errnum, char *buf, size_t size)
{
#ifdef HAVE_STRERROR_R

#ifdef _GNU_SOURCE
    const char *err_str = strerror_r(errnum, buf, size);
    if (err_str != buf)
        mxf_snprintf(buf, size, "%s", err_str);
#else
    if (strerror_r(errnum, buf, size) != 0)
        mxf_snprintf(buf, size, "unknown error code %d", errnum);
#endif

#elif defined(_MSC_VER)
    if (strerror_s(buf, size, errnum) != 0)
        mxf_snprintf(buf, size, "unknown error code %d", errnum);
#else
    mxf_snprintf(buf, size, "error code %d", errnum);
#endif

    return buf;
}

void mxf_print_key(const mxfKey *key)
{
    char keyStr[KEY_STR_SIZE];
    mxf_sprint_key(keyStr, key);
    printf("K = %s\n", keyStr);
}

void mxf_sprint_key(char *str, const mxfKey *key)
{
    mxf_snprintf(str, KEY_STR_SIZE,
                 "%02x %02x %02x %02x %02x %02x %02x %02x "
                 "%02x %02x %02x %02x %02x %02x %02x %02x",
                 key->octet0, key->octet1, key->octet2, key->octet3,
                 key->octet4, key->octet5, key->octet6, key->octet7,
                 key->octet8, key->octet9, key->octet10, key->octet11,
                 key->octet12, key->octet13, key->octet14, key->octet15);
}

void mxf_print_label(const mxfUL *label)
{
    mxf_print_key((const mxfKey*)label);
}

void mxf_sprint_label(char *str, const mxfUL *label)
{
    mxf_sprint_key(str, (const mxfKey*)label);
}

void mxf_print_umid(const mxfUMID *umid)
{
    char umidStr[UMID_STR_SIZE];
    mxf_sprint_umid(umidStr, umid);
    printf("UMID = %s\n", umidStr);
}

void mxf_sprint_umid(char *str, const mxfUMID *umid)
{
    mxf_snprintf(str, UMID_STR_SIZE,
                 "%02x %02x %02x %02x %02x %02x %02x %02x "
                 "%02x %02x %02x %02x %02x %02x %02x %02x "
                 "%02x %02x %02x %02x %02x %02x %02x %02x "
                 "%02x %02x %02x %02x %02x %02x %02x %02x",
                 umid->octet0, umid->octet1, umid->octet2, umid->octet3,
                 umid->octet4, umid->octet5, umid->octet6, umid->octet7,
                 umid->octet8, umid->octet9, umid->octet10, umid->octet11,
                 umid->octet12, umid->octet13, umid->octet14, umid->octet15,
                 umid->octet16, umid->octet17, umid->octet18, umid->octet19,
                 umid->octet20, umid->octet21, umid->octet22, umid->octet23,
                 umid->octet24, umid->octet25, umid->octet26, umid->octet27,
                 umid->octet28, umid->octet29, umid->octet30, umid->octet31);
}

void mxf_default_generate_uuid(mxfUUID *uuid)
{
#if defined(_WIN32)

    GUID guid;
    CoCreateGuid(&guid);
    uuid->octet0 = (uint8_t)((guid.Data1 >> 24) & 0xff);
    uuid->octet1 = (uint8_t)((guid.Data1 >> 16) & 0xff);
    uuid->octet2 = (uint8_t)((guid.Data1 >>  8) & 0xff);
    uuid->octet3 = (uint8_t)((guid.Data1      ) & 0xff);
    uuid->octet4 = (uint8_t)((guid.Data2 >>  8) & 0xff);
    uuid->octet5 = (uint8_t)((guid.Data2      ) & 0xff);
    uuid->octet6 = (uint8_t)((guid.Data3 >>  8) & 0xff);
    uuid->octet7 = (uint8_t)((guid.Data3      ) & 0xff);
    memcpy(&uuid->octet8, guid.Data4, 8);

#else

    uuid_t newUUID;
    uuid_generate(newUUID);
    memcpy(uuid, newUUID, 16);

#endif
}

void mxf_default_get_timestamp_now(mxfTimestamp *now)
{
#if defined(_WIN32) && defined(__GNUC__)
    /* MinGW */

    /* NOTE: gmtime is not thread safe (not reentrant) */

    struct __timeb64 tb;
    struct tm gmt;

    memset(&gmt, 0, sizeof(struct tm));

    _ftime64(&tb);
    memcpy(&gmt, _gmtime64(&tb.time), sizeof(struct tm)); /* memcpy does nothing if gmtime returns NULL */

    now->year  = gmt.tm_year + 1900;
    now->month = gmt.tm_mon + 1;
    now->day   = gmt.tm_mday;
    now->hour  = gmt.tm_hour;
    now->min   = gmt.tm_min;
    now->sec   = gmt.tm_sec;
    now->qmsec = (uint8_t)(tb.millitm / 4 + 0.5); /* 1/250th second */

#elif defined(_MSC_VER)
    /* MSVC */

    struct _timeb tb;
    struct tm gmt;

    memset(&gmt, 0, sizeof(struct tm));

    /* using the secure _ftime */
    _ftime_s(&tb);

    /* using the secure (and reentrant) gmtime */
    gmtime_s(&gmt, &tb.time);

    now->year  = (int16_t)gmt.tm_year + 1900;
    now->month = (uint8_t)gmt.tm_mon + 1;
    now->day   = (uint8_t)gmt.tm_mday;
    now->hour  = (uint8_t)gmt.tm_hour;
    now->min   = (uint8_t)gmt.tm_min;
    now->sec   = (uint8_t)gmt.tm_sec;
    now->qmsec = (uint8_t)(tb.millitm / 4 + 0.5); /* 1/250th second */

#else

    struct timeval tv;
    gettimeofday(&tv, NULL);

    /* use the reentrant gmtime */
    struct tm gmt;
    memset(&gmt, 0, sizeof(struct tm));
    gmtime_r(&tv.tv_sec, &gmt);

    now->year  = gmt.tm_year + 1900;
    now->month = gmt.tm_mon + 1;
    now->day   = gmt.tm_mday;
    now->hour  = gmt.tm_hour;
    now->min   = gmt.tm_min;
    now->sec   = gmt.tm_sec;
    now->qmsec = (uint8_t)(tv.tv_usec / 4000 + 0.5); /* 1/250th second */

#endif
}


void mxf_default_generate_umid(mxfUMID *umid)
{
    mxfUUID uuid;

    umid->octet0  = 0x06;
    umid->octet1  = 0x0a;
    umid->octet2  = 0x2b;
    umid->octet3  = 0x34;
    umid->octet4  = 0x01;
    umid->octet5  = 0x01;
    umid->octet6  = 0x01;
    umid->octet7  = 0x05; /* registry version */
    umid->octet8  = 0x01;
    umid->octet9  = 0x01;
    umid->octet10 = 0x0f; /* material type not identified */
    umid->octet11 = 0x20; /* UUID/UL material generation method, no instance method defined */

    umid->octet12 = 0x13;
    umid->octet13 = 0x00;
    umid->octet14 = 0x00;
    umid->octet15 = 0x00;

    /* Note: a UUID is mapped directly and a UL is half swapped */
    mxf_generate_uuid(&uuid);
    memcpy(&umid->octet16, &uuid, 16);
}

void mxf_default_generate_key(mxfKey *key)
{
    mxfUUID uuid;

    mxf_generate_uuid(&uuid);
    mxf_swap_uid((mxfUID*)key, (const mxfUID*)&uuid);
}

void mxf_set_regtest_funcs(void)
{
    mxf_get_version = mxf_regtest_get_version;
    mxf_get_platform_string = mxf_regtest_get_platform_string;
    mxf_get_platform_wstring = mxf_regtest_get_platform_wstring;
    mxf_get_scm_version_string = mxf_regtest_get_scm_version_string;
    mxf_get_scm_version_wstring = mxf_regtest_get_scm_version_wstring;

    mxf_generate_uuid = mxf_regtest_generate_uuid;
    mxf_get_timestamp_now = mxf_regtest_get_timestamp_now;
    mxf_generate_umid = mxf_regtest_generate_umid;
    mxf_generate_key = mxf_regtest_generate_key;
}

void mxf_regtest_generate_uuid(mxfUUID *uuid)
{
    static uint32_t count = 1;

    memset(uuid, 0, sizeof(*uuid));
    uuid->octet12 = (uint8_t)((count >> 24) & 0xff);
    uuid->octet13 = (uint8_t)((count >> 16) & 0xff);
    uuid->octet14 = (uint8_t)((count >> 8)  & 0xff);
    uuid->octet15 = (uint8_t)( count        & 0xff);

    count++;
}

void mxf_regtest_get_timestamp_now(mxfTimestamp *now)
{
    memset(now, 0, sizeof(*now));
}

void mxf_regtest_generate_umid(mxfUMID *umid)
{
    static uint32_t count = 1;

    memset(umid, 0, sizeof(*umid));
    umid->octet28 = (uint8_t)((count >> 24) & 0xff);
    umid->octet29 = (uint8_t)((count >> 16) & 0xff);
    umid->octet30 = (uint8_t)((count >> 8)  & 0xff);
    umid->octet31 = (uint8_t)( count        & 0xff);

    count++;
}

void mxf_regtest_generate_key(mxfKey *key)
{
    static uint32_t count = 1;

    memset(key, 0, sizeof(*key));
    key->octet12 = (uint8_t)((count >> 24) & 0xff);
    key->octet13 = (uint8_t)((count >> 16) & 0xff);
    key->octet14 = (uint8_t)((count >> 8)  & 0xff);
    key->octet15 = (uint8_t)( count        & 0xff);

    count++;
}

int mxf_equals_key(const mxfKey *keyA, const mxfKey *keyB)
{
    return memcmp((const void*)keyA, (const void*)keyB, sizeof(mxfKey)) == 0;
}

int mxf_equals_key_prefix(const mxfKey *keyA, const mxfKey *keyB, size_t cmpLen)
{
    return memcmp((const void*)keyA, (const void*)keyB, cmpLen) == 0;
}

int mxf_equals_key_mod_regver(const mxfKey *keyA, const mxfKey *keyB)
{
    /* ignore difference in octet7, the registry version */
    return memcmp((const void*)keyA, (const void*)keyB, 7) == 0 &&
           memcmp((const void*)&keyA->octet8, (const void*)&keyB->octet8, 8) == 0;
}

int mxf_equals_ul(const mxfUL *labelA, const mxfUL *labelB)
{
    return memcmp((const void*)labelA, (const void*)labelB, sizeof(mxfUL)) == 0;
}

int mxf_equals_ul_mod_regver(const mxfUL *labelA, const mxfUL *labelB)
{
    /* ignore difference in octet7, the registry version */
    return memcmp((const void*)labelA, (const void*)labelB, 7) == 0 &&
           memcmp((const void*)&labelA->octet8, (const void*)&labelB->octet8, 8) == 0;
}

int mxf_equals_uuid(const mxfUUID *uuidA, const mxfUUID *uuidB)
{
    return memcmp((const void*)uuidA, (const void*)uuidB, sizeof(mxfUUID)) == 0;
}

int mxf_equals_uid(const mxfUID *uidA, const mxfUID *uidB)
{
    return memcmp((const void*)uidA, (const void*)uidB, sizeof(mxfUID)) == 0;
}

int mxf_equals_umid(const mxfUMID *umidA, const mxfUMID *umidB)
{
    return memcmp((const void*)umidA, (const void*)umidB, sizeof(mxfUMID)) == 0;
}

int mxf_equals_ext_umid(const mxfExtendedUMID *extUMIDA, const mxfExtendedUMID *extUMIDB)
{
    return memcmp(extUMIDA->bytes, extUMIDB->bytes, sizeof(extUMIDA->bytes)) == 0;
}

int mxf_equals_rgba_layout(const mxfRGBALayout *layoutA, const mxfRGBALayout *layoutB)
{
    int i;
    for (i = 0; i < 8; i++) {
        if (layoutA->components[i].code == 0 || layoutB->components[i].code == 0)
            return layoutA->components[i].code == layoutB->components[i].code;

        if (layoutA->components[i].code  != layoutB->components[i].code ||
            layoutA->components[i].depth != layoutB->components[i].depth)
        {
            return 0;
        }
    }

    return 1;
}

/* Note: this function only works if half-swapping is used
   a UL always has the MSB of the 1st byte == 0 and a UUID (non-NCS) has the MSB of the 9th byte == 1
   The UUID should be half swapped when used where a UL is expected
   Note: the UL is half swapped in AAF AUIDs
   Note: the UL is half swapped in UMIDs when using the UUID/UL material generation method

   MXF AUID (the opposite of AAF AUID!): the first bit is always 0 for a UL
*/
int mxf_is_ul(const mxfUID *uid)
{
    /* requiring more than just the 1st bit is always 0 to make it a SMPTE UL*/
    return uid->octet0 == 0x06 && uid->octet1 == 0x0e && uid->octet2 == 0x2b && uid->octet3 == 0x34;
}

/* MXF IDAU: the 65th bit is always 0 for a UL */
int mxf_is_swapped_ul(const mxfUID *uid)
{
    /* requiring more than just the 65th bit is always 0 to make it a SMPTE UL */
    return uid->octet8 == 0x06 && uid->octet9 == 0x0e && uid->octet10 == 0x2b && uid->octet11 == 0x34;
}

void mxf_swap_uid(mxfUID *swap_uid, const mxfUID *uid)
{
    memcpy(&swap_uid->octet0, &uid->octet8, 8);
    memcpy(&swap_uid->octet8, &uid->octet0, 8);
}

int mxf_is_simple_idau_umid(const mxfUMID *umid)
{
    const mxfUMID simple_idau_umid = {
        0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05, 0x01, 0x01, 0x0f, 0x20,                          // label for UUID/UL material number
        0x13, 0x00, 0x00, 0x00,                                                                          // length & instance number (0)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // UUID/UL material number
    };

    return memcmp(umid, &simple_idau_umid, 16) == 0;
}

void mxf_extract_umid_material_number(mxfUID *idau, const mxfUMID *umid)
{
    memcpy(&idau->octet0, &umid->octet16, 16);
}

MXFEssenceWrappingType mxf_get_essence_wrapping_type(const mxfUL *label)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(EC_LABEL_INFO); i++) {
        if (memcmp(label,          &EC_LABEL_INFO[i].label,        7) == 0 &&
            memcmp(&label->octet8, &EC_LABEL_INFO[i].label.octet8, 6) == 0)
        {
            uint8_t indicator_byte = ((const uint8_t*)label)[EC_LABEL_INFO[i].indicator_byte_offset];
            if (EC_LABEL_INFO[i].indicator_type == COMMON_INDICATOR) {
                if (indicator_byte == 0x01)
                    return MXF_FRAME_WRAPPED;
                else if (indicator_byte == 0x02)
                    return MXF_CLIP_WRAPPED;
            } else if (EC_LABEL_INFO[i].indicator_type == FRAME_ONLY_NO_INDICATOR) {
                return MXF_FRAME_WRAPPED;
            } else if (EC_LABEL_INFO[i].indicator_type == CLIP_ONLY_NO_INDICATOR) {
                return MXF_CLIP_WRAPPED;
            } else if (EC_LABEL_INFO[i].indicator_type == UNC_INDICATOR) {
                if ((indicator_byte & 0x03) == 0x01)
                    return MXF_FRAME_WRAPPED;
                else if ((indicator_byte & 0x03) == 0x02)
                    return MXF_CLIP_WRAPPED;
            } else if (EC_LABEL_INFO[i].indicator_type == D10_D11_INDICATOR) {
                if (indicator_byte == 0x01 || indicator_byte == 0x02)
                    return MXF_FRAME_WRAPPED;
                // indicator_byte 0x7f can be either wrapping, e.g. Avid clip-wrapped D-10,
                // contrary to what the registry says
            } else if (EC_LABEL_INFO[i].indicator_type == AES_BWF_INDICATOR) {
                if (indicator_byte == 0x01 || indicator_byte == 0x03)
                    return MXF_FRAME_WRAPPED;
                else if (indicator_byte == 0x02 || indicator_byte == 0x04)
                    return MXF_CLIP_WRAPPED;
            }
            break;
        }
    }

    return MXF_UNKNOWN_WRAPPING_TYPE;
}

size_t mxf_utf16_to_utf8(char *u8_str, const mxfUTF16Char *u16_str, size_t u8_size)
{
    size_t u8_len;
    size_t convert_size = 0;
    const mxfUTF16Char *u16_str_ptr = u16_str;
    char *u8_str_ptr = u8_str;
    size_t u8_code_len = 0;
    size_t u16_code_len = 0;

    if (!u16_str)
        return (size_t)(-1);

    u8_len = utf8_strlen_from_utf16(u16_str);
    if (u8_len == (size_t)(-1) || !u8_str)
        return u8_len;

    while (*u16_str_ptr && convert_size < u8_size) {
        if (utf16_code_to_utf8(u8_str_ptr, u8_size - convert_size, u16_str_ptr,
                               &u16_code_len, &u8_code_len) == (size_t)(-1))
        {
            break;
        }

        u8_str_ptr += u8_code_len;
        u16_str_ptr += u16_code_len;
        convert_size += u8_code_len;
    }
    if (convert_size < u8_size)
        *u8_str_ptr = 0;

    return convert_size;
}

size_t mxf_utf8_to_utf16(mxfUTF16Char *u16_str, const char *u8_str, size_t u16_size)
{
    size_t u16_len;
    size_t convert_size = 0;
    mxfUTF16Char *u16_str_ptr = u16_str;
    const char *u8_str_ptr = u8_str;
    size_t u8_code_len = 0;
    size_t u16_code_len = 0;

    if (!u8_str)
        return (size_t)(-1);

    u16_len = utf16_strlen_from_utf8(u8_str);
    if (u16_len == (size_t)(-1) || !u16_str)
        return u16_len;

    while (*u8_str_ptr && convert_size < u16_size) {
        if (utf8_code_to_utf16(u16_str_ptr, u16_size - convert_size, u8_str_ptr,
                               &u8_code_len, &u16_code_len) == (size_t)(-1))
        {
            break;
        }

        u8_str_ptr += u8_code_len;
        u16_str_ptr += u16_code_len;
        convert_size += u16_code_len;
    }
    if (convert_size < u16_size)
        *u16_str_ptr = 0;

    return convert_size;
}

uint32_t mxf_get_system_page_size()
{
    uint32_t pageSize;
#if defined (_WIN32)
    SYSTEM_INFO systemInfo;
#else
    long psResult;
    char errorBuf[128];
#endif

#if defined (_WIN32)
    GetSystemInfo(&systemInfo);
    pageSize = systemInfo.dwPageSize;
#else
    psResult = sysconf(_SC_PAGESIZE);
    if (psResult < 0) {
        pageSize = 8192;
        mxf_log_warn("Failed to get system page size using sysconf(__SC_PAGESIZE): %s. Defaulting to %u\n",
                     mxf_strerror(errno, errorBuf, sizeof(errorBuf)), pageSize);
    } else {
        pageSize = (uint32_t)psResult;
    }
#endif

    return pageSize;
}

