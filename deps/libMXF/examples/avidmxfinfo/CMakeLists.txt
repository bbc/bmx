add_library(avidmxfinfo
    avid_mxf_info.c
)
set(avidmxfinfo_headers
    avid_mxf_info.h
)

target_include_directories(avidmxfinfo PRIVATE
    ${PROJECT_BINARY_DIR}
)
target_compile_definitions(avidmxfinfo PRIVATE
    HAVE_CONFIG_H=1
)

target_link_libraries(avidmxfinfo PUBLIC
    MXF
)

set_target_properties(avidmxfinfo PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(avidmxfinfo "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

install(TARGETS avidmxfinfo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES ${avidmxfinfo_headers} DESTINATION include/mxf/examples/avidmxfinfo)


add_executable(avidmxfinfo-bin
    main.c
)
target_link_libraries(avidmxfinfo-bin
    avidmxfinfo
    MXF
)

set_target_properties(avidmxfinfo-bin PROPERTIES
    OUTPUT_NAME avidmxfinfo
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(avidmxfinfo-bin "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

install(TARGETS avidmxfinfo-bin DESTINATION bin)


if(LIBMXF_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    if(LIBMXF_TEST_WITH_VALGRIND)
        set(command_prefix valgrind --leak-check=full -q --error-exitcode=1)
    else()
        set(command_prefix)
    endif()

    set(tests_25
        IMX30 IMX40 IMX50
        DNxHD1080p1235 DNxHD1080p1237 DNxHD1080p1238
        DNxHD1080i1241 DNxHD1080i1242 DNxHD1080i1243
        DNxHD720p1250 DNxHD720p1251 DNxHD720p1252
        DNxHD1080p1253
        unc unc1080i unc720p
    )

    foreach(test ${tests_25})
        set(test_file "../writeavidmxf/test_${test}_v1.mxf")
        add_test(NAME libMXF_avidmxfinfo_${test}
            COMMAND ${command_prefix} $<TARGET_FILE:avidmxfinfo-bin> --test ${test_file}
        )
        set_tests_properties(libMXF_avidmxfinfo_${test} PROPERTIES
            FIXTURES_REQUIRED writeavidmxf_samples
        )
    endforeach()

    set(tests_film23.976
        DNxHD1080p1253 DNxHD1080p1235 DNxHD1080p1237 DNxHD1080p1238
    )

    foreach(test ${tests_film23.976})
        set(test_file "../writeavidmxf/test_${test}_film23.976_v1.mxf")
        add_test(NAME libMXF_avidmxfinfo_${test}_film23.976
            COMMAND ${command_prefix} $<TARGET_FILE:avidmxfinfo-bin> --test ${test_file}
        )
        set_tests_properties(libMXF_avidmxfinfo_${test}_film23.976 PROPERTIES
            FIXTURES_REQUIRED writeavidmxf_samples
        )
    endforeach()
endif()
