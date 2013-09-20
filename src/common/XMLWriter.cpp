/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#include <cstring>
#include <cerrno>

#include <bmx/XMLWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



static const string INTERNAL_DEFAULT_NAMESPACE_PREFIX = " ";

static const string LT   = "&lt;";
static const string GT   = "&gt;";
static const string AMP  = "&amp;";
static const string QUOT = "&quot;";
static const string APOS = "&apos;";
static const string LF   = "&#x0A;";
static const string CR   = "&#x0D;";



XMLWriter* XMLWriter::Open(const string &filename)
{
    FILE *xml_file = fopen(filename.c_str(), "wb");
    if (!xml_file) {
        log_error("Failed to open XML file '%s' for writing: %s\n", filename.c_str(), bmx_strerror(errno).c_str());
        return 0;
    }

    return new XMLWriter(xml_file);
}


XMLWriter::XMLWriter(FILE *xml_file)
{
    mXMLFile = xml_file;
    mPrevWriteType = NONE;
    mLevel = 0;
    mEscapeCR = false;
    mEscapeAttrNewlineChars = false;
    mSkipCR = false;
}

XMLWriter::~XMLWriter()
{
    size_t i;
    for (i = 0; i < mElementStack.size(); i++)
        delete mElementStack[i];

    if (mXMLFile && mXMLFile != stdout && mXMLFile != stderr)
        fclose(mXMLFile);
}

void XMLWriter::EscapeCR(bool escape)
{
    mEscapeCR = escape;
}

void XMLWriter::EscapeAttrNewlineChars(bool escape)
{
    mEscapeAttrNewlineChars = escape;
}

void XMLWriter::SkipCR(bool skip)
{
    mSkipCR = skip;
}

void XMLWriter::WriteDocumentStart()
{
    BMX_CHECK(mPrevWriteType == NONE);

    WriteProlog();

    mPrevWriteType = START;
}

void XMLWriter::WriteDocumentEnd()
{
    size_t i;
    for (i = 0; i < mElementStack.size(); i++)
        WriteElementEnd();
    mElementStack.clear();

    BMX_CHECK(mXMLFile);
    fclose(mXMLFile);
    mXMLFile = 0;

    mPrevWriteType = END;
}

void XMLWriter::WriteElementStart(const string &ns, const string &local_name)
{
    BMX_CHECK(mPrevWriteType == START ||
              mPrevWriteType == ELEMENT_START ||
              mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT ||
              mPrevWriteType == ATTRIBUTE_END ||
              mPrevWriteType == ELEMENT_CONTENT ||
              mPrevWriteType == ELEMENT_END ||
              mPrevWriteType == COMMENT ||
              mPrevWriteType == PROC_INSTRUCTION);

    if (mPrevWriteType == ATTRIBUTE_START || mPrevWriteType == ATTRIBUTE_CONTENT)
        WriteAttributeEnd();

    if (mPrevWriteType == ELEMENT_START || mPrevWriteType == ATTRIBUTE_END)
    {
        Write(">\n", 2);
    }
    else if (mPrevWriteType == ELEMENT_CONTENT ||
             mPrevWriteType == COMMENT ||
             mPrevWriteType == PROC_INSTRUCTION)
    {
        WriteElementEnd();
    }

    Element *parent_element = (mElementStack.empty() ? 0 : mElementStack.back());
    Element *element = new Element(parent_element, ns, local_name);
    mElementStack.push_back(element);

    string prefix = element->GetPrefix(ns);
    if (prefix.empty()) {
        mPrevWriteType = DELAYED_ELEMENT_START;
        return;
    }

    WriteIndent(mLevel);
    Write("<", 1);
    if (prefix != INTERNAL_DEFAULT_NAMESPACE_PREFIX) {
        Write(prefix);
        Write(":", 1);
    }
    Write(local_name);

    mLevel++;
    mPrevWriteType = ELEMENT_START;
}

