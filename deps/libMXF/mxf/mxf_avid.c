/*
 * Avid data model extensions and utilities
 *
 * Copyright (C) 2006, British Broadcasting Corporation
 * All rights reserved.
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
#include <limits.h>


#if defined(_WIN32)

#include <time.h>
#include <windows.h>

#else

#include <uuid/uuid.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>

#endif


#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>


typedef struct MXFAvidObjectReference
{
    struct MXFAvidObjectReference *next;

    mxfUUID instanceUID;
    uint64_t offset;
    uint8_t flags;
} MXFAvidObjectReference;

typedef struct
{
    MXFAvidObjectReference *references;
} MXFAvidObjectDirectory;


typedef struct
{
    mxfUUID id;
    int64_t directoryOffset;
    uint32_t formatVersion;
    mxfUUID metaDictionaryInstanceUID;
    mxfUUID prefaceInstanceUID;
} MXFAvidMetadataRoot;

typedef struct
{
    MXFReadFilter metaDictFilter;
    MXFReadFilter dictFilter;

    MXFReadFilter filter;
} MXFAvidReadFilter;



mxf_generate_aafsdk_umid_func mxf_generate_aafsdk_umid = mxf_default_generate_aafsdk_umid;
mxf_generate_old_aafsdk_umid_func mxf_generate_old_aafsdk_umid = mxf_default_generate_old_aafsdk_umid;



static int avid_before_set_read(void *privateData, MXFHeaderMetadata *headerMetadata,
                                const mxfKey *key, uint8_t llen, uint64_t len, int *skip)
{
    MXFAvidReadFilter *filter = (MXFAvidReadFilter*)privateData;
    int newSkip = 0;

    if (filter->metaDictFilter.before_set_read != NULL)
    {
        CHK_ORET(filter->metaDictFilter.before_set_read(filter->metaDictFilter.privateData,
                                                        headerMetadata, key, llen, len, &newSkip));
    }
    if (!newSkip)
    {
        if (filter->dictFilter.before_set_read != NULL)
        {
            CHK_ORET(filter->dictFilter.before_set_read(filter->dictFilter.privateData,
                                                        headerMetadata, key, llen, len, &newSkip));
        }
    }

    *skip = newSkip;
    return 1;
}

static int avid_after_set_read(void *privateData, MXFHeaderMetadata *headerMetadata, MXFMetadataSet *set, int *skip)
{
    MXFAvidReadFilter *filter = (MXFAvidReadFilter*)privateData;
    int newSkip = 0;

    if (filter->metaDictFilter.after_set_read != NULL)
    {
        CHK_ORET(filter->metaDictFilter.after_set_read(filter->metaDictFilter.privateData, headerMetadata,
                                                       set, &newSkip));
    }
    if (!newSkip)
    {
        if (filter->dictFilter.after_set_read != NULL)
        {
            CHK_ORET(filter->dictFilter.after_set_read(filter->dictFilter.privateData, headerMetadata, set, &newSkip));
        }
    }

    *skip = newSkip;
    return 1;
}

static int initialise_read_filter(MXFAvidReadFilter *readFilter, int skipDataDefs)
{
    CHK_ORET(mxf_initialise_metadict_read_filter(&readFilter->metaDictFilter));
    CHK_ORET(mxf_initialise_dict_read_filter(&readFilter->dictFilter, skipDataDefs));

    readFilter->filter.privateData = readFilter;
    readFilter->filter.before_set_read = avid_before_set_read;
    readFilter->filter.after_set_read = avid_after_set_read;

    return 1;
}

static void clear_read_filter(MXFAvidReadFilter *readFilter)
{
    if (readFilter == NULL)
    {
        return;
    }

    mxf_clear_metadict_read_filter(&readFilter->metaDictFilter);
    mxf_clear_dict_read_filter(&readFilter->dictFilter);
}


static int create_object_directory(MXFAvidObjectDirectory **directory)
{
    MXFAvidObjectDirectory *newDirectory;

    CHK_MALLOC_ORET(newDirectory, MXFAvidObjectDirectory);
    memset(newDirectory, 0, sizeof(MXFAvidObjectDirectory));

    *directory = newDirectory;
    return 1;
}

static void free_object_directory(MXFAvidObjectDirectory **directory)
{
    if (*directory == NULL)
    {
        return;
    }

    if ((*directory)->references != NULL)
    {
        MXFAvidObjectReference *entry = (*directory)->references->next;
        while (entry != NULL)
        {
            MXFAvidObjectReference *tmpNextEntry = entry->next;
            SAFE_FREE(entry);
            entry = tmpNextEntry;
        }
        SAFE_FREE((*directory)->references);
    }
    SAFE_FREE(*directory);
}

static int add_object_directory_entry(MXFAvidObjectDirectory *directory,
                                      const mxfUUID *instanceUID, uint64_t offset, uint8_t flags)
{
    MXFAvidObjectReference *newEntry = NULL;

    CHK_MALLOC_ORET(newEntry, MXFAvidObjectReference);
    memset(newEntry, 0, sizeof(MXFAvidObjectReference));
    newEntry->instanceUID = *instanceUID;
    newEntry->offset = offset;
    newEntry->flags = flags;

    if (directory->references == NULL)
    {
        directory->references = newEntry;
    }
    else
    {
        MXFAvidObjectReference *lastEntry = directory->references;
        while (lastEntry->next != NULL)
        {
            lastEntry = lastEntry->next;
        }
        lastEntry->next = newEntry;
    }

    return 1;
}

static int write_object_directory(MXFFile *mxfFile, const MXFAvidObjectDirectory *directory)
{
    const MXFAvidObjectReference *entry = NULL;
    uint64_t numEntries = 0;

    entry = directory->references;
    while (entry != NULL)
    {
        numEntries++;
        entry = entry->next;
    }

    CHK_ORET(mxf_write_kl(mxfFile, &g_AvidObjectDirectory_key, 9 + 25 * numEntries));

    CHK_ORET(mxf_write_uint64(mxfFile, numEntries));
    CHK_ORET(mxf_write_uint8(mxfFile, 25));

    entry = directory->references;
    while (entry != NULL)
    {
        CHK_ORET(mxf_write_uuid(mxfFile, &entry->instanceUID));
        CHK_ORET(mxf_write_uint64(mxfFile, entry->offset));
        CHK_ORET(mxf_write_uint8(mxfFile, entry->flags));
        entry = entry->next;
    }

    return 1;
}

static void initialise_root_set(MXFAvidMetadataRoot *root)
{
    memset(root, 0, sizeof(*root));

    mxf_generate_uuid(&root->id);
    root->formatVersion = 0x0008;
}

static int write_root_set(MXFFile *mxfFile, const MXFAvidMetadataRoot *root)
{
    CHK_ORET(mxf_write_kl(mxfFile, &g_AvidMetadataRoot_key, 96));

    CHK_ORET(mxf_write_local_tl(mxfFile, 0x3c0a, mxfUUID_extlen));
    CHK_ORET(mxf_write_uuid(mxfFile, &root->id));

    CHK_ORET(mxf_write_local_tl(mxfFile, 0x0003, mxfUUID_extlen + 8));
    CHK_ORET(mxf_write_uuid(mxfFile, &g_Null_UUID));
    CHK_ORET(mxf_write_int64(mxfFile, root->directoryOffset));

    CHK_ORET(mxf_write_local_tl(mxfFile, 0x0004, 4));
    CHK_ORET(mxf_write_uint32(mxfFile, root->formatVersion));

    CHK_ORET(mxf_write_local_tl(mxfFile, 0x0001, mxfUUID_extlen));
    CHK_ORET(mxf_write_uuid(mxfFile, &root->metaDictionaryInstanceUID));

    CHK_ORET(mxf_write_local_tl(mxfFile, 0x0002, mxfUUID_extlen));
    CHK_ORET(mxf_write_uuid(mxfFile, &root->prefaceInstanceUID));

    return 1;
}

static int write_set(MXFFile *mxfFile, MXFMetadataSet *set, int64_t *offset, MXFAvidObjectDirectory *objectDirectory)
{
    CHK_ORET(add_object_directory_entry(objectDirectory, &set->instanceUID, *offset, 0x00));
    CHK_ORET(mxf_write_set(mxfFile, set));
    *offset += mxf_get_set_size(mxfFile, set);

    return 1;
}

static int write_metadict_sets(MXFFile *mxfFile, MXFHeaderMetadata *headerMetadata,
                               MXFAvidObjectDirectory *objectDirectory)
{
    MXFListIterator iter;
    MXFMetadataSet *metaDictSet;
    int64_t offset;

    CHK_ORET((offset = mxf_file_tell(mxfFile)) >= 0);

    /* must write the MetaDictionary set first */
    CHK_ORET(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(MetaDictionary), &metaDictSet));
    CHK_ORET(write_set(mxfFile, metaDictSet, &offset, objectDirectory));

    mxf_initialise_list_iter(&iter, &headerMetadata->sets);
    while (mxf_next_list_iter_element(&iter))
    {
        MXFMetadataSet *set = (MXFMetadataSet*)mxf_get_iter_element(&iter);

        if (mxf_is_subclass_of(headerMetadata->dataModel, &set->key, &MXF_SET_K(MetaDefinition)))
        {
            CHK_ORET(write_set(mxfFile, set, &offset, objectDirectory));
        }
    }

    return 1;
}

