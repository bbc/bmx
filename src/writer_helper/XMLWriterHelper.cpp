/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#include <expat.h>

#include <bmx/writer_helper/XMLWriterHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;

static const char NAMESPACE_SEPARATOR = ' ';


static void split_qname(const char *qname, string *ns, string *name)
{
    const char *ptr = qname;
    while (*ptr && *ptr != NAMESPACE_SEPARATOR)
        ptr++;

    if (!(*ptr)) {
        ns->clear();
        name->assign(qname);
    } else {
        ns->assign(qname, (size_t)(ptr - qname));
        name->assign(ptr + 1);
    }
}

static void expat_StartElement(void *user_data, const char *qname, const char **atts)
{
    string ns, name;
    split_qname(qname, &ns, &name);
    static_cast<XMLWriterHelper*>(user_data)->StartElement(ns, name, atts);
}


XMLWriterHelper::XMLWriterHelper()
{
    mTextEncoding = UNKNOWN_TEXT_ENCODING;
    mByteOrder = UNKNOWN_BYTE_ORDER;
    mSize = 0;
    mXMLParser = 0;
}

XMLWriterHelper::~XMLWriterHelper()
{
}

void XMLWriterHelper::ExtractInfo(const string &filename)
{
    FILE *file = 0;
    try
    {
        unsigned char buffer[8192];
        file = fopen(filename.c_str(), "rb");
        if (!file)
            BMX_EXCEPTION(("Failed to open XML file '%s': %s", filename.c_str(), bmx_strerror(errno).c_str()));
        size_t num_read = fread(buffer, 1, sizeof(buffer), file);
        ExtractInfo(buffer, (uint32_t)num_read);

        mSize = get_file_size(file);

        fclose(file);
    }
    catch (...)
    {
        if (file)
            fclose(file);
        throw;
    }
}

void XMLWriterHelper::ExtractInfo(const unsigned char *data, uint32_t size)
{
    mSize = size;
    get_xml_encoding(data, size, &mTextEncoding, &mByteOrder);

    XML_Parser parser = XML_ParserCreateNS(0, NAMESPACE_SEPARATOR);
    if (!parser)
        throw BMXException("XML_ParserCreate returned NULL");

    XML_SetStartElementHandler(parser, expat_StartElement);
    XML_SetUserData(parser, this);
    mXMLParser = parser;

    if (XML_Parse(parser, (const char*)data, (int)size, true) == XML_STATUS_ERROR) {
        log_warn("Failed to parse XML root element to extract information: %s\n",
                 XML_ErrorString(XML_GetErrorCode(parser)));
    }

    XML_ParserFree(parser);
    mXMLParser = 0;
}

void XMLWriterHelper::StartElement(const string &ns, const string &name, const char **atts)
{
    (void)name;

    mNamespace = ns;

    const char **atts_ptr = atts;
    while (atts_ptr && atts_ptr[0] && atts_ptr[1]) {
        const char *att_qname = *atts_ptr++;
        const char *att_value = *atts_ptr++;

        string att_ns;
        string att_name;
        split_qname(att_qname, &att_ns, &att_name);
        if (att_ns == "http://www.w3.org/XML/1998/namespace" && att_name == "lang") {
            mLanguageCode = att_value;
            break;
        }
    }

    XML_StopParser(static_cast<XML_Parser>(mXMLParser), XML_TRUE);
}