void XMLWriter::DeclareNamespace(const string &ns, const string &prefix)
{
    BMX_CHECK(mPrevWriteType == DELAYED_ELEMENT_START ||
              mPrevWriteType == ELEMENT_START);
    BMX_CHECK(!ns.empty());

    BMX_CHECK(!mElementStack.empty());
    Element *element = mElementStack.back();

    bool was_added;
    if (prefix.empty())
        was_added = element->AddNamespaceDecl(ns, INTERNAL_DEFAULT_NAMESPACE_PREFIX);
    else
        was_added = element->AddNamespaceDecl(ns, prefix);

    if (was_added) {
        if (mPrevWriteType == DELAYED_ELEMENT_START) {
            WriteIndent(mLevel);
            Write("<", 1);
            if (!prefix.empty()) {
                Write(prefix);
                Write(":", 1);
            }
            Write(element->GetLocalName());

            mLevel++;
            mPrevWriteType = ELEMENT_START;
        }

        Write(" xmlns", 6);
        if (!prefix.empty()) {
            Write(":", 1);
            Write(prefix);
        }
        Write("=\"", 2);
        WriteAttributeData(ns);
        Write("\"", 1);
    }
}

void XMLWriter::WriteAttribute(const string &ns, const string &local_name, const string &value)
{
    BMX_CHECK(mPrevWriteType == ELEMENT_START ||
              mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT ||
              mPrevWriteType == ATTRIBUTE_END);

    if (mPrevWriteType == ATTRIBUTE_START || mPrevWriteType == ATTRIBUTE_CONTENT)
        WriteAttributeEnd();

    Write(" ", 1);

    if (!ns.empty()) {
        string prefix = GetNonDefaultNSPrefix(ns);
        if (!prefix.empty()) {
            Write(prefix);
            Write(":", 1);
        }
    }

    Write(local_name);
    Write("=\"", 2);
    WriteAttributeData(value);
    Write("\"", 1);

    mPrevWriteType = ATTRIBUTE_END;
}

void XMLWriter::WriteAttributeStart(const string &ns, const string &local_name)
{
    BMX_CHECK(mPrevWriteType == ELEMENT_START ||
              mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT ||
              mPrevWriteType == ATTRIBUTE_END);

    if (mPrevWriteType == ATTRIBUTE_START || mPrevWriteType == ATTRIBUTE_CONTENT)
        WriteAttributeEnd();

    Write(" ", 1);

    if (!ns.empty()) {
        string prefix = GetNonDefaultNSPrefix(ns);
        if (!prefix.empty()) {
            Write(prefix);
            Write(":", 1);
        }
    }

    Write(local_name);
    Write("=\"", 2);

    mPrevWriteType = ATTRIBUTE_START;
}

void XMLWriter::WriteAttributeContent(const string &value)
{
    BMX_CHECK(mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT);

    WriteAttributeData(value);

    mPrevWriteType = ATTRIBUTE_CONTENT;
}

void XMLWriter::WriteAttributeEnd()
{
    BMX_CHECK(mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT);

    Write("\"", 1);

    mPrevWriteType = ATTRIBUTE_END;
}

void XMLWriter::WriteElementContent(const string &content)
{
    BMX_CHECK(mPrevWriteType == ELEMENT_START ||
              mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT ||
              mPrevWriteType == ATTRIBUTE_END ||
              mPrevWriteType == ELEMENT_CONTENT);

    if (mPrevWriteType == ATTRIBUTE_START || mPrevWriteType == ATTRIBUTE_CONTENT)
        WriteAttributeEnd();

    if (mPrevWriteType == ELEMENT_START || mPrevWriteType == ATTRIBUTE_END)
        Write(">", 1);

    WriteElementData(content);

    mPrevWriteType = ELEMENT_CONTENT;
}