static int write_preface_sets(MXFFile *mxfFile, MXFHeaderMetadata *headerMetadata,
                              MXFAvidObjectDirectory *objectDirectory)
{
    MXFListIterator iter;
    MXFMetadataSet *prefaceSet;
    int64_t offset;

    CHK_ORET((offset = mxf_file_tell(mxfFile)) >= 0);

    /* must write the Preface set first */
    CHK_ORET(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    CHK_ORET(write_set(mxfFile, prefaceSet, &offset, objectDirectory));

    mxf_initialise_list_iter(&iter, &headerMetadata->sets);
    while (mxf_next_list_iter_element(&iter))
    {
        MXFMetadataSet *set = (MXFMetadataSet*)mxf_get_iter_element(&iter);

        if (!mxf_is_subclass_of(headerMetadata->dataModel, &set->key, &MXF_SET_K(MetaDictionary)) &&
            !mxf_is_subclass_of(headerMetadata->dataModel, &set->key, &MXF_SET_K(MetaDefinition)) &&
            !mxf_equals_key(&set->key, &MXF_SET_K(Preface)))
        {
            CHK_ORET(write_set(mxfFile, set, &offset, objectDirectory));
        }
    }

    return 1;
}

static int get_indirect_string(MXFMetadataSet *set, const mxfKey *itemKey, mxfUTF16Char **value)
{
    /* prefix is 0x42 ('B') for big endian or 0x4c ('L') for little endian, followed by half-swapped key for String type */
    const uint8_t prefix_BE[17] =
    {
        0x42,
        0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01
    };
    const uint8_t prefix_LE[17] =
    {
        0x4c,
        0x00, 0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01
    };
    int isBigEndian;
    MXFMetadataItem *item;
    mxfUTF16Char *newValue = NULL;
    uint16_t i;
    uint8_t *itemValuePtr;
    uint16_t strSize;

    CHK_OFAIL(mxf_get_item(set, itemKey, &item));

    /* initial check */
    if (item->length <= sizeof(prefix_BE) ||
        (item->value[0] != 0x42 && item->value[0] != 0x4c))
    {
        return 0;
    }
    isBigEndian = item->value[0] == 0x42;

    /* check string type label */
    if ((isBigEndian  && memcmp(&item->value[1], &prefix_BE[1], sizeof(prefix_BE) - 1) != 0) ||
        (!isBigEndian && memcmp(&item->value[1], &prefix_LE[1], sizeof(prefix_LE) - 1) != 0))
    {
        return 0;
    }
    else if (!value)
    {
        return 1;
    }

    strSize = (item->length - sizeof(prefix_BE)) / 2 ;

    CHK_MALLOC_ARRAY_OFAIL(newValue, mxfUTF16Char, strSize + 1 /* always have a null terminator */);
    memset(newValue, 0, sizeof(mxfUTF16Char) * (strSize + 1));

    itemValuePtr = &item->value[sizeof(prefix_BE)];
    for (i = 0; i < strSize; i++)
    {
        if (isBigEndian)
        {
            newValue[i] = ((*itemValuePtr) << 8) | (*(itemValuePtr + 1));
        }
        else
        {
            newValue[i] = (*itemValuePtr) | ((*(itemValuePtr + 1)) << 8);
        }
        if (newValue[i] == 0)
        {
            break;
        }
        itemValuePtr += 2;
    }

    *value = newValue;
    return 1;

fail:
    SAFE_FREE(newValue);
    return 0;
}


static int get_indirect_int32(MXFMetadataSet *set, const mxfKey *itemKey, int32_t *value)
{
    /* prefix is 0x42 ('B') for big endian or 0x4c ('L') for little endian, followed by half-swapped key for Int32 type */
    const uint8_t prefix_BE[17] =
    {
        0x42,
        0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01
    };
    const uint8_t prefix_LE[17] =
    {
        0x4c,
        0x00, 0x07, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01
    };

    int isBigEndian;
    MXFMetadataItem *item;

    CHK_ORET(mxf_get_item(set, itemKey, &item));

    /* initial check */
    if (item->length <= sizeof(prefix_BE) ||
        (item->value[0] != 0x42 && item->value[0] != 0x4c))
    {
        return 0;
    }
    isBigEndian = item->value[0] == 0x42;

    /* check int32 type label */
    if ((isBigEndian  && memcmp(&item->value[1], &prefix_BE[1], sizeof(prefix_BE) - 1) != 0) ||
        (!isBigEndian && memcmp(&item->value[1], &prefix_LE[1], sizeof(prefix_LE) - 1) != 0))
    {
        return 0;
    }
    else if (!value)
    {
        return 1;
    }

    if (isBigEndian)
    {
        *value = (int32_t)(((uint32_t)item->value[17] << 24) |
                           ((uint32_t)item->value[18] << 16) |
                           ((uint32_t)item->value[19] << 8) |
                            (uint32_t)item->value[20]);
    }
    else
    {
        *value = (int32_t)( (uint32_t)item->value[17] |
                           ((uint32_t)item->value[18] << 8) |
                           ((uint32_t)item->value[19] << 16) |
                           ((uint32_t)item->value[20] << 24));
    }

    return 1;
}

static int get_int32_string(int32_t value, mxfUTF16Char **strValue)
{
    mxfUTF16Char *newStrValue = NULL;
    char cstrValue[16];
    size_t len;
    size_t i;

    mxf_snprintf(cstrValue, sizeof(cstrValue), "%d", value);
    len = strlen(cstrValue);

    CHK_MALLOC_ARRAY_ORET(newStrValue, mxfUTF16Char, len + 1);
    for (i = 0; i < len; i++)
        newStrValue[i] = (mxfUTF16Char)cstrValue[i];
    newStrValue[len] = 0;

    *strValue = newStrValue;

    return 1;
}


#define MXF_COMPOUND_TYPE_DEF(id, name) \
    CHK_ORET((itemType = mxf_register_compound_type(dataModel, name, id)) != NULL);

#define MXF_COMPOUND_TYPE_MEMBER(name, typeId) \
    CHK_ORET(mxf_register_compound_type_member(itemType, name, typeId));

#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_ORET(mxf_register_set_def(dataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));

#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    CHK_ORET(mxf_register_item_def(dataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId, isRequired));

int mxf_avid_load_extensions(MXFDataModel *dataModel)
{
    MXFItemType *itemType = NULL;

#include <mxf/mxf_avid_extensions_data_model.h>

    return 1;
}

int mxf_avid_read_filtered_header_metadata(MXFFile *mxfFile, int skipDataDefs, MXFHeaderMetadata *headerMetadata,
                                           uint64_t headerByteCount, const mxfKey *key, uint8_t llen, uint64_t len)
{
    MXFAvidReadFilter readFilter;
    memset(&readFilter, 0, sizeof(readFilter));

    CHK_OFAIL(initialise_read_filter(&readFilter, skipDataDefs));

    CHK_OFAIL(mxf_read_filtered_header_metadata(mxfFile, &readFilter.filter, headerMetadata, headerByteCount,
                                                key, llen, len));

    clear_read_filter(&readFilter);
    return 1;

fail:
    clear_read_filter(&readFilter);
    return 0;
}


int mxf_avid_write_header_metadata(MXFFile *mxfFile, MXFHeaderMetadata *headerMetadata, MXFPartition *headerPartition)
{
    int64_t rootPos;
    int64_t endPos;
    MXFAvidObjectDirectory *objectDirectory = NULL;
    MXFMetadataSet *prefaceSet;
    MXFMetadataSet *metaDictSet;
    MXFAvidMetadataRoot root;


    CHK_OFAIL(create_object_directory(&objectDirectory));

    initialise_root_set(&root);
    CHK_OFAIL(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(MetaDictionary), &metaDictSet));
    root.metaDictionaryInstanceUID = metaDictSet->instanceUID;
    CHK_OFAIL(mxf_find_singular_set_by_key(headerMetadata, &MXF_SET_K(Preface), &prefaceSet));
    root.prefaceInstanceUID = prefaceSet->instanceUID;


    /* write the primer pack, root set, meta-dictionary sets, preface sets and object directory */

    CHK_OFAIL(mxf_write_header_primer_pack(mxfFile, headerMetadata));
    CHK_OFAIL(mxf_fill_to_kag(mxfFile, headerPartition));

    CHK_OFAIL((rootPos = mxf_file_tell(mxfFile)) >= 0);
    CHK_OFAIL(add_object_directory_entry(objectDirectory, &root.id, rootPos, 0x00));
    CHK_OFAIL(write_root_set(mxfFile, &root));

    CHK_OFAIL(write_metadict_sets(mxfFile, headerMetadata, objectDirectory));

    CHK_OFAIL(write_preface_sets(mxfFile, headerMetadata, objectDirectory));

    CHK_OFAIL((root.directoryOffset = mxf_file_tell(mxfFile)) >= 0);
    CHK_OFAIL(write_object_directory(mxfFile, objectDirectory));
    CHK_OFAIL((endPos = mxf_file_tell(mxfFile)) >= 0);


    /* go back and re-write the root set with an updated object directory offset */
    CHK_OFAIL(mxf_file_seek(mxfFile, rootPos, SEEK_SET));
    CHK_OFAIL(write_root_set(mxfFile, &root));


    /* position file after object directory */
    CHK_OFAIL(mxf_file_seek(mxfFile, endPos, SEEK_SET));


    free_object_directory(&objectDirectory);
    return 1;

fail:
    free_object_directory(&objectDirectory);
    return 0;
}



