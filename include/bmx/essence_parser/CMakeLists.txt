list(APPEND bmx_headers
    bmx/essence_parser/AVCEssenceParser.h
    bmx/essence_parser/AVCIRawEssenceReader.h
    bmx/essence_parser/D10RawEssenceReader.h
    bmx/essence_parser/DVEssenceParser.h
    bmx/essence_parser/EssenceFilter.h
    bmx/essence_parser/EssenceParser.h
    bmx/essence_parser/EssenceSource.h
    bmx/essence_parser/FileEssenceSource.h
    bmx/essence_parser/FilePatternEssenceSource.h
    bmx/essence_parser/J2CEssenceParser.h
    bmx/essence_parser/KLVEssenceSource.h
    bmx/essence_parser/MJPEGEssenceParser.h
    bmx/essence_parser/MPEG2AspectRatioFilter.h
    bmx/essence_parser/MPEG2EssenceParser.h
    bmx/essence_parser/RDD36EssenceParser.h
    bmx/essence_parser/RawEssenceReader.h
    bmx/essence_parser/SoundConversion.h
    bmx/essence_parser/VC2EssenceParser.h
    bmx/essence_parser/VC3EssenceParser.h
)

set(bmx_headers ${bmx_headers} PARENT_SCOPE)
