list(APPEND bmx_headers
    bmx/mxf_reader/EssenceChunkHelper.h
    bmx/mxf_reader/EssenceReader.h
    bmx/mxf_reader/FrameMetadataReader.h
    bmx/mxf_reader/IndexTableHelper.h
    bmx/mxf_reader/MXFAPPInfo.h
    bmx/mxf_reader/MXFFileIndex.h
    bmx/mxf_reader/MXFFileReader.h
    bmx/mxf_reader/MXFFileTrackReader.h
    bmx/mxf_reader/MXFFrameBuffer.h
    bmx/mxf_reader/MXFFrameMetadata.h
    bmx/mxf_reader/MXFGroupReader.h
    bmx/mxf_reader/MXFIndexEntryExt.h
    bmx/mxf_reader/MXFMCALabelIndex.h
    bmx/mxf_reader/MXFPackageResolver.h
    bmx/mxf_reader/MXFReader.h
    bmx/mxf_reader/MXFSequenceReader.h
    bmx/mxf_reader/MXFSequenceTrackReader.h
    bmx/mxf_reader/MXFTextObject.h
    bmx/mxf_reader/MXFTimedTextTrackReader.h
    bmx/mxf_reader/MXFTrackInfo.h
    bmx/mxf_reader/MXFTrackReader.h
)

set(bmx_headers ${bmx_headers} PARENT_SCOPE)
