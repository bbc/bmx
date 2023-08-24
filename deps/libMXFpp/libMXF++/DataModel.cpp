/*
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

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace mxfpp;



ItemType::ItemType(::MXFItemType *cItemType)
: _cItemType(cItemType)
{}

ItemType::~ItemType()
{}

ItemDef::ItemDef(::MXFItemDef *cItemDef)
: _cItemDef(cItemDef)
{}

ItemDef::~ItemDef()
{}

SetDef::SetDef(::MXFSetDef *cSetDef)
: _cSetDef(cSetDef)
{}

SetDef::~SetDef()
{}

bool SetDef::findItemDef(const mxfKey *key, ItemDef **itemDef)
{
    ::MXFItemDef *cItemDef;
    if (mxf_find_item_def_in_set_def(key, _cSetDef, &cItemDef))
    {
        *itemDef = new ItemDef(cItemDef);
        return true;
    }
    return false;
}




DataModel::DataModel()
{
    MXFPP_CHECK(mxf_load_data_model(&_cDataModel));
    MXFPP_CHECK(mxf_finalise_data_model(_cDataModel));
    _ownCDataModel = true;
}

DataModel::DataModel(::MXFDataModel *c_data_model, bool take_ownership)
{
    _cDataModel = c_data_model;
    _ownCDataModel = take_ownership;
}

DataModel::~DataModel()
{
    if (_ownCDataModel)
    {
        mxf_free_data_model(&_cDataModel);
    }
}

void DataModel::finalise()
{
    MXFPP_CHECK(mxf_finalise_data_model(_cDataModel));
}

bool DataModel::check() const
{
    return mxf_check_data_model(_cDataModel) != 0;
}

void DataModel::registerSetDef(string name, const mxfKey *parentKey, const mxfKey *key)
{
    MXFPP_CHECK(mxf_register_set_def(_cDataModel, name.c_str(), parentKey, key));
}

void DataModel::registerItemDef(string name, const mxfKey *setKey,  const mxfKey *key,
                                mxfLocalTag tag, unsigned int typeId, bool isRequired)
{
    MXFPP_CHECK(mxf_register_item_def(_cDataModel, name.c_str(), setKey, key, tag, typeId, isRequired));
}

ItemType* DataModel::registerBasicType(string name, unsigned int typeId, unsigned int size)
{
    ::MXFItemType *cItemType = mxf_register_basic_type(_cDataModel, name.c_str(), typeId, size);
    MXFPP_CHECK(cItemType != 0);
    return new ItemType(cItemType);
}

ItemType* DataModel::registerArrayType(string name, unsigned int typeId, unsigned int elementTypeId,
                                       unsigned int fixedSize)
{
    ::MXFItemType *cItemType = mxf_register_array_type(_cDataModel, name.c_str(), typeId, elementTypeId, fixedSize);
    MXFPP_CHECK(cItemType != 0);
    return new ItemType(cItemType);
}

ItemType* DataModel::registerCompoundType(string name, unsigned int typeId)
{
    ::MXFItemType *cItemType = mxf_register_compound_type(_cDataModel, name.c_str(), typeId);
    MXFPP_CHECK(cItemType != 0);
    return new ItemType(cItemType);
}

void DataModel::registerCompoundTypeMember(ItemType *itemType, string memberName, unsigned int memberTypeId)
{
    MXFPP_CHECK(mxf_register_compound_type_member(itemType->getCItemType(), memberName.c_str(), memberTypeId));
}

ItemType* DataModel::registerInterpretType(string name, unsigned int typeId, unsigned int interpretedTypeId,
                                           unsigned int fixedArraySize)
{
    ::MXFItemType *cItemType = mxf_register_interpret_type(_cDataModel, name.c_str(), typeId, interpretedTypeId,
                                                           fixedArraySize);
    MXFPP_CHECK(cItemType != 0);
    return new ItemType(cItemType);
}

bool DataModel::findSetDef(const mxfKey *key, SetDef **setDef) const
{
    ::MXFSetDef *cSetDef;
    if (mxf_find_set_def(_cDataModel, key, &cSetDef))
    {
        *setDef = new SetDef(cSetDef);
        return true;
    }
    return false;
}

bool DataModel::findItemDef(const mxfKey *key, ItemDef **itemDef) const
{
    ::MXFItemDef *cItemDef;
    if (mxf_find_item_def(_cDataModel, key, &cItemDef))
    {
        *itemDef = new ItemDef(cItemDef);
        return true;
    }
    return false;
}

ItemType* DataModel::getItemDefType(unsigned int typeId) const
{
    ::MXFItemType *cItemType = mxf_get_item_def_type(_cDataModel, typeId);
    MXFPP_CHECK(cItemType != 0);
    return new ItemType(cItemType);
}

bool DataModel::isSubclassOf(const mxfKey *setKey, const mxfKey *parentSetKey) const
{
    return mxf_is_subclass_of(_cDataModel, setKey, parentSetKey) != 0;
}

bool DataModel::isSubclassOf(const MetadataSet *set, const mxfKey *parentSetKey) const
{
    return isSubclassOf(set->getKey(), parentSetKey);
}

