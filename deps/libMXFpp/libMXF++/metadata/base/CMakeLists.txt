list(APPEND MXFpp_sources
    metadata/base/ADM_CHNASubDescriptorBase.cpp
    metadata/base/ADMAudioMetadataSubDescriptorBase.cpp
    metadata/base/ADMChannelMappingBase.cpp
    metadata/base/ADMSoundfieldGroupLabelSubDescriptorBase.cpp
    metadata/base/AES3AudioDescriptorBase.cpp
    metadata/base/ANCDataDescriptorBase.cpp
    metadata/base/AVCSubDescriptorBase.cpp
    metadata/base/AudioChannelLabelSubDescriptorBase.cpp
    metadata/base/CDCIEssenceDescriptorBase.cpp
    metadata/base/ContainerConstraintsSubDescriptorBase.cpp
    metadata/base/ContentStorageBase.cpp
    metadata/base/DCTimedTextDescriptorBase.cpp
    metadata/base/DCTimedTextResourceSubDescriptorBase.cpp
    metadata/base/DMFrameworkBase.cpp
    metadata/base/DMSegmentBase.cpp
    metadata/base/DMSetBase.cpp
    metadata/base/DMSourceClipBase.cpp
    metadata/base/EssenceContainerDataBase.cpp
    metadata/base/EventTrackBase.cpp
    metadata/base/FileDescriptorBase.cpp
    metadata/base/GenericDataEssenceDescriptorBase.cpp
    metadata/base/GenericDescriptorBase.cpp
    metadata/base/GenericPackageBase.cpp
    metadata/base/GenericPictureEssenceDescriptorBase.cpp
    metadata/base/GenericSoundEssenceDescriptorBase.cpp
    metadata/base/GenericStreamTextBasedSetBase.cpp
    metadata/base/GenericTrackBase.cpp
    metadata/base/GroupOfSoundfieldGroupsLabelSubDescriptorBase.cpp
    metadata/base/IdentificationBase.cpp
    metadata/base/InterchangeObjectBase.cpp
    metadata/base/JPEG2000SubDescriptorBase.cpp
    metadata/base/JPEGXSSubDescriptorBase.cpp
    metadata/base/LocatorBase.cpp
    metadata/base/MCALabelSubDescriptorBase.cpp
    metadata/base/MGAAudioMetadataSubDescriptorBase.cpp
    metadata/base/MGASoundEssenceDescriptorBase.cpp
    metadata/base/MGASoundfieldGroupLabelSubDescriptorBase.cpp
    metadata/base/MPEGVideoDescriptorBase.cpp
    metadata/base/MaterialPackageBase.cpp
    metadata/base/MultipleDescriptorBase.cpp
    metadata/base/NetworkLocatorBase.cpp
    metadata/base/PrefaceBase.cpp
    metadata/base/RGBAEssenceDescriptorBase.cpp
    metadata/base/RIFFChunkDefinitionSubDescriptorBase.cpp
    metadata/base/RIFFChunkReferencesSubDescriptorBase.cpp
    metadata/base/SADMAudioMetadataSubDescriptorBase.cpp
    metadata/base/SequenceBase.cpp
    metadata/base/SoundfieldGroupLabelSubDescriptorBase.cpp
    metadata/base/SourceClipBase.cpp
    metadata/base/SourcePackageBase.cpp
    metadata/base/StaticTrackBase.cpp
    metadata/base/StructuralComponentBase.cpp
    metadata/base/SubDescriptorBase.cpp
    metadata/base/TextBasedDMFrameworkBase.cpp
    metadata/base/TextBasedObjectBase.cpp
    metadata/base/TextLocatorBase.cpp
    metadata/base/TimecodeComponentBase.cpp
    metadata/base/TrackBase.cpp
    metadata/base/UTF16TextBasedSetBase.cpp
    metadata/base/UTF8TextBasedSetBase.cpp
    metadata/base/VBIDataDescriptorBase.cpp
    metadata/base/VC2SubDescriptorBase.cpp
    metadata/base/WaveAudioDescriptorBase.cpp
)

