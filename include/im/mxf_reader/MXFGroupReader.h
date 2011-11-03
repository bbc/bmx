/*
 * Copyright (C) 2011  British Broadcasting Corporation.
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifndef __IM_MXF_GROUP_READER_H__
#define __IM_MXF_GROUP_READER_H__

#include <vector>

#include <im/mxf_reader/MXFReader.h>



namespace im
{


class MXFGroupReader : public MXFReader
{
public:
    MXFGroupReader();
    virtual ~MXFGroupReader();

    void AddReader(MXFReader *reader);
    bool Finalize();

public:
    virtual void SetReadLimits();
    virtual void SetReadLimits(int64_t start_position, int64_t end_position, bool seek_start_position);
    virtual int64_t GetReadStartPosition() const { return mReadStartPosition; }
    virtual int64_t GetReadEndPosition() const   { return mReadEndPosition; }
    virtual int64_t GetReadDuration() const      { return mReadEndPosition - mReadStartPosition; }

    virtual uint32_t Read(uint32_t num_samples, int64_t frame_position = CURRENT_POSITION_VALUE);
    virtual void Seek(int64_t position);

    virtual int64_t GetPosition() const;

    virtual int16_t GetMaxPrecharge(int64_t position, bool limit_to_available) const;
    virtual int16_t GetMaxRollout(int64_t position, bool limit_to_available) const;

public:
    virtual bool HaveFixedLeadFillerOffset() const;
    virtual int64_t GetFixedLeadFillerOffset() const;

public:
    virtual size_t GetNumTrackReaders() const { return mTrackReaders.size(); }
    virtual MXFTrackReader* GetTrackReader(size_t track_index) const;

    virtual bool IsEnabled() const;

private:
    int64_t ConvertGroupDuration(int64_t group_duration, size_t member_index) const;
    int64_t ConvertGroupPosition(int64_t group_position, size_t member_index) const;
    int64_t ConvertMemberDuration(int64_t track_duration, size_t member_index) const;
    int64_t ConvertMemberPosition(int64_t track_position, size_t member_index) const;

private:
    std::vector<im::MXFReader*> mReaders;
    std::vector<im::MXFTrackReader*> mTrackReaders;

    int64_t mReadStartPosition;
    int64_t mReadEndPosition;

    std::vector<std::vector<uint32_t> > mSampleSequences;
    std::vector<int64_t> mSampleSequenceSizes;
};


};



#endif