/* MobID generation code following the same algorithm as implemented in the AAF SDK */
/* NOTE: this function is not guaranteed to create a unique UMID in multi-threaded environment */
void mxf_default_generate_aafsdk_umid(mxfUMID *umid)
{
    uint32_t major, minor;
    static uint32_t last_part2 = 0;

    major = (uint32_t)time(NULL);
#if defined(_WIN32)
    minor = (uint32_t)GetTickCount();
#else
    struct tms tms_buf;
    minor = (uint32_t)times(&tms_buf);
    assert(minor != 0 && minor != (uint32_t)-1);
#endif

    if (last_part2 >= minor)
    {
        minor = last_part2 + 1;
    }

    last_part2 = minor;

    umid->octet0  = 0x06;
    umid->octet1  = 0x0a;
    umid->octet2  = 0x2b;
    umid->octet3  = 0x34;
    umid->octet4  = 0x01;
    umid->octet5  = 0x01;
    umid->octet6  = 0x01;
    umid->octet7  = 0x01;
    umid->octet8  = 0x01;
    umid->octet9  = 0x01;
    umid->octet10 = 0x0f; /* material type not identified */
    umid->octet11 = 0x00; /* no method specified for material and instance number generation */
    umid->octet12 = 0x13;
    umid->octet13 = 0x00;
    umid->octet14 = 0x00;
    umid->octet15 = 0x00;

    umid->octet24 = 0x06;
    umid->octet25 = 0x0e;
    umid->octet26 = 0x2b;
    umid->octet27 = 0x34;
    umid->octet28 = 0x7f;
    umid->octet29 = 0x7f;
    umid->octet30 = (uint8_t)(42 & 0x7f); /* company specific prefix = 42 */
    umid->octet31 = (uint8_t)((42 >> 7L) | 0x80);  /* company specific prefix = 42 */

    umid->octet16 = (uint8_t)((major >> 24) & 0xff);
    umid->octet17 = (uint8_t)((major >> 16) & 0xff);
    umid->octet18 = (uint8_t)((major >> 8)  & 0xff);
    umid->octet19 = (uint8_t) (major        & 0xff);

    umid->octet20 = (uint8_t)(((uint16_t)(minor & 0xffff) >> 8) & 0xff);
    umid->octet21 = (uint8_t)((uint16_t) (minor & 0xffff)       & 0xff);

    umid->octet22 = (uint8_t)(((uint16_t)((minor >> 16L) & 0xffff) >> 8) & 0xff);
    umid->octet23 = (uint8_t)((uint16_t) ((minor >> 16L) & 0xffff)       & 0xff);

}

