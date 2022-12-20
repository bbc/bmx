add_executable(bmxtranswrap
    bmxtranswrap.cpp
    MXFInputTrack.cpp
)

target_include_directories(bmxtranswrap PRIVATE
    "${PROJECT_BINARY_DIR}"
)
target_compile_definitions(bmxtranswrap PRIVATE
    HAVE_CONFIG_H
)

target_link_libraries(bmxtranswrap PRIVATE
    bmx_app_writers
)

install(TARGETS bmxtranswrap DESTINATION bin)
