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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_LIMIT_MACROS

#include <cstring>

#include <memory>

#include <bmx/st436/RDD6Metadata.h>
#include "RDD6MetadataXML.h"
#include <bmx/XMLWriter.h>
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;



typedef struct
{
    uint8_t program_config;
    uint8_t program_count;
    uint8_t channel_count;
} ProgramConfigInfo;

static const ProgramConfigInfo PROGRAM_CONFIG_INFO[] =
{
    { 0, 2, 8},
    { 1, 3, 8},
    { 2, 2, 8},
    { 3, 3, 8},
    { 4, 4, 8},
    { 5, 5, 8},
    { 6, 4, 8},
    { 7, 5, 8},
    { 8, 6, 8},
    { 9, 7, 8},
    {10, 8, 8},
    {11, 1, 6},
    {12, 2, 6},
    {13, 3, 6},
    {14, 3, 6},
    {15, 4, 6},
    {16, 5, 6},
    {17, 6, 6},
    {18, 1, 4},
    {19, 2, 4},
    {20, 3, 4},
    {21, 4, 4},
    {22, 1, 8},
    {23, 1, 8},
};



static bool get_program_config_info(uint8_t program_config, uint8_t *program_count, uint8_t *channel_count)
{
    if (program_config >= BMX_ARRAY_SIZE(PROGRAM_CONFIG_INFO))
        return false;

    *program_count = PROGRAM_CONFIG_INFO[program_config].program_count;
    *channel_count = PROGRAM_CONFIG_INFO[program_config].channel_count;

    return true;
}

static void validate_range(const string &name, int value, int min, int max)
{
    if (value < min) {
        log_error("Validation error: '%s' value %d < %d\n", name.c_str(), value, min);
        throw false;
    } else if (value > max) {
        log_error("Validation error: '%s' value %d > %d\n", name.c_str(), value, max);
        throw false;
    }
}



RDD6DolbyEComplete::RDD6DolbyEComplete()
{
    program_config = 0;
    frame_rate_code = 3;
    pitch_shift_code = 0;
    smpte_time_code = (uint64_t)0x3f << 48;
    reserved = 0;
    memset(desc_elements, 0, sizeof(desc_elements));
    memset(res1_elements, 0, sizeof(res1_elements));

    memset(res2_elements, 0, sizeof(res2_elements));
    size_t i;
    for (i = 0; i < BMX_ARRAY_SIZE(res2_elements); i++) {
        res2_elements[i].reserved_a = 0x3c0;
        res2_elements[i].reserved_b = 0x3c0;
    }
}

bool RDD6DolbyEComplete::Validate()
{
    try
    {
        validate_range("program_config", program_config, 0, 23);
        validate_range("frame_rate_code", frame_rate_code, 1, 5);
        validate_range("pitch_shift_code", pitch_shift_code, -2048, 2047);

        // allow any value in description_text. RDD6 states reserved values should be discarded

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RDD6DolbyEComplete::Construct8BitPayload(RDD6DataSegment *segment) const
{
    if (segment->id == RDD6DataSegment::DS_NONE) {
        segment->id = RDD6DataSegment::DS_DOLBY_E_COMPLETE;
    } else if (segment->id != RDD6DataSegment::DS_DOLBY_E_COMPLETE) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested construct data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_E_COMPLETE));
    }

    uint8_t program_count, channel_count;
    if (!get_program_config_info(program_config, &program_count, &channel_count))
        BMX_EXCEPTION(("Unknown program config %u in data segment with id %u", program_config, segment->id));

    segment->PrepareConstruct8BitPayload();

    PutBitBuffer buffer(&segment->payload_buffer);
    buffer.PutBits( 6, program_config);
    buffer.PutBits( 4, frame_rate_code);
    buffer.PutBits(12, pitch_shift_code);
    buffer.PutBits(64, smpte_time_code);
    buffer.PutBits( 8, reserved);
    uint8_t i;
    for (i = 0; i < program_count; i++) {
        buffer.PutBits(8, desc_elements[i].description_text);
        buffer.PutBits(2, desc_elements[i].reserved);
    }
    for (i = 0; i < channel_count; i++) {
        buffer.PutBits(4, res1_elements[i].reserved_a);
        buffer.PutBits(1, res1_elements[i].reserved_b);
    }
    for (i = 0; i < channel_count; i++) {
        buffer.PutBits(10, res2_elements[i].reserved_a);
        buffer.PutBits(10, res2_elements[i].reserved_b);
    }

    segment->CompleteConstruct8BitPayload();
}

void RDD6DolbyEComplete::Parse8BitPayload(const RDD6DataSegment *segment)
{
    if (segment->id != RDD6DataSegment::DS_DOLBY_E_COMPLETE) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_E_COMPLETE));
    }

    RDD6GetBitBuffer buffer(segment->payload, segment->size, 0, 0);
    uint8_t program_count, channel_count;
    uint8_t i;

    if (buffer.GetRemBitSize() < 6)
        BMX_EXCEPTION(("Insufficient payload data in RDD6 data segment with id %u", segment->id));
    buffer.GetBits(6, &program_config);

    if (!get_program_config_info(program_config, &program_count, &channel_count))
        BMX_EXCEPTION(("Unknown program config %u in data segment with id %u", program_config, segment->id));

    if (buffer.GetRemBitSize() < (uint64_t)88 + program_count * 10 + channel_count * 25)
        BMX_EXCEPTION(("Insufficient payload data in RDD6 data segment with id %u", segment->id));

    buffer.GetBits( 4, &frame_rate_code);
    buffer.GetBits(12, &pitch_shift_code);
    buffer.GetBits(64, &smpte_time_code);
    buffer.GetBits( 8, &reserved);
    for (i = 0; i < program_count; i++) {
        buffer.GetBits(8, &desc_elements[i].description_text);
        buffer.GetBits(2, &desc_elements[i].reserved);
    }
    for (i = 0; i < channel_count; i++) {
        buffer.GetBits(4, &res1_elements[i].reserved_a);
        buffer.GetBits(1, &res1_elements[i].reserved_b);
    }
    for (i = 0; i < channel_count; i++) {
        buffer.GetBits(10, &res2_elements[i].reserved_a);
        buffer.GetBits(10, &res2_elements[i].reserved_b);
    }
}

void RDD6DolbyEComplete::UnparseXML(XMLWriter *writer) const
{
    uint8_t i;
    uint8_t program_count, channel_count;
    BMX_CHECK(get_program_config_info(program_config, &program_count, &channel_count));

    writer->WriteElementStart(RDD6_NAMESPACE, "dolby_e_complete");

    writer->WriteElement(RDD6_NAMESPACE, "program_config",
                         unparse_xml_enum("program_config", PROGRAM_CONFIG_ENUM, program_config));
    writer->WriteElement(RDD6_NAMESPACE, "frame_rate",
                         unparse_xml_enum("frame_rate", FRAME_RATE_ENUM, frame_rate_code));
    if (pitch_shift_code != 0)
        writer->WriteElement(RDD6_NAMESPACE, "pitch_shift_code", unparse_xml_int16(pitch_shift_code));
    if (((smpte_time_code >> 48) & 0x3f) != 0x3f) // hour = 0x3f indicates timecode is invalid
        writer->WriteElement(RDD6_NAMESPACE, "smpte_time_code", unparse_xml_smpte_timecode(smpte_time_code));

    bool write_descr_text = false;
    for (i = 0; i < program_count; i++) {
        if (desc_elements[i].description_text == 0x02 ||
            desc_elements[i].description_text == 0x03 ||
            (desc_elements[i].description_text >= 0x20 && desc_elements[i].description_text <= 0x7e))
        {
            write_descr_text = true;
            break;
        }
    }
    if (write_descr_text) {
        writer->WriteElementStart(RDD6_NAMESPACE, "descr_text");
        for (i = 0; i < program_count; i++) {
            writer->WriteElement(RDD6_NAMESPACE, "program",
                                 unparse_xml_char(desc_elements[i].description_text));
        }
        writer->WriteElementEnd();
    }

    writer->WriteElementEnd();
}



RDD6DolbyEEssential::RDD6DolbyEEssential()
{
    program_config = 0;
    frame_rate_code = 3;
    pitch_shift_code = 0;
    reserved = 0;
}

bool RDD6DolbyEEssential::Validate()
{
    try
    {
        validate_range("program_config", program_config, 0, 23);
        validate_range("frame_rate_code", frame_rate_code, 1, 5);
        validate_range("pitch_shift_code", pitch_shift_code, -2048, 2047);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RDD6DolbyEEssential::Construct8BitPayload(RDD6DataSegment *segment) const
{
    if (segment->id == RDD6DataSegment::DS_NONE) {
        segment->id = RDD6DataSegment::DS_DOLBY_E_ESSENTIAL;
    } else if (segment->id != RDD6DataSegment::DS_DOLBY_E_ESSENTIAL) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_E_ESSENTIAL));
    }

    segment->PrepareConstruct8BitPayload();

    PutBitBuffer buffer(&segment->payload_buffer);
    buffer.PutBits( 6, program_config);
    buffer.PutBits( 4, frame_rate_code);
    buffer.PutBits(12, pitch_shift_code);
    buffer.PutBits( 8, reserved);

    segment->CompleteConstruct8BitPayload();
}