list(APPEND MXFpp_headers
    metadata/base/ADM_CHNASubDescriptorBase.h
    metadata/base/ADMAudioMetadataSubDescriptorBase.h
    metadata/base/ADMChannelMappingBase.h
    metadata/base/ADMSoundfieldGroupLabelSubDescriptorBase.h
    metadata/base/AES3AudioDescriptorBase.h
    metadata/base/ANCDataDescriptorBase.h
    metadata/base/AVCSubDescriptorBase.h
    metadata/base/AudioChannelLabelSubDescriptorBase.h
    metadata/base/CDCIEssenceDescriptorBase.h
    metadata/base/ContainerConstraintsSubDescriptorBase.h
    metadata/base/ContentStorageBase.h
    metadata/base/DCTimedTextDescriptorBase.h
    metadata/base/DCTimedTextResourceSubDescriptorBase.h
    metadata/base/DMFrameworkBase.h
    metadata/base/DMSegmentBase.h
    metadata/base/DMSetBase.h
    metadata/base/DMSourceClipBase.h
    metadata/base/EssenceContainerDataBase.h
    metadata/base/EventTrackBase.h
    metadata/base/FileDescriptorBase.h
    metadata/base/GenericDataEssenceDescriptorBase.h
    metadata/base/GenericDescriptorBase.h
    metadata/base/GenericPackageBase.h
    metadata/base/GenericPictureEssenceDescriptorBase.h
    metadata/base/GenericSoundEssenceDescriptorBase.h
    metadata/base/GenericStreamTextBasedSetBase.h
    metadata/base/GenericTrackBase.h
    metadata/base/GroupOfSoundfieldGroupsLabelSubDescriptorBase.h
    metadata/base/IdentificationBase.h
    metadata/base/InterchangeObjectBase.h
    metadata/base/JPEG2000SubDescriptorBase.h
    metadata/base/JPEGXSSubDescriptorBase.h
    metadata/base/LocatorBase.h
    metadata/base/MCALabelSubDescriptorBase.h
    metadata/base/MGAAudioMetadataSubDescriptorBase.h
    metadata/base/MGASoundEssenceDescriptorBase.h
    metadata/base/MGASoundfieldGroupLabelSubDescriptorBase.h
    metadata/base/MPEGVideoDescriptorBase.h
    metadata/base/MaterialPackageBase.h
    metadata/base/MultipleDescriptorBase.h
    metadata/base/NetworkLocatorBase.h
    metadata/base/PrefaceBase.h
    metadata/base/RGBAEssenceDescriptorBase.h
    metadata/base/RIFFChunkDefinitionSubDescriptorBase.h
    metadata/base/RIFFChunkReferencesSubDescriptorBase.h
    metadata/base/SADMAudioMetadataSubDescriptorBase.h
    metadata/base/SequenceBase.h
    metadata/base/SoundfieldGroupLabelSubDescriptorBase.h
    metadata/base/SourceClipBase.h
    metadata/base/SourcePackageBase.h
    metadata/base/StaticTrackBase.h
    metadata/base/StructuralComponentBase.h
    metadata/base/SubDescriptorBase.h
    metadata/base/TextBasedDMFrameworkBase.h
    metadata/base/TextBasedObjectBase.h
    metadata/base/TextLocatorBase.h
    metadata/base/TimecodeComponentBase.h
    metadata/base/TrackBase.h
    metadata/base/UTF16TextBasedSetBase.h
    metadata/base/UTF8TextBasedSetBase.h
    metadata/base/VBIDataDescriptorBase.h
    metadata/base/VC2SubDescriptorBase.h
    metadata/base/WaveAudioDescriptorBase.h
)

set(MXFpp_sources ${MXFpp_sources} PARENT_SCOPE)
set(MXFpp_headers ${MXFpp_headers} PARENT_SCOPE)
