/*
 * Copyright (C) 2010  British Broadcasting Corporation.
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

#ifndef __IM_MXF_FILE_READER_H__
#define __IM_MXF_FILE_READER_H__


#include <string>
#include <vector>
#include <map>

#include <libMXF++/MXF.h>

#include <im/mxf_reader/MXFReader.h>
#include <im/mxf_reader/MXFFileTrackReader.h>
#include <im/mxf_reader/EssenceReader.h>
#include <im/mxf_reader/MXFPackageResolver.h>
#include <im/URI.h>



namespace im
{



class MXFFileReader : public MXFReader
{
public:
    friend class EssenceReader;
    friend class IndexTableHelper;
    friend class EssenceChunkHelper;
    friend class MXFFileTrackReader;

public:
    typedef enum
    {
        MXF_RESULT_SUCCESS = 0,
        MXF_RESULT_OPEN_FAIL,
        MXF_RESULT_INVALID_FILE,
        MXF_RESULT_NOT_SUPPORTED,
        MXF_RESULT_NO_HEADER_METADATA,
        MXF_RESULT_INVALID_HEADER_METADATA,
        MXF_RESULT_NO_ESSENCE,
        MXF_RESULT_NO_ESSENCE_INDEX,
        MXF_RESULT_INCOMPLETE_ESSENCE_INDEX,
        MXF_RESULT_FAIL, // keep last
    } OpenResult;

public:
    static OpenResult Open(std::string filename, MXFPackageResolver *resolver, bool resolver_take_ownership,
                           MXFFileReader **file_reader);
    static std::string ResultToString(OpenResult result);

public:
    virtual ~MXFFileReader();

    MXFPackageResolver* GetPackageResolver() const { return mPackageResolver; }

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

    mxfpp::HeaderMetadata* GetHeaderMetadata() const { return mHeaderMetadata; }

    std::string GetFilename() const { return mFilename; }
    const URI& GetAbsoluteURI() const { return mAbsoluteURI; }

public:
    virtual size_t GetNumTrackReaders() const { return mTrackReaders.size(); }
    virtual MXFTrackReader* GetTrackReader(size_t index) const;

    virtual bool IsEnabled() const;

private:
    typedef enum
    {
        MATERIAL_PACKAGE_TYPE,
        FILE_SOURCE_PACKAGE_TYPE,
        PHYSICAL_SOURCE_PACKAGE_TYPE,
    } PackageType;

private:
    MXFFileReader(std::string filename, mxfpp::File *file, MXFPackageResolver *resolver, bool resolver_take_ownership);

    void ProcessMetadata(mxfpp::Partition *partition);

    MXFTrackReader* CreateInternalTrackReader(mxfpp::Partition *partition, mxfpp::MaterialPackage *material_package,
                                              mxfpp::Track *mp_track, mxfpp::SourceClip *mp_source_clip,
                                              bool is_picture, const ResolvedPackage *resolved_package);
    MXFTrackReader* GetExternalTrackReader(mxfpp::SourceClip *mp_source_clip,
                                           mxfpp::SourcePackage *file_source_package);

    void GetStartTimecodes(mxfpp::Preface *preface, mxfpp::MaterialPackage *material_package,
                           mxfpp::Track *infile_mp_track);
    bool GetStartTimecode(mxfpp::GenericPackage *package, mxfpp::Track *track, int64_t offset, Timecode *timecode);
    bool GetReferencedPackage(mxfpp::Preface *preface, mxfpp::Track *track, int64_t offset_in, PackageType package_type,
                              mxfpp::GenericPackage **ref_package_out, mxfpp::Track **ref_track_out,
                              int64_t *ref_offset_out);

    void ProcessDescriptor(mxfpp::FileDescriptor *file_descriptor, MXFTrackInfo *track_info);
    void ProcessPictureDescriptor(mxfpp::FileDescriptor *file_descriptor, MXFPictureTrackInfo *picture_track_info);
    void ProcessSoundDescriptor(mxfpp::FileDescriptor *file_descriptor, MXFSoundTrackInfo *sound_track_info);

    bool IsClipWrapped() { return mIsClipWrapped; }
    bool IsFrameWrapped() { return !mIsClipWrapped; }

    size_t GetNumInternalTrackReaders() const { return mInternalTrackReaders.size(); }
    MXFTrackReader* GetInternalTrackReader(size_t index) const;
    MXFTrackReader* GetInternalTrackReaderByNumber(uint32_t track_number) const;
    MXFTrackReader* GetInternalTrackReaderById(uint32_t id) const;

    void ForceDuration(int64_t duration);

    bool GetInternalIndexEntry(MXFIndexEntryExt *entry, int64_t position) const;
    int16_t GetInternalPrecharge(int64_t position, bool limit_to_available) const;
    int16_t GetInternalRollout(int64_t position, bool limit_to_available) const;

    bool InternalIsEnabled() const;

private:
    std::string mFilename;
    URI mAbsoluteURI;
    mxfpp::File *mFile;

    MXFPackageResolver *mPackageResolver;
    bool mOwnPackageResolver;

    mxfpp::DataModel *mDataModel;
    mxfpp::HeaderMetadata *mHeaderMetadata;

    bool mIsClipWrapped;
    uint32_t mBodySID;
    uint32_t mIndexSID;

    int64_t mOrigin;

    int64_t mReadStartPosition;
    int64_t mReadEndPosition;

    std::vector<MXFTrackReader*> mTrackReaders;
    std::vector<MXFTrackReader*> mInternalTrackReaders;
    std::map<uint32_t, MXFTrackReader*> mInternalTrackReaderNumberMap;
    std::vector<MXFFileReader*> mExternalReaders;
    std::vector<std::vector<uint32_t> > mExternalSampleSequences;
    std::vector<int64_t> mExternalSampleSequenceSizes;
    std::vector<MXFTrackReader*> mExternalTrackReaders;
    
    EssenceReader *mEssenceReader;
};


};



#endif