void RDD6DolbyEEssential::Parse8BitPayload(const RDD6DataSegment *segment)
{
    if (segment->id != RDD6DataSegment::DS_DOLBY_E_ESSENTIAL) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_E_ESSENTIAL));
    }

    RDD6GetBitBuffer buffer(segment->payload, segment->size, 0, 0);

    if (buffer.GetRemBitSize() < 30)
        BMX_EXCEPTION(("Insufficient payload data in RDD6 data segment with id %u", segment->id));

    buffer.GetBits( 6, &program_config);
    buffer.GetBits( 4, &frame_rate_code);
    buffer.GetBits(12, &pitch_shift_code);
    buffer.GetBits( 8, &reserved);
}

void RDD6DolbyEEssential::UnparseXML(XMLWriter *writer) const
{
    uint8_t program_count, channel_count;
    BMX_CHECK(get_program_config_info(program_config, &program_count, &channel_count));

    writer->WriteElementStart(RDD6_NAMESPACE, "dolby_e_essential");

    writer->WriteElement(RDD6_NAMESPACE, "program_config",
                         unparse_xml_enum("program_config", PROGRAM_CONFIG_ENUM, program_config));
    writer->WriteElement(RDD6_NAMESPACE, "frame_rate",
                         unparse_xml_enum("frame_rate", FRAME_RATE_ENUM, frame_rate_code));
    if (pitch_shift_code != 0)
        writer->WriteElement(RDD6_NAMESPACE, "pitch_shift_code", unparse_xml_int16(pitch_shift_code));

    writer->WriteElementEnd();
}



RDD6DolbyDigitalCompleteExtBSI::RDD6DolbyDigitalCompleteExtBSI()
{
    program_id = 0;
    ac3_datarate = 0;
    ac3_bsmod = 0;
    ac3_acmod = 1;
    ac3_cmixlev = 0;
    ac3_surmixlev = 0;
    ac3_dsurmod = 0;
    ac3_lfeon = 0;
    ac3_dialnorm = 0;
    ac3_langcode = 0;
    ac3_langcod = 0;
    ac3_audprodie = 0;
    ac3_mixlevel = 0;
    ac3_roomtyp = 0;
    ac3_copyrightb = 0;
    ac3_origbs = 0;
    ac3_xbsi1e = 0;
    ac3_dmixmod = 0;
    ac3_ltrtcmixlev = 0;
    ac3_ltrtsurmixlev = 0;
    ac3_lorocmixlev = 0;
    ac3_lorosurmixlev = 0;
    ac3_xbsi2e = 0;
    ac3_dsurexmod = 0;
    ac3_dheadphonmod = 0;
    ac3_adconvtyp = 0;
    reserved_1 = 0;
    reserved_2 = 0;
    ac3_hpfon = 0;
    ac3_bwlpfon = 0;
    ac3_lfelpfon = 0;
    ac3_sur90on = 0;
    ac3_suratton = 0;
    ac3_rfpremphon = 0;
    ac3_compre = 0;
    ac3_compr1 = 0;
    ac3_dynrnge = 0;
    ac3_dynrng1 = 0;
    ac3_dynrng2 = 0;
    ac3_dynrng3 = 0;
    ac3_dynrng4 = 0;
    reserved_3 = 0;
}

bool RDD6DolbyDigitalCompleteExtBSI::Validate()
{
    try
    {
        validate_range("program_id",        program_id, 0, 7);
        if (ac3_datarate != 31)
            validate_range("ac3_datarate",  ac3_datarate, 0, 18);
        validate_range("ac3_bsmod",         ac3_bsmod, 0, 7);
        validate_range("ac3_acmod",         ac3_acmod, 1, 7);
        validate_range("ac3_cmixlev",       ac3_cmixlev, 0, 3);
        validate_range("ac3_surmixlev",     ac3_surmixlev, 0, 3);
        validate_range("ac3_dsurmod",       ac3_dsurmod, 0, 3);
        validate_range("ac3_lfeon",         ac3_lfeon, 0, 1);
        validate_range("ac3_dialnorm",      ac3_dialnorm, 0, 31);
        validate_range("ac3_langcode",      ac3_langcode, 0, 1);
        validate_range("ac3_langcod",       ac3_langcod, 0, 0x7f);
        validate_range("ac3_audprodie",     ac3_audprodie, 0, 1);
        validate_range("ac3_mixlevel",      ac3_mixlevel, 0, 31);
        validate_range("ac3_roomtyp",       ac3_roomtyp, 0, 3);
        validate_range("ac3_copyrightb",    ac3_copyrightb, 0, 1);
        validate_range("ac3_origbs",        ac3_origbs, 0, 1);
        validate_range("ac3_xbsi1e",        ac3_xbsi1e, 0, 1);
        if (!ac3_xbsi1e) {
            log_error("Validation error: 'ac3_xbsi1e' value must be 1 in extended set\n");
            return false;
        }
        validate_range("ac3_dmixmod",       ac3_dmixmod, 0, 3);
        validate_range("ac3_ltrtcmixlev",   ac3_ltrtcmixlev, 0, 7);
        validate_range("ac3_ltrtsurmixlev", ac3_ltrtsurmixlev, 0, 7);
        validate_range("ac3_lorocmixlev",   ac3_lorocmixlev, 0, 7);
        validate_range("ac3_lorosurmixlev", ac3_lorosurmixlev, 0, 7);
        validate_range("ac3_xbsi2e",        ac3_xbsi2e, 0, 1);
        if (!ac3_xbsi2e) {
            log_error("Validation error: 'ac3_xbsi2e' value must be 1 in extended set\n");
            return false;
        }
        validate_range("ac3_dsurexmod",     ac3_dsurexmod, 0, 3);
        validate_range("ac3_dheadphonmod",  ac3_dheadphonmod, 0, 3);
        validate_range("ac3_adconvtyp",     ac3_adconvtyp, 0, 1);
        validate_range("ac3_hpfon",         ac3_hpfon, 0, 1);
        validate_range("ac3_bwlpfon",       ac3_bwlpfon, 0, 1);
        validate_range("ac3_lfelpfon",      ac3_lfelpfon, 0, 1);
        validate_range("ac3_sur90on",       ac3_sur90on, 0, 1);
        validate_range("ac3_suratton",      ac3_suratton, 0, 1);
        validate_range("ac3_rfpremphon",    ac3_rfpremphon, 0, 1);
        validate_range("ac3_compre",        ac3_compre, 0, 1);
        if (ac3_compre == 0)
            validate_range("ac3_compr1",    ac3_compr1, 0, 5);
        else
            validate_range("ac3_compr1",    ac3_compr1, 0, 255);
        validate_range("ac3_dynrnge",       ac3_dynrnge, 0, 1);
        if (ac3_dynrnge == 0) {
            validate_range("ac3_dynrng1",   ac3_dynrng1, 0, 5);
            validate_range("ac3_dynrng2",   ac3_dynrng2, 0, 5);
            validate_range("ac3_dynrng3",   ac3_dynrng3, 0, 5);
            validate_range("ac3_dynrng4",   ac3_dynrng4, 0, 5);
        } else {
            validate_range("ac3_dynrng1",   ac3_dynrng1, 0, 255);
            validate_range("ac3_dynrng2",   ac3_dynrng2, 0, 255);
            validate_range("ac3_dynrng3",   ac3_dynrng3, 0, 255);
            validate_range("ac3_dynrng4",   ac3_dynrng4, 0, 255);
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RDD6DolbyDigitalCompleteExtBSI::Construct8BitPayload(RDD6DataSegment *segment) const
{
    if (segment->id == RDD6DataSegment::DS_NONE) {
        segment->id = RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI;
    } else if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI));
    }

    segment->PrepareConstruct8BitPayload();

    PutBitBuffer buffer(&segment->payload_buffer);
    buffer.PutBits(5, program_id);
    buffer.PutBits(5, ac3_datarate);
    buffer.PutBits(3, ac3_bsmod);
    buffer.PutBits(3, ac3_acmod);
    buffer.PutBits(2, ac3_cmixlev);
    buffer.PutBits(2, ac3_surmixlev);
    buffer.PutBits(2, ac3_dsurmod);
    buffer.PutBits(1, ac3_lfeon);
    buffer.PutBits(5, ac3_dialnorm);
    buffer.PutBits(1, ac3_langcode);
    buffer.PutBits(8, ac3_langcod);
    buffer.PutBits(1, ac3_audprodie);
    buffer.PutBits(5, ac3_mixlevel);
    buffer.PutBits(2, ac3_roomtyp);
    buffer.PutBits(1, ac3_copyrightb);
    buffer.PutBits(1, ac3_origbs);
    buffer.PutBits(1, ac3_xbsi1e);
    buffer.PutBits(2, ac3_dmixmod);
    buffer.PutBits(3, ac3_ltrtcmixlev);
    buffer.PutBits(3, ac3_ltrtsurmixlev);
    buffer.PutBits(3, ac3_lorocmixlev);
    buffer.PutBits(3, ac3_lorosurmixlev);
    buffer.PutBits(1, ac3_xbsi2e);
    buffer.PutBits(2, ac3_dsurexmod);
    buffer.PutBits(2, ac3_dheadphonmod);
    buffer.PutBits(1, ac3_adconvtyp);
    buffer.PutBits(8, reserved_1);
    buffer.PutBits(1, reserved_2);
    buffer.PutBits(1, ac3_hpfon);
    buffer.PutBits(1, ac3_bwlpfon);
    buffer.PutBits(1, ac3_lfelpfon);
    buffer.PutBits(1, ac3_sur90on);
    buffer.PutBits(1, ac3_suratton);
    buffer.PutBits(1, ac3_rfpremphon);
    buffer.PutBits(1, ac3_compre);
    buffer.PutBits(8, ac3_compr1);
    buffer.PutBits(1, ac3_dynrnge);
    buffer.PutBits(8, ac3_dynrng1);
    buffer.PutBits(8, ac3_dynrng2);
    buffer.PutBits(8, ac3_dynrng3);
    buffer.PutBits(8, ac3_dynrng4);
    buffer.PutBits(1, reserved_3);

    segment->CompleteConstruct8BitPayload();
}

