/*
 * Copyright (C) 2022, British Broadcasting Corporation
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

#ifndef BMX_WAVE_CHNA_H_
#define BMX_WAVE_CHNA_H_


#include <vector>

#include <bmx/BMXTypes.h>
#include <bmx/wave/WaveIO.h>



namespace bmx
{


class WaveCHNA
{
public:
    WaveCHNA();
    ~WaveCHNA();

public:
    typedef struct {
        uint16_t track_index;
        uint8_t uid[12];
        uint8_t track_ref[14];
        uint8_t pack_ref[11];
    } AudioID;

public:
    void AppendAudioID(const AudioID &audio_id);

    uint32_t GetSize();
    uint32_t GetAlignedSize() { return GetSize(); }

    void Write(WaveIO *output);

public:
    uint16_t GetNumTracks() const                   { return mNumTracks; }
    uint16_t GetNumUIDs() const                     { return mNumUIDs; }
    const std::vector<AudioID>& GetAudioIDs() const { return mAudioIDs; }

    std::vector<AudioID> GetAudioIDs(uint16_t track_index) const;

    void Read(WaveIO *input, uint32_t size);

private:
    uint16_t mNumTracks;
    uint16_t mNumUIDs;
    std::vector<AudioID> mAudioIDs;
};


};



#endif

