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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>


typedef struct
{
    mxfUL identification;
    mxfUUID instanceUID;
} MetaDefData;

typedef struct
{
    MXFMetadataItem *item;
    int recordMemberIndex;
    mxfUL targetIdentification;
} WeakRefData;

struct AvidMetaDictionary
{
    MXFHeaderMetadata *headerMetadata;
    MXFMetadataSet *metaDictSet;
    MXFList classMetaDefList;
    MXFList typeMetaDefList;
    MXFList classWeakRefList;
    MXFList typeWeakRefList;
};


static int add_weakref_to_list(MXFList *list, MXFMetadataItem *item, int recordMemberIndex,
                               const mxfUL *targetIdentification)
{
    WeakRefData *data = NULL;

    CHK_MALLOC_ORET(data, WeakRefData);
    data->item = item;
    data->recordMemberIndex = recordMemberIndex;
    data->targetIdentification = *targetIdentification;

    CHK_OFAIL(mxf_append_list_element(list, (void*)data));
    data = NULL;

    return 1;

fail:
    SAFE_FREE(data);
    return 0;
}

static int add_metadef_to_list(MXFList *list, const mxfUL *identification, const mxfUUID *instanceUID)
{
    MetaDefData *data = NULL;

    CHK_MALLOC_ORET(data, MetaDefData);
    data->identification = *identification;
    data->instanceUID = *instanceUID;

    CHK_OFAIL(mxf_append_list_element(list, (void*)data));
    data = NULL;

    return 1;

fail:
    SAFE_FREE(data);
    return 0;
}

static uint8_t* get_array_element(MXFMetadataItem *item, int index)
{
    uint32_t arrayLen;
    uint32_t arrayItemLen;

    mxf_get_array_header(item->value, &arrayLen, &arrayItemLen);

    return item->value + 8 + index * arrayItemLen;
}

static int find_weakref_target_instance_uid(MXFList *mapList, const mxfUL *targetIdentification, mxfUUID *instanceUID)
{
    MXFListIterator iter;

    mxf_initialise_list_iter(&iter, mapList);
    while (mxf_next_list_iter_element(&iter))
    {
        MetaDefData *data = (MetaDefData*)mxf_get_iter_element(&iter);

        if (mxf_equals_ul(&data->identification, targetIdentification))
        {
            *instanceUID = data->instanceUID;
            return 1;
        }
    }

    return 0;
}


static int metadict_before_set_read(void *privateData, MXFHeaderMetadata *headerMetadata,
                                    const mxfKey *key, uint8_t llen, uint64_t len, int *skip)
{
    (void)privateData;
    (void)llen;
    (void)len;

    if (mxf_avid_is_metadictionary(headerMetadata->dataModel, key) ||
        mxf_avid_is_metadef(headerMetadata->dataModel, key))
    {
        *skip = 1;
    }
    else
    {
        *skip = 0;
    }

    return 1;
}

static int append_name_to_string_array(MXFMetadataSet *set, const mxfKey *itemKey, const mxfUTF16Char *name)
{
    uint8_t *nameArray = NULL;
    uint16_t existingNameArraySize = 0;
    uint16_t nameArraySize = 0;
    MXFMetadataItem *namesItem = NULL;

    if (mxf_have_item(set, itemKey))
    {
        CHK_ORET(mxf_get_item(set, itemKey, &namesItem));
        existingNameArraySize = namesItem->length;
    }
    nameArraySize = mxf_get_external_utf16string_size(name);
    CHK_ORET(nameArraySize < UINT16_MAX - existingNameArraySize);
    nameArraySize += existingNameArraySize;

    CHK_MALLOC_ARRAY_ORET(nameArray, uint8_t, nameArraySize);
    if (existingNameArraySize > 0)
    {
        memcpy(nameArray, namesItem->value, existingNameArraySize);
    }
    mxf_set_utf16string(name, &nameArray[existingNameArraySize], nameArraySize - existingNameArraySize);
    nameArray[nameArraySize - 2] = 0;
    nameArray[nameArraySize - 1] = 0;

    CHK_OFAIL(mxf_set_item(set, itemKey, nameArray, nameArraySize));

    SAFE_FREE(nameArray);
    return 1;

fail:
    SAFE_FREE(nameArray);
    return 0;
}

