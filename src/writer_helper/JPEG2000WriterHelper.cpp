/*
 * Copyright (C) 2020, British Broadcasting Corporation
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

#include <bmx/writer_helper/JPEG2000WriterHelper.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


JPEG2000WriterHelper::JPEG2000WriterHelper()
{
    mPosition = 0;
    mDescriptorHelper = 0;
    mCodingStyleDefault = new ByteArray();
    mQuantizationDefault = new ByteArray();
}

JPEG2000WriterHelper::~JPEG2000WriterHelper()
{
    delete mCodingStyleDefault;
    delete mQuantizationDefault;
}

void JPEG2000WriterHelper::SetDescriptorHelper(JPEG2000MXFDescriptorHelper *descriptor_helper)
{
    mDescriptorHelper = descriptor_helper;
}

void JPEG2000WriterHelper::ProcessFrame(const unsigned char *data, uint32_t size)
{
    mPosition++;

    if (!mCodingStyleDefault && !mQuantizationDefault)
        return;

    mEssenceParser.ParseFrameInfo(data, size);

    if (mPosition == 1)  // the first frame. mPosition has already been incremented
        mDescriptorHelper->UpdateFileDescriptor(&mEssenceParser);

    if (mCodingStyleDefault) {
        bool is_static = false;
        unsigned char *coding_style = 0;
        size_t size = 10 + mEssenceParser.GetSPcodPrecintSizes().size();
        if (mCodingStyleDefault->GetSize() == 0 || mCodingStyleDefault->GetSize() == size) {
            coding_style = new unsigned char[size];
            coding_style[0] = mEssenceParser.GetScod();
            coding_style[1] = mEssenceParser.GetSGcodProgOrder();
            coding_style[2] = (uint8_t)((mEssenceParser.GetSGcodNumLayers() >> 8) & 0xff);
            coding_style[3] = (uint8_t)(mEssenceParser.GetSGcodNumLayers() & 0xff);
            coding_style[4] = mEssenceParser.GetSGcodTransformUsage();
            coding_style[5] = mEssenceParser.GetSPcodNumLevels();
            coding_style[6] = mEssenceParser.GetSPcodWidth();
            coding_style[7] = mEssenceParser.GetSPcodHeight();
            coding_style[8] = mEssenceParser.GetSPcodStyle();
            coding_style[9] = mEssenceParser.GetSPcodTransformType();
            for (size_t i = 0; i < mEssenceParser.GetSPcodPrecintSizes().size(); i++)
                coding_style[10 + i] = mEssenceParser.GetSPcodPrecintSizes()[i];

            is_static = true;
            for (size_t i = 0; i < mCodingStyleDefault->GetSize(); i++) {
                if (coding_style[i] != mCodingStyleDefault->GetBytes()[i]) {
                    is_static = false;
                    break;
                }
            }
        }

        if (is_static) {
            if (mCodingStyleDefault->GetSize() == 0)
                mCodingStyleDefault->Append(coding_style, (uint32_t)size);
        } else {
            delete mCodingStyleDefault;
            mCodingStyleDefault = 0;
        }
        delete [] coding_style;
    }

    if (mQuantizationDefault) {
        bool is_static = false;
        unsigned char *quantization = 0;
        size_t size = 1 + mEssenceParser.GetSPqcd().size();
        if (mQuantizationDefault->GetSize() == 0 || mQuantizationDefault->GetSize() == size) {
            quantization = new unsigned char[size];
            quantization[0] = mEssenceParser.GetSqcd();
            for (size_t i = 0; i < mEssenceParser.GetSPqcd().size(); i++)
                quantization[1 + i] = mEssenceParser.GetSPqcd()[i];

            is_static = true;
            for (size_t i = 0; i < mQuantizationDefault->GetSize(); i++) {
                if (quantization[i] != mQuantizationDefault->GetBytes()[i]) {
                    is_static = false;
                    break;
                }
            }
        }

        if (is_static) {
            if (mQuantizationDefault->GetSize() == 0)
                mQuantizationDefault->Append(quantization, (uint32_t)size);
        } else {
            delete mQuantizationDefault;
            mQuantizationDefault = 0;
        }
        delete [] quantization;
    }
}

void JPEG2000WriterHelper::CompleteProcess()
{
    JPEG2000SubDescriptor *jpeg2000_sub_desc = dynamic_cast<JPEG2000MXFDescriptorHelper*>(mDescriptorHelper)->GetJPEG2000SubDescriptor();

    if (mCodingStyleDefault && mCodingStyleDefault->GetSize() > 0) {
        mxfpp::ByteArray mxf_byte_array;
        mxf_byte_array.data = mCodingStyleDefault->GetBytes();
        mxf_byte_array.length = (uint16_t)mCodingStyleDefault->GetSize();
        jpeg2000_sub_desc->setCodingStyleDefault(mxf_byte_array);
    }

    if (mQuantizationDefault && mQuantizationDefault->GetSize() > 0) {
        mxfpp::ByteArray mxf_byte_array;
        mxf_byte_array.data = mQuantizationDefault->GetBytes();
        mxf_byte_array.length = (uint16_t)mQuantizationDefault->GetSize();
        jpeg2000_sub_desc->setQuantizationDefault(mxf_byte_array);
    }
}