void XMLWriter::WriteElementEnd()
{
    BMX_CHECK(mPrevWriteType == ELEMENT_START ||
              mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT ||
              mPrevWriteType == ATTRIBUTE_END ||
              mPrevWriteType == ELEMENT_CONTENT ||
              mPrevWriteType == ELEMENT_END ||
              mPrevWriteType == COMMENT ||
              mPrevWriteType == PROC_INSTRUCTION);

    mLevel--;

    BMX_CHECK(!mElementStack.empty());
    Element *element = mElementStack.back();
    string prefix = element->GetPrefix();
    BMX_CHECK(!prefix.empty());

    if (mPrevWriteType == ATTRIBUTE_START || mPrevWriteType == ATTRIBUTE_CONTENT)
        WriteAttributeEnd();

    if (mPrevWriteType == ELEMENT_START || mPrevWriteType == ATTRIBUTE_END) {
        Write("/>\n", 3);
    } else {
        if (mPrevWriteType != ELEMENT_CONTENT) {
            if (mPrevWriteType != ELEMENT_END &&
                mPrevWriteType != COMMENT &&
                mPrevWriteType != PROC_INSTRUCTION)
            {
                Write("\n", 1);
            }
            WriteIndent(mLevel);
        }

        Write("</", 2);
        if (prefix != INTERNAL_DEFAULT_NAMESPACE_PREFIX) {
            Write(prefix);
            Write(":", 1);
        }
        Write(element->GetLocalName());
        Write(">\n", 2);
    }

    mElementStack.pop_back();
    delete element;

    mPrevWriteType = ELEMENT_END;
}

void XMLWriter::WriteElement(const string &ns, const string &local_name, const string &content)
{
    WriteElementStart(ns, local_name);
    if (!content.empty())
        WriteElementContent(content);
    WriteElementEnd();
}

void XMLWriter::WriteComment(const string &comment)
{
    BMX_CHECK(mPrevWriteType == START ||
              mPrevWriteType == ELEMENT_START ||
              mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT ||
              mPrevWriteType == ATTRIBUTE_END ||
              mPrevWriteType == ELEMENT_END ||
              mPrevWriteType == COMMENT ||
              mPrevWriteType == PROC_INSTRUCTION);

    if (mPrevWriteType == ATTRIBUTE_START || mPrevWriteType == ATTRIBUTE_CONTENT)
        WriteAttributeEnd();

    if (mPrevWriteType == ELEMENT_START || mPrevWriteType == ATTRIBUTE_END)
        Write(">\n", 2);

    WriteIndent(mLevel);
    Write("<!--", 4);
    Write(comment);
    Write("-->\n", 4);

    if (mPrevWriteType != ELEMENT_END && mPrevWriteType != START)
        mPrevWriteType = COMMENT;
}

void XMLWriter::WriteProcInstruction(const string &target, const string &instruction)
{
    BMX_CHECK(mPrevWriteType == START ||
              mPrevWriteType == ELEMENT_START ||
              mPrevWriteType == ATTRIBUTE_START ||
              mPrevWriteType == ATTRIBUTE_CONTENT ||
              mPrevWriteType == ATTRIBUTE_END ||
              mPrevWriteType == ELEMENT_END ||
              mPrevWriteType == COMMENT ||
              mPrevWriteType == PROC_INSTRUCTION);

    if (mPrevWriteType == ATTRIBUTE_START || mPrevWriteType == ATTRIBUTE_CONTENT)
        WriteAttributeEnd();

    if (mPrevWriteType == ELEMENT_START || mPrevWriteType == ATTRIBUTE_END)
        Write(">\n", 2);

    WriteIndent(mLevel);
    Write("<?", 2);
    Write(target);
    Write(" ", 1);
    Write(instruction);
    Write("?>\n", 3);

    if (mPrevWriteType != ELEMENT_END && mPrevWriteType != START)
        mPrevWriteType = PROC_INSTRUCTION;
}

void XMLWriter::WriteText(const string &text)
{
    Write(text);
}

void XMLWriter::Flush()
{
    fflush(mXMLFile);
}

string XMLWriter::GetPrefix(const string &ns)
{
    if (mElementStack.empty())
        return "";

    return mElementStack.back()->GetPrefix(ns);
}

string XMLWriter::GetNonDefaultNSPrefix(const string &ns)
{
    if (mElementStack.empty())
        return "";

    return mElementStack.back()->GetNonDefaultNSPrefix(ns);
}


XMLWriter::Element::Element(Element *parent_element, const string &ns, const string &local_name)
{
    mParentElement = parent_element;
    mNS = ns;
    mLocalName = local_name;
    mDefaultNS = "";

    if (parent_element) {
        mNSpaceDecls = parent_element->GetNamespaceDecls();
        mDefaultNS = parent_element->GetDefaultNamespace();
    }
    mPrefix = GetPrefix(ns);
}

