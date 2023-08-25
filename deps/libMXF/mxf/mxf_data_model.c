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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>



static void clear_type(MXFItemType *type)
{
    size_t i;

    if (!type)
        return;

    if (type->typeId != 0) {
        SAFE_FREE(type->name);
        if (type->category == MXF_COMPOUND_TYPE_CAT) {
            for (i = 0; i < ARRAY_SIZE(type->info.compound.members); i++)
                SAFE_FREE(type->info.compound.members[i].name);
        }
    }
    memset(type, 0, sizeof(MXFItemType));
}

static void free_item_def(MXFItemDef **itemDef)
{
    if (!(*itemDef))
        return;

    SAFE_FREE((*itemDef)->name);
    SAFE_FREE(*itemDef);
}

static void free_item_def_in_list(void *data)
{
    MXFItemDef *itemDef;

    if (!data)
        return;

    itemDef = (MXFItemDef*)data;
    free_item_def(&itemDef);
}

static void free_set_def(MXFSetDef **setDef)
{
    if (!(*setDef))
        return;

    SAFE_FREE((*setDef)->name);
    SAFE_FREE(*setDef);
}

static void free_set_def_in_tree(void *data)
{
    MXFSetDef *setDef;

    if (!data)
        return;

    setDef = (MXFSetDef*)data;
    mxf_clear_list(&setDef->itemDefs);
    free_set_def(&setDef);
}

static int compare_set_def_in_tree(void *left_in, void *right_in)
{
    MXFSetDef *left = (MXFSetDef*)left_in;
    MXFSetDef *right = (MXFSetDef*)right_in;

    return memcmp(&left->key, &right->key, sizeof(left->key));
}

static int item_def_eq(void *data, void *info)
{
    return mxf_equals_key((mxfKey*)info, &((MXFItemDef*)data)->key);
}

static unsigned int get_new_type_id(MXFDataModel *dataModel)
{
    size_t i;
    unsigned int lastTypeId;
    unsigned int typeId = 0;

    if (dataModel->lastTypeId == 0 ||
        dataModel->lastTypeId >= ARRAY_SIZE(dataModel->types))
    {
        lastTypeId = MXF_EXTENSION_TYPE;
    }
    else
    {
        lastTypeId = dataModel->lastTypeId;
    }

    /* try from the last type id to the end of the list */
    for (i = lastTypeId; i < ARRAY_SIZE(dataModel->types); i++) {
        if (dataModel->types[i].typeId == 0) {
            typeId = (unsigned int)i;
            break;
        }
    }

    if (typeId == 0 && lastTypeId > MXF_EXTENSION_TYPE) {
        /* try from MXF_EXTENSION_TYPE to lastTypeId */
        for (i = MXF_EXTENSION_TYPE; i < lastTypeId; i++) {
            if (dataModel->types[i].typeId == 0) {
                typeId = (unsigned int)i;
                break;
            }
        }
    }

    return typeId;
}

static MXFSetDef* register_set_def(MXFDataModel *dataModel, const char *name, const mxfKey *parentKey,
                                   const mxfKey *key)
{
    MXFSetDef *newSetDef = NULL;

    CHK_MALLOC_ORET(newSetDef, MXFSetDef);
    memset(newSetDef, 0, sizeof(MXFSetDef));
    if (name)
        CHK_OFAIL((newSetDef->name = strdup(name)) != NULL);
    newSetDef->parentSetDefKey = *parentKey;
    newSetDef->key = *key;
    mxf_initialise_list(&newSetDef->itemDefs, NULL);

    CHK_OFAIL(mxf_tree_insert(&dataModel->setDefs, newSetDef));

    return newSetDef;

fail:
    free_set_def(&newSetDef);
    return NULL;
}

static MXFItemDef* register_item_def(MXFDataModel *dataModel, const char *name, const mxfKey *setKey,
                                     const mxfKey *key, mxfLocalTag tag, unsigned int typeId, int isRequired)
{
    MXFItemDef *newItemDef = NULL;

    CHK_MALLOC_ORET(newItemDef, MXFItemDef);
    memset(newItemDef, 0, sizeof(MXFItemDef));
    if (name)
        CHK_OFAIL((newItemDef->name = strdup(name)) != NULL);
    newItemDef->setDefKey = *setKey;
    newItemDef->key = *key;
    newItemDef->localTag = tag;
    newItemDef->typeId = typeId;
    newItemDef->isRequired = isRequired;

    CHK_OFAIL(mxf_append_list_element(&dataModel->itemDefs, newItemDef));

    return newItemDef;

fail:
    free_item_def(&newItemDef);
    return NULL;
}

