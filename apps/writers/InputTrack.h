/*
 * Copyright (C) 2016, British Broadcasting Corporation
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


#ifndef BMX_INPUT_TRACK_H_
#define BMX_INPUT_TRACK_H_

#include <vector>

#include <bmx/BMXTypes.h>


namespace bmx
{


class OutputTrack;

class InputTrack
{
public:
    InputTrack();
    virtual ~InputTrack();

    void AddOutput(OutputTrack *output_track, uint32_t output_channel_index, uint32_t input_channel_index);

public:
    virtual MXFDataDefEnum GetDataDef() = 0;

public:
    size_t GetOutputTrackCount();
    OutputTrack* GetOutputTrack(size_t track_index);
    uint32_t GetOutputChannelIndex(size_t track_index);
    uint32_t GetInputChannelIndex(size_t track_index);

protected:
    typedef struct
    {
        OutputTrack *output_track;
        uint32_t output_channel_index;
        uint32_t input_channel_index;
    } OutputMap;

protected:
    std::vector<OutputMap> mOutputMaps;
};


};


#endif
