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

#include <cstring>
#include <cstdarg>

#include <bmx/apps/AppXMLInfoWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_utils.h>

using namespace std;
using namespace bmx;


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};



static bool get_utf8_code(const char *u8, uint32_t *code, size_t *len)
{
    if ((unsigned char)u8[0] < 0x80)
    {
        *code = u8[0] & 0x7f;
        *len = 1;
        return true;
    }
    else if (((unsigned char)u8[0] & 0xe0) == 0xc0 &&
             ((unsigned char)u8[1] & 0xc0) == 0x80)
    {
        *code = ((((uint32_t)u8[0]) & 0x1f) << 6) |
                 (((uint32_t)u8[1]) & 0x3f);
        *len = 2;
        return true;
    }
    else if (((unsigned char)u8[0] & 0xf0) == 0xe0 &&
             ((unsigned char)u8[1] & 0xc0) == 0x80 &&
             ((unsigned char)u8[2] & 0xc0) == 0x80)
    {
        *code = ((((uint32_t)u8[0]) & 0x0f) << 12) |
                ((((uint32_t)u8[1]) & 0x3f) << 6) |
                 (((uint32_t)u8[2]) & 0x3f);
        *len = 3;
        return true;
    }
    else if (((unsigned char)u8[0] & 0xf8) == 0xf0 &&
             ((unsigned char)u8[1] & 0xc0) == 0x80 &&
             ((unsigned char)u8[2] & 0xc0) == 0x80 &&
             ((unsigned char)u8[3] & 0xc0) == 0x80)
    {
        *code = ((((uint32_t)u8[0]) & 0x07) << 18) |
                ((((uint32_t)u8[1]) & 0x3f) << 12) |
                ((((uint32_t)u8[2]) & 0x3f) << 6) |
                 (((uint32_t)u8[3]) & 0x3f);
        *len = 4;
        return true;
    }

    return false;
}

static const string& get_xml_name(const string &name)
{
    // assert that XML element and attribute names use a limited set of characters
    // Note that it is more restrictive than what XML requires

    uint32_t code;
    size_t code_len = 0;
    size_t i;
    for (i = 0; i < name.size(); i += code_len) {
        if (!get_utf8_code(&name.c_str()[i], &code, &code_len))
        {
            // invalid UTF-8 character
            break;
        }
        else if (i == 0)
        {
            // allow [a-zA-Z] as first character
            if (!(code >= 'a' && code <= 'z') &&
                !(code >= 'A' && code <= 'Z'))
            {
                break;
            }
        }
        else
        {
            // allow [a-zA-Z0-9_] as non-first characters
            if (code != '_' &&
                !(code >= 'a' && code <= 'z') &&
                !(code >= 'A' && code <= 'Z') &&
                !(code >= '0' && code <= '9'))
            {
                break;
            }
        }
    }
    BMX_ASSERT(i >= name.size());

    return name;
}

static string get_xml_content(const string &value)
{
    string xml_content;
    xml_content.reserve(value.size());

    uint32_t code;
    size_t code_len = 0;
    size_t i;
    for (i = 0; i < value.size(); i += code_len) {
        if (!get_utf8_code(&value.c_str()[i], &code, &code_len))
        {
            // ignore invalid UTF-8 characters
            code_len = 1;
        }
        else if (code == 0x09 ||
                 code == 0x0a ||
                 code == 0x0d ||
                 (code >= 0x20    && code <= 0xd7ff) ||
                 (code >= 0xe000  && code <= 0xfffd) ||
                 (code >= 0x10000 && code <= 0x10ffff))
        {
            xml_content.append(&value.c_str()[i], code_len);
        }
        // else ignore characters that can't be represented in XML 1.0
    }

    return xml_content;
}



AppXMLInfoWriter* AppXMLInfoWriter::Open(const string &filename)
{
    XMLWriter *xml_writer = XMLWriter::Open(filename);
    if (!xml_writer)
        return 0;

    return new AppXMLInfoWriter(xml_writer);
}

