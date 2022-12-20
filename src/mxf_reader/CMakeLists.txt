list(APPEND bmx_sources
    mxf_reader/EssenceChunkHelper.cpp
    mxf_reader/EssenceReader.cpp
    mxf_reader/FrameMetadataReader.cpp
    mxf_reader/IndexTableHelper.cpp
    mxf_reader/MXFAPPInfo.cpp
    mxf_reader/MXFFileIndex.cpp
    mxf_reader/MXFFileReader.cpp
    mxf_reader/MXFFileTrackReader.cpp
    mxf_reader/MXFFrameBuffer.cpp
    mxf_reader/MXFFrameMetadata.cpp
    mxf_reader/MXFGroupReader.cpp
    mxf_reader/MXFIndexEntryExt.cpp
    mxf_reader/MXFMCALabelIndex.cpp
    mxf_reader/MXFPackageResolver.cpp
    mxf_reader/MXFReader.cpp
    mxf_reader/MXFSequenceReader.cpp
    mxf_reader/MXFSequenceTrackReader.cpp
    mxf_reader/MXFTextObject.cpp
    mxf_reader/MXFTimedTextTrackReader.cpp
    mxf_reader/MXFTrackInfo.cpp
)

set(bmx_sources ${bmx_sources} PARENT_SCOPE)
