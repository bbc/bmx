/*
 * Copyright (C) 2023, British Broadcasting Corporation
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

#include <bmx/mxf_helper/MGAMXFDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


typedef struct
{
    mxfUL ec_label;
    EssenceType essence_type;
    bool frame_wrapped;
} SupportedEssence;

static const SupportedEssence SUPPORTED_ESSENCE[] =
{
    {MXF_EC_L(MGAFrameWrapped),  MGA,     true},
    {MXF_EC_L(MGAClipWrapped),   MGA,     false},
};

static const UL SADM_METADATA_PAYLOAD_UL = {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x04, 0x02, 0x12, 0x00, 0x00, 0x00, 0x00};



EssenceType MGAMXFDescriptorHelper::IsSupported(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    GenericSoundEssenceDescriptor *sound_descriptor = dynamic_cast<GenericSoundEssenceDescriptor*>(file_descriptor);
    if (!sound_descriptor || !sound_descriptor->haveSoundEssenceCompression())
        return UNKNOWN_ESSENCE_TYPE;

    mxfUL pc_label = sound_descriptor->getSoundEssenceCompression();
    if (!mxf_equals_ul_mod_regver(&pc_label, &MXF_CMDEF_L(MGA_UNC_SOUND)))
        return UNKNOWN_ESSENCE_TYPE;

    EssenceType essence_type = UNKNOWN_ESSENCE_TYPE;
    mxfUL ec_label = file_descriptor->getEssenceContainer();
    for (size_t i = 0; i < BMX_ARRAY_SIZE(SUPPORTED_ESSENCE); i++) {
        if (CompareECULs(ec_label, alternative_ec_label, SUPPORTED_ESSENCE[i].ec_label)) {
            essence_type = SUPPORTED_ESSENCE[i].essence_type;
            break;
        }
    }
    if (essence_type == UNKNOWN_ESSENCE_TYPE)
        return UNKNOWN_ESSENCE_TYPE;

    // Identify it as S-ADM if there is an S-ADM payload
    if (file_descriptor->haveSubDescriptors()) {
        vector<SubDescriptor*> sub_descriptors = file_descriptor->getSubDescriptors();
        for (size_t i = 0; i < sub_descriptors.size(); i++) {
            MGAAudioMetadataSubDescriptor *mga_sub_desc = dynamic_cast<MGAAudioMetadataSubDescriptor*>(sub_descriptors[i]);
            if (mga_sub_desc) {
                vector<UL> payload_uls = mga_sub_desc->getMGAAudioMetadataPayloadULArray();
                for (size_t k = 0; k < payload_uls.size(); k++) {
                    if (payload_uls[k] == SADM_METADATA_PAYLOAD_UL) {
                        essence_type = MGA_SADM;
                        break;
                    }
                }
                if (essence_type == MGA_SADM)
                    break;
            }
        }
    }

    return essence_type;
}

bool MGAMXFDescriptorHelper::IsSupported(EssenceType essence_type)
{
    return essence_type == MGA || essence_type == MGA_SADM;
}

MGAMXFDescriptorHelper::MGAMXFDescriptorHelper()
: SoundMXFDescriptorHelper()
{
    mEssenceType = MGA;
}

MGAMXFDescriptorHelper::~MGAMXFDescriptorHelper()
{
}
