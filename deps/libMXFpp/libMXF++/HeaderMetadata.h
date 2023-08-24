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

#ifndef MXFPP_HEADERMETADATA_H_
#define MXFPP_HEADERMETADATA_H_

#include <map>

#include <libMXF++/File.h>
#include <libMXF++/DataModel.h>



namespace mxfpp
{


class Preface;
class HeaderMetadata;


class HeaderMetadata
{
public:
    friend class MetadataSet; // allow metadata sets to remove themselves from the _objectDirectory

public:
    static bool isHeaderMetadata(const mxfKey *key);

    HeaderMetadata(DataModel *dataModel);
    HeaderMetadata(::MXFHeaderMetadata *c_header_metadata, bool take_ownership);
    virtual ~HeaderMetadata();

    static mxfProductVersion getToolkitVersion();
    static std::string getPlatform();


    void enableGenerationUIDInit(mxfUUID generationUID);
    void disableGenerationUIDInit();


    void registerObjectFactory(const mxfKey *key, AbsMetadataSetFactory *factory);

    void registerPrimerEntry(const mxfUID *itemKey, mxfLocalTag newTag, mxfLocalTag *assignedTag);


    virtual void read(File *file, Partition *partition, const mxfKey *key, uint8_t llen, uint64_t len);

    virtual void write(File *file, Partition *partition, FillerWriter *filler);


    Preface* getPreface();


    DataModel* getDataModel() const { return _dataModel; }

    void add(MetadataSet *set);
    void moveToEnd(MetadataSet *set);

    MetadataSet* wrap(::MXFMetadataSet *cMetadataSet);
    ::MXFMetadataSet* createCSet(const mxfKey *key);
    MetadataSet* createAndWrap(const mxfKey *key);

    ::MXFHeaderMetadata* getCHeaderMetadata() const { return _cHeaderMetadata; }

private:
    void initialiseObjectFactory();
    void remove(MetadataSet *set);

    DataModel *_dataModel;

    std::map<mxfKey, AbsMetadataSetFactory*> _objectFactory;

    ::MXFHeaderMetadata* _cHeaderMetadata;
    bool _ownCHeaderMetadata;
    std::map<mxfUUID, MetadataSet*> _objectDirectory;
    bool _busyDestructing;

    bool _initGenerationUID;
    mxfUUID _generationUID;
};


};



#endif

