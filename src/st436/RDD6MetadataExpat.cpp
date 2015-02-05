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

#define __STDC_FORMAT_MACROS

#include <cstring>
#include <cstdio>
#include <cerrno>

#include <exception>

#include "RDD6MetadataExpat.h"
#include "RDD6MetadataXML.h"
#include <bmx/Utils.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;


#define START_EXPAT_CALLBACK                            \
    try                                                 \
    {

#define END_EXPAT_CALLBACK                              \
    }                                                   \
    catch (const BMXException &ex)                      \
    {                                                   \
        log_error("%s\n", ex.what());                   \
        XML_StopParser(mParser, XML_FALSE);             \
    }                                                   \
    catch (const std::exception &ex)                    \
    {                                                   \
        log_error("Exception: %s\n", ex.what());        \
        XML_StopParser(mParser, XML_FALSE);             \
    }                                                   \
    catch (...)                                         \
    {                                                   \
        log_error("Unknown exception thrown\n");        \
        XML_StopParser(mParser, XML_FALSE);             \
    }


static const char NAMESPACE_SEPARATOR = ' ';



static void split_qname(const char *qname, string *ns, string *name)
{
    const char *ptr = qname;
    while (*ptr && *ptr != NAMESPACE_SEPARATOR)
        ptr++;

    if (!(*ptr)) {
        ns->clear();
        name->assign(qname);
    } else {
        ns->assign(qname, (size_t)(ptr - qname));
        name->assign(ptr + 1);
    }
}

static void expat_StartElement(void *user_data, const char *qname, const char **atts)
{
    string ns, name;
    split_qname(qname, &ns, &name);
    reinterpret_cast<RDD6MetadataExpat*>(user_data)->StartElement(ns, name, atts);
}

static void expat_EndElement(void *user_data, const char *qname)
{
    string ns, name;
    split_qname(qname, &ns, &name);
    reinterpret_cast<RDD6MetadataExpat*>(user_data)->EndElement(ns, name);
}

static void expat_CharacterData(void *user_data, const char *s, int len)
{
    reinterpret_cast<RDD6MetadataExpat*>(user_data)->CharacterData(s, len);
}



ExpatHandler::ExpatHandler()
{
}

ExpatHandler::~ExpatHandler()
{
}

bool ExpatHandler::ElementNSEquals(const string &ns, const string &left, const string &right) const
{
    return ns == mNamespace && left == right;
}

bool ExpatHandler::HaveAttribute(const string &name, const char **atts)
{
    const char **atts_ptr = atts;
    while (atts_ptr && atts_ptr[0] && atts_ptr[1]) {
        const char *att_name = *atts_ptr++;
        atts_ptr++; // value
        if (name == att_name)
            return true;
    }

    return false;
}

string ExpatHandler::GetAttribute(const string &name, const char **atts)
{
    const char **atts_ptr = atts;
    while (atts_ptr && atts_ptr[0] && atts_ptr[1]) {
        const char *att_name = *atts_ptr++;
        const char *att_value = *atts_ptr++;
        if (name == att_name)
            return att_value;
    }

    throw BMXException("Missing attribute '%s'", name.c_str());
}



IgnoreExpatHandler::IgnoreExpatHandler()
: ExpatHandler()
{
    mLevel = 0;
}

IgnoreExpatHandler::~IgnoreExpatHandler()
{
}

void IgnoreExpatHandler::StartElement(const string &ns, const string &name, const char **atts)
{
    (void)ns;
    (void)name;
    (void)atts;

    mLevel++;
}

bool IgnoreExpatHandler::EndElement(const string &ns, const string &name)
{
    (void)ns;
    (void)name;

    mLevel--;
    return mLevel <= 0;
}

void IgnoreExpatHandler::CharacterData(const char *s, int len)
{
    (void)s;
    (void)len;
}



SimpleTextExpatHandler::SimpleTextExpatHandler()
: ExpatHandler()
{
}

SimpleTextExpatHandler::~SimpleTextExpatHandler()
{
}

void SimpleTextExpatHandler::StartElement(const string &ns, const string &name, const char **atts)
{
    (void)atts;

    if (mName.empty()) {
        mNamespace = ns;
        mName = name;
    } else {
        throw BMXException("Unexpected XML child element '%s' in simple element '%s'", name.c_str(), mName.c_str());
    }
}

bool SimpleTextExpatHandler::EndElement(const string &ns, const string &name)
{
    (void)ns;
    (void)name;

    return true;
}

void SimpleTextExpatHandler::CharacterData(const char *s, int len)
{
    mCData.append(s, len);
}



SimpleChildrenExpatHandler::SimpleChildrenExpatHandler()
: ExpatHandler()
{
    mChildHandler = 0;
}

SimpleChildrenExpatHandler::~SimpleChildrenExpatHandler()
{
    delete mChildHandler;
}

void SimpleChildrenExpatHandler::StartElement(const string &ns, const string &name, const char **atts)
{
    if (mName.empty()) {
        mNamespace = ns;
        mName = name;
    } else if (!mChildHandler) {
        if (ns == mNamespace) {
            mChildHandler = new SimpleTextExpatHandler();
        } else {
            log_warn("Ignoring XML element '%s %s'\n", ns.c_str(), name.c_str());
            mChildHandler = new IgnoreExpatHandler();
        }
    }

    if (mChildHandler)
        mChildHandler->StartElement(ns, name, atts);
}

