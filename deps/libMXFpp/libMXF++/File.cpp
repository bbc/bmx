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

#include <cstdio>

#include <algorithm>

#include <libMXF++/MXF.h>

using namespace std;
using namespace mxfpp;



// Utility class to hold a ::MXFPartitions list
class PartitionList
{
public:
    PartitionList()
    {
        mxf_initialise_list(&_cPartitions, NULL);
    }

    PartitionList(vector<Partition*> &partitions, size_t firstPartitionIndex = 0,
                  size_t lastPartitionIndex = (size_t)(-1))
    {
        mxf_initialise_list(&_cPartitions, NULL);

        size_t end = partitions.size();
        if (lastPartitionIndex != (size_t)(-1) && lastPartitionIndex + 1 < partitions.size())
        {
            end = lastPartitionIndex + 1;
        }

        size_t i;
        for (i = firstPartitionIndex; i < end; i++)
        {
            MXFPP_CHECK(mxf_append_partition(&_cPartitions, partitions[i]->getCPartition()));
        }
    }

    ~PartitionList()
    {
        mxf_clear_list(&_cPartitions);
    }

    ::MXFFilePartitions* getList()
    {
        return &_cPartitions;
    }

private:
    ::MXFFilePartitions _cPartitions;
};



File* File::openNew(string filename)
{
    ::MXFFile *cFile;
    MXFPP_CHECK(mxf_disk_file_open_new(filename.c_str(), &cFile));

    return new File(cFile);
}

File* File::openRead(string filename)
{
    ::MXFFile *cFile;
    MXFPP_CHECK(mxf_disk_file_open_read(filename.c_str(), &cFile));

    return new File(cFile);
}

File* File::openModify(string filename)
{
    ::MXFFile *cFile;
    MXFPP_CHECK(mxf_disk_file_open_modify(filename.c_str(), &cFile));

    return new File(cFile);
}

File::File(::MXFFile *cFile)
{
    _cFile = cFile;
    _cOriginalFile = 0;
    _cMemoryFile = 0;
    _firstMemoryPartitionIndex = (size_t)(-1);
    _lastMemoryPartitionIndex = (size_t)(-1);
}

File::~File()
{
    mxf_file_close(&_cFile);
    mxf_file_close(&_cOriginalFile);

    size_t i;
    for (i = 0; i < _partitions.size(); i++)
        delete _partitions[i];
}

void File::setMinLLen(uint8_t llen)
{
    mxf_file_set_min_llen(_cFile, llen);
}

uint8_t File::getMinLLen()
{
    return mxf_get_min_llen(_cFile);
}

Partition& File::createPartition()
{
    Partition *previousPartition = 0;
    Partition *partition = 0;

    if (_partitions.size() > 0)
    {
        previousPartition = _partitions.back();
    }

    _partitions.push_back(new Partition());
    partition = _partitions.back();
    if (previousPartition)
    {
        MXFPP_CHECK(mxf_initialise_with_partition(previousPartition->getCPartition(), partition->getCPartition()));
        partition->setPreviousPartition(previousPartition->getThisPartition());
    }

    return (*_partitions.back());
}

void File::writeRIP()
{
    PartitionList partitionList(_partitions);

    MXFPP_CHECK(mxf_write_rip(_cFile, partitionList.getList()));
}

void File::updatePartitions()
{
    size_t firstIndex = 0, lastIndex = (size_t)(-1);
    if (_cMemoryFile) {
        firstIndex = _firstMemoryPartitionIndex;
        lastIndex  = _lastMemoryPartitionIndex;
    }
    if (firstIndex >= _partitions.size())
        return;

    updatePartitions(firstIndex, lastIndex);
}

void File::updateBodyPartitions(const mxfKey *pp_key)
{
    size_t firstIndex, lastIndex;
    if (_cMemoryFile) {
        firstIndex = _firstMemoryPartitionIndex;
        if (_lastMemoryPartitionIndex == (size_t)(-1) || _lastMemoryPartitionIndex >= _partitions.size())
          lastIndex = _partitions.size() - 1;
        else
          lastIndex = _lastMemoryPartitionIndex;
    } else {
        firstIndex = 0;
        lastIndex = _partitions.size() - 1;
    }

    while (firstIndex <= lastIndex) {
      size_t firstBodyIndex = firstIndex;
      while (firstBodyIndex <= lastIndex &&
             !(_partitions[firstBodyIndex]->isBody() && !_partitions[firstBodyIndex]->isGenericStream()))
      {
          firstBodyIndex++;
      }
      size_t lastBodyIndex = firstBodyIndex;
      while (lastBodyIndex + 1 <= lastIndex &&
             (_partitions[lastBodyIndex + 1]->isBody() && !_partitions[lastBodyIndex + 1]->isGenericStream()))
      {
          lastBodyIndex++;
      }

      if (lastBodyIndex <= lastIndex) {
          if (pp_key) {
              size_t i;
              for (i = firstBodyIndex; i <= lastBodyIndex; i++)
                  _partitions[i]->setKey(pp_key);
          }
          updatePartitions(firstBodyIndex, lastBodyIndex);
      }

      firstIndex = lastBodyIndex + 1;
    }
}

