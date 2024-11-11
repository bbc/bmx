list(APPEND bmx_sources
    essence_parser/AVCEssenceParser.cpp
    essence_parser/AVCIRawEssenceReader.cpp
    essence_parser/D10RawEssenceReader.cpp
    essence_parser/DVEssenceParser.cpp
    essence_parser/EssenceParser.cpp
    essence_parser/EssenceParserUtils.cpp
    essence_parser/FileEssenceSource.cpp
    essence_parser/FilePatternEssenceSource.cpp
    essence_parser/J2CEssenceParser.cpp
    essence_parser/KLVEssenceReader.cpp
    essence_parser/KLVEssenceSource.cpp
    essence_parser/MJPEGEssenceParser.cpp
    essence_parser/MPEG2AspectRatioFilter.cpp
    essence_parser/MPEG2EssenceParser.cpp
    essence_parser/RDD36EssenceParser.cpp
    essence_parser/RawEssenceReader.cpp
    essence_parser/SoundConversion.cpp
    essence_parser/VC2EssenceParser.cpp
    essence_parser/VC3EssenceParser.cpp
    essence_parser/JXSEssenceParser.cpp
)

set(bmx_sources ${bmx_sources} PARENT_SCOPE)
