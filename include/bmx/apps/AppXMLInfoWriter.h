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

#ifndef BMX_APP_XML_INFO_WRITER_H_
#define BMX_APP_XML_INFO_WRITER_H_

#include <bmx/apps/AppInfoWriter.h>
#include <bmx/XMLWriter.h>



namespace bmx
{


class AppXMLInfoWriter : public AppInfoWriter
{
public:
    static AppXMLInfoWriter* Open(const std::string &filename);

public:
    AppXMLInfoWriter(FILE *xml_file);
    AppXMLInfoWriter(XMLWriter *xml_writer);
    virtual ~AppXMLInfoWriter();

    void SetNamespace(const std::string &ns, const std::string &prefix);
    void SetVersion(const std::string &version);

public:
    virtual void Start(const std::string &name);
    virtual void End();

    virtual void StartSection(const std::string &name);
    virtual void EndSection();
    virtual void StartArrayItem(const std::string &name, size_t size);
    virtual void EndArrayItem();
    virtual void StartArrayElement(const std::string &name, size_t index);
    virtual void EndArrayElement();
    virtual void StartComplexItem(const std::string &name)  { StartSection(name); }
    virtual void EndComplexItem()                           { EndSection(); }

    virtual void WriteTimestampItem(const std::string &name, Timestamp value);

protected:
    virtual void WriteItem(const std::string &name, const std::string &value);

private:
    void WriteElementStart(const std::string &name);

private:
    XMLWriter *mXMLWriter;
    std::string mNamespace;
    std::string mPrefix;
    std::string mVersion;
};


};


#endif