AppXMLInfoWriter::AppXMLInfoWriter(FILE *xml_file)
: AppInfoWriter()
{
    mXMLWriter = new XMLWriter(xml_file);
}

AppXMLInfoWriter::AppXMLInfoWriter(XMLWriter *xml_writer)
: AppInfoWriter()
{
    mXMLWriter = xml_writer;
}

AppXMLInfoWriter::~AppXMLInfoWriter()
{
    delete mXMLWriter;
}

void AppXMLInfoWriter::SetNamespace(const string &ns, const string &prefix)
{
    mNamespace = ns;
    mPrefix = prefix;
}

void AppXMLInfoWriter::SetVersion(const string &version)
{
    mVersion = version;
}

void AppXMLInfoWriter::Start(const string &name)
{
    if (BMX_REGRESSION_TEST)
        mXMLWriter->SkipCR(true); // avoids CR/LF versus LF difference
    else
        mXMLWriter->EscapeCR(true);
    mXMLWriter->EscapeAttrNewlineChars(true);

    StartAnnotations();
    WriteStringItem("version", mVersion);
    EndAnnotations();

    mXMLWriter->WriteDocumentStart();
    mXMLWriter->WriteElementStart(mNamespace, get_xml_name(name));
    if (!mNamespace.empty())
        mXMLWriter->DeclareNamespace(mNamespace, mPrefix);

    size_t i;
    for (i = 0; i < mAnnotations.size(); i++) {
        mXMLWriter->WriteAttribute(mNamespace, get_xml_name(mAnnotations[i].first),
                                               get_xml_content(mAnnotations[i].second));
    }
    mAnnotations.clear();
}

void AppXMLInfoWriter::End()
{
    mXMLWriter->WriteElementEnd();
    mXMLWriter->WriteDocumentEnd();
}

void AppXMLInfoWriter::StartSection(const string &name)
{
    WriteElementStart(name);
}

void AppXMLInfoWriter::EndSection()
{
    mXMLWriter->WriteElementEnd();
}

void AppXMLInfoWriter::StartArrayItem(const string &name, size_t size)
{
    StartAnnotations();
    WriteIntegerItem("size", (uint64_t)size);
    EndAnnotations();

    StartSection(name);
}

void AppXMLInfoWriter::EndArrayItem()
{
    EndSection();
}

void AppXMLInfoWriter::StartArrayElement(const string &name, size_t index)
{
    StartAnnotations();
    WriteIntegerItem("index", (uint64_t)index);
    EndAnnotations();

    StartSection(name);
}

void AppXMLInfoWriter::EndArrayElement()
{
    EndSection();
}

void AppXMLInfoWriter::WriteTimestampItem(const string &name, Timestamp value)
{
    char buffer[64];

    if (value.year <= 0 ||
        value.month == 0 || value.month > 12 ||
        value.day == 0 || value.day > 31 ||
        value.hour > 23 ||
        value.min > 59 ||
        value.sec > 59)
    {
        bmx_snprintf(buffer, sizeof(buffer), "1000-01-01T00:00:00");
    }
    else
    {
        bmx_snprintf(buffer, sizeof(buffer), "%d-%02u-%02uT%02u:%02u:%02u.%03uZ",
                     value.year, value.month, value.day,
                     value.hour, value.min, value.sec, value.qmsec * 4);
    }

    if (mIsAnnotation)
        mAnnotations.push_back(make_pair(name, buffer));
    else
        WriteItem(name, buffer);
}

void AppXMLInfoWriter::WriteItem(const string &name, const string &value)
{
    WriteElementStart(name);
    mXMLWriter->WriteElementContent(get_xml_content(value));
    mXMLWriter->WriteElementEnd();
}

void AppXMLInfoWriter::WriteElementStart(const string &name)
{
    mXMLWriter->WriteElementStart(mNamespace, get_xml_name(name));

    size_t i;
    for (i = 0; i < mAnnotations.size(); i++) {
        mXMLWriter->WriteAttribute(mNamespace, get_xml_name(mAnnotations[i].first),
                                               get_xml_content(mAnnotations[i].second));
    }
    mAnnotations.clear();
}