/* MobID generation code following the same algorithm as implemented in the older AAF SDK versions
  - see revision 1.47 of AAF/ref-impl/src/impl/AAFUtils.c */
void mxf_default_generate_old_aafsdk_umid(mxfUMID *umid)
{
    uint32_t major, minor;
    static uint32_t last_part2 = 0;

    major = (uint32_t)time(NULL);
#if defined(_WIN32)
    minor = (uint32_t)GetTickCount();
#else
    struct tms tms_buf;
    minor = (uint32_t)times(&tms_buf);
    assert(minor != 0 && minor != (uint32_t)-1);
#endif

    if (last_part2 >= minor)
    {
        minor = last_part2 + 1;
    }

    last_part2 = minor;

    umid->octet0  = 0x06;
    umid->octet1  = 0x0c;
    umid->octet2  = 0x2b;
    umid->octet3  = 0x34;
    umid->octet4  = 0x02;
    umid->octet5  = 0x05;
    umid->octet6  = 0x11;
    umid->octet7  = 0x01;
    umid->octet8  = 0x01;
    umid->octet9  = 0x04; /* UMIDType */
    umid->octet10 = 0x10;
    umid->octet11 = 0x00; /* no method specified for material and instance number generation */
    umid->octet12 = 0x13;
    umid->octet13 = 0x00;
    umid->octet14 = 0x00;
    umid->octet15 = 0x00;

    umid->octet24 = 0x06;
    umid->octet25 = 0x0e;
    umid->octet26 = 0x2b;
    umid->octet27 = 0x34;
    umid->octet28 = 0x7f;
    umid->octet29 = 0x7f;
    umid->octet30 = (uint8_t)(42 & 0x7f); /* company specific prefix = 42 */
    umid->octet31 = (uint8_t)((42 >> 7L) | 0x80);  /* company specific prefix = 42 */

    umid->octet16 = (uint8_t)((major >> 24) & 0xff);
    umid->octet17 = (uint8_t)((major >> 16) & 0xff);
    umid->octet18 = (uint8_t)((major >> 8)  & 0xff);
    umid->octet19 = (uint8_t) (major        & 0xff);

    umid->octet20 = (uint8_t)(((uint16_t)(minor & 0xFFFF) >> 8) & 0xff);
    umid->octet21 = (uint8_t)((uint16_t) (minor & 0xFFFF)       & 0xff);

    umid->octet22 = (uint8_t)(((uint16_t)((minor >> 16L) & 0xFFFF) >> 8) & 0xff);
    umid->octet23 = (uint8_t)((uint16_t) ((minor >> 16L) & 0xFFFF)       & 0xff);

}

void mxf_avid_set_regtest_funcs(void)
{
    mxf_generate_aafsdk_umid = mxf_regtest_generate_aafsdk_umid;
    mxf_generate_old_aafsdk_umid = mxf_regtest_generate_old_aafsdk_umid;
}

