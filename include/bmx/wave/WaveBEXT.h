/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef BMX_WAVE_BEXT_H_
#define BMX_WAVE_BEXT_H_


#include <vector>
#include <string>

#include <bmx/BMXTypes.h>
#include <bmx/wave/WaveIO.h>



namespace bmx
{


class WaveBEXT
{
public:
    WaveBEXT();
    ~WaveBEXT();

public:
    void SetDescription(std::string description);           // default ""
    void SetOriginator(std::string originator);             // default ""
    void SetOriginatorReference(std::string reference);     // default ""
    void SetOriginatorTimestamp(Timestamp timestamp);       // default 'now'
    void SetTimeReference(uint64_t reference);              // default 0
    void SetUMID(UMID umid);                                // default generated
    void SetLoudnessValue(int16_t value);                   // default not set or 0
    void SetLoudnessRange(int16_t range);                   // default not set or 0
    void SetMaxTruePeakLevel(int16_t level);                // default not set or 0
    void SetMaxMomentaryLoudness(int16_t loudness);         // default not set or 0
    void SetMaxShortTermLoudness(int16_t loudness);         // default not set or 0

    void AppendCodingHistory(std::string line);             // default ""
    void AppendCodingHistory(std::vector<std::pair<std::string, std::string> > line);

    uint32_t GetSize();
    bool WasUpdated() { return mWasUpdated; }

    void Write(WaveIO *output);

public:
    std::string GetDescription()          { return mDescription; }
    std::string GetOriginator()           { return mOriginator; }
    std::string GetOriginatorReference()  { return mOriginatorReference; }
    Timestamp GetOriginatorTimestamp()    { return mOriginatorTimestamp; }
    uint64_t GetTimeReference()           { return mTimeReference; }
    uint16_t GetVersion()                 { return mVersion; }
    unsigned char* GetUMID64()            { return mUMID64; }
    int16_t GetLoudnessValue()            { return mLoudnessValue; }
    int16_t GetLoudnessRange()            { return mLoudnessRange; }
    int16_t GetMaxTruePeakLevel()         { return mMaxTruePeakLevel; }
    int16_t GetMaxMomentaryLoudness()     { return mMaxMomentaryLoudness; }
    int16_t GetMaxShortTermLoudness()     { return mMaxShortTermLoudness; }

    std::string GetCodingHistory()        { return mCodingHistory; }

    void Read(WaveIO *input, uint32_t size);

private:
    std::string ConvertToASCII(std::string input);

private:
    char mDescription[256 + 1];
    char mOriginator[32 + 1];
    char mOriginatorReference[32 + 1];
    Timestamp mOriginatorTimestamp;
    uint64_t mTimeReference;

    uint16_t mVersion;
    // version 1 extensions
    unsigned char mUMID64[64];
    // version 2 extensions
    int16_t mLoudnessValue;
    int16_t mLoudnessRange;
    int16_t mMaxTruePeakLevel;
    int16_t mMaxMomentaryLoudness;
    int16_t mMaxShortTermLoudness;

    std::string mCodingHistory;

    bool mWasUpdated;
    uint32_t mWrittenSize;
};


};



#endif