void RDD6DolbyDigitalCompleteExtBSI::Parse8BitPayload(const RDD6DataSegment *segment)
{
    if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI));
    }

    RDD6GetBitBuffer buffer(segment->payload, segment->size, 0, 0);

    if (buffer.GetRemBitSize() < 126)
        BMX_EXCEPTION(("Insufficient payload data in RDD6 data segment with id %u", segment->id));

    buffer.GetBits(5, &program_id);
    buffer.GetBits(5, &ac3_datarate);
    buffer.GetBits(3, &ac3_bsmod);
    buffer.GetBits(3, &ac3_acmod);
    buffer.GetBits(2, &ac3_cmixlev);
    buffer.GetBits(2, &ac3_surmixlev);
    buffer.GetBits(2, &ac3_dsurmod);
    buffer.GetBits(1, &ac3_lfeon);
    buffer.GetBits(5, &ac3_dialnorm);
    buffer.GetBits(1, &ac3_langcode);
    buffer.GetBits(8, &ac3_langcod);
    buffer.GetBits(1, &ac3_audprodie);
    buffer.GetBits(5, &ac3_mixlevel);
    buffer.GetBits(2, &ac3_roomtyp);
    buffer.GetBits(1, &ac3_copyrightb);
    buffer.GetBits(1, &ac3_origbs);
    buffer.GetBits(1, &ac3_xbsi1e);
    buffer.GetBits(2, &ac3_dmixmod);
    buffer.GetBits(3, &ac3_ltrtcmixlev);
    buffer.GetBits(3, &ac3_ltrtsurmixlev);
    buffer.GetBits(3, &ac3_lorocmixlev);
    buffer.GetBits(3, &ac3_lorosurmixlev);
    buffer.GetBits(1, &ac3_xbsi2e);
    buffer.GetBits(2, &ac3_dsurexmod);
    buffer.GetBits(2, &ac3_dheadphonmod);
    buffer.GetBits(1, &ac3_adconvtyp);
    buffer.GetBits(8, &reserved_1);
    buffer.GetBits(1, &reserved_2);
    buffer.GetBits(1, &ac3_hpfon);
    buffer.GetBits(1, &ac3_bwlpfon);
    buffer.GetBits(1, &ac3_lfelpfon);
    buffer.GetBits(1, &ac3_sur90on);
    buffer.GetBits(1, &ac3_suratton);
    buffer.GetBits(1, &ac3_rfpremphon);
    buffer.GetBits(1, &ac3_compre);
    buffer.GetBits(8, &ac3_compr1);
    buffer.GetBits(1, &ac3_dynrnge);
    buffer.GetBits(8, &ac3_dynrng1);
    buffer.GetBits(8, &ac3_dynrng2);
    buffer.GetBits(8, &ac3_dynrng3);
    buffer.GetBits(8, &ac3_dynrng4);
    buffer.GetBits(1, &reserved_3);
}

void RDD6DolbyDigitalCompleteExtBSI::UnparseXML(XMLWriter *writer) const
{
    writer->WriteElementStart(RDD6_NAMESPACE, "dolby_digital_complete_ext_bsi");

    writer->WriteElement(RDD6_NAMESPACE, "program_id", unparse_xml_uint8(program_id));
    if (ac3_datarate != 31) { // 0x31 = not specified
        writer->WriteElement(RDD6_NAMESPACE, "data_rate",
                             unparse_xml_enum("data_rate", DATARATE_ENUM, ac3_datarate));
    }
    if (ac3_acmod <= 1) {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_1_ENUM, ac3_bsmod));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_2_ENUM, ac3_bsmod));
    }
    writer->WriteElement(RDD6_NAMESPACE, "ac_mode",
                         unparse_xml_enum("ac_mode", ACMOD_ENUM, ac3_acmod));
    writer->WriteElement(RDD6_NAMESPACE, "center_mix_level",
                         unparse_xml_enum("center_mix_level", CMIXLEVEL_ENUM, ac3_cmixlev));
    writer->WriteElement(RDD6_NAMESPACE, "sur_mix_level",
                         unparse_xml_enum("sur_mix_level", SURMIXLEVEL_ENUM, ac3_surmixlev));
    if (ac3_dsurmod == 1 || ac3_dsurmod == 2)
        writer->WriteElement(RDD6_NAMESPACE, "sur_encoded", unparse_xml_bool(ac3_dsurmod == 2));
    writer->WriteElement(RDD6_NAMESPACE, "lfe_on", unparse_xml_bool(ac3_lfeon));
    if (ac3_dialnorm == 0)
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(31)); // 0 is treated as 31
    else
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(ac3_dialnorm));
    if (ac3_langcode)
        writer->WriteElement(RDD6_NAMESPACE, "lang_code", unparse_xml_hex_uint8(ac3_langcod));
    if (ac3_audprodie) {
        writer->WriteElement(RDD6_NAMESPACE, "mix_level", unparse_xml_uint8(ac3_mixlevel));
        if (ac3_roomtyp == 1 || ac3_roomtyp == 2) {
            writer->WriteElement(RDD6_NAMESPACE, "room_type",
                                 unparse_xml_enum("room_type", ROOM_TYPE_ENUM, ac3_roomtyp));
        }
    }
    writer->WriteElement(RDD6_NAMESPACE, "copyright", unparse_xml_bool(ac3_copyrightb));
    writer->WriteElement(RDD6_NAMESPACE, "orig_bs", unparse_xml_bool(ac3_origbs));
    BMX_CHECK(ac3_xbsi1e);
    if (ac3_acmod == 3 || ac3_acmod == 4 || ac3_acmod == 5 || ac3_acmod == 6 || ac3_acmod == 7) {
        writer->WriteElement(RDD6_NAMESPACE, "downmix_mode",
                             unparse_xml_enum("downmix_mode", DMIXMOD_ENUM, ac3_dmixmod));
    }
    if (ac3_acmod == 3 || ac3_acmod == 5 || ac3_acmod == 7) {
        writer->WriteElement(RDD6_NAMESPACE, "lt_rt_center_mix",
                             unparse_xml_enum("lt_rt_center_mix", LTRTCMIXLEVEL_ENUM, ac3_ltrtcmixlev));
    }
    if (ac3_acmod == 4 || ac3_acmod == 5 || ac3_acmod == 6 || ac3_acmod == 7) {
        writer->WriteElement(RDD6_NAMESPACE, "lt_rt_sur_mix",
                             unparse_xml_enum("lt_rt_sur_mix", LTRTSURMIXLEVEL_ENUM, ac3_ltrtsurmixlev));
    }
    if (ac3_acmod == 3 || ac3_acmod == 5 || ac3_acmod == 7) {
        writer->WriteElement(RDD6_NAMESPACE, "lo_ro_center_mix",
                             unparse_xml_enum("lo_ro_center_mix", LOROCMIXLEVEL_ENUM, ac3_lorocmixlev));
    }
    if (ac3_acmod == 4 || ac3_acmod == 5 || ac3_acmod == 6 || ac3_acmod == 7) {
        writer->WriteElement(RDD6_NAMESPACE, "lo_ro_sur_mix",
                             unparse_xml_enum("lo_ro_sur_mix", LOROSURMIXLEVEL_ENUM, ac3_lorosurmixlev));
    }
    BMX_CHECK(ac3_xbsi2e);
    if ((ac3_acmod == 6 || ac3_acmod == 7) && (ac3_dsurexmod == 1 || ac3_dsurexmod == 2))
        writer->WriteElement(RDD6_NAMESPACE, "sur_ex_encoded", unparse_xml_bool(ac3_dsurexmod == 2));
    if (ac3_acmod == 2 && (ac3_dheadphonmod == 1 || ac3_dheadphonmod == 2))
        writer->WriteElement(RDD6_NAMESPACE, "headphone_encoded", unparse_xml_bool(ac3_dheadphonmod == 2));
    writer->WriteElement(RDD6_NAMESPACE, "ad_conv_type",
                         unparse_xml_enum("ad_conv_type", ADCONVTYPE_ENUM, ac3_adconvtyp));
    writer->WriteElement(RDD6_NAMESPACE, "hp_filter", unparse_xml_bool(ac3_hpfon));
    writer->WriteElement(RDD6_NAMESPACE, "bw_lp_filter", unparse_xml_bool(ac3_bwlpfon));
    writer->WriteElement(RDD6_NAMESPACE, "lfe_lp_filter", unparse_xml_bool(ac3_lfelpfon));
    writer->WriteElement(RDD6_NAMESPACE, "sur_90_filter", unparse_xml_bool(ac3_sur90on));
    writer->WriteElement(RDD6_NAMESPACE, "sur_att_filter", unparse_xml_bool(ac3_suratton));
    writer->WriteElement(RDD6_NAMESPACE, "rf_preemph_filter", unparse_xml_bool(ac3_rfpremphon));
    if (ac3_compre == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "comp_pf_1",
                             unparse_xml_enum("compr_pf_1", COMPR_ENUM, ac3_compr1));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "compr_wd_1", unparse_xml_hex_uint8(ac3_compr1));
    }
    if (ac3_dynrnge == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_1",
                             unparse_xml_enum("dyn_range_pf_1", DYNRNG_ENUM, ac3_dynrng1));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_2",
                             unparse_xml_enum("dyn_range_pf_2", DYNRNG_ENUM, ac3_dynrng2));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_3",
                             unparse_xml_enum("dyn_range_pf_3", DYNRNG_ENUM, ac3_dynrng3));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_4",
                             unparse_xml_enum("dyn_range_pf_4", DYNRNG_ENUM, ac3_dynrng4));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_1", unparse_xml_hex_uint8(ac3_dynrng1));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_2", unparse_xml_hex_uint8(ac3_dynrng2));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_3", unparse_xml_hex_uint8(ac3_dynrng3));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_4", unparse_xml_hex_uint8(ac3_dynrng4));
    }

    writer->WriteElementEnd();
}



