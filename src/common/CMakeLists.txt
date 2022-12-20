list(APPEND bmx_sources
    common/BMXException.cpp
    common/BMXTypes.cpp
    common/BitBuffer.cpp
    common/ByteArray.cpp
    common/ByteBuffer.cpp
    common/CRC32.cpp
    common/Checksum.cpp
    common/EssenceType.cpp
    common/KLVParser.cpp
    common/Logging.cpp
    common/MD5.cpp
    common/MXFChecksumFile.cpp
    common/MXFHTTPFile.cpp
    common/MXFUtils.cpp
    common/SHA1.cpp
    common/URI.cpp
    common/Utils.cpp
    common/Version.cpp
    common/XMLUtils.cpp
    common/XMLWriter.cpp
)

set(bmx_sources ${bmx_sources} PARENT_SCOPE)
