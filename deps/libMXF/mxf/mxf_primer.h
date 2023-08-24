/*
 * MXF header metadata primer pack
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

#ifndef MXF_PRIMER_H_
#define MXF_PRIMER_H_



#ifdef __cplusplus
extern "C"
{
#endif


typedef struct
{
    mxfLocalTag localTag;
    mxfUID uid;
} MXFPrimerPackEntry;

typedef struct
{
    mxfLocalTag nextTag;
    MXFList entries;
} MXFPrimerPack;



int mxf_is_primer_pack(const mxfKey *key);

int mxf_create_primer_pack(MXFPrimerPack **primerPack);
void mxf_free_primer_pack(MXFPrimerPack **primerPack);

int mxf_register_primer_entry(MXFPrimerPack *primerPack, const mxfUID *itemKey, mxfLocalTag newTag,
                              mxfLocalTag *assignedTag);

int mxf_get_item_key(MXFPrimerPack *primerPack, mxfLocalTag localTag, mxfKey *key);
int mxf_get_item_tag(MXFPrimerPack *primerPack, const mxfKey *key, mxfLocalTag *localTag);

int mxf_create_item_tag(MXFPrimerPack *primerPack, mxfLocalTag *localTag);

int mxf_write_primer_pack(MXFFile *mxfFile, MXFPrimerPack *primerPack);
int mxf_read_primer_pack(MXFFile *mxfFile, MXFPrimerPack **primerPack);

void mxf_get_primer_pack_size(MXFFile *mxfFile, MXFPrimerPack *primerPack, uint64_t *size);


#ifdef __cplusplus
}
#endif


#endif