bool SimpleChildrenExpatHandler::EndElement(const string &ns, const string &name)
{
    if (mChildHandler) {
        if (mChildHandler->EndElement(ns, name)) {
            SimpleTextExpatHandler *text_handler = dynamic_cast<SimpleTextExpatHandler*>(mChildHandler);
            if (text_handler)
                mChildren[text_handler->GetName()] = text_handler->GetCData();
            delete mChildHandler;
            mChildHandler = 0;
        }
        return false;
    } else {
        return true;
    }
}

void SimpleChildrenExpatHandler::CharacterData(const char *s, int len)
{
    if (mChildHandler)
        mChildHandler->CharacterData(s, len);
}

bool SimpleChildrenExpatHandler::HaveChild(const string &name) const
{
    return mChildren.find(name) != mChildren.end();
}

const string& SimpleChildrenExpatHandler::GetChildStr(const string &name) const
{
    map<string, string>::const_iterator res = mChildren.find(name);
    if (res == mChildren.end())
        throw BMXException("Missing '%s' in element '%s'", name.c_str(), mName.c_str());

    return res->second;
}



DataSegmentHandler::DataSegmentHandler()
{
    mDataSegment = new RDD6DataSegment();
}

DataSegmentHandler::~DataSegmentHandler()
{
    delete mDataSegment;
}

void DataSegmentHandler::CompletePayload(RDD6ParsedPayload *payload)
{
    payload->Construct8BitPayload(mDataSegment);
    mDataSegment->CalcChecksum();
}

RDD6DataSegment* DataSegmentHandler::TakeDataSegment()
{
    RDD6DataSegment *ret = mDataSegment;
    mDataSegment = 0;
    return ret;
}



DescriptionTextExpatHandler::DescriptionTextExpatHandler(RDD6DolbyEComplete *payload)
: ExpatHandler()
{
    mPayload = payload;
    mChildHandler = 0;
    mNextProgram = 0;

    uint8_t channel_count;
    get_program_config_info(mPayload->program_config, &mProgramCount, &channel_count);
}

DescriptionTextExpatHandler::~DescriptionTextExpatHandler()
{
    delete mChildHandler;
}

void DescriptionTextExpatHandler::StartElement(const string &ns, const string &name, const char **atts)
{
    if (mName.empty()) {
        mNamespace = ns;
        mName = name;
    } else if (!mChildHandler) {
        if (ElementNSEquals(ns, name, "program")) {
            mChildHandler = new SimpleTextExpatHandler();
        } else {
            log_warn("Ignoring element '%s %s'\n", ns.c_str(), name.c_str());
            mChildHandler = new IgnoreExpatHandler();
        }
    }

    if (mChildHandler)
        mChildHandler->StartElement(ns, name, atts);
}

bool DescriptionTextExpatHandler::EndElement(const string &ns, const string &name)
{
    if (mChildHandler) {
        if (mChildHandler->EndElement(ns, name)) {
            SimpleTextExpatHandler *text_handler = dynamic_cast<SimpleTextExpatHandler*>(mChildHandler);
            if (text_handler) {
                if (ElementNSEquals(ns, name, "program")) {
                    if (mNextProgram >= mProgramCount)
                        throw BMXException("Too many 'program' elements in 'descr_text'");

                    string clean_desc_text = text_handler->GetCData();
                    string::iterator iter = clean_desc_text.begin();
                    while (iter != clean_desc_text.end()) {
                        if (*iter < 0x20 || *iter > 0x7e)
                            iter = clean_desc_text.erase(iter);
                        else
                            iter++;
                    }

                    if (clean_desc_text.empty()) {
                        mPayload->desc_elements[mNextProgram].description_text = 0;
                        mPayload->xml_desc_elements[mNextProgram] = "";
                    } else {
                        mPayload->desc_elements[mNextProgram].description_text = clean_desc_text[0];
                        mPayload->xml_desc_elements[mNextProgram] = clean_desc_text;
                    }

                    mNextProgram++;
                }
            }
            delete mChildHandler;
            mChildHandler = 0;
        }
        return false;
    } else {
        if (mNextProgram != mProgramCount) {
            throw BMXException("Count of 'program' elements, %"PRIszt" does not equal expected value %"PRIszt,
                               mNextProgram, mProgramCount);
        }
        return true;
    }
}

void DescriptionTextExpatHandler::CharacterData(const char *s, int len)
{
    if (mChildHandler)
        mChildHandler->CharacterData(s, len);
}


DolbyECompleteExpatHandler::DolbyECompleteExpatHandler()
: DataSegmentHandler(), ExpatHandler()
{
    mPayload = new RDD6DolbyEComplete();
    mChildHandler = 0;
}

DolbyECompleteExpatHandler::~DolbyECompleteExpatHandler()
{
    delete mChildHandler;
    delete mPayload;
}

void DolbyECompleteExpatHandler::StartElement(const string &ns, const string &name, const char **atts)
{
    if (mName.empty()) {
        mNamespace = ns;
        mName = name;
    } else if (!mChildHandler) {
        if (ElementNSEquals(ns, name, "program_config") ||
            ElementNSEquals(ns, name, "frame_rate") ||
            ElementNSEquals(ns, name, "pitch_shift_code") ||
            ElementNSEquals(ns, name, "smpte_time_code"))
        {
            mChildHandler = new SimpleTextExpatHandler();
        }
        else if (ElementNSEquals(ns, name, "descr_text"))
        {
            mChildHandler = new DescriptionTextExpatHandler(mPayload);
        }
        else
        {
            log_warn("Ignoring element '%s %s'\n", ns.c_str(), name.c_str());
            mChildHandler = new IgnoreExpatHandler();
        }
    }

    if (mChildHandler)
        mChildHandler->StartElement(ns, name, atts);
}

