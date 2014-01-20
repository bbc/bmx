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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <climits>

#include <bmx/apps/AppTextInfoWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_utils.h>

using namespace std;
using namespace bmx;



AppTextInfoWriter* AppTextInfoWriter::Open(const string &filename)
{
    FILE *text_file = fopen(filename.c_str(), "wb");
    if (!text_file) {
        log_error("Failed to open text file '%s' for writing: %s\n", filename.c_str(), bmx_strerror(errno).c_str());
        return 0;
    }

    return new AppTextInfoWriter(text_file);
}

AppTextInfoWriter::AppTextInfoWriter(FILE *text_file)
: AppInfoWriter()
{
    mTextFile = text_file;
    mLevel = 0;
    mItemValueIndent.push(16);
}

AppTextInfoWriter::~AppTextInfoWriter()
{
    if (mTextFile != stdout && mTextFile != stderr)
        fclose(mTextFile);
}

void AppTextInfoWriter::PushItemValueIndent(size_t value_indent)
{
    BMX_CHECK(value_indent <= INT_MAX);
    mItemValueIndent.push((int)value_indent);
}

void AppTextInfoWriter::PopItemValueIndent()
{
    mItemValueIndent.pop();

    if (mItemValueIndent.empty())
        mItemValueIndent.push(16);
}

void AppTextInfoWriter::Start(const string &name)
{
    (void)name;

    WriteAnnotations();
    fprintf(mTextFile, "\n");
}

void AppTextInfoWriter::End()
{
    fflush(mTextFile);
    mLevel = 0;
}

void AppTextInfoWriter::StartSection(const string &name)
{
    if (mLevel == 0)
        fprintf(mTextFile, "%s", CamelCaseName(name).c_str());
    else
        fprintf(mTextFile, "%*c%s:", mLevel * 2, ' ', CamelCaseName(name).c_str());
    WriteAnnotations();
    fprintf(mTextFile, "\n");

    mLevel++;
}

void AppTextInfoWriter::EndSection()
{
    BMX_ASSERT(mLevel > 0);
    mLevel--;
}

void AppTextInfoWriter::StartArrayItem(const string &name, size_t size)
{
    if (mLevel == 0)
        fprintf(mTextFile, "%s: (%" PRIszt ")", CamelCaseName(name).c_str(), size);
    else
        fprintf(mTextFile, "%*c%s: (%" PRIszt ")", mLevel * 2, ' ', CamelCaseName(name).c_str(), size);
    WriteAnnotations();
    fprintf(mTextFile, "\n");

    mLevel++;
}

void AppTextInfoWriter::EndArrayItem()
{
    BMX_ASSERT(mLevel > 0);
    mLevel--;
}

void AppTextInfoWriter::StartArrayElement(const string &name, size_t index)
{
    if (mLevel == 0)
        fprintf(mTextFile, "%s #%" PRIszt ":", CamelCaseName(name).c_str(), index);
    else
        fprintf(mTextFile, "%*c%s #%" PRIszt ":", mLevel * 2, ' ', CamelCaseName(name).c_str(), index);
    WriteAnnotations();
    fprintf(mTextFile, "\n");

    mLevel++;
}

void AppTextInfoWriter::EndArrayElement()
{
    BMX_ASSERT(mLevel > 0);
    mLevel--;
}

void AppTextInfoWriter::StartComplexItem(const string &name)
{
    if (mLevel == 0)
        fprintf(mTextFile, "%s:", name.c_str());
    else
        fprintf(mTextFile, "%*c%s:", mLevel * 2, ' ', name.c_str());
    WriteAnnotations();
    fprintf(mTextFile, "\n");

    mLevel++;
}

void AppTextInfoWriter::EndComplexItem()
{
    BMX_ASSERT(mLevel > 0);
    mLevel--;
}

void AppTextInfoWriter::WriteDurationItem(const string &name, int64_t duration, Rational rate)
{
    string duration_str;
    if (duration >= 0)
        duration_str = get_duration_string(duration, rate);
    else
        duration_str = "unknown";

    if (mIsAnnotation) {
        mAnnotations.push_back(make_pair(name, duration_str));
    } else {
        StartAnnotations();
        WriteIntegerItem("count", duration);
        if (rate != mClipEditRate)
            WriteIntegerItem("rate", get_rounded_tc_base(rate));
        EndAnnotations();

        WriteItem(name, duration_str);
    }
}

void AppTextInfoWriter::WriteItem(const string &name, const string &value)
{
    int value_indent = mItemValueIndent.top();
    if ((int)name.size() < value_indent)
        value_indent -= (int)name.size();
    else
        value_indent = 0;

    if (mLevel == 0) {
        if (value_indent == 0)
            fprintf(mTextFile, "%s: %s", name.c_str(), value.c_str());
        else
            fprintf(mTextFile, "%s%*c: %s", name.c_str(), value_indent, ' ', value.c_str());
    } else {
        fprintf(mTextFile, "%*c%s%*c: %s", mLevel * 2, ' ', name.c_str(), value_indent, ' ', value.c_str());
    }
    WriteAnnotations();
    fprintf(mTextFile, "\n");
}

void AppTextInfoWriter::WriteAnnotations()
{
    if (mAnnotations.empty())
        return;

    fprintf(mTextFile, " (");
    size_t i;
    for (i = 0; i < mAnnotations.size(); i++) {
        if (i > 0)
            fprintf(mTextFile, ", ");
        fprintf(mTextFile, "%s='%s'", mAnnotations[i].first.c_str(), mAnnotations[i].second.c_str());
    }
    fprintf(mTextFile, ")");

    mAnnotations.clear();
}

