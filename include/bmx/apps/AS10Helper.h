/*
 * Copyright (C) 2016, British Broadcasting Corporation
 * All Rights Reserved.
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

#ifndef AS10_HELPER_H_
#define AS10_HELPER_H_

#include <vector>

#include <bmx/apps/FrameworkHelper.h>
#include <bmx/as10/AS10Info.h>
#include <bmx/as10/AS10WriterHelper.h>
#include <bmx/mxf_reader/MXFFileReader.h>



namespace bmx
{


class AS10Helper
{
public:
    AS10Helper();
    ~AS10Helper();

    void ReadSourceInfo(MXFFileReader *source_file);

    bool SupportFrameworkType(const char *type_str);
    bool ParseFrameworkFile(const char *type_str, const char *filename);
    bool SetFrameworkProperty(const char *type_str, const char *name, const char *value);

    bool HaveMainTitle() const;
    std::string GetMainTitle() const;
    const char* GetShimName() const;

public:
    void AddMetadata(ClipWriter *clip);
    void Complete();

private:
    bool ParseFrameworkType(const char *type_str, FrameworkType *type) const;
    void SetFrameworkProperty(FrameworkType type, std::string name, std::string value);

private:
    std::vector<FrameworkProperty> mFrameworkProperties;

    AS10Info *mSourceInfo;
    std::string mSourceMainTitle;

    AS10WriterHelper *mWriterHelper;
    FrameworkHelper *mAS10FrameworkHelper;
};


};



#endif
