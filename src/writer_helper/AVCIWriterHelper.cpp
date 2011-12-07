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

#include <cstring>

#include <im/writer_helper/AVCIWriterHelper.h>
#include <im/IMException.h>
#include <im/Logging.h>

using namespace std;
using namespace im;



AVCIWriterHelper::AVCIWriterHelper()
{
    mMode = AVCI_PASS_MODE;
    mHeader = 0;
    mSampleCount = 0;
    mFirstFrameHeader = false;
    mSecondFrameHeader = false;

    IM_ASSERT(sizeof(AVCI_FILLER) == sizeof(AVCI_ACCESS_UNIT_DELIMITER));
}

AVCIWriterHelper::~AVCIWriterHelper()
{
    delete [] mHeader;
}

void AVCIWriterHelper::SetMode(AVCIMode mode)
{
    mMode = mode;
}

void AVCIWriterHelper::SetHeader(const unsigned char *data, uint32_t size)
{
    IM_CHECK(size == AVCI_HEADER_SIZE);

    if (!mHeader)
        mHeader = new unsigned char[AVCI_HEADER_SIZE];

    memcpy(mHeader, data, AVCI_HEADER_SIZE);
}

uint32_t AVCIWriterHelper::ProcessFrame(const unsigned char *data, uint32_t size,
                                        const CDataBuffer **data_array, uint32_t *array_size)
{
    mEssenceParser.ParseFrameInfo(data, size);
    bool have_header = mEssenceParser.HaveSequenceParameterSet();

    uint32_t output_frame_size = 0;
    switch (mMode)
    {
        case AVCI_PASS_MODE:
            output_frame_size = PassFrame(data, size, array_size);
            break;
        case AVCI_NO_FRAME_HEADER_MODE:
            if (have_header)
                output_frame_size = StripFrameHeader(data, size, array_size);
            else
                output_frame_size = PassFrame(data, size, array_size);
            break;
        case AVCI_NO_OR_ALL_FRAME_HEADER_MODE:
            if (have_header) {
                if (mSampleCount == 0) {
                    mFirstFrameHeader = true;
                    SetHeader(data, AVCI_HEADER_SIZE);
                }

                if (mFirstFrameHeader)
                    output_frame_size = PassFrame(data, size, array_size);
                else
                    output_frame_size = StripFrameHeader(data, size, array_size);
            } else {
                if (mFirstFrameHeader)
                    output_frame_size = PrependFrameHeader(data, size, array_size);
                else
                    output_frame_size = PassFrame(data, size, array_size);
            }
            break;
        case AVCI_FIRST_FRAME_HEADER_MODE:
            if (have_header) {
                if (mSampleCount == 0) {
                    output_frame_size = PassFrame(data, size, array_size);
                    SetHeader(data, AVCI_HEADER_SIZE);
                } else {
                    output_frame_size = StripFrameHeader(data, size, array_size);
                }
            } else {
                if (mSampleCount == 0) {
                    if (!mHeader) {
                        IM_EXCEPTION(("Missing AVCI header sets in first frame"));
                    }
                    output_frame_size = PrependFrameHeader(data, size, array_size);
                } else {
                    output_frame_size = PassFrame(data, size, array_size);
                }
            }
            break;
        case AVCI_FIRST_OR_ALL_FRAME_HEADER_MODE:
            if (have_header) {
                if (mSampleCount == 0) {
                    output_frame_size = PassFrame(data, size, array_size);
                    SetHeader(data, AVCI_HEADER_SIZE);
                } else if (mSampleCount == 1) {
                    mSecondFrameHeader = true;
                    output_frame_size = PassFrame(data, size, array_size);
                } else {
                    if (mSecondFrameHeader)
                        output_frame_size = PassFrame(data, size, array_size);
                    else
                        output_frame_size = StripFrameHeader(data, size, array_size);
                }
            } else {
                if (mSampleCount == 0) {
                    if (!mHeader) {
                        IM_EXCEPTION(("Missing AVCI header sets in first frame"));
                    }
                    output_frame_size = PrependFrameHeader(data, size, array_size);
                } else if (mSampleCount == 1) {
                    mSecondFrameHeader = false;
                    output_frame_size = PassFrame(data, size, array_size);
                } else if (mSecondFrameHeader) {
                    output_frame_size = PrependFrameHeader(data, size, array_size);
                } else {
                    output_frame_size = PassFrame(data, size, array_size);
                }
            }
            break;
        case AVCI_ALL_FRAME_HEADER_MODE:
            if (have_header) {
                if (mSampleCount == 0)
                    SetHeader(data, AVCI_HEADER_SIZE);
                output_frame_size = PassFrame(data, size, array_size);
            } else {
                if (!mHeader) {
                    IM_EXCEPTION(("Missing AVCI header sets in first frame"));
                }
                output_frame_size = PrependFrameHeader(data, size, array_size);
            }
            break;
    }

    mSampleCount++;

    *data_array = mDataArray;
    return output_frame_size;
}

