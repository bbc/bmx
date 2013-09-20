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

#ifndef BMX_TYPES_H_
#define BMX_TYPES_H_


#include <libMXF++/MXFTypes.h>



namespace bmx
{


typedef enum
{
    UNKNOWN_FRAME_TYPE,
    I_FRAME,
    P_FRAME,
    B_FRAME
} MPEGFrameType;

typedef enum
{
    COLOR_WHITE,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_BLACK
} Color;


typedef mxfRational         Rational;
typedef mxfTimestamp        Timestamp;
typedef mxfUL               UL;
typedef mxfUUID             UUID;
typedef mxfUMID             UMID;
typedef mxfExtendedUMID     ExtendedUMID;

typedef struct
{
    unsigned char bytes[8];
} SMPTE12MTimecode;


static const Rational FRAME_RATE_23976  = {24000, 1001};
static const Rational FRAME_RATE_24     = {24, 1};
static const Rational FRAME_RATE_25     = {25, 1};
static const Rational FRAME_RATE_2997   = {30000, 1001};
static const Rational FRAME_RATE_30     = {30, 1};
static const Rational FRAME_RATE_50     = {50, 1};
static const Rational FRAME_RATE_5994   = {60000, 1001};
static const Rational FRAME_RATE_60     = {60, 1};

static const Rational SAMPLING_RATE_48K = {48000, 1};

static const Rational ASPECT_RATIO_4_3  = {4, 3};
static const Rational ASPECT_RATIO_16_9 = {16, 9};

static const Rational ZERO_RATIONAL     = {0, 1};


class Timecode
{
public:
    Timecode();
    Timecode(uint16_t rounded_rate, bool drop_frame);
    Timecode(uint16_t rounded_rate, bool drop_frame, int64_t offset);
    Timecode(Rational rate, bool drop_frame);
    Timecode(Rational rate, bool drop_frame, int64_t offset);
    Timecode(Rational rate, bool drop_frame, int16_t hour, int16_t min, int16_t sec, int16_t frame);

    bool IsInvalid()            const { return mRoundedTCBase == 0; }
    uint16_t GetRoundedTCBase() const { return mRoundedTCBase; }
    bool IsDropFrame()          const { return mDropFrame; }
    int16_t GetHour()           const { return mHour; }
    int16_t GetMin()            const { return mMin; }
    int16_t GetSec()            const { return mSec; }
    int16_t GetFrame()          const { return mFrame; }
    int64_t GetOffset()         const { return mOffset; }

    void SetInvalid();
    void Init(uint16_t rounded_rate, bool drop_frame);
    void Init(uint16_t rounded_rate, bool drop_frame, int64_t offset);
    void Init(Rational rate, bool drop_frame);
    void Init(int64_t offset);
    void Init(Rational rate, bool drop_frame, int64_t offset);
    void Init(int16_t hour, int16_t min, int16_t sec, int16_t frame);
    void Init(Rational rate, bool drop_frame, int16_t hour, int16_t min, int16_t sec, int16_t frame);

    void AddOffset(int64_t offset);
    void AddOffset(int64_t offset, Rational rate);

    int64_t GetMaxOffset() const;

    bool operator ==(const Timecode &right) const;
    bool operator !=(const Timecode &right) const;
    bool operator  <(const Timecode &right) const;

private:
    void CleanOffset();
    void CleanTimecode();

    void UpdateOffset();
    void UpdateTimecode();

private:
    uint16_t mRoundedTCBase;
    bool mDropFrame;
    int16_t mHour;
    int16_t mMin;
    int16_t mSec;
    int16_t mFrame;
    int64_t mOffset;

    uint32_t mFramesPerMin;
    uint32_t mNDFramesPerMin;
    uint32_t mFramesPer10Min;
    uint32_t mFramesPerHour;
    int16_t mDropCount;
};



};



#endif

