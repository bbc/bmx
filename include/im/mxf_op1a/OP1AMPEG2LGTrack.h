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

#ifndef __IM_OP1A_MPEG2LG_TRACK_H__
#define __IM_OP1A_MPEG2LG_TRACK_H__

#include <im/mxf_op1a/OP1APictureTrack.h>
#include <im/writer_helper/MPEG2LGWriterHelper.h>
#include <im/mxf_helper/MPEG2LGMXFDescriptorHelper.h>
#include <im/ByteArray.h>



namespace im
{


class OP1AMPEG2LGTrack : public OP1APictureTrack
{
public:
    OP1AMPEG2LGTrack(OP1AFile *file, uint32_t track_index, uint32_t track_id, uint8_t track_type_number,
                     mxfRational frame_rate, OP1AEssenceType essence_type);
    virtual ~OP1AMPEG2LGTrack();

    void SetSignalStandard(uint8_t signal_standard);    // 0x04=SMPTE274, 0x05=SMPTE-296; default=0x04
    void SetFrameLayout(uint8_t frame_layout);          // 0x00=FullFrame (progressive), 0x01=SeparateFields (interlaced), default=0x01

protected:
    virtual void PrepareWrite(uint8_t picture_track_count, uint8_t sound_track_count);
    virtual void WriteSamplesInt(const unsigned char *data, uint32_t size, uint32_t num_samples);
    virtual void CompleteWrite();

private:
    MPEG2LGMXFDescriptorHelper *mMPEG2LGDescriptorHelper;
    MPEG2LGWriterHelper mWriterHelper;
};


};



#endif