void File::updateGenericStreamPartitions()
{
    size_t firstIndex, lastIndex;
    if (_cMemoryFile) {
        firstIndex = _firstMemoryPartitionIndex;
        if (_lastMemoryPartitionIndex == (size_t)(-1) || _lastMemoryPartitionIndex >= _partitions.size())
          lastIndex = _partitions.size() - 1;
        else
          lastIndex = _lastMemoryPartitionIndex;
    } else {
        firstIndex = 0;
        lastIndex = _partitions.size() - 1;
    }

    while (firstIndex <= lastIndex) {
      size_t firstGSIndex = firstIndex;
      while (firstGSIndex <= lastIndex &&
             !_partitions[firstGSIndex]->isGenericStream())
      {
          firstGSIndex++;
      }
      size_t lastGSIndex = firstGSIndex;
      while (lastGSIndex + 1 <= lastIndex &&
             _partitions[lastGSIndex + 1]->isGenericStream())
      {
          lastGSIndex++;
      }

      if (lastGSIndex <= lastIndex)
          updatePartitions(firstGSIndex, lastGSIndex);

      firstIndex = lastGSIndex + 1;
    }
}

void File::updatePartitions(size_t rewriteFirstIndex, size_t rewriteLastIndex)
{
    PartitionList completePartitionList(_partitions);
    mxf_update_partitions_in_memory(completePartitionList.getList());

    PartitionList partitionList(_partitions, rewriteFirstIndex, rewriteLastIndex);
    MXFPP_CHECK(mxf_rewrite_partitions(_cFile, partitionList.getList()));
}

Partition& File::getPartition(size_t index)
{
    MXFPP_ASSERT(index < _partitions.size());
    return *_partitions[index];
}

bool File::readHeaderPartition()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    size_t i;
    for (i = 0; i < _partitions.size(); i++)
        delete _partitions[i];
    _partitions.clear();

    try
    {
        seek(mxf_get_runin_len(_cFile), SEEK_SET);
        if (!mxf_read_header_pp_kl(_cFile, &key, &llen, &len))
            return false;

        _partitions.push_back(Partition::read(this, &key, len));
        // file is positioned after the partition pack

        return true;
    }
    catch (...)
    {
        for (i = 0; i < _partitions.size(); i++)
            delete _partitions[i];
        _partitions.clear();
        return false;
    }
}

Partition* File::readFooterPartition()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    MXFRIP rip;
    MXFRIPEntry *last_rip_entry;
    int64_t footer_this_partition = 0;

    // Try find the footer partition in the RIP
    if (mxf_read_rip(_cFile, &rip)) {
        last_rip_entry = (MXFRIPEntry*)mxf_get_last_list_element(&rip.entries);
        if (last_rip_entry) {
            footer_this_partition = last_rip_entry->thisPartition;
        }
        mxf_clear_rip(&rip);
    }

    // Else use the header partition's footerPartition property
    if (footer_this_partition <= 0) {
        if (_partitions.empty() || !readHeaderPartition()) {
            return 0;
        }
        footer_this_partition = _partitions[0]->getFooterPartition();
    }

    if (footer_this_partition > 0) {
        try
        {
            seek(mxf_get_runin_len(_cFile) + footer_this_partition, SEEK_SET);
            readKL(&key, &llen, &len);
            return Partition::read(this, &key, len);
        }
        catch (...)
        {
            mxf_log_error("Failed to read the footer partition\n");
        }
    }

    return 0;
}

