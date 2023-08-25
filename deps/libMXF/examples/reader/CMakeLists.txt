add_library(mxfreader
    mxf_essence_helper.c
    mxf_index_helper.c
    mxf_op1a_reader.c
    mxf_opatom_reader.c
    mxf_reader.c
)

set(mxfreader_headers
    mxf_reader.h
)

target_include_directories(mxfreader PRIVATE
    ${PROJECT_BINARY_DIR}
)
target_compile_definitions(mxfreader PRIVATE
    HAVE_CONFIG_H=1
)

target_link_libraries(mxfreader PUBLIC
    MXF
)

set_target_properties(mxfreader PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(mxfreader "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

install(TARGETS mxfreader
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES ${mxfreader_headers} DESTINATION include/mxf/examples/reader)


if(LIBMXF_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    add_executable(test_mxf_reader
        test_mxf_reader.c
    )
    target_link_libraries(test_mxf_reader
        mxfreader
        MXF
    )

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(test_mxf_reader "${CMAKE_CURRENT_LIST_DIR}" "libMXF")


    if(LIBMXF_TEST_WITH_VALGRIND)
        set(command_prefix valgrind --leak-check=full -q --error-exitcode=1)
    else()
        set(command_prefix)
    endif()

    set(tests_25
        IMX30 IMX40 IMX50
        DNxHD1080p1237 DNxHD1080p1238
        DNxHD1080i1242 DNxHD1080i1243
        DNxHD720p1251 DNxHD720p1252
        DNxHD1080p1253
        unc unc1080i unc720p
    )
    # Not supported: DNxHD1080i1241, DNxHD1080p1235, DNxHD720p1250

    foreach(test ${tests_25})
        set(test_file "../writeavidmxf/test_${test}_v1.mxf")
        add_test(NAME libMXF_reader_${test}
            COMMAND ${command_prefix} $<TARGET_FILE:test_mxf_reader> ${test_file} test.dat
        )
        # Requires output of tests in ../writeavidmxf
        set_tests_properties(libMXF_reader_${test} PROPERTIES
            FIXTURES_REQUIRED writeavidmxf_samples
        )
    endforeach()

    set(tests_film23.976
        DNxHD1080p1253 DNxHD1080p1237 DNxHD1080p1238
    )
    # Not supported: DNxHD1080p1235

    foreach(test ${tests_film23.976})
        set(test_file "../writeavidmxf/test_${test}_film23.976_v1.mxf")
        add_test(NAME libMXF_reader_${test}_film23.976
            COMMAND ${command_prefix} $<TARGET_FILE:test_mxf_reader> ${test_file} test.dat
        )
        # Requires output of tests in ../writeavidmxf
        set_tests_properties(libMXF_reader_${test}_film23.976 PROPERTIES
            FIXTURES_REQUIRED writeavidmxf_samples
        )
    endforeach()
endif()