void mxf_regtest_generate_aafsdk_umid(mxfUMID *umid)
{
    static uint32_t count = 1;

    memset(umid, 0, sizeof(*umid));
    umid->octet28 = (uint8_t)((count >> 24) & 0xff);
    umid->octet29 = (uint8_t)((count >> 16) & 0xff);
    umid->octet30 = (uint8_t)((count >> 8)  & 0xff);
    umid->octet31 = (uint8_t)( count        & 0xff);

    count++;
}

void mxf_regtest_generate_old_aafsdk_umid(mxfUMID *umid)
{
    static uint32_t count = 1;

    memset(umid, 0, sizeof(*umid));
    umid->octet28 = (uint8_t)((count >> 24) & 0xff);
    umid->octet29 = (uint8_t)((count >> 16) & 0xff);
    umid->octet30 = (uint8_t)((count >> 8)  & 0xff);
    umid->octet31 = (uint8_t)( count        & 0xff);

    count++;
}


int mxf_avid_set_indirect_string_item(MXFMetadataSet *set, const mxfKey *itemKey, const mxfUTF16Char *value)
{
    uint8_t *buffer = NULL;
    uint16_t size;
    /* prefix is 0x42 ('B') for big endian, followed by half-swapped key for String type */
    const uint8_t prefix[17] =
    {
        0x42,
        0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01
    };

    size = mxf_get_external_utf16string_size(value);
    if (size > UINT16_MAX - 17)
    {
        size = UINT16_MAX;
    }
    else
    {
        size += 17;
    }

    CHK_MALLOC_ARRAY_ORET(buffer, uint8_t, size);
    memset(buffer, 0, size);

    memcpy(buffer, prefix, 17);
    mxf_set_utf16string(value, &buffer[17], size - 17);

    CHK_OFAIL(mxf_set_item(set, itemKey, buffer, size));

    SAFE_FREE(buffer);
    return 1;

fail:
    SAFE_FREE(buffer);
    return 0;
}

int mxf_avid_set_indirect_int32_item(MXFMetadataSet *set, const mxfKey *itemKey, int32_t value)
{
    /* prefix is 0x42 ('B') for big endian, followed by half-swapped key for Int32 type */
    uint8_t itemValue[21] =
    {
        0x42,
        0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01,
        0x00, 0x00, 0x00, 0x00
    };

    mxf_set_int32(value, &itemValue[17]);

    CHK_ORET(mxf_set_item(set, itemKey, itemValue, sizeof(itemValue)));

    return 1;
}


int mxf_avid_set_rgb_color_item(MXFMetadataSet *set, const mxfKey *itemKey, const RGBColor *value)
{
    uint8_t *buffer = NULL;

    CHK_MALLOC_ARRAY_ORET(buffer, uint8_t, 3 * 2);

    mxf_set_uint16(value->red, buffer);
    mxf_set_uint16(value->green, &buffer[2]);
    mxf_set_uint16(value->blue, &buffer[4]);

    CHK_OFAIL(mxf_set_item(set, itemKey, buffer, 3 * 2));

    SAFE_FREE(buffer);
    return 1;

fail:
    SAFE_FREE(buffer);
    return 0;
}

int mxf_avid_get_rgb_color_item(MXFMetadataSet *set, const mxfKey *itemKey, RGBColor *value)
{
    MXFMetadataItem *item;

    CHK_ORET(mxf_get_item(set, itemKey, &item));
    CHK_ORET(item->length == 3 * 2);

    mxf_get_uint16(item->value, &value->red);
    mxf_get_uint16(&item->value[2], &value->green);
    mxf_get_uint16(&item->value[4], &value->blue);

    return 1;
}


/* in some Avid files, the StructuralComponent::DataDefinition is not a UL but is a
   weak reference to a DataDefinition object in the Dictionary
   So we try dereferencing it, expecting to find a DataDefinition object with Identification item,
   NOTE: skipDataDefs in mxf_avid_read_filtered_header_metadata must be set to 0 for this to work */
int mxf_avid_get_data_def(MXFHeaderMetadata *headerMetadata, mxfUUID *uuid, mxfUL *dataDef)
{
    MXFMetadataSet *dataDefSet;

    if (mxf_dereference(headerMetadata, uuid, &dataDefSet))
    {
        if (mxf_get_ul_item(dataDefSet, &MXF_ITEM_K(DefinitionObject, Identification), dataDef))
        {
            return 1;
        }
    }
    return 0;
}


int mxf_avid_write_index_entry_array_header(MXFFile *mxfFile, uint8_t sliceCount, uint8_t posTableCount,
                                            uint32_t numIndexEntries)
{
    CHK_ORET(mxf_write_local_tag(mxfFile, 0x3f0a));
    if (8 + numIndexEntries * (11 + sliceCount * 4 + posTableCount * 8) > UINT16_MAX)
    {
        /* Avid ignores the local set 16-bit size restriction and relies on the array header to
           provide the size */
        CHK_ORET(mxf_write_uint16(mxfFile, UINT16_MAX));
    }
    else
    {
        CHK_ORET(mxf_write_uint16(mxfFile, (uint16_t)(8 + numIndexEntries * (11 + sliceCount * 4 + posTableCount * 8))));
    }
    CHK_ORET(mxf_write_uint32(mxfFile, numIndexEntries));
    CHK_ORET(mxf_write_uint32(mxfFile, 11 + sliceCount * 4 + posTableCount * 8));

    return 1;
}

int mxf_avid_is_string_tagged_value(MXFMetadataSet *taggedValueSet)
{
    return get_indirect_string(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), NULL);
}

int mxf_avid_is_int32_tagged_value(MXFMetadataSet *taggedValueSet)
{
    return get_indirect_int32(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), NULL);
}

int mxf_avid_get_string_tagged_value(MXFMetadataSet *taggedValueSet, mxfUTF16Char **value)
{
    return get_indirect_string(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), value);
}

int mxf_avid_get_int32_tagged_value(MXFMetadataSet *taggedValueSet, int32_t *value)
{
    return get_indirect_int32(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), value);
}

