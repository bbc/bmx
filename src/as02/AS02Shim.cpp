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

#include <bmx/as02/AS02Shim.h>
#include <bmx/XMLWriter.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const char AS02_V10_NAMESPACE[] = "http://www.amwa.tv/as-02/1.0/shim";



AS02Shim::AS02Shim()
{
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
    XMLWriter *xml_writer = XMLWriter::Open(filename);
    BMX_CHECK(xml_writer);

    try
    {
        xml_writer->WriteDocumentStart();
        xml_writer->WriteElementStart(AS02_V10_NAMESPACE, "Shim");
        xml_writer->DeclareNamespace(AS02_V10_NAMESPACE, "");

        xml_writer->WriteElement(AS02_V10_NAMESPACE, "ShimName", mName);

        if (!mId.empty())
            xml_writer->WriteElement(AS02_V10_NAMESPACE, "ShimID", mId);

        if (!mApplicationSpec.empty())
            xml_writer->WriteElement(AS02_V10_NAMESPACE, "ApplicationSpec", mApplicationSpec);

        size_t i;
        for (i = 0; i < mAnnotations.size(); i++)
            xml_writer->WriteElement(AS02_V10_NAMESPACE, "AnnotationText", mAnnotations[i]);

        xml_writer->WriteElementEnd();
        xml_writer->WriteDocumentEnd();

        delete xml_writer;
    }
    catch (...)
    {
        delete xml_writer;
        throw;
    }
}