static MXFItemType* register_type(MXFDataModel *dataModel, const char *name, unsigned int typeId,
                                  MXFItemTypeCategory category)
{
    MXFItemType *type = NULL;
    unsigned int actualTypeId = typeId;

    if (category == MXF_BASIC_TYPE_CAT) {
        /* basic types can only be built-in */
        CHK_ORET(actualTypeId < MXF_EXTENSION_TYPE);
    } else if (actualTypeId <= 0) {
        actualTypeId = get_new_type_id(dataModel);
    }

    /* check the type id is valid and free to use */
    CHK_ORET(actualTypeId > 0 &&
             actualTypeId < ARRAY_SIZE(dataModel->types) &&
             dataModel->types[actualTypeId].typeId == 0);

    type = &dataModel->types[actualTypeId];
    type->typeId = actualTypeId; /* set first to indicate type is present */
    type->category = category;
    if (name)
        CHK_OFAIL((type->name = strdup(name)) != NULL);

    return type;

fail:
    clear_type(type);
    return NULL;
}

static int clone_item_type(MXFDataModel *fromDataModel, unsigned int fromItemTypeId,
                           MXFDataModel *toDataModel, MXFItemType **toItemType)
{
    MXFItemType *clonedItemType = NULL;
    MXFItemType *fromItemType;
    MXFItemType *toRefItemType;
    int i;

    clonedItemType = mxf_get_item_def_type(toDataModel, fromItemTypeId);
    if (clonedItemType) {
        *toItemType = clonedItemType;
        return 1;
    }

    CHK_ORET((fromItemType = mxf_get_item_def_type(fromDataModel, fromItemTypeId)) != NULL);

    switch (fromItemType->category)
    {
        case MXF_BASIC_TYPE_CAT:
            clonedItemType = mxf_register_basic_type(toDataModel, fromItemType->name, fromItemType->typeId,
                                                     fromItemType->info.basic.size);
            CHK_ORET(clonedItemType);
            break;
        case MXF_ARRAY_TYPE_CAT:
            CHK_ORET(clone_item_type(fromDataModel, fromItemType->info.array.elementTypeId, toDataModel, &toRefItemType));
            clonedItemType = mxf_register_array_type(toDataModel, fromItemType->name, fromItemType->typeId,
                                                     fromItemType->info.array.elementTypeId,
                                                     fromItemType->info.array.fixedSize);
            CHK_ORET(clonedItemType);
            break;
        case MXF_COMPOUND_TYPE_CAT:
            clonedItemType = mxf_register_compound_type(toDataModel, fromItemType->name, fromItemType->typeId);
            CHK_ORET(clonedItemType);

            i = 0;
            while (fromItemType->info.compound.members[i].typeId) {
                CHK_ORET(clone_item_type(fromDataModel, fromItemType->info.compound.members[i].typeId,
                                         toDataModel, &toRefItemType));
                CHK_ORET(mxf_register_compound_type_member(clonedItemType, fromItemType->info.compound.members[i].name,
                                                           fromItemType->info.compound.members[i].typeId));
                i++;
            }
            break;
        case MXF_INTERPRET_TYPE_CAT:
            CHK_ORET(clone_item_type(fromDataModel, fromItemType->info.interpret.typeId,
                                     toDataModel, &toRefItemType));
            clonedItemType = mxf_register_interpret_type(toDataModel, fromItemType->name, fromItemType->typeId,
                                                         fromItemType->info.interpret.typeId,
                                                         fromItemType->info.interpret.fixedArraySize);
            CHK_ORET(clonedItemType);
            break;
    }

    *toItemType = clonedItemType;
    return 1;
}

static int clone_item_def(MXFDataModel *fromDataModel, MXFItemDef *fromItemDef,
                          MXFDataModel *toDataModel, MXFItemDef **toItemDef)
{
    MXFItemType *toItemType;
    MXFItemDef *clonedItemDef;

    CHK_ORET(clone_item_type(fromDataModel, fromItemDef->typeId, toDataModel, &toItemType));

    clonedItemDef = register_item_def(toDataModel, fromItemDef->name, &fromItemDef->setDefKey, &fromItemDef->key,
                                      fromItemDef->localTag, fromItemDef->typeId, fromItemDef->isRequired);
    CHK_ORET(clonedItemDef);

    *toItemDef = clonedItemDef;
    return 1;
}

