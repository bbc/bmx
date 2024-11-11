/*
 * Copyright (C) 2024, British Broadcasting Corporation
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

#include <bmx/essence_parser/KLVEssenceReader.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


KLVEssenceReader::KLVEssenceReader(KLVEssenceSource *essence_source)
{
    mEssenceSource = essence_source;
}

KLVEssenceReader::~KLVEssenceReader()
{
    delete mEssenceSource;
}

uint32_t KLVEssenceReader::ReadValue()
{
    // Position at the next non-zero Value
    uint64_t v_size64 = 0;
    while (v_size64 == 0) {
        if (!mEssenceSource->PositionInV(&v_size64)) {
            mValueBuffer.SetSize(0);
            return 0;
        }
    }

    if (v_size64 > UINT32_MAX) {
        log_warn("KLV value size %" PRIu64 " > max uint32 is not supported\n", v_size64);
        mValueBuffer.SetSize(0);
        return 0;
    }
    uint32_t value_size = (uint32_t)v_size64;

    // Expect to be at the start of the V because the read below reads the whole V
    BMX_CHECK(mEssenceSource->GetOffsetInV() == 0);

    mValueBuffer.Allocate(value_size);

    uint32_t read_size = mEssenceSource->Read(mValueBuffer.GetBytes(), value_size);
    if (read_size != value_size) {
        log_warn("Incomplete KLV; only read %u of %u\n", read_size, value_size);
        mValueBuffer.SetSize(0);
        return 0;
    }

    mValueBuffer.SetSize(read_size);
    return read_size;
}
