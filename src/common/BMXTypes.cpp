/*
 * Copyright (C) 2010, British Broadcasting Corporation
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

#include <bmx/BMXTypes.h>
#include <bmx/Utils.h>

using namespace bmx;



Timecode::Timecode()
{
    SetInvalid();
}

Timecode::Timecode(uint16_t rounded_rate, bool drop_frame)
{
    Init(rounded_rate, drop_frame);
}

Timecode::Timecode(uint16_t rounded_rate, bool drop_frame, int64_t offset)
{
    Init(rounded_rate, drop_frame, offset);
}

Timecode::Timecode(Rational rate, bool drop_frame)
{
    Init(rate, drop_frame);
}

Timecode::Timecode(Rational rate, bool drop_frame, int16_t hour, int16_t min, int16_t sec, int16_t frame)
{
    Init(rate, drop_frame);
    Init(hour, min, sec, frame);
}

Timecode::Timecode(Rational rate, bool drop_frame, int64_t offset)
{
    Init(rate, drop_frame);
    Init(offset);
}

void Timecode::SetInvalid()
{
    mRoundedTCBase = 0;
    mDropFrame = false;
    mHour = 0;
    mMin = 0;
    mSec = 0;
    mFrame = 0;
    mOffset = 0;
    mFramesPerMin = 0;
    mNDFramesPerMin = 0;
    mFramesPer10Min = 0;
    mFramesPerHour = 0;
}

void Timecode::Init(uint16_t rounded_rate, bool drop_frame)
{
    mRoundedTCBase = rounded_rate;
    if (rounded_rate == 30 || rounded_rate == 60)
        mDropFrame = drop_frame;
    else
        mDropFrame = false;

    if (mDropFrame) {
        // first 2 frame numbers shall be omitted at the start of each minute,
        // except minutes 0, 10, 20, 30, 40 and 50
        mDropCount = 2;
        if (mRoundedTCBase == 60)
            mDropCount *= 2;
        mFramesPerMin = mRoundedTCBase * 60 - mDropCount;
        mFramesPer10Min = mFramesPerMin * 10 + mDropCount;
    } else {
        mFramesPerMin = mRoundedTCBase * 60;
        mFramesPer10Min = mFramesPerMin * 10;
    }
    mFramesPerHour = mFramesPer10Min * 6;
    mNDFramesPerMin = mRoundedTCBase * 60;

    mHour = 0;
    mMin = 0;
    mSec = 0;
    mFrame = 0;
    mOffset = 0;
}

void Timecode::Init(uint16_t rounded_rate, bool drop_frame, int64_t offset)
{
    Init(rounded_rate, drop_frame);
    mOffset = offset;
    CleanOffset();

    UpdateTimecode();
}

void Timecode::Init(Rational rate, bool drop_frame)
{
    Init(get_rounded_tc_base(rate), (rate == FRAME_RATE_2997 || rate == FRAME_RATE_5994) ? drop_frame : false);
}

void Timecode::Init(int64_t offset)
{
    mOffset = offset;
    CleanOffset();

    UpdateTimecode();
}

void Timecode::Init(Rational rate, bool drop_frame, int64_t offset)
{
    Init(rate, drop_frame);
    Init(offset);
}

void Timecode::Init(int16_t hour, int16_t min, int16_t sec, int16_t frame)
{
    mHour = hour;
    mMin = min;
    mSec = sec;
    mFrame = frame;
    CleanTimecode();

    UpdateOffset();
}

void Timecode::Init(Rational rate, bool drop_frame, int16_t hour, int16_t min, int16_t sec, int16_t frame)
{
    Init(rate, drop_frame);
    Init(hour, min, sec, frame);
}

void Timecode::AddOffset(int64_t offset)
{
    mOffset += offset;
    CleanOffset();

    UpdateTimecode();
}

void Timecode::AddOffset(int64_t offset, Rational rate)
{
    AddOffset(convert_position(offset, mRoundedTCBase, get_rounded_tc_base(rate), ROUND_AUTO));
}

int64_t Timecode::GetMaxOffset() const
{
    return 24 * mFramesPerHour;
}

bool Timecode::operator==(const Timecode &right) const
{
    return (mRoundedTCBase == 0 && right.mRoundedTCBase == 0) ||
           (mRoundedTCBase != 0 && right.mRoundedTCBase != 0 &&
               mOffset == convert_position(right.mOffset, mRoundedTCBase, right.mRoundedTCBase, ROUND_AUTO));
}

bool Timecode::operator!=(const Timecode &right) const
{
    return !(*this == right);
}

bool Timecode::operator<(const Timecode &right) const
{
    return (mRoundedTCBase != 0 && right.mRoundedTCBase == 0) ||
           (mRoundedTCBase != 0 && right.mRoundedTCBase != 0 &&
               mOffset < convert_position(right.mOffset, mRoundedTCBase, right.mRoundedTCBase, ROUND_AUTO));
}

void Timecode::CleanOffset()
{
    if (mRoundedTCBase == 0)
        mOffset = 0;
    else
        mOffset %= GetMaxOffset();

    if (mOffset < 0)
        mOffset += GetMaxOffset();
}

void Timecode::CleanTimecode()
{
    if (mRoundedTCBase == 0) {
        mHour  = 0;
        mMin   = 0;
        mSec   = 0;
        mFrame = 0;
    } else {
        mHour  %= 24;
        mMin   %= 60;
        mSec   %= 60;
        mFrame %= mRoundedTCBase;

        // replace invalid drop frame hhmmssff with a valid one after it
        if (mDropFrame && mSec == 0 && (mMin % 10) && mFrame < mDropCount)
            mFrame = mDropCount;
    }
}

void Timecode::UpdateOffset()
{
    if (mRoundedTCBase == 0) {
        mOffset = 0;
        return;
    }

    mOffset = mHour * mFramesPerHour;
    if (mDropFrame) {
        mOffset += (mMin / 10) * mFramesPer10Min;
        mOffset += (mMin % 10) * mFramesPerMin;
    } else {
        mOffset += mMin * mFramesPerMin;
    }
    mOffset += mSec * mRoundedTCBase;
    mOffset += mFrame;
}

void Timecode::UpdateTimecode()
{
    if (mRoundedTCBase == 0) {
        mHour  = 0;
        mMin   = 0;
        mSec   = 0;
        mFrame = 0;
        return;
    }

    int64_t offset = mOffset;
    bool frames_dropped = false;

    mHour = (int16_t)(offset / mFramesPerHour);
    offset = offset % mFramesPerHour;
    mMin = (int16_t)(offset / mFramesPer10Min * 10);
    offset = offset % mFramesPer10Min;
    if (offset >= mNDFramesPerMin) {
        offset -= mNDFramesPerMin;
        mMin += (int16_t)((offset / mFramesPerMin) + 1);
        offset = offset % mFramesPerMin;
        frames_dropped = mDropFrame;
    }
    mSec = (int16_t)(offset / mRoundedTCBase);
    mFrame = (int16_t)(offset % mRoundedTCBase);

    if (frames_dropped) {
        mFrame += mDropCount;
        if (mFrame >= mRoundedTCBase) {
            mFrame -= mRoundedTCBase;
            mSec++;
            if (mSec >= 60) {
                mSec = 0;
                mMin++;
                if (mMin >= 60) {
                    mMin = 0;
                    mHour++;
                    if (mHour >= 24)
                        mHour = 0;
                }
            }
        }
    }
}

