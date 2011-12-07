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

#ifndef __BMX_MXF_PACKAGE_RESOLVER_H__
#define __BMX_MXF_PACKAGE_RESOLVER_H__


#include <vector>

#include <libMXF++/MXF.h>



namespace bmx
{


class MXFFileReader;


class ResolvedPackage
{
public:
    ResolvedPackage();

    mxfpp::GenericPackage *package;
    mxfpp::GenericTrack *generic_track; /* can be 0 when resolving package only */
    uint32_t track_id;
    MXFFileReader *file_reader;
    bool is_file_source_package;
    bool external_essence;
};


class MXFPackageResolver
{
public:
    virtual ~MXFPackageResolver() {};

    virtual std::vector<ResolvedPackage> ResolveSourceClip(mxfpp::SourceClip *source_clip) = 0;
    virtual std::vector<ResolvedPackage> ResolveSourceClip(mxfpp::SourceClip *source_clip,
                                                           std::vector<mxfpp::Locator*> &locators) = 0;

    virtual std::vector<ResolvedPackage> GetResolvedPackages() = 0;
};



class MXFDefaultPackageResolver : public MXFPackageResolver
{
public:
    MXFDefaultPackageResolver(MXFFileReader *file_reader);
    virtual ~MXFDefaultPackageResolver();

    virtual std::vector<ResolvedPackage> ResolveSourceClip(mxfpp::SourceClip *source_clip);
    virtual std::vector<ResolvedPackage> ResolveSourceClip(mxfpp::SourceClip *source_clip,
                                                           std::vector<mxfpp::Locator*> &locators);

    virtual std::vector<ResolvedPackage> GetResolvedPackages() { return mResolvedPackages; }

protected:
    void ExtractResolvedPackages(MXFFileReader *file_reader);

protected:
    std::vector<ResolvedPackage> mResolvedPackages;

    MXFFileReader* mFileReader;
    std::vector<MXFFileReader*> mExternalReaders;
};


};



#endif

