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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_LIMIT_MACROS

#include <cstring>
#include <cstdio>

#include <bmx/wave/WaveBEXT.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


// string size equals var size - extra null terminator
#define STRING_SIZE(var)   (sizeof(var) - 1)


namespace bmx
{
extern bool BMX_REGRESSION_TEST;
};



WaveBEXT::WaveBEXT()
{
    SetDescription("");
    SetOriginator("");
    SetOriginatorReference("");
    if (BMX_REGRESSION_TEST)
        memset(&mOriginatorTimestamp, 0, sizeof(mOriginatorTimestamp));
    else
        SetOriginatorTimestamp(generate_timestamp_now());
    mTimeReference = 0;
    mVersion = 0;
    if (BMX_REGRESSION_TEST)
        memset(mUMID64, 0, sizeof(mUMID64));
    else
        SetUMID(generate_umid());
    mLoudnessValue = 0;
    mLoudnessRange = 0;
    mMaxTruePeakLevel = 0;
    mMaxMomentaryLoudness = 0;
    mMaxShortTermLoudness = 0;
    mWasUpdated = false;
    mWrittenSize = 0;
}

WaveBEXT::~WaveBEXT()
{
}

void WaveBEXT::SetDescription(string description)
{
    string ascii = ConvertToASCII(description);

    strncpy(mDescription, ascii.c_str(), STRING_SIZE(mDescription));
    if (ascii.size() < STRING_SIZE(mDescription))
        memset(mDescription + ascii.size(), 0, STRING_SIZE(mDescription) - ascii.size());

    mWasUpdated = true;
}

void WaveBEXT::SetOriginator(string originator)
{
    string ascii = ConvertToASCII(originator);

    strncpy(mOriginator, ascii.c_str(), STRING_SIZE(mOriginator));
    if (ascii.size() < STRING_SIZE(mOriginator))
        memset(mOriginator + ascii.size(), 0, STRING_SIZE(mOriginator) - ascii.size());

    mWasUpdated = true;
}

void WaveBEXT::SetOriginatorReference(string reference)
{
    string ascii = ConvertToASCII(reference);

    strncpy(mOriginatorReference, ascii.c_str(), STRING_SIZE(mOriginatorReference));
    if (ascii.size() < STRING_SIZE(mOriginatorReference))
        memset(mOriginatorReference + ascii.size(), 0, STRING_SIZE(mOriginatorReference) - ascii.size());

    mWasUpdated = true;
}

void WaveBEXT::SetOriginatorTimestamp(Timestamp timestamp)
{
    if ((timestamp.year  < 0  || timestamp.year  > 9999) ||
        (timestamp.month < 1  || timestamp.month > 12) ||
        (timestamp.day   < 1  || timestamp.day   > 31) ||
        timestamp.hour   > 23 ||
        timestamp.min    > 59 ||
        timestamp.sec    > 59)
    {
        log_warn("Ignoring invalid originator timestamp\n");
        return;
    }

    mOriginatorTimestamp = timestamp;

    mWasUpdated = true;
}

void WaveBEXT::SetTimeReference(uint64_t reference)
{
    mTimeReference = reference;

    mWasUpdated = true;
}

void WaveBEXT::SetUMID(UMID umid)
{
    memcpy(mUMID64, &umid.octet0, 32);
    memset(&mUMID64[32], 0, 32);

    if (mVersion < 1)
        mVersion = 1;

    mWasUpdated = true;
}

void WaveBEXT::SetLoudnessValue(int16_t value)
{
    mLoudnessValue = value;

    if (mVersion < 2)
        mVersion = 2;

    mWasUpdated = true;
}

void WaveBEXT::SetLoudnessRange(int16_t range)
{
    mLoudnessRange = range;

    if (mVersion < 2)
        mVersion = 2;

    mWasUpdated = true;
}

void WaveBEXT::SetMaxTruePeakLevel(int16_t level)
{
    mMaxTruePeakLevel = level;

    if (mVersion < 2)
        mVersion = 2;

    mWasUpdated = true;
}

void WaveBEXT::SetMaxMomentaryLoudness(int16_t loudness)
{
    mMaxMomentaryLoudness = loudness;

    if (mVersion < 2)
        mVersion = 2;

    mWasUpdated = true;
}

void WaveBEXT::SetMaxShortTermLoudness(int16_t loudness)
{
    mMaxShortTermLoudness = loudness;

    if (mVersion < 2)
        mVersion = 2;

    mWasUpdated = true;
}

void WaveBEXT::AppendCodingHistory(string line)
{
    mCodingHistory.append(ConvertToASCII(line)).append(1, 0x0d).append(1, 0x0a);

    mWasUpdated = true;
}

void WaveBEXT::AppendCodingHistory(vector<pair<string, string> > line)
{
    // append line: <name>=<value>,...
    string line_str;
    size_t i;
    for (i = 0; i < line.size(); i++) {
        if (i != 0)
            line_str.append(",");
        line_str.append(line[i].first).append("=").append(line[i].second);
    }

    AppendCodingHistory(line_str);

    mWasUpdated = true;
}