static mxfUL* bounce_label(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t d5,
                           uint8_t d6, uint8_t d7, uint8_t d8, uint8_t d9, uint8_t d10, uint8_t d11,
                           uint8_t d12, uint8_t d13, uint8_t d14, uint8_t d15,
                           mxfUL *result)
{
    result->octet0  = d0;
    result->octet1  = d1;
    result->octet2  = d2;
    result->octet3  = d3;
    result->octet4  = d4;
    result->octet5  = d5;
    result->octet6  = d6;
    result->octet7  = d7;
    result->octet8  = d8;
    result->octet9  = d9;
    result->octet10 = d10;
    result->octet11 = d11;
    result->octet12 = d12;
    result->octet13 = d13;
    result->octet14 = d14;
    result->octet15 = d15;

    return result;
}

static void free_avid_metadictionary(AvidMetaDictionary **metaDict)
{
    if (*metaDict == NULL)
    {
        return;
    }

    if ((*metaDict)->metaDictSet != NULL)
    {
        mxf_remove_set((*metaDict)->headerMetadata, (*metaDict)->metaDictSet);
        mxf_free_set(&(*metaDict)->metaDictSet);
    }

    mxf_clear_list(&(*metaDict)->classMetaDefList);
    mxf_clear_list(&(*metaDict)->typeMetaDefList);
    mxf_clear_list(&(*metaDict)->classWeakRefList);
    mxf_clear_list(&(*metaDict)->typeWeakRefList);

    SAFE_FREE(*metaDict);
}


int mxf_avid_is_metadictionary(MXFDataModel *dataModel, const mxfKey *setKey)
{
    (void)dataModel;
    return mxf_equals_key(setKey, &MXF_SET_K(MetaDictionary));
}

int mxf_avid_is_metadef(MXFDataModel *dataModel, const mxfKey *setKey)
{
    (void)dataModel;
    return mxf_equals_key_prefix(setKey, &MXF_SET_K(MetaDefinition), 13);
}


int mxf_avid_create_metadictionary(MXFHeaderMetadata *headerMetadata, AvidMetaDictionary **metaDict)
{
    AvidMetaDictionary *newMetaDict = NULL;

    CHK_MALLOC_ORET(newMetaDict, AvidMetaDictionary);
    memset(newMetaDict, 0, sizeof(*newMetaDict));
    newMetaDict->headerMetadata = headerMetadata;

    mxf_initialise_list(&newMetaDict->classMetaDefList, free);
    mxf_initialise_list(&newMetaDict->typeMetaDefList, free);
    mxf_initialise_list(&newMetaDict->classWeakRefList, free);
    mxf_initialise_list(&newMetaDict->typeWeakRefList, free);

    CHK_OFAIL(mxf_create_set(headerMetadata, &MXF_SET_K(MetaDictionary), &newMetaDict->metaDictSet));

    *metaDict = newMetaDict;
    return 1;

fail:
    free_avid_metadictionary(&newMetaDict);
    return 0;
}