bool DolbyECompleteExpatHandler::EndElement(const string &ns, const string &name)
{
    if (mChildHandler) {
        if (mChildHandler->EndElement(ns, name)) {
            SimpleTextExpatHandler *text_handler = dynamic_cast<SimpleTextExpatHandler*>(mChildHandler);
            if (text_handler) {
                if (ElementNSEquals(ns, name, "program_config")) {
                    mPayload->program_config = parse_xml_enum("program_config", PROGRAM_CONFIG_ENUM,
                                                              text_handler->GetCData());
                } else if (ElementNSEquals(ns, name, "frame_rate")) {
                    mPayload->frame_rate_code = parse_xml_enum("frame_rate", FRAME_RATE_ENUM,
                                                               text_handler->GetCData());
                } else if (ElementNSEquals(ns, name, "pitch_shift_code")) {
                    mPayload->pitch_shift_code = parse_xml_int16("pitch_shift_code", text_handler->GetCData());
                } else if (ElementNSEquals(ns, name, "smpte_time_code")) {
                    mPayload->smpte_time_code = parse_xml_smpte_timecode("smpte_time_code", text_handler->GetCData());
                }
            }
            delete mChildHandler;
            mChildHandler = 0;
        }
        return false;
    } else {
        size_t i;
        for (i = 0; i < BMX_ARRAY_SIZE(mPayload->xml_desc_elements); i++) {
            mDataSegment->xml_desc_elements[i] = mPayload->xml_desc_elements[i];
            mPayload->xml_desc_elements[i].clear();
        }
        CompletePayload(mPayload);
        return true;
    }
}

void DolbyECompleteExpatHandler::CharacterData(const char *s, int len)
{
    if (mChildHandler)
        mChildHandler->CharacterData(s, len);
}



DolbyEEssentialExpatHandler::DolbyEEssentialExpatHandler()
: DataSegmentHandler(), SimpleChildrenExpatHandler()
{
    mPayload = new RDD6DolbyEEssential();
}

DolbyEEssentialExpatHandler::~DolbyEEssentialExpatHandler()
{
    delete mPayload;
}

bool DolbyEEssentialExpatHandler::EndElement(const string &ns, const string &name)
{
    if (!SimpleChildrenExpatHandler::EndElement(ns, name))
        return false;

    mPayload->program_config = parse_xml_enum("program_config", PROGRAM_CONFIG_ENUM, GetChildStr("program_config"));
    mPayload->frame_rate_code = parse_xml_enum("frame_rate", FRAME_RATE_ENUM, GetChildStr("frame_rate"));
    if (HaveChild("pitch_shift_code"))
        mPayload->pitch_shift_code = parse_xml_int16("pitch_shift_code", GetChildStr("pitch_shift_code"));

    CompletePayload(mPayload);

    return true;
}



DolbyDigitalCompleteExtBSIExpatHandler::DolbyDigitalCompleteExtBSIExpatHandler()
: DataSegmentHandler(), SimpleChildrenExpatHandler()
{
    mPayload = new RDD6DolbyDigitalCompleteExtBSI();
}

DolbyDigitalCompleteExtBSIExpatHandler::~DolbyDigitalCompleteExtBSIExpatHandler()
{
    delete mPayload;
}

