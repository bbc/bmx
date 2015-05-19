/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#ifndef BMX_OP1A_VC2_TRACK_H_
#define BMX_OP1A_VC2_TRACK_H_

#include <bmx/mxf_helper/VC2MXFDescriptorHelper.h>
#include <bmx/writer_helper/VC2WriterHelper.h>
#include <bmx/mxf_op1a/OP1APictureTrack.h>



namespace bmx
{


class OP1AVC2Track : public OP1APictureTrack
{
public:
    OP1AVC2Track(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                 mxfRational frame_rate);
    virtual ~OP1AVC2Track();

    void SetModeFlags(int flags);
    void SetSequenceHeader(const unsigned char *data, uint32_t size);

protected:
    virtual void PrepareWrite(uint8_t track_count);
    virtual void WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples);
    virtual void CompleteWrite();

private:
    VC2MXFDescriptorHelper *mVC2DescriptorHelper;
    VC2WriterHelper mWriterHelper;
};



};



#endif
