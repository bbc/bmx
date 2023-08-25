/*
 * Copyright (C) 2015, British Broadcasting Corporation
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

#ifndef MXFPP_VC2SUBDESCRIPTOR_BASE_H_
#define MXFPP_VC2SUBDESCRIPTOR_BASE_H_



#include <libMXF++/metadata/SubDescriptor.h>


namespace mxfpp
{


class VC2SubDescriptorBase : public SubDescriptor
{
public:
    friend class MetadataSetFactory<VC2SubDescriptorBase>;
    static const mxfKey setKey;

public:
    VC2SubDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~VC2SubDescriptorBase();


   // getters

   uint8_t getVC2MajorVersion() const;
   uint8_t getVC2MinorVersion() const;
   uint8_t getVC2Profile() const;
   uint8_t getVC2Level() const;
   bool haveVC2WaveletFilters() const;
   std::vector<uint8_t> getVC2WaveletFilters() const;
   bool haveVC2SequenceHeadersIdentical() const;
   bool getVC2SequenceHeadersIdentical() const;
   bool haveVC2EditUnitsAreCompleteSequences() const;
   bool getVC2EditUnitsAreCompleteSequences() const;


   // setters

   void setVC2MajorVersion(uint8_t value);
   void setVC2MinorVersion(uint8_t value);
   void setVC2Profile(uint8_t value);
   void setVC2Level(uint8_t value);
   void setVC2WaveletFilters(const std::vector<uint8_t> &value);
   void appendVC2WaveletFilters(uint8_t value);
   void setVC2SequenceHeadersIdentical(bool value);
   void setVC2EditUnitsAreCompleteSequences(bool value);


protected:
    VC2SubDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
