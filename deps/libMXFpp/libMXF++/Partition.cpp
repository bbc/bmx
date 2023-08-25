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

using namespace std;
using namespace mxfpp;



KAGFillerWriter::KAGFillerWriter(Partition *partition, uint32_t allocSpace)
: _partition(partition), _allocSpace(allocSpace)
{}

KAGFillerWriter::~KAGFillerWriter()
{}

void KAGFillerWriter::write(File *file)
{
    _partition->allocateSpaceToKag(file, _allocSpace);
}


PositionFillerWriter::PositionFillerWriter(int64_t position)
: _position(position)
{}

PositionFillerWriter::~PositionFillerWriter()
{}

void PositionFillerWriter::write(File *file)
{
    file->fillToPosition(_position);
}



Partition* Partition::read(File *file, const mxfKey *key, uint64_t len)
{
    ::MXFPartition *cPartition;
    MXFPP_CHECK(mxf_read_partition(file->getCFile(), key, len, &cPartition));

    return new Partition(cPartition);
}


Partition::Partition()
{
    MXFPP_CHECK(mxf_create_partition(&_cPartition));
}

Partition::Partition(::MXFPartition *cPartition)
: _cPartition(cPartition)
{}

Partition::Partition(const Partition &partition)
{
    MXFPP_CHECK(mxf_create_from_partition(partition._cPartition, &_cPartition));
}

Partition::~Partition()
{
    mxf_free_partition(&_cPartition);
}

void Partition::setKey(const mxfKey *key)
{
    _cPartition->key = *key;
}

void Partition::setVersion(uint16_t majorVersion, uint16_t minorVersion)
{
    _cPartition->majorVersion = majorVersion;
    _cPartition->minorVersion = minorVersion;
}

void Partition::setKagSize(uint32_t kagSize)
{
    _cPartition->kagSize = kagSize;
}

void Partition::setThisPartition(uint64_t thisPartition)
{
    _cPartition->thisPartition = thisPartition;
}

void Partition::setPreviousPartition(uint64_t previousPartition)
{
    _cPartition->previousPartition = previousPartition;
}

void Partition::setFooterPartition(uint64_t footerPartition)
{
    _cPartition->footerPartition = footerPartition;
}

void Partition::setHeaderByteCount(uint64_t headerByteCount)
{
    _cPartition->headerByteCount = headerByteCount;
}

void Partition::setIndexByteCount(uint64_t indexByteCount)
{
    _cPartition->indexByteCount = indexByteCount;
}

void Partition::setIndexSID(uint32_t indexSID)
{
    _cPartition->indexSID = indexSID;
}

void Partition::setBodyOffset(uint64_t bodyOffset)
{
    _cPartition->bodyOffset = bodyOffset;
}

void Partition::setBodySID(uint32_t bodySID)
{
    _cPartition->bodySID = bodySID;
}

void Partition::setOperationalPattern(const mxfUL *operationalPattern)
{
    _cPartition->operationalPattern = *operationalPattern;
}

void Partition::setOperationalPattern(mxfUL operationalPattern)
{
    _cPartition->operationalPattern = operationalPattern;
}

void Partition::addEssenceContainer(const mxfUL *essenceContainer)
{
    MXFPP_CHECK(mxf_append_partition_esscont_label(_cPartition, essenceContainer));
}

void Partition::addEssenceContainer(mxfUL essenceContainer)
{
    MXFPP_CHECK(mxf_append_partition_esscont_label(_cPartition, &essenceContainer));
}

const mxfKey* Partition::getKey() const
{
    return &_cPartition->key;
}

uint16_t Partition::getMajorVersion() const
{
    return _cPartition->majorVersion;
}

uint16_t Partition::getMinorVersion() const
{
    return _cPartition->minorVersion;
}

uint32_t Partition::getKagSize() const
{
    return _cPartition->kagSize;
}

uint64_t Partition::getThisPartition() const
{
    return _cPartition->thisPartition;
}

uint64_t Partition::getPreviousPartition() const
{
    return _cPartition->previousPartition;
}

uint64_t Partition::getFooterPartition() const
{
    return _cPartition->footerPartition;
}

uint64_t Partition::getHeaderByteCount() const
{
    return _cPartition->headerByteCount;
}

uint64_t Partition::getIndexByteCount() const
{
    return _cPartition->indexByteCount;
}

uint32_t Partition::getIndexSID() const
{
    return _cPartition->indexSID;
}

uint64_t Partition::getBodyOffset() const
{
    return _cPartition->bodyOffset;
}

uint32_t Partition::getBodySID() const
{
    return _cPartition->bodySID;
}

const mxfUL* Partition::getOperationalPattern() const
{
    return &_cPartition->operationalPattern;
}

vector<mxfUL> Partition::getEssenceContainers() const
{
    vector<mxfUL> result;
    ::MXFListIterator iter;

    mxf_initialise_list_iter(&iter, &_cPartition->essenceContainers);

    while (mxf_next_list_iter_element(&iter))
    {
        mxfUL label;
        mxf_get_ul((const uint8_t*)mxf_get_iter_element(&iter), &label);
        result.push_back(label);
    }

    return result;
}

bool Partition::isClosed() const
{
    return mxf_partition_is_closed(&_cPartition->key);
}

bool Partition::isComplete() const
{
    return mxf_partition_is_complete(&_cPartition->key);
}

bool Partition::isClosedAndComplete() const
{
    return mxf_partition_is_closed_and_complete(&_cPartition->key);
}

bool Partition::isHeader() const
{
    return mxf_is_header_partition_pack(&_cPartition->key);
}

bool Partition::isBody() const
{
    return mxf_is_body_partition_pack(&_cPartition->key);
}

bool Partition::isGenericStream() const
{
    return mxf_is_generic_stream_partition_pack(&_cPartition->key);
}

bool Partition::isFooter() const
{
    return mxf_is_footer_partition_pack(&_cPartition->key);
}

void Partition::markHeaderStart(File *file)
{
    MXFPP_CHECK(mxf_mark_header_start(file->getCFile(), _cPartition));
}

void Partition::markHeaderEnd(File *file)
{
    MXFPP_CHECK(mxf_mark_header_end(file->getCFile(), _cPartition));
}

void Partition::markIndexStart(File *file)
{
    MXFPP_CHECK(mxf_mark_index_start(file->getCFile(), _cPartition));
}

void Partition::markIndexEnd(File *file)
{
    MXFPP_CHECK(mxf_mark_index_end(file->getCFile(), _cPartition));
}

void Partition::write(File *file)
{
    MXFPP_CHECK(mxf_write_partition(file->getCFile(), _cPartition));
    fillToKag(file);
}

void Partition::fillToKag(File *file)
{
    if (_cPartition->kagSize > 0)
    {
        MXFPP_CHECK(mxf_fill_to_kag(file->getCFile(), _cPartition));
    }
}

void Partition::allocateSpaceToKag(File *file, uint32_t size)
{
    if (_cPartition->kagSize > 0)
    {
        MXFPP_CHECK(mxf_allocate_space_to_kag(file->getCFile(), _cPartition, size));
    }
    else
    {
        MXFPP_CHECK(mxf_allocate_space(file->getCFile(), size));
    }
}

