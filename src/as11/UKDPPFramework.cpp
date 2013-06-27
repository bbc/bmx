/*
 * Copyright (C) 2011, British Broadcasting Corporation
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

#include <bmx/as11/UKDPPFramework.h>
#include <bmx/as11/UKDPPDMS.h>
#include <bmx/BMXException.h>
#include <bmx/Logging.h>

using namespace std;
using namespace bmx;
using namespace mxfpp;



const mxfKey UKDPPFramework::set_key = MXF_SET_K(UKDPPFramework);


void UKDPPFramework::RegisterObjectFactory(HeaderMetadata *header_metadata)
{
    header_metadata->registerObjectFactory(&set_key, new MetadataSetFactory<UKDPPFramework>());
}

UKDPPFramework::UKDPPFramework(HeaderMetadata *header_metadata)
: DMFramework(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);
}

UKDPPFramework::UKDPPFramework(HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set)
: DMFramework(header_metadata, c_metadata_set)
{
}

UKDPPFramework::~UKDPPFramework()
{
}

string UKDPPFramework::GetProductionNumber()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProductionNumber));
}

string UKDPPFramework::GetSynopsis()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPSynopsis));
}

string UKDPPFramework::GetOriginator()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOriginator));
}

uint16_t UKDPPFramework::GetCopyrightYear()
{
    return getUInt16Item(&MXF_ITEM_K(UKDPPFramework, UKDPPCopyrightYear));
}

bool UKDPPFramework::HaveOtherIdentifier()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifier));
}

string UKDPPFramework::GetOtherIdentifier()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifier));
}

bool UKDPPFramework::HaveOtherIdentifierType()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifierType));
}

string UKDPPFramework::GetOtherIdentifierType()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifierType));
}

bool UKDPPFramework::HaveGenre()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPGenre));
}

string UKDPPFramework::GetGenre()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPGenre));
}

bool UKDPPFramework::HaveDistributor()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPDistributor));
}

string UKDPPFramework::GetDistributor()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPDistributor));
}

bool UKDPPFramework::HavePictureRatio()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPictureRatio));
}

mxfRational UKDPPFramework::GetPictureRatio()
{
    return getRationalItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPictureRatio));
}

bool UKDPPFramework::Get3D()
{
    return getBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPP3D));
}

bool UKDPPFramework::Have3DType()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPP3DType));
}

uint8_t UKDPPFramework::Get3DType()
{
    return getUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPP3DType));
}

bool UKDPPFramework::HaveProductPlacement()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProductPlacement));
}

bool UKDPPFramework::GetProductPlacement()
{
    return getBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProductPlacement));
}

uint8_t UKDPPFramework::GetPSEPass()
{
    return getUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEPass));
}

bool UKDPPFramework::HavePSEManufacturer()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEManufacturer));
}

string UKDPPFramework::GetPSEManufacturer()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEManufacturer));
}

bool UKDPPFramework::HavePSEVersion()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEVersion));
}

string UKDPPFramework::GetPSEVersion()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEVersion));
}

bool UKDPPFramework::HaveVideoComments()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPVideoComments));
}

string UKDPPFramework::GetVideoComments()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPVideoComments));
}

string UKDPPFramework::GetSecondaryAudioLanguage()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPSecondaryAudioLanguage));
}

string UKDPPFramework::GetTertiaryAudioLanguage()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTertiaryAudioLanguage));
}

uint8_t UKDPPFramework::GetAudioLoudnessStandard()
{
    return getUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioLoudnessStandard));
}

bool UKDPPFramework::HaveAudioComments()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioComments));
}

string UKDPPFramework::GetAudioComments()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioComments));
}

int64_t UKDPPFramework::GetLineUpStart()
{
    return getInt64Item(&MXF_ITEM_K(UKDPPFramework, UKDPPLineUpStart));
}

int64_t UKDPPFramework::GetIdentClockStart()
{
    return getInt64Item(&MXF_ITEM_K(UKDPPFramework, UKDPPIdentClockStart));
}

uint16_t UKDPPFramework::GetTotalNumberOfParts()
{
    return getUInt16Item(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalNumberOfParts));
}

int64_t UKDPPFramework::GetTotalProgrammeDuration()
{
    return getInt64Item(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalProgrammeDuration));
}

bool UKDPPFramework::GetAudioDescriptionPresent()
{
    return getBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionPresent));
}

bool UKDPPFramework::HaveAudioDescriptionType()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionType));
}

uint8_t UKDPPFramework::GetAudioDescriptionType()
{
    return getUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionType));
}

bool UKDPPFramework::GetOpenCaptionsPresent()
{
    return getBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsPresent));
}

bool UKDPPFramework::HaveOpenCaptionsType()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsType));
}

uint8_t UKDPPFramework::GetOpenCaptionsType()
{
    return getUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsType));
}

bool UKDPPFramework::HaveOpenCaptionsLanguage()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsLanguage));
}

string UKDPPFramework::GetOpenCaptionsLanguage()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsLanguage));
}

uint8_t UKDPPFramework::GetSigningPresent()
{
    return getUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPSigningPresent));
}

bool UKDPPFramework::HaveSignLanguage()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPSignLanguage));
}

uint8_t UKDPPFramework::GetSignLanguage()
{
    return getUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPSignLanguage));
}

mxfTimestamp UKDPPFramework::GetCompletionDate()
{
    return getTimestampItem(&MXF_ITEM_K(UKDPPFramework, UKDPPCompletionDate));
}

bool UKDPPFramework::HaveTextlessElementsExist()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTextlessElementsExist));
}

bool UKDPPFramework::GetTextlessElementsExist()
{
    return getBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTextlessElementsExist));
}

bool UKDPPFramework::HaveProgrammeHasText()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeHasText));
}

bool UKDPPFramework::GetProgrammeHasText()
{
    return getBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeHasText));
}

bool UKDPPFramework::HaveProgrammeTextLanguage()
{
    return haveItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeTextLanguage));
}

string UKDPPFramework::GetProgrammeTextLanguage()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeTextLanguage));
}

string UKDPPFramework::GetContactEmail()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPContactEmail));
}

string UKDPPFramework::GetContactTelephoneNumber()
{
    return getStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPContactTelephoneNumber));
}

void UKDPPFramework::SetProductionNumber(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProductionNumber), value);
}

void UKDPPFramework::SetSynopsis(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPSynopsis), value);
}

void UKDPPFramework::SetOriginator(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOriginator), value);
}

void UKDPPFramework::SetCopyrightYear(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(UKDPPFramework, UKDPPCopyrightYear), value);
}

void UKDPPFramework::SetOtherIdentifier(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifier), value);
}

void UKDPPFramework::SetOtherIdentifierType(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOtherIdentifierType), value);
}

void UKDPPFramework::SetGenre(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPGenre), value);
}

void UKDPPFramework::SetDistributor(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPDistributor), value);
}

void UKDPPFramework::SetPictureRatio(mxfRational value)
{
    setRationalItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPictureRatio), value);
}

void UKDPPFramework::Set3D(bool value)
{
    setBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPP3D), value);
}

void UKDPPFramework::Set3DType(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPP3DType), value);
}

void UKDPPFramework::SetProductPlacement(bool value)
{
    setBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProductPlacement), value);
}

void UKDPPFramework::SetPSEPass(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEPass), value);
}

void UKDPPFramework::SetPSEManufacturer(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEManufacturer), value);
}

void UKDPPFramework::SetPSEVersion(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPPSEVersion), value);
}

void UKDPPFramework::SetVideoComments(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPVideoComments), value);
}

void UKDPPFramework::SetSecondaryAudioLanguage(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPSecondaryAudioLanguage), value);
}

void UKDPPFramework::SetTertiaryAudioLanguage(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTertiaryAudioLanguage), value);
}

void UKDPPFramework::SetAudioLoudnessStandard(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioLoudnessStandard), value);
}

void UKDPPFramework::SetAudioComments(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioComments), value);
}

void UKDPPFramework::SetLineUpStart(int64_t value)
{
    setInt64Item(&MXF_ITEM_K(UKDPPFramework, UKDPPLineUpStart), value);
}

void UKDPPFramework::SetIdentClockStart(int64_t value)
{
    setInt64Item(&MXF_ITEM_K(UKDPPFramework, UKDPPIdentClockStart), value);
}

void UKDPPFramework::SetTotalNumberOfParts(uint16_t value)
{
    setUInt16Item(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalNumberOfParts), value);
}

void UKDPPFramework::SetTotalProgrammeDuration(int64_t value)
{
    setInt64Item(&MXF_ITEM_K(UKDPPFramework, UKDPPTotalProgrammeDuration), value);
}

void UKDPPFramework::SetAudioDescriptionPresent(bool value)
{
    setBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionPresent), value);
}

void UKDPPFramework::SetAudioDescriptionType(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPAudioDescriptionType), value);
}

void UKDPPFramework::SetOpenCaptionsPresent(bool value)
{
    setBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsPresent), value);
}

void UKDPPFramework::SetOpenCaptionsType(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsType), value);
}

void UKDPPFramework::SetOpenCaptionsLanguage(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPOpenCaptionsLanguage), value);
}

void UKDPPFramework::SetSigningPresent(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPSigningPresent), value);
}

void UKDPPFramework::SetSignLanguage(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(UKDPPFramework, UKDPPSignLanguage), value);
}

void UKDPPFramework::SetCompletionDate(mxfTimestamp value)
{
    setTimestampItem(&MXF_ITEM_K(UKDPPFramework, UKDPPCompletionDate), value);
}

void UKDPPFramework::SetTextlessElementsExist(bool value)
{
    setBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPTextlessElementsExist), value);
}

void UKDPPFramework::SetProgrammeHasText(bool value)
{
    setBooleanItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeHasText), value);
}

void UKDPPFramework::SetProgrammeTextLanguage(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPProgrammeTextLanguage), value);
}

void UKDPPFramework::SetContactEmail(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPContactEmail), value);
}

void UKDPPFramework::SetContactTelephoneNumber(string value)
{
    setStringItem(&MXF_ITEM_K(UKDPPFramework, UKDPPContactTelephoneNumber), value);
}

