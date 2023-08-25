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

#ifndef MXF_UTILS_H_
#define MXF_UTILS_H_


#include <stdarg.h>


#ifdef __cplusplus
extern "C"
{
#endif


/* strdup was deprecated in Visual C++ 2005 */
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define strdup _strdup
#endif


#define KEY_STR_SIZE        48
#define LABEL_STR_SIZE      48
#define UMID_STR_SIZE       96


typedef void (*mxf_generate_uuid_func)(mxfUUID *uuid);
typedef void (*mxf_get_timestamp_now_func)(mxfTimestamp *now);
typedef void (*mxf_generate_umid_func)(mxfUMID *umid);
typedef void (*mxf_generate_key_func)(mxfKey *key);


void mxf_snprintf(char *str, size_t size, const char *format, ...);
void mxf_vsnprintf(char *str, size_t size, const char *format, va_list ap);

char* mxf_strerror(int errnum, char *buf, size_t size);


void mxf_print_key(const mxfKey *key);
void mxf_sprint_key(char *str, const mxfKey *key);

void mxf_print_label(const mxfUL *label);
void mxf_sprint_label(char *str, const mxfUL *label);

void mxf_print_umid(const mxfUMID *umid);
void mxf_sprint_umid(char *str, const mxfUMID *umid);


extern mxf_generate_uuid_func mxf_generate_uuid;
extern mxf_get_timestamp_now_func mxf_get_timestamp_now;
extern mxf_generate_umid_func mxf_generate_umid;
extern mxf_generate_key_func mxf_generate_key;

void mxf_default_generate_uuid(mxfUUID *uuid);
void mxf_default_get_timestamp_now(mxfTimestamp *now);
void mxf_default_generate_umid(mxfUMID *umid);
void mxf_default_generate_key(mxfKey *key);

void mxf_set_regtest_funcs(void);
void mxf_regtest_generate_uuid(mxfUUID *uuid);
void mxf_regtest_get_timestamp_now(mxfTimestamp *now);
void mxf_regtest_generate_umid(mxfUMID *umid);
void mxf_regtest_generate_key(mxfKey *key);


int mxf_equals_key(const mxfKey *keyA, const mxfKey *keyB);
int mxf_equals_key_prefix(const mxfKey *keyA, const mxfKey *keyB, size_t cmpLen);
int mxf_equals_key_mod_regver(const mxfKey *keyA, const mxfKey *keyB);
int mxf_equals_ul(const mxfUL *labelA, const mxfUL *labelB);
int mxf_equals_ul_mod_regver(const mxfUL *labelA, const mxfUL *labelB);
int mxf_equals_uuid(const mxfUUID *uuidA, const mxfUUID *uuidB);
int mxf_equals_uid(const mxfUID *uidA, const mxfUID *uidB);
int mxf_equals_umid(const mxfUMID *umidA, const mxfUMID *umidB);
int mxf_equals_ext_umid(const mxfExtendedUMID *extUMIDA, const mxfExtendedUMID *extUMIDB);
int mxf_equals_rgba_layout(const mxfRGBALayout *layoutA, const mxfRGBALayout *layoutB);

int mxf_is_ul(const mxfUID *uid);
int mxf_is_swapped_ul(const mxfUID *uid);
void mxf_swap_uid(mxfUID *swap_uid, const mxfUID *uid);

int mxf_is_simple_idau_umid(const mxfUMID *umid);
void mxf_extract_umid_material_number(mxfUID *idau, const mxfUMID *umid);


MXFEssenceWrappingType mxf_get_essence_wrapping_type(const mxfUL *label);


size_t mxf_utf16_to_utf8(char *u8_str, const mxfUTF16Char *u16_str, size_t u8_size);
size_t mxf_utf8_to_utf16(mxfUTF16Char *u16_str, const char *u8_str, size_t u16_size);


uint32_t mxf_get_system_page_size();


#ifdef __cplusplus
}
#endif


#endif