uint32_t AVCIWriterHelper::PassFrame(const unsigned char *data, uint32_t size, uint32_t *array_size)
{
    // ensure frame starts with access unit delimiter
    if (memcmp(data, AVCI_ACCESS_UNIT_DELIMITER, sizeof(AVCI_ACCESS_UNIT_DELIMITER)) == 0) {
        mDataArray[0].data = (unsigned char*)data;
        mDataArray[0].size = size;
        *array_size = 1;
    } else if (memcmp(data, AVCI_FILLER, sizeof(AVCI_FILLER)) == 0) {
        mDataArray[0].data = (unsigned char*)AVCI_ACCESS_UNIT_DELIMITER;
        mDataArray[0].size = sizeof(AVCI_ACCESS_UNIT_DELIMITER);
        mDataArray[1].data = (unsigned char*)(data + sizeof(AVCI_ACCESS_UNIT_DELIMITER));
        mDataArray[1].size = size - sizeof(AVCI_ACCESS_UNIT_DELIMITER);
        *array_size = 2;
    } else {
        IM_EXCEPTION(("AVCI frame does not start with access unit delimiter or filler"));
    }

    return size;
}

uint32_t AVCIWriterHelper::StripFrameHeader(const unsigned char *data, uint32_t size, uint32_t *array_size)
{
    // strip header and ensure frame starts with access unit delimiter
    if (memcmp(data + AVCI_HEADER_SIZE, AVCI_ACCESS_UNIT_DELIMITER, sizeof(AVCI_ACCESS_UNIT_DELIMITER)) == 0) {
        mDataArray[0].data = (unsigned char*)(data + AVCI_HEADER_SIZE);
        mDataArray[0].size = size - AVCI_HEADER_SIZE;
        *array_size = 1;
    } else if (memcmp(data + AVCI_HEADER_SIZE, AVCI_FILLER, sizeof(AVCI_FILLER)) == 0) {
        mDataArray[0].data = (unsigned char*)AVCI_ACCESS_UNIT_DELIMITER;
        mDataArray[0].size = sizeof(AVCI_ACCESS_UNIT_DELIMITER);
        mDataArray[1].data = (unsigned char*)(data + AVCI_HEADER_SIZE + sizeof(AVCI_ACCESS_UNIT_DELIMITER));
        mDataArray[1].size = size - AVCI_HEADER_SIZE - sizeof(AVCI_ACCESS_UNIT_DELIMITER);
        *array_size = 2;
    } else {
        IM_EXCEPTION(("Failed to strip AVCI header because filler or access unit delimiter "
                      "not found at offset %u", AVCI_HEADER_SIZE));
    }

    return size - AVCI_HEADER_SIZE;
}

uint32_t AVCIWriterHelper::PrependFrameHeader(const unsigned char *data, uint32_t size, uint32_t *array_size)
{
    // prepend header and replace access unit delimiter with filler
    IM_ASSERT(mHeader);
    if (memcmp(data, AVCI_ACCESS_UNIT_DELIMITER, sizeof(AVCI_ACCESS_UNIT_DELIMITER)) == 0) {
        mDataArray[0].data = mHeader;
        mDataArray[0].size = AVCI_HEADER_SIZE;
        mDataArray[1].data = (unsigned char*)AVCI_FILLER;
        mDataArray[1].size = sizeof(AVCI_FILLER);
        mDataArray[2].data = (unsigned char*)(data + sizeof(AVCI_FILLER));
        mDataArray[2].size = size - sizeof(AVCI_FILLER);
        *array_size = 3;
    } else {
        mDataArray[0].data = mHeader;
        mDataArray[0].size = AVCI_HEADER_SIZE;
        mDataArray[1].data = (unsigned char*)data;
        mDataArray[1].size = size;
        *array_size = 2;
    }

    return size + AVCI_HEADER_SIZE;
}
