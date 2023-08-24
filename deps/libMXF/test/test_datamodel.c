/*
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mxf/mxf.h>
#include <mxf/mxf_macros.h>


#define EXT_DATA_MODEL \
MXF_SET_DEFINITION(InterchangeObject, TestSet1, \
    MXF_LABEL(0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) \
); \
\
    MXF_ITEM_DEFINITION(TestSet1, TestItem1, \
        MXF_LABEL(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), \
        0x7f00, \
        MXF_UINT8_TYPE, \
        0 \
    ); \
\
    MXF_ITEM_DEFINITION(TestSet1, TestItem2, \
        MXF_LABEL(0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), \
        0x0000, \
        MXF_UINT8_TYPE, \
        0 \
    ); \
\
MXF_SET_DEFINITION(MaterialPackage, TestSet2, \
    MXF_LABEL(0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) \
); \
\
MXF_SET_DEFINITION(TestSet1, TestSet3, \
    MXF_LABEL(0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) \
);


#define EXT_DATA_MODEL_2 \
MXF_SET_DEFINITION(TestSet1, TestSet4, \
    MXF_LABEL(0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) \
); \
\
    MXF_ITEM_DEFINITION(TestSet4, TestItem3, \
        MXF_LABEL(0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), \
        0x0000, \
        MXF_UINT8_TYPE, \
        0 \
    );

#define EXT_TYPES \
MXF_ARRAY_TYPE_DEF(0, "TimestampArray", MXF_TIMESTAMP_TYPE, 0); \
\
MXF_COMPOUND_TYPE_DEF(0, "Dimensions"); \
MXF_COMPOUND_TYPE_MEMBER("Width", MXF_UINT32_TYPE); \
MXF_COMPOUND_TYPE_MEMBER("Height", MXF_UINT32_TYPE); \
\
MXF_INTERPRETED_TYPE_DEF(0, "NewInt", MXF_UINT8_TYPE, 0);



#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;

#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

EXT_DATA_MODEL
EXT_DATA_MODEL_2

#undef MXF_LABEL
#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION


static int test_model(MXFDataModel *dataModel)
{
    MXFSetDef *setDef;
    MXFItemDef *itemDef;

    CHK_ORET(mxf_find_set_def(dataModel, &MXF_SET_K(SourcePackage), &setDef));
    CHK_ORET(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet1), &setDef));
    CHK_ORET(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet2), &setDef));
    CHK_ORET(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet3), &setDef));
    CHK_ORET(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet4), &setDef));

    CHK_ORET(mxf_find_item_def(dataModel, &MXF_ITEM_K(GenericPackage, PackageUID), &itemDef));
    CHK_ORET(mxf_find_item_def(dataModel, &MXF_ITEM_K(TestSet1, TestItem1), &itemDef));
    CHK_ORET(mxf_find_item_def(dataModel, &MXF_ITEM_K(TestSet1, TestItem2), &itemDef));
    CHK_ORET(mxf_find_item_def(dataModel, &MXF_ITEM_K(TestSet4, TestItem3), &itemDef));

    CHK_ORET(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet4), &setDef));
    CHK_ORET(mxf_find_item_def_in_set_def(&MXF_ITEM_K(TestSet1, TestItem1), setDef, &itemDef));

    CHK_ORET(mxf_is_subclass_of(dataModel, &MXF_SET_K(TestSet4), &MXF_SET_K(TestSet1)));

    return 1;
}


int test()
{
    MXFDataModel *dataModel = NULL;
    MXFDataModel *clonedDataModel = NULL;
    MXFSetDef *setDef;
    MXFSetDef *clonedSetDef;
    unsigned int extTypeIds[16];
    MXFItemType *type;
    int typeIndex = 0;
    int memberIndex = 0;

    CHK_ORET(mxf_load_data_model(&dataModel));


#define MXF_ARRAY_TYPE_DEF(id, name, elementTypeId, fixedSize) \
    CHK_OFAIL(type = mxf_register_array_type(dataModel, name, id, elementTypeId, fixedSize)); \
    extTypeIds[typeIndex++] = type->typeId;

#define MXF_COMPOUND_TYPE_DEF(id, name) \
    CHK_OFAIL(type = mxf_register_compound_type(dataModel, name, id)); \
    extTypeIds[typeIndex++] = type->typeId;

#define MXF_COMPOUND_TYPE_MEMBER(name, typeId) \
    CHK_OFAIL(mxf_register_compound_type_member(type, name, typeId));

#define MXF_INTERPRETED_TYPE_DEF(id, name, ptypeId, fixedSize) \
    CHK_OFAIL(type = mxf_register_interpret_type(dataModel, name, id, ptypeId, fixedSize)); \
    extTypeIds[typeIndex++] = type->typeId;

#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    CHK_OFAIL(mxf_register_set_def(dataModel, #name, &MXF_SET_K(parentName), &MXF_SET_K(name)));

#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    CHK_OFAIL(mxf_register_item_def(dataModel, #name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId, isRequired));


    EXT_TYPES
    EXT_DATA_MODEL
    EXT_DATA_MODEL_2

#undef MXF_BASIC_TYPE_DEF
#undef MXF_ARRAY_TYPE_DEF
#undef MXF_COMPOUND_TYPE_DEF
#undef MXF_COMPOUND_TYPE_MEMBER
#undef MXF_INTERPRETED_TYPE_DEF
#undef MXF_LABEL
#undef MXF_SET_DEFINITION
#undef MXF_ITEM_DEFINITION


    CHK_OFAIL(mxf_finalise_data_model(dataModel));
    CHK_OFAIL(mxf_check_data_model(dataModel));

    CHK_OFAIL(test_model(dataModel));


#define MXF_ARRAY_TYPE_DEF(pid, pname, pelementTypeId, pfixedSize) \
    type = mxf_get_item_def_type(dataModel, extTypeIds[typeIndex]); \
    CHK_ORET(type != NULL); \
    CHK_ORET(strcmp(type->name, pname) == 0); \
    CHK_ORET(type->info.array.elementTypeId == pelementTypeId); \
    CHK_ORET(type->info.array.fixedSize == pfixedSize); \
    typeIndex++;

#define MXF_COMPOUND_TYPE_DEF(pid, pname) \
    type = mxf_get_item_def_type(dataModel, extTypeIds[typeIndex]); \
    CHK_ORET(type != NULL); \
    CHK_ORET(strcmp(type->name, pname) == 0); \
    memberIndex = 0; \
    typeIndex++;

#define MXF_COMPOUND_TYPE_MEMBER(pname, ptypeId) \
    CHK_ORET(type->info.compound.members[memberIndex].typeId == ptypeId); \
    CHK_ORET(strcmp(type->info.compound.members[memberIndex].name, pname) == 0); \
    memberIndex++;

#define MXF_INTERPRETED_TYPE_DEF(pid, pname, ptypeId, pfixedSize) \
    type = mxf_get_item_def_type(dataModel, extTypeIds[typeIndex]); \
    CHK_ORET(type != NULL); \
    CHK_ORET(strcmp(type->name, pname) == 0); \
    CHK_ORET(type->info.interpret.typeId == ptypeId); \
    CHK_ORET(type->info.interpret.fixedArraySize == pfixedSize); \
    typeIndex++;


    typeIndex = 0;
    EXT_TYPES

#undef MXF_ARRAY_TYPE_DEF
#undef MXF_COMPOUND_TYPE_DEF
#undef MXF_COMPOUND_TYPE_MEMBER
#undef MXF_INTERPRETED_TYPE_DEF


    CHK_OFAIL(mxf_load_data_model(&clonedDataModel));
    CHK_OFAIL(mxf_finalise_data_model(clonedDataModel));
    CHK_OFAIL(mxf_check_data_model(clonedDataModel));

    CHK_OFAIL(mxf_find_set_def(dataModel, &MXF_SET_K(SourcePackage), &setDef));
    CHK_OFAIL(mxf_clone_set_def(dataModel, setDef, clonedDataModel, &clonedSetDef));
    CHK_OFAIL(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet1), &setDef));
    CHK_OFAIL(mxf_clone_set_def(dataModel, setDef, clonedDataModel, &clonedSetDef));
    CHK_OFAIL(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet2), &setDef));
    CHK_OFAIL(mxf_clone_set_def(dataModel, setDef, clonedDataModel, &clonedSetDef));
    CHK_OFAIL(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet3), &setDef));
    CHK_OFAIL(mxf_clone_set_def(dataModel, setDef, clonedDataModel, &clonedSetDef));
    CHK_OFAIL(mxf_find_set_def(dataModel, &MXF_SET_K(TestSet4), &setDef));
    CHK_OFAIL(mxf_clone_set_def(dataModel, setDef, clonedDataModel, &clonedSetDef));

    CHK_OFAIL(test_model(clonedDataModel));


    mxf_free_data_model(&dataModel);
    mxf_free_data_model(&clonedDataModel);
    return 1;

fail:
    mxf_free_data_model(&dataModel);
    mxf_free_data_model(&clonedDataModel);
    return 0;
}


void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s\n", cmd);
}

int main(int argc, const char *argv[])
{
    if (argc != 1)
    {
        usage(argv[0]);
        return 1;
    }

    if (!test())
    {
        return 1;
    }

    return 0;
}