static int finalise_set_def(void *nodeData, void *processData)
{
    MXFSetDef *setDef = (MXFSetDef*)nodeData;
    MXFDataModel *dataModel = (MXFDataModel*)processData;

    mxf_clear_list(&setDef->itemDefs);
    setDef->parentSetDef = NULL;

    if (!mxf_equals_key(&setDef->parentSetDefKey, &g_Null_Key))
        CHK_ORET(mxf_find_set_def(dataModel, &setDef->parentSetDefKey, &setDef->parentSetDef));

    return 1;
}



#define MXF_BASIC_TYPE_DEF(id, name, size) \
    CHK_OFAIL(mxf_register_basic_type(newDataModel, name, id, size));

#define MXF_ARRAY_TYPE_DEF(id, name, elementTypeId, fixedSize) \
    CHK_OFAIL(mxf_register_array_type(newDataModel, name, id, elementTypeId, fixedSize));

#define MXF_COMPOUND_TYPE_DEF(id, name) \
    CHK_OFAIL((itemType = mxf_register_compound_type(newDataModel, name, id)) != NULL);

#define MXF_COMPOUND_TYPE_MEMBER(name, typeId) \
    CHK_OFAIL(mxf_register_compound_type_member(itemType, name, typeId));

#define MXF_INTERPRETED_TYPE_DEF(id, name, typeId, fixedSize) \
    CHK_OFAIL(mxf_register_interpret_type(newDataModel, name, id, typeId, fixedSize));


#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_OFAIL(mxf_register_set_def(newDataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));

#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    CHK_OFAIL(mxf_register_item_def(newDataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, \
                                    typeId, isRequired));


int mxf_load_data_model(MXFDataModel **dataModel)
{
    MXFDataModel *newDataModel;
    MXFItemType *itemType = NULL;

    CHK_MALLOC_ORET(newDataModel, MXFDataModel);
    memset(newDataModel, 0, sizeof(MXFDataModel));
    mxf_initialise_list(&newDataModel->itemDefs, free_item_def_in_list);
    mxf_tree_init(&newDataModel->setDefs, 0, compare_set_def_in_tree, free_set_def_in_tree);

#define KEEP_DATA_MODEL_DEFS 1
#include <mxf/mxf_baseline_data_model.h>

#undef KEEP_DATA_MODEL_DEFS
#include <mxf/mxf_extensions_data_model.h>

    *dataModel = newDataModel;
    return 1;

fail:
    mxf_free_data_model(&newDataModel);
    return 0;
}

void mxf_free_data_model(MXFDataModel **dataModel)
{
    size_t i;

    if (!(*dataModel))
        return;

    mxf_tree_clear(&(*dataModel)->setDefs);
    mxf_clear_list(&(*dataModel)->itemDefs);

    for (i = 0; i < ARRAY_SIZE((*dataModel)->types); i++)
        clear_type(&(*dataModel)->types[i]);

    SAFE_FREE(*dataModel);
}

int mxf_register_set_def(MXFDataModel *dataModel, const char *name, const mxfKey *parentKey, const mxfKey *key)
{
    return register_set_def(dataModel, name, parentKey, key) != NULL;
}

int mxf_register_item_def(MXFDataModel *dataModel, const char *name, const mxfKey *setKey, const mxfKey *key,
                          mxfLocalTag tag, unsigned int typeId, int isRequired)
{
    return register_item_def(dataModel, name, setKey, key, tag, typeId, isRequired) != NULL;
}

MXFItemType* mxf_register_basic_type(MXFDataModel *dataModel, const char *name, unsigned int typeId, unsigned int size)
{
    MXFItemType *type = register_type(dataModel, name, typeId, MXF_BASIC_TYPE_CAT);
    if (!type)
        return NULL;

    type->info.basic.size = size;

    return type;
}

MXFItemType* mxf_register_array_type(MXFDataModel *dataModel, const char *name, unsigned int typeId,
                                     unsigned int elementTypeId, unsigned int fixedSize)
{
    MXFItemType *type = register_type(dataModel, name, typeId, MXF_ARRAY_TYPE_CAT);
    if (!type)
        return NULL;

    type->info.array.elementTypeId = elementTypeId;
    type->info.array.fixedSize = fixedSize;

    return type;
}

MXFItemType* mxf_register_compound_type(MXFDataModel *dataModel, const char *name, unsigned int typeId)
{
    MXFItemType *type = register_type(dataModel, name, typeId, MXF_COMPOUND_TYPE_CAT);
    if (!type)
        return NULL;

    memset(type->info.compound.members, 0, sizeof(type->info.compound.members));

    return type;
}