RDD6DolbyDigitalEssentialExtBSI::RDD6DolbyDigitalEssentialExtBSI()
{
    program_id = 0;
    ac3_datarate = 0;
    ac3_bsmod = 0;
    ac3_acmod = 1;
    ac3_lfeon = 0;
    ac3_dialnorm = 0;
    ac3_compre = 0;
    ac3_compr2 = 0;
    ac3_dynrnge = 0;
    ac3_dynrng5 = 0;
    ac3_dynrng6 = 0;
    ac3_dynrng7 = 0;
    ac3_dynrng8 = 0;
}

void RDD6DolbyDigitalEssentialExtBSI::Parse8BitPayload(const RDD6DataSegment *segment)
{
    if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI));
    }

    RDD6GetBitBuffer buffer(segment->payload, segment->size, 0, 0);

    if (buffer.GetRemBitSize() < 64)
        BMX_EXCEPTION(("Insufficient payload data in RDD6 data segment with id %u", segment->id));

    buffer.GetBits(5, &program_id);
    buffer.GetBits(5, &ac3_datarate);
    buffer.GetBits(3, &ac3_bsmod);
    buffer.GetBits(3, &ac3_acmod);
    buffer.GetBits(1, &ac3_lfeon);
    buffer.GetBits(5, &ac3_dialnorm);
    buffer.GetBits(1, &ac3_compre);
    buffer.GetBits(8, &ac3_compr2);
    buffer.GetBits(1, &ac3_dynrnge);
    buffer.GetBits(8, &ac3_dynrng5);
    buffer.GetBits(8, &ac3_dynrng6);
    buffer.GetBits(8, &ac3_dynrng7);
    buffer.GetBits(8, &ac3_dynrng8);
}

void RDD6DolbyDigitalEssentialExtBSI::Construct8BitPayload(RDD6DataSegment *segment) const
{
    if (segment->id == RDD6DataSegment::DS_NONE) {
        segment->id = RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI;
    } else if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI));
    }

    segment->PrepareConstruct8BitPayload();

    PutBitBuffer buffer(&segment->payload_buffer);
    buffer.PutBits(5, program_id);
    buffer.PutBits(5, ac3_datarate);
    buffer.PutBits(3, ac3_bsmod);
    buffer.PutBits(3, ac3_acmod);
    buffer.PutBits(1, ac3_lfeon);
    buffer.PutBits(5, ac3_dialnorm);
    buffer.PutBits(1, ac3_compre);
    buffer.PutBits(8, ac3_compr2);
    buffer.PutBits(1, ac3_dynrnge);
    buffer.PutBits(8, ac3_dynrng5);
    buffer.PutBits(8, ac3_dynrng6);
    buffer.PutBits(8, ac3_dynrng7);
    buffer.PutBits(8, ac3_dynrng8);

    segment->CompleteConstruct8BitPayload();
}

bool RDD6DolbyDigitalEssentialExtBSI::Validate()
{
    try
    {
        validate_range("program_id",       program_id, 0, 7);
        if (ac3_datarate != 31)
            validate_range("ac3_datarate", ac3_datarate, 0, 18);
        validate_range("ac3_bsmod",        ac3_bsmod, 0, 7);
        validate_range("ac3_acmod",        ac3_acmod, 1, 7);
        validate_range("ac3_lfeon",        ac3_lfeon, 0, 1);
        validate_range("ac3_dialnorm",     ac3_dialnorm, 0, 31);
        validate_range("ac3_compre",       ac3_compre, 0, 1);
        if (ac3_compre == 0)
            validate_range("ac3_compr2",   ac3_compr2, 0, 5);
        else
            validate_range("ac3_compr2",   ac3_compr2, 0, 255);
        validate_range("ac3_dynrnge",      ac3_dynrnge, 0, 1);
        if (ac3_dynrnge == 0) {
            validate_range("ac3_dynrng5",  ac3_dynrng5, 0, 5);
            validate_range("ac3_dynrng6",  ac3_dynrng6, 0, 5);
            validate_range("ac3_dynrng7",  ac3_dynrng7, 0, 5);
            validate_range("ac3_dynrng8",  ac3_dynrng8, 0, 5);
        } else {
            validate_range("ac3_dynrng5",  ac3_dynrng5, 0, 255);
            validate_range("ac3_dynrng6",  ac3_dynrng6, 0, 255);
            validate_range("ac3_dynrng7",  ac3_dynrng7, 0, 255);
            validate_range("ac3_dynrng8",  ac3_dynrng8, 0, 255);
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RDD6DolbyDigitalEssentialExtBSI::UnparseXML(XMLWriter *writer) const
{
    writer->WriteElementStart(RDD6_NAMESPACE, "dolby_digital_essential_ext_bsi");

    writer->WriteElement(RDD6_NAMESPACE, "program_id", unparse_xml_uint8(program_id));
    if (ac3_datarate != 31) { // 0x31 = not specified
        writer->WriteElement(RDD6_NAMESPACE, "data_rate",
                             unparse_xml_enum("data_rate", DATARATE_ENUM, ac3_datarate));
    }
    if (ac3_acmod <= 1) {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_1_ENUM, ac3_bsmod));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_2_ENUM, ac3_bsmod));
    }
    writer->WriteElement(RDD6_NAMESPACE, "ac_mode",
                         unparse_xml_enum("ac_mode", ACMOD_ENUM, ac3_acmod));
    writer->WriteElement(RDD6_NAMESPACE, "lfe_on", unparse_xml_bool(ac3_lfeon));
    if (ac3_dialnorm == 0)
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(31)); // 0 is treated as 31
    else
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(ac3_dialnorm));
    if (ac3_compre == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "compr_pf_2",
                             unparse_xml_enum("compr_pf_2", COMPR_ENUM, ac3_compr2));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "compr_wd_2", unparse_xml_hex_uint8(ac3_compr2));
    }
    if (ac3_dynrnge == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_5",
                             unparse_xml_enum("dyn_range_pf_5", DYNRNG_ENUM, ac3_dynrng5));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_6",
                             unparse_xml_enum("dyn_range_pf_6", DYNRNG_ENUM, ac3_dynrng6));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_7",
                             unparse_xml_enum("dyn_range_pf_7", DYNRNG_ENUM, ac3_dynrng7));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_8",
                             unparse_xml_enum("dyn_range_pf_8", DYNRNG_ENUM, ac3_dynrng8));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_5", unparse_xml_hex_uint8(ac3_dynrng5));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_6", unparse_xml_hex_uint8(ac3_dynrng6));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_7", unparse_xml_hex_uint8(ac3_dynrng7));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_8", unparse_xml_hex_uint8(ac3_dynrng8));
    }

    writer->WriteElementEnd();
}



