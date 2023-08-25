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

using namespace std;
using namespace mxfpp;



bool HeaderMetadata::isHeaderMetadata(const mxfKey *key)
{
    return mxf_is_header_metadata(key) != 0;
}

HeaderMetadata::HeaderMetadata(DataModel *dataModel)
{
    _initGenerationUID = false;
    _generationUID = g_Null_UUID;

    initialiseObjectFactory();
    MXFPP_CHECK(mxf_create_header_metadata(&_cHeaderMetadata, dataModel->getCDataModel()));
    _ownCHeaderMetadata = true;

    _dataModel = new DataModel(_cHeaderMetadata->dataModel, false);
}

HeaderMetadata::HeaderMetadata(::MXFHeaderMetadata *c_header_metadata, bool take_ownership)
{
    initialiseObjectFactory();
    _cHeaderMetadata = c_header_metadata;
    _ownCHeaderMetadata = take_ownership;

    _dataModel = new DataModel(_cHeaderMetadata->dataModel, false);
}

HeaderMetadata::~HeaderMetadata()
{
    map<mxfKey, AbsMetadataSetFactory*>::iterator iter1;
    for (iter1 = _objectFactory.begin(); iter1 != _objectFactory.end(); iter1++)
    {
        delete (*iter1).second;
    }

    map<mxfUUID, MetadataSet*>::iterator iter2;
    for (iter2 = _objectDirectory.begin(); iter2 != _objectDirectory.end(); iter2++)
    {
        (*iter2).second->_headerMetadata = 0; // break containment link
        delete (*iter2).second;
    }

    if (_ownCHeaderMetadata)
    {
        mxf_free_header_metadata(&_cHeaderMetadata);
    }

    delete _dataModel;
}

mxfProductVersion HeaderMetadata::getToolkitVersion()
{
    return *mxf_get_version();
}

string HeaderMetadata::getPlatform()
{
    return mxf_get_platform_string();
}


void HeaderMetadata::enableGenerationUIDInit(mxfUUID generationUID)
{
    _initGenerationUID = true;
    _generationUID = generationUID;
}

void HeaderMetadata::disableGenerationUIDInit()
{
    _initGenerationUID = false;
}

void HeaderMetadata::registerObjectFactory(const mxfKey *key, AbsMetadataSetFactory *factory)
{
    pair<map<mxfKey, AbsMetadataSetFactory*>::iterator, bool> result =
        _objectFactory.insert(pair<mxfKey, AbsMetadataSetFactory*>(*key, factory));
    if (result.second == false)
    {
        // replace existing factory with new one
        _objectFactory.erase(result.first);
        _objectFactory.insert(pair<mxfKey, AbsMetadataSetFactory*>(*key, factory));
    }
}

void HeaderMetadata::registerPrimerEntry(const mxfUID *itemKey, mxfLocalTag newTag, mxfLocalTag *assignedTag)
{
    MXFPP_CHECK(mxf_register_primer_entry(_cHeaderMetadata->primerPack, itemKey, newTag, assignedTag));
}

void HeaderMetadata::read(File *file, Partition *partition, const mxfKey *key, uint8_t llen, uint64_t len)
{
    MXFPP_CHECK(mxf_read_header_metadata(file->getCFile(), _cHeaderMetadata,
                                         partition->getCPartition()->headerByteCount, key, llen, len));
}

void HeaderMetadata::write(File *file, Partition *partition, FillerWriter *filler)
{
    partition->markHeaderStart(file);

    MXFPP_CHECK(mxf_write_header_primer_pack(file->getCFile(), _cHeaderMetadata));
    partition->fillToKag(file);
    MXFPP_CHECK(mxf_write_header_sets(file->getCFile(), _cHeaderMetadata));
    if (filler)
    {
        filler->write(file);
    }
    else
    {
        partition->fillToKag(file);
    }

    partition->markHeaderEnd(file);
}

Preface* HeaderMetadata::getPreface()
{
    ::MXFMetadataSet *cSet;
    if (!mxf_find_singular_set_by_key(_cHeaderMetadata, &MXF_SET_K(Preface), &cSet))
    {
        throw MXFException("Header metadata is missing a Preface object");
    }

    return dynamic_cast<Preface*>(wrap(cSet));
}

void HeaderMetadata::add(MetadataSet *set)
{
    _objectDirectory.insert(pair<mxfUUID, MetadataSet*>(set->getCMetadataSet()->instanceUID, set));
    if (_initGenerationUID)
    {
        InterchangeObjectBase *interchangeObject = dynamic_cast<InterchangeObjectBase*>(set);
        if (interchangeObject && !dynamic_cast<IdentificationBase*>(set))
        {
            interchangeObject->setGenerationUID(_generationUID);
        }
    }
}

void HeaderMetadata::moveToEnd(MetadataSet *set)
{
    if (set->getHeaderMetadata()) {
        MXFPP_CHECK(_cHeaderMetadata == set->getHeaderMetadata()->_cHeaderMetadata);
        MXFPP_CHECK(mxf_remove_set(_cHeaderMetadata, set->getCMetadataSet()));
    }
    MXFPP_CHECK(mxf_add_set(_cHeaderMetadata, set->getCMetadataSet()));
    _objectDirectory.insert(pair<mxfUUID, MetadataSet*>(set->getCMetadataSet()->instanceUID, set));
}

