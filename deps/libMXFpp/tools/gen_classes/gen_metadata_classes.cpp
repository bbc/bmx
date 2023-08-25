/*
 * Generate base and user editable metadata classes from the libMXF data model
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>


#define CHECK(cmd) \
    if (!(cmd)) \
    { \
        fprintf(stderr, "'%s' failed at line %d\n", #cmd, __LINE__); \
        exit(1); \
    }

typedef struct
{
    FILE *baseHeaderFile;
    const char *directory;
    MXFDataModel *dataModel;
} SetDefsData;



static const char* get_sw_ref_name(MXFDataModel *dataModel, MXFItemDef *itemDef, char refName[256])
{
    (void)dataModel;


    size_t i;
    struct NameInfo
    {
        const mxfKey *key;
        const char *name;
    } nameInfo[] =
    {
        { &MXF_ITEM_K(Preface, ContentStorage), "ContentStorage*" },
        { &MXF_ITEM_K(GenericTrack, Sequence), "StructuralComponent*" },
        { &MXF_ITEM_K(DMSegment, DMFramework), "DMFramework*" },
        { &MXF_ITEM_K(SourcePackage, Descriptor), "GenericDescriptor*" },
        { &MXF_ITEM_K(Preface, Identifications), "Identification*" },
        { &MXF_ITEM_K(GenericPackage, Tracks), "GenericTrack*" },
        { &MXF_ITEM_K(Sequence, StructuralComponents), "StructuralComponent*" },
        { &MXF_ITEM_K(GenericDescriptor, Locators), "Locator*" },
        { &MXF_ITEM_K(MultipleDescriptor, SubDescriptorUIDs), "GenericDescriptor*" },
        { &MXF_ITEM_K(ContentStorage, Packages), "GenericPackage*" },
        { &MXF_ITEM_K(ContentStorage, EssenceContainerData), "EssenceContainerData*" },
        { &MXF_ITEM_K(Preface, PrimaryPackage), "GenericPackage*" },
        { &MXF_ITEM_K(TextBasedDMFramework, TextBasedObject), "TextBasedObject*" },
    };

    for (i = 0; i < ARRAY_SIZE(nameInfo); i++)
    {
        if (mxf_equals_key(&itemDef->key, nameInfo[i].key))
        {
            strcpy(refName, nameInfo[i].name);
            break;
        }
    }
    if (i == ARRAY_SIZE(nameInfo))
    {
        fprintf(stderr, "Unknown class strong/weak reference\n");
        strcpy(refName, "XXX");
    }

    return refName;
}

static const char* get_type_name(MXFDataModel *dataModel, MXFItemDef *itemDef,
    MXFItemType *itemType, char typeName[256])
{
    MXFItemType *interpretTtemType;

    CHECK(itemType->category == MXF_BASIC_TYPE_CAT ||
        itemType->category == MXF_INTERPRET_TYPE_CAT ||
        itemType->category == MXF_COMPOUND_TYPE_CAT);

    if (itemType->category == MXF_BASIC_TYPE_CAT)
    {
        switch (itemType->typeId)
        {
            case MXF_INT8_TYPE:
                strcpy(typeName, "int8_t");
                break;
            case MXF_INT16_TYPE:
                strcpy(typeName, "int16_t");
                break;
            case MXF_INT32_TYPE:
                strcpy(typeName, "int32_t");
                break;
            case MXF_INT64_TYPE:
                strcpy(typeName, "int64_t");
                break;
            case MXF_UINT8_TYPE:
                strcpy(typeName, "uint8_t");
                break;
            case MXF_UINT16_TYPE:
                strcpy(typeName, "uint16_t");
                break;
            case MXF_UINT32_TYPE:
                strcpy(typeName, "uint32_t");
                break;
            case MXF_UINT64_TYPE:
                strcpy(typeName, "uint64_t");
                break;
            case MXF_RAW_TYPE:
                strcpy(typeName, "ByteArray");
                break;
            default:
                fprintf(stderr, "Warning: unknown basic type %d\n", itemType->typeId);
                strcpy(typeName, "XXX");
                break;
        }
    }
    else if (itemType->category == MXF_INTERPRET_TYPE_CAT)
    {
        switch (itemType->typeId)
        {
            case MXF_VERSIONTYPE_TYPE:
                CHECK((interpretTtemType = mxf_get_item_def_type(dataModel, itemType->info.interpret.typeId)) != NULL);
                get_type_name(dataModel, itemDef, interpretTtemType, typeName);
                break;
            case MXF_UTF16_TYPE:
                strcpy(typeName, "std::string");
                break;
            case MXF_UTF8_TYPE:
                strcpy(typeName, "std::string");
                break;
            case MXF_BOOLEAN_TYPE:
                strcpy(typeName, "bool");
                break;
            case MXF_ISO7_TYPE:
                strcpy(typeName, "std::string");
                break;
            case MXF_LENGTH_TYPE:
                CHECK((interpretTtemType = mxf_get_item_def_type(dataModel, itemType->info.interpret.typeId)) != NULL);
                get_type_name(dataModel, itemDef, interpretTtemType, typeName);
                break;
            case MXF_POSITION_TYPE:
                CHECK((interpretTtemType = mxf_get_item_def_type(dataModel, itemType->info.interpret.typeId)) != NULL);
                get_type_name(dataModel, itemDef, interpretTtemType, typeName);
                break;
            case MXF_RGBACODE_TYPE:
                CHECK((interpretTtemType = mxf_get_item_def_type(dataModel, itemType->info.interpret.typeId)) != NULL);
                get_type_name(dataModel, itemDef, interpretTtemType, typeName);
                break;
            case MXF_STREAM_TYPE:
                strcpy(typeName, "ByteArray");
                break;
            case MXF_DATAVALUE_TYPE:
                strcpy(typeName, "ByteArray");
                break;
            case MXF_IDENTIFIER_TYPE:
                strcpy(typeName, "ByteArray");
                break;
            case MXF_OPAQUE_TYPE:
                strcpy(typeName, "ByteArray");
                break;
            case MXF_UMID_TYPE:
                strcpy(typeName, "mxfUMID");
                break;
            case MXF_UID_TYPE:
                strcpy(typeName, "mxfUID");
                break;
            case MXF_UL_TYPE:
                strcpy(typeName, "mxfUL");
                break;
            case MXF_UUID_TYPE:
                strcpy(typeName, "mxfUUID");
                break;
            case MXF_AUID_TYPE:
                strcpy(typeName, "mxfAUID");
                break;
            case MXF_PACKAGEID_TYPE:
                CHECK((interpretTtemType = mxf_get_item_def_type(dataModel, itemType->info.interpret.typeId)) != NULL);
                get_type_name(dataModel, itemDef, interpretTtemType, typeName);
                break;
            case MXF_STRONGREF_TYPE:
                get_sw_ref_name(dataModel, itemDef, typeName);
                break;
            case MXF_WEAKREF_TYPE:
                get_sw_ref_name(dataModel, itemDef, typeName);
                break;
            case MXF_ORIENTATION_TYPE:
                CHECK((interpretTtemType = mxf_get_item_def_type(dataModel, itemType->info.interpret.typeId)) != NULL);
                get_type_name(dataModel, itemDef, interpretTtemType, typeName);
                break;
            case MXF_CODED_CONTENT_TYPE_TYPE:
                CHECK((interpretTtemType = mxf_get_item_def_type(dataModel, itemType->info.interpret.typeId)) != NULL);
                get_type_name(dataModel, itemDef, interpretTtemType, typeName);
                break;
            default:
                fprintf(stderr, "Warning: unknown interpret type %d\n", itemType->typeId);
                strcpy(typeName, "XXX");
                break;
        }
    }
    else /* MXF_COMPOUND_TYPE_CAT */
    {
        switch (itemType->typeId)
        {
            case MXF_RATIONAL_TYPE:
                strcpy(typeName, "mxfRational");
                break;
            case MXF_TIMESTAMP_TYPE:
                strcpy(typeName, "mxfTimestamp");
                break;
            case MXF_PRODUCTVERSION_TYPE:
                strcpy(typeName, "mxfProductVersion");
                break;
            case MXF_INDIRECT_TYPE:
                strcpy(typeName, "mxfIndirect");
                break;
            case MXF_RGBALAYOUTCOMPONENT_TYPE:
                strcpy(typeName, "mxfRGBALayoutComponent");
                break;
            default:
                fprintf(stderr, "Warning: unknown compound type %d\n", itemType->typeId);
                strcpy(typeName, "XXX");
                break;
        }
    }

    return typeName;
}