RDD6DolbyDigitalComplete::RDD6DolbyDigitalComplete()
{
    program_id = 0;
    reserved_1 = 0;
    ac3_bsmod = 0;
    ac3_acmod = 0;
    ac3_cmixlev = 0;
    ac3_surmixlev = 0;
    ac3_dsurmod = 0;
    ac3_lfeon = 0;
    ac3_dialnorm = 0;
    ac3_langcode = 0;
    ac3_langcod = 0;
    ac3_audprodie = 0;
    ac3_mixlevel = 0;
    ac3_roomtyp = 0;
    ac3_copyrightb = 0;
    ac3_origbs = 0;
    ac3_timecod1e = 0;
    ac3_timecod1 = 0;
    ac3_timecod2e = 0;
    ac3_timecod2 = 0;
    ac3_hpfon = 0;
    ac3_bwlpfon = 0;
    ac3_lfelpfon = 0;
    ac3_sur90on = 0;
    ac3_suratton = 0;
    ac3_rfpremphon = 0;
    ac3_compre = 0;
    ac3_compr1 = 0;
    ac3_dynrnge = 0;
    ac3_dynrng1 = 0;
    ac3_dynrng2 = 0;
    ac3_dynrng3 = 0;
    ac3_dynrng4 = 0;
    reserved_2 = 0;
}

bool RDD6DolbyDigitalComplete::Validate()
{
    try
    {
        validate_range("program_id",       program_id, 0, 7);
        validate_range("ac3_bsmod",        ac3_bsmod, 0, 7);
        validate_range("ac3_acmod",        ac3_acmod, 1, 7);
        validate_range("ac3_cmixlev",      ac3_cmixlev, 0, 3);
        validate_range("ac3_surmixlev",    ac3_surmixlev, 0, 3);
        validate_range("ac3_dsurmod",      ac3_dsurmod, 0, 3);
        validate_range("ac3_lfeon",        ac3_lfeon, 0, 1);
        validate_range("ac3_dialnorm",     ac3_dialnorm, 0, 31);
        validate_range("ac3_langcode",     ac3_langcode, 0, 1);
        validate_range("ac3_langcod",      ac3_langcod, 0, 0x7f);
        validate_range("ac3_audprodie",    ac3_audprodie, 0, 1);
        validate_range("ac3_mixlevel",     ac3_mixlevel, 0, 31);
        validate_range("ac3_roomtyp",      ac3_roomtyp, 0, 3);
        validate_range("ac3_copyrightb",   ac3_copyrightb, 0, 1);
        validate_range("ac3_origbs",       ac3_origbs, 0, 1);
        validate_range("ac3_timecod1e",    ac3_timecod1e, 0, 1);
        // ac3_timecod1
        validate_range("ac3_timecod2e",    ac3_timecod2e, 0, 1);
        // ac3_timecod2
        validate_range("ac3_hpfon",        ac3_hpfon, 0, 1);
        validate_range("ac3_bwlpfon",      ac3_bwlpfon, 0, 1);
        validate_range("ac3_lfelpfon",     ac3_lfelpfon, 0, 1);
        validate_range("ac3_sur90on",      ac3_sur90on, 0, 1);
        validate_range("ac3_suratton",     ac3_suratton, 0, 1);
        validate_range("ac3_rfpremphon",   ac3_rfpremphon, 0, 1);
        validate_range("ac3_compre",       ac3_compre, 0, 1);
        if (ac3_compre == 0)
            validate_range("ac3_compr1",   ac3_compr1, 0, 5);
        else
            validate_range("ac3_compr1",   ac3_compr1, 0, 255);
        validate_range("ac3_dynrnge",      ac3_dynrnge, 0, 1);
        if (ac3_dynrnge == 0) {
            validate_range("ac3_dynrng1",  ac3_dynrng1, 0, 5);
            validate_range("ac3_dynrng2",  ac3_dynrng2, 0, 5);
            validate_range("ac3_dynrng3",  ac3_dynrng3, 0, 5);
            validate_range("ac3_dynrng4",  ac3_dynrng4, 0, 5);
        } else {
            validate_range("ac3_dynrng1",  ac3_dynrng1, 0, 255);
            validate_range("ac3_dynrng2",  ac3_dynrng2, 0, 255);
            validate_range("ac3_dynrng3",  ac3_dynrng3, 0, 255);
            validate_range("ac3_dynrng4",  ac3_dynrng4, 0, 255);
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RDD6DolbyDigitalComplete::Construct8BitPayload(RDD6DataSegment *segment) const
{
    if (segment->id == RDD6DataSegment::DS_NONE) {
        segment->id = RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE;
    } else if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE));
    }

    segment->PrepareConstruct8BitPayload();

    PutBitBuffer buffer(&segment->payload_buffer);
    buffer.PutBits( 5, program_id);
    buffer.PutBits( 5, reserved_1);
    buffer.PutBits( 3, ac3_bsmod);
    buffer.PutBits( 3, ac3_acmod);
    buffer.PutBits( 2, ac3_cmixlev);
    buffer.PutBits( 2, ac3_surmixlev);
    buffer.PutBits( 2, ac3_dsurmod);
    buffer.PutBits( 1, ac3_lfeon);
    buffer.PutBits( 5, ac3_dialnorm);
    buffer.PutBits( 1, ac3_langcode);
    buffer.PutBits( 8, ac3_langcod);
    buffer.PutBits( 1, ac3_audprodie);
    buffer.PutBits( 5, ac3_mixlevel);
    buffer.PutBits( 2, ac3_roomtyp);
    buffer.PutBits( 1, ac3_copyrightb);
    buffer.PutBits( 1, ac3_origbs);
    buffer.PutBits( 1, ac3_timecod1e);
    buffer.PutBits(14, ac3_timecod1);
    buffer.PutBits( 1, ac3_timecod2e);
    buffer.PutBits(14, ac3_timecod2);
    buffer.PutBits( 1, ac3_hpfon);
    buffer.PutBits( 1, ac3_bwlpfon);
    buffer.PutBits( 1, ac3_lfelpfon);
    buffer.PutBits( 1, ac3_sur90on);
    buffer.PutBits( 1, ac3_suratton);
    buffer.PutBits( 1, ac3_rfpremphon);
    buffer.PutBits( 1, ac3_compre);
    buffer.PutBits( 8, ac3_compr1);
    buffer.PutBits( 1, ac3_dynrnge);
    buffer.PutBits( 8, ac3_dynrng1);
    buffer.PutBits( 8, ac3_dynrng2);
    buffer.PutBits( 8, ac3_dynrng3);
    buffer.PutBits( 8, ac3_dynrng4);
    buffer.PutBits( 1, reserved_2);

    segment->CompleteConstruct8BitPayload();
}

void RDD6DolbyDigitalComplete::Parse8BitPayload(const RDD6DataSegment *segment)
{
    if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_COMPLETE));
    }

    RDD6GetBitBuffer buffer(segment->payload, segment->size, 0, 0);

    if (buffer.GetRemBitSize() < 126)
        BMX_EXCEPTION(("Insufficient payload data in RDD6 data segment with id %u", segment->id));

    buffer.GetBits( 5, &program_id);
    buffer.GetBits( 5, &reserved_1);
    buffer.GetBits( 3, &ac3_bsmod);
    buffer.GetBits( 3, &ac3_acmod);
    buffer.GetBits( 2, &ac3_cmixlev);
    buffer.GetBits( 2, &ac3_surmixlev);
    buffer.GetBits( 2, &ac3_dsurmod);
    buffer.GetBits( 1, &ac3_lfeon);
    buffer.GetBits( 5, &ac3_dialnorm);
    buffer.GetBits( 1, &ac3_langcode);
    buffer.GetBits( 8, &ac3_langcod);
    buffer.GetBits( 1, &ac3_audprodie);
    buffer.GetBits( 5, &ac3_mixlevel);
    buffer.GetBits( 2, &ac3_roomtyp);
    buffer.GetBits( 1, &ac3_copyrightb);
    buffer.GetBits( 1, &ac3_origbs);
    buffer.GetBits( 1, &ac3_timecod1e);
    buffer.GetBits(14, &ac3_timecod1);
    buffer.GetBits( 1, &ac3_timecod2e);
    buffer.GetBits(14, &ac3_timecod2);
    buffer.GetBits( 1, &ac3_hpfon);
    buffer.GetBits( 1, &ac3_bwlpfon);
    buffer.GetBits( 1, &ac3_lfelpfon);
    buffer.GetBits( 1, &ac3_sur90on);
    buffer.GetBits( 1, &ac3_suratton);
    buffer.GetBits( 1, &ac3_rfpremphon);
    buffer.GetBits( 1, &ac3_compre);
    buffer.GetBits( 8, &ac3_compr1);
    buffer.GetBits( 1, &ac3_dynrnge);
    buffer.GetBits( 8, &ac3_dynrng1);
    buffer.GetBits( 8, &ac3_dynrng2);
    buffer.GetBits( 8, &ac3_dynrng3);
    buffer.GetBits( 8, &ac3_dynrng4);
    buffer.GetBits( 1, &reserved_2);
}

