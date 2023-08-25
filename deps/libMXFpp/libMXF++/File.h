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

#ifndef MXFPP_FILE_H_
#define MXFPP_FILE_H_

#include <string>
#include <vector>

#include <libMXF++/Partition.h>

#include <mxf/mxf_memory_file.h>



namespace mxfpp
{


class File
{
public:
    static File* openNew(std::string filename);
    static File* openRead(std::string filename);
    static File* openModify(std::string filename);

public:
    File(::MXFFile* _cFile);
    ~File();

    void setMinLLen(uint8_t llen);
    uint8_t getMinLLen();

    Partition& createPartition();

    void writeRIP();
    void updatePartitions();
    void updateBodyPartitions(const mxfKey *pp_key);
    void updateGenericStreamPartitions();
    void updatePartitions(size_t rewriteFirstIndex, size_t rewriteLastIndex);

    Partition& getPartition(size_t index);
    const std::vector<Partition*>& getPartitions() const { return _partitions; }

    bool readHeaderPartition();
    Partition* readFooterPartition();  // Caller takes ownership
    bool readPartitions();
    void readNextPartition(const mxfKey *key, uint64_t len);

    uint8_t readUInt8();
    uint16_t readUInt16();
    uint32_t readUInt32();
    uint64_t readUInt64();
    int8_t readInt8();
    int16_t readInt16();
    int32_t readInt32();
    int64_t readInt64();

    void readK(mxfKey *key);
    void readL(uint8_t *llen, uint64_t *len);
    void readKL(mxfKey *key, uint8_t *llen, uint64_t *len);
    void readNextNonFillerKL(mxfKey *key, uint8_t *llen, uint64_t *len);
    void readLocalTL(mxfLocalTag *tag, uint16_t *len);
    void readBatchHeader(uint32_t *len, uint32_t *elementLen);

    uint32_t read(unsigned char *data, uint32_t count);
    int64_t tell();
    void seek(int64_t position, int whence);
    void skip(uint64_t len);
    int64_t size();
    bool eof();
    bool isSeekable();


    uint32_t write(const unsigned char *data, uint32_t count);

    void writeUInt8(uint8_t value);
    void writeUInt16(uint16_t value);
    void writeUInt32(uint32_t value);
    void writeUInt64(uint64_t value);
    void writeInt8(int8_t value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeInt64(int64_t value);
    void writeUL(const mxfUL *ul);

    void writeKL(const mxfKey *key, uint64_t len);
    void writeFixedL(uint8_t llen, uint64_t len);
    void writeFixedKL(const mxfKey *key, uint8_t llen, uint64_t len);

    void writeBatchHeader(uint32_t len, uint32_t eleLen);
    void writeArrayHeader(uint32_t len, uint32_t eleLen);

    void writeZeros(uint64_t len);

    void fillToPosition(uint64_t position);
    void writeFill(uint32_t size);


    void openMemoryFile(uint32_t chunkSize);
    void setMemoryPartitionIndexes(size_t first, size_t last = (size_t)(-1));
    bool isMemoryFileOpen() const { return _cMemoryFile != 0; }
    void closeMemoryFile();


    ::MXFFile* swapCFile(::MXFFile *newCFile);

    ::MXFFile* getCFile() const { return _cFile; }

private:
    std::vector<Partition*> _partitions;

    ::MXFFile *_cFile;
    ::MXFFile *_cOriginalFile;
    MXFMemoryFile *_cMemoryFile;
    size_t _firstMemoryPartitionIndex;
    size_t _lastMemoryPartitionIndex;
};


};



#endif