MetadataSet* HeaderMetadata::wrap(::MXFMetadataSet *cMetadataSet)
{
    MXFPP_CHECK(cMetadataSet->headerMetadata == _cHeaderMetadata);
    MXFPP_CHECK(!mxf_equals_uuid(&cMetadataSet->instanceUID, &g_Null_UUID));

    MetadataSet *set = 0;

    map<mxfUUID, MetadataSet*>::const_iterator objIter;
    objIter = _objectDirectory.find(cMetadataSet->instanceUID);
    if (objIter != _objectDirectory.end())
    {
        set = (*objIter).second;

        if (cMetadataSet != set->getCMetadataSet())
        {
            mxf_log(MXF_WLOG, "Metadata set with same instance UUID found when creating "
                "C++ object. Changing wrapped C metadata set.");
            set->_cMetadataSet = cMetadataSet;
        }
    }
    else
    {
        map<mxfKey, AbsMetadataSetFactory*>::iterator iter;

        ::MXFSetDef *setDef = 0;
        MXFPP_CHECK(mxf_find_set_def(_cHeaderMetadata->dataModel, &cMetadataSet->key, &setDef));

        while (setDef != 0)
        {
            iter = _objectFactory.find(setDef->key);
            if (iter != _objectFactory.end())
            {
                set = (*iter).second->create(this, cMetadataSet);
                break;
            }
            else
            {
                setDef = setDef->parentSetDef;
            }
        }

        if (set == 0)
        {
            // shouldn't be here if every class is a sub-class of interchange object
            // and libMXF ignores sets with unknown defs
            throw MXFException("Could not create C++ object for metadata set");
        }

        add(set);
    }

    return set;
}

::MXFMetadataSet* HeaderMetadata::createCSet(const mxfKey *key)
{
    ::MXFMetadataSet *cMetadataSet;

    MXFPP_CHECK(mxf_create_set(_cHeaderMetadata, key, &cMetadataSet));

    return cMetadataSet;
}

MetadataSet* HeaderMetadata::createAndWrap(const mxfKey *key)
{
    return wrap(createCSet(key));
}

void HeaderMetadata::initialiseObjectFactory()
{
#define REGISTER_CLASS(className) \
    _objectFactory.insert(pair<mxfKey, AbsMetadataSetFactory*>( \
        className::setKey, new MetadataSetFactory<className>()));


    REGISTER_CLASS(InterchangeObject);
    REGISTER_CLASS(Preface);
    REGISTER_CLASS(Identification);
    REGISTER_CLASS(ContentStorage);
    REGISTER_CLASS(EssenceContainerData);
    REGISTER_CLASS(GenericPackage);
    REGISTER_CLASS(Locator);
    REGISTER_CLASS(NetworkLocator);
    REGISTER_CLASS(TextLocator);
    REGISTER_CLASS(GenericTrack);
    REGISTER_CLASS(StaticTrack);
    REGISTER_CLASS(Track);
    REGISTER_CLASS(EventTrack);
    REGISTER_CLASS(StructuralComponent);
    REGISTER_CLASS(Sequence);
    REGISTER_CLASS(TimecodeComponent);
    REGISTER_CLASS(SourceClip);
    REGISTER_CLASS(DMSegment);
    REGISTER_CLASS(DMSourceClip);
    REGISTER_CLASS(MaterialPackage);
    REGISTER_CLASS(SourcePackage);
    REGISTER_CLASS(GenericDescriptor);
    REGISTER_CLASS(FileDescriptor);
    REGISTER_CLASS(GenericPictureEssenceDescriptor);
    REGISTER_CLASS(CDCIEssenceDescriptor);
    REGISTER_CLASS(MPEGVideoDescriptor);
    REGISTER_CLASS(RGBAEssenceDescriptor);
    REGISTER_CLASS(GenericSoundEssenceDescriptor);
    REGISTER_CLASS(GenericDataEssenceDescriptor);
    REGISTER_CLASS(MultipleDescriptor);
    REGISTER_CLASS(WaveAudioDescriptor);
    REGISTER_CLASS(AES3AudioDescriptor);
    REGISTER_CLASS(ANCDataDescriptor);
    REGISTER_CLASS(VBIDataDescriptor);
    REGISTER_CLASS(DMFramework);
    REGISTER_CLASS(DMSet);
    REGISTER_CLASS(SubDescriptor);
    REGISTER_CLASS(AVCSubDescriptor);
    REGISTER_CLASS(TextBasedObject);
    REGISTER_CLASS(TextBasedDMFramework);
    REGISTER_CLASS(GenericStreamTextBasedSet);
    REGISTER_CLASS(UTF8TextBasedSet);
    REGISTER_CLASS(UTF16TextBasedSet);
    REGISTER_CLASS(MCALabelSubDescriptor);
    REGISTER_CLASS(AudioChannelLabelSubDescriptor);
    REGISTER_CLASS(SoundfieldGroupLabelSubDescriptor);
    REGISTER_CLASS(GroupOfSoundfieldGroupsLabelSubDescriptor);
    REGISTER_CLASS(VC2SubDescriptor);
    REGISTER_CLASS(DCTimedTextDescriptor);
    REGISTER_CLASS(DCTimedTextResourceSubDescriptor);
    REGISTER_CLASS(JPEG2000SubDescriptor);
    REGISTER_CLASS(ContainerConstraintsSubDescriptor);

    // Add new classes here

}

void HeaderMetadata::remove(MetadataSet *set)
{
    map<mxfUUID, MetadataSet*>::iterator objIter;
    objIter = _objectDirectory.find(set->getCMetadataSet()->instanceUID);
    if (objIter != _objectDirectory.end())
    {
        _objectDirectory.erase(objIter);
    }
    // TODO: throw exception or log warning if set not in there?
}
