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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <im/as02/AS02Shim.h>
#include <im/XMLUtils.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace xercesc;


static const char AS02_V10_NAMESPACE[] = "http://www.amwa.tv/as-02/1.0/shim";



AS02Shim::AS02Shim()
{
    XMLInitializer::Initialize();
}

AS02Shim::~AS02Shim()
{
}

void AS02Shim::SetName(string name)
{
    mName = name;
}

void AS02Shim::SetId(string uri)
{
    mId = uri;
}

void AS02Shim::SetApplicationSpec(string spec)
{
    mApplicationSpec = spec;
}

void AS02Shim::AppendAnnotation(string annotation)
{
    mAnnotations.push_back(annotation);
}

void AS02Shim::Write(string filename)
{
    DOMDocument *doc = 0;

    try
    {
        DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(XStr("Core"));
        DOMDocument *doc = impl->createDocument(XStr(AS02_V10_NAMESPACE),
                                                XStr("Shim"),
                                                0);

        DOMElement *root = doc->getDocumentElement();

        DOMElement *ele;

        ele = doc->createElementNS(XStr(AS02_V10_NAMESPACE), XStr("ShimName"));
        root->appendChild(ele);
        ele->appendChild(doc->createTextNode(XStr(mName)));

        if (!mId.empty()) {
            ele = doc->createElementNS(XStr(AS02_V10_NAMESPACE), XStr("ShimID"));
            root->appendChild(ele);
            ele->appendChild(doc->createTextNode(XStr(mId)));
        }

        if (!mApplicationSpec.empty()) {
            ele = doc->createElementNS(XStr(AS02_V10_NAMESPACE), XStr("ApplicationSpec"));
            root->appendChild(ele);
            ele->appendChild(doc->createTextNode(XStr(mApplicationSpec)));
        }

        size_t i;
        for (i = 0; i < mAnnotations.size(); i++) {
            ele = doc->createElementNS(XStr(AS02_V10_NAMESPACE), XStr("AnnotationText"));
            root->appendChild(ele);
            ele->appendChild(doc->createTextNode(XStr(mAnnotations[i])));
        }


        IM_CHECK_M(write_dom_to_file(doc, filename),
                   ("Failed to write shim file '%s'", filename.c_str()));


        delete doc;
    }
    catch (const xercesc::XMLException &ex)
    {
        delete doc;
        throw new IMException("XML Exception: ", StrX(ex.getMessage()));
    }
    catch (...)
    {
        delete doc;
        throw;
    }
}

