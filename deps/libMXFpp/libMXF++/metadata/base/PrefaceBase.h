/*
 * Copyright (C) 2008, British Broadcasting Corporation
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

#ifndef MXFPP_PREFACE_BASE_H_
#define MXFPP_PREFACE_BASE_H_



#include <libMXF++/metadata/InterchangeObject.h>


namespace mxfpp
{


class PrefaceBase : public InterchangeObject
{
public:
    friend class MetadataSetFactory<PrefaceBase>;
    static const mxfKey setKey;

public:
    PrefaceBase(HeaderMetadata *headerMetadata);
    virtual ~PrefaceBase();


   // getters

   mxfTimestamp getLastModifiedDate() const;
   uint16_t getVersion() const;
   bool haveObjectModelVersion() const;
   uint32_t getObjectModelVersion() const;
   bool havePrimaryPackage() const;
   GenericPackage* getPrimaryPackage() const;
   std::vector<Identification*> getIdentifications() const;
   ContentStorage* getContentStorage() const;
   mxfUL getOperationalPattern() const;
   std::vector<mxfUL> getEssenceContainers() const;
   std::vector<mxfUL> getDMSchemes() const;
   bool haveIsRIPPresent() const;
   bool getIsRIPPresent() const;


   // setters

   void setLastModifiedDate(mxfTimestamp value);
   void setVersion(uint16_t value);
   void setObjectModelVersion(uint32_t value);
   void setPrimaryPackage(GenericPackage *value);
   void setIdentifications(const std::vector<Identification*> &value);
   void appendIdentifications(Identification *value);
   void setContentStorage(ContentStorage *value);
   void setOperationalPattern(mxfUL value);
   void setEssenceContainers(const std::vector<mxfUL> &value);
   void appendEssenceContainers(mxfUL value);
   void setDMSchemes(const std::vector<mxfUL> &value);
   void appendDMSchemes(mxfUL value);
   void setIsRIPPresent(bool value);


protected:
    PrefaceBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