bool DolbyDigitalCompleteExtBSIExpatHandler::EndElement(const string &ns, const string &name)
{
    if (!SimpleChildrenExpatHandler::EndElement(ns, name))
        return false;

    mPayload->program_id = parse_xml_uint8("program_id", GetChildStr("program_id"));
    if (HaveChild("data_rate"))
        mPayload->ac3_datarate = parse_xml_enum("data_rate", DATARATE_ENUM, GetChildStr("data_rate"));
    mPayload->ac3_acmod = parse_xml_enum("ac_mode", ACMOD_ENUM, GetChildStr("ac_mode"));
    if (mPayload->ac3_acmod <= 1)
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_1_ENUM, GetChildStr("bs_mode"));
    else
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_2_ENUM, GetChildStr("bs_mode"));
    mPayload->ac3_cmixlev = parse_xml_enum("center_mix_level", CMIXLEVEL_ENUM, GetChildStr("center_mix_level"));
    mPayload->ac3_surmixlev = parse_xml_enum("sur_mix_level", SURMIXLEVEL_ENUM, GetChildStr("sur_mix_level"));
    if (HaveChild("sur_encoded"))
        mPayload->ac3_dsurmod = (parse_xml_bool("sur_encoded", GetChildStr("sur_encoded")) ? 2 : 1);
    mPayload->ac3_lfeon = parse_xml_bool("lfe_on", GetChildStr("lfe_on"));
    mPayload->ac3_dialnorm = - parse_xml_int8("dialnorm", GetChildStr("dialnorm"));
    if (HaveChild("lang_code")) {
        mPayload->ac3_langcode = 1;
        mPayload->ac3_langcod = parse_xml_hex_uint8("lang_code", GetChildStr("lang_code"));
    }
    if (HaveChild("mix_level")) {
        mPayload->ac3_audprodie = 1;
        mPayload->ac3_mixlevel = parse_xml_uint8("mix_level", GetChildStr("mix_level"));
        if (HaveChild("room_type"))
            mPayload->ac3_roomtyp = parse_xml_enum("room_type", ROOM_TYPE_ENUM, GetChildStr("room_type"));;
    }
    mPayload->ac3_copyrightb = parse_xml_bool("copyright", GetChildStr("copyright"));
    mPayload->ac3_origbs = parse_xml_bool("orig_bs", GetChildStr("orig_bs"));
    mPayload->ac3_xbsi1e = 1;
    if (HaveChild("downmix_mode"))
        mPayload->ac3_dmixmod = parse_xml_enum("downmix_mode", DMIXMOD_ENUM, GetChildStr("downmix_mode"));
    if (HaveChild("lt_rt_center_mix"))
        mPayload->ac3_ltrtcmixlev = parse_xml_enum("lt_rt_center_mix", LTRTCMIXLEVEL_ENUM, GetChildStr("lt_rt_center_mix"));
    if (HaveChild("lt_rt_sur_mix"))
        mPayload->ac3_ltrtsurmixlev = parse_xml_enum("lt_rt_sur_mix", LTRTSURMIXLEVEL_ENUM, GetChildStr("lt_rt_sur_mix"));
    if (HaveChild("lo_ro_center_mix"))
        mPayload->ac3_lorocmixlev = parse_xml_enum("lo_ro_center_mix", LOROCMIXLEVEL_ENUM, GetChildStr("lo_ro_center_mix"));
    if (HaveChild("lo_ro_sur_mix"))
        mPayload->ac3_lorosurmixlev = parse_xml_enum("lo_ro_sur_mix", LOROSURMIXLEVEL_ENUM, GetChildStr("lo_ro_sur_mix"));
    mPayload->ac3_xbsi2e = 1;
    if (HaveChild("sur_ex_encoded"))
        mPayload->ac3_dsurexmod = (parse_xml_bool("sur_ex_encoded", GetChildStr("sur_ex_encoded")) ? 2 : 1);
    if (HaveChild("headphone_encoded"))
        mPayload->ac3_dheadphonmod = (parse_xml_bool("headphone_encoded", GetChildStr("headphone_encoded")) ? 2 : 1);
    mPayload->ac3_adconvtyp = parse_xml_enum("ad_conv_type", ADCONVTYPE_ENUM, GetChildStr("ad_conv_type"));
    mPayload->ac3_hpfon = parse_xml_bool("hp_filter", GetChildStr("hp_filter"));
    mPayload->ac3_bwlpfon = parse_xml_bool("bw_lp_filter", GetChildStr("bw_lp_filter"));
    mPayload->ac3_lfelpfon = parse_xml_bool("lfe_lp_filter", GetChildStr("lfe_lp_filter"));
    mPayload->ac3_sur90on = parse_xml_bool("sur_90_filter", GetChildStr("sur_90_filter"));
    mPayload->ac3_suratton = parse_xml_bool("sur_att_filter", GetChildStr("sur_att_filter"));
    mPayload->ac3_rfpremphon = parse_xml_bool("rf_preemph_filter", GetChildStr("rf_preemph_filter"));
    if (HaveChild("compr_pf_1")) {
        mPayload->ac3_compr1 = parse_xml_enum("compr_pf_1", COMPR_ENUM, GetChildStr("compr_pf_1"));
    } else {
        mPayload->ac3_compre = 1;
        mPayload->ac3_compr1 = parse_xml_hex_uint8("compr_wd_1", GetChildStr("compr_wd_1"));
    }
    if (HaveChild("dyn_range_pf_1")) {
        mPayload->ac3_dynrng1 = parse_xml_enum("dyn_range_pf_1", DYNRNG_ENUM, GetChildStr("dyn_range_pf_1"));
        mPayload->ac3_dynrng2 = parse_xml_enum("dyn_range_pf_2", DYNRNG_ENUM, GetChildStr("dyn_range_pf_2"));
        mPayload->ac3_dynrng3 = parse_xml_enum("dyn_range_pf_3", DYNRNG_ENUM, GetChildStr("dyn_range_pf_3"));
        mPayload->ac3_dynrng4 = parse_xml_enum("dyn_range_pf_4", DYNRNG_ENUM, GetChildStr("dyn_range_pf_4"));
    } else {
        mPayload->ac3_dynrnge = 1;
        mPayload->ac3_dynrng1 = parse_xml_hex_uint8("dyn_range_wd_1", GetChildStr("dyn_range_wd_1"));
        mPayload->ac3_dynrng2 = parse_xml_hex_uint8("dyn_range_wd_2", GetChildStr("dyn_range_wd_2"));
        mPayload->ac3_dynrng3 = parse_xml_hex_uint8("dyn_range_wd_3", GetChildStr("dyn_range_wd_3"));
        mPayload->ac3_dynrng4 = parse_xml_hex_uint8("dyn_range_wd_4", GetChildStr("dyn_range_wd_4"));
    }

    CompletePayload(mPayload);

    return true;
}



DolbyDigitalEssentialExtBSIExpatHandler::DolbyDigitalEssentialExtBSIExpatHandler()
: DataSegmentHandler(), SimpleChildrenExpatHandler()
{
    mPayload = new RDD6DolbyDigitalEssentialExtBSI();
}

DolbyDigitalEssentialExtBSIExpatHandler::~DolbyDigitalEssentialExtBSIExpatHandler()
{
    delete mPayload;
}

