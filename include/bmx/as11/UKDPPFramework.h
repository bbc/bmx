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

#ifndef BMX_UK_DPP_FRAMEWORK_H_
#define BMX_UK_DPP_FRAMEWORK_H_


#include <libMXF++/metadata/DMFramework.h>



namespace bmx
{


class UKDPPFramework : public mxfpp::DMFramework
{
public:
    static void RegisterObjectFactory(mxfpp::HeaderMetadata *header_metadata);

public:
    static const mxfKey set_key;

public:
    friend class mxfpp::MetadataSetFactory<UKDPPFramework>;

public:
    UKDPPFramework(mxfpp::HeaderMetadata *header_metadata);
    virtual ~UKDPPFramework();

    std::string GetProductionNumber();
    std::string GetSynopsis();
    std::string GetOriginator();
    uint16_t GetCopyrightYear();
    bool HaveOtherIdentifier();
    std::string GetOtherIdentifier();
    bool HaveOtherIdentifierType();
    std::string GetOtherIdentifierType();
    bool HaveGenre();
    std::string GetGenre();
    bool HaveDistributor();
    std::string GetDistributor();
    bool HavePictureRatio();
    mxfRational GetPictureRatio();
    bool Get3D();
    bool Have3DType();
    uint8_t Get3DType();
    bool HaveProductPlacement();
    bool GetProductPlacement();
    uint8_t GetPSEPass();
    bool HavePSEManufacturer();
    std::string GetPSEManufacturer();
    bool HavePSEVersion();
    std::string GetPSEVersion();
    bool HaveVideoComments();
    std::string GetVideoComments();
    std::string GetSecondaryAudioLanguage();
    std::string GetTertiaryAudioLanguage();
    uint8_t GetAudioLoudnessStandard();
    bool HaveAudioComments();
    std::string GetAudioComments();
    int64_t GetLineUpStart();
    int64_t GetIdentClockStart();
    uint16_t GetTotalNumberOfParts();
    int64_t GetTotalProgrammeDuration();
    bool GetAudioDescriptionPresent();
    bool HaveAudioDescriptionType();
    uint8_t GetAudioDescriptionType();
    bool GetOpenCaptionsPresent();
    bool HaveOpenCaptionsType();
    uint8_t GetOpenCaptionsType();
    bool HaveOpenCaptionsLanguage();
    std::string GetOpenCaptionsLanguage();
    uint8_t GetSigningPresent();
    bool HaveSignLanguage();
    uint8_t GetSignLanguage();
    mxfTimestamp GetCompletionDate();
    bool HaveTextlessElementsExist();
    bool GetTextlessElementsExist();
    bool HaveProgrammeHasText();
    bool GetProgrammeHasText();
    bool HaveProgrammeTextLanguage();
    std::string GetProgrammeTextLanguage();
    std::string GetContactEmail();
    std::string GetContactTelephoneNumber();

    void SetProductionNumber(std::string value);
    void SetSynopsis(std::string value);
    void SetOriginator(std::string value);
    void SetCopyrightYear(uint16_t value);
    void SetOtherIdentifier(std::string value);
    void SetOtherIdentifierType(std::string value);
    void SetGenre(std::string value);
    void SetDistributor(std::string value);
    void SetPictureRatio(mxfRational value);
    void Set3D(bool value);
    void Set3DType(uint8_t value);
    void SetProductPlacement(bool value);
    void SetPSEPass(uint8_t value);
    void SetPSEManufacturer(std::string value);
    void SetPSEVersion(std::string value);
    void SetVideoComments(std::string value);
    void SetSecondaryAudioLanguage(std::string value);
    void SetTertiaryAudioLanguage(std::string value);
    void SetAudioLoudnessStandard(uint8_t value);
    void SetAudioComments(std::string value);
    void SetLineUpStart(int64_t value);
    void SetIdentClockStart(int64_t value);
    void SetTotalNumberOfParts(uint16_t value);
    void SetTotalProgrammeDuration(int64_t value);
    void SetAudioDescriptionPresent(bool value);
    void SetAudioDescriptionType(uint8_t value);
    void SetOpenCaptionsPresent(bool value);
    void SetOpenCaptionsType(uint8_t value);
    void SetOpenCaptionsLanguage(std::string value);
    void SetSigningPresent(uint8_t value);
    void SetSignLanguage(uint8_t value);
    void SetCompletionDate(mxfTimestamp value);
    void SetTextlessElementsExist(bool value);
    void SetProgrammeHasText(bool value);
    void SetProgrammeTextLanguage(std::string value);
    void SetContactEmail(std::string value);
    void SetContactTelephoneNumber(std::string value);

protected:
    UKDPPFramework(mxfpp::HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set);
};



};


#endif