int mxf_avid_add_default_metadictionary(AvidMetaDictionary *metaDict)
{
    MXFMetadataSet *classDefSet;
    MXFMetadataSet *set;
    mxfUL label1;
    mxfUL label2;
    mxfUL label3;

    /* register default meta-definitions */

    /* set temporary weak reference values, equal to the MetaDefinition::Identification rather than
       the InstanceUID of the target, which will be replaced later in mxf_avid_finalise_metadictionary */

#define LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    bounce_label(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, &label1)

#define LABEL_2(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    bounce_label(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, &label2)

#define WEAKREF(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    bounce_label(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, &label3)

#define CLASS_DEF(id, name, description, parentId, isConcrete) \
    CHK_ORET(mxf_avid_create_classdef(metaDict, id, name, description, parentId, isConcrete, &classDefSet));

#define PROPERTY_DEF(id, name, description, typeId, isOptional, localId, isUniqueId) \
    CHK_ORET(mxf_avid_create_propertydef(metaDict, metaDict->headerMetadata->primerPack, classDefSet, \
        id, name, description, typeId, isOptional, localId, isUniqueId, &set));

#define CHARACTER_DEF(id, name, description) \
    CHK_ORET(mxf_avid_create_typedef_char(metaDict, id, name, description, &set));

#define ENUM_DEF(id, name, description, typeId) \
    CHK_ORET(mxf_avid_create_typedef_enum(metaDict, id, name, description, typeId, &set));
#define ENUM_ELEMENT(name, value) \
    CHK_ORET(mxf_avid_add_typedef_enum_element(set, name, value));

#define EXTENUM_DEF(id, name, description) \
    CHK_ORET(mxf_avid_create_typedef_extenum(metaDict, id, name, description, &set));
#define EXTENUM_ELEMENT(name, value) \
    CHK_ORET(mxf_avid_add_typedef_extenum_element(set, name, value));

#define FIXEDARRAY_DEF(id, name, description, typeId, count) \
    CHK_ORET(mxf_avid_create_typedef_fixedarray(metaDict, id, name, description, typeId, count, &set));

#define INDIRECT_DEF(id, name, description) \
    CHK_ORET(mxf_avid_create_typedef_indirect(metaDict, id, name, description, &set));

#define INTEGER_DEF(id, name, description, size, isSigned) \
    CHK_ORET(mxf_avid_create_typedef_integer(metaDict, id, name, description, size, isSigned, &set)); \

#define OPAQUE_DEF(id, name, description) \
    CHK_ORET(mxf_avid_create_typedef_opaque(metaDict, id, name, description, &set));

#define RENAME_DEF(id, name, description, typeId) \
    CHK_ORET(mxf_avid_create_typedef_rename(metaDict, id, name, description, typeId, &set));

#define RECORD_DEF(id, name, description) \
    CHK_ORET(mxf_avid_create_typedef_record(metaDict, id, name, description, &set));
#define RECORD_MEMBER(name, type) \
    CHK_ORET(mxf_avid_add_typedef_record_member(metaDict, set, name, type));

#define SET_DEF(id, name, description, typeId) \
    CHK_ORET(mxf_avid_create_typedef_set(metaDict, id, name, description, typeId, &set));

#define STREAM_DEF(id, name, description) \
    CHK_ORET(mxf_avid_create_typedef_stream(metaDict, id, name, description, &set));

#define STRING_DEF(id, name, description, typeId) \
    CHK_ORET(mxf_avid_create_typedef_string(metaDict, id, name, description, typeId, &set));

#define STRONGOBJREF_DEF(id, name, description, refTypeId) \
    CHK_ORET(mxf_avid_create_typedef_strongref(metaDict, id, name, description, refTypeId, &set));

#define WEAKOBJREF_DEF(id, name, description, refTypeId) \
    CHK_ORET(mxf_avid_create_typedef_weakref(metaDict, id, name, description, refTypeId, &set));
#define WEAKOBJREF_TARGET_ELEMENT(id) \
    CHK_ORET(mxf_avid_add_typedef_weakref_target(set, id));

#define VARARRAY_DEF(id, name, description, typeId) \
    CHK_ORET(mxf_avid_create_typedef_vararray(metaDict, id, name, description, typeId, &set));



#include "mxf_avid_metadictionary_data.h"

    return 1;
}

