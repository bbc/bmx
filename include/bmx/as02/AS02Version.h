/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#ifndef BMX_AS02_VERSION_H_
#define BMX_AS02_VERSION_H_

#include <vector>
#include <set>

#include <libMXF++/MXF.h>

#include <bmx/as02/AS02Bundle.h>
#include <bmx/as02/AS02Clip.h>
#include <bmx/as02/AS02Manifest.h>



namespace bmx
{


class AS02Version : public AS02Clip
{
public:
    static AS02Version* OpenNewPrimary(AS02Bundle *bundle, mxfRational frame_rate);
    static AS02Version* OpenNew(AS02Bundle *bundle, std::string name, mxfRational frame_rate);

public:
    virtual ~AS02Version();

public:
    virtual void PrepareWrite();
    virtual void CompleteWrite();

private:
    AS02Version(AS02Bundle *bundle, std::string filepath, std::string rel_uri, mxfpp::File *mxf_file,
                mxfRational frame_rate);

    void CreateHeaderMetadata();
    void CreateFile();
    void UpdatePackageMetadata();

private:
    mxfpp::File *mMXFFile;
    AS02ManifestFile *mManifestFile;

    mxfUMID mMaterialPackageUID;

    mxfpp::DataModel *mDataModel;
    mxfpp::HeaderMetadata *mHeaderMetadata;
    int64_t mHeaderMetadataEndPos;

    std::set<mxfUL> mEssenceContainerULs;

    mxfpp::MaterialPackage *mMaterialPackage;

    std::vector<mxfpp::SourcePackage*> mFilePackages;
};


};



#endif

