/*
 * Wraps a C-file and provides low-level MXF file functions
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier, Stuart Cunningham
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

#ifndef MXF_FILE_H_
#define MXF_FILE_H_


#ifdef __cplusplus
extern "C"
{
#endif


#define MAX_RUNIN_LEN       0xffff


typedef struct MXFFileSysData MXFFileSysData;

typedef struct
{
    /* MXF file implementations must set and implement these functions */
    void        (*close)        (MXFFileSysData *sysData);
    uint32_t    (*read)         (MXFFileSysData *sysData, uint8_t *data, uint32_t count);
    uint32_t    (*write)        (MXFFileSysData *sysData, const uint8_t *data, uint32_t count);
    int         (*get_char)     (MXFFileSysData *sysData);
    int         (*put_char)     (MXFFileSysData *sysData, int c);
    int         (*eof)          (MXFFileSysData *sysData);
    int         (*seek)         (MXFFileSysData *sysData, int64_t offset, int whence);
    int64_t     (*tell)         (MXFFileSysData *sysData);
    int         (*is_seekable)  (MXFFileSysData *sysData);
    int64_t     (*size)         (MXFFileSysData *sysData);

    /* private data for the MXF file implementation */
    void (*free_sys_data)(MXFFileSysData *sysData);
    MXFFileSysData *sysData;

    /* general data */
    uint8_t minLLen;
    uint16_t runinLen;
    uint8_t *zerosBuffer;
    uint32_t zerosBufferSize;
} MXFFile;


/* open files on disk */
int mxf_disk_file_open_new(const char *filename, MXFFile **mxfFile);
int mxf_disk_file_open_read(const char *filename, MXFFile **mxfFile);
int mxf_disk_file_open_modify(const char *filename, MXFFile **mxfFile);

/* wrap standard input and output in an MXF file */
int mxf_stdin_wrap_read(MXFFile **mxfFile);
int mxf_stdout_wrap_write(MXFFile **mxfFile);

void mxf_file_close(MXFFile **mxfFile);
void mxf_file_close_2(MXFFile **mxfFile, void (*free_func)(void*));
uint32_t mxf_file_read(MXFFile *mxfFile, uint8_t *data, uint32_t count);
uint32_t mxf_file_write(MXFFile *mxfFile, const uint8_t *data, uint32_t count);
int mxf_file_getc(MXFFile *mxfFile);
int mxf_file_putc(MXFFile *mxfFile, int c);
int mxf_file_eof(MXFFile *mxfFile);
int mxf_file_seek(MXFFile *mxfFile, int64_t offset, int whence);
int64_t mxf_file_tell(MXFFile *mxfFile);
int mxf_file_is_seekable(MXFFile *mxfFile);
int64_t mxf_file_size(MXFFile *mxfFile);


void mxf_file_set_min_llen(MXFFile *mxfFile, uint8_t llen);
uint8_t mxf_get_min_llen(MXFFile *mxfFile);


int mxf_write_uint8(MXFFile *mxfFile, uint8_t value);
int mxf_write_uint16(MXFFile *mxfFile, uint16_t value);
int mxf_write_uint32(MXFFile *mxfFile, uint32_t value);
int mxf_write_uint64(MXFFile *mxfFile, uint64_t value);
int mxf_write_int8(MXFFile *mxfFile, int8_t value);
int mxf_write_int16(MXFFile *mxfFile, int16_t value);
int mxf_write_int32(MXFFile *mxfFile, int32_t value);
int mxf_write_int64(MXFFile *mxfFile, int64_t value);
int mxf_read_uint8(MXFFile *mxfFile, uint8_t *value);
int mxf_read_uint16(MXFFile *mxfFile, uint16_t *value);
int mxf_read_uint32(MXFFile *mxfFile, uint32_t *value);
int mxf_read_uint64(MXFFile *mxfFile, uint64_t *value);
int mxf_read_int8(MXFFile *mxfFile, int8_t *value);
int mxf_read_int16(MXFFile *mxfFile, int16_t *value);
int mxf_read_int32(MXFFile *mxfFile, int32_t *value);
int mxf_read_int64(MXFFile *mxfFile, int64_t *value);

int mxf_read_k(MXFFile *mxfFile, mxfKey *key);
int mxf_read_l(MXFFile *mxfFile, uint8_t *llen, uint64_t *len);
int mxf_read_kl(MXFFile *mxfFile, mxfKey *key, uint8_t *llen, uint64_t *len);
int mxf_read_ul(MXFFile *mxfFile, mxfUL *value);
int mxf_read_key(MXFFile *mxfFile, mxfKey *value);
int mxf_read_uid(MXFFile *mxfFile, mxfUID *value);
int mxf_read_uuid(MXFFile *mxfFile, mxfUUID *value);
int mxf_read_local_tag(MXFFile *mxfFile, mxfLocalTag *tag);
int mxf_read_local_tl(MXFFile *mxfFile, mxfLocalTag *tag, uint16_t *len);

int mxf_skip(MXFFile *mxfFile, uint64_t len);


int mxf_write_local_tag(MXFFile *mxfFile, mxfLocalTag tag);
int mxf_write_local_tl(MXFFile *mxfFile, mxfLocalTag tag, uint16_t len);
int mxf_write_k(MXFFile *mxfFile, const mxfKey *key);
uint8_t mxf_write_l(MXFFile *mxfFile, uint64_t len);
int mxf_write_kl(MXFFile *mxfFile, const mxfKey *key, uint64_t len);
int mxf_write_fixed_l(MXFFile *mxfFile, uint8_t llen, uint64_t len);
int mxf_write_fixed_kl(MXFFile *mxfFile, const mxfKey *key, uint8_t llen, uint64_t len);
int mxf_write_ul(MXFFile *mxfFile, const mxfUL *label);
int mxf_write_uid(MXFFile *mxfFile, const mxfUID *uid);
int mxf_write_uuid(MXFFile *mxfFile, const mxfUUID *uuid);

uint8_t mxf_get_llen(MXFFile *mxfFile, uint64_t len);

int mxf_read_batch_header(MXFFile *mxfFile, uint32_t *len, uint32_t *eleLen);
int mxf_write_batch_header(MXFFile *mxfFile, uint32_t len, uint32_t eleLen);
int mxf_read_array_header(MXFFile *mxfFile, uint32_t *len, uint32_t *eleLen);
int mxf_write_array_header(MXFFile *mxfFile, uint32_t len, uint32_t eleLen);

int mxf_write_zeros(MXFFile *mxfFile, uint64_t len);

void mxf_set_runin_len(MXFFile *mxfFile, uint16_t runinLen);
uint16_t mxf_get_runin_len(MXFFile *mxfFile);


#ifdef __cplusplus
}
#endif


#endif


