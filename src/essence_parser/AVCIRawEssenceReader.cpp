/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#define __STDC_FORMAT_MACROS

#include <bmx/essence_parser/AVCIRawEssenceReader.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



AVCIRawEssenceReader::AVCIRawEssenceReader(EssenceSource *essence_source)
: RawEssenceReader(essence_source)
{
    SetEssenceParser(new AVCIEssenceParser());
    mLastSampleSize = 0;
}

AVCIRawEssenceReader::~AVCIRawEssenceReader()
{
}

uint32_t AVCIRawEssenceReader::ReadSamples(uint32_t num_samples)
{
    BMX_ASSERT(num_samples == 1);
    BMX_ASSERT(mFixedSampleSize != 0);

    if (mLastSampleRead)
        return 0;

    // shift data from previous read to start of sample data
    ShiftSampleData(0, mSampleDataSize);
    mSampleDataSize = 0;
    mNumSamples = 0;


    // read same size as previous frame assuming the size remains constant after the second frame
    uint32_t read_size;
    if (mLastSampleSize > 0)
        read_size = mLastSampleSize - mSampleBuffer.GetSize();
    else
        read_size = mFixedSampleSize - mSampleBuffer.GetSize();

    ReadBytes(read_size);
    if (mSampleBuffer.GetSize() < mFixedSampleSize - AVCI_HEADER_SIZE) {
        mLastSampleRead = true;
        return 0;
    }


    mAVCIParser->ParseFrameInfo(mSampleBuffer.GetBytes(), mSampleBuffer.GetSize());

    if (mAVCIParser->HaveSequenceParameterSet()) {
        if (mSampleBuffer.GetSize() < mFixedSampleSize) {
            if (ReadBytes(AVCI_HEADER_SIZE) != AVCI_HEADER_SIZE) {
                mLastSampleRead = true;
                return 0;
            }
        }
        mSampleDataSize = mFixedSampleSize;
    } else {
        mSampleDataSize = mFixedSampleSize - AVCI_HEADER_SIZE;
    }
    mNumSamples = 1;

    mLastSampleSize = mSampleDataSize;

    return 1;
}

void AVCIRawEssenceReader::SetEssenceParser(EssenceParser *essence_parser)
{
    BMX_CHECK(dynamic_cast<AVCIEssenceParser*>(essence_parser));

    if (mEssenceParser) {
        delete mEssenceParser;
        mEssenceParser = 0;
    }

    RawEssenceReader::SetEssenceParser(essence_parser);
    mAVCIParser = dynamic_cast<AVCIEssenceParser*>(essence_parser);
}

