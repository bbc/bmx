/*
 * Read a single OP-Atom file representing a clip track
 *
 * Copyright (C) 2009, British Broadcasting Corporation
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

#include <cstring>
#include <cstdio>

#include <libMXF++/MXF.h>
#include <libMXF++/extensions/TaggedValue.h>

#include <mxf/mxf_avid.h>
#include <mxf/mxf_macros.h>

#include "OPAtomTrackReader.h"

using namespace std;
using namespace mxfpp;


const char * const ERROR_STRINGS[] =
{
    "success",
    "general error",
    "could not open for reading",
    "no header partition found",
    "file is not OP-Atom",
    "error reading header metadata",
    "no file source package found",
    "essence data not found"
};



OPAtomOpenResult OPAtomTrackReader::Open(std::string filename, OPAtomTrackReader **track_reader)
{
    File *file = 0;

    try
    {
        // open the file
        try
        {
            file = File::openRead(filename);
        }
        catch (...)
        {
            throw OP_ATOM_FILE_OPEN_READ_ERROR;
        }

        // read the header partition pack and check the operational pattern
        if (!file->readHeaderPartition())
            throw OP_ATOM_NO_HEADER_PARTITION;
        Partition &header_partition = file->getPartition(0);

        if (!mxf_is_op_atom(header_partition.getOperationalPattern()))
            throw OP_ATOM_NOT_OP_ATOM;


        *track_reader = new OPAtomTrackReader(filename, file);

        return OP_ATOM_SUCCESS;
    }
    catch (const OPAtomOpenResult &ex)
    {
        delete file;
        return ex;
    }
    catch (...)
    {
        delete file;
        return OP_ATOM_FAIL;
    }
}

string OPAtomTrackReader::ErrorToString(OPAtomOpenResult result)
{
    size_t index = (size_t)(-1 * (int)result);
    MXFPP_ASSERT(index < ARRAY_SIZE(ERROR_STRINGS));

    return ERROR_STRINGS[index];
}




OPAtomTrackReader::OPAtomTrackReader(string filename, File *file)
{
    mFilename = filename;

    mTrackId = 0;
    mDurationInMetadata = -1;
    mIsPicture = true;

    DataModel *data_model = 0;
    AvidHeaderMetadata *header_metadata = 0;
    FrameOffsetIndexTableSegment *index_table = 0;

    try
    {
        int64_t essence_length = 0;
        mxfUL essence_label = g_Null_UL;
        mxfRational edit_rate = {0, 1};
        FileDescriptor *file_descriptor = 0;
        uint32_t frame_size = 0;
        mxfKey key;
        uint8_t llen;
        uint64_t len;
        Partition &header_partition = file->getPartition(0);

        // get essence container label
        vector<mxfUL> container_labels = header_partition.getEssenceContainers();
        MXFPP_CHECK(container_labels.size() == 1);
        essence_label = container_labels[0];

        // read the header metadata

        data_model = new DataModel();
        header_metadata = new AvidHeaderMetadata(data_model);

        TaggedValue::registerObjectFactory(header_metadata);

        file->readNextNonFillerKL(&key, &llen, &len);
        if (!mxf_is_header_metadata(&key))
            throw OP_ATOM_FAIL;

        header_metadata->read(file, &header_partition, &key, llen, len);


        Preface *preface = header_metadata->getPreface();
        ContentStorage *content = preface->getContentStorage();
        vector<GenericPackage*> packages = content->getPackages();

        // get the file source package and descriptor
        SourcePackage *fsp = 0;
        size_t i;
        for (i = 0; i < packages.size(); i++) {
            fsp = dynamic_cast<SourcePackage*>(packages[i]);
            if (!fsp || !fsp->haveDescriptor())
                continue;

            file_descriptor = dynamic_cast<FileDescriptor*>(fsp->getDescriptor());
            if (file_descriptor)
                break;
        }
        if (!file_descriptor)
            throw OP_ATOM_NO_FILE_PACKAGE;

        // get the material track info
        Track *mp_track = 0;
        for (i = 0; i < packages.size(); i++) {
            MaterialPackage *mp = dynamic_cast<MaterialPackage*>(packages[i]);
            if (!mp)
                continue;

            vector<GenericTrack*> tracks = mp->getTracks();
            size_t j;
            for (j = 0; j < tracks.size(); j++) {
                Track *track = dynamic_cast<Track*>(tracks[j]);
                if (!track)
                    continue;

                StructuralComponent *track_sequence = track->getSequence();

                Sequence *sequence = dynamic_cast<Sequence*>(track_sequence);
                SourceClip *source_clip = dynamic_cast<SourceClip*>(track_sequence);
                if (sequence) {
                    vector<StructuralComponent*> components = sequence->getStructuralComponents();
                    if (components.size() != 1)
                        continue;

                    source_clip = dynamic_cast<SourceClip*>(components[0]);
                }

                if (!source_clip)
                    continue;

                mxfUMID fsp_umid = fsp->getPackageUID();
                mxfUMID sc_umid = source_clip->getSourcePackageID();
                if (memcmp(&fsp_umid, &sc_umid, sizeof(fsp_umid)) == 0) {
                    mp_track = track;
                    edit_rate = mp_track->getEditRate();
                    mTrackId = mp_track->getTrackID();
                    mDurationInMetadata = source_clip->getDuration();
                    break;
                }
            }

            if (mp_track)
                break;
        }
        if (!mp_track)
            throw OP_ATOM_HEADER_ERROR;



        // read the index table if present and complete
        int64_t file_pos = file->tell();
        try
        {
            file->readNextNonFillerKL(&key, &llen, &len);
            while (!IndexTableSegment::isIndexTableSegment(&key)) {
                file->skip(len);
                file->readNextNonFillerKL(&key, &llen, &len);
            }

            if (IndexTableSegment::isIndexTableSegment(&key)) {
                index_table = FrameOffsetIndexTableSegment::read(file, len);
                mxfRational index_edit_rate = index_table->getIndexEditRate();
                if (memcmp(&index_edit_rate, &edit_rate, sizeof(index_edit_rate)) == 0)
                    frame_size = index_table->getEditUnitByteCount();
            }
        }
        catch (...)
        {
            mxf_log_warn("Ignore errors - failed to find or read the index table segment\n");
            // do nothing
        }

        file->seek(file_pos, SEEK_SET);

        // position the file at the start of the essence data
        try
        {
            file->readNextNonFillerKL(&key, &llen, &len);
            while (!mxf_is_gc_essence_element(&key) && !mxf_avid_is_essence_element(&key)) {
                file->skip(len);
                file->readNextNonFillerKL(&key, &llen, &len);
            }
        }
        catch (...)
        {
            throw OP_ATOM_ESSENCE_DATA_NOT_FOUND;
        }

        essence_length = len;


        mEssenceParser = RawEssenceParser::Create(file, essence_length, essence_label, file_descriptor, edit_rate,
                                                  frame_size, index_table);
        if (!mEssenceParser)
            throw MXFException("Failed to create essence parser");

        mDataModel = data_model;
        mHeaderMetadata = header_metadata;
        mIndexTable = index_table;
    }
    catch (...)
    {
        delete data_model;
        delete header_metadata;
        delete index_table;
        throw;
    }
}

OPAtomTrackReader::~OPAtomTrackReader()
{
    delete mEssenceParser;
    delete mHeaderMetadata;
    delete mDataModel;
    delete mIndexTable;
}

mxfUL OPAtomTrackReader::GetEssenceContainerLabel()
{
    return mEssenceParser->GetEssenceContainerLabel();
}

int64_t OPAtomTrackReader::GetDuration()
{
    int64_t essence_duration = mEssenceParser->GetDuration();

    if (mDurationInMetadata > 0 && essence_duration > mDurationInMetadata)
        return mDurationInMetadata;
    else
        return essence_duration;
}

int64_t OPAtomTrackReader::DetermineDuration()
{
    int64_t essence_duration = mEssenceParser->DetermineDuration();

    if (mDurationInMetadata > 0 && essence_duration > mDurationInMetadata)
        return mDurationInMetadata;
    else
        return essence_duration;
}

int64_t OPAtomTrackReader::GetPosition()
{
    return mEssenceParser->GetPosition();
}

bool OPAtomTrackReader::Read(OPAtomContentPackage *content)
{
    OPAtomContentElement *element = content->GetEssenceData(mTrackId, false);
    if (!element) {
        element = content->AddElement(mTrackId);
        element->mMaterialTrackId = mTrackId;
        element->mIsPicture = mIsPicture;
    }

    int64_t essence_offset = mEssenceParser->GetEssenceOffset();

    if (!mEssenceParser->Read(&element->mEssenceData, &element->mNumSamples))
        return false;


    element->mEssenceOffset = essence_offset;

    return true;
}

bool OPAtomTrackReader::Seek(int64_t position)
{
    return mEssenceParser->Seek(position);
}

bool OPAtomTrackReader::IsEOF()
{
    return mEssenceParser->IsEOF();
}


