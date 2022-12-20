set(tool_sources
    h264dump.c
    j2cdump.c
    jp2extract.c
    movdump.cpp
    rdd36dump.c
    vc2dump.c
)

foreach(tool_source ${tool_sources})
    get_filename_component(tool_target ${tool_source} NAME_WLE)
    add_executable(${tool_target}
        ${tool_source}
    )
    install(TARGETS ${tool_target} DESTINATION bin)
endforeach()
