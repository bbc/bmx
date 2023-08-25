/*
 * Extracts the macros for the meta-dictionary and dictionary from an Avid MXF file
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

/*


    NOTES:
    - In the example Avid file the Character type is missing. This type def must be added
    and the String type must reference it
    - In the example Avid file the Integer size and isSigned properties could not be read
    because the item tags are not registered in the primer pack. These properties must be
    manually edited
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mxf/mxf.h>
#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>


#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in line %d\n", #cmd, __LINE__); \
        exit(1); \
    }

#define CHECK_OFAIL(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in line %d\n", #cmd, __LINE__); \
        goto fail; \
    }

#define CHECK_ORET(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed in line %d\n", #cmd, __LINE__); \
        return 0; \
    }



static int convert_string(const mxfUTF16Char *utf16Str, char **str)
{
    size_t utf8Size;

    utf8Size = wcstombs(0, utf16Str, 0);
    CHECK_OFAIL(utf8Size != (size_t)(-1));
    utf8Size += 1;
    CHECK_OFAIL((*str = malloc(utf8Size)) != NULL);
    wcstombs(*str, utf16Str, utf8Size);

    return 1;

fail:
    return 0;
}

static int get_string_value(MXFMetadataSet *set, const mxfKey *itemKey, char **str)
{
    uint16_t utf16Size;
    mxfUTF16Char *utf16Str = NULL;

    CHECK_OFAIL(mxf_get_utf16string_item_size(set, itemKey, &utf16Size));
    CHECK_OFAIL((utf16Str = malloc(utf16Size * sizeof(mxfUTF16Char))) != NULL);
    CHECK_OFAIL(mxf_get_utf16string_item(set, itemKey, utf16Str));

    CHECK_OFAIL(convert_string(utf16Str, str));

    SAFE_FREE(utf16Str);
    return 1;

fail:
    SAFE_FREE(utf16Str);
    return 0;
}

static void print_spaces(int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        printf(" ");
    }
}

static int print_string_ext(const uint8_t *value, uint16_t len, int spaceCount)
{
    uint16_t utf16Size;
    mxfUTF16Char *utf16Str = NULL;
    char *str = NULL;

    utf16Size = mxf_get_utf16string_size(value, len);
    CHECK_OFAIL((utf16Str = malloc(utf16Size * sizeof(mxfUTF16Char))) != NULL);
    mxf_get_utf16string(value, len, utf16Str);

    CHECK_OFAIL(convert_string(utf16Str, &str));

    print_spaces(spaceCount);
    if (str == NULL)
    {
        printf("NULL");
    }
    else
    {
        printf("L\"%s\"", str);
    }

    SAFE_FREE(utf16Str);
    SAFE_FREE(str);
    return 1;

fail:
    SAFE_FREE(utf16Str);
    SAFE_FREE(str);
    return 0;
}

static int print_string(const char *value, int spaceCount)
{
    print_spaces(spaceCount);
    if (value == NULL)
    {
        printf("NULL");
    }
    else
    {
        printf("L\"%s\"", value);
    }

    return 1;
}

static int print_string_item(MXFMetadataSet *set, const mxfKey *itemKey, int spaceCount)
{
    char *value = NULL;

    if (mxf_have_item(set, itemKey))
    {
        CHECK_ORET(get_string_value(set, itemKey, &value));
    }

    print_string(value, spaceCount);

    SAFE_FREE(value);
    return 1;
}

static void print_ul(const mxfUL *value, int spaceCount)
{
    int i;

    print_spaces(spaceCount);
    printf("LABEL(");

    for (i = 0; i < 16; i++)
    {
        if (i != 0)
        {
            printf(", ");
        }
        printf("0x%02x", ((unsigned char*)value)[i]);
    }

    printf(")");
}

static int print_ul_item(MXFMetadataSet *set, const mxfKey *itemKey, int spaceCount)
{
    mxfUL value;

    CHECK_ORET(mxf_get_ul_item(set, itemKey, &value));

    print_ul(&value, spaceCount);

    return 1;
}

static void print_ul_2(const mxfUL *value, int spaceCount)
{
    int i;

    print_spaces(spaceCount);
    printf("LABEL_2(");

    for (i = 0; i < 16; i++)
    {
        if (i != 0)
        {
            printf(", ");
        }
        printf("0x%02x", ((unsigned char*)value)[i]);
    }

    printf(")");
}

static int print_ul_item_2(MXFMetadataSet *set, const mxfKey *itemKey, int spaceCount)
{
    mxfUL value;

    CHECK_ORET(mxf_get_ul_item(set, itemKey, &value));

    print_ul_2(&value, spaceCount);

    return 1;
}

static int print_bool_item(MXFMetadataSet *set, const mxfKey *itemKey, int spaceCount)
{
    mxfBoolean value = 0;

    if (mxf_have_item(set, itemKey))
    {
        CHECK_ORET(mxf_get_boolean_item(set, itemKey, &value));
    }

    print_spaces(spaceCount);
    printf("%d", value);

    return 1;
}

static int print_uint8_item(MXFMetadataSet *set, const mxfKey *itemKey, int spaceCount)
{
    uint8_t value = 0;

    CHECK_ORET(mxf_get_uint8_item(set, itemKey, &value));

    print_spaces(spaceCount);
    printf("%d", value);

    return 1;
}

static int print_uint32_item(MXFMetadataSet *set, const mxfKey *itemKey, int spaceCount)
{
    uint32_t value = 0;

    CHECK_ORET(mxf_get_uint32_item(set, itemKey, &value));

    print_spaces(spaceCount);
    printf("%d", value);

    return 1;
}

static int print_int64(int64_t value, int spaceCount)
{
    print_spaces(spaceCount);
    printf("%" PRId64 "", value);

    return 1;
}

static int print_local_id_item(MXFMetadataSet *set, const mxfKey *itemKey, int spaceCount)
{
    uint16_t value;

    CHECK_ORET(mxf_get_uint16_item(set, itemKey, &value));

    print_spaces(spaceCount);
    if (value < 0x8000)
    {
        printf("0x%04x", value);
    }
    else
    {
        printf("0x0000");
    }

    return 1;
}

static int print_weakref_ext(MXFMetadataSet *set, const mxfUUID *id, const mxfKey *refItemKey, int spaceCount)
{
    MXFMetadataSet *refSet;
    int i;
    mxfUL value;

    CHECK_ORET(mxf_dereference(set->headerMetadata, id, &refSet));

    CHECK_ORET(mxf_get_ul_item(refSet, refItemKey, &value));

    print_spaces(spaceCount);
    printf("WEAKREF(");

    for (i = 0; i < 16; i++)
    {
        if (i != 0)
        {
            printf(", ");
        }
        printf("0x%02x", ((unsigned char*)&value)[i]);
    }

    printf(")");

    return 1;
}

static int print_weakref_item(MXFMetadataSet *set, const mxfKey *itemKey, const mxfKey *refItemKey, int spaceCount)
{
    MXFMetadataSet *refSet;
    int i;
    mxfUUID value;

    CHECK_ORET(mxf_get_weakref_item(set, itemKey, &refSet));

    CHECK_ORET(mxf_get_uuid_item(refSet, refItemKey, &value));

    print_spaces(spaceCount);
    printf("WEAKREF(");

    for (i = 0; i < 16; i++)
    {
        if (i != 0)
        {
            printf(", ");
        }
        printf("0x%02x", ((unsigned char*)&value)[i]);
    }

    printf(")");

    return 1;
}

static int get_string_array_item_element(MXFMetadataSet *set, const mxfKey *itemKey, uint32_t index, uint8_t **value, uint16_t *valueLen)
{
    MXFMetadataItem *item;
    uint32_t count;
    uint16_t i;

    CHECK_ORET(mxf_get_item(set, itemKey, &item));


    i = 0;
    count = 0;
    if (index > 0)
    {
        while (count < index && i < item->length)
        {
            if (item->value[i] == 0 && item->value[i + 1] == 0)
            {
                i++;
                count++;
            }
            i++;
        }
        if (count != index)
        {
            return 0;
        }
    }

    *value = &item->value[i];
    *valueLen = 0;
    while (i < item->length)
    {
        if (item->value[i] == 0 && item->value[i + 1] == 0)
        {
            *valueLen += 2;
            break;
        }
        *valueLen += 1;
        i++;
    }

    return 1;
}



static int print_metadef_data(MXFMetadataSet *defSet, int spaceCount)
{
    CHECK_ORET(print_ul_item(defSet, &MXF_ITEM_K(MetaDefinition, Identification), spaceCount));
    printf(",\n");
    CHECK_ORET(print_string_item(defSet, &MXF_ITEM_K(MetaDefinition, Name), spaceCount));
    printf(",\n");
    CHECK_ORET(print_string_item(defSet, &MXF_ITEM_K(MetaDefinition, Description), spaceCount));

    return 1;
}

static int process_metadef(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *metaDefSet)
{
    uint32_t count;
    uint32_t i;
    uint8_t *arrayElement;
    mxfUL ul;
    uint8_t *stringArrayElement;
    uint16_t stringLen;
    int64_t int64Value;
    mxfUUID uuid;


    if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(ClassDefinition)))
    {
        printf("CLASS_DEF(\n");
        print_metadef_data(metaDefSet, 4);
        printf(",\n");
        print_weakref_item(metaDefSet, &MXF_ITEM_K(ClassDefinition, ParentClass), &MXF_ITEM_K(MetaDefinition, Identification), 4);
        printf(",\n");
        print_bool_item(metaDefSet, &MXF_ITEM_K(ClassDefinition, IsConcrete), 4);
        printf("\n");
        printf(");\n\n");
    }
    else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(PropertyDefinition)))
    {
        print_spaces(4);
        printf("PROPERTY_DEF(\n");
        print_metadef_data(metaDefSet, 8);
        printf(",\n");
        print_ul_item_2(metaDefSet, &MXF_ITEM_K(PropertyDefinition, Type), 8);
        printf(",\n");
        print_bool_item(metaDefSet, &MXF_ITEM_K(PropertyDefinition, IsOptional), 8);
        printf(",\n");
        print_local_id_item(metaDefSet, &MXF_ITEM_K(PropertyDefinition, LocalIdentification), 8);
        printf(",\n");
        print_bool_item(metaDefSet, &MXF_ITEM_K(PropertyDefinition, IsUniqueIdentifier), 8);
        printf("\n");
        print_spaces(4);
        printf(");\n\n");
    }
    else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinition)))
    {
        if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionCharacter)))
        {
            printf("CHARACTER_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionEnumeration)))
        {
            printf("ENUM_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Type), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf("\n");
            printf(");\n\n");

            CHECK_ORET(mxf_get_array_item_count(metaDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Values), &count));
            for (i = 0; i < count; i++)
            {
                CHECK_ORET(mxf_get_array_item_element(metaDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Values), i, &arrayElement));
                mxf_get_int64(arrayElement, &int64Value);

                CHECK_ORET(get_string_array_item_element(metaDefSet, &MXF_ITEM_K(TypeDefinitionEnumeration, Names), i, &stringArrayElement, &stringLen));

                print_spaces(4);
                printf("ENUM_ELEMENT(\n");
                print_string_ext(stringArrayElement, stringLen, 8);
                printf(",\n");
                print_int64(int64Value, 8);
                printf("\n");
                print_spaces(4);
                printf(");\n");
            }
            printf("\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionExtendibleEnumeration)))
        {
            printf("EXTENUM_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(");\n\n");

            CHECK_ORET(mxf_get_array_item_count(metaDefSet, &MXF_ITEM_K(TypeDefinitionExtendibleEnumeration, Values), &count));
            for (i = 0; i < count; i++)
            {
                CHECK_ORET(mxf_get_array_item_element(metaDefSet, &MXF_ITEM_K(TypeDefinitionExtendibleEnumeration, Values), i, &arrayElement));
                mxf_get_ul(arrayElement, &ul);

                CHECK_ORET(get_string_array_item_element(metaDefSet, &MXF_ITEM_K(TypeDefinitionExtendibleEnumeration, Names), i, &stringArrayElement, &stringLen));

                print_spaces(4);
                printf("EXTENUM_ELEMENT(\n");
                print_string_ext(stringArrayElement, stringLen, 8);
                printf(",\n");
                print_ul(&ul, 8);
                printf("\n");
                print_spaces(4);
                printf(");\n");
            }
            printf("\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionFixedArray)))
        {
            printf("FIXEDARRAY_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementType), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf(",\n");
            print_uint32_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionFixedArray, ElementCount), 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionInteger)))
        {
            printf("INTEGER_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_uint8_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionInteger, Size), 4);
            printf(",\n");
            print_bool_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionInteger, IsSigned), 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionIndirect)))
        {
            printf("INDIRECT_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionOpaque)))
        {
            printf("OPAQUE_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionRecord)))
        {
            printf("RECORD_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf("\n");
            printf(");\n\n");

            CHECK_ORET(mxf_get_array_item_count(metaDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberTypes), &count));
            for (i = 0; i < count; i++)
            {
                CHECK_ORET(mxf_get_array_item_element(metaDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberTypes), i, &arrayElement));
                mxf_get_uuid(arrayElement, &uuid);

                CHECK_ORET(get_string_array_item_element(metaDefSet, &MXF_ITEM_K(TypeDefinitionRecord, MemberNames), i, &stringArrayElement, &stringLen));

                print_spaces(4);
                printf("RECORD_MEMBER(\n");
                print_string_ext(stringArrayElement, stringLen, 8);
                printf(",\n");
                print_weakref_ext(metaDefSet, &uuid, &MXF_ITEM_K(MetaDefinition, Identification), 8);
                printf("\n");
                print_spaces(4);
                printf(");\n");
            }
            printf("\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionRename)))
        {
            printf("RENAME_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionRename, RenamedType), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionSet)))
        {
            printf("SET_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionSet, ElementType), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionStream)))
        {
            printf("STREAM_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionString)))
        {
            printf("STRING_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionString, ElementType), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionStrongObjectReference)))
        {
            printf("STRONGOBJREF_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionStrongObjectReference, ReferencedType), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionVariableArray)))
        {
            printf("VARARRAY_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionVariableArray, ElementType), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf("\n");
            printf(");\n\n");
        }
        else if (mxf_is_subclass_of(headerMetadata->dataModel, &metaDefSet->key, &MXF_SET_K(TypeDefinitionWeakObjectReference)))
        {
            printf("WEAKOBJREF_DEF(\n");
            print_metadef_data(metaDefSet, 4);
            printf(",\n");
            print_weakref_item(metaDefSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferencedType), &MXF_ITEM_K(MetaDefinition, Identification), 4);
            printf("\n");
            printf(");\n\n");

            CHECK_ORET(mxf_get_array_item_count(metaDefSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferenceTargetSet), &count));
            for (i = 0; i < count; i++)
            {
                CHECK_ORET(mxf_get_array_item_element(metaDefSet, &MXF_ITEM_K(TypeDefinitionWeakObjectReference, ReferenceTargetSet), i, &arrayElement));
                mxf_get_ul(arrayElement, &ul);

                print_spaces(4);
                printf("WEAKOBJREF_TARGET_ELEMENT(\n");
                print_ul(&ul, 8);
                printf("\n");
                print_spaces(4);
                printf(");\n");
            }
            printf("\n");
        }
        else
        {
            fprintf(stderr, "TODO: Unexpected type definition\n");
            print_metadef_data(metaDefSet, 0);
            return 0;
        }
    }
    else
    {
        fprintf(stderr, "TODO: Unexpected meta-definition\n");
        print_metadef_data(metaDefSet, 0);
        return 0;
    }

    return 1;
}

static int print_defobject_data(MXFMetadataSet *defSet)
{
    CHECK_ORET(print_ul_item(defSet, &MXF_ITEM_K(DefinitionObject, Identification), 4));
    printf(",\n");
    CHECK_ORET(print_string_item(defSet, &MXF_ITEM_K(DefinitionObject, Name), 4));
    printf(",\n");
    CHECK_ORET(print_string_item(defSet, &MXF_ITEM_K(DefinitionObject, Description), 4));
    printf("\n");

    return 1;
}

static int process_defobject(MXFHeaderMetadata *headerMetadata, MXFMetadataSet *defSet)
{
    if (mxf_is_subclass_of(headerMetadata->dataModel, &defSet->key, &MXF_SET_K(DataDefinition)))
    {
        printf("DATA_DEF(\n");
        print_defobject_data(defSet);
        printf(");\n\n");
    }
    else if (mxf_is_subclass_of(headerMetadata->dataModel, &defSet->key, &MXF_SET_K(ContainerDefinition)))
    {
        printf("CONTAINER_DEF(\n");
        print_defobject_data(defSet);
        printf(");\n\n");
    }
    else if (mxf_is_subclass_of(headerMetadata->dataModel, &defSet->key, &MXF_SET_K(TaggedValueDefinition)))
    {
        printf("TAGGEDVALUE_DEF(\n");
        print_defobject_data(defSet);
        printf(");\n\n");
    }
    else
    {
        fprintf(stderr, "TODO: Unexpected definition object\n");
        print_defobject_data(defSet);
        return 0;
    }

    return 1;
}


int main(int argc, const char **argv)
{
    const char *filename = NULL;
    MXFFile *mxfFile = NULL;
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFHeaderMetadata *headerMetadata;
    MXFDataModel *dataModel;
    MXFPartition *headerPartition;
    MXFListIterator setsIter;
    int outputMetaDefs = 1;
    int outputDefObjs = 1;


    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <mxf filename>\n", argv[0]);
        return 1;
    }

    filename = argv[1];


    /* open file */

    CHECK(mxf_disk_file_open_read(filename, &mxfFile));


    /* read header partition pack */

    CHECK(mxf_read_header_pp_kl(mxfFile, &key, &llen, &len));
    CHECK(mxf_read_partition(mxfFile, &key, len, &headerPartition));


    /* skip to the header metadata */

    while (1)
    {
        CHECK(mxf_read_next_nonfiller_kl(mxfFile, &key, &llen, &len));
        if (mxf_is_header_metadata(&key))
        {
            break;
        }
    }


    /* read the header metadata */

    CHECK(mxf_load_data_model(&dataModel));
    CHECK(mxf_avid_load_extensions(dataModel));
    CHECK(mxf_finalise_data_model(dataModel));

    CHECK(mxf_create_header_metadata(&headerMetadata, dataModel));
    CHECK(mxf_read_header_metadata(mxfFile, headerMetadata, headerPartition->headerByteCount, &key, llen, len));


    /* process sets */

    mxf_initialise_list_iter(&setsIter, &headerMetadata->sets);
    while (mxf_next_list_iter_element(&setsIter))
    {
        MXFMetadataSet *set = (MXFMetadataSet*)mxf_get_iter_element(&setsIter);

        if (mxf_is_subclass_of(dataModel, &set->key, &MXF_SET_K(MetaDefinition)))
        {
            if (outputMetaDefs)
            {
                CHECK(process_metadef(headerMetadata, set));
            }
        }
        else if (mxf_is_subclass_of(dataModel, &set->key, &MXF_SET_K(DefinitionObject)))
        {
            if (outputDefObjs)
            {
                CHECK(process_defobject(headerMetadata, set));
            }
        }
    }


    return 0;
}