void RDD6DolbyDigitalComplete::UnparseXML(XMLWriter *writer) const
{
    writer->WriteElementStart(RDD6_NAMESPACE, "dolby_digital_complete");

    writer->WriteElement(RDD6_NAMESPACE, "program_id", unparse_xml_uint8(program_id));
    if (ac3_acmod <= 1) {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_1_ENUM, ac3_bsmod));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_2_ENUM, ac3_bsmod));
    }
    writer->WriteElement(RDD6_NAMESPACE, "ac_mode",
                         unparse_xml_enum("ac_mode", ACMOD_ENUM, ac3_acmod));
    writer->WriteElement(RDD6_NAMESPACE, "center_mix_level",
                         unparse_xml_enum("center_mix_level", CMIXLEVEL_ENUM, ac3_cmixlev));
    writer->WriteElement(RDD6_NAMESPACE, "sur_mix_level",
                         unparse_xml_enum("sur_mix_level", SURMIXLEVEL_ENUM, ac3_surmixlev));
    if (ac3_dsurmod == 1 || ac3_dsurmod == 2)
        writer->WriteElement(RDD6_NAMESPACE, "sur_encoded", unparse_xml_bool(ac3_dsurmod == 2));
    writer->WriteElement(RDD6_NAMESPACE, "lfe_on", unparse_xml_bool(ac3_lfeon));
    if (ac3_dialnorm == 0)
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(31)); // 0 is treated as 31
    else
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(ac3_dialnorm));
    if (ac3_langcode)
        writer->WriteElement(RDD6_NAMESPACE, "lang_code", unparse_xml_hex_uint8(ac3_langcod));
    if (ac3_audprodie) {
        writer->WriteElement(RDD6_NAMESPACE, "mix_level", unparse_xml_uint8(ac3_mixlevel));
        if (ac3_roomtyp == 1 || ac3_roomtyp == 2) {
            writer->WriteElement(RDD6_NAMESPACE, "room_type",
                                 unparse_xml_enum("room_type", ROOM_TYPE_ENUM, ac3_roomtyp));
        }
    }
    writer->WriteElement(RDD6_NAMESPACE, "copyright", unparse_xml_bool(ac3_copyrightb));
    writer->WriteElement(RDD6_NAMESPACE, "orig_bs", unparse_xml_bool(ac3_origbs));
    if (ac3_timecod1e || ac3_timecod2e) {
        writer->WriteElement(RDD6_NAMESPACE, "timecode", unparse_xml_timecode(ac3_timecod1e, ac3_timecod2e,
                                                                              ac3_timecod1, ac3_timecod2));
    }
    writer->WriteElement(RDD6_NAMESPACE, "hp_filter", unparse_xml_bool(ac3_hpfon));
    writer->WriteElement(RDD6_NAMESPACE, "bw_lp_filter", unparse_xml_bool(ac3_bwlpfon));
    writer->WriteElement(RDD6_NAMESPACE, "lfe_lp_filter", unparse_xml_bool(ac3_lfelpfon));
    writer->WriteElement(RDD6_NAMESPACE, "sur_90_filter", unparse_xml_bool(ac3_sur90on));
    writer->WriteElement(RDD6_NAMESPACE, "sur_att_filter", unparse_xml_bool(ac3_suratton));
    writer->WriteElement(RDD6_NAMESPACE, "rf_preemph_filter", unparse_xml_bool(ac3_rfpremphon));
    if (ac3_compre == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "comp_pf_1",
                             unparse_xml_enum("compr_pf_1", COMPR_ENUM, ac3_compr1));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "compr_wd_1", unparse_xml_hex_uint8(ac3_compr1));
    }
    if (ac3_dynrnge == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_1",
                             unparse_xml_enum("dyn_range_pf_1", DYNRNG_ENUM, ac3_dynrng1));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_2",
                             unparse_xml_enum("dyn_range_pf_2", DYNRNG_ENUM, ac3_dynrng2));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_3",
                             unparse_xml_enum("dyn_range_pf_3", DYNRNG_ENUM, ac3_dynrng3));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_4",
                             unparse_xml_enum("dyn_range_pf_4", DYNRNG_ENUM, ac3_dynrng4));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_1", unparse_xml_hex_uint8(ac3_dynrng1));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_2", unparse_xml_hex_uint8(ac3_dynrng2));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_3", unparse_xml_hex_uint8(ac3_dynrng3));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_4", unparse_xml_hex_uint8(ac3_dynrng4));
    }

    writer->WriteElementEnd();
}



RDD6DolbyDigitalEssential::RDD6DolbyDigitalEssential()
{
    program_id = 0;
    ac3_datarate = 0;
    ac3_bsmod = 0;
    ac3_acmod = 1;
    ac3_lfeon = 0;
    ac3_dialnorm = 0;
    ac3_compre = 0;
    ac3_compr2 = 0;
    ac3_dynrnge = 0;
    ac3_dynrng5 = 0;
    ac3_dynrng6 = 0;
    ac3_dynrng7 = 0;
    ac3_dynrng8 = 0;
}

bool RDD6DolbyDigitalEssential::Validate()
{
    try
    {
        validate_range("program_id",       program_id, 0, 7);
        if (ac3_datarate != 31)
            validate_range("ac3_datarate", ac3_datarate, 0, 18);
        validate_range("ac3_bsmod",        ac3_bsmod, 0, 7);
        validate_range("ac3_acmod",        ac3_acmod, 1, 7);
        validate_range("ac3_lfeon",        ac3_lfeon, 0, 1);
        validate_range("ac3_dialnorm",     ac3_dialnorm, 0, 31);
        validate_range("ac3_compre",       ac3_compre, 0, 1);
        if (ac3_compre == 0)
            validate_range("ac3_compr2",   ac3_compr2, 0, 5);
        else
            validate_range("ac3_compr2",   ac3_compr2, 0, 255);
        validate_range("ac3_dynrnge",      ac3_dynrnge, 0, 1);
        if (ac3_dynrnge == 0) {
            validate_range("ac3_dynrng5",  ac3_dynrng5, 0, 5);
            validate_range("ac3_dynrng6",  ac3_dynrng6, 0, 5);
            validate_range("ac3_dynrng7",  ac3_dynrng7, 0, 5);
            validate_range("ac3_dynrng8",  ac3_dynrng8, 0, 5);
        } else {
            validate_range("ac3_dynrng5",  ac3_dynrng5, 0, 255);
            validate_range("ac3_dynrng6",  ac3_dynrng6, 0, 255);
            validate_range("ac3_dynrng7",  ac3_dynrng7, 0, 255);
            validate_range("ac3_dynrng8",  ac3_dynrng8, 0, 255);
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RDD6DolbyDigitalEssential::Construct8BitPayload(RDD6DataSegment *segment) const
{
    if (segment->id == RDD6DataSegment::DS_NONE) {
        segment->id = RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL;
    } else if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL));
    }

    segment->PrepareConstruct8BitPayload();

    PutBitBuffer buffer(&segment->payload_buffer);
    buffer.PutBits(5, program_id);
    buffer.PutBits(5, ac3_datarate);
    buffer.PutBits(3, ac3_bsmod);
    buffer.PutBits(3, ac3_acmod);
    buffer.PutBits(1, ac3_lfeon);
    buffer.PutBits(5, ac3_dialnorm);
    buffer.PutBits(1, ac3_compre);
    buffer.PutBits(8, ac3_compr2);
    buffer.PutBits(1, ac3_dynrnge);
    buffer.PutBits(8, ac3_dynrng5);
    buffer.PutBits(8, ac3_dynrng6);
    buffer.PutBits(8, ac3_dynrng7);
    buffer.PutBits(8, ac3_dynrng8);

    segment->CompleteConstruct8BitPayload();
}

