/*
 * MXF file body essence container functions
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

#ifndef MXF_ESSENCE_CONTAINER_H_
#define MXF_ESSENCE_CONTAINER_H_


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct MXFEssenceElement
{
    mxfKey key;
    uint8_t llen;
    uint64_t startFilePos;
    uint64_t totalLen;
    uint64_t currentFilePos;
} MXFEssenceElement;


int mxf_is_gc_essence_element(const mxfKey *key);

int mxf_open_essence_element_write(MXFFile *mxfFile, const mxfKey *key, uint8_t llen, uint64_t len,
                                   MXFEssenceElement **essenceElement);
int mxf_write_essence_element_data(MXFFile *mxfFile, MXFEssenceElement *essenceElement,
                                   const uint8_t *data, uint32_t len);
int mxf_finalize_essence_element_write(MXFFile *mxfFile, MXFEssenceElement *essenceElement);

int mxf_open_essence_element_read(MXFFile *mxfFile, const mxfKey *key, uint8_t llen, uint64_t len,
                                  MXFEssenceElement **essenceElement);
int mxf_read_essence_element_data(MXFFile *mxfFile, MXFEssenceElement *essenceElement,
                                  uint32_t len, uint8_t *data, uint32_t *numRead);

void mxf_close_essence_element(MXFEssenceElement **essenceElement);

uint64_t mxf_get_essence_element_size(MXFEssenceElement *essenceElement);

uint32_t mxf_get_track_number(const mxfKey *essenceElementKey);
uint8_t mxf_get_essence_element_item_type(uint32_t trackNumber);
uint8_t mxf_get_essence_element_count(uint32_t trackNumber);
uint8_t mxf_get_essence_element_type(uint32_t trackNumber);
uint8_t mxf_get_essence_element_number(uint32_t trackNumber);


#ifdef __cplusplus
}
#endif


#endif


