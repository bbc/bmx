add_executable(mxf2raw
    APPInfoOutput.cpp
    AS10InfoOutput.cpp
    AS11InfoOutput.cpp
    AvidInfoOutput.cpp
    mxf2raw.cpp
    OutputFileManager.cpp
)

target_include_directories(mxf2raw PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(mxf2raw PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(mxf2raw PRIVATE
    bmx
)

install(TARGETS mxf2raw DESTINATION bin)