bool DolbyDigitalEssentialExtBSIExpatHandler::EndElement(const string &ns, const string &name)
{
    if (!SimpleChildrenExpatHandler::EndElement(ns, name))
        return false;

    mPayload->program_id = parse_xml_uint8("program_id", GetChildStr("program_id"));
    if (HaveChild("data_rate"))
        mPayload->ac3_datarate = parse_xml_enum("data_rate", DATARATE_ENUM, GetChildStr("data_rate"));
    mPayload->ac3_acmod = parse_xml_enum("ac_mode", ACMOD_ENUM, GetChildStr("ac_mode"));
    if (mPayload->ac3_acmod <= 1)
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_1_ENUM, GetChildStr("bs_mode"));
    else
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_2_ENUM, GetChildStr("bs_mode"));
    mPayload->ac3_lfeon = parse_xml_bool("lfe_on", GetChildStr("lfe_on"));
    mPayload->ac3_dialnorm = - parse_xml_int8("dialnorm", GetChildStr("dialnorm"));
    if (HaveChild("compr_pf_2")) {
        mPayload->ac3_compr2 = parse_xml_enum("compr_pf_2", COMPR_ENUM, GetChildStr("compr_pf_2"));
    } else {
        mPayload->ac3_compre = 1;
        mPayload->ac3_compr2 = parse_xml_hex_uint8("compr_wd_2", GetChildStr("compr_wd_2"));
    }
    if (HaveChild("dyn_range_pf_5")) {
        mPayload->ac3_dynrng5 = parse_xml_enum("dyn_range_pf_5", DYNRNG_ENUM, GetChildStr("dyn_range_pf_5"));
        mPayload->ac3_dynrng6 = parse_xml_enum("dyn_range_pf_6", DYNRNG_ENUM, GetChildStr("dyn_range_pf_6"));
        mPayload->ac3_dynrng7 = parse_xml_enum("dyn_range_pf_7", DYNRNG_ENUM, GetChildStr("dyn_range_pf_7"));
        mPayload->ac3_dynrng8 = parse_xml_enum("dyn_range_pf_8", DYNRNG_ENUM, GetChildStr("dyn_range_pf_8"));
    } else {
        mPayload->ac3_dynrnge = 1;
        mPayload->ac3_dynrng5 = parse_xml_hex_uint8("dyn_range_wd_5", GetChildStr("dyn_range_wd_5"));
        mPayload->ac3_dynrng6 = parse_xml_hex_uint8("dyn_range_wd_6", GetChildStr("dyn_range_wd_6"));
        mPayload->ac3_dynrng7 = parse_xml_hex_uint8("dyn_range_wd_7", GetChildStr("dyn_range_wd_7"));
        mPayload->ac3_dynrng8 = parse_xml_hex_uint8("dyn_range_wd_8", GetChildStr("dyn_range_wd_8"));
    }

    CompletePayload(mPayload);

    return true;
}



DolbyDigitalCompleteExpatHandler::DolbyDigitalCompleteExpatHandler()
: DataSegmentHandler(), SimpleChildrenExpatHandler()
{
    mPayload = new RDD6DolbyDigitalComplete();
}

DolbyDigitalCompleteExpatHandler::~DolbyDigitalCompleteExpatHandler()
{
    delete mPayload;
}

