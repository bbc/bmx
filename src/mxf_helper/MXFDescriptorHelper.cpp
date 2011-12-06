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

#include <im/mxf_helper/MXFDescriptorHelper.h>
#include <im/mxf_helper/PictureMXFDescriptorHelper.h>
#include <im/mxf_helper/SoundMXFDescriptorHelper.h>
#include <im/Utils.h>
#include <im/IMTypes.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;
using namespace mxfpp;



EssenceType MXFDescriptorHelper::IsSupported(mxfpp::FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    EssenceType essence_type = PictureMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
    if (essence_type)
        return essence_type;
    else
        return SoundMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label);
}

MXFDescriptorHelper* MXFDescriptorHelper::Create(mxfpp::FileDescriptor *file_descriptor,
                                                 mxfUL alternative_ec_label)
{
    if (PictureMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        return PictureMXFDescriptorHelper::Create(file_descriptor, alternative_ec_label);
    else if (SoundMXFDescriptorHelper::IsSupported(file_descriptor, alternative_ec_label))
        return SoundMXFDescriptorHelper::Create(file_descriptor, alternative_ec_label);

    IM_ASSERT(false);
    return 0;
}

MXFDescriptorHelper* MXFDescriptorHelper::Create(EssenceType essence_type)
{
    if (PictureMXFDescriptorHelper::IsSupported(essence_type))
        return PictureMXFDescriptorHelper::Create(essence_type);
    else if (SoundMXFDescriptorHelper::IsSupported(essence_type))
        return SoundMXFDescriptorHelper::Create(essence_type);

    IM_ASSERT(false);
    return 0;
}

MXFDescriptorHelper::MXFDescriptorHelper()
{
    mSampleRate = FRAME_RATE_25;
    mFrameWrapped = true;
    mFileDescriptor = 0;
    mFlavour = SMPTE_377_2004_FLAVOUR;

    // mEssenceType is set by subclass
}

MXFDescriptorHelper::~MXFDescriptorHelper()
{
}

void MXFDescriptorHelper::Initialize(FileDescriptor *file_descriptor, mxfUL alternative_ec_label)
{
    (void)alternative_ec_label;

    mSampleRate = file_descriptor->getSampleRate();
    // mEssenceType and mFrameWrapped are set by subclass

    mFileDescriptor = file_descriptor;
}

void MXFDescriptorHelper::SetEssenceType(EssenceType essence_type)
{
    IM_ASSERT(!mFileDescriptor);

    mEssenceType = essence_type;
}

void MXFDescriptorHelper::SetSampleRate(mxfRational sample_rate)
{
    IM_ASSERT(!mFileDescriptor);
    IM_CHECK((sample_rate.numerator == 25    && sample_rate.denominator == 1) ||
             (sample_rate.numerator == 30000 && sample_rate.denominator == 1001) ||
             (sample_rate.numerator == 50    && sample_rate.denominator == 1) ||
             (sample_rate.numerator == 60000 && sample_rate.denominator == 1001) ||
             (sample_rate.numerator == 60    && sample_rate.denominator == 1) ||
             (IsSound() &&
                 (sample_rate.numerator == 48000 && sample_rate.denominator == 1)));

    mSampleRate = sample_rate;
}

void MXFDescriptorHelper::SetFrameWrapped(bool frame_wrapped)
{
    IM_ASSERT(!mFileDescriptor);

    mFrameWrapped = frame_wrapped;
}

void MXFDescriptorHelper::SetFlavour(DescriptorFlavour flavour)
{
    IM_ASSERT(!mFileDescriptor);

    mFlavour = flavour;
    if (flavour == AVID_FLAVOUR)
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

