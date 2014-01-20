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

#ifndef BMX_APP_TEXT_INFO_WRITER_H_
#define BMX_APP_TEXT_INFO_WRITER_H_

#include <cstdio>

#include <stack>

#include <bmx/apps/AppInfoWriter.h>



namespace bmx
{


class AppTextInfoWriter : public AppInfoWriter
{
public:
    static AppTextInfoWriter* Open(const std::string &filename);

public:
    AppTextInfoWriter(FILE *text_file);
    virtual ~AppTextInfoWriter();

    void PushItemValueIndent(size_t value_indent);
    void PopItemValueIndent();

public:
    virtual void Start(const std::string &name);
    virtual void End();

    virtual void StartSection(const std::string &name);
    virtual void EndSection();
    virtual void StartArrayItem(const std::string &name, size_t size);
    virtual void EndArrayItem();
    virtual void StartArrayElement(const std::string &name, size_t index);
    virtual void EndArrayElement();
    virtual void StartComplexItem(const std::string &name);
    virtual void EndComplexItem();

    virtual void WriteDurationItem(const std::string &name, int64_t duration, Rational rate);

protected:
    virtual void WriteItem(const std::string &name, const std::string &value);

private:
    void WriteAnnotations();

private:
    FILE *mTextFile;
    int mLevel;
    std::stack<int> mItemValueIndent;
};


};


#endif

