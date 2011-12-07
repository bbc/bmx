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

#ifndef __BMX_MXF_FILE_READER_H__
#define __BMX_MXF_FILE_READER_H__


#include <string>
#include <vector>
#include <map>

#include <libMXF++/MXF.h>

#include <bmx/mxf_reader/MXFReader.h>
#include <bmx/mxf_reader/MXFFileTrackReader.h>
#include <bmx/mxf_reader/EssenceReader.h>
#include <bmx/mxf_reader/MXFPackageResolver.h>
#include <bmx/URI.h>



namespace bmx
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
    static OpenResult Open(mxfpp::File *file, std::string filename, MXFPackageResolver *resolver,
                           bool resolver_take_ownership, MXFFileReader **file_reader);
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

    virtual uint32_t Read(uint32_t num_samples, bool is_top = true);
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

public:
    virtual void SetNextFramePosition(int64_t position);
    virtual void SetNextFrameTrackPositions();

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

    void ExtractInfoFromFirstFrame();

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

