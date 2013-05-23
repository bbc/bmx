/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <cstring>

#include <bmx/mxf_reader/FrameMetadataReader.h>
#include <bmx/mxf_reader/MXFFileReader.h>
#include <bmx/MXFUtils.h>
#include <bmx/Utils.h>
#include <bmx/ByteArray.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;


static const mxfKey SS1_KEY_PREFIX                  = MXF_SS1_ELEMENT_KEY(0, 0);
static const mxfKey SDTI_CP_PACKAGE_META_KEY_PREFIX = MXF_SDTI_CP_PACKAGE_METADATA_KEY(0);

static const mxfKey ESSENCE_MARK_ISO7BIT_KEY =
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05, 0x03, 0x01, 0x02, 0x0a, 0x02, 0x00, 0x00, 0x00};
static const mxfKey ESSENCE_MARK_UTF16_KEY =
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05, 0x03, 0x01, 0x02, 0x0a, 0x02, 0x01, 0x00, 0x00};



SystemScheme1Reader::SystemScheme1Reader(File *file, Rational frame_rate, bool is_bbc_preservation_file)
{
    mFile = file;
    mFrameRate = frame_rate;
    mIsBBCPreservationFile = is_bbc_preservation_file;
    mTimecodeArray = 0;
}

SystemScheme1Reader::~SystemScheme1Reader()
{
    delete mTimecodeArray;
}

void SystemScheme1Reader::Reset()
{
    delete mTimecodeArray;
    mTimecodeArray = 0;
    mCRC32s.clear();
    mTrackNumbers.clear();
}

bool SystemScheme1Reader::ProcessFrameMetadata(const mxfKey *key, uint64_t len)
{
    if (!mxf_equals_key_prefix(key, &SS1_KEY_PREFIX, 14) ||
        (key->octet14 != 0x01 && key->octet14 != 0x02))
    {
        if (mxf_is_gc_essence_element(key))
            mTrackNumbers.push_back(mxf_get_track_number(key));
        return false;
    }

    uint64_t read_count = 0;
    uint16_t tag, tag_len;
    while (read_count < len) {
        mFile->readLocalTL(&tag, &tag_len);
        read_count += 4;

        switch (tag)
        {
            case 0x0102: // Timecode Array
            {
                BMX_CHECK_M(tag_len >= 8 && tag_len % 8 == 0,
                            ("Unexpected system scheme 1 timecode array data item length %u", len));

                uint32_t array_len, element_len;
                mFile->readBatchHeader(&array_len, &element_len);
                read_count += 8;

                SMPTE12MTimecode s12m;
                BMX_CHECK_M(array_len == 0 || element_len == sizeof(s12m.bytes),
                            ("Unexpected system scheme 1 timecode array element length %u", element_len));

                uint32_t i;
                for (i = 0; i < array_len; i++) {
                    if (!mTimecodeArray)
                        mTimecodeArray = new SS1TimecodeArray(mFrameRate, mIsBBCPreservationFile);
                    BMX_CHECK(mFile->read(s12m.bytes, sizeof(s12m.bytes)) == sizeof(s12m.bytes));
                    read_count += sizeof(s12m.bytes);
                    mTimecodeArray->mS12MTimecodes.push_back(s12m);
                }
                break;
            }
            case 0xffff:
                if (mIsBBCPreservationFile) {
                    BMX_CHECK_M(tag_len >= 8 && (tag_len - 8) % 4 == 0,
                                ("Unexpected BBC preservation CRC-32 array data item length %u", len));

                    uint32_t array_len, element_len;
                    mFile->readBatchHeader(&array_len, &element_len);
                    read_count += 8;

                    BMX_CHECK_M(array_len == 0 || element_len == 4,
                                ("Unexpected BBC preservation CRC-32 array element length %u", element_len));

                    uint32_t crc32;
                    uint32_t i;
                    for (i = 0; i < array_len; i++) {
                        crc32 = mFile->readUInt32();
                        read_count += 4;
                        mCRC32s.push_back(crc32);
                    }
                }
                break;
            default:
                mFile->skip(tag_len);
                read_count += tag_len;
                break;
        }
    }
    BMX_CHECK_M(read_count == len,
                ("System scheme 1 element length mismatch"));

    return true;
}

