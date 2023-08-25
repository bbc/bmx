add_executable(MXFDump
    MXFDump.cpp
)

if(UNIX)
    # Ignore unsed functions warnings
    target_compile_options(MXFDump PRIVATE -Wno-unused-function)
endif()

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(MXFDump "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

install(TARGETS MXFDump DESTINATION bin)