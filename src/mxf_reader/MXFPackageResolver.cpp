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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bmx/mxf_reader/MXFPackageResolver.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/MXFUtils.h>
#include <bmx/URI.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



ResolvedPackage::ResolvedPackage()
{
    package = 0;
    generic_track = 0;
    track_id = 0;
    file_reader = 0;
    is_file_source_package = false;
    external_essence = false;
}



DefaultMXFPackageResolver::DefaultMXFPackageResolver()
{
    mFileReader = 0;
}

DefaultMXFPackageResolver::~DefaultMXFPackageResolver()
{
    size_t i;
    for (i = 0; i < mExternalReaders.size(); i++)
        delete mExternalReaders[i];
}

void DefaultMXFPackageResolver::ExtractResolvedPackages(MXFFileReader *file_reader)
{
    if (!mFileReader)
        mFileReader = file_reader;


    ContentStorage *content_storage = file_reader->GetHeaderMetadata()->getPreface()->getContentStorage();

    vector<GenericPackage*> packages = content_storage->getPackages();
    vector<EssenceContainerData*> ess_data;
    if (content_storage->haveEssenceContainerData())
        ess_data = content_storage->getEssenceContainerData();

    size_t i;
    for (i = 0; i < packages.size(); i++) {
        ResolvedPackage resolved_package;
        resolved_package.package = packages[i];
        resolved_package.file_reader = file_reader;

        SourcePackage *source_package = dynamic_cast<SourcePackage*>(packages[i]);
        if (source_package &&
            source_package->haveDescriptor() && dynamic_cast<FileDescriptor*>(source_package->getDescriptor()))
        {
            resolved_package.is_file_source_package = true;

            // essence is internal if there exists an essence container linked with the file source package and
            // the body SID != 0
            resolved_package.external_essence = true;
            size_t j;
            for (j = 0; j < ess_data.size(); j++) {
                if (ess_data[j]->getLinkedPackageUID() == packages[i]->getPackageUID()) {
                    if (ess_data[j]->getBodySID() != 0)
                        resolved_package.external_essence = false;
                    break;
                }
            }
        }

        mResolvedPackages.push_back(resolved_package);
    }
}

vector<ResolvedPackage> DefaultMXFPackageResolver::ResolveSourceClip(SourceClip *source_clip)
{
    vector<ResolvedPackage> resolved_packages;

    size_t i;
    for (i = 0; i < mResolvedPackages.size(); i++) {
        if (mResolvedPackages[i].package->getPackageUID() == source_clip->getSourcePackageID()) {
            GenericTrack *generic_track = mResolvedPackages[i].package->findTrack(source_clip->getSourceTrackID());
            if (generic_track) {
                ResolvedPackage resolved_package = mResolvedPackages[i];
                resolved_package.generic_track = generic_track;
                resolved_package.track_id = source_clip->getSourceTrackID();

                resolved_packages.push_back(resolved_package);
            }
        }
    }

    return resolved_packages;
}

vector<ResolvedPackage> DefaultMXFPackageResolver::ResolveSourceClip(SourceClip *source_clip,
                                                                     vector<Locator*> &locators)
{
    // it is sufficient to return existing resolved packages if resolving a non-file source package or
    // the file has internal essence
    vector<ResolvedPackage> resolved_packages = ResolveSourceClip(source_clip);
    size_t i;
    for (i = 0; i < resolved_packages.size(); i++) {
        if (!resolved_packages[i].is_file_source_package ||
            !resolved_packages[i].external_essence)
        {
            return resolved_packages;
        }
    }

    // open referenced files if not already done so
    for (i = 0; i < locators.size(); i++) {
        NetworkLocator *network_locator = dynamic_cast<NetworkLocator*>(locators[i]);
        if (!network_locator)
            continue;

        URI uri;
        if (!uri.Parse(network_locator->getURLString())) {
            log_warn("Failed to parse url string '%s' from MXF file\n", network_locator->getURLString().c_str());
            continue;
        }
        if (!uri.IsAbsFile() && !uri.IsRelative())
            continue;

        if (uri.IsRelative())
            uri.MakeAbsolute(mFileReader->GetAbsoluteURI());

        // check whether file has already been opened
        size_t j;
        for (j = 0; j < mExternalReaders.size(); j++) {
            if (mExternalReaders[i]->GetAbsoluteURI() == uri)
                break;
        }
        if (j < mExternalReaders.size() || mFileReader->GetAbsoluteURI() == uri)
            continue;

        string filename = uri.ToFilename();
        MXFFileReader *file_reader = new MXFFileReader();
        MXFFileReader::OpenResult result = file_reader->Open(filename);
        if (result != MXFFileReader::MXF_RESULT_SUCCESS) {
            log_warn("Failed to open external MXF file '%s'\n", filename.c_str());
            delete file_reader;
            continue;
        }

        mExternalReaders.push_back(file_reader);
        ExtractResolvedPackages(file_reader);
    }

    return ResolveSourceClip(source_clip);
}