static void gen_class(const char *directory, MXFDataModel *dataModel, MXFSetDef *setDef)
{
    FILE *baseSourceFile;
    FILE *baseHeaderFile;
    FILE *sourceFile;
    FILE *headerFile;
    char filename[FILENAME_MAX];
    char baseClassName[256];
    char baseClassNameU[256];
    char className[256];
    char classNameU[256];
    char parentClassName[256];
    int i;
    MXFListIterator itemIter;
    char itemName[256];
    char typeName[256];
    char elementTypeName[256];
    char c;


    /* open header and source files */

    strcpy(filename, directory);
    strcat(filename, "/base/");
    strcat(filename, setDef->name);
    strcat(filename, "Base.h");
    if ((baseHeaderFile = fopen(filename, "wb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }

    strcpy(filename, directory);
    strcat(filename, "/base/");
    strcat(filename, setDef->name);
    strcat(filename, "Base.cpp");
    if ((baseSourceFile = fopen(filename, "wb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }

    strcpy(filename, directory);
    strcat(filename, "/");
    strcat(filename, setDef->name);
    strcat(filename, ".h");
    if ((headerFile = fopen(filename, "wb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }

    strcpy(filename, directory);
    strcat(filename, "/");
    strcat(filename, setDef->name);
    strcat(filename, ".cpp");
    if ((sourceFile = fopen(filename, "wb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }



    /* get names */

    strcpy(className, setDef->name);
    className[0] = toupper(className[0]);
    strcpy(classNameU, setDef->name);
    for (i = 0; i < (int)strlen(classNameU); i++)
    {
        classNameU[i] = toupper(classNameU[i]);
    }
    strcpy(baseClassName, setDef->name);
    strcat(baseClassName, "Base");
    className[0] = toupper(className[0]);
    strcpy(baseClassNameU, setDef->name);
    strcat(baseClassNameU, "_Base");
    baseClassNameU[0] = toupper(baseClassNameU[0]);
    for (i = 0; i < (int)strlen(baseClassNameU); i++)
    {
        baseClassNameU[i] = toupper(baseClassNameU[i]);
    }
    if (setDef->parentSetDef == NULL)
    {
        strcpy(parentClassName, "MetadataSet");
    }
    else
    {
        strcpy(parentClassName, setDef->parentSetDef->name);
    }
    parentClassName[0] = toupper(parentClassName[0]);



    /* header */

    fprintf(baseHeaderFile,
        "/*\n"
        " * Copyright (C) 2008, British Broadcasting Corporation\n"
        " * All Rights Reserved.\n"
        " *\n"
        " * Author: Philip de Nier\n"
        " *\n"
        " * Redistribution and use in source and binary forms, with or without\n"
        " * modification, are permitted provided that the following conditions are met:\n"
        " *\n"
        " *     * Redistributions of source code must retain the above copyright notice,\n"
        " *       this list of conditions and the following disclaimer.\n"
        " *     * Redistributions in binary form must reproduce the above copyright\n"
        " *       notice, this list of conditions and the following disclaimer in the\n"
        " *       documentation and/or other materials provided with the distribution.\n"
        " *     * Neither the name of the British Broadcasting Corporation nor the names\n"
        " *       of its contributors may be used to endorse or promote products derived\n"
        " *       from this software without specific prior written permission.\n"
        " *\n"
        " * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
        " * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        " * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
        " * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
        " * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
        " * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
        " * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
        " * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
        " * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
        " * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
        " * POSSIBILITY OF SUCH DAMAGE.\n"
        " */\n"
        "\n"
        "#ifndef MXFPP_%s_H_\n"
        "#define MXFPP_%s_H_\n"
        "\n"
        "\n"
        "\n"
        "#include <%s/%s.h>\n"
        "\n"
        "\n"
        "namespace mxfpp\n"
        "{\n"
        "\n"
        "\n"
        "class %s : public %s\n"
        "{\n"
        "public:\n"
        "    friend class MetadataSetFactory<%s>;\n"
        "    static const mxfKey setKey;\n"
        "\n"
        "public:\n"
        "    %s(HeaderMetadata *headerMetadata);\n"
        "    virtual ~%s();\n"
        "\n"
        "\n",
        baseClassNameU,
        baseClassNameU,
        (setDef->parentSetDef != NULL) ? "libMXF++/metadata" : "libMXF++",
        (setDef->parentSetDef != NULL) ? parentClassName : "MetadataSet",
        baseClassName, parentClassName,
        baseClassName,
        baseClassName,
        baseClassName);


    fprintf(headerFile,
        "/*\n"
        " * Copyright (C) 2008, British Broadcasting Corporation\n"
        " * All Rights Reserved.\n"
        " *\n"
        " * Author: Philip de Nier\n"
        " *\n"
        " * Redistribution and use in source and binary forms, with or without\n"
        " * modification, are permitted provided that the following conditions are met:\n"
        " *\n"
        " *     * Redistributions of source code must retain the above copyright notice,\n"
        " *       this list of conditions and the following disclaimer.\n"
        " *     * Redistributions in binary form must reproduce the above copyright\n"
        " *       notice, this list of conditions and the following disclaimer in the\n"
        " *       documentation and/or other materials provided with the distribution.\n"
        " *     * Neither the name of the British Broadcasting Corporation nor the names\n"
        " *       of its contributors may be used to endorse or promote products derived\n"
        " *       from this software without specific prior written permission.\n"
        " *\n"
        " * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
        " * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        " * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
        " * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
        " * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
        " * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
        " * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
        " * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
        " * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
        " * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
        " * POSSIBILITY OF SUCH DAMAGE.\n"
        " */\n"
        "\n"
        "#ifndef MXFPP_%s_H_\n"
        "#define MXFPP_%s_H_\n"
        "\n"
        "\n"
        "\n"
        "#include <libMXF++/metadata/base/%s.h>\n"
        "\n"
        "\n"
        "namespace mxfpp\n"
        "{\n"
        "\n"
        "\n"
        "class %s : public %s\n"
        "{\n"
        "public:\n"
        "    friend class MetadataSetFactory<%s>;\n"
        "\n"
        "public:\n"
        "    %s(HeaderMetadata *headerMetadata);\n"
        "    virtual ~%s();\n"
        "\n"
        "\n",
        classNameU,
        classNameU,
        baseClassName,
        className, baseClassName,
        className,
        className,
        className);


    /* source */

    fprintf(baseSourceFile,
        "/*\n"
        " * Copyright (C) 2008, British Broadcasting Corporation\n"
        " * All Rights Reserved.\n"
        " *\n"
        " * Author: Philip de Nier\n"
        " *\n"
        " * Redistribution and use in source and binary forms, with or without\n"
        " * modification, are permitted provided that the following conditions are met:\n"
        " *\n"
        " *     * Redistributions of source code must retain the above copyright notice,\n"
        " *       this list of conditions and the following disclaimer.\n"
        " *     * Redistributions in binary form must reproduce the above copyright\n"
        " *       notice, this list of conditions and the following disclaimer in the\n"
        " *       documentation and/or other materials provided with the distribution.\n"
        " *     * Neither the name of the British Broadcasting Corporation nor the names\n"
        " *       of its contributors may be used to endorse or promote products derived\n"
        " *       from this software without specific prior written permission.\n"
        " *\n"
        " * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
        " * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        " * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
        " * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
        " * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
        " * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
        " * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
        " * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
        " * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
        " * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
        " * POSSIBILITY OF SUCH DAMAGE.\n"
        " */\n"
        "\n"
        "#ifdef HAVE_CONFIG_H\n"
        "#include \"config.h\"\n"
        "#endif\n"
        "\n"
        "#include <memory>\n"
        "\n"
        "#include <libMXF++/MXF.h>\n"
        "\n"
        "\n"
        "using namespace std;\n"
        "using namespace mxfpp;\n"
        "\n"
        "\n"
        "const mxfKey %s::setKey = MXF_SET_K(%s);\n"
        "\n"
        "\n"
        "%s::%s(HeaderMetadata *headerMetadata)\n"
        ": %s(headerMetadata, headerMetadata->createCSet(&setKey))\n"
        "{\n"
        "    headerMetadata->add(this);\n"
        "}\n"
        "\n"
        "%s::%s(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)\n"
        ": %s(headerMetadata, cMetadataSet)\n"
        "{}\n"
        "\n"
        "%s::~%s()\n"
        "{}\n"
        "\n"
        "\n",
        baseClassName, className,
        baseClassName, baseClassName,
        parentClassName,
        baseClassName, baseClassName,
        parentClassName,
        baseClassName, baseClassName
        );


    fprintf(sourceFile,
        "/*\n"
        " * Copyright (C) 2008, British Broadcasting Corporation\n"
        " * All Rights Reserved.\n"
        " *\n"
        " * Author: Philip de Nier\n"
        " *\n"
        " * Redistribution and use in source and binary forms, with or without\n"
        " * modification, are permitted provided that the following conditions are met:\n"
        " *\n"
        " *     * Redistributions of source code must retain the above copyright notice,\n"
        " *       this list of conditions and the following disclaimer.\n"
        " *     * Redistributions in binary form must reproduce the above copyright\n"
        " *       notice, this list of conditions and the following disclaimer in the\n"
        " *       documentation and/or other materials provided with the distribution.\n"
        " *     * Neither the name of the British Broadcasting Corporation nor the names\n"
        " *       of its contributors may be used to endorse or promote products derived\n"
        " *       from this software without specific prior written permission.\n"
        " *\n"
        " * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
        " * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        " * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
        " * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
        " * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
        " * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
        " * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
        " * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
        " * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
        " * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
        " * POSSIBILITY OF SUCH DAMAGE.\n"
        " */\n"
        "\n"
        "#ifdef HAVE_CONFIG_H\n"
        "#include \"config.h\"\n"
        "#endif\n"
        "\n"
        "#include <libMXF++/MXF.h>\n"
        "\n"
        "\n"
        "using namespace std;\n"
        "using namespace mxfpp;\n"
        "\n"
        "\n"
        "\n"
        "%s::%s(HeaderMetadata *headerMetadata)\n"
        ": %s(headerMetadata)\n"
        "{}\n"
        "\n"
        "%s::%s(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)\n"
        ": %s(headerMetadata, cMetadataSet)\n"
        "{}\n"
        "\n"
        "%s::~%s()\n"
        "{}\n"
        "\n"
        "\n",
        className, className,
        baseClassName,
        className, className,
        baseClassName,
        className, className
        );




    /* getters and havers */

    if (mxf_get_list_length(&setDef->itemDefs) > 0)
    {
        fprintf(baseHeaderFile,
            "   // getters\n"
            "\n");
    }

    mxf_initialise_list_iter(&itemIter, &setDef->itemDefs);
    while (mxf_next_list_iter_element(&itemIter))
    {
        MXFItemDef *itemDef = (MXFItemDef*)mxf_get_iter_element(&itemIter);
        MXFItemType *itemType = mxf_get_item_def_type(dataModel, itemDef->typeId);
        CHECK(itemType != NULL);

        strcpy(itemName, itemDef->name);
        itemName[0] = toupper(itemName[0]);

        if (itemType->category == MXF_BASIC_TYPE_CAT ||
            itemType->category == MXF_INTERPRET_TYPE_CAT ||
            itemType->category == MXF_COMPOUND_TYPE_CAT)
        {
            get_type_name(dataModel, itemDef, itemType, typeName);
        }
        else
        {
            if (itemType->typeId == MXF_UTF16STRING_TYPE || itemType->typeId == MXF_UTF8STRING_TYPE)
            {
                strcpy(typeName, "std::string");
            }
            else
            {
                MXFItemType *elementType = mxf_get_item_def_type(dataModel, itemType->info.array.elementTypeId);
                CHECK(elementType != NULL);
                get_type_name(dataModel, itemDef, elementType, elementTypeName);
                strcpy(typeName, "std::vector<");
                strcat(typeName, elementTypeName);
                strcat(typeName, ">");
            }
        }

        if (!itemDef->isRequired)
        {
            fprintf(baseHeaderFile,
                "   bool have%s() const;\n",
                itemName);
        }

        fprintf(baseHeaderFile,
            "   %s get%s() const;\n",
            typeName, itemName);


        if (!itemDef->isRequired)
        {
            fprintf(baseSourceFile,
                "bool %s::have%s() const\n"
                "{\n"
                "    return haveItem(&MXF_ITEM_K(%s, %s));\n"
                "}\n"
                "\n",
                baseClassName, itemName,
                className, itemName);
        }

        fprintf(baseSourceFile,
            "%s %s::get%s() const\n"
            "{\n",
            typeName, baseClassName, itemName);


        if (itemType->category == MXF_BASIC_TYPE_CAT)
        {
            switch (itemType->typeId)
            {
                case MXF_INT8_TYPE:
                    fprintf(baseSourceFile, "    return getInt8Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_INT16_TYPE:
                    fprintf(baseSourceFile, "    return getInt16Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_INT32_TYPE:
                    fprintf(baseSourceFile, "    return getInt32Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_INT64_TYPE:
                    fprintf(baseSourceFile, "    return getInt64Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UINT8_TYPE:
                    fprintf(baseSourceFile, "    return getUInt8Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UINT16_TYPE:
                    fprintf(baseSourceFile, "    return getUInt16Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UINT32_TYPE:
                    fprintf(baseSourceFile, "    return getUInt32Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UINT64_TYPE:
                    fprintf(baseSourceFile, "    return getUInt64Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_RAW_TYPE:
                    fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                default:
                    fprintf(stderr, "Warning: unknown basic type %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                    break;
            }
        }
        else if (itemType->category == MXF_INTERPRET_TYPE_CAT)
        {
            switch (itemType->typeId)
            {
                case MXF_VERSIONTYPE_TYPE:
                    fprintf(baseSourceFile, "    return getVersionTypeItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_BOOLEAN_TYPE:
                    fprintf(baseSourceFile, "    return getBooleanItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
               case MXF_LENGTH_TYPE:
                    fprintf(baseSourceFile, "    return getLengthItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_POSITION_TYPE:
                    fprintf(baseSourceFile, "    return getPositionItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_RGBACODE_TYPE:
                    fprintf(baseSourceFile, "    return getUInt8Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_STREAM_TYPE:
                    fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_DATAVALUE_TYPE:
                    fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_IDENTIFIER_TYPE:
                    fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_OPAQUE_TYPE:
                    fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UMID_TYPE:
                    fprintf(baseSourceFile, "    return getUMIDItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UID_TYPE:
                    fprintf(baseSourceFile, "    return getUIDItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UL_TYPE:
                    fprintf(baseSourceFile, "    return getULItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_UUID_TYPE:
                    fprintf(baseSourceFile, "    return getUUIDItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_AUID_TYPE:
                    fprintf(baseSourceFile, "    return getAUIDItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_PACKAGEID_TYPE:
                    fprintf(baseSourceFile, "    return getUMIDItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_STRONGREF_TYPE:
                    c = typeName[strlen(typeName) - 1];
                    typeName[strlen(typeName) - 1] = '\0';
                    fprintf(baseSourceFile,
                        "    unique_ptr<MetadataSet> obj(getStrongRefItem(&MXF_ITEM_K(%s, %s)));\n"
                        "    MXFPP_CHECK(dynamic_cast<%s*>(obj.get()) != 0);\n"
                        "    return dynamic_cast<%s*>(obj.release());\n",
                        className, itemName, typeName, typeName);
                    typeName[strlen(typeName) - 1] = c;
                    break;
                case MXF_WEAKREF_TYPE:
                    c = typeName[strlen(typeName) - 1];
                    typeName[strlen(typeName) - 1] = '\0';
                    fprintf(baseSourceFile,
                        "    unique_ptr<MetadataSet> obj(getWeakRefItem(&MXF_ITEM_K(%s, %s)));\n"
                        "    MXFPP_CHECK(dynamic_cast<%s*>(obj.get()) != 0);\n"
                        "    return dynamic_cast<%s*>(obj.release());\n",
                        className, itemName, typeName, typeName);
                    typeName[strlen(typeName) - 1] = c;
                    break;
                case MXF_ORIENTATION_TYPE:
                    fprintf(baseSourceFile, "    return getUInt8Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_CODED_CONTENT_TYPE_TYPE:
                    fprintf(baseSourceFile, "    return getUInt8Item(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                default:
                    fprintf(stderr, "Warning: unknown interpret type %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                    break;
            }
        }
        else if (itemType->category == MXF_COMPOUND_TYPE_CAT)
        {
            switch (itemType->typeId)
            {
                case MXF_RATIONAL_TYPE:
                    fprintf(baseSourceFile, "    return getRationalItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_TIMESTAMP_TYPE:
                    fprintf(baseSourceFile, "    return getTimestampItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_PRODUCTVERSION_TYPE:
                    fprintf(baseSourceFile, "    return getProductVersionItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                case MXF_INDIRECT_TYPE:
                    fprintf(baseSourceFile, "    return getRGBALayoutComponentItem(&MXF_ITEM_K(%s, %s));\n",
                        className, itemName);
                    break;
                default:
                    fprintf(stderr, "Warning: unknown compound type %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                    break;
            }
        }
        else
        {
            if (itemType->typeId == MXF_UTF16STRING_TYPE)
            {
                fprintf(baseSourceFile, "    return getStringItem(&MXF_ITEM_K(%s, %s));\n",
                    className, itemName);
            }
            else if (itemType->typeId == MXF_UTF8STRING_TYPE)
            {
                fprintf(baseSourceFile, "    return getUTF8StringItem(&MXF_ITEM_K(%s, %s));\n",
                    className, itemName);
            }
            else
            {
                MXFItemType *elementType = mxf_get_item_def_type(dataModel, itemType->info.array.elementTypeId);
                CHECK(elementType != NULL);
                get_type_name(dataModel, itemDef, elementType, elementTypeName);

                if (elementType->category == MXF_BASIC_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_INT8_TYPE:
                            fprintf(baseSourceFile, "    return getInt8ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_INT16_TYPE:
                            fprintf(baseSourceFile, "    return getInt16ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_INT32_TYPE:
                            fprintf(baseSourceFile, "    return getInt32ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_INT64_TYPE:
                            fprintf(baseSourceFile, "    return getInt64ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UINT8_TYPE:
                            fprintf(baseSourceFile, "    return getUInt8ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UINT16_TYPE:
                            fprintf(baseSourceFile, "    return getUInt16ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UINT32_TYPE:
                            fprintf(baseSourceFile, "    return getUInt32ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UINT64_TYPE:
                            fprintf(baseSourceFile, "    return getUInt64ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_RAW_TYPE:
                            fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown basic type array %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else if (elementType->category == MXF_INTERPRET_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_VERSIONTYPE_TYPE:
                            fprintf(baseSourceFile, "    return getVersionTypeArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_BOOLEAN_TYPE:
                            fprintf(baseSourceFile, "    return getBooleanArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_LENGTH_TYPE:
                            fprintf(baseSourceFile, "    return getLengthArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_POSITION_TYPE:
                            fprintf(baseSourceFile, "    return getPositionArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_RGBACODE_TYPE:
                            fprintf(baseSourceFile, "    return getUInt8ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_STREAM_TYPE:
                            fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_DATAVALUE_TYPE:
                            fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_IDENTIFIER_TYPE:
                            fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_OPAQUE_TYPE:
                            fprintf(baseSourceFile, "    return getRawBytesItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UMID_TYPE:
                            fprintf(baseSourceFile, "    return getUMIDArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UID_TYPE:
                            fprintf(baseSourceFile, "    return getUIDArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UL_TYPE:
                            fprintf(baseSourceFile, "    return getULArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_UUID_TYPE:
                            fprintf(baseSourceFile, "    return getUUIDArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_AUID_TYPE:
                            fprintf(baseSourceFile, "    return getAUIDArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_PACKAGEID_TYPE:
                            fprintf(baseSourceFile, "    return getUMIDArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_STRONGREF_TYPE:
                            c = elementTypeName[strlen(elementTypeName) - 1];
                            elementTypeName[strlen(elementTypeName) - 1] = '\0';
                            fprintf(baseSourceFile,
                                "    vector<%s*> result;\n"
                                "    unique_ptr<ObjectIterator> iter(getStrongRefArrayItem(&MXF_ITEM_K(%s, %s)));\n"
                                "    while (iter->next())\n"
                                "    {\n"
                                "        MXFPP_CHECK(dynamic_cast<%s*>(iter->get()) != 0);\n"
                                "        result.push_back(dynamic_cast<%s*>(iter->get()));\n"
                                "    }\n"
                                "    return result;\n",
                                elementTypeName, className, itemName, elementTypeName, elementTypeName);
                            elementTypeName[strlen(elementTypeName) - 1] = c;
                            break;
                        case MXF_WEAKREF_TYPE:
                            c = elementTypeName[strlen(elementTypeName) - 1];
                            elementTypeName[strlen(elementTypeName) - 1] = '\0';
                            fprintf(baseSourceFile,
                                "    vector<%s*> result;\n"
                                "    unique_ptr<ObjectIterator> iter(getWeakRefArrayItem(&MXF_ITEM_K(%s, %s)));\n"
                                "    while (iter->next())\n"
                                "    {\n"
                                "        MXFPP_CHECK(dynamic_cast<%s*>(iter->get()) != 0);\n"
                                "        result.push_back(dynamic_cast<%s*>(iter->get()));\n"
                                "    }\n"
                                "    return result;\n",
                                elementTypeName, className, itemName, elementTypeName, elementTypeName);
                            elementTypeName[strlen(elementTypeName) - 1] = c;
                            break;
                        case MXF_ORIENTATION_TYPE:
                            fprintf(baseSourceFile, "    return getUInt8ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_CODED_CONTENT_TYPE_TYPE:
                            fprintf(baseSourceFile, "    return getUInt8ArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown interpret type array %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else if (elementType->category == MXF_COMPOUND_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_RATIONAL_TYPE:
                            fprintf(baseSourceFile, "    return getRationalArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_TIMESTAMP_TYPE:
                            fprintf(baseSourceFile, "    return getTimestampArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_PRODUCTVERSION_TYPE:
                            fprintf(baseSourceFile, "    return getProductVersionArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        case MXF_RGBALAYOUTCOMPONENT_TYPE:
                            fprintf(baseSourceFile, "    return getRGBALayoutComponentArrayItem(&MXF_ITEM_K(%s, %s));\n",
                                className, itemName);
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown compound type %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: invalid element type for array/batch: %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                }
            }
        }

        fprintf(baseSourceFile,
            "}\n"
            "\n");

    }


    /* header setters */

    if (mxf_get_list_length(&setDef->itemDefs) > 0)
    {
        fprintf(baseHeaderFile,
            "\n"
            "\n"
            "   // setters\n"
            "\n");
    }

    mxf_initialise_list_iter(&itemIter, &setDef->itemDefs);
    while (mxf_next_list_iter_element(&itemIter))
    {
        MXFItemDef *itemDef = (MXFItemDef*)mxf_get_iter_element(&itemIter);
        MXFItemType *itemType = mxf_get_item_def_type(dataModel, itemDef->typeId);
        CHECK(itemType != NULL);

        strcpy(itemName, itemDef->name);
        itemName[0] = toupper(itemName[0]);

        if (itemType->category == MXF_BASIC_TYPE_CAT ||
            itemType->category == MXF_INTERPRET_TYPE_CAT ||
            itemType->category == MXF_COMPOUND_TYPE_CAT)
        {
            get_type_name(dataModel, itemDef, itemType, typeName);
        }
        else
        {
            if (itemType->typeId == MXF_UTF16STRING_TYPE || itemType->typeId == MXF_UTF8STRING_TYPE)
            {
                strcpy(typeName, "std::string");
            }
            else
            {
                MXFItemType *elementType = mxf_get_item_def_type(dataModel, itemType->info.array.elementTypeId);
                CHECK(elementType != NULL);
                get_type_name(dataModel, itemDef, elementType, elementTypeName);
                strcpy(typeName, "const std::vector<");
                strcat(typeName, elementTypeName);
                strcat(typeName, ">&");
            }
        }

        fprintf(baseHeaderFile,
            "   void set%s(%s value);\n",
            itemName, typeName);

        fprintf(baseSourceFile,
            "void %s::set%s(%s value)\n"
            "{\n",
            baseClassName, itemName, typeName);

        if (itemType->category == MXF_BASIC_TYPE_CAT)
        {
            switch (itemType->typeId)
            {
                case MXF_INT8_TYPE:
                    fprintf(baseSourceFile, "    setInt8Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_INT16_TYPE:
                    fprintf(baseSourceFile, "    setInt16Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_INT32_TYPE:
                    fprintf(baseSourceFile, "    setInt32Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_INT64_TYPE:
                    fprintf(baseSourceFile, "    setInt64Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UINT8_TYPE:
                    fprintf(baseSourceFile, "    setUInt8Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UINT16_TYPE:
                    fprintf(baseSourceFile, "    setUInt16Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UINT32_TYPE:
                    fprintf(baseSourceFile, "    setUInt32Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UINT64_TYPE:
                    fprintf(baseSourceFile, "    setUInt64Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_RAW_TYPE:
                    fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                default:
                    fprintf(stderr, "Warning: unknown basic type %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                    break;
            }
        }
        else if (itemType->category == MXF_INTERPRET_TYPE_CAT)
        {
            switch (itemType->typeId)
            {
                case MXF_VERSIONTYPE_TYPE:
                    fprintf(baseSourceFile, "    setVersionTypeItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_BOOLEAN_TYPE:
                    fprintf(baseSourceFile, "    setBooleanItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_LENGTH_TYPE:
                    fprintf(baseSourceFile, "    setLengthItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_POSITION_TYPE:
                    fprintf(baseSourceFile, "    setPositionItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_RGBACODE_TYPE:
                    fprintf(baseSourceFile, "    setUInt8Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_STREAM_TYPE:
                    fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_DATAVALUE_TYPE:
                    fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_IDENTIFIER_TYPE:
                    fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_OPAQUE_TYPE:
                    fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UMID_TYPE:
                    fprintf(baseSourceFile, "    setUMIDItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UID_TYPE:
                    fprintf(baseSourceFile, "    setUIDItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UL_TYPE:
                    fprintf(baseSourceFile, "    setULItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_UUID_TYPE:
                    fprintf(baseSourceFile, "    setUUIDItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_AUID_TYPE:
                    fprintf(baseSourceFile, "    setAUIDItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_PACKAGEID_TYPE:
                    fprintf(baseSourceFile, "    setUMIDItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_STRONGREF_TYPE:
                    fprintf(baseSourceFile, "    setStrongRefItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_WEAKREF_TYPE:
                    fprintf(baseSourceFile, "    setWeakRefItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_ORIENTATION_TYPE:
                    fprintf(baseSourceFile, "    setUInt8Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_CODED_CONTENT_TYPE_TYPE:
                    fprintf(baseSourceFile, "    setUInt8Item(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                default:
                    fprintf(stderr, "Warning: unknown interpret type %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                    break;
            }
        }
        else if (itemType->category == MXF_COMPOUND_TYPE_CAT)
        {
            switch (itemType->typeId)
            {
                case MXF_RATIONAL_TYPE:
                    fprintf(baseSourceFile, "    setRationalItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_TIMESTAMP_TYPE:
                    fprintf(baseSourceFile, "    setTimestampItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_PRODUCTVERSION_TYPE:
                    fprintf(baseSourceFile, "    setProductVersionItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                case MXF_RGBALAYOUTCOMPONENT_TYPE:
                    fprintf(baseSourceFile, "    setRGBALayoutComponentItem(&MXF_ITEM_K(%s, %s), value);\n",
                        className, itemName);
                    break;
                default:
                    fprintf(stderr, "Warning: unknown compound type %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                    break;
            }
        }
        else
        {
            if (itemType->typeId == MXF_UTF16STRING_TYPE)
            {
                fprintf(baseSourceFile, "    setStringItem(&MXF_ITEM_K(%s, %s), value);\n",
                    className, itemName);
            }
            else if (itemType->typeId == MXF_UTF8STRING_TYPE)
            {
                fprintf(baseSourceFile, "    setUTF8StringItem(&MXF_ITEM_K(%s, %s), value);\n",
                    className, itemName);
            }
            else
            {
                MXFItemType *elementType = mxf_get_item_def_type(dataModel, itemType->info.array.elementTypeId);
                CHECK(elementType != NULL);

                if (elementType->category == MXF_BASIC_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_INT8_TYPE:
                            fprintf(baseSourceFile, "    setInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_INT16_TYPE:
                            fprintf(baseSourceFile, "    setInt16ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_INT32_TYPE:
                            fprintf(baseSourceFile, "    setInt32ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_INT64_TYPE:
                            fprintf(baseSourceFile, "    setInt64ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UINT8_TYPE:
                            fprintf(baseSourceFile, "    setUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UINT16_TYPE:
                            fprintf(baseSourceFile, "    setUInt16ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UINT32_TYPE:
                            fprintf(baseSourceFile, "    setUInt32ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UINT64_TYPE:
                            fprintf(baseSourceFile, "    setUInt64ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_RAW_TYPE:
                            fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown basic type array %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else if (elementType->category == MXF_INTERPRET_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_VERSIONTYPE_TYPE:
                            fprintf(baseSourceFile, "    setVersionTypeArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_BOOLEAN_TYPE:
                            fprintf(baseSourceFile, "    setBooleanArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_LENGTH_TYPE:
                            fprintf(baseSourceFile, "    setLengthArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_POSITION_TYPE:
                            fprintf(baseSourceFile, "    setPositionArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_RGBACODE_TYPE:
                            fprintf(baseSourceFile, "    setUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_STREAM_TYPE:
                            fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_DATAVALUE_TYPE:
                            fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_IDENTIFIER_TYPE:
                            fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_OPAQUE_TYPE:
                            fprintf(baseSourceFile, "    setRawBytesItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UMID_TYPE:
                            fprintf(baseSourceFile, "    setUMIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UID_TYPE:
                            fprintf(baseSourceFile, "    setUIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UL_TYPE:
                            fprintf(baseSourceFile, "    setULArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_UUID_TYPE:
                            fprintf(baseSourceFile, "    setUUIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_AUID_TYPE:
                            fprintf(baseSourceFile, "    setAUIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_PACKAGEID_TYPE:
                            fprintf(baseSourceFile, "    setUMIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_STRONGREF_TYPE:
                            c = elementTypeName[strlen(elementTypeName) - 1];
                            elementTypeName[strlen(elementTypeName) - 1] = '\0';
                            fprintf(baseSourceFile,
                                "    WrapObjectVectorIterator<%s> iter(value);\n"
                                "    setStrongRefArrayItem(&MXF_ITEM_K(%s, %s), &iter);\n",
                                elementTypeName, className, itemName);
                            elementTypeName[strlen(elementTypeName) - 1] = c;
                            break;
                        case MXF_WEAKREF_TYPE:
                            c = elementTypeName[strlen(elementTypeName) - 1];
                            elementTypeName[strlen(elementTypeName) - 1] = '\0';
                            fprintf(baseSourceFile,
                                "    WrapObjectVectorIterator<%s> iter(value);\n"
                                "    setWeakRefArrayItem(&MXF_ITEM_K(%s, %s), &iter);\n",
                                elementTypeName, className, itemName);
                            elementTypeName[strlen(elementTypeName) - 1] = c;
                            break;
                        case MXF_ORIENTATION_TYPE:
                            fprintf(baseSourceFile, "    setUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_CODED_CONTENT_TYPE_TYPE:
                            fprintf(baseSourceFile, "    setUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown interpret type array %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else if (elementType->category == MXF_COMPOUND_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_RATIONAL_TYPE:
                            fprintf(baseSourceFile, "    setRationalArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_TIMESTAMP_TYPE:
                            fprintf(baseSourceFile, "    setTimestampArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_PRODUCTVERSION_TYPE:
                            fprintf(baseSourceFile, "    setProductVersionArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        case MXF_RGBALAYOUTCOMPONENT_TYPE:
                            fprintf(baseSourceFile, "    setRGBALayoutComponentArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown compound type %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: invalid element type for array/batch: %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                }
            }
        }

        fprintf(baseSourceFile,
            "}\n"
            "\n");


        if (itemType->category == MXF_ARRAY_TYPE_CAT)
        {
            MXFItemType *elementType = mxf_get_item_def_type(dataModel, itemType->info.array.elementTypeId);
            CHECK(elementType != NULL);
            get_type_name(dataModel, itemDef, elementType, elementTypeName);

            if (itemType->typeId == MXF_UTF16STRING_TYPE || itemType->typeId == MXF_UTF8STRING_TYPE)
            {
                // do nothing
            }
            else
            {
                MXFItemType *elementType = mxf_get_item_def_type(dataModel, itemType->info.array.elementTypeId);
                CHECK(elementType != NULL);

                if (elementType->category == MXF_BASIC_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_INT8_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_INT16_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendInt16ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_INT32_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendInt32ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_INT64_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendInt64ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_UINT8_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_UINT16_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUInt16ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_UINT32_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUInt32ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_UINT64_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUInt64ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_RAW_TYPE:
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown basic type array %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else if (elementType->category == MXF_INTERPRET_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_VERSIONTYPE_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendVersionTypeArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_BOOLEAN_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendBooleanArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_LENGTH_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendLengthArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_POSITION_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendPositionArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_RGBACODE_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_STREAM_TYPE:
                            break;
                        case MXF_DATAVALUE_TYPE:
                            break;
                        case MXF_IDENTIFIER_TYPE:
                            break;
                        case MXF_OPAQUE_TYPE:
                            break;
                        case MXF_UMID_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUMIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_UID_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_UL_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendULArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_UUID_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUUIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_AUID_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendAUIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_PACKAGEID_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUMIDArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_STRONGREF_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendStrongRefArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_WEAKREF_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendWeakRefArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_ORIENTATION_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_CODED_CONTENT_TYPE_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendUInt8ArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown interpret type array %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else if (elementType->category == MXF_COMPOUND_TYPE_CAT)
                {
                    switch (elementType->typeId)
                    {
                        case MXF_RATIONAL_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendRationalArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_TIMESTAMP_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendTimestampArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_PRODUCTVERSION_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendProductVersionArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        case MXF_RGBALAYOUTCOMPONENT_TYPE:
                            fprintf(baseHeaderFile,
                                "   void append%s(%s value);\n",
                                itemName, elementTypeName);
                            fprintf(baseSourceFile,
                                "void %s::append%s(%s value)\n"
                                "{\n",
                                baseClassName, itemName, elementTypeName);
                            fprintf(baseSourceFile, "    appendRGBALayoutComponentArrayItem(&MXF_ITEM_K(%s, %s), value);\n",
                                className, itemName);
                            fprintf(baseSourceFile,
                                "}\n"
                                "\n");
                            break;
                        default:
                            fprintf(stderr, "Warning: unknown compound type %d\n", itemType->typeId);
                            fprintf(baseSourceFile, "    XXX;\n");
                            break;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: invalid element type for array/batch: %d\n", itemType->typeId);
                    fprintf(baseSourceFile, "    XXX;\n");
                }

            }
        }

    }


    /* header footer */

    fprintf(baseHeaderFile,
        "\n"
        "\n"
        "protected:\n"
        "    %s(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);\n"
        "};\n"
        "\n"
        "\n"
        "};\n"
        "\n"
        "\n"
        "#endif\n",
        baseClassName);

    fprintf(headerFile,
        "\n"
        "\n"
        "protected:\n"
        "    %s(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);\n"
        "};\n"
        "\n"
        "\n"
        "};\n"
        "\n"
        "\n"
        "#endif\n",
        className);



    fclose(baseSourceFile);
    fclose(baseHeaderFile);
    fclose(sourceFile);
    fclose(headerFile);
}

static int process_set_defs(void *setDefIn, void *dataIn)
{
    MXFSetDef *setDef = (MXFSetDef*)setDefIn;
    SetDefsData *data = (SetDefsData*)dataIn;
    char className[256];

    if (mxf_equals_key(&setDef->key, &g_Null_Key))
    {
        /* root set */
        return 1;
    }

    strcpy(className, setDef->name);
    className[0] = toupper(className[0]);

    /* include */
    fprintf(data->baseHeaderFile,
        "#include <libMXF++/metadata/%s.h>\n",
        className);


    gen_class(data->directory, data->dataModel, setDef);

    printf("    REGISTER_CLASS(%s);\n", className);

    return 1;
}


static void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s <directory>\n", cmd);
}

int main(int argc, const char** argv)
{
    MXFDataModel *dataModel;
    char mkdirCmd[FILENAME_MAX];
    FILE *baseHeaderFile;
    char filename[FILENAME_MAX];
    const char *directory;
    SetDefsData setDefsData;

    if (argc != 2)
    {
        usage(argv[0]);
        exit(1);
    }

    directory = argv[1];


    CHECK(mxf_load_data_model(&dataModel));
    CHECK(mxf_finalise_data_model(dataModel));

    /* create the directories */

    strcpy(mkdirCmd, "mkdir -p ");
    strcat(mkdirCmd, directory);
    CHECK(system(mkdirCmd) == 0);

    strcat(mkdirCmd, "/base");
    CHECK(system(mkdirCmd) == 0);


    /* open all header */

    strcpy(filename, directory);
    strcat(filename, "/");
    strcat(filename, "Metadata.h");
    if ((baseHeaderFile = fopen(filename, "wb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }


    /* header */

    fprintf(baseHeaderFile,
        "/*\n"
        " * Copyright (C) 2008, British Broadcasting Corporation\n"
        " * All Rights Reserved.\n"
        " *\n"
        " * Author: Philip de Nier\n"
        " *\n"
        " * Redistribution and use in source and binary forms, with or without\n"
        " * modification, are permitted provided that the following conditions are met:\n"
        " *\n"
        " *     * Redistributions of source code must retain the above copyright notice,\n"
        " *       this list of conditions and the following disclaimer.\n"
        " *     * Redistributions in binary form must reproduce the above copyright\n"
        " *       notice, this list of conditions and the following disclaimer in the\n"
        " *       documentation and/or other materials provided with the distribution.\n"
        " *     * Neither the name of the British Broadcasting Corporation nor the names\n"
        " *       of its contributors may be used to endorse or promote products derived\n"
        " *       from this software without specific prior written permission.\n"
        " *\n"
        " * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
        " * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        " * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
        " * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
        " * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
        " * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
        " * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
        " * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
        " * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
        " * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
        " * POSSIBILITY OF SUCH DAMAGE.\n"
        " */\n"
        "\n"
        "#ifndef MXFPP_METADATA_H_\n"
        "#define MXFPP_METADATA_H_\n"
        "\n"
        "\n"
        "\n");

    setDefsData.baseHeaderFile = baseHeaderFile;
    setDefsData.directory = directory;
    setDefsData.dataModel = dataModel;
    mxf_tree_traverse(&dataModel->setDefs, process_set_defs, &setDefsData);


    /* footer */

    fprintf(baseHeaderFile,
        "\n"
        "\n"
        "#endif\n");

    fclose(baseHeaderFile);

    mxf_free_data_model(&dataModel);

    return 0;
}

