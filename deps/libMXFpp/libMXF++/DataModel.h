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

#ifndef MXFPP_DATA_MODEL_H_
#define MXFPP_DATA_MODEL_H_



namespace mxfpp
{


class ItemType
{
public:
    ItemType(::MXFItemType *cItemType);
    ~ItemType();

    ::MXFItemType* getCItemType() const { return _cItemType; }

private:
    ::MXFItemType* _cItemType;
};


class ItemDef
{
public:
    ItemDef(::MXFItemDef *cItemDef);
    ~ItemDef();

    ::MXFItemDef* getCItemDef() const { return _cItemDef; }

private:
    ::MXFItemDef* _cItemDef;
};


class SetDef
{
public:
    SetDef(::MXFSetDef *cSetDef);
    ~SetDef();

    bool findItemDef(const mxfKey *key, ItemDef **itemDef);

    ::MXFSetDef* getCSetDef() const { return _cSetDef; }

private:
    ::MXFSetDef* _cSetDef;
};



class MetadataSet;

class DataModel
{
public:
    DataModel();
    DataModel(::MXFDataModel *c_data_model, bool take_ownership);
    ~DataModel();

    void finalise();
    bool check() const;

    void registerSetDef(std::string name, const mxfKey *parentKey, const mxfKey *key);
    void registerItemDef(std::string name, const mxfKey *setKey,  const mxfKey *key,
        mxfLocalTag tag, unsigned int typeId, bool isRequired);

    ItemType* registerBasicType(std::string name, unsigned int typeId, unsigned int size);
    ItemType* registerArrayType(std::string name, unsigned int typeId, unsigned int elementTypeId,
                                unsigned int fixedSize);
    ItemType* registerCompoundType(std::string name, unsigned int typeId);
    void registerCompoundTypeMember(ItemType *itemType, std::string memberName, unsigned int memberTypeId);
    ItemType* registerInterpretType(std::string name, unsigned int typeId, unsigned int interpretedTypeId,
                                    unsigned int fixedArraySize);

    bool findSetDef(const mxfKey *key, SetDef **setDef) const;
    bool findItemDef(const mxfKey *key, ItemDef **itemDef) const;

    ItemType* getItemDefType(unsigned int typeId) const;

    bool isSubclassOf(const mxfKey *setKey, const mxfKey *parentSetKey) const;
    bool isSubclassOf(const MetadataSet *set, const mxfKey *parentSetKey) const;


    ::MXFDataModel* getCDataModel() const { return _cDataModel; }

private:
    ::MXFDataModel* _cDataModel;
    bool _ownCDataModel;
};



};



#endif