int mxf_avid_finalise_metadictionary(AvidMetaDictionary **metaDict, MXFMetadataSet **metaDictSet)
{
    MXFListIterator iter;
    mxfUUID targetInstanceUID;

    /* replace the temporary MetaDefinition::Identification value in weak references with InstanceUID of the
       weak-referenced MetaDefinition */

    mxf_initialise_list_iter(&iter, &(*metaDict)->classWeakRefList);
    while (mxf_next_list_iter_element(&iter))
    {
        WeakRefData *data = (WeakRefData*)mxf_get_iter_element(&iter);

        CHK_ORET(find_weakref_target_instance_uid(&(*metaDict)->classMetaDefList, &data->targetIdentification,
                                                  &targetInstanceUID));

        if (data->recordMemberIndex >= 0)
        {
            mxf_set_uuid(&targetInstanceUID, get_array_element(data->item, data->recordMemberIndex));
        }
        else
        {
            mxf_set_uuid(&targetInstanceUID, data->item->value);
        }
    }

    mxf_initialise_list_iter(&iter, &(*metaDict)->typeWeakRefList);
    while (mxf_next_list_iter_element(&iter))
    {
        WeakRefData *data = (WeakRefData*)mxf_get_iter_element(&iter);

        CHK_ORET(find_weakref_target_instance_uid(&(*metaDict)->typeMetaDefList, &data->targetIdentification,
                                                  &targetInstanceUID));

        if (data->recordMemberIndex >= 0)
        {
            mxf_set_uuid(&targetInstanceUID, get_array_element(data->item, data->recordMemberIndex));
        }
        else
        {
            mxf_set_uuid(&targetInstanceUID, data->item->value);
        }
    }

    *metaDictSet = (*metaDict)->metaDictSet;
    (*metaDict)->metaDictSet = NULL;

    free_avid_metadictionary(metaDict);

    return 1;
}


int mxf_avid_set_metadef_items(MXFMetadataSet *set, const mxfUL *id, const mxfUTF16Char *name,
                               const mxfUTF16Char *description)
{
    CHK_ORET(mxf_set_ul_item(set, &MXF_ITEM_K(MetaDefinition, Identification), id));
    CHK_ORET(name != NULL);
    CHK_ORET(mxf_set_utf16string_item(set, &MXF_ITEM_K(MetaDefinition, Name), name));
    if (description != NULL)
    {
        CHK_ORET(mxf_set_utf16string_item(set, &MXF_ITEM_K(MetaDefinition, Description), description));
    }

    return 1;
}

int mxf_avid_create_classdef(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                             const mxfUTF16Char *description, const mxfUL *parentId, mxfBoolean isConcrete,
                             MXFMetadataSet **classDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_create_set(metaDict->headerMetadata, &MXF_SET_K(ClassDefinition), &newSet));
    CHK_ORET(mxf_add_array_item_strongref(metaDict->metaDictSet, &MXF_ITEM_K(MetaDictionary, ClassDefinitions), newSet));

    CHK_ORET(mxf_avid_set_metadef_items(newSet, id, name, description));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(ClassDefinition, ParentClass), parentId));
    CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(ClassDefinition, IsConcrete), isConcrete));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(ClassDefinition, ParentClass), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->classWeakRefList, item, -1, parentId));
    CHK_ORET(add_metadef_to_list(&metaDict->classMetaDefList, id, &newSet->instanceUID));

    *classDefSet = newSet;
    return 1;
}

