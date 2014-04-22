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


#define RDD6_START_TEXT   0x02 // next character is the start of the text description
#define RDD6_END_TEXT     0x03 // previous character was the last character of the text description
#define RDD6_NUL_TEXT     0x00 // no text description


namespace bmx
{


void get_program_config_info(uint8_t program_config, uint8_t *program_count, uint8_t *channel_count);


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
    static void GetProgramConfigInfo(const unsigned char *payload, uint32_t size,
                                     uint8_t *program_count, uint8_t *channel_count);
    static void UpdateStatic(unsigned char *payload, uint32_t size, const std::vector<uint8_t> &description_text_chars);
    static void GetDescriptionTextChars(const unsigned char *payload, uint32_t size, std::vector<uint8_t> *desc_chars);

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

public:
    // used in parsing/unparsing XML
    std::string xml_desc_elements[8];
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
    uint8_t reserved;               // 8 bits
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
    static void UpdateStatic(unsigned char *payload, uint32_t size);

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
    uint16_t ac3_timecod1;          // 14 bits
    uint8_t ac3_timecod2e;          // 1 bits
    uint16_t ac3_timecod2;          // 14 bits
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

    void Construct8Bit(PutBitBuffer *buffer);
    void Parse8Bit(RDD6GetBitBuffer *buffer);

    void Construct8BitPayload(const RDD6ParsedPayload *data);
    void Parse8BitPayload(RDD6ParsedPayload *data) const;

    void UnparseXML(XMLWriter *writer) const;

    uint8_t CalcChecksum();

    void UpdateStatic(const std::vector<uint8_t> &description_text_chars);

    void GetDescriptionTextChars(std::vector<uint8_t> *desc_chars);
    void SetCumulativeDescriptionTextChars(const std::vector<std::string> &cumulative_desc_chars);

    void BufferPayload();

public:
    uint8_t id;                         // 8 bits
    uint16_t size;                      // 8 bits (input value 0 interpreted as 256)
    ByteArray payload_buffer;
    const unsigned char *payload;       // only valid for lifetime of referenced data
    uint8_t checksum;                   // 8 bits

public:
    // used for parsing/unparsing XML
    std::string xml_desc_elements[8];

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
    RDD6MetadataSubFrame(bool is_first);
    ~RDD6MetadataSubFrame();

    void Construct8Bit(PutBitBuffer *buffer);
    void Parse8Bit(RDD6GetBitBuffer *buffer);

    void UnparseXML(XMLWriter *writer) const;

    void UpdateStatic(const std::vector<uint8_t> &description_text_chars, uint16_t frame_count);

    void GetDescriptionTextChars(std::vector<uint8_t> *desc_chars);
    void SetCumulativeDescriptionTextChars(const std::vector<std::string> &cumulative_desc_chars);

    void BufferPayloads();

    bool IsFirst() const { return sync_segment.start_subframe_sync_word == FIRST_SUBFRAME_SYNC_WORD; }

public:
    RDD6SyncSegment sync_segment;
    std::vector<RDD6DataSegment*> data_segments;

private:
    void ClearDataSegments();
};


class RDD6MetadataSequence;

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

    void ConstructST2020(ByteArray *data, uint8_t sdid, bool first);
    void ParseST2020(const unsigned char *st2020_data_a, uint32_t st2020_size_a,
                     const unsigned char *st2020_data_b, uint32_t st2020_size_b);

    void Construct8Bit(ByteArray *data, uint32_t *first_end_offset);
    void Construct8Bit(PutBitBuffer *buffer, uint32_t *first_end_offset);
    void Parse8Bit(const unsigned char *data_a, uint32_t size_a,
                   const unsigned char *data_b, uint32_t size_b);
    void Parse8Bit(RDD6GetBitBuffer *buffer);

    bool UnparseXML(XMLWriter *writer) const;
    bool UnparseXML(const std::string &filename) const;
    bool ParseXML(const std::string &filename);

    void InitStaticSequence(RDD6MetadataSequence *sequence);
    void UpdateStaticFrame(const RDD6MetadataSequence *sequence);
    void UpdateStaticFrameForXML(const RDD6MetadataSequence *sequence);

    void GetDescriptionTextChars(std::vector<uint8_t> *desc_chars);
    void SetCumulativeDescriptionTextChars(const std::vector<std::string> &cumulative_desc_chars);

    void BufferPayloads();

    bool IsEmpty() const    { return !first_sub_frame && !second_sub_frame; }
    bool IsComplete() const { return first_sub_frame && second_sub_frame; }

public:
    RDD6MetadataSubFrame *first_sub_frame;
    RDD6MetadataSubFrame *second_sub_frame;
    uint16_t end_frame_sync_word;               // 16 bits

private:
    void Reset();

    void ParseST2020Header(const unsigned char *st2020_data, uint32_t st2020_size,
                           const unsigned char **data, uint32_t *size);
};


class RDD6MetadataSequence
{
public:
    RDD6MetadataSequence();
    ~RDD6MetadataSequence();

    void Clear();
    void AppendDescriptionText(const std::string &text);

    std::vector<uint8_t> GetDescriptionTextChars() const;

    void UpdateForNextStaticFrame();

public:
    uint16_t start_frame_count;
    uint16_t frame_count;
    std::vector<std::pair<std::string, int> > description_text;
};


};



#endif

