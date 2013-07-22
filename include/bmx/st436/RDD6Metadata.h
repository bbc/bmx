/*
 * Copyright (C) 2013, British Broadcasting Corporation
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

#ifndef BMX_RDD6_METADATA_H_
#define BMX_RDD6_METADATA_H_

#include <string>
#include <vector>

#include <bmx/BMXTypes.h>
#include <bmx/ByteArray.h>
#include <bmx/st436/RDD6BitBuffer.h>



namespace bmx
{


class XMLWriter;

class RDD6DataSegment;

class RDD6ParsedPayload
{
public:
    virtual ~RDD6ParsedPayload() {}

    virtual bool Validate() = 0;

    virtual void Construct8BitPayload(RDD6DataSegment *segment) const = 0;
    virtual void Parse8BitPayload(const RDD6DataSegment *segment) = 0;

    virtual void UnparseXML(XMLWriter *writer) const = 0;
};


class RDD6DolbyEComplete : public RDD6ParsedPayload
{
public:
    RDD6DolbyEComplete();

    virtual bool Validate();

    virtual void Construct8BitPayload(RDD6DataSegment *segment) const;
    virtual void Parse8BitPayload(const RDD6DataSegment *segment);

    virtual void UnparseXML(XMLWriter *writer) const;

public:
    uint8_t program_config;         // 6 bits
    uint8_t frame_rate_code;        // 4 bits
    int16_t pitch_shift_code;       // 12 bits
    uint64_t smpte_time_code;       // 64 bits
    uint8_t reserved;               // 8 bits
    struct
    {
        uint8_t description_text;   // 8 bits
        uint8_t reserved;           // 2 bits
    } desc_elements[8];
    struct
    {
        uint8_t reserved_a;         // 4 bits
        uint8_t reserved_b;         // 1 bits
    } res1_elements[8];
    struct
    {
        uint16_t reserved_a;        // 10 bits
        uint16_t reserved_b;        // 10 bits
    } res2_elements[8];
};


class RDD6DolbyEEssential : public RDD6ParsedPayload
{
public:
    RDD6DolbyEEssential();

    virtual bool Validate();

    virtual void Construct8BitPayload(RDD6DataSegment *segment) const;
    virtual void Parse8BitPayload(const RDD6DataSegment *segment);

    virtual void UnparseXML(XMLWriter *writer) const;

public:
    uint8_t program_config;         // 6 bits
    uint8_t frame_rate_code;        // 4 bits
    int16_t pitch_shift_code;       // 12 bits
    uint8_t reserved;               // 8 bits;
};


class RDD6DolbyDigitalCompleteExtBSI : public RDD6ParsedPayload
{
public:
    RDD6DolbyDigitalCompleteExtBSI();

    virtual bool Validate();

    virtual void Construct8BitPayload(RDD6DataSegment *segment) const;
    virtual void Parse8BitPayload(const RDD6DataSegment *segment);

    virtual void UnparseXML(XMLWriter *writer) const;

public:
    uint8_t program_id;             // 5 bits
    uint8_t ac3_datarate;           // 5 bits
    uint8_t ac3_bsmod;              // 3 bits
    uint8_t ac3_acmod;              // 3 bits
    uint8_t ac3_cmixlev;            // 2 bits
    uint8_t ac3_surmixlev;          // 2 bits
    uint8_t ac3_dsurmod;            // 2 bits
    uint8_t ac3_lfeon;              // 1 bits
    uint8_t ac3_dialnorm;           // 5 bits
    uint8_t ac3_langcode;           // 1 bits
    uint8_t ac3_langcod;            // 8 bits
    uint8_t ac3_audprodie;          // 1 bits
    uint8_t ac3_mixlevel;           // 5 bits
    uint8_t ac3_roomtyp;            // 2 bits
    uint8_t ac3_copyrightb;         // 1 bits
    uint8_t ac3_origbs;             // 1 bits
    uint8_t ac3_xbsi1e;             // 1 bits
    uint8_t ac3_dmixmod;            // 2 bits
    uint8_t ac3_ltrtcmixlev;        // 3 bits
    uint8_t ac3_ltrtsurmixlev;      // 3 bits
    uint8_t ac3_lorocmixlev;        // 3 bits
    uint8_t ac3_lorosurmixlev;      // 3 bits
    uint8_t ac3_xbsi2e;             // 1 bits
    uint8_t ac3_dsurexmod;          // 2 bits
    uint8_t ac3_dheadphonmod;       // 2 bits
    uint8_t ac3_adconvtyp;          // 1 bits
    uint8_t reserved_1;             // 8 bits
    uint8_t reserved_2;             // 1 bits
    uint8_t ac3_hpfon;              // 1 bits
    uint8_t ac3_bwlpfon;            // 1 bits
    uint8_t ac3_lfelpfon;           // 1 bits
    uint8_t ac3_sur90on;            // 1 bits
    uint8_t ac3_suratton;           // 1 bits
    uint8_t ac3_rfpremphon;         // 1 bits
    uint8_t ac3_compre;             // 1 bits
    uint8_t ac3_compr1;             // 8 bits
    uint8_t ac3_dynrnge;            // 1 bits
    uint8_t ac3_dynrng1;            // 8 bits
    uint8_t ac3_dynrng2;            // 8 bits
    uint8_t ac3_dynrng3;            // 8 bits
    uint8_t ac3_dynrng4;            // 8 bits
    uint8_t reserved_3;             // 1 bits
};


class RDD6DolbyDigitalEssentialExtBSI : public RDD6ParsedPayload
{
public:
    RDD6DolbyDigitalEssentialExtBSI();

    virtual bool Validate();

    virtual void Construct8BitPayload(RDD6DataSegment *segment) const;
    virtual void Parse8BitPayload(const RDD6DataSegment *segment);

    virtual void UnparseXML(XMLWriter *writer) const;

public:
    uint8_t program_id;             // 5 bits
    uint8_t ac3_datarate;           // 5 bits
    uint8_t ac3_bsmod;              // 3 bits
    uint8_t ac3_acmod;              // 3 bits
    uint8_t ac3_lfeon;              // 1 bits
    uint8_t ac3_dialnorm;           // 5 bits
    uint8_t ac3_compre;             // 1 bits
    uint8_t ac3_compr2;             // 8 bits
    uint8_t ac3_dynrnge;            // 1 bits
    uint8_t ac3_dynrng5;            // 8 bits
    uint8_t ac3_dynrng6;            // 8 bits
    uint8_t ac3_dynrng7;            // 8 bits
    uint8_t ac3_dynrng8;            // 8 bits
};


class RDD6DolbyDigitalComplete : public RDD6ParsedPayload
{
public:
    RDD6DolbyDigitalComplete();

    virtual bool Validate();

    virtual void Parse8BitPayload(const RDD6DataSegment *segment);
    virtual void Construct8BitPayload(RDD6DataSegment *segment) const;

    virtual void UnparseXML(XMLWriter *writer) const;

public:
    uint8_t program_id;             // 5 bits
    uint8_t reserved_1;             // 5 bits
    uint8_t ac3_bsmod;              // 3 bits
    uint8_t ac3_acmod;              // 3 bits
    uint8_t ac3_cmixlev;            // 2 bits
    uint8_t ac3_surmixlev;          // 2 bits
    uint8_t ac3_dsurmod;            // 2 bits
    uint8_t ac3_lfeon;              // 1 bits
    uint8_t ac3_dialnorm;           // 5 bits
    uint8_t ac3_langcode;           // 1 bits
    uint8_t ac3_langcod;            // 8 bits
    uint8_t ac3_audprodie;          // 1 bits
    uint8_t ac3_mixlevel;           // 5 bits
    uint8_t ac3_roomtyp;            // 2 bits
    uint8_t ac3_copyrightb;         // 1 bits
    uint8_t ac3_origbs;             // 1 bits
    uint8_t ac3_timecod1e;          // 1 bits
    uint8_t ac3_timecod1;           // 14 bits
    uint8_t ac3_timecod2e;          // 1 bits
    uint8_t ac3_timecod2;           // 14 bits
    uint8_t ac3_hpfon;              // 1 bits
    uint8_t ac3_bwlpfon;            // 1 bits
    uint8_t ac3_lfelpfon;           // 1 bits
    uint8_t ac3_sur90on;            // 1 bits
    uint8_t ac3_suratton;           // 1 bits
    uint8_t ac3_rfpremphon;         // 1 bits
    uint8_t ac3_compre;             // 1 bits
    uint8_t ac3_compr1;             // 8 bits
    uint8_t ac3_dynrnge;            // 1 bits
    uint8_t ac3_dynrng1;            // 8 bits
    uint8_t ac3_dynrng2;            // 8 bits
    uint8_t ac3_dynrng3;            // 8 bits
    uint8_t ac3_dynrng4;            // 8 bits
    uint8_t reserved_2;             // 1 bits
};

class RDD6DolbyDigitalEssential : public RDD6ParsedPayload
{
public:
    RDD6DolbyDigitalEssential();

    virtual bool Validate();

    virtual void Construct8BitPayload(RDD6DataSegment *segment) const;
    virtual void Parse8BitPayload(const RDD6DataSegment *segment);

    virtual void UnparseXML(XMLWriter *writer) const;

public:
    uint8_t program_id;             // 5 bits
    uint8_t ac3_datarate;           // 5 bits
    uint8_t ac3_bsmod;              // 3 bits
    uint8_t ac3_acmod;              // 3 bits
    uint8_t ac3_lfeon;              // 1 bits
    uint8_t ac3_dialnorm;           // 5 bits
    uint8_t ac3_compre;             // 1 bits
    uint8_t ac3_compr2;             // 8 bits
    uint8_t ac3_dynrnge;            // 1 bits
    uint8_t ac3_dynrng5;            // 8 bits
    uint8_t ac3_dynrng6;            // 8 bits
    uint8_t ac3_dynrng7;            // 8 bits
    uint8_t ac3_dynrng8;            // 8 bits
};



class RDD6DataSegment
{
public:
    friend class RDD6DolbyEComplete;
    friend class RDD6DolbyEEssential;
    friend class RDD6DolbyDigitalCompleteExtBSI;
    friend class RDD6DolbyDigitalEssentialExtBSI;
    friend class RDD6DolbyDigitalComplete;
    friend class RDD6DolbyDigitalEssential;

public:
    typedef enum
    {
        DS_NONE                             = 0,
        DS_DOLBY_E_COMPLETE                 = 1,
        DS_DOLBY_E_ESSENTIAL                = 2,
        DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI   = 3,
        DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI  = 4,
        DS_DOLBY_DIGITAL_COMPLETE           = 5,
        DS_DOLBY_DIGITAL_ESSENTIAL          = 6,
    } DataSegmentId;

public:
    RDD6DataSegment();

    void Construct8Bit(RDD6PutBitBuffer *buffer);
    void Parse8Bit(RDD6GetBitBuffer *buffer);

    void Construct8BitPayload(const RDD6ParsedPayload *data);
    void Parse8BitPayload(RDD6ParsedPayload *data) const;

    void UnparseXML(XMLWriter *writer) const;

    uint8_t CalcChecksum();

    uint8_t GetProgramId() const;

public:
    uint8_t id;                         // 8 bits
    uint16_t size;                      // 8 bits (input value 0 interpreted as 256)
    ByteArray payload_buffer;
    const unsigned char *payload;       // only valid for lifetime of referenced data
    uint8_t checksum;                   // 8 bits

private:
    void PrepareConstruct8BitPayload();
    void CompleteConstruct8BitPayload();
};


typedef struct
{
    uint16_t start_subframe_sync_word;  // 16 bits
    uint8_t revision_id;                // 4 bits
    uint8_t originator_id;              // 8 bits
    uint16_t originator_address;        // 16 bits
    uint16_t frame_count;               // 16 bits
    uint16_t reserved;                  // 12 bits
} RDD6SyncSegment;


class RDD6MetadataSubFrame
{
public:
    typedef enum
    {
        FIRST_SUBFRAME_SYNC_WORD    = 0xcffc,
        SECOND_SUBFRAME_SYNC_WORD   = 0x3ff3
    } RDD6StartSubframeSyncWord;

public:
    RDD6MetadataSubFrame();
    ~RDD6MetadataSubFrame();

    void Construct8Bit(RDD6PutBitBuffer *buffer);
    void Parse8Bit(RDD6GetBitBuffer *buffer);

    void UnparseXML(XMLWriter *writer) const;

    void GetDataSegments(uint8_t id, std::vector<RDD6DataSegment*> *segments) const;
    RDD6DataSegment* GetProgramDataSegment(uint8_t id, uint8_t program_id) const;

public:
    RDD6SyncSegment sync_segment;
    std::vector<RDD6DataSegment*> data_segments;

private:
    void ClearDataSegments();
};


class RDD6MetadataFrame
{
public:
    typedef enum
    {
        END_FRAME_SYNC_WORD = 0x0a0a
    } EndFrameSyncWord;

public:
    RDD6MetadataFrame();
    ~RDD6MetadataFrame();

    void Construct8Bit(ByteArray *data, uint32_t *first_end_offset);
    void Construct8Bit(RDD6PutBitBuffer *buffer, uint32_t *first_end_offset);
    void Parse8Bit(const unsigned char *data_a, uint32_t size_a,
                   const unsigned char *data_b, uint32_t size_b);
    void Parse8Bit(RDD6GetBitBuffer *buffer);

    bool UnparseXML(XMLWriter *writer) const;
    bool UnparseXML(const std::string &filename) const;

    std::vector<RDD6DataSegment*> GetDataSegments(uint8_t id) const;
    RDD6DataSegment* GetProgramDataSegment(uint8_t id, uint8_t program_id) const;

public:
    RDD6MetadataSubFrame *first_sub_frame;
    RDD6MetadataSubFrame *second_sub_frame;
    uint16_t end_frame_sync_word;               // 16 bits
};


};



#endif