int mxf_avid_create_propertydef(AvidMetaDictionary *metaDict, MXFPrimerPack *primerPack, MXFMetadataSet *classDefSet,
                                const mxfUL *id, const mxfUTF16Char *name, const mxfUTF16Char *description,
                                const mxfUL *typeId, mxfBoolean isOptional, mxfLocalTag localId,
                                mxfBoolean isUniqueIdentifier, MXFMetadataSet **propertyDefSet)
{
    MXFMetadataSet *newSet = NULL;
    mxfLocalTag assignedLocalId;

    CHK_ORET(mxf_register_primer_entry(primerPack, id, localId, &assignedLocalId));

    CHK_ORET(mxf_create_set(metaDict->headerMetadata, &MXF_SET_K(PropertyDefinition), &newSet));
    CHK_ORET(mxf_add_array_item_strongref(classDefSet, &MXF_ITEM_K(ClassDefinition, Properties), newSet));

    CHK_ORET(mxf_avid_set_metadef_items(newSet, id, name, description));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(PropertyDefinition, Type), typeId));
    CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(PropertyDefinition, IsOptional), isOptional));
    CHK_ORET(mxf_set_uint16_item(newSet, &MXF_ITEM_K(PropertyDefinition, LocalIdentification), assignedLocalId));
    if (isUniqueIdentifier)
    {
        CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(PropertyDefinition, IsUniqueIdentifier), isUniqueIdentifier));
    }

    *propertyDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef(AvidMetaDictionary *metaDict, const mxfKey *setId, const mxfUL *id,
                            const mxfUTF16Char *name, const mxfUTF16Char *description, MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;

    CHK_ORET(mxf_create_set(metaDict->headerMetadata, setId, &newSet));
    CHK_ORET(mxf_add_array_item_strongref(metaDict->metaDictSet, &MXF_ITEM_K(MetaDictionary, TypeDefinitions), newSet));

    CHK_ORET(mxf_avid_set_metadef_items(newSet, id, name, description));

    CHK_ORET(add_metadef_to_list(&metaDict->typeMetaDefList, id, &newSet->instanceUID));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_char(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                 const mxfUTF16Char *description, MXFMetadataSet **typeDefSet)
{
    return mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionCharacter), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_enum(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                 const mxfUTF16Char *description, const mxfUL *typeId, MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionEnumeration), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Type), typeId));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Type), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->typeWeakRefList, item, -1, typeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_enum_element(MXFMetadataSet *typeDefSet, const mxfUTF16Char *name, int64_t value)
{
    uint8_t *elementValue;

    CHK_ORET(append_name_to_string_array(typeDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Names), name));

    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Values), 8, 1, &elementValue));
    mxf_set_int64(value, elementValue);

    return 1;
}

int mxf_avid_create_typedef_extenum(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                    const mxfUTF16Char *description, MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionExtendibleEnumeration), id, name, description,
                                     &newSet));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_extenum_element(MXFMetadataSet *typeDefSet, const mxfUTF16Char *name, const mxfUL *value)
{
    uint8_t *elementValue;

    CHK_ORET(append_name_to_string_array(typeDefSet, &MXF_ITEM_K(TypeDefinitionExtendibleEnumeration, Names), name));

    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionExtendibleEnumeration, Values), mxfUL_extlen,
                                 1, &elementValue));
    mxf_set_ul(value, elementValue);

    return 1;
}

int mxf_avid_create_typedef_fixedarray(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                       const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                       uint32_t elementCount, MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionFixedArray), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementType), elementTypeId));
    CHK_ORET(mxf_set_uint32_item(newSet, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementCount), elementCount));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementType), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->typeWeakRefList, item, -1, elementTypeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_indirect(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                     const mxfUTF16Char *description, MXFMetadataSet **typeDefSet)
{
    return mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionIndirect), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_integer(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                    const mxfUTF16Char *description, uint8_t size, mxfBoolean isSigned,
                                    MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionInteger), id, name, description, &newSet));

    CHK_ORET(mxf_set_uint8_item(newSet, &MXF_ITEM_K(TypeDefinitionInteger, Size), size));
    CHK_ORET(mxf_set_boolean_item(newSet, &MXF_ITEM_K(TypeDefinitionInteger, IsSigned), isSigned));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_opaque(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, MXFMetadataSet **typeDefSet)
{
    return mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionOpaque), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_record(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionRecord), id, name, description, &newSet));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_record_member(AvidMetaDictionary *metaDict, MXFMetadataSet *typeDefSet,
                                       const mxfUTF16Char *name, const mxfUL *typeId)
{
    uint8_t *elementValue;
    MXFMetadataItem *item;
    uint32_t memberCount;

    CHK_ORET(append_name_to_string_array(typeDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberNames), name));

    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberTypes), mxfUL_extlen,
                                 1, &elementValue));
    mxf_set_ul(typeId, elementValue);

    CHK_ORET(mxf_get_array_item_count(typeDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberTypes), &memberCount));

    CHK_ORET(mxf_get_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberTypes), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->typeWeakRefList, item, memberCount - 1, typeId));

    return 1;
}