void SystemScheme1Reader::InsertFrameMetadata(Frame *frame, uint32_t track_number)
{
    if (mTimecodeArray)
        frame->InsertMetadata(new SS1TimecodeArray(*mTimecodeArray));

    if (!mCRC32s.empty()) {
        size_t i;
        for (i = 0; i < mCRC32s.size() && i < mTrackNumbers.size(); i++) {
            if (mTrackNumbers[i] == track_number) {
                frame->InsertMetadata(new SS1APPChecksum(mCRC32s[i]));
                break;
            }
        }
    }
}



SDTICPSystemMetadataReader::SDTICPSystemMetadataReader(File *file)
{
    mFile = file;
    mMetadata = 0;
}

SDTICPSystemMetadataReader::~SDTICPSystemMetadataReader()
{
    delete mMetadata;
}

void SDTICPSystemMetadataReader::Reset()
{
    delete mMetadata;
    mMetadata = 0;
}

bool SDTICPSystemMetadataReader::ProcessFrameMetadata(const mxfKey *key, uint64_t len)
{
    if (!mxf_equals_key(key, &MXF_EE_K(SDTI_CP_System_Pack)))
        return false;

    delete mMetadata;
    mMetadata = new SDTICPSystemMetadata();

    BMX_CHECK_M(len >= 2 && len <= 57,
                ("Unexpected len %"PRIu64" for system metadata pack", len));
    unsigned char bytes[57];
    uint32_t num_read = sizeof(bytes);
    if (num_read > len)
        num_read = (uint32_t)len;
    BMX_CHECK(mFile->read(bytes, num_read) == num_read);

    // Package Rate. See SMPTE ST 326, section 7.2
    // b6 and b7 not defined; b1-b5, rate per second; b0, 1 or 1.001 factor flag
    static const int32_t rate_numerators[13] = {0, 24, 25, 30, 48, 50, 60, 72, 75, 90, 96, 100, 120};
    unsigned char numer_byte = (bytes[1] >> 1) & 0x0f;
    if (numer_byte > 0 && numer_byte < sizeof(rate_numerators)) {
        mMetadata->mCPRate.numerator = rate_numerators[numer_byte];
        if (bytes[1] & 0x01) {
            mMetadata->mCPRate.numerator *= 1000;
            mMetadata->mCPRate.denominator = 1001;
        } else {
            mMetadata->mCPRate.denominator = 1;
        }
    }

    // User Timecode (User Date / Timestamp)
    if (mMetadata->mCPRate.numerator != 0 &&
        num_read >= 57 &&
        (bytes[0] & 0x10) &&    // user date/time stamp flag
        bytes[40] == 0x81)      // SMPTE 12M Timecode
    {
        mMetadata->mHaveUserTimecode = true;
        memcpy(mMetadata->mUserTimecode.bytes, &bytes[41], sizeof(mMetadata->mUserTimecode.bytes));
    }

    return true;
}

void SDTICPSystemMetadataReader::InsertFrameMetadata(Frame *frame, uint32_t track_number)
{
    (void)track_number;

    if (mMetadata)
        frame->InsertMetadata(new SDTICPSystemMetadata(*mMetadata));
}



SDTICPPackageMetadataReader::SDTICPPackageMetadataReader(File *file)
{
    mFile = file;
    mMetadata = 0;
}

SDTICPPackageMetadataReader::~SDTICPPackageMetadataReader()
{
    delete mMetadata;
}

void SDTICPPackageMetadataReader::Reset()
{
    delete mMetadata;
    mMetadata = 0;
}