int mxf_avid_attach_mob_attribute(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                  const mxfUTF16Char *name, const mxfUTF16Char *value)
{
    MXFMetadataSet *taggedValueSet = NULL;
    CHK_ORET(name != NULL && value != NULL);

    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(TaggedValue), &taggedValueSet));
    CHK_OFAIL(mxf_add_array_item_strongref(packageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList), taggedValueSet));
    CHK_OFAIL(mxf_set_utf16string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), name));
    CHK_OFAIL(mxf_avid_set_indirect_string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), value));

    return 1;

fail:
    mxf_free_set(&taggedValueSet);
    return 0;
}

int mxf_avid_attach_int32_mob_attribute(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                        const mxfUTF16Char *name, int32_t value)
{
    MXFMetadataSet *taggedValueSet = NULL;
    CHK_ORET(name != NULL);

    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(TaggedValue), &taggedValueSet));
    CHK_OFAIL(mxf_add_array_item_strongref(packageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList), taggedValueSet));
    CHK_OFAIL(mxf_set_utf16string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), name));
    CHK_OFAIL(mxf_avid_set_indirect_int32_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), value));

    return 1;

fail:
    mxf_free_set(&taggedValueSet);
    return 0;
}

int mxf_avid_attach_user_comment(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *packageSet,
                                 const mxfUTF16Char *name, const mxfUTF16Char *value)
{
    MXFMetadataSet *taggedValueSet;
    CHK_ORET(name != NULL && value != NULL);

    CHK_ORET(mxf_create_set(headerMetadata, &MXF_SET_K(TaggedValue), &taggedValueSet));
    CHK_ORET(mxf_add_array_item_strongref(packageSet, &MXF_ITEM_K(GenericPackage, UserComments), taggedValueSet));
    CHK_ORET(mxf_set_utf16string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), name));
    CHK_ORET(mxf_avid_set_indirect_string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), value));

    return 1;
}

int mxf_avid_read_string_mob_attributes(MXFMetadataSet *packageSet, MXFList **names, MXFList **values)
{
    return mxf_avid_read_string_tagged_values(packageSet, &MXF_ITEM_K(GenericPackage, MobAttributeList), names, values);
}

int mxf_avid_read_string_user_comments(MXFMetadataSet *packageSet, MXFList **names, MXFList **values)
{
    return mxf_avid_read_string_tagged_values(packageSet, &MXF_ITEM_K(GenericPackage, UserComments), names, values);
}

int mxf_avid_read_string_tagged_values(MXFMetadataSet *set, const mxfKey *itemKey, MXFList **names, MXFList **values)
{
    MXFMetadataSet *taggedValueSet;
    uint32_t count;
    uint32_t i;
    uint8_t *element;
    mxfUTF16Char *taggedValueName = NULL;
    mxfUTF16Char *taggedValueValue = NULL;
    MXFList *newNames = NULL;
    MXFList *newValues = NULL;

    if (!mxf_have_item(set, itemKey))
    {
        return 0;
    }

    CHK_OFAIL(mxf_create_list(&newNames, free));
    CHK_OFAIL(mxf_create_list(&newValues, free));

    CHK_OFAIL(mxf_get_array_item_count(set, itemKey, &count));
    for (i = 0; i < count; i++)
    {
        CHK_OFAIL(mxf_get_array_item_element(set, itemKey, i, &element));
        CHK_OFAIL(mxf_get_strongref(set->headerMetadata, element, &taggedValueSet));

        if (mxf_avid_read_string_tagged_value(taggedValueSet, &taggedValueName, &taggedValueValue))
        {
            CHK_OFAIL(mxf_append_list_element(newNames, taggedValueName));
            taggedValueName = NULL;
            CHK_OFAIL(mxf_append_list_element(newValues, taggedValueValue));
            taggedValueValue = NULL;
        }
    }

    *names = newNames;
    *values = newValues;
    return 1;

fail:
    mxf_free_list(&newNames);
    mxf_free_list(&newValues);
    SAFE_FREE(taggedValueName);
    SAFE_FREE(taggedValueValue);
    return 0;
}

int mxf_avid_read_string_tagged_value(MXFMetadataSet *taggedValueSet, mxfUTF16Char **name, mxfUTF16Char **value)
{
    mxfUTF16Char *newName = NULL;
    mxfUTF16Char *newValue = NULL;
    int32_t int32Value;
    uint16_t taggedValueNameSize;

    if (get_indirect_string(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), &newValue))
    {
        CHK_OFAIL(mxf_get_utf16string_item_size(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), &taggedValueNameSize));
        CHK_MALLOC_ARRAY_OFAIL(newName, mxfUTF16Char, taggedValueNameSize);
        CHK_OFAIL(mxf_get_utf16string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), newName));
    }
    else if (get_indirect_int32(taggedValueSet, &MXF_ITEM_K(TaggedValue, Value), &int32Value))
    {
        CHK_OFAIL(get_int32_string(int32Value, &newValue));
        CHK_OFAIL(mxf_get_utf16string_item_size(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), &taggedValueNameSize));
        CHK_MALLOC_ARRAY_OFAIL(newName, mxfUTF16Char, taggedValueNameSize);
        CHK_OFAIL(mxf_get_utf16string_item(taggedValueSet, &MXF_ITEM_K(TaggedValue, Name), newName));
    }

    if (newValue == NULL)
    {
        return 0;
    }

    *name = newName;
    *value = newValue;
    return 1;

fail:
    SAFE_FREE(newName);
    SAFE_FREE(newValue);
    return 0;
}

int mxf_avid_get_mob_attribute(const mxfUTF16Char *name, const MXFList *names, const MXFList *values,
                               const mxfUTF16Char **value)
{
    MXFListIterator namesIter;
    MXFListIterator valuesIter;
    const mxfUTF16Char *listName;

    mxf_initialise_list_iter(&namesIter, names);
    mxf_initialise_list_iter(&valuesIter, values);
    while (mxf_next_list_iter_element(&namesIter) && mxf_next_list_iter_element(&valuesIter))
    {
        listName = (const mxfUTF16Char*)mxf_get_iter_element(&namesIter);

        if (wcscmp(name, listName) == 0)
        {
            *value = (const mxfUTF16Char*)mxf_get_iter_element(&valuesIter);
            return 1;
        }
    }

    return 0;
}

