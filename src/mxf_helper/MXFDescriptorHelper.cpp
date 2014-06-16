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

#include <bmx/mxf_helper/MXFDescriptorHelper.h>
#include <bmx/mxf_helper/PictureMXFDescriptorHelper.h>
#include <bmx/mxf_helper/SoundMXFDescriptorHelper.h>
#include <bmx/mxf_helper/DataMXFDescriptorHelper.h>
#include <bmx/Utils.h>
#include <bmx/BMXTypes.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

#include <mxf/mxf_avid_labels_and_keys.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



EssenceType MXFDescriptorHelper::IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    EssenceType essence_type = PictureMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = SoundMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    essence_type = DataMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    return essence_type;
}

MXFDescriptorHelper* MXFDescriptorHelper::Create(mxfpp::FileDescriptor *file_descriptor, uint16_t mxf_version,
                                                 mxfUL alternative_ec_label)
{
    if (PictureMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        return PictureMXFDescriptorHelper::Create(file_descriptor, mxf_version, alternative_ec_label);
    else if (SoundMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        return SoundMXFDescriptorHelper::Create(file_descriptor, mxf_version, alternative_ec_label);
    else if (DataMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        return DataMXFDescriptorHelper::Create(file_descriptor, mxf_version, alternative_ec_label);

    BMX_ASSERT(false);
    return 0;
}

MXFDescriptorHelper* MXFDescriptorHelper::Create(EssenceType essence_type)
{
    if (PictureMXFDescriptorHelper::IsSupported(essence_type))
        return PictureMXFDescriptorHelper::Create(essence_type);
    else if (SoundMXFDescriptorHelper::IsSupported(essence_type))
        return SoundMXFDescriptorHelper::Create(essence_type);
    else if (DataMXFDescriptorHelper::IsSupported(essence_type))
        return DataMXFDescriptorHelper::Create(essence_type);

    BMX_ASSERT(false);
    return 0;
}

bool MXFDescriptorHelper::CompareECULs(mxfUL ec_label_a, mxfUL alternative_ec_label_a, mxfUL ec_label_b)
{
    return mxf_equals_ul_mod_regver(&ec_label_a, &ec_label_b) ||
           mxf_equals_ul_mod_regver(&alternative_ec_label_a, &ec_label_b);
}

bool MXFDescriptorHelper::IsNullAvidECUL(mxfUL ec_label, mxfUL alternative_ec_label)
{
    return mxf_equals_ul_mod_regver(&ec_label, &MXF_EC_L(AvidAAFKLVEssenceContainer)) &&
           mxf_equals_ul(&alternative_ec_label, &g_Null_UL);
}

MXFDescriptorHelper::MXFDescriptorHelper()
{
    mSampleRate = FRAME_RATE_25;
    mFrameWrapped = true;
    mFileDescriptor = 0;
    mFlavour = MXFDESC_SMPTE_377_2004_FLAVOUR;

    // mEssenceType is set by subclass
}

MXFDescriptorHelper::~MXFDescriptorHelper()
{
}

void MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, uint16_t mxf_version, mxfUL alternative_ec_label)
{
    (void)mxf_version;
    (void)alternative_ec_label;

    mSampleRate = normalize_rate(file_descriptor->getSampleRate());
    // mEssenceType and mFrameWrapped are set by subclass

    mFileDescriptor = file_descriptor;
}

void MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    BMX_ASSERT(!mFileDescriptor);

    mEssenceType = essence_type;
}

void MXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    BMX_ASSERT(!mFileDescriptor);
    BMX_CHECK(sample_rate == FRAME_RATE_23976 ||
              sample_rate == FRAME_RATE_24 ||
              sample_rate == FRAME_RATE_25 ||
              sample_rate == FRAME_RATE_2997 ||
              sample_rate == FRAME_RATE_30 ||
              sample_rate == FRAME_RATE_50 ||
              sample_rate == FRAME_RATE_5994 ||
              sample_rate == FRAME_RATE_60 ||
              (IsSound() && sample_rate == SAMPLING_RATE_48K));

    mSampleRate = sample_rate;
}

void MXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    BMX_ASSERT(!mFileDescriptor);

    mFrameWrapped = frame_wrapped;
}

void MXFDescriptorHelper::SetFlavour(int flavour)
{
    BMX_ASSERT(!mFileDescriptor);

    mFlavour = flavour;
    if ((flavour & MXFDESC_AVID_FLAVOUR))
        mFrameWrapped = false;
}

void MXFDescriptorHelper::UpdateFileDescriptor()
{
    mFileDescriptor->setEssenceContainer(ChooseEssenceContainerUL());
    mFileDescriptor->setSampleRate(mSampleRate);
}

mxfUL MXFDescriptorHelper::GetEssenceContainerUL() const
{
    if (mFileDescriptor)
        return mFileDescriptor->getEssenceContainer();
    else
        return ChooseEssenceContainerUL();
}