bool DolbyDigitalCompleteExpatHandler::EndElement(const string &ns, const string &name)
{
    if (!SimpleChildrenExpatHandler::EndElement(ns, name))
        return false;

    mPayload->program_id = parse_xml_uint8("program_id", GetChildStr("program_id"));
    mPayload->ac3_acmod = parse_xml_enum("ac_mode", ACMOD_ENUM, GetChildStr("ac_mode"));
    if (mPayload->ac3_acmod <= 1)
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_1_ENUM, GetChildStr("bs_mode"));
    else
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_2_ENUM, GetChildStr("bs_mode"));
    mPayload->ac3_cmixlev = parse_xml_enum("center_mix_level", CMIXLEVEL_ENUM, GetChildStr("center_mix_level"));
    mPayload->ac3_surmixlev = parse_xml_enum("sur_mix_level", SURMIXLEVEL_ENUM, GetChildStr("sur_mix_level"));
    if (HaveChild("sur_encoded"))
        mPayload->ac3_dsurmod = (parse_xml_bool("sur_encoded", GetChildStr("sur_encoded")) ? 2 : 1);
    mPayload->ac3_lfeon = parse_xml_bool("lfe_on", GetChildStr("lfe_on"));
    mPayload->ac3_dialnorm = - parse_xml_int8("dialnorm", GetChildStr("dialnorm"));
    if (HaveChild("lang_code")) {
        mPayload->ac3_langcode = 1;
        mPayload->ac3_langcod = parse_xml_hex_uint8("lang_code", GetChildStr("lang_code"));
    }
    if (HaveChild("mix_level")) {
        mPayload->ac3_audprodie = 1;
        mPayload->ac3_mixlevel = parse_xml_uint8("mix_level", GetChildStr("mix_level"));
        if (HaveChild("room_type"))
            mPayload->ac3_roomtyp = parse_xml_enum("room_type", ROOM_TYPE_ENUM, GetChildStr("room_type"));;
    }
    mPayload->ac3_copyrightb = parse_xml_bool("copyright", GetChildStr("copyright"));
    mPayload->ac3_origbs = parse_xml_bool("orig_bs", GetChildStr("orig_bs"));
    if (HaveChild("timecode")) {
        parse_xml_timecode("timecode", GetChildStr("timecode"),
                           &mPayload->ac3_timecod1e, &mPayload->ac3_timecod2e,
                           &mPayload->ac3_timecod1, &mPayload->ac3_timecod2);
    }
    mPayload->ac3_hpfon = parse_xml_bool("hp_filter", GetChildStr("hp_filter"));
    mPayload->ac3_bwlpfon = parse_xml_bool("bw_lp_filter", GetChildStr("bw_lp_filter"));
    mPayload->ac3_lfelpfon = parse_xml_bool("lfe_lp_filter", GetChildStr("lfe_lp_filter"));
    mPayload->ac3_sur90on = parse_xml_bool("sur_90_filter", GetChildStr("sur_90_filter"));
    mPayload->ac3_suratton = parse_xml_bool("sur_att_filter", GetChildStr("sur_att_filter"));
    mPayload->ac3_rfpremphon = parse_xml_bool("rf_preemph_filter", GetChildStr("rf_preemph_filter"));
    if (HaveChild("compr_pf_1")) {
        mPayload->ac3_compr1 = parse_xml_enum("compr_pf_1", COMPR_ENUM, GetChildStr("compr_pf_1"));
    } else {
        mPayload->ac3_compre = 1;
        mPayload->ac3_compr1 = parse_xml_hex_uint8("compr_wd_1", GetChildStr("compr_wd_1"));
    }
    if (HaveChild("dyn_range_pf_1")) {
        mPayload->ac3_dynrng1 = parse_xml_enum("dyn_range_pf_1", DYNRNG_ENUM, GetChildStr("dyn_range_pf_1"));
        mPayload->ac3_dynrng2 = parse_xml_enum("dyn_range_pf_2", DYNRNG_ENUM, GetChildStr("dyn_range_pf_2"));
        mPayload->ac3_dynrng3 = parse_xml_enum("dyn_range_pf_3", DYNRNG_ENUM, GetChildStr("dyn_range_pf_3"));
        mPayload->ac3_dynrng4 = parse_xml_enum("dyn_range_pf_4", DYNRNG_ENUM, GetChildStr("dyn_range_pf_4"));
    } else {
        mPayload->ac3_dynrnge = 1;
        mPayload->ac3_dynrng1 = parse_xml_hex_uint8("dyn_range_wd_1", GetChildStr("dyn_range_wd_1"));
        mPayload->ac3_dynrng2 = parse_xml_hex_uint8("dyn_range_wd_2", GetChildStr("dyn_range_wd_2"));
        mPayload->ac3_dynrng3 = parse_xml_hex_uint8("dyn_range_wd_3", GetChildStr("dyn_range_wd_3"));
        mPayload->ac3_dynrng4 = parse_xml_hex_uint8("dyn_range_wd_4", GetChildStr("dyn_range_wd_4"));
    }

    CompletePayload(mPayload);

    return true;
}



DolbyDigitalEssentialExpatHandler::DolbyDigitalEssentialExpatHandler()
: DataSegmentHandler(), SimpleChildrenExpatHandler()
{
    mPayload = new RDD6DolbyDigitalEssential();
}

DolbyDigitalEssentialExpatHandler::~DolbyDigitalEssentialExpatHandler()
{
    delete mPayload;
}

bool DolbyDigitalEssentialExpatHandler::EndElement(const string &ns, const string &name)
{
    if (!SimpleChildrenExpatHandler::EndElement(ns, name))
        return false;

    mPayload->program_id = parse_xml_uint8("program_id", GetChildStr("program_id"));
    if (HaveChild("data_rate"))
        mPayload->ac3_datarate = parse_xml_enum("data_rate", DATARATE_ENUM, GetChildStr("data_rate"));
    mPayload->ac3_acmod = parse_xml_enum("ac_mode", ACMOD_ENUM, GetChildStr("ac_mode"));
    if (mPayload->ac3_acmod <= 1)
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_1_ENUM, GetChildStr("bs_mode"));
    else
        mPayload->ac3_bsmod = parse_xml_enum("bs_mode", BSMOD_2_ENUM, GetChildStr("bs_mode"));
    mPayload->ac3_lfeon = parse_xml_bool("lfe_on", GetChildStr("lfe_on"));
    mPayload->ac3_dialnorm = - parse_xml_int8("dialnorm", GetChildStr("dialnorm"));
    if (HaveChild("compr_pf_2")) {
        mPayload->ac3_compr2 = parse_xml_enum("compr_pf_2", COMPR_ENUM, GetChildStr("compr_pf_2"));
    } else {
        mPayload->ac3_compre = 1;
        mPayload->ac3_compr2 = parse_xml_hex_uint8("compr_wd_2", GetChildStr("compr_wd_2"));
    }
    if (HaveChild("dyn_range_pf_5")) {
        mPayload->ac3_dynrng5 = parse_xml_enum("dyn_range_pf_5", DYNRNG_ENUM, GetChildStr("dyn_range_pf_5"));
        mPayload->ac3_dynrng6 = parse_xml_enum("dyn_range_pf_6", DYNRNG_ENUM, GetChildStr("dyn_range_pf_6"));
        mPayload->ac3_dynrng7 = parse_xml_enum("dyn_range_pf_7", DYNRNG_ENUM, GetChildStr("dyn_range_pf_7"));
        mPayload->ac3_dynrng8 = parse_xml_enum("dyn_range_pf_8", DYNRNG_ENUM, GetChildStr("dyn_range_pf_8"));
    } else {
        mPayload->ac3_dynrnge = 1;
        mPayload->ac3_dynrng5 = parse_xml_hex_uint8("dyn_range_wd_5", GetChildStr("dyn_range_wd_5"));
        mPayload->ac3_dynrng6 = parse_xml_hex_uint8("dyn_range_wd_6", GetChildStr("dyn_range_wd_6"));
        mPayload->ac3_dynrng7 = parse_xml_hex_uint8("dyn_range_wd_7", GetChildStr("dyn_range_wd_7"));
        mPayload->ac3_dynrng8 = parse_xml_hex_uint8("dyn_range_wd_8", GetChildStr("dyn_range_wd_8"));
    }

    CompletePayload(mPayload);

    return true;
}