int mxf_register_compound_type_member(MXFItemType *type, const char *memberName, unsigned int memberTypeId)
{
    size_t memberIndex;
    size_t maxMembers = ARRAY_SIZE(type->info.compound.members) - 1;

    /* find null terminator (eg. typeId == 0) */
    for (memberIndex = 0; memberIndex < maxMembers; memberIndex++) {
        if (type->info.compound.members[memberIndex].typeId == 0)
            break;
    }
    if (memberIndex == maxMembers) {
        mxf_log_error("Number of compound item type members exceeds hardcoded maximum %d"
                      LOG_LOC_FORMAT, maxMembers, LOG_LOC_PARAMS);
        return 0;
    }


    /* set member info */
    CHK_ORET((type->info.compound.members[memberIndex].name = strdup(memberName)) != NULL);
    type->info.compound.members[memberIndex].typeId = memberTypeId; /* set last when everything ok */

    return 1;
}

MXFItemType* mxf_register_interpret_type(MXFDataModel *dataModel, const char *name, unsigned int typeId,
                                         unsigned int interpretedTypeId, unsigned int fixedArraySize)
{
    MXFItemType *type = register_type(dataModel, name, typeId, MXF_INTERPRET_TYPE_CAT);
    if (!type)
        return NULL;

    type->info.interpret.typeId = interpretedTypeId;
    type->info.interpret.fixedArraySize = fixedArraySize;

    return type;
}

int mxf_finalise_data_model(MXFDataModel *dataModel)
{
    MXFListIterator iter;
    MXFItemDef *itemDef;
    MXFSetDef *setDef;

    /* reset set defs and set the parent set def if the parent set def key != g_Null_Key */
    CHK_ORET(mxf_tree_traverse(&dataModel->setDefs, finalise_set_def, dataModel));

    /* add item defs to owner set def */
    mxf_initialise_list_iter(&iter, &dataModel->itemDefs);
    while (mxf_next_list_iter_element(&iter)) {
        itemDef = (MXFItemDef*)mxf_get_iter_element(&iter);
        CHK_ORET(mxf_find_set_def(dataModel, &itemDef->setDefKey, &setDef));
        CHK_ORET(mxf_append_list_element(&setDef->itemDefs, (void*)itemDef));
    }

    return 1;
}

int mxf_check_data_model(MXFDataModel *dataModel)
{
    MXFListIterator iter1;
    MXFListIterator iter2;
    MXFItemDef *itemDef1;
    MXFItemDef *itemDef2;
    size_t listIndex;

    /* check that the item defs are unique (both key and static local tag),
       that the item def is contained in a set def
       and the item type is known */
    listIndex = 0;
    mxf_initialise_list_iter(&iter1, &dataModel->itemDefs);
    while (mxf_next_list_iter_element(&iter1)) {
        itemDef1 = (MXFItemDef*)mxf_get_iter_element(&iter1);

        /* check with item defs with higher index in list */
        mxf_initialise_list_iter_at(&iter2, &dataModel->itemDefs, listIndex + 1);
        while (mxf_next_list_iter_element(&iter2)) {
            itemDef2 = (MXFItemDef*)mxf_get_iter_element(&iter2);
            if (mxf_equals_key(&itemDef1->key, &itemDef2->key)) {
                /* if the items have the same key then check the local tags are identical */
                if (itemDef1->localTag != itemDef2->localTag) {
                    char keyStr[KEY_STR_SIZE];
                    mxf_sprint_key(keyStr, &itemDef1->key);
                    mxf_log_warn("Duplicate item defs have different local tags. Key = %s"
                                 LOG_LOC_FORMAT, keyStr, LOG_LOC_PARAMS);
                    return 0;
                }
            } else {
                /* if the items do not have the same key then check the local tags are different */
                if (itemDef1->localTag != 0 && itemDef1->localTag == itemDef2->localTag) {
                    char keyStr[KEY_STR_SIZE];
                    mxf_sprint_key(keyStr, &itemDef1->key);
                    mxf_log_warn("Duplicate item def local tag found. LocalTag = 0x%04x, Key = %s"
                                 LOG_LOC_FORMAT, itemDef1->localTag, keyStr, LOG_LOC_PARAMS);
                    return 0;
                }
            }
        }

        /* check item type is valid and known */
        if (!mxf_get_item_def_type(dataModel, itemDef1->typeId)) {
            char keyStr[KEY_STR_SIZE];
            mxf_sprint_key(keyStr, &itemDef1->key);
            mxf_log_warn("Item def has unknown type (%d). LocalTag = 0x%04x, Key = %s"
                         LOG_LOC_FORMAT, itemDef1->typeId, itemDef1->localTag, keyStr, LOG_LOC_PARAMS);
            return 0;
        }

        listIndex++;
    }

    return 1;
}

