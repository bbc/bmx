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

#ifndef BMX_MXF_INDEX_ENTRY_EXT_H_
#define BMX_MXF_INDEX_ENTRY_EXT_H_


#include <bmx/BMXTypes.h>


#define RANDOM_ACCESS_FLAG      0x80
#define SEQUENCE_HEADER_FLAG    0x40
#define FORWARD_PREDICT_FLAG    0x20
#define BACKWARD_PREDICT_FLAG   0x10

#define FRAME_TYPE_FLAG_MASK    0x30
#define I_FRAME_FLAG            0x00
#define P_FRAME_FLAG            0x20
#define B_BACKWARD_FRAME_FLAG   0x10
#define B_FRAME_FLAG            0x30



namespace bmx
{


class MXFIndexEntryExt
{
public:
    MXFIndexEntryExt();

    int64_t container_offset;
    int64_t file_offset;
    int64_t edit_unit_size;
    int8_t temporal_offset;
    int8_t key_frame_offset;
    uint8_t flags;
};


};



#endif