RawDataSegmentExpatHandler::RawDataSegmentExpatHandler()
: DataSegmentHandler(), SimpleTextExpatHandler()
{
}

RawDataSegmentExpatHandler::~RawDataSegmentExpatHandler()
{
}

void RawDataSegmentExpatHandler::StartElement(const string &ns, const string &name, const char **atts)
{
    SimpleTextExpatHandler::StartElement(ns, name, atts);

    mDataSegment->id = parse_xml_uint8("id", GetAttribute("id", atts));
}

bool RawDataSegmentExpatHandler::EndElement(const string &ns, const string &name)
{
    SimpleTextExpatHandler::EndElement(ns, name);

    if (mCData.size() > 1) {
        mDataSegment->payload_buffer.Allocate((uint32_t)(mCData.size() / 2)); // 2 hex digits per byte
        size_t parsed_size = parse_xml_bytes(name, mCData,
                                             mDataSegment->payload_buffer.GetBytesAvailable(),
                                             mDataSegment->payload_buffer.GetSizeAvailable());
        if (parsed_size > 256)
            throw BMXException("Data segment size %"PRIszt" exceeds maximum size 256", parsed_size);

        mDataSegment->size = (uint16_t)parsed_size;
        mDataSegment->payload_buffer.SetSize(mDataSegment->size);
        if (mDataSegment->size > 0)
            mDataSegment->payload = mDataSegment->payload_buffer.GetBytes();
    }
    if (mDataSegment->size == 0)
        log_warn("Failed to parse payload bytes from 'segment' element\n");

    mDataSegment->CalcChecksum();

    return true;
}



SyncExpatHandler::SyncExpatHandler(RDD6SyncSegment *sync_segment)
: SimpleChildrenExpatHandler()
{
    mSyncSegment = sync_segment;
}

SyncExpatHandler::~SyncExpatHandler()
{
}

bool SyncExpatHandler::EndElement(const string &ns, const string &name)
{
    if (!SimpleChildrenExpatHandler::EndElement(ns, name))
        return false;

    mSyncSegment->revision_id = parse_xml_hex_uint8("rev_id", GetChildStr("rev_id"));
    mSyncSegment->originator_id = parse_xml_hex_uint8("orig_id", GetChildStr("orig_id"));
    if (mSyncSegment->originator_id != 0)
        mSyncSegment->originator_address = parse_xml_hex_uint16("orig_addr", GetChildStr("orig_addr"));
    mSyncSegment->frame_count = parse_xml_uint16("frame_count", GetChildStr("frame_count"));

    return true;
}



SubFrameExpatHandler::SubFrameExpatHandler(bool is_first)
: ExpatHandler()
{
    mSubFrame = new RDD6MetadataSubFrame(is_first);
    mChildHandler = 0;
}

SubFrameExpatHandler::~SubFrameExpatHandler()
{
    delete mChildHandler;
    delete mSubFrame;
}

void SubFrameExpatHandler::StartElement(const string &ns, const string &name, const char **atts)
{
    if (mName.empty()) {
        mNamespace = ns;
        mName = name;
    } else if (!mChildHandler) {
        if (ElementNSEquals(ns, name, "sync")) {
            mChildHandler = new SyncExpatHandler(&mSubFrame->sync_segment);
        } else if (ElementNSEquals(ns, name, "segment")) {
            mChildHandler = new RawDataSegmentExpatHandler();
        } else if (ElementNSEquals(ns, name, "dolby_e_complete")) {
            mChildHandler = new DolbyECompleteExpatHandler();
        } else if (ElementNSEquals(ns, name, "dolby_e_essential")) {
            mChildHandler = new DolbyEEssentialExpatHandler();
        } else if (ElementNSEquals(ns, name, "dolby_digital_complete_ext_bsi")) {
            mChildHandler = new DolbyDigitalCompleteExtBSIExpatHandler();
        } else if (ElementNSEquals(ns, name, "dolby_digital_essential_ext_bsi")) {
            mChildHandler = new DolbyDigitalEssentialExtBSIExpatHandler();
        } else if (ElementNSEquals(ns, name, "dolby_digital_complete")) {
            mChildHandler = new DolbyDigitalCompleteExpatHandler();
        } else if (ElementNSEquals(ns, name, "dolby_digital_essential")) {
            mChildHandler = new DolbyDigitalEssentialExpatHandler();
        } else {
            log_warn("Ignoring element '%s %s'\n", ns.c_str(), name.c_str());
            mChildHandler = new IgnoreExpatHandler();
        }
    }

    if (mChildHandler)
        mChildHandler->StartElement(ns, name, atts);
}

