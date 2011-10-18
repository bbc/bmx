/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#ifndef __IM_XML_UTILS_H__
#define __IM_XML_UTILS_H__


#include <string>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>

#include <im/IMTypes.h>



namespace im
{


class XMLInitializer
{
public:
    static void Initialize();

public:
    XMLInitializer();
    ~XMLInitializer();
};



bool write_dom_to_file(xercesc::DOMDocument *doc, std::string filename);

std::string get_xml_bool_str(bool value);
std::string get_xml_uint64_str(uint64_t value);
std::string get_xml_timestamp_str(Timestamp timestamp);
std::string get_xml_uuid_str(UUID value);
std::string get_xml_umid_str(UMID value);

bool parse_xml_umid_str(std::string umid_str, UMID *umid);

};



// Following code was copied from Xerces code samples

class XmlStr
{
public:
    XmlStr(const char* const toTranscode)
    {
        fUnicodeForm = xercesc::XMLString::transcode(toTranscode);
    }
    XmlStr(std::string toTranscode)
    {
        fUnicodeForm = xercesc::XMLString::transcode(toTranscode.c_str());
    }
    ~XmlStr()
    {
        xercesc::XMLString::release(&fUnicodeForm);
    }


    const XMLCh* unicodeForm() const
    {
        return fUnicodeForm;
    }

private:
    XMLCh *fUnicodeForm;
};

#define XStr(str) XmlStr(str).unicodeForm()


class StrXml
{
public :
    StrXml(const XMLCh* const toTranscode)
    {
        fLocalForm = xercesc::XMLString::transcode(toTranscode);
    }
    ~StrXml()
    {
        xercesc::XMLString::release(&fLocalForm);
    }


    const char* localForm() const
    {
        return fLocalForm;
    }

private:
    char *fLocalForm;
};

#define StrX(xmlstr) StrXml(xmlstr).localForm()



// End of code copied from Xerces code samples



#endif
