/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#ifndef BMX_APP_MXF_FILE_FACTORY_H_
#define BMX_APP_MXF_FILE_FACTORY_H_

#include <vector>
#include <map>
#include <set>

#include <bmx/mxf_helper/MXFFileFactory.h>
#include <bmx/MXFChecksumFile.h>
#include <bmx/URI.h>

#include <mxf/mxf_rw_intl_file.h>

#if defined(_WIN32)
#include <mxf/mxf_win32_file.h>
#endif



namespace bmx
{


class AppMXFFileFactory : public MXFFileFactory
{
public:
    AppMXFFileFactory();
    virtual ~AppMXFFileFactory();

    void SetInputChecksumTypes(const std::set<ChecksumType> &types);
    void AddInputChecksumType(ChecksumType type);
    void SetInputFlags(int flags);
    void SetRWInterleave(uint32_t rw_interleave_size);
    void SetHTTPMinReadSize(uint32_t size);

public:
    virtual mxfpp::File* OpenNew(std::string filename);
    virtual mxfpp::File* OpenRead(std::string filename);
    virtual mxfpp::File* OpenModify(std::string filename);

public:
    void ForceInputChecksumUpdate();
    void FinalizeInputChecksum();

    size_t GetNumInputChecksumFiles() const { return mInputChecksumFiles.size(); }
    std::string GetInputChecksumFilename(size_t file_index) const;
    URI GetInputChecksumAbsURI(size_t file_index) const;
    std::vector<ChecksumType> GetInputChecksumTypes(size_t file_index) const;

    size_t GetInputChecksumDigestSize(size_t file_index, ChecksumType type) const;
    void GetInputChecksumDigest(size_t file_index, ChecksumType type, unsigned char *digest, size_t size) const;
    std::string GetInputChecksumDigestString(size_t file_index, ChecksumType type) const;

private:
    MXFChecksumFile* GetChecksumFile(size_t file_index, ChecksumType type) const;

private:
    typedef struct
    {
        std::string filename;
        URI abs_uri;
        std::vector<std::pair<ChecksumType, MXFChecksumFile*> > checksum_files;
    } InputChecksumFile;

private:
    std::set<ChecksumType> mInputChecksumTypes;
    int mInputFlags;
    std::vector<InputChecksumFile> mInputChecksumFiles;
    MXFRWInterleaver *mRWInterleaver;
    uint32_t mHTTPMinReadSize;
};



};


#endif