bool SubFrameExpatHandler::EndElement(const string &ns, const string &name)
{
    if (mChildHandler) {
        if (mChildHandler->EndElement(ns, name)) {
            DataSegmentHandler *dseg_handler = dynamic_cast<DataSegmentHandler*>(mChildHandler);
            if (dseg_handler)
                mSubFrame->data_segments.push_back(dseg_handler->TakeDataSegment());
            delete mChildHandler;
            mChildHandler = 0;
        }
        return false;
    } else {
        return true;
    }
}

void SubFrameExpatHandler::CharacterData(const char *s, int len)
{
    if (mChildHandler)
        mChildHandler->CharacterData(s, len);
}

RDD6MetadataSubFrame* SubFrameExpatHandler::TakeSubFrame()
{
    RDD6MetadataSubFrame *ret = mSubFrame;
    mSubFrame = 0;
    return ret;
}



RDD6MetadataExpat::RDD6MetadataExpat(RDD6MetadataFrame *rdd6_frame)
: ExpatHandler()
{
    mRDD6Frame = rdd6_frame;
    mParser = 0;
    mChildHandler = 0;
}

RDD6MetadataExpat::~RDD6MetadataExpat()
{
    delete mChildHandler;
}

bool RDD6MetadataExpat::ParseXML(const string &filename)
{
    FILE *file = 0;
    char *buffer = 0;
    const size_t buffer_size = 4096;
    bool result = true;

    try
    {
        file = fopen(filename.c_str(), "rb");
        if (!file)
            throw BMXException("Failed to open XML file '%s': %s", filename.c_str(), strerror(errno));

        mParser = XML_ParserCreateNS(0, NAMESPACE_SEPARATOR);
        if (!mParser)
            throw BMXException("XML_ParserCreate returned NULL");

        XML_SetStartElementHandler(mParser,  expat_StartElement);
        XML_SetEndElementHandler(mParser,    expat_EndElement);
        XML_SetCharacterDataHandler(mParser, expat_CharacterData);
        XML_SetUserData(mParser, this);

        buffer = new char[buffer_size];
        size_t num_read;
        do {
            num_read = fread(buffer, 1, buffer_size, file);
            if (num_read != buffer_size && ferror(file))
                throw BMXException("Failed to read from XML file: %s", bmx_strerror(errno).c_str());

            if (XML_Parse(mParser, buffer, (int)num_read, num_read != buffer_size) == XML_STATUS_ERROR) {
                throw BMXException("XML parse error near line %lu: %s",
                                   XML_GetCurrentLineNumber(mParser), XML_ErrorString(XML_GetErrorCode(mParser)));
            }
        } while (num_read == buffer_size);
    }
    catch (const BMXException &ex)
    {
        log_error("Failed to parse RDD-6 XML file '%s': %s\n", filename.c_str(), ex.what());
        result = false;
    }
    catch (...)
    {
        log_error("Failed to parse RDD-6 XML file '%s': unknown exception thrown\n", filename.c_str());
        result = false;
    }

    delete [] buffer;
    if (file)
        fclose(file);
    if (mParser) {
        XML_ParserFree(mParser);
        mParser = 0;
    }

    return result;
}

void RDD6MetadataExpat::StartElement(const string &ns, const string &name, const char **atts)
{
    START_EXPAT_CALLBACK

    if (mName.empty()) {
        if (name != "rdd6")
            throw BMXException("Root element name should be 'rdd6', not '%s'", name.c_str());
        mNamespace = ns;
        mName = name;
    } else {
        if (!mChildHandler) {
            if (ElementNSEquals(ns, name, "first_subframe")) {
                mChildHandler = new SubFrameExpatHandler(true);
            } else if (ElementNSEquals(ns, name, "second_subframe")) {
                mChildHandler = new SubFrameExpatHandler(false);
            } else {
                log_warn("Ignoring element '%s %s'\n", ns.c_str(), name.c_str());
                mChildHandler = new IgnoreExpatHandler();
            }
        }
        mChildHandler->StartElement(ns, name, atts);
    }

    END_EXPAT_CALLBACK
}

bool RDD6MetadataExpat::EndElement(const string &ns, const string &name)
{
    START_EXPAT_CALLBACK

    if (mChildHandler) {
        if (mChildHandler->EndElement(ns, name)) {
            SubFrameExpatHandler *subframe_handler = dynamic_cast<SubFrameExpatHandler*>(mChildHandler);
            if (subframe_handler) {
                RDD6MetadataSubFrame *subframe = subframe_handler->TakeSubFrame();
                if (subframe->IsFirst()) {
                    delete mRDD6Frame->second_sub_frame;
                    mRDD6Frame->second_sub_frame = 0;
                    delete mRDD6Frame->first_sub_frame;
                    mRDD6Frame->first_sub_frame = subframe;
                } else {
                    delete mRDD6Frame->second_sub_frame;
                    mRDD6Frame->second_sub_frame = subframe;
                }
            }
            delete mChildHandler;
            mChildHandler = 0;
        }
        return false;
    } else {
        if (!mRDD6Frame->first_sub_frame)
            log_warn("Missing first subframe element\n");
        if (!mRDD6Frame->second_sub_frame)
            log_warn("Missing second subframe element\n");
        mRDD6Frame->end_frame_sync_word = RDD6MetadataFrame::END_FRAME_SYNC_WORD;
        return true;
    }

    END_EXPAT_CALLBACK
    return false;
}

void RDD6MetadataExpat::CharacterData(const char *s, int len)
{
    START_EXPAT_CALLBACK

    if (mChildHandler)
        mChildHandler->CharacterData(s, len);

    END_EXPAT_CALLBACK
}

