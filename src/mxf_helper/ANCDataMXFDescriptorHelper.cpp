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

#include <bmx/mxf_helper/ANCDataMXFDescriptorHelper.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



EssenceType ANCDataMXFDescriptorHelper::IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    (void)alternative_ec_label;

    mxfUL ec_label = file_descriptor->getEssenceContainer();
    if (!mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(ANCData)))
        return UNKNOWN_ESSENCE_TYPE;

    ANCDataDescriptor *anc_descriptor = dynamic_cast<ANCDataDescriptor*>(file_descriptor);
    if (!anc_descriptor)
        return UNKNOWN_ESSENCE_TYPE;

    return ANC_DATA;
}

bool ANCDataMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return essence_type == ANC_DATA;
}

ANCDataMXFDescriptorHelper::ANCDataMXFDescriptorHelper()
: DataMXFDescriptorHelper()
{
    mEssenceType = ANC_DATA;
}

ANCDataMXFDescriptorHelper::~ANCDataMXFDescriptorHelper()
{
}

void ANCDataMXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label)
{
    DataMXFDescriptorHelper::Initialize(file_descriptor, mxf_version, alternative_ec_label);

    mEssenceType = ANC_DATA;
}

FileDescriptor* ANCDataMXFDescriptorHelper::CreateFileDescriptor(HeaderMetadata *header_metadata)
{
    mFileDescriptor = new ANCDataDescriptor(header_metadata);
    UpdateFileDescriptor();
    return mFileDescriptor;
}

mxfUL ANCDataMXFDescriptorHelper::ChooseEssenceContainerUL() const
{
    return MXF_EC_L(ANCData);
}

