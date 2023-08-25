/*
 * Copyright (C) 2016, British Broadcasting Corporation
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

#ifndef MXFPP_MCALABELSUBDESCRIPTOR_BASE_H_
#define MXFPP_MCALABELSUBDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/SubDescriptor.h>


namespace mxfpp
{


class MCALabelSubDescriptorBase : public SubDescriptor
{
public:
    friend class MetadataSetFactory<MCALabelSubDescriptorBase>;
    static const mxfKey setKey;

public:
    MCALabelSubDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~MCALabelSubDescriptorBase();


   // getters

   bool haveMCAChannelID() const;
   uint32_t getMCAChannelID() const;
   mxfUL getMCALabelDictionaryID() const;
   std::string getMCATagSymbol() const;
   bool haveMCATagName() const;
   std::string getMCATagName() const;
   mxfUUID getMCALinkID() const;
   bool haveMCAPartitionKind() const;
   std::string getMCAPartitionKind() const;
   bool haveMCAPartitionNumber() const;
   std::string getMCAPartitionNumber() const;
   bool haveMCATitle() const;
   std::string getMCATitle() const;
   bool haveMCATitleVersion() const;
   std::string getMCATitleVersion() const;
   bool haveMCATitleSubVersion() const;
   std::string getMCATitleSubVersion() const;
   bool haveMCAEpisode() const;
   std::string getMCAEpisode() const;
   bool haveRFC5646SpokenLanguage() const;
   std::string getRFC5646SpokenLanguage() const;
   bool haveMCAAudioContentKind() const;
   std::string getMCAAudioContentKind() const;
   bool haveMCAAudioElementKind() const;
   std::string getMCAAudioElementKind() const;
   bool haveMCAContent() const;
   std::string getMCAContent() const;
   bool haveMCAUseClass() const;
   std::string getMCAUseClass() const;
   bool haveMCAContentSubtype() const;
   std::string getMCAContentSubtype() const;
   bool haveMCAContentDifferentiator() const;
   std::string getMCAContentDifferentiator() const;
   bool haveMCASpokenLanguageAttribute() const;
   std::string getMCASpokenLanguageAttribute() const;
   bool haveRFC5646AdditionalSpokenLanguages() const;
   std::string getRFC5646AdditionalSpokenLanguages() const;
   bool haveMCAAdditionalLanguageAttributes() const;
   std::string getMCAAdditionalLanguageAttributes() const;


   // setters

   void setMCAChannelID(uint32_t value);
   void setMCALabelDictionaryID(mxfUL value);
   void setMCATagSymbol(std::string value);
   void setMCATagName(std::string value);
   void setMCALinkID(mxfUUID value);
   void setMCAPartitionKind(std::string value);
   void setMCAPartitionNumber(std::string value);
   void setMCATitle(std::string value);
   void setMCATitleVersion(std::string value);
   void setMCATitleSubVersion(std::string value);
   void setMCAEpisode(std::string value);
   void setRFC5646SpokenLanguage(std::string value);
   void setMCAAudioContentKind(std::string value);
   void setMCAAudioElementKind(std::string value);
   void setMCAContent(std::string value);
   void setMCAUseClass(std::string value);
   void setMCAContentSubtype(std::string value);
   void setMCAContentDifferentiator(std::string value);
   void setMCASpokenLanguageAttribute(std::string value);
   void setRFC5646AdditionalSpokenLanguages(std::string value);
   void setMCAAdditionalLanguageAttributes(std::string value);


protected:
    MCALabelSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