XMLWriter::Element::~Element()
{
}

bool XMLWriter::Element::AddNamespaceDecl(const string &ns, const string &prefix)
{
    BMX_CHECK(!ns.empty());
    BMX_CHECK(!prefix.empty());

    if (prefix == INTERNAL_DEFAULT_NAMESPACE_PREFIX) {
        mDefaultNS = ns;
        if (mNS == ns)
            mPrefix = prefix;
    } else {
        pair<map<string, string>::iterator, bool> result = mNSpaceDecls.insert(pair<string, string>(ns, prefix));
        if (!result.second) {
            if (result.first->first == ns && result.first->second == prefix)
                return false;
            mNSpaceDecls.erase(result.first);
            mNSpaceDecls.insert(make_pair(ns, prefix));
        }

        if (mNS == ns && mDefaultNS != ns)
            mPrefix = prefix;
    }

    return true;
}

string XMLWriter::Element::GetPrefix(const string &ns) const
{
    if (mDefaultNS == ns) {
        return INTERNAL_DEFAULT_NAMESPACE_PREFIX;
    } else {
        map<string, string>::const_iterator result = mNSpaceDecls.find(ns);
        if (result == mNSpaceDecls.end())
            return "";

        return result->second;
    }
}

string XMLWriter::Element::GetNonDefaultNSPrefix(const string &ns) const
{
    map<string, string>::const_iterator result = mNSpaceDecls.find(ns);
    if (result == mNSpaceDecls.end())
        return "";

    return result->second;
}


void XMLWriter::WriteProlog()
{
    Write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
}

void XMLWriter::WriteElementData(const string &data)
{
    string escaped_data;
    escaped_data.reserve(data.size() * 2);
    const char *data_ptr = data.c_str();
    size_t i;
    for (i = 0; i < data.size(); i++) {
        if (*data_ptr == '>') {
            escaped_data.append(GT);
        } else if (*data_ptr == '<') {
            escaped_data.append(LT);
        } else if (*data_ptr == '&') {
            escaped_data.append(AMP);
        } else if (*data_ptr == 0x0d) {
            if (!mSkipCR) {
                if (mEscapeCR)
                    escaped_data.append(CR);
                else
                    escaped_data.append(data_ptr, 1);
            }
        } else {
            escaped_data.append(data_ptr, 1);
        }

        data_ptr++;
    }

    Write(escaped_data);
}

void XMLWriter::WriteAttributeData(const string &data)
{
    string escaped_data;
    escaped_data.reserve(data.size() * 2);
    const char *data_ptr = data.c_str();
    size_t i;
    for (i = 0; i < data.size(); i++) {
        if (*data_ptr == '\"') {
            escaped_data.append(QUOT);
        } else if (*data_ptr == '\'') {
            escaped_data.append(APOS);
        } else if (*data_ptr == '&') {
            escaped_data.append(AMP);
        } else if (*data_ptr == 0x0a && mEscapeAttrNewlineChars) {
            escaped_data.append(LF);
        } else if (*data_ptr == 0x0d) {
            if (!mSkipCR) {
                if (mEscapeAttrNewlineChars || mEscapeCR)
                    escaped_data.append(CR);
                else
                    escaped_data.append(data_ptr, 1);
            }
        } else {
            escaped_data.append(data_ptr, 1);
        }

        data_ptr++;
    }

    Write(escaped_data);
}

void XMLWriter::WriteIndent(int level)
{
    BMX_CHECK(level >= 0);

    int i;
    for (i = 0; i < level; i++)
        Write("  ", 2);
}

void XMLWriter::Write(const string &data)
{
    Write(data.c_str(), data.size());
}

void XMLWriter::Write(const char *data, size_t len)
{
    BMX_CHECK(mXMLFile);
    if (fwrite(data, 1, len, mXMLFile) != len)
        log_error("XML fwrite failed: %s\n", bmx_strerror(errno).c_str());
}

