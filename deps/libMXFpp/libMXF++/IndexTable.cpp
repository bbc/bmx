/*
 * Copyright (C) 2008, British Broadcasting Corporation
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

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

using namespace std;
using namespace mxfpp;



bool IndexTableSegment::isIndexTableSegment(const mxfKey *key)
{
    return mxf_is_index_table_segment(key) != 0;
}

IndexTableSegment* IndexTableSegment::read(File *mxfFile, uint64_t segmentLen)
{
    ::MXFIndexTableSegment *cSegment;
    MXFPP_CHECK(mxf_read_index_table_segment(mxfFile->getCFile(), segmentLen, &cSegment));
    return new IndexTableSegment(cSegment);
}


IndexTableSegment::IndexTableSegment()
: _cSegment(0)
{
    MXFPP_CHECK(mxf_create_index_table_segment(&_cSegment));
}

IndexTableSegment::IndexTableSegment(::MXFIndexTableSegment *cSegment)
: _cSegment(cSegment)
{}

IndexTableSegment::~IndexTableSegment()
{
    mxf_free_index_table_segment(&_cSegment);
}


mxfUUID IndexTableSegment::getInstanceUID() const
{
    return _cSegment->instanceUID;
}

mxfRational IndexTableSegment::getIndexEditRate() const
{
    return _cSegment->indexEditRate;
}

int64_t IndexTableSegment::getIndexStartPosition() const
{
    return _cSegment->indexStartPosition;
}

int64_t IndexTableSegment::getIndexDuration() const
{
    return _cSegment->indexDuration;
}

uint32_t IndexTableSegment::getEditUnitByteCount() const
{
    return _cSegment->editUnitByteCount;
}

uint32_t IndexTableSegment::getIndexSID() const
{
    return _cSegment->indexSID;
}

uint32_t IndexTableSegment::getBodySID() const
{
    return _cSegment->bodySID;
}

uint8_t IndexTableSegment::getSliceCount() const
{
    return _cSegment->sliceCount;
}

uint8_t IndexTableSegment::getPosTableCount() const
{
    return _cSegment->posTableCount;
}

uint64_t IndexTableSegment::getExtStartOffset() const
{
    return _cSegment->extStartOffset;
}

uint64_t IndexTableSegment::getVBEByteCount() const
{
    return _cSegment->vbeByteCount;
}

mxfOptBool IndexTableSegment::getSingleIndexLocation() const
{
    return _cSegment->singleIndexLocation;
}

mxfOptBool IndexTableSegment::getSingleEssenceLocation() const
{
    return _cSegment->singleEssenceLocation;
}

mxfOptBool IndexTableSegment::getForwardIndexDirection() const
{
    return _cSegment->forwardIndexDirection;
}

bool IndexTableSegment::haveDeltaEntryAtDelta(uint32_t delta, uint8_t slice) const
{
    MXFDeltaEntry *entry = _cSegment->deltaEntryArray;
    while (entry &&
            (entry->slice < slice ||
                (entry->slice == slice && entry->elementData < delta)))
    {
        entry = entry->next;
    }

    return (entry && entry->slice == slice && entry->elementData == delta);
}

const MXFDeltaEntry* IndexTableSegment::getDeltaEntryAtDelta(uint32_t delta, uint8_t slice) const
{
    MXFDeltaEntry *entry = _cSegment->deltaEntryArray;
    while (entry &&
            (entry->slice < slice ||
                (entry->slice == slice && entry->elementData < delta)))
    {
        entry = entry->next;
    }

    MXFPP_ASSERT(entry && entry->slice == slice && entry->elementData == delta);
    return entry;
}


void IndexTableSegment::setInstanceUID(mxfUUID value)
{
    _cSegment->instanceUID = value;
}

void IndexTableSegment::setIndexEditRate(mxfRational value)
{
    _cSegment->indexEditRate = value;
}

void IndexTableSegment::setIndexStartPosition(int64_t value)
{
    _cSegment->indexStartPosition = value;
}

void IndexTableSegment::setIndexDuration(int64_t value)
{
    _cSegment->indexDuration = value;
}

void IndexTableSegment::incrementIndexDuration()
{
    _cSegment->indexDuration++;
}

void IndexTableSegment::setEditUnitByteCount(uint32_t value)
{
    _cSegment->editUnitByteCount = value;
}

void IndexTableSegment::setIndexSID(uint32_t value)
{
    _cSegment->indexSID = value;
}

void IndexTableSegment::setBodySID(uint32_t value)
{
    _cSegment->bodySID = value;
}

void IndexTableSegment::setSliceCount(uint8_t value)
{
    _cSegment->sliceCount = value;
}

void IndexTableSegment::forceWriteSliceCount(bool enable)
{
    _cSegment->forceWriteSliceCount = enable;
}

void IndexTableSegment::forceWriteCBEDuration0(bool enable)
{
    _cSegment->forceWriteCBEDuration0 = enable;
}

void IndexTableSegment::setPosTableCount(uint8_t value)
{
    _cSegment->posTableCount = value;
}

void IndexTableSegment::setExtStartOffset(uint64_t value)
{
    _cSegment->extStartOffset = value;
}

void IndexTableSegment::setVBEByteCount(uint64_t value)
{
    _cSegment->vbeByteCount = value;
}

void IndexTableSegment::setSingleIndexLocation(mxfOptBool value)
{
    _cSegment->singleIndexLocation = value;
}

void IndexTableSegment::setSingleEssenceLocation(mxfOptBool value)
{
    _cSegment->singleEssenceLocation = value;
}

void IndexTableSegment::setForwardIndexDirection(mxfOptBool value)
{
    _cSegment->forwardIndexDirection = value;
}

void IndexTableSegment::appendDeltaEntry(int8_t posTableIndex, uint8_t slice, uint32_t elementData)
{
    MXFPP_CHECK(mxf_default_add_delta_entry(NULL, 0, _cSegment, posTableIndex, slice, elementData));
}

void IndexTableSegment::appendIndexEntry(int8_t temporalOffset, int8_t keyFrameOffset, uint8_t flags,
                                         uint64_t streamOffset, const vector<uint32_t> &sliceOffset,
                                         const vector<mxfRational> &posTable)
{
    uint32_t *cSliceOffset = 0;
    mxfRational *cPosTable = 0;
    uint8_t i;

    try
    {
        if (sliceOffset.size() > 0)
        {
            cSliceOffset = new uint32_t[sliceOffset.size()];
            for (i = 0; i < (uint8_t)sliceOffset.size(); i++)
            {
                cSliceOffset[i] = sliceOffset.at(i);
            }
        }

        if (posTable.size() > 0)
        {
            cPosTable = new mxfRational[posTable.size()];
            for (i = 0; i < (uint8_t)posTable.size(); i++)
            {
                cPosTable[i] = posTable.at(i);
            }
        }

        MXFPP_CHECK(mxf_default_add_index_entry(NULL, 0, _cSegment, temporalOffset, keyFrameOffset, flags,
                                                streamOffset, cSliceOffset, cPosTable));

        delete [] cSliceOffset;
        delete [] cPosTable;
    }
    catch (...)
    {
        delete [] cSliceOffset;
        delete [] cPosTable;
        throw;
    }
}

void IndexTableSegment::write(File *mxfFile, Partition *partition, FillerWriter *filler)
{
    partition->markIndexStart(mxfFile);

    MXFPP_CHECK(mxf_write_index_table_segment(mxfFile->getCFile(), _cSegment));
    if (filler)
    {
        filler->write(mxfFile);
    }
    else
    {
        partition->fillToKag(mxfFile);
    }

    partition->markIndexEnd(mxfFile);
}

void IndexTableSegment::writeHeader(File *mxfFile, uint32_t numDeltaEntries, uint32_t numIndexEntries)
{
    MXFPP_CHECK(mxf_write_index_table_segment_header(mxfFile->getCFile(), _cSegment, numDeltaEntries, numIndexEntries));
}

void IndexTableSegment::writeDeltaEntryArrayHeader(File *mxfFile, uint32_t numDeltaEntries)
{
    MXFPP_CHECK(mxf_write_delta_entry_array_header(mxfFile->getCFile(), numDeltaEntries));
}

void IndexTableSegment::writeDeltaEntry(File *mxfFile, int8_t posTableIndex, uint8_t slice, uint32_t elementData)
{
    ::MXFDeltaEntry entry;
    entry.posTableIndex = posTableIndex;
    entry.slice = slice;
    entry.elementData = elementData;

    MXFPP_CHECK(mxf_write_delta_entry(mxfFile->getCFile(), &entry));
}

void IndexTableSegment::writeIndexEntryArrayHeader(File *mxfFile, uint8_t sliceCount, uint8_t posTableCount,
                                                   uint32_t numIndexEntries)
{
    MXFPP_CHECK(mxf_write_index_entry_array_header(mxfFile->getCFile(), sliceCount, posTableCount, numIndexEntries));
}

// TODO: this is not very efficient if sliceOffset or posTable size > 0
void IndexTableSegment::writeIndexEntry(File *mxfFile, int8_t temporalOffset, int8_t keyFrameOffset, uint8_t flags,
                                        uint64_t streamOffset, const std::vector<uint32_t> &sliceOffset,
                                        const std::vector<mxfRational> &posTable)
{
    uint8_t i;
    MXFIndexEntry entry;

    entry.temporalOffset = temporalOffset;
    entry.keyFrameOffset = keyFrameOffset;
    entry.flags = flags;
    entry.streamOffset = streamOffset;
    entry.sliceOffset = 0;
    entry.posTable = 0;

    try
    {
        if (sliceOffset.size() > 0)
        {
            entry.sliceOffset = new uint32_t[sliceOffset.size()];
            MXFPP_CHECK(entry.sliceOffset != 0);
            for (i = 0; i < (uint32_t)sliceOffset.size(); i++)
            {
                entry.sliceOffset[i] = sliceOffset.at(i);
            }
        }

        if (posTable.size() > 0)
        {
            entry.posTable = new mxfRational[posTable.size()];
            MXFPP_CHECK(entry.posTable != 0);
            for (i = 0; i < (uint32_t)posTable.size(); i++)
            {
                entry.posTable[i] = posTable.at(i);
            }
        }

        MXFPP_CHECK(mxf_write_index_entry(mxfFile->getCFile(), (uint8_t)sliceOffset.size(),
            (uint8_t)posTable.size(), &entry));

        delete [] entry.sliceOffset;
        delete [] entry.posTable;
    }
    catch (...)
    {
        delete [] entry.sliceOffset;
        delete [] entry.posTable;
        throw;
    }
}

void IndexTableSegment::writeAvidIndexEntryArrayHeader(File *mxfFile, uint8_t sliceCount, uint8_t posTableCount,
                                                       uint32_t numIndexEntries)
{
    MXFPP_CHECK(mxf_avid_write_index_entry_array_header(mxfFile->getCFile(), sliceCount, posTableCount,
                                                        numIndexEntries));
}