void RDD6DolbyDigitalEssential::Parse8BitPayload(const RDD6DataSegment *segment)
{
    if (segment->id != RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL) {
        BMX_EXCEPTION(("RDD6 data segment id %u does not match requested data id %u",
                       segment->id, RDD6DataSegment::DS_DOLBY_DIGITAL_ESSENTIAL));
    }

    RDD6GetBitBuffer buffer(segment->payload, segment->size, 0, 0);

    if (buffer.GetRemBitSize() < 64)
        BMX_EXCEPTION(("Insufficient payload data in RDD6 data segment with id %u", segment->id));

    buffer.GetBits(5, &program_id);
    buffer.GetBits(5, &ac3_datarate);
    buffer.GetBits(3, &ac3_bsmod);
    buffer.GetBits(3, &ac3_acmod);
    buffer.GetBits(1, &ac3_lfeon);
    buffer.GetBits(5, &ac3_dialnorm);
    buffer.GetBits(1, &ac3_compre);
    buffer.GetBits(8, &ac3_compr2);
    buffer.GetBits(1, &ac3_dynrnge);
    buffer.GetBits(8, &ac3_dynrng5);
    buffer.GetBits(8, &ac3_dynrng6);
    buffer.GetBits(8, &ac3_dynrng7);
    buffer.GetBits(8, &ac3_dynrng8);
}

void RDD6DolbyDigitalEssential::UnparseXML(XMLWriter *writer) const
{
    writer->WriteElementStart(RDD6_NAMESPACE, "dolby_digital_complete");

    writer->WriteElement(RDD6_NAMESPACE, "program_id", unparse_xml_uint8(program_id));
    if (ac3_datarate != 31) { // 0x31 = not specified
        writer->WriteElement(RDD6_NAMESPACE, "data_rate",
                             unparse_xml_enum("data_rate", DATARATE_ENUM, ac3_datarate));
    }
    if (ac3_acmod <= 1) {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_1_ENUM, ac3_bsmod));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "bs_mode",
                             unparse_xml_enum("bs_mode", BSMOD_2_ENUM, ac3_bsmod));
    }
    writer->WriteElement(RDD6_NAMESPACE, "ac_mode",
                         unparse_xml_enum("ac_mode", ACMOD_ENUM, ac3_acmod));
    writer->WriteElement(RDD6_NAMESPACE, "lfe_on", unparse_xml_bool(ac3_lfeon));
    if (ac3_dialnorm == 0)
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(31)); // 0 is treated as 31
    else
        writer->WriteElement(RDD6_NAMESPACE, "dialnorm", unparse_xml_uint8(ac3_dialnorm));
    if (ac3_compre == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "comp_pf_2",
                             unparse_xml_enum("compr_pf_2", COMPR_ENUM, ac3_compr2));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "compr_wd_2", unparse_xml_hex_uint8(ac3_compr2));
    }
    if (ac3_dynrnge == 0) {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_5",
                             unparse_xml_enum("dyn_range_pf_5", DYNRNG_ENUM, ac3_dynrng5));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_6",
                             unparse_xml_enum("dyn_range_pf_6", DYNRNG_ENUM, ac3_dynrng6));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_7",
                             unparse_xml_enum("dyn_range_pf_7", DYNRNG_ENUM, ac3_dynrng7));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_pf_8",
                             unparse_xml_enum("dyn_range_pf_8", DYNRNG_ENUM, ac3_dynrng8));
    } else {
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_5", unparse_xml_hex_uint8(ac3_dynrng5));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_6", unparse_xml_hex_uint8(ac3_dynrng6));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_7", unparse_xml_hex_uint8(ac3_dynrng7));
        writer->WriteElement(RDD6_NAMESPACE, "dyn_range_wd_8", unparse_xml_hex_uint8(ac3_dynrng8));
    }

    writer->WriteElementEnd();
}



RDD6DataSegment::RDD6DataSegment()
{
    id = 0;
    size = 0;
    payload = 0;
    checksum = 0;
}

void RDD6DataSegment::Construct8Bit(PutBitBuffer *buffer)
{
    BMX_CHECK(size > 0 && size <= 256);

    buffer->PutBits(8, id);
    if (size < 256)
        buffer->PutBits(8, size);
    else
        buffer->PutBits(8, (uint8_t)0);
    buffer->PutBytes(payload, size);
    buffer->PutBits(8, checksum);
}

void RDD6DataSegment::Parse8Bit(RDD6GetBitBuffer *buffer)
{
    // id has been read and set

    uint32_t rem_size = buffer->GetRemSize();
    if (rem_size < 2) // size + checksum bytes
        BMX_EXCEPTION(("RDD6 data segment size (%u) too small", rem_size));
    uint8_t u8_size;
    buffer->GetUInt8(&u8_size);
    size = u8_size;
    if (size == 0)
        size = 256;
    if (size > rem_size - 1)
        BMX_EXCEPTION(("RDD6 data segment payload size %u exceeds available size %u", size, rem_size - 1));

    payload = 0;
    payload_buffer.Clear();
    if (size > 0) {
        const unsigned char *data_a, *data_b;
        uint32_t size_a, size_b;
        buffer->GetBytes(size, &data_a, &size_a, &data_b, &size_b);
        if (data_a && data_b) {
            payload_buffer.Grow(size_a + size_b);
            payload_buffer.Append(data_a, size_a);
            payload_buffer.Append(data_b, size_b);
            payload = payload_buffer.GetBytes();
        } else if (data_a) {
            payload = data_a;
        } else if (data_b) {
            payload = data_b;
        }
    }

    buffer->GetUInt8(&checksum);

    uint8_t calc_checksum = CalcChecksum();
    if (checksum != calc_checksum)
        log_warn("RDD6 data segment checksum 0x%02x differs from calculated checksum 0x%02x\n", checksum, calc_checksum);
}

void RDD6DataSegment::Construct8BitPayload(const RDD6ParsedPayload *data)
{
    data->Construct8BitPayload(this);
}

void RDD6DataSegment::Parse8BitPayload(RDD6ParsedPayload *data) const
{
    data->Parse8BitPayload(this);
}

void RDD6DataSegment::UnparseXML(XMLWriter *writer) const
{
    auto_ptr<RDD6ParsedPayload> data;
    switch (id)
    {
        case DS_NONE:                            return;
        case DS_DOLBY_E_COMPLETE:                data.reset(new RDD6DolbyEComplete); break;
        case DS_DOLBY_E_ESSENTIAL:               data.reset(new RDD6DolbyEEssential); break;
        case DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI:  data.reset(new RDD6DolbyDigitalCompleteExtBSI); break;
        case DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI: data.reset(new RDD6DolbyDigitalEssentialExtBSI); break;
        case DS_DOLBY_DIGITAL_COMPLETE:          data.reset(new RDD6DolbyDigitalComplete); break;
        case DS_DOLBY_DIGITAL_ESSENTIAL:         data.reset(new RDD6DolbyDigitalEssential); break;
        default:                                 break;
    }
    if (data.get()) {
        Parse8BitPayload(data.get());
        data->UnparseXML(writer);
    } else {
        writer->WriteElementStart(RDD6_NAMESPACE, "segment");
        writer->WriteAttribute(RDD6_NAMESPACE, "id", unparse_xml_uint8(id));
        writer->WriteAttribute(RDD6_NAMESPACE, "size", unparse_xml_uint16(size));
        writer->WriteElementContent(unparse_xml_bytes(payload, size));
        writer->WriteElementEnd();
    }
}

uint8_t RDD6DataSegment::CalcChecksum()
{
    BMX_CHECK(size <= 256);
    uint8_t result = (size > 255 ? 0 : (uint8_t)size); // input data segment size byte 0 is interpreted as 256
    uint8_t i;
    for (i = 0; i < size; i++)
        result += payload[i];
    result = (~result) + 1;

    return result;
}

uint8_t RDD6DataSegment::GetProgramId() const
{
    uint8_t program_id = UINT8_MAX;

    if (size >= 5 &&
            (id == DS_DOLBY_DIGITAL_COMPLETE_EXT_BSI ||
             id == DS_DOLBY_DIGITAL_ESSENTIAL_EXT_BSI ||
             id == DS_DOLBY_DIGITAL_COMPLETE ||
             id == DS_DOLBY_DIGITAL_ESSENTIAL))
    {
        RDD6GetBitBuffer buffer(payload, size, 0, 0);
        buffer.GetBits(5, &program_id);
    }

    return program_id;
}

void RDD6DataSegment::PrepareConstruct8BitPayload()
{
    payload_buffer.Clear();

    payload  = 0;
    size     = 0;
    checksum = 0;
}

void RDD6DataSegment::CompleteConstruct8BitPayload()
{
    BMX_ASSERT(payload_buffer.GetSize() > 0 && payload_buffer.GetSize() <= 256);

    payload  = payload_buffer.GetBytes();
    size     = payload_buffer.GetSize();
    checksum = CalcChecksum();
}



RDD6MetadataSubFrame::RDD6MetadataSubFrame()
{
    memset(&sync_segment, 0, sizeof(sync_segment));
}