bool SDTICPPackageMetadataReader::ProcessFrameMetadata(const mxfKey *key, uint64_t len)
{
    if (!mxf_equals_key_prefix(key, &SDTI_CP_PACKAGE_META_KEY_PREFIX, 15))
        return false;

    delete mMetadata;
    mMetadata = new SDTICPPackageMetadata();

    uint64_t read_count = 0;
    uint8_t block_tag;
    uint16_t item_len;
    ByteArray value;
    while (read_count < len) {
        block_tag = mFile->readUInt8();
        item_len = mFile->readUInt16();
        read_count += 3;

        switch (block_tag)
        {
            case 0x83:  // UMID
            {
                BMX_CHECK_M(item_len == 32 || item_len == 64,
                            ("Unexpected item len %u for UMID in system item package metadata", item_len));
                BMX_CHECK(mFile->read(mMetadata->mUMID.bytes, item_len) == item_len);
                mMetadata->mHaveUMID = true;
                read_count += item_len;
                break;
            }
            case 0x88: // KLV metadata
            {
                BMX_CHECK_M(item_len >= 17,
                            ("Unexpected item len %u for KLV in system item package metadata", item_len));
                mxfKey key;
                uint64_t len;
                uint8_t llen;
                mFile->readKL(&key, &llen, &len);
                BMX_CHECK_M(item_len == 16 + llen + len,
                            ("Unexpected item len %u for KLV in system item package metadata", item_len));
                uint16_t u16_len = (uint16_t)len;

                if (u16_len > 0) {
                    if (mxf_equals_key(&key, &ESSENCE_MARK_ISO7BIT_KEY)) {
                        value.Allocate(u16_len + 1);
                        mFile->read(value.GetBytes(), u16_len);
                        value.GetBytes()[u16_len] = 0;
                        mMetadata->mEssenceMark = (char*)value.GetBytes();
                    } else if (mxf_equals_key(&key, &ESSENCE_MARK_UTF16_KEY)) {
                        value.Allocate(u16_len);
                        mFile->read(value.GetBytes(), u16_len);
                        mMetadata->mEssenceMark = convert_utf16_string(value.GetBytes(), u16_len);
                    } else {
                        mFile->skip(u16_len);
                    }
                }

                read_count += item_len;
                break;
            }
            default:
                mFile->skip(item_len);
                read_count += item_len;
                break;
        }
    }
    BMX_CHECK_M(read_count == len,
                ("System item package metadata pack length mismatch"));

    return true;
}

void SDTICPPackageMetadataReader::InsertFrameMetadata(Frame *frame, uint32_t track_number)
{
    (void)track_number;

    if (mMetadata)
        frame->InsertMetadata(new SDTICPPackageMetadata(*mMetadata));
}



FrameMetadataReader::FrameMetadataReader(MXFFileReader *file_reader)
{
    bool is_bbc_preservation_file = false;
    vector<mxfUL> dm_schemes = file_reader->GetHeaderMetadata()->getPreface()->getDMSchemes();
    size_t i;
    for (i = 0; i < dm_schemes.size(); i++) {
        if (mxf_equals_ul_mod_regver(&MXF_DM_L(APP_PreservationDescriptiveScheme), &dm_schemes[i])) {
            is_bbc_preservation_file = true;
            break;
        }
    }

    mReaders.push_back(new SystemScheme1Reader(file_reader->mFile, file_reader->GetEditRate(),
                                               is_bbc_preservation_file));
    mReaders.push_back(new SDTICPSystemMetadataReader(file_reader->mFile));
    mReaders.push_back(new SDTICPPackageMetadataReader(file_reader->mFile));
}

FrameMetadataReader::~FrameMetadataReader()
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        delete mReaders[i];
}

void FrameMetadataReader::Reset()
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        mReaders[i]->Reset();
}

bool FrameMetadataReader::ProcessFrameMetadata(const mxfKey *key, uint64_t len)
{
    bool result = false;
    size_t i;
    for (i = 0; i < mReaders.size(); i++) {
        if (mReaders[i]->ProcessFrameMetadata(key, len))
            result = true;
    }
    return result;
}

void FrameMetadataReader::InsertFrameMetadata(Frame *frame, uint32_t track_number)
{
    size_t i;
    for (i = 0; i < mReaders.size(); i++)
        mReaders[i]->InsertFrameMetadata(frame, track_number);
}

