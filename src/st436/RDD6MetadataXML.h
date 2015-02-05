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

#ifndef BMX_RDD6_METADATA_XML_H_
#define BMX_RDD6_METADATA_XML_H_

#include <string>

#include <bmx/BMXTypes.h>



namespace bmx
{


typedef struct
{
    const char *name;
    int value;
} XMLEnumInfo;


extern const char RDD6_NAMESPACE[];

extern const XMLEnumInfo PROGRAM_CONFIG_ENUM[];
extern const XMLEnumInfo FRAME_RATE_ENUM[];
extern const XMLEnumInfo DATARATE_ENUM[];
extern const XMLEnumInfo BSMOD_1_ENUM[];
extern const XMLEnumInfo BSMOD_2_ENUM[];
extern const XMLEnumInfo ACMOD_ENUM[];
extern const XMLEnumInfo CMIXLEVEL_ENUM[];
extern const XMLEnumInfo SURMIXLEVEL_ENUM[];
extern const XMLEnumInfo ROOM_TYPE_ENUM[];
extern const XMLEnumInfo DMIXMOD_ENUM[];
extern const XMLEnumInfo LTRTCMIXLEVEL_ENUM[];
extern const XMLEnumInfo LTRTSURMIXLEVEL_ENUM[];
extern const XMLEnumInfo ADCONVTYPE_ENUM[];
extern const XMLEnumInfo COMPR_ENUM[];


#define LOROCMIXLEVEL_ENUM      LTRTCMIXLEVEL_ENUM
#define LOROSURMIXLEVEL_ENUM    LTRTSURMIXLEVEL_ENUM
#define DYNRNG_ENUM             COMPR_ENUM


uint8_t parse_xml_uint8(const std::string &context, const std::string &str);
int8_t parse_xml_int8(const std::string &context, const std::string &str);
uint16_t parse_xml_uint16(const std::string &context, const std::string &str);
int16_t parse_xml_int16(const std::string &context, const std::string &str);
int parse_xml_int(const std::string &context, const std::string &str);
uint8_t parse_xml_hex_uint8(const std::string &context, const std::string &str);
uint16_t parse_xml_hex_uint16(const std::string &context, const std::string &str);
bool parse_xml_bool(const std::string &context, const std::string &str);
uint64_t parse_xml_smpte_timecode(const std::string &context, const std::string &str);
void parse_xml_timecode(const std::string &context, const std::string &str,
                        uint8_t *timecode_1_e, uint8_t *timecode_2_e,
                        uint16_t *timecode_1, uint16_t *timecode_2);
int parse_xml_enum(const std::string &context, const XMLEnumInfo *info, const std::string &str);
size_t parse_xml_bytes(const std::string &context, const std::string &str, unsigned char *bytes, size_t size);

std::string unparse_xml_uint8(uint8_t value);
std::string unparse_xml_int8(int8_t value);
std::string unparse_xml_uint16(uint16_t value);
std::string unparse_xml_int16(int16_t value);
std::string unparse_xml_int(int value);
std::string unparse_xml_hex_uint8(uint8_t value);
std::string unparse_xml_hex_uint16(uint16_t value);
std::string unparse_xml_bool(bool value);
std::string unparse_xml_smpte_timecode(uint64_t value);
std::string unparse_xml_timecode(uint8_t timecode_1_e, uint8_t timecode_2_e,
                                 uint16_t timecode_1, uint16_t timecode_2);
std::string unparse_xml_enum(const std::string &context, const XMLEnumInfo *info, int value);
std::string unparse_xml_bytes(const unsigned char *bytes, size_t size);

};



#endif