RDD6MetadataSubFrame::~RDD6MetadataSubFrame()
{
    ClearDataSegments();
}

void RDD6MetadataSubFrame::Construct8Bit(PutBitBuffer *buffer)
{
    buffer->PutBits(16, sync_segment.start_subframe_sync_word);
    buffer->PutBits( 4, sync_segment.revision_id);
    buffer->PutBits( 8, sync_segment.originator_id);
    buffer->PutBits(16, sync_segment.originator_address);
    buffer->PutBits(16, sync_segment.frame_count);
    buffer->PutBits(12, sync_segment.reserved);

    size_t i;
    for (i = 0; i < data_segments.size(); i++)
        data_segments[i]->Construct8Bit(buffer);
    buffer->PutBits(8, (uint8_t)0);
}

void RDD6MetadataSubFrame::Parse8Bit(RDD6GetBitBuffer *buffer)
{
    // sync_segment.start_subframe_sync_word has been read and set

    ClearDataSegments();

    if (buffer->GetRemSize() < 7)
        BMX_EXCEPTION(("Data size (%u) too small for RDD6 metadata sub-frame sync segment", buffer->GetRemSize()));
    buffer->GetBits( 4, &sync_segment.revision_id);
    buffer->GetBits( 8, &sync_segment.originator_id);
    buffer->GetBits(16, &sync_segment.originator_address);
    buffer->GetBits(16, &sync_segment.frame_count);
    buffer->GetBits(12, &sync_segment.reserved);

    uint8_t id;
    while (true) {
        if (!buffer->GetUInt8(&id))
            BMX_EXCEPTION(("Missing data segment identifier byte in RDD6 metadata sub-frame"));
        if (id == 0)
            break;

        auto_ptr<RDD6DataSegment> data_segment(new RDD6DataSegment());
        data_segment->id = id;
        data_segment->Parse8Bit(buffer);
        data_segments.push_back(data_segment.release());
    }
}

void RDD6MetadataSubFrame::UnparseXML(XMLWriter *writer) const
{
    writer->WriteElementStart(RDD6_NAMESPACE, "subframe");

    writer->WriteElementStart(RDD6_NAMESPACE, "sync");
    writer->WriteElement(RDD6_NAMESPACE, "rev_id", unparse_xml_hex_uint8(sync_segment.revision_id));
    writer->WriteElementStart(RDD6_NAMESPACE, "orig_id");
    if (sync_segment.originator_id >= 1 && sync_segment.originator_id <= 32)
        writer->WriteAttribute(RDD6_NAMESPACE, "manufacturer", "Dolby Laboratories");
    else if (sync_segment.originator_id >= 33 && sync_segment.originator_id <= 42)
        writer->WriteAttribute(RDD6_NAMESPACE, "manufacturer", "Harris Broadcast Communications (Leitch)");
    writer->WriteElementContent(unparse_xml_hex_uint8(sync_segment.originator_id));
    writer->WriteElementEnd();
    if (sync_segment.originator_id != 0) {
        writer->WriteElement(RDD6_NAMESPACE, "orig_addr",
                             unparse_xml_hex_uint16(sync_segment.originator_address));
    }
    writer->WriteElement(RDD6_NAMESPACE, "frame_count", unparse_xml_uint16(sync_segment.frame_count));
    writer->WriteElementEnd();

    size_t i;
    for (i = 0; i < data_segments.size(); i++)
        data_segments[i]->UnparseXML(writer);

    writer->WriteElementEnd();
}

void RDD6MetadataSubFrame::GetDataSegments(uint8_t id, vector<RDD6DataSegment*> *segments) const
{
    size_t i;
    for (i = 0; i < data_segments.size(); i++) {
        if (data_segments[i]->id == id)
            segments->push_back(data_segments[i]);
    }
}

RDD6DataSegment* RDD6MetadataSubFrame::GetProgramDataSegment(uint8_t id, uint8_t program_id) const
{
    size_t i;
    for (i = 0; i < data_segments.size(); i++) {
        if (data_segments[i]->id == id && data_segments[i]->GetProgramId() == program_id)
            return data_segments[i];
    }

    return 0;
}

void RDD6MetadataSubFrame::ClearDataSegments()
{
    size_t i;
    for (i = 0; i < data_segments.size(); i++)
        delete data_segments[i];
    data_segments.clear();
}



RDD6MetadataFrame::RDD6MetadataFrame()
{
    first_sub_frame = 0;
    second_sub_frame = 0;
    end_frame_sync_word = 0;
}

RDD6MetadataFrame::~RDD6MetadataFrame()
{
    delete first_sub_frame;
    delete second_sub_frame;
}

void RDD6MetadataFrame::Construct8Bit(ByteArray *data, uint32_t *first_end_offset)
{
    PutBitBuffer buffer(data);
    Construct8Bit(&buffer, first_end_offset);
}

void RDD6MetadataFrame::Construct8Bit(PutBitBuffer *buffer, uint32_t *first_end_offset)
{
    if (first_sub_frame)
        first_sub_frame->Construct8Bit(buffer);
    *first_end_offset = buffer->GetSize();

    if (second_sub_frame) {
        second_sub_frame->Construct8Bit(buffer);
        buffer->PutBits(16, (uint16_t)END_FRAME_SYNC_WORD);
    }
}

void RDD6MetadataFrame::Parse8Bit(const unsigned char *data_a, uint32_t size_a,
                                  const unsigned char *data_b, uint32_t size_b)
{
    RDD6GetBitBuffer buffer(data_a, size_a, data_b, size_b);
    Parse8Bit(&buffer);
}

void RDD6MetadataFrame::Parse8Bit(RDD6GetBitBuffer *buffer)
{
    bool read_first = false, read_second = false;
    uint8_t byte;
    uint16_t sync_word = 0;
    while (buffer->GetUInt8(&byte)) {
        sync_word = (sync_word << 8) | byte;

        if (sync_word == RDD6MetadataSubFrame::FIRST_SUBFRAME_SYNC_WORD) {
            delete first_sub_frame;
            first_sub_frame = 0;
            delete second_sub_frame;
            second_sub_frame = 0;

            first_sub_frame = new RDD6MetadataSubFrame();
            first_sub_frame->sync_segment.start_subframe_sync_word = RDD6MetadataSubFrame::FIRST_SUBFRAME_SYNC_WORD;
            first_sub_frame->Parse8Bit(buffer);
            read_first = true;

            sync_word = 0;
        } else if (sync_word == RDD6MetadataSubFrame::SECOND_SUBFRAME_SYNC_WORD) {
            delete second_sub_frame;
            second_sub_frame = 0;

            second_sub_frame = new RDD6MetadataSubFrame();
            second_sub_frame->sync_segment.start_subframe_sync_word = RDD6MetadataSubFrame::SECOND_SUBFRAME_SYNC_WORD;
            second_sub_frame->Parse8Bit(buffer);
            read_second = true;

            sync_word = 0;
        } else if (sync_word == END_FRAME_SYNC_WORD) {
            break;
        }
    }

    if (sync_word != END_FRAME_SYNC_WORD)
        log_warn("Missing RDD6 metadata end frame sync word\n");
    if (!read_first && !read_second)
        log_warn("Failed to find first or second RDD6 metadata subframe\n");
}

bool RDD6MetadataFrame::UnparseXML(XMLWriter *writer) const
{
    try
    {
        writer->WriteDocumentStart();
        writer->WriteElementStart(RDD6_NAMESPACE, "rdd6");
        writer->DeclareNamespace(RDD6_NAMESPACE, "");

        if (first_sub_frame)
            first_sub_frame->UnparseXML(writer);
        if (second_sub_frame)
            second_sub_frame->UnparseXML(writer);

        writer->WriteElementEnd();
        writer->WriteDocumentEnd();

        writer->Flush();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RDD6MetadataFrame::UnparseXML(const string &filename) const
{
    XMLWriter *writer = XMLWriter::Open(filename);
    if (!writer)
        return false;

    bool result = UnparseXML(writer);
    delete writer;

    return result;
}

vector<RDD6DataSegment*> RDD6MetadataFrame::GetDataSegments(uint8_t id) const
{
    vector<RDD6DataSegment*> segments;
    if (first_sub_frame)
        first_sub_frame->GetDataSegments(id, &segments);
    if (second_sub_frame)
        second_sub_frame->GetDataSegments(id, &segments);

    return segments;
}

RDD6DataSegment* RDD6MetadataFrame::GetProgramDataSegment(uint8_t id, uint8_t program_id) const
{
    RDD6DataSegment* segment = 0;
    if (first_sub_frame)
        segment = first_sub_frame->GetProgramDataSegment(id, program_id);
    if (!segment && second_sub_frame)
        segment = second_sub_frame->GetProgramDataSegment(id, program_id);

    return segment;
}

