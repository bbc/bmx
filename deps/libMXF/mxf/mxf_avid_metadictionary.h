/*
 * Avid (AAF) Meta-dictionary
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

#ifndef MXF_AVID_METADICTIONARY_H_
#define MXF_AVID_METADICTIONARY_H_


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct AvidMetaDictionary AvidMetaDictionary;


int mxf_avid_is_metadictionary(MXFDataModel *dataModel, const mxfKey *setKey);
int mxf_avid_is_metadef(MXFDataModel *dataModel, const mxfKey *setKey);


int mxf_avid_create_metadictionary(MXFHeaderMetadata *headerMetadata, AvidMetaDictionary **metaDict);
int mxf_avid_add_default_metadictionary(AvidMetaDictionary *metaDict);
int mxf_avid_finalise_metadictionary(AvidMetaDictionary **metaDict, MXFMetadataSet **metaDictSet);


int mxf_avid_set_metadef_items(MXFMetadataSet *set, const mxfUL *id, const mxfUTF16Char *name,
                               const mxfUTF16Char *description);

int mxf_avid_create_classdef(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                             const mxfUTF16Char *description, const mxfUL *parentId, mxfBoolean isConcrete,
                             MXFMetadataSet **classDefSet);
int mxf_avid_create_propertydef(AvidMetaDictionary *metaDict, MXFPrimerPack *primerPack,
                                MXFMetadataSet *classDefSet, const mxfUL *id, const mxfUTF16Char *name,
                                const mxfUTF16Char *description, const mxfUL *typeId, mxfBoolean isOptional,
                                mxfLocalTag localId, mxfBoolean isUniqueIdentifier, MXFMetadataSet **propertyDefSet);

int mxf_avid_create_typedef(AvidMetaDictionary *metaDict, const mxfKey *setId, const mxfUL *id,
                            const mxfUTF16Char *name, const mxfUTF16Char *description, MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_char(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                 const mxfUTF16Char *description, MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_enum(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                 const mxfUTF16Char *description, const mxfUL *typeId, MXFMetadataSet **typeDefSet);
int mxf_avid_add_typedef_enum_element(MXFMetadataSet *typeDefSet, const mxfUTF16Char *name, int64_t value);
int mxf_avid_create_typedef_extenum(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                    const mxfUTF16Char *description, MXFMetadataSet **typeDefSet);
int mxf_avid_add_typedef_extenum_element(MXFMetadataSet *typeDefSet, const mxfUTF16Char *name, const mxfUL *value);
int mxf_avid_create_typedef_fixedarray(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                       const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                       uint32_t elementCount, MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_indirect(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                     const mxfUTF16Char *description, MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_integer(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                    const mxfUTF16Char *description, uint8_t size, mxfBoolean isSigned,
                                    MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_opaque(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_record(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, MXFMetadataSet **typeDefSet);
int mxf_avid_add_typedef_record_member(AvidMetaDictionary *metaDict, MXFMetadataSet *typeDefSet,
                                       const mxfUTF16Char *name, const mxfUL *typeId);
int mxf_avid_create_typedef_rename(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, const mxfUL *renamedTypeId,
                                   MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_set(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_stream(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_string(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                   MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_strongref(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                      const mxfUTF16Char *description, const mxfUL *referencedTypeId,
                                      MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_vararray(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                     const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                     MXFMetadataSet **typeDefSet);
int mxf_avid_create_typedef_weakref(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                    const mxfUTF16Char *description, const mxfUL *referencedTypeId,
                                    MXFMetadataSet **typeDefSet);
int mxf_avid_add_typedef_weakref_target(MXFMetadataSet *typeDefSet, const mxfUL *targetId);


int mxf_initialise_metadict_read_filter(MXFReadFilter *filter);
void mxf_clear_metadict_read_filter(MXFReadFilter *filter);


int mxf_avid_create_default_metadictionary(MXFHeaderMetadata *headerMetadata, MXFMetadataSet **metaDictSet);



#ifdef __cplusplus
}
#endif


#endif