int mxf_avid_get_user_comment(const mxfUTF16Char *name, const MXFList *names, const MXFList *values,
                              const mxfUTF16Char **value)
{
    return mxf_avid_get_mob_attribute(name, names, values, value);
}

/* The ProductVersion type in AAF/Avid differs from MXF. The last record member (release aka type) is a UInt8 */
int mxf_avid_set_product_version_item(MXFMetadataSet *set, const mxfKey *itemKey, const mxfProductVersion *value)
{
    uint8_t buffer[mxfProductVersion_extlen - 1]; /* -1 because last member is UInt8 and not UInt16 */

    mxf_set_uint16(value->major,              buffer);
    mxf_set_uint16(value->minor,             &buffer[2]);
    mxf_set_uint16(value->patch,             &buffer[4]);
    mxf_set_uint16(value->build,             &buffer[6]);
    mxf_set_uint8((uint8_t)(value->release), &buffer[8]);

    CHK_ORET(mxf_set_item(set, itemKey, buffer, mxfProductVersion_extlen - 1));

    return 1;
}

void mxf_avid_get_product_version(const uint8_t *value, mxfProductVersion *result)
{
    uint8_t uint8_release;
    mxf_get_uint16( value,    &result->major);
    mxf_get_uint16(&value[2], &result->minor);
    mxf_get_uint16(&value[4], &result->patch);
    mxf_get_uint16(&value[6], &result->build);
    mxf_get_uint8( &value[8], &uint8_release);
    result->release = uint8_release;
}

int mxf_avid_get_product_version_item(MXFMetadataSet *set, const mxfKey *itemKey, mxfProductVersion *value)
{
    MXFMetadataItem *item = NULL;

    CHK_ORET(mxf_get_item(set, itemKey, &item));
    CHK_ORET(item->length == mxfProductVersion_extlen - 1);

    mxf_avid_get_product_version(item->value, value);

    return 1;
}

