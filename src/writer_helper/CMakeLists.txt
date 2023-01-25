list(APPEND bmx_sources
    writer_helper/AVCIWriterHelper.cpp
    writer_helper/AVCWriterHelper.cpp
    writer_helper/D10WriterHelper.cpp
    writer_helper/JPEG2000WriterHelper.cpp
    writer_helper/MPEG2LGWriterHelper.cpp
    writer_helper/VC2WriterHelper.cpp
    writer_helper/XMLWriterHelper.cpp
)

set(bmx_sources ${bmx_sources} PARENT_SCOPE)