uint32_t WaveBEXT::GetSize()
{
    size_t coding_history_size = mCodingHistory.size();
    if (coding_history_size > 0) {
        if (602 + (uint64_t)coding_history_size > UINT32_MAX - 2)
            coding_history_size = UINT32_MAX - 2 - 602;
        coding_history_size += 1; // null terminator
    }

    return (uint32_t)(602 + coding_history_size);
}

void WaveBEXT::Write(WaveIO *output)
{
    uint32_t size = GetSize();
    uint32_t coding_history_size = size - 602;

    BMX_CHECK_M(mWrittenSize <= 0 || mWrittenSize == size, ("BEXT chunk size has changed"));

    output->WriteTag("bext");
    output->WriteSize(size);

    output->Write((const unsigned char*)mDescription, STRING_SIZE(mDescription));
    output->Write((const unsigned char*)mOriginator, STRING_SIZE(mOriginator));
    output->Write((const unsigned char*)mOriginatorReference, STRING_SIZE(mOriginatorReference));

    char buffer[32];
    bmx_snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u", mOriginatorTimestamp.year, mOriginatorTimestamp.month,
                 mOriginatorTimestamp.day);
    output->Write((const unsigned char*)buffer, 10);

    bmx_snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", mOriginatorTimestamp.hour, mOriginatorTimestamp.min,
                 mOriginatorTimestamp.sec);
    output->Write((const unsigned char*)buffer, 8);

    output->WriteUInt64(mTimeReference);

    output->WriteUInt16(mVersion);
    int reserved_count = 254;
    if (mVersion > 0) {
        output->Write(mUMID64, sizeof(mUMID64));
        reserved_count -= 64;
        if (mVersion > 1) {
            output->WriteInt16(mLoudnessValue);
            output->WriteInt16(mLoudnessRange);
            output->WriteInt16(mMaxTruePeakLevel);
            output->WriteInt16(mMaxMomentaryLoudness);
            output->WriteInt16(mMaxShortTermLoudness);
            reserved_count -= 10;
        }
    }
    output->WriteZeros(reserved_count);

    if (!mCodingHistory.empty())
        output->Write((const unsigned char*)mCodingHistory.c_str(), coding_history_size);

    mWasUpdated = false;
    mWrittenSize = size;
}

void WaveBEXT::Read(WaveIO *input, uint32_t size)
{
    int64_t start_pos = input->Tell();

    input->ReadString(mDescription, STRING_SIZE(mDescription));
    input->ReadString(mOriginator, STRING_SIZE(mOriginator));
    input->ReadString(mOriginatorReference, STRING_SIZE(mOriginatorReference));

    char buffer[20];
    unsigned int year, month, day, hour, min, sec;
    input->ReadString(buffer, 10);
    buffer[10] = ' ';
    input->ReadString(&buffer[11], 8);
    buffer[19] = '\0';
    if (sscanf(buffer, "%u%*c%u%*c%u%*c%u%*c%u%*c%u", &year, &month, &day, &hour, &min, &sec) != 6) {
        log_warn("Failed to parse bext timestamp %s\n", buffer);
    } else {
        mOriginatorTimestamp.year  = year;
        mOriginatorTimestamp.month = month;
        mOriginatorTimestamp.day   = day;
        mOriginatorTimestamp.hour  = hour;
        mOriginatorTimestamp.min   = min;
        mOriginatorTimestamp.sec   = sec;
    }

    mTimeReference = input->ReadUInt64();

    mVersion = input->ReadUInt16();
    int reserved_count = 254;
    if (mVersion > 0) {
        BMX_CHECK(input->Read(mUMID64, sizeof(mUMID64)) == sizeof(mUMID64));
        reserved_count -= 64;
        if (mVersion > 1) {
            mLoudnessValue        = input->ReadInt16();
            mLoudnessRange        = input->ReadInt16();
            mMaxTruePeakLevel     = input->ReadInt16();
            mMaxMomentaryLoudness = input->ReadInt16();
            mMaxShortTermLoudness = input->ReadInt16();
            reserved_count -= 10;
        }
    }
    input->Seek(reserved_count, SEEK_CUR);

    int64_t bext_remainder = size - (input->Tell() - start_pos);
    BMX_CHECK(bext_remainder >= 0);
    if (bext_remainder > 0) {
        char *coding_history_buffer = 0;
        uint32_t coding_history_size = (uint32_t)bext_remainder;
        try
        {
            coding_history_buffer = new char[coding_history_size + 1];
            input->ReadString(coding_history_buffer, coding_history_size);
            coding_history_buffer[coding_history_size] = '\0';
            mCodingHistory = coding_history_buffer;

            delete [] coding_history_buffer;
        }
        catch (...)
        {
            delete [] coding_history_buffer;
            throw;
        }
    }
}

string WaveBEXT::ConvertToASCII(string input)
{
    string output = input;

    size_t i;
    for (i = 0; i < output.size(); i++) {
        if ((unsigned char)output[i] > 0x7f)
            output[i] = ' ';
    }

    return output;
}

