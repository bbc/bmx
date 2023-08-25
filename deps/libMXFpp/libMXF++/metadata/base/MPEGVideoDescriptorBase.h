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

#ifndef MXFPP_MPEGVIDEODESCRIPTOR_BASE_H_
#define MXFPP_MPEGVIDEODESCRIPTOR_BASE_H_



#include <libMXF++/metadata/CDCIEssenceDescriptor.h>


namespace mxfpp
{


class MPEGVideoDescriptorBase : public CDCIEssenceDescriptor
{
public:
    friend class MetadataSetFactory<MPEGVideoDescriptorBase>;
    static const mxfKey setKey;

public:
    MPEGVideoDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~MPEGVideoDescriptorBase();


   // getters

   bool haveSingleSequence() const;
   bool getSingleSequence() const;
   bool haveConstantBFrames() const;
   bool getConstantBFrames() const;
   bool haveCodedContentType() const;
   uint8_t getCodedContentType() const;
   bool haveLowDelay() const;
   bool getLowDelay() const;
   bool haveClosedGOP() const;
   bool getClosedGOP() const;
   bool haveIdenticalGOP() const;
   bool getIdenticalGOP() const;
   bool haveMaxGOP() const;
   uint16_t getMaxGOP() const;
   bool haveMaxBPictureCount() const;
   uint16_t getMaxBPictureCount() const;
   bool haveBitRate() const;
   uint32_t getBitRate() const;
   bool haveProfileAndLevel() const;
   uint8_t getProfileAndLevel() const;


   // setters

   void setSingleSequence(bool value);
   void setConstantBFrames(bool value);
   void setCodedContentType(uint8_t value);
   void setLowDelay(bool value);
   void setClosedGOP(bool value);
   void setIdenticalGOP(bool value);
   void setMaxGOP(uint16_t value);
   void setMaxBPictureCount(uint16_t value);
   void setBitRate(uint32_t value);
   void setProfileAndLevel(uint8_t value);


protected:
    MPEGVideoDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
