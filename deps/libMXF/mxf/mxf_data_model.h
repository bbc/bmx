/*
 * MXF header metadata data model
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

#ifndef MXF_DATA_MODEL_H_
#define MXF_DATA_MODEL_H_


#ifdef __cplusplus
extern "C"
{
#endif


#define MXF_SET_K(name) \
    g_##name##_set_key

#define MXF_ITEM_K(setname, name) \
    g_##setname##_##name##_item_key

typedef enum
{
    MXF_BASIC_TYPE_CAT,
    MXF_ARRAY_TYPE_CAT,
    MXF_COMPOUND_TYPE_CAT,
    MXF_INTERPRET_TYPE_CAT
} MXFItemTypeCategory;

typedef struct
{
    unsigned int size;
} MXFBasicTypeInfo;

typedef struct
{
    unsigned int elementTypeId;
    unsigned int fixedSize;
} MXFArrayTypeInfo;

typedef struct
{
    char *name;
    unsigned int typeId;
} MXFCompoundTypeMemberInfo;

typedef struct
{
    MXFCompoundTypeMemberInfo members[16]; /* array terminated by typeId == 0 */
} MXFCompoundTypeInfo;

typedef struct
{
    unsigned int typeId;
    unsigned int fixedArraySize; /* only used if interpret array types */
} MXFInterpretTypeInfo;

typedef struct
{
    MXFItemTypeCategory category;
    unsigned int typeId;
    char *name;
    union
    {
        MXFBasicTypeInfo basic;
        MXFArrayTypeInfo array;
        MXFCompoundTypeInfo compound;
        MXFInterpretTypeInfo interpret;
    } info;
} MXFItemType;

typedef struct
{
    char *name;
    mxfKey setDefKey;
    mxfKey key;
    mxfLocalTag localTag;
    unsigned int typeId;
    int isRequired;
} MXFItemDef;

typedef struct MXFSetDef
{
    char *name;
    mxfKey parentSetDefKey;
    mxfKey key;
    MXFList itemDefs;
    struct MXFSetDef *parentSetDef;
} MXFSetDef;

typedef struct
{
    MXFList itemDefs;
    MXFTree setDefs;
    MXFItemType types[128]; /* index 0 is not used */
    unsigned int lastTypeId;
} MXFDataModel;


/* built-in types */

typedef enum
{
    MXF_UNKNOWN_TYPE = 0,

    /* basic */
    MXF_INT8_TYPE,
    MXF_INT16_TYPE,
    MXF_INT32_TYPE,
    MXF_INT64_TYPE,
    MXF_UINT8_TYPE,
    MXF_UINT16_TYPE,
    MXF_UINT32_TYPE,
    MXF_UINT64_TYPE,
    MXF_RAW_TYPE,

    /* array */
    MXF_UTF16STRING_TYPE,
    MXF_UTF16STRINGARRAY_TYPE,
    MXF_UTF8STRING_TYPE,
    MXF_INT8ARRAY_TYPE,
    MXF_INT16ARRAY_TYPE,
    MXF_INT32ARRAY_TYPE,
    MXF_INT64ARRAY_TYPE,
    MXF_UINT8ARRAY_TYPE,
    MXF_UINT16ARRAY_TYPE,
    MXF_UINT32ARRAY_TYPE,
    MXF_UINT64ARRAY_TYPE,
    MXF_ISO7STRING_TYPE,
    MXF_INT8BATCH_TYPE,
    MXF_INT16BATCH_TYPE,
    MXF_INT32BATCH_TYPE,
    MXF_INT64BATCH_TYPE,
    MXF_UINT8BATCH_TYPE,
    MXF_UINT16BATCH_TYPE,
    MXF_UINT32BATCH_TYPE,
    MXF_UINT64BATCH_TYPE,
    MXF_AUIDARRAY_TYPE,
    MXF_ULARRAY_TYPE,
    MXF_ULBATCH_TYPE,
    MXF_UUIDARRAY_TYPE,
    MXF_UUIDBATCH_TYPE,
    MXF_STRONGREFARRAY_TYPE,
    MXF_STRONGREFBATCH_TYPE,
    MXF_WEAKREFARRAY_TYPE,
    MXF_WEAKREFBATCH_TYPE,
    MXF_RATIONALARRAY_TYPE,
    MXF_RGBALAYOUT_TYPE,
    MXF_AES3_FIXED_DATA_ARRAY_TYPE,
    MXF_J2K_COMPONENT_SIZING_ARRAY_TYPE,
    MXF_THREE_COLOR_PRIMARIES_TYPE,

    /* compound */
    MXF_RATIONAL_TYPE,
    MXF_TIMESTAMP_TYPE,
    MXF_PRODUCTVERSION_TYPE,
    MXF_INDIRECT_TYPE,
    MXF_RGBALAYOUTCOMPONENT_TYPE,
    MXF_J2K_COMPONENT_SIZING_TYPE,
    MXF_COLOR_PRIMARY_TYPE,

    /* interpret */
    MXF_VERSIONTYPE_TYPE,
    MXF_UTF16_TYPE,
    MXF_UTF8_TYPE,
    MXF_BOOLEAN_TYPE,
    MXF_ISO7_TYPE,
    MXF_LENGTH_TYPE,
    MXF_POSITION_TYPE,
    MXF_RGBACODE_TYPE,
    MXF_STREAM_TYPE,
    MXF_DATAVALUE_TYPE,
    MXF_IDENTIFIER_TYPE,
    MXF_OPAQUE_TYPE,
    MXF_UMID_TYPE,
    MXF_UID_TYPE,
    MXF_UL_TYPE,
    MXF_UUID_TYPE,
    MXF_AUID_TYPE,
    MXF_PACKAGEID_TYPE,
    MXF_STRONGREF_TYPE,
    MXF_WEAKREF_TYPE,
    MXF_ORIENTATION_TYPE,
    MXF_CODED_CONTENT_TYPE_TYPE,
    MXF_AES3_FIXED_DATA_TYPE,
    MXF_J2K_EXTENDED_CAPABILITIES_TYPE,

    MXF_EXTENSION_TYPE /* extension types must have integer value >= this */

} MXFItemTypeId;