int mxf_avid_create_typedef_rename(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, const mxfUL *renamedTypeId,
                                   MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionRename), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionRename, RenamedType), renamedTypeId));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionRename, RenamedType), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->typeWeakRefList, item, -1, renamedTypeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_set(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionSet), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionSet, ElementType), elementTypeId));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionSet, ElementType), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->typeWeakRefList, item, -1, elementTypeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_stream(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, MXFMetadataSet **typeDefSet)
{
    return mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionStream), id, name, description, typeDefSet);
}

int mxf_avid_create_typedef_string(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                   const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                   MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionString), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionString, ElementType), elementTypeId));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionString, ElementType), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->typeWeakRefList, item, -1, elementTypeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_strongref(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                      const mxfUTF16Char *description, const mxfUL *referencedTypeId,
                                      MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionStrongObjectReference), id, name,
                                     description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionStrongObjectReference, ReferencedType),
                             referencedTypeId));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionStrongObjectReference, ReferencedType), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->classWeakRefList, item, -1, referencedTypeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_vararray(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                     const mxfUTF16Char *description, const mxfUL *elementTypeId,
                                     MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionVariableArray), id, name, description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionVariableArray, ElementType), elementTypeId));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionVariableArray, ElementType), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->typeWeakRefList, item, -1, elementTypeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_create_typedef_weakref(AvidMetaDictionary *metaDict, const mxfUL *id, const mxfUTF16Char *name,
                                    const mxfUTF16Char *description, const mxfUL *referencedTypeId,
                                    MXFMetadataSet **typeDefSet)
{
    MXFMetadataSet *newSet = NULL;
    MXFMetadataItem *item;

    CHK_ORET(mxf_avid_create_typedef(metaDict, &MXF_SET_K(TypeDefinitionWeakObjectReference), id, name,
                                     description, &newSet));

    CHK_ORET(mxf_set_ul_item(newSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferencedType), referencedTypeId));

    CHK_ORET(mxf_get_item(newSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferencedType), &item));
    CHK_ORET(add_weakref_to_list(&metaDict->classWeakRefList, item, -1, referencedTypeId));

    *typeDefSet = newSet;
    return 1;
}

int mxf_avid_add_typedef_weakref_target(MXFMetadataSet *typeDefSet, const mxfUL *targetId)
{
    uint8_t *elementValue;

    CHK_ORET(mxf_grow_array_item(typeDefSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferenceTargetSet),
                                 mxfUL_extlen, 1, &elementValue));
    mxf_set_ul(targetId, elementValue);

    return 1;
}


int mxf_initialise_metadict_read_filter(MXFReadFilter *filter)
{
    filter->privateData = NULL;
    filter->before_set_read = metadict_before_set_read;
    filter->after_set_read = NULL;

    return 1;
}

void mxf_clear_metadict_read_filter(MXFReadFilter *filter)
{
    if (filter == NULL)
    {
        return;
    }

    /* nothing to clear */
    return;
}


int mxf_avid_create_default_metadictionary(MXFHeaderMetadata *headerMetadata, MXFMetadataSet **metaDictSet)
{
    AvidMetaDictionary *metaDict = NULL;

    CHK_ORET(mxf_avid_create_metadictionary(headerMetadata, &metaDict));
    CHK_OFAIL(mxf_avid_add_default_metadictionary(metaDict));
    CHK_OFAIL(mxf_avid_finalise_metadictionary(&metaDict, metaDictSet));

    return 1;

fail:
    if (metaDict != NULL)
    {
        free_avid_metadictionary(&metaDict);
    }
    return 0;
}