int mxf_find_set_def(MXFDataModel *dataModel, const mxfKey *key, MXFSetDef **setDef)
{
    MXFSetDef *result;
    MXFSetDef setDefKey;

    setDefKey.key = *key;
    result = mxf_tree_find(&dataModel->setDefs, &setDefKey);
    if (!result)
        return 0;

    *setDef = (MXFSetDef*)result;
    return 1;
}

int mxf_find_item_def(MXFDataModel *dataModel, const mxfKey *key, MXFItemDef **itemDef)
{
    void *result = mxf_find_list_element(&dataModel->itemDefs, (void*)key, item_def_eq);
    if (!result)
        return 0;

    *itemDef = (MXFItemDef*)result;
    return 1;
}

int mxf_find_item_def_in_set_def(const mxfKey *key, const MXFSetDef *setDef, MXFItemDef **itemDef)
{
    void *result = mxf_find_list_element(&setDef->itemDefs, (void*)key, item_def_eq);
    if (!result) {
        if (setDef->parentSetDef)
            return mxf_find_item_def_in_set_def(key, setDef->parentSetDef, itemDef);
        return 0;
    }

    *itemDef = (MXFItemDef*)result;
    return 1;
}

MXFItemType* mxf_get_item_def_type(MXFDataModel *dataModel, unsigned int typeId)
{
    if (typeId == 0 || typeId >= ARRAY_SIZE(dataModel->types) || dataModel->types[typeId].typeId == MXF_UNKNOWN_TYPE)
        return NULL;

    return &dataModel->types[typeId];
}

int mxf_is_subclass_of(MXFDataModel *dataModel, const mxfKey *setKey, const mxfKey *parentSetKey)
{
    MXFSetDef *setDef;

    if (mxf_equals_key(setKey, parentSetKey))
        return 1;

    if (!mxf_find_set_def(dataModel, setKey, &setDef))
        return 0;

    return mxf_is_subclass_of_2(dataModel, setDef, parentSetKey);
}

int mxf_is_subclass_of_2(MXFDataModel *dataModel, MXFSetDef *setDef, const mxfKey *parentSetKey)
{
    if (mxf_equals_key(&setDef->key, parentSetKey))
        return 1;

    if (!setDef->parentSetDef || mxf_equals_key(&setDef->key, &setDef->parentSetDefKey))
        return 0;

    return mxf_is_subclass_of_2(dataModel, setDef->parentSetDef, parentSetKey);
}

int mxf_clone_set_def(MXFDataModel *fromDataModel, MXFSetDef *fromSetDef,
                      MXFDataModel *toDataModel, MXFSetDef **toSetDef)
{
    MXFSetDef *clonedSetDef = NULL;
    MXFSetDef *toParentSetDef = NULL;
    MXFListIterator iter;
    MXFItemDef *fromItemDef;
    MXFItemDef *toItemDef = NULL;

    if (!mxf_find_set_def(toDataModel, &fromSetDef->key, &clonedSetDef)) {
        if (fromSetDef->parentSetDef)
            CHK_ORET(mxf_clone_set_def(fromDataModel, fromSetDef->parentSetDef, toDataModel, &toParentSetDef));

        clonedSetDef = register_set_def(toDataModel, fromSetDef->name, &fromSetDef->parentSetDefKey, &fromSetDef->key);
        CHK_ORET(clonedSetDef);
        clonedSetDef->parentSetDef = toParentSetDef;
    }

    mxf_initialise_list_iter(&iter, &fromSetDef->itemDefs);
    while (mxf_next_list_iter_element(&iter)) {
        fromItemDef = (MXFItemDef*)mxf_get_iter_element(&iter);

        if (mxf_find_item_def_in_set_def(&fromItemDef->key, clonedSetDef, &toItemDef))
            continue;

        CHK_ORET(clone_item_def(fromDataModel, fromItemDef, toDataModel, &toItemDef));
        CHK_ORET(mxf_append_list_element(&clonedSetDef->itemDefs, (void*)toItemDef));
    }

    *toSetDef = clonedSetDef;
    return 1;
}
