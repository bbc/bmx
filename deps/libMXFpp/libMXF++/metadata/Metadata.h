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

#ifndef MXFPP_METADATA_H_
#define MXFPP_METADATA_H_



#include <libMXF++/metadata/InterchangeObject.h>
#include <libMXF++/metadata/Sequence.h>
#include <libMXF++/metadata/GenericTrack.h>
#include <libMXF++/metadata/GenericPackage.h>
#include <libMXF++/metadata/Identification.h>
#include <libMXF++/metadata/EssenceContainerData.h>
#include <libMXF++/metadata/ContentStorage.h>
#include <libMXF++/metadata/Locator.h>
#include <libMXF++/metadata/NetworkLocator.h>
#include <libMXF++/metadata/TextLocator.h>
#include <libMXF++/metadata/StaticTrack.h>
#include <libMXF++/metadata/Track.h>
#include <libMXF++/metadata/EventTrack.h>
#include <libMXF++/metadata/StructuralComponent.h>
#include <libMXF++/metadata/TimecodeComponent.h>
#include <libMXF++/metadata/SourceClip.h>
#include <libMXF++/metadata/DMFramework.h>
#include <libMXF++/metadata/DMSegment.h>
#include <libMXF++/metadata/DMSourceClip.h>
#include <libMXF++/metadata/SubDescriptor.h>
#include <libMXF++/metadata/GenericDescriptor.h>
#include <libMXF++/metadata/FileDescriptor.h>
#include <libMXF++/metadata/GenericPictureEssenceDescriptor.h>
#include <libMXF++/metadata/CDCIEssenceDescriptor.h>
#include <libMXF++/metadata/MPEGVideoDescriptor.h>
#include <libMXF++/metadata/RGBAEssenceDescriptor.h>
#include <libMXF++/metadata/GenericSoundEssenceDescriptor.h>
#include <libMXF++/metadata/GenericDataEssenceDescriptor.h>
#include <libMXF++/metadata/MultipleDescriptor.h>
#include <libMXF++/metadata/WaveAudioDescriptor.h>
#include <libMXF++/metadata/AES3AudioDescriptor.h>
#include <libMXF++/metadata/MaterialPackage.h>
#include <libMXF++/metadata/SourcePackage.h>
#include <libMXF++/metadata/Preface.h>
#include <libMXF++/metadata/ANCDataDescriptor.h>
#include <libMXF++/metadata/VBIDataDescriptor.h>
#include <libMXF++/metadata/DMSet.h>
#include <libMXF++/metadata/TextBasedObject.h>
#include <libMXF++/metadata/TextBasedDMFramework.h>
#include <libMXF++/metadata/GenericStreamTextBasedSet.h>
#include <libMXF++/metadata/UTF8TextBasedSet.h>
#include <libMXF++/metadata/UTF16TextBasedSet.h>
#include <libMXF++/metadata/AVCSubDescriptor.h>
#include <libMXF++/metadata/MCALabelSubDescriptor.h>
#include <libMXF++/metadata/AudioChannelLabelSubDescriptor.h>
#include <libMXF++/metadata/SoundfieldGroupLabelSubDescriptor.h>
#include <libMXF++/metadata/GroupOfSoundfieldGroupsLabelSubDescriptor.h>
#include <libMXF++/metadata/VC2SubDescriptor.h>
#include <libMXF++/metadata/DCTimedTextDescriptor.h>
#include <libMXF++/metadata/DCTimedTextResourceSubDescriptor.h>
#include <libMXF++/metadata/JPEG2000SubDescriptor.h>
#include <libMXF++/metadata/JPEGXSSubDescriptor.h>
#include <libMXF++/metadata/RIFFChunkDefinitionSubDescriptor.h>
#include <libMXF++/metadata/ADM_CHNASubDescriptor.h>
#include <libMXF++/metadata/ADMChannelMapping.h>
#include <libMXF++/metadata/ADMAudioMetadataSubDescriptor.h>
#include <libMXF++/metadata/RIFFChunkReferencesSubDescriptor.h>
#include <libMXF++/metadata/ADMSoundfieldGroupLabelSubDescriptor.h>
#include <libMXF++/metadata/MGASoundEssenceDescriptor.h>
#include <libMXF++/metadata/MGAAudioMetadataSubDescriptor.h>
#include <libMXF++/metadata/SADMAudioMetadataSubDescriptor.h>
#include <libMXF++/metadata/MGASoundfieldGroupLabelSubDescriptor.h>
#include <libMXF++/metadata/ContainerConstraintsSubDescriptor.h>


#endif