int mxf_avid_read_index_table_segment_2(MXFFile *mxfFile, uint64_t segmentLen,
                                        mxf_add_delta_entry *addDeltaEntry, void *addDeltaEntryData,
                                        mxf_add_index_entry *addIndexEntry, void *addIndexEntryData,
                                        MXFIndexTableSegment **segment)
{
    MXFIndexTableSegment *newSegment = NULL;
    mxfLocalTag localTag;
    uint16_t localLen;
    uint64_t totalLen;
    uint32_t deltaEntryArrayLen;
    uint32_t deltaEntryLen;
    int8_t posTableIndex;
    uint8_t slice;
    uint32_t elementData;
    uint32_t *sliceOffset = NULL;
    mxfRational *posTable = NULL;
    uint32_t indexEntryArrayLen;
    uint32_t indexEntryLen;
    uint8_t temporalOffset;
    uint8_t keyFrameOffset;
    uint8_t flags;
    uint64_t streamOffset;
    uint32_t entry;
    uint64_t actualLen;
    uint32_t actualEntryLen;
    uint8_t tempOptBool;
    uint8_t i;

    CHK_ORET(mxf_create_index_table_segment(&newSegment));

    totalLen = 0;
    while (totalLen < segmentLen)
    {
        CHK_OFAIL(mxf_read_local_tl(mxfFile, &localTag, &localLen));
        totalLen += mxfLocalTag_extlen + 2;

        actualLen = 0;

        switch (localTag)
        {
            case 0x3c0a:
                CHK_OFAIL(localLen == mxfUUID_extlen);
                CHK_OFAIL(mxf_read_uuid(mxfFile, &newSegment->instanceUID));
                break;
            case 0x3f0b:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_int32(mxfFile, &newSegment->indexEditRate.numerator));
                CHK_OFAIL(mxf_read_int32(mxfFile, &newSegment->indexEditRate.denominator));
                break;
            case 0x3f0c:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_int64(mxfFile, &newSegment->indexStartPosition));
                break;
            case 0x3f0d:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_int64(mxfFile, &newSegment->indexDuration));
                break;
            case 0x3f05:
                CHK_OFAIL(localLen == 4);
                CHK_OFAIL(mxf_read_uint32(mxfFile, &newSegment->editUnitByteCount));
                break;
            case 0x3f06:
                CHK_OFAIL(localLen == 4);
                CHK_OFAIL(mxf_read_uint32(mxfFile, &newSegment->indexSID));
                break;
            case 0x3f07:
                CHK_OFAIL(localLen == 4);
                CHK_OFAIL(mxf_read_uint32(mxfFile, &newSegment->bodySID));
                break;
            case 0x3f08:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &newSegment->sliceCount));
                break;
            case 0x3f0e:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &newSegment->posTableCount));
                break;
            case 0x3f0f:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_uint64(mxfFile, &newSegment->extStartOffset));
                break;
            case 0x3f10:
                CHK_OFAIL(localLen == 8);
                CHK_OFAIL(mxf_read_uint64(mxfFile, &newSegment->vbeByteCount));
                break;
            case 0x3f11:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &tempOptBool));
                newSegment->singleIndexLocation = (tempOptBool ? MXF_OPT_BOOL_TRUE : MXF_OPT_BOOL_FALSE);
                break;
            case 0x3f12:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &tempOptBool));
                newSegment->singleEssenceLocation = (tempOptBool ? MXF_OPT_BOOL_TRUE : MXF_OPT_BOOL_FALSE);
                break;
            case 0x3f13:
                CHK_OFAIL(localLen == 1);
                CHK_OFAIL(mxf_read_uint8(mxfFile, &tempOptBool));
                newSegment->forwardIndexDirection = (tempOptBool ? MXF_OPT_BOOL_TRUE : MXF_OPT_BOOL_FALSE);
                break;
            case 0x3f09:
                CHK_ORET(mxf_read_uint32(mxfFile, &deltaEntryArrayLen));
                CHK_ORET(mxf_read_uint32(mxfFile, &deltaEntryLen));
                if (deltaEntryArrayLen != 0)
                {
                    CHK_ORET(deltaEntryLen == 6);
                }
                CHK_ORET(localLen == 8 + deltaEntryArrayLen * 6);
                entry = deltaEntryArrayLen;
                for (; entry > 0; entry--)
                {
                    CHK_ORET(mxf_read_int8(mxfFile, &posTableIndex));
                    CHK_ORET(mxf_read_uint8(mxfFile, &slice));
                    CHK_ORET(mxf_read_uint32(mxfFile, &elementData));
                    if (addDeltaEntry != NULL)
                    {
                        CHK_OFAIL((*addDeltaEntry)(addDeltaEntryData, deltaEntryArrayLen, newSegment, posTableIndex,
                                                   slice, elementData));
                    }
                }
                break;
            case 0x3f0a:
                /* NOTE: Avid ignores the local len and only looks at the index array len value */
                actualLen = 0;
                if (newSegment->sliceCount > 0)
                {
                    CHK_MALLOC_ARRAY_ORET(sliceOffset, uint32_t, newSegment->sliceCount);
                }
                if (newSegment->posTableCount > 0)
                {
                    CHK_MALLOC_ARRAY_ORET(posTable, mxfRational, newSegment->posTableCount);
                }
                CHK_OFAIL(mxf_read_uint32(mxfFile, &indexEntryArrayLen));
                actualLen += 4;
                CHK_OFAIL(mxf_read_uint32(mxfFile, &indexEntryLen));
                actualLen += 4;
                if (indexEntryArrayLen != 0 &&
                    indexEntryLen != (uint32_t)11 + newSegment->sliceCount * 4 + newSegment->posTableCount * 8)
                {
                    /* GrassValley D10 sample files where found to have SliceCount 1 but index entry length 11 */
                    mxf_log_warn("Index entry length %u is incorrect; it should be %u\n", indexEntryLen,
                                 (uint32_t)11 + newSegment->sliceCount * 4 + newSegment->posTableCount * 8);
                }
                entry = indexEntryArrayLen;
                for (; entry > 0; entry--)
                {
                    actualEntryLen = 0;
                    CHK_OFAIL(mxf_read_uint8(mxfFile, &temporalOffset));
                    CHK_OFAIL(mxf_read_uint8(mxfFile, &keyFrameOffset));
                    CHK_OFAIL(mxf_read_uint8(mxfFile, &flags));
                    CHK_OFAIL(mxf_read_uint64(mxfFile, &streamOffset));
                    actualEntryLen += 11;
                    for (i = 0; i < newSegment->sliceCount && actualEntryLen < indexEntryLen; i++)
                    {
                        CHK_OFAIL(mxf_read_uint32(mxfFile, &sliceOffset[i]));
                        actualEntryLen += 4;
                    }
                    for (i = 0; i < newSegment->posTableCount && actualEntryLen < indexEntryLen; i++)
                    {
                        CHK_OFAIL(mxf_read_int32(mxfFile, &posTable[i].numerator));
                        CHK_OFAIL(mxf_read_int32(mxfFile, &posTable[i].denominator));
                        actualEntryLen += 8;
                    }
                    if (addIndexEntry != NULL)
                    {
                        CHK_OFAIL((*addIndexEntry)(addIndexEntryData, indexEntryArrayLen, newSegment, temporalOffset,
                                                   keyFrameOffset, flags, streamOffset, sliceOffset, posTable));
                    }

                    if (actualEntryLen < indexEntryLen)
                    {
                        CHK_OFAIL(mxf_skip(mxfFile, indexEntryLen - actualEntryLen));
                    }
                    actualLen += indexEntryLen;
                }
                SAFE_FREE(sliceOffset);
                SAFE_FREE(posTable);
                break;
            default:
                mxf_log_warn("Unknown local item (%u) in index table segment\n", localTag);
                CHK_OFAIL(mxf_skip(mxfFile, localLen));
        }

        if (actualLen > 0)
        {
            totalLen += actualLen;
        }
        else
        {
            totalLen += localLen;
        }
    }
    CHK_ORET(totalLen == segmentLen);

    *segment = newSegment;
    return 1;

fail:
    SAFE_FREE(sliceOffset);
    SAFE_FREE(posTable);
    mxf_free_index_table_segment(&newSegment);
    return 0;
}

int mxf_avid_read_index_table_segment(MXFFile *mxfFile, uint64_t segmentLen, MXFIndexTableSegment **segment)
{
    CHK_ORET(mxf_avid_read_index_table_segment_2(mxfFile, segmentLen, mxf_default_add_delta_entry, NULL,
                                                 mxf_default_add_index_entry, NULL, segment));

    return 1;
}

int mxf_avid_is_mjpeg_essence_element(const mxfKey *key)
{
    return mxf_equals_key_prefix(key, &MXF_EE_K(AvidMJPEGClipWrapped), 13) && key->octet14 == 0x01;
}

int mxf_avid_is_dnxhd_essence_element(const mxfKey *key)
{
    return mxf_equals_key_prefix(key, &MXF_EE_K(DNxHD), 13) && key->octet14 == 0x06;
}

int mxf_avid_is_mpeg_essence_element(const mxfKey *key)
{
    return mxf_equals_key_prefix(key, &MXF_EE_K(AvidMPEGClipWrapped), 13) && key->octet14 == 0x03;
}

int mxf_avid_is_unc10bit_essence_element(const mxfKey *key)
{
    return mxf_equals_key_prefix(key, &MXF_EE_K(AvidUnc10BitClipWrapped), 13) && key->octet14 == 0x07;
}

int mxf_avid_is_essence_element(const mxfKey *key)
{
    static const mxfKey avidEEKPrefix = MXF_AVID_EE_K(0, 0, 0, 0);
    return mxf_equals_key_prefix(key, &avidEEKPrefix, 12);
}


void mxf_avid_set_auid(const mxfUL *ul, mxfAUID *auid)
{
    /* an AUID is an MXF UL half-swapped */
    mxf_swap_uid((mxfUID*)auid, (const mxfUID*)ul);
}