/* declare the baseline set and item keys */

#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;

#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

#define KEEP_DATA_MODEL_DEFS 1
#include <mxf/mxf_baseline_data_model.h>

/* declare the extensions set and item keys */

#undef KEEP_DATA_MODEL_DEFS
#include <mxf/mxf_extensions_data_model.h>


int mxf_load_data_model(MXFDataModel **dataModel);
void mxf_free_data_model(MXFDataModel **dataModel);

int mxf_register_set_def(MXFDataModel *dataModel, const char *name, const mxfKey *parentKey, const mxfKey *key);
int mxf_register_item_def(MXFDataModel *dataModel, const char *name, const mxfKey *setKey,
                          const mxfKey *key, mxfLocalTag tag, unsigned int typeId, int isRequired);

/* if the typeId parameter is 0 in the following functions, then a new id is created */
MXFItemType* mxf_register_basic_type(MXFDataModel *dataModel, const char *name, unsigned int typeId, unsigned int size);
MXFItemType* mxf_register_array_type(MXFDataModel *dataModel, const char *name, unsigned int typeId,
                                     unsigned int elementTypeId, unsigned int fixedSize);
MXFItemType* mxf_register_compound_type(MXFDataModel *dataModel, const char *name, unsigned int typeId);
/* adds a member to the end of the list */
int mxf_register_compound_type_member(MXFItemType *type, const char *memberName, unsigned int memberTypeId);
MXFItemType* mxf_register_interpret_type(MXFDataModel *dataModel, const char *name, unsigned int typeId,
                                         unsigned int interpretedTypeId, unsigned int fixedArraySize);


int mxf_finalise_data_model(MXFDataModel *dataModel);
int mxf_check_data_model(MXFDataModel *dataModel);

int mxf_find_set_def(MXFDataModel *dataModel, const mxfKey *key, MXFSetDef **setDef);
int mxf_find_item_def(MXFDataModel *dataModel, const mxfKey *key, MXFItemDef **itemDef);
int mxf_find_item_def_in_set_def(const mxfKey *key, const MXFSetDef *setDef, MXFItemDef **itemDef);

MXFItemType* mxf_get_item_def_type(MXFDataModel *dataModel, unsigned int typeId);

int mxf_is_subclass_of(MXFDataModel *dataModel, const mxfKey *setKey, const mxfKey *parentSetKey);
int mxf_is_subclass_of_2(MXFDataModel *dataModel, MXFSetDef *setDef, const mxfKey *parentSetKey);


int mxf_clone_set_def(MXFDataModel *fromDataModel, MXFSetDef *fromSetDef,
                      MXFDataModel *toDataModel, MXFSetDef **toSetDef);


#ifdef __cplusplus
}
#endif


#endif



