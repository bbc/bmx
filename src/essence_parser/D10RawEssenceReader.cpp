/*
 * Copyright (C) 2021, British Broadcasting Corporation
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

#define __STDC_LIMIT_MACROS

#include <libMXF++/MXF.h>

#include <bmx/essence_parser/D10RawEssenceReader.h>
#include <bmx/essence_parser/KLVEssenceSource.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


static const mxfKey BASE_D10_ELEMENT_KEY = MXF_D10_PICTURE_EE_K(0x00);


D10RawEssenceReader::D10RawEssenceReader(EssenceSource *essence_source)
: RawEssenceReader(essence_source)
{
    mHaveCheckedKL = false;
}

D10RawEssenceReader::~D10RawEssenceReader()
{
}

uint32_t D10RawEssenceReader::ReadSamples(uint32_t num_samples)
{
    // For the first read, check whether the data starts with a D-10 MXF K.
    // If it does then insert the KLVEssenceSource so that only the V in the KLV is used.
    if (!mHaveCheckedKL) {
        mHaveCheckedKL = true;

        unsigned char buffer[16 + 9];
        uint32_t num_read = 0;

        // Read 16-bytes for the K
        num_read += mEssenceSource->Read(buffer, 17);
        if (num_read == 17) {
            mxfKey d10_key;
            mxf_get_ul(buffer, static_cast<mxfUL*>(&d10_key));
            if (mxf_equals_key_prefix(&d10_key, &BASE_D10_ELEMENT_KEY, 15)) {
                // Insert the KLVEssenceSource, and initialise it to start from the V in KLV

                // Read and calculate the L in KLV
                uint32_t llen = 1;
                if ((buffer[16] & 0x80))
                    llen += buffer[16] & 0x7f;
                if (llen > 9)
                    BMX_EXCEPTION(("Length of L in D-10 frame KL prefix exceeds 9 bytes"));

                uint64_t len = 0;
                if (llen == 1) {
                    len = buffer[16];
                } else {
                    num_read += mEssenceSource->Read(&buffer[17], llen - 1);
                    if (num_read == 16 + llen) {
                        uint32_t i;
                        for (i = 1; i < llen; i++) {
                            len <<= 8;
                            len |= buffer[16 + i];
                        }
                    }
                }

                // Insert the KLVEssenceSource if L could be read
                if (num_read == 16 + llen && len <= INT64_MAX) {
                    mEssenceSource = new KLVEssenceSource(mEssenceSource, &d10_key, len);
                    log_warn("Using a KLV parser for the D-10 after finding a D-10 MXF KL at the start\n");
                }
            }
        }

        // If no KLVEssenceSource was inserted then pass the bytes read to the
        // RawEssenceReader parent class
        if (!dynamic_cast<KLVEssenceSource*>(mEssenceSource) && num_read > 0) {
            AppendBytes(buffer, num_read);
        }
    }

    return RawEssenceReader::ReadSamples(num_samples);
}
