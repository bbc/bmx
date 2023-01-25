set(tool_sources
    h264dump.c
    j2cdump.c
    jp2extract.c
    movdump.cpp
    rdd36dump.c
    vc2dump.c
)

foreach(tool_source ${tool_sources})
    get_filename_component(tool_target ${tool_source} NAME_WE)
    add_executable(${tool_target}
        ${tool_source}
    )

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(${tool_target} "${CMAKE_CURRENT_LIST_DIR}" "bmx")

    install(TARGETS ${tool_target} DESTINATION bin)
endforeach()
