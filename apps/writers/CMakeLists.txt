add_library(bmx_app_writers STATIC
    InputTrack.cpp
    OutputTrack.cpp
    TrackMapper.cpp
)

target_include_directories(bmx_app_writers PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(bmx_app_writers PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(bmx_app_writers PUBLIC
    bmx
)

if(UNIX)
    # The static library will be linked into the dynamic libraries of the writer apps.
    # The -fPIC compile option ensures it generates position independent code
    target_compile_options(bmx_app_writers PRIVATE
        -fPIC
    )
endif()
