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



Preface::Preface(HeaderMetadata *headerMetadata)
: PrefaceBase(headerMetadata)
{}

Preface::Preface(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: PrefaceBase(headerMetadata, cMetadataSet)
{}

Preface::~Preface()
{}

GenericPackage* Preface::findPackage(mxfUMID package_uid) const
{
    vector<GenericPackage*> packages = getContentStorage()->getPackages();
    mxfUMID uid;
    size_t i;
    for (i = 0; i < packages.size(); i++) {
        uid = packages[i]->getPackageUID();
        if (mxf_equals_umid(&package_uid, &uid))
            return packages[i];
    }

    return 0;
}

MaterialPackage* Preface::findMaterialPackage() const
{
    vector<GenericPackage*> packages = getContentStorage()->getPackages();
    MaterialPackage *material_package;
    size_t i;
    for (i = 0; i < packages.size(); i++) {
        material_package = dynamic_cast<MaterialPackage*>(packages[i]);
        if (material_package)
            return material_package;
    }

    return 0;
}

vector<SourcePackage*> Preface::findFileSourcePackages() const
{
    vector<GenericPackage*> packages = getContentStorage()->getPackages();
    vector<SourcePackage*> file_packages;
    SourcePackage *file_package;
    size_t i;
    for (i = 0; i < packages.size(); i++) {
        file_package = dynamic_cast<SourcePackage*>(packages[i]);
        if (!file_package ||
            !file_package->haveDescriptor() ||
            !dynamic_cast<FileDescriptor*>(file_package->getDescriptorLight()))
        {
            continue;
        }

        file_packages.push_back(file_package);
    }

    return file_packages;
}

vector<mxfUL> Preface::getDMSchemes(bool allow_missing) const
{
    if (!allow_missing || haveItem(&MXF_ITEM_K(Preface, DMSchemes))) {
        return getULArrayItem(&MXF_ITEM_K(Preface, DMSchemes));
    } else {
        mxf_log_warn("Missing Preface::DMSchemes property\n");
        return vector<mxfUL>();
    }
}

