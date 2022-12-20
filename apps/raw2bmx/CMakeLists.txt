add_executable(raw2bmx
    raw2bmx.cpp
    RawInputTrack.cpp
)

target_include_directories(raw2bmx PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(raw2bmx PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(raw2bmx PRIVATE
    bmx_app_writers
)

install(TARGETS raw2bmx DESTINATION bin)
