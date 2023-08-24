/*
 * Avid dictionary
 *
 * Copyright (C) 2008, British Broadcasting Corporation
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

#ifndef MXF_AVID_DICTIONARY_H_
#define MXF_AVID_DICTIONARY_H_


#ifdef __cplusplus
extern "C"
{
#endif



int mxf_avid_is_dictionary(MXFDataModel *dataModel, const mxfKey *setKey);
int mxf_avid_is_def_object(MXFDataModel *dataModel, const mxfKey *setKey);


int mxf_avid_create_dictionary(MXFHeaderMetadata *headerMetadata, MXFMetadataSet **dictSet);


int mxf_avid_set_defobject_items(MXFMetadataSet *dictSet, const mxfUL *id, const mxfUTF16Char *name,
                                 const mxfUTF16Char *description);

int mxf_avid_create_datadef(MXFMetadataSet *dictSet, const mxfUL *id, const mxfUTF16Char *name,
                            const mxfUTF16Char *description, MXFMetadataSet **defSet);
int mxf_avid_create_containerdef(MXFMetadataSet *dictSet, const mxfUL *id, const mxfUTF16Char *name,
                                 const mxfUTF16Char *description, MXFMetadataSet **defSet);
int mxf_avid_create_taggedvaluedef(MXFMetadataSet *dictSet, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, MXFMetadataSet **defSet);

/* additional defs are codec, operation, parameter, plugin, interpolation and klv def */


int mxf_initialise_dict_read_filter(MXFReadFilter *filter, int skipDataDefs);
void mxf_clear_dict_read_filter(MXFReadFilter *filter);


int mxf_avid_create_default_dictionary(MXFHeaderMetadata *headerMetadata, MXFMetadataSet **dictSet);


#ifdef __cplusplus
}
#endif


#endif


