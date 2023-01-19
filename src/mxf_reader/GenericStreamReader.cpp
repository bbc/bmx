/*
 * Copyright (C) 2022, British Broadcasting Corporation
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

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <errno.h>
#include <limits.h>

#include <libMXF++/MXF.h>

#include <bmx/mxf_reader/GenericStreamReader.h>
#include <bmx/ByteArray.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


GenericStreamReader::GenericStreamReader(mxfpp::File *mxf_file, uint32_t stream_id, const vector<mxfKey> &expected_stream_keys)
: BMXIO()
{
    mMXFFile = mxf_file;
    mStreamId = stream_id;
    mExpectedStreamKeys = expected_stream_keys;
    mHaveFileOffsetRanges = false;
    mSize = 0;
    mPosition = 0;
    mRangeIndex = 0;
    mRangeOffset = 0;
}

GenericStreamReader::~GenericStreamReader()
{
}

uint32_t GenericStreamReader::Read(unsigned char *data, uint32_t size)
{
    if (!mHaveFileOffsetRanges)
        GetFileOffsetRanges();

    if (mPosition >= mSize)
        return 0;

    int64_t read_size = mSize - mPosition;
    if (read_size > (int64_t)size)
        read_size = size;

    int64_t read_remainder = read_size;
    while (read_remainder > 0) {
        const pair<int64_t, int64_t> &range = mFileOffsetRanges[mRangeIndex];

        int64_t range_remainder = range.second - mRangeOffset;
        if (range_remainder > 0) {
            uint32_t to_read = (uint32_t)read_remainder;
            if ((int64_t)to_read > range_remainder)
                to_read = (uint32_t)range_remainder;

            if (mRangeOffset == 0)
                mMXFFile->seek(range.first, SEEK_SET);

            uint32_t have_read = mMXFFile->read(&data[read_size - read_remainder], to_read);

            mPosition += have_read;
            mRangeOffset += have_read;
            read_remainder -= have_read;

            if (have_read < to_read)
                break;
        }

        if (mRangeOffset >= range.second) {
            mRangeOffset = 0;
            mRangeIndex += 1;
        }
    }

    return read_size - read_remainder;
}

int64_t GenericStreamReader::Tell()
{
    return mPosition;
}

int64_t GenericStreamReader::Size()
{
    if (!mHaveFileOffsetRanges)
        GetFileOffsetRanges();

    return mSize;
}

void GenericStreamReader::Read(FILE *file)
{
    if (mHaveFileOffsetRanges) {
        mPosition = 0;
        mRangeIndex = 0;
        mRangeOffset = 0;

        BMXIO::Read(file);
    } else {
        Read(file, 0, 0, 0);
    }
}

void GenericStreamReader::Read(BMXIO *bmx_io)
{
    if (mHaveFileOffsetRanges) {
        mPosition = 0;
        mRangeIndex = 0;
        mRangeOffset = 0;

        BMXIO::Read(bmx_io);
    } else {
        Read(0, bmx_io, 0, 0);
    }
}

void GenericStreamReader::Read(unsigned char **data, size_t *size)
{
    Read(0, 0, data, size);
}

const std::vector<std::pair<int64_t, int64_t> >& GenericStreamReader::GetFileOffsetRanges()
{
    if (!mHaveFileOffsetRanges) {
        Read(0, 0, 0, 0);

        mSize = 0;
        size_t i;
        for (i = 0; i < mFileOffsetRanges.size(); i++)
            mSize += mFileOffsetRanges[i].second;

        mHaveFileOffsetRanges = true;
    }

    return mFileOffsetRanges;
}

bool GenericStreamReader::MatchesExpectedStreamKey(const mxfKey *key)
{
    if (mExpectedStreamKeys.empty())
        return mxf_is_gs_data_element(key);

    size_t i;
    for (i = 0; i < mExpectedStreamKeys.size(); i++) {
        if (mxf_equals_key_mod_regver(key, &mExpectedStreamKeys[i]))
            return true;
    }

    return false;
}

void GenericStreamReader::Read(FILE *file, BMXIO *bmx_io, unsigned char **data, size_t *size)
{
    mStreamKey = g_Null_Key;
    mFileOffsetRanges.clear();

    int64_t original_file_pos = mMXFFile->tell();
    try
    {
        uint64_t body_size = 0;
        bmx::ByteArray buffer;
        mxfKey key;
        uint8_t llen;
        uint64_t len;
        const vector<Partition*> &partitions = mMXFFile->getPartitions();
        size_t i;
        for (i = 0; i < partitions.size(); i++) {
            if (partitions[i]->getBodySID() != mStreamId)
                continue;

            if (partitions[i]->getBodyOffset() > body_size) {
                log_warn("Ignoring potential missing stream data; "
                         "partition pack's BodyOffset 0x%" PRIx64 " > expected offset 0x" PRIx64 "\n",
                         partitions[i]->getBodyOffset(), body_size);
                continue;
            } else if (partitions[i]->getBodyOffset() < body_size) {
                log_warn("Ignoring overlapping or repeated stream data; "
                         "partition pack's BodyOffset 0x%" PRIx64 " < expected offset 0x" PRIx64 "\n",
                         partitions[i]->getBodyOffset(), body_size);
                continue;
            }

            mMXFFile->seek(partitions[i]->getThisPartition(), SEEK_SET);
            mMXFFile->readKL(&key, &llen, &len);
            mMXFFile->skip(len);

            bool have_stream_key = false;
            while (!mMXFFile->eof())
            {
                mMXFFile->readNextNonFillerKL(&key, &llen, &len);

                if (mxf_is_partition_pack(&key)) {
                    break;
                } else if (mxf_is_header_metadata(&key)) {
                    if (partitions[i]->getHeaderByteCount() > mxfKey_extlen + llen + len)
                        mMXFFile->skip(partitions[i]->getHeaderByteCount() - (mxfKey_extlen + llen));
                    else
                        mMXFFile->skip(len);
                } else if (mxf_is_index_table_segment(&key)) {
                    if (partitions[i]->getIndexByteCount() > mxfKey_extlen + llen + len)
                        mMXFFile->skip(partitions[i]->getIndexByteCount() - (mxfKey_extlen + llen));
                    else
                        mMXFFile->skip(len);
                } else if (MatchesExpectedStreamKey(&key)) {
                    if (body_size == 0)
                        mStreamKey = key;
                    else if (mStreamKey != key)
                        mStreamKey = g_Null_Key;  // stream key is not fixed

                    if (data) {
                        if (len > UINT32_MAX || (uint64_t)buffer.GetAllocatedSize() + len > UINT32_MAX)
                            BMX_EXCEPTION(("Stream data size exceeds maximum supported in-memory size"));
                        buffer.Grow((uint32_t)len);
                    } else if (file || bmx_io) {
                        buffer.Allocate(8192);
                    }

                    if (data || file || bmx_io) {
                        uint64_t rem_len = len;
                        while (rem_len > 0) {
                            uint32_t count = 8192;
                            if (count > rem_len)
                                count = (uint32_t)rem_len;
                            uint32_t num_read = mMXFFile->read(buffer.GetBytesAvailable(), count);
                            if (num_read != count)
                                BMX_EXCEPTION(("Failed to read data from generic stream"));

                            if (file) {
                                size_t num_write = fwrite(buffer.GetBytesAvailable(), 1, num_read, file);
                                if (num_write != num_read) {
                                    BMX_EXCEPTION(("Failed to write generic stream data to file: %s",
                                                   bmx_strerror(errno).c_str()));
                                }
                            } else if (bmx_io) {
                                uint32_t num_write = bmx_io->Write(buffer.GetBytesAvailable(), num_read);
                                if (num_write != num_read) {
                                    BMX_EXCEPTION(("Failed to write generic stream data to BMX IO"));
                                }
                            } else if (data) {
                                buffer.IncrementSize(num_read);
                            }

                            rem_len -= num_read;
                        }
                    } else {
                        if (len > 0) {
                            mFileOffsetRanges.push_back(make_pair(mMXFFile->tell(), len));
                            mMXFFile->skip(len);
                        }
                    }

                    have_stream_key = true;
                    body_size += mxfKey_extlen + llen + len;
                } else {
                    mMXFFile->skip(len);
                    if (have_stream_key)
                        body_size += mxfKey_extlen + llen + len;
                }
            }
        }

        if (data) {
            if (buffer.GetSize() > 0) {
                *data = buffer.GetBytes();
                *size = buffer.GetSize();
                buffer.TakeBytes();
            } else {
                *data = 0;
                *size = 0;
            }
        }

        mMXFFile->seek(original_file_pos, SEEK_SET);
    }
    catch (...)
    {
        mMXFFile->seek(original_file_pos, SEEK_SET);
        throw;
    }
}
