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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>



int mxf_is_gc_essence_element(const mxfKey *key)
{
    /* generic container picture/sound/data element */
    if (key->octet0 == 0x06 &&
        key->octet1 == 0x0e &&
        key->octet2 == 0x2b &&
        key->octet3 == 0x34 &&
        key->octet4 == 0x01 &&
        key->octet6 == 0x01 &&
        key->octet8 == 0x0d &&
        key->octet9 == 0x01 &&
        key->octet10 == 0x03 &&
        key->octet11 == 0x01)
    {
        return 1;
    }
    /* generic container system element */
    else if (key->octet0 == 0x06 &&
             key->octet1 == 0x0e &&
             key->octet2 == 0x2b &&
             key->octet3 == 0x34 &&
             key->octet4 == 0x02 &&
             key->octet6 == 0x01 &&
             key->octet8 == 0x0d &&
             key->octet9 == 0x01 &&
             key->octet10 == 0x03 &&
             key->octet11 == 0x01)
    {
        return 1;
    }

    return 0;
}



static int create_essence_element(const mxfKey *key, uint8_t llen, MXFEssenceElement **essenceElement)
{
    MXFEssenceElement *newEssenceElement = NULL;

    CHK_MALLOC_ORET(newEssenceElement, MXFEssenceElement);
    memset(newEssenceElement, 0, sizeof(MXFEssenceElement));
    newEssenceElement->key = *key;
    newEssenceElement->llen = llen;

    *essenceElement = newEssenceElement;
    return 1;
}

static void free_essence_element(MXFEssenceElement **essenceElement)
{
    if (*essenceElement == NULL)
    {
        return;
    }

    SAFE_FREE(*essenceElement);
}



int mxf_open_essence_element_write(MXFFile *mxfFile, const mxfKey *key, uint8_t llen, uint64_t len,
                                   MXFEssenceElement **essenceElement)
{
    MXFEssenceElement *newEssenceElement = NULL;
    int64_t filePos;

    CHK_ORET(create_essence_element(key, llen, &newEssenceElement));

    CHK_OFAIL((filePos = mxf_file_tell(mxfFile)) >= 0);
    newEssenceElement->startFilePos = filePos;
    newEssenceElement->currentFilePos = newEssenceElement->startFilePos;
    CHK_OFAIL(mxf_write_k(mxfFile, key));
    CHK_OFAIL(mxf_write_fixed_l(mxfFile, llen, len));

    *essenceElement = newEssenceElement;
    return 1;

fail:
    SAFE_FREE(newEssenceElement);
    return 0;
}

int mxf_write_essence_element_data(MXFFile *mxfFile, MXFEssenceElement *essenceElement,
                                   const uint8_t *data, uint32_t len)
{
    uint64_t numWritten = mxf_file_write(mxfFile, data, len);
    essenceElement->totalLen += numWritten;
    essenceElement->currentFilePos += numWritten;

    if (numWritten != len)
    {
        return 0;
    }
    return 1;
}

int mxf_finalize_essence_element_write(MXFFile *mxfFile, MXFEssenceElement *essenceElement)
{
    int64_t filePos;

    assert(essenceElement != NULL);

    CHK_ORET((filePos = mxf_file_tell(mxfFile)) >= 0);

    CHK_ORET(mxf_file_seek(mxfFile, essenceElement->startFilePos + 16, SEEK_SET));
    CHK_ORET(mxf_write_fixed_l(mxfFile, essenceElement->llen, essenceElement->totalLen));

    CHK_ORET(mxf_file_seek(mxfFile, filePos, SEEK_SET));

    return 1;
}


int mxf_open_essence_element_read(MXFFile *mxfFile, const mxfKey *key, uint8_t llen, uint64_t len,
                                  MXFEssenceElement **essenceElement)
{
    MXFEssenceElement *newEssenceElement = NULL;
    int64_t filePos;

    CHK_ORET(create_essence_element(key, llen, &newEssenceElement));
    newEssenceElement->totalLen = len;

    CHK_OFAIL((filePos = mxf_file_tell(mxfFile)) >= 0);
    newEssenceElement->startFilePos = filePos;
    newEssenceElement->currentFilePos = newEssenceElement->startFilePos;

    *essenceElement = newEssenceElement;
    return 1;

fail:
    SAFE_FREE(newEssenceElement);
    return 0;
}

int mxf_read_essence_element_data(MXFFile *mxfFile, MXFEssenceElement *essenceElement,
                                  uint32_t len, uint8_t *data, uint32_t *numRead)
{
    uint32_t actualNumRead = 0;
    uint32_t actualLen = len;

    if ((uint64_t)(essenceElement->currentFilePos - essenceElement->startFilePos) >= essenceElement->totalLen)
    {
        *numRead = 0;
        return 1;
    }

    if (actualLen + (uint64_t)(essenceElement->currentFilePos - essenceElement->startFilePos) > essenceElement->totalLen)
    {
        actualLen = (uint32_t)(essenceElement->totalLen -
                        (uint64_t)(essenceElement->currentFilePos - essenceElement->startFilePos));
    }

    actualNumRead = (uint32_t)mxf_file_read(mxfFile, data, actualLen);
    essenceElement->currentFilePos += actualNumRead;
    CHK_ORET(actualNumRead == actualLen);

    *numRead = actualNumRead;
    return 1;
}

void mxf_close_essence_element(MXFEssenceElement **essenceElement)
{
    free_essence_element(essenceElement);
}

uint64_t mxf_get_essence_element_size(MXFEssenceElement *essenceElement)
{
    return essenceElement->totalLen;
}


uint32_t mxf_get_track_number(const mxfKey *essenceElementKey)
{
    return ((uint32_t)essenceElementKey->octet12 << 24) |
           ((uint32_t)essenceElementKey->octet13 << 16) |
           ((uint32_t)essenceElementKey->octet14 << 8) |
            (uint32_t)essenceElementKey->octet15;
}

uint8_t mxf_get_essence_element_item_type(uint32_t trackNumber)
{
    return (uint8_t)((trackNumber & 0xff000000) >> 24);
}

uint8_t mxf_get_essence_element_count(uint32_t trackNumber)
{
    return (uint8_t)((trackNumber & 0x00ff0000) >> 16);
}

uint8_t mxf_get_essence_element_type(uint32_t trackNumber)
{
    return (uint8_t)((trackNumber & 0x0000ff00) >> 8);
}

uint8_t mxf_get_essence_element_number(uint32_t trackNumber)
{
    return (uint8_t)((trackNumber & 0x000000ff));
}

