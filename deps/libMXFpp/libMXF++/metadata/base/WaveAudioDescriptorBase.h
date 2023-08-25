/*
 * Copyright (C) 2012, British Broadcasting Corporation
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

#ifndef MXFPP_WAVEAUDIODESCRIPTOR_BASE_H_
#define MXFPP_WAVEAUDIODESCRIPTOR_BASE_H_



#include <libMXF++/metadata/GenericSoundEssenceDescriptor.h>


namespace mxfpp
{


class WaveAudioDescriptorBase : public GenericSoundEssenceDescriptor
{
public:
    friend class MetadataSetFactory<WaveAudioDescriptorBase>;
    static const mxfKey setKey;

public:
    WaveAudioDescriptorBase(HeaderMetadata *headerMetadata);
    virtual ~WaveAudioDescriptorBase();


   // getters

   uint16_t getBlockAlign() const;
   bool haveSequenceOffset() const;
   uint8_t getSequenceOffset() const;
   uint32_t getAvgBps() const;
   bool haveChannelAssignment() const;
   mxfUL getChannelAssignment() const;
   bool havePeakEnvelopeVersion() const;
   uint32_t getPeakEnvelopeVersion() const;
   bool havePeakEnvelopeFormat() const;
   uint32_t getPeakEnvelopeFormat() const;
   bool havePointsPerPeakValue() const;
   uint32_t getPointsPerPeakValue() const;
   bool havePeakEnvelopeBlockSize() const;
   uint32_t getPeakEnvelopeBlockSize() const;
   bool havePeakChannels() const;
   uint32_t getPeakChannels() const;
   bool havePeakFrames() const;
   uint32_t getPeakFrames() const;
   bool havePeakOfPeaksPosition() const;
   int64_t getPeakOfPeaksPosition() const;
   bool havePeakEnvelopeTimestamp() const;
   mxfTimestamp getPeakEnvelopeTimestamp() const;
   bool havePeakEnvelopeData() const;
   ByteArray getPeakEnvelopeData() const;


   // setters

   void setBlockAlign(uint16_t value);
   void setSequenceOffset(uint8_t value);
   void setAvgBps(uint32_t value);
   void setChannelAssignment(mxfUL value);
   void setPeakEnvelopeVersion(uint32_t value);
   void setPeakEnvelopeFormat(uint32_t value);
   void setPointsPerPeakValue(uint32_t value);
   void setPeakEnvelopeBlockSize(uint32_t value);
   void setPeakChannels(uint32_t value);
   void setPeakFrames(uint32_t value);
   void setPeakOfPeaksPosition(int64_t value);
   void setPeakEnvelopeTimestamp(mxfTimestamp value);
   void setPeakEnvelopeData(ByteArray value);


protected:
    WaveAudioDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet);
};


};


#endif