bool File::readPartitions()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint64_t this_partition;
    MXFRIP rip;
    MXFRIPEntry *rip_entry;
    MXFListIterator iter;
    Partition *header_partition = 0;

    // read the header partition if not already done so and clear the partition list

    if ((!_partitions.empty() && _partitions[0]->isHeader()) ||
        readHeaderPartition())
    {
        header_partition = _partitions[0];
    }

    size_t i;
    for (i = 0; i < _partitions.size(); i++) {
        if (_partitions[i] != header_partition)
            delete _partitions[i];
    }
    _partitions.clear();

    if (!header_partition)
        return false;


    try
    {
        // use the RIP if there is one
        if (mxf_read_rip(_cFile, &rip)) {
            try
            {
                _partitions.push_back(header_partition);

                mxf_initialise_list_iter(&iter, &rip.entries);
                while (mxf_next_list_iter_element(&iter)) {
                    rip_entry = (MXFRIPEntry*)mxf_get_iter_element(&iter);
                    if (rip_entry->thisPartition <= header_partition->getThisPartition())
                        continue;

                    seek(mxf_get_runin_len(_cFile) + rip_entry->thisPartition, SEEK_SET);
                    readKL(&key, &llen, &len);
                    _partitions.push_back(Partition::read(this, &key, len));
                }

                mxf_clear_rip(&rip);
            }
            catch (...)
            {
                mxf_log_error("Failed to read partitions listed in RIP\n");
                mxf_clear_rip(&rip);
                throw;
            }
        } else {

            // start from footer partition and work back to the header partition

            this_partition = header_partition->getFooterPartition();

            if (this_partition <= header_partition->getThisPartition()) {
                if (header_partition->isClosed())
                    mxf_log_warn("Header partition is marked Closed but footer partition offset is not set\n");

                if (!mxf_find_footer_partition(_cFile))
                    throw false;

                mxf_log_warn("File is missing both a RIP and a footer partition offset in the header partition pack\n");
                this_partition = mxf_file_tell(_cFile) - mxf_get_runin_len(_cFile);
            }

            do {
                seek(mxf_get_runin_len(_cFile) + this_partition, SEEK_SET);
                readKL(&key, &llen, &len);
                _partitions.push_back(Partition::read(this, &key, len));

                this_partition = _partitions.back()->getPreviousPartition();
            }
            while (this_partition < _partitions.back()->getThisPartition() &&
                   this_partition > header_partition->getThisPartition());

            _partitions.push_back(header_partition);

            // reorder to start with the header partition
            reverse(_partitions.begin(), _partitions.end());
        }

        return true;
    }
    catch (...)
    {
        for (i = 0; i < _partitions.size(); i++) {
            if (_partitions[i] != header_partition)
                delete _partitions[i];
        }
        _partitions.clear();

        // restore the header partition to the list
        _partitions.push_back(header_partition);

        return false;
    }
}

void File::readNextPartition(const mxfKey *key, uint64_t len)
{
    _partitions.push_back(Partition::read(this, key, len));
}

uint8_t File::readUInt8()
{
    uint8_t value;
    MXFPP_CHECK(mxf_read_uint8(_cFile, &value));
    return value;
}

uint16_t File::readUInt16()
{
    uint16_t value;
    MXFPP_CHECK(mxf_read_uint16(_cFile, &value));
    return value;
}

uint32_t File::readUInt32()
{
    uint32_t value;
    MXFPP_CHECK(mxf_read_uint32(_cFile, &value));
    return value;
}

uint64_t File::readUInt64()
{
    uint64_t value;
    MXFPP_CHECK(mxf_read_uint64(_cFile, &value));
    return value;
}

int8_t File::readInt8()
{
    int8_t value;
    MXFPP_CHECK(mxf_read_int8(_cFile, &value));
    return value;
}

int16_t File::readInt16()
{
    int16_t value;
    MXFPP_CHECK(mxf_read_int16(_cFile, &value));
    return value;
}

int32_t File::readInt32()
{
    int32_t value;
    MXFPP_CHECK(mxf_read_int32(_cFile, &value));
    return value;
}

int64_t File::readInt64()
{
    int64_t value;
    MXFPP_CHECK(mxf_read_int64(_cFile, &value));
    return value;
}

void File::readK(mxfKey *key)
{
    MXFPP_CHECK(mxf_read_k(_cFile, key));
}

void File::readL(uint8_t *llen, uint64_t *len)
{
    MXFPP_CHECK(mxf_read_l(_cFile, llen, len));
}

void File::readKL(mxfKey *key, uint8_t *llen, uint64_t *len)
{
    MXFPP_CHECK(mxf_read_kl(_cFile, key, llen, len));
}

void File::readNextNonFillerKL(mxfKey *key, uint8_t *llen, uint64_t *len)
{
    MXFPP_CHECK(mxf_read_next_nonfiller_kl(_cFile, key, llen, len));
}

void File::readLocalTL(mxfLocalTag *tag, uint16_t *len)
{
    MXFPP_CHECK(mxf_read_local_tl(_cFile, tag, len));
}

void File::readBatchHeader(uint32_t *len, uint32_t *elementLen)
{
    MXFPP_CHECK(mxf_read_batch_header(_cFile, len, elementLen));
}

uint32_t File::read(unsigned char *data, uint32_t count)
{
    return mxf_file_read(_cFile, data, count);
}

int64_t File::tell()
{
    return mxf_file_tell(_cFile);
}

