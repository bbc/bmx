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

#include <map>

#include <bmx/mxf_helper/MXFFileFactory.h>
#include <bmx/MXFUtils.h>

#include <mxf/mxf_rw_intl_file.h>

#if defined(_WIN32)
#include <mxf/mxf_win32_file.h>
#endif



namespace bmx
{


class AppMXFFileFactory : public MXFFileFactory
{
public:
    typedef struct
    {
        unsigned char bytes[16];
    } MD5Digest;

public:
    AppMXFFileFactory(bool md5_wrap_input, int input_flags);
    AppMXFFileFactory(bool md5_wrap_input, int input_flags, bool rw_interleave, uint32_t rw_interleave_size);
    virtual ~AppMXFFileFactory();

    virtual mxfpp::File* OpenNew(std::string filename);
    virtual mxfpp::File* OpenRead(std::string filename);
    virtual mxfpp::File* OpenModify(std::string filename);

    void ForceInputMD5Update();
    void FinalizeInputMD5();
    const std::map<std::string, MD5Digest>& GetInputMD5Digests() { return mInputMD5Digests; }

private:
    bool mMD5WrapInput;
    int mInputFlags;
    std::map<std::string, MXFMD5WrapperFile*> mInputMD5WrapFiles;
    std::map<std::string, MD5Digest> mInputMD5Digests;
    MXFRWInterleaver *mRWInterleaver;
};



};


#endif

