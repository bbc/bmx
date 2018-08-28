/*
 * Copyright (C) 2018, British Broadcasting Corporation
 * All Rights Reserved.
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

#include <bmx/mxf_helper/TimedTextMXFDescriptorHelper.h>

#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



EssenceType TimedTextMXFDescriptorHelper::IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    (void)alternative_ec_label;

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(TimedText)))
        return UNKNOWN_ESSENCE_TYPE;

    DCTimedTextDescriptor *tt_descriptor = dynamic_cast<DCTimedTextDescriptor*>(file_descriptor);
    if (!tt_descriptor)
        return UNKNOWN_ESSENCE_TYPE;

    return TIMED_TEXT;
}

bool TimedTextMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return essence_type == TIMED_TEXT;
}

TimedTextManifest* TimedTextMXFDescriptorHelper::CreateManifest(FileDescriptor *file_descriptor)
{
    DCTimedTextDescriptor *tt_descriptor = dynamic_cast<DCTimedTextDescriptor*>(file_descriptor);
    BMX_ASSERT(tt_descriptor);

    TimedTextManifest *manifest = new TimedTextManifest();
    try
    {
        if (tt_descriptor->haveResourceID()) {
            manifest->mResourceId = tt_descriptor->getResourceID();
        }
        manifest->SetProfileDesignator(tt_descriptor->getNamespaceURI());
        manifest->mEncoding = tt_descriptor->getUCSEncoding();
        if (tt_descriptor->haveRFC5646LanguageTagList()) {
            manifest->SetLanguagesString(tt_descriptor->getRFC5646LanguageTagList());
        }

        if (tt_descriptor->haveSubDescriptors()) {
            vector<SubDescriptor*> sub_descriptors = tt_descriptor->getSubDescriptors();
            size_t i;
            for (i = 0; i < sub_descriptors.size(); i++) {
                DCTimedTextResourceSubDescriptor *tt_subdescriptor =
                    dynamic_cast<DCTimedTextResourceSubDescriptor*>(sub_descriptors[i]);
                if (tt_subdescriptor) {
                    TimedTextAncillaryResource anc_resource;
                    anc_resource.resource_id = tt_subdescriptor->getAncillaryResourceID();
                    anc_resource.stream_id   = tt_subdescriptor->getEssenceStreamID();
                    anc_resource.mime_type   = tt_subdescriptor->getMIMEType();
                    manifest->mAncillaryResources.push_back(anc_resource);
                }
            }
        }

        return manifest;
    }
    catch (...)
    {
        delete manifest;
        throw;
    }
}

TimedTextMXFDescriptorHelper::TimedTextMXFDescriptorHelper()
: DataMXFDescriptorHelper()
{
    mEssenceType = TIMED_TEXT;

    BMX_OPT_PROP_DEFAULT(mResourceId, g_Null_UUID);
    BMX_OPT_PROP_DEFAULT(mLanguageList, "");
}

TimedTextMXFDescriptorHelper::~TimedTextMXFDescriptorHelper()
{
}

void TimedTextMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label)
{
    MXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mEssenceType = TIMED_TEXT;
}

void TimedTextMXFDescriptorHelper::SetManifest(TimedTextManifest *manifest)
{
    if (manifest->HaveResourceId()) {
        BMX_OPT_PROP_SET(mResourceId, manifest->GetResourceId());
    }
    mNamespaceURI = manifest->GetProfileDesignator();
    mUCSEncoding = manifest->GetEncoding();
    if (manifest->HaveLanguages()) {
        BMX_OPT_PROP_SET(mLanguageList, manifest->GetLanguagesString());
    }

    mAncillaryResources = manifest->GetAncillaryResources();
}

FileDescriptor* TimedTextMXFDescriptorHelper::CreateFileDescriptor(HeaderMetadata *header_metadata)
{
    mFileDescriptor = new DCTimedTextDescriptor(header_metadata);

    size_t i;
    for (i = 0; i < mAncillaryResources.size(); i++) {
        DCTimedTextResourceSubDescriptor *res_subdescriptor = new DCTimedTextResourceSubDescriptor(header_metadata);
        res_subdescriptor->setAncillaryResourceID(mAncillaryResources[i].resource_id);
        res_subdescriptor->setEssenceStreamID(mAncillaryResources[i].stream_id);
        res_subdescriptor->setMIMEType(mAncillaryResources[i].mime_type);
        mFileDescriptor->appendSubDescriptors(res_subdescriptor);
    }

    UpdateFileDescriptor();

    return mFileDescriptor;
}

void TimedTextMXFDescriptorHelper::UpdateFileDescriptor()
{
    DataMXFDescriptorHelper::UpdateFileDescriptor();

    DCTimedTextDescriptor *tt_descriptor = dynamic_cast<DCTimedTextDescriptor*>(mFileDescriptor);
    BMX_ASSERT(tt_descriptor);

    if (BMX_OPT_PROP_IS_SET(mResourceId))
        tt_descriptor->setResourceID(mResourceId);
    tt_descriptor->setNamespaceURI(mNamespaceURI);
    tt_descriptor->setUCSEncoding(mUCSEncoding);
    if (BMX_OPT_PROP_IS_SET(mLanguageList))
        tt_descriptor->setRFC5646LanguageTagList(mLanguageList);
}

mxfUL TimedTextMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return MXF_EC_L(TimedText);
}