void File::seek(int64_t position, int whence)
{
    MXFPP_CHECK(mxf_file_seek(_cFile, position, whence));
}

void File::skip(uint64_t len)
{
    MXFPP_CHECK(mxf_skip(_cFile, len));
}

int64_t File::size()
{
    return mxf_file_size(_cFile);
}

bool File::eof()
{
    return mxf_file_eof(_cFile) == 1;
}

bool File::isSeekable()
{
    return mxf_file_is_seekable(_cFile) == 1;
}

uint32_t File::write(const unsigned char *data, uint32_t count)
{
    return mxf_file_write(_cFile, data, count);
}

void File::writeUInt8(uint8_t value)
{
    MXFPP_CHECK(mxf_write_uint8(_cFile, value));
}

void File::writeUInt16(uint16_t value)
{
    MXFPP_CHECK(mxf_write_uint16(_cFile, value));
}

void File::writeUInt32(uint32_t value)
{
    MXFPP_CHECK(mxf_write_uint32(_cFile, value));
}

void File::writeUInt64(uint64_t value)
{
    MXFPP_CHECK(mxf_write_uint64(_cFile, value));
}

void File::writeInt8(int8_t value)
{
    MXFPP_CHECK(mxf_write_int8(_cFile, value));
}

void File::writeInt16(int16_t value)
{
    MXFPP_CHECK(mxf_write_int16(_cFile, value));
}

void File::writeInt32(int32_t value)
{
    MXFPP_CHECK(mxf_write_int32(_cFile, value));
}

void File::writeInt64(int64_t value)
{
    MXFPP_CHECK(mxf_write_int64(_cFile, value));
}

void File::writeUL(const mxfUL *ul)
{
    MXFPP_CHECK(mxf_write_ul(_cFile, ul));
}

void File::writeKL(const mxfKey *key, uint64_t len)
{
    MXFPP_CHECK(mxf_write_kl(_cFile, key, len));
}

void File::writeFixedL(uint8_t llen, uint64_t len)
{
    MXFPP_CHECK(mxf_write_fixed_l(_cFile, llen, len));
}

void File::writeFixedKL(const mxfKey *key, uint8_t llen, uint64_t len)
{
    MXFPP_CHECK(mxf_write_fixed_kl(_cFile, key, llen, len));
}

void File::writeBatchHeader(uint32_t len, uint32_t eleLen)
{
    MXFPP_CHECK(mxf_write_batch_header(_cFile, len, eleLen));
}

void File::writeArrayHeader(uint32_t len, uint32_t eleLen)
{
    MXFPP_CHECK(mxf_write_array_header(_cFile, len, eleLen));
}

void File::writeZeros(uint64_t len)
{
    MXFPP_CHECK(mxf_write_zeros(_cFile, len));
}

void File::fillToPosition(uint64_t position)
{
    MXFPP_CHECK(mxf_fill_to_position(_cFile, position));
}

void File::writeFill(uint32_t size)
{
    MXFPP_CHECK(mxf_write_fill(_cFile, size));
}

void File::openMemoryFile(uint32_t chunkSize)
{
    MXFPP_CHECK(!_cMemoryFile);

    MXFMemoryFile *memFile;
    MXFPP_CHECK(mxf_mem_file_open_new(chunkSize, mxf_file_tell(_cFile), &memFile));

    // set min llen on memory file if already set on the original file
    MXFFile *mxfMemFile = mxf_mem_file_get_file(memFile);
    mxf_file_set_min_llen(mxfMemFile, mxf_get_min_llen(_cFile));

    _cOriginalFile = _cFile;
    _cFile = mxfMemFile;
    _cMemoryFile = memFile;
    _firstMemoryPartitionIndex = _partitions.size();
    _lastMemoryPartitionIndex = (size_t)(-1);
}

void File::setMemoryPartitionIndexes(size_t first, size_t last)
{
    _firstMemoryPartitionIndex = first;
    _lastMemoryPartitionIndex = last;
}

void File::closeMemoryFile()
{
    if (!_cMemoryFile)
        return;

    MXFPP_CHECK(mxf_mem_file_flush_to_file(_cMemoryFile, _cOriginalFile));

    // set min llen on original file if it was set on the memory file
    mxf_file_set_min_llen(_cOriginalFile, mxf_get_min_llen(_cFile));

    mxf_file_close(&_cFile);
    _cFile = _cOriginalFile;
    _cOriginalFile = 0;
    _cMemoryFile = 0;
    _firstMemoryPartitionIndex = (size_t)(-1);
    _lastMemoryPartitionIndex = (size_t)(-1);
}

::MXFFile* File::swapCFile(::MXFFile *newCFile)
{
    ::MXFFile *ret = _cFile;
    _cFile = newCFile;
    return ret;
}

