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

#ifndef BMX_XML_WRITER_H_
#define BMX_XML_WRITER_H_

#include <cstdio>

#include <vector>
#include <map>
#include <string>



namespace bmx
{


class XMLWriter
{
public:
    static XMLWriter* Open(const std::string &filename);

public:
    XMLWriter(FILE *xml_file);
    virtual ~XMLWriter();

    void EscapeCR(bool escape);
    void EscapeAttrNewlineChars(bool escape);
    void SkipCR(bool skip);

    void WriteDocumentStart();
    void WriteDocumentEnd();
    void WriteElementStart(const std::string &ns, const std::string &local_name);
    void DeclareNamespace(const std::string &ns, const std::string &prefix);
    void WriteAttribute(const std::string &ns, const std::string &local_name, const std::string &value);
    void WriteAttributeStart(const std::string &ns, const std::string &local_name);
    void WriteAttributeContent(const std::string &value);
    void WriteAttributeEnd();
    void WriteElementContent(const std::string &content);
    void WriteElementEnd();
    void WriteElement(const std::string &ns, const std::string &local_name, const std::string &content);
    void WriteComment(const std::string &comment);
    void WriteProcInstruction(const std::string &target, const std::string &instruction);
    void WriteText(const std::string &text);

    void Flush();

private:
    std::string GetPrefix(const std::string &ns);
    std::string GetNonDefaultNSPrefix(const std::string &ns);

    class Element
    {
    public:
        Element(Element* parentElement, const std::string &ns, const std::string &local_name);
        ~Element();

        bool AddNamespaceDecl(const std::string &ns, const std::string &prefix);
        std::map<std::string, std::string>& GetNamespaceDecls() { return mNSpaceDecls; }

        const std::string& GetNamespace() const { return mNS; }
        const std::string& GetLocalName() const { return mLocalName; }
        std::string GetPrefix(const std::string &ns) const;
        std::string GetNonDefaultNSPrefix(const std::string &ns) const;
        const std::string& GetPrefix() const { return mPrefix; }
        const std::string& GetDefaultNamespace() const { return mDefaultNS; }

    private:
        Element *mParentElement;
        std::string mNS;
        std::string mPrefix;
        std::string mLocalName;
        std::string mDefaultNS;
        std::map<std::string, std::string> mNSpaceDecls;
    };

    enum WriteType
    {
        NONE,
        START,
        END,
        ELEMENT_START,
        DELAYED_ELEMENT_START,
        ATTRIBUTE_START,
        ATTRIBUTE_CONTENT,
        ATTRIBUTE_END,
        ELEMENT_CONTENT,
        ELEMENT_END,
        COMMENT,
        PROC_INSTRUCTION
    };

    WriteType mPrevWriteType;
    std::vector<Element*> mElementStack;
    int mLevel;


private:
    void WriteProlog();
    void WriteElementData(const std::string &data);
    void WriteAttributeData(const std::string &data);
    void WriteIndent(int level);

    void Write(const std::string &data);
    void Write(const char *data, size_t len);

private:
    FILE *mXMLFile;
    bool mEscapeCR;
    bool mEscapeAttrNewlineChars;
    bool mSkipCR;
};


};



#endif

