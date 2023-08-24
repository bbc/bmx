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

#ifndef MXFPP_AVCSUBDESCRIPTOR_BASE_H_
#define MXFPP_AVCSUBDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/SubDescriptor.h>


namespace mxfpp
{


class AVCSubDescriptorBase : public SubDescriptor
{
public:
    friend class MetadataSetFactory<AVCSubDescriptorBase>;
    static const mxfKey setKey;

public:
    AVCSubDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~AVCSubDescriptorBase();


   // getters

   uint8_t getAVCDecodingDelay() const;
   bool haveAVCConstantBPictureFlag() const;
   bool getAVCConstantBPictureFlag() const;
   bool haveAVCCodedContentKind() const;
   uint8_t getAVCCodedContentKind() const;
   bool haveAVCClosedGOPIndicator() const;
   bool getAVCClosedGOPIndicator() const;
   bool haveAVCIdenticalGOPIndicator() const;
   bool getAVCIdenticalGOPIndicator() const;
   bool haveAVCMaximumGOPSize() const;
   uint16_t getAVCMaximumGOPSize() const;
   bool haveAVCMaximumBPictureCount() const;
   uint16_t getAVCMaximumBPictureCount() const;
   bool haveAVCMaximumBitrate() const;
   uint32_t getAVCMaximumBitrate() const;
   bool haveAVCAverageBitrate() const;
   uint32_t getAVCAverageBitrate() const;
   bool haveAVCProfile() const;
   uint8_t getAVCProfile() const;
   bool haveAVCProfileConstraint() const;
   uint8_t getAVCProfileConstraint() const;
   bool haveAVCLevel() const;
   uint8_t getAVCLevel() const;
   bool haveAVCMaximumRefFrames() const;
   uint8_t getAVCMaximumRefFrames() const;
   bool haveAVCSequenceParameterSetFlag() const;
   uint8_t getAVCSequenceParameterSetFlag() const;
   bool haveAVCPictureParameterSetFlag() const;
   uint8_t getAVCPictureParameterSetFlag() const;


   // setters

   void setAVCDecodingDelay(uint8_t value);
   void setAVCConstantBPictureFlag(bool value);
   void setAVCCodedContentKind(uint8_t value);
   void setAVCClosedGOPIndicator(bool value);
   void setAVCIdenticalGOPIndicator(bool value);
   void setAVCMaximumGOPSize(uint16_t value);
   void setAVCMaximumBPictureCount(uint16_t value);
   void setAVCMaximumBitrate(uint32_t value);
   void setAVCAverageBitrate(uint32_t value);
   void setAVCProfile(uint8_t value);
   void setAVCProfileConstraint(uint8_t value);
   void setAVCLevel(uint8_t value);
   void setAVCMaximumRefFrames(uint8_t value);
   void setAVCSequenceParameterSetFlag(uint8_t value);
   void setAVCPictureParameterSetFlag(uint8_t value);


protected:
    AVCSubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
