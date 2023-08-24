add_library(writeavidmxf
    package_definitions.c
    write_avid_mxf.c
)

set(writeavidmxf_headers
    write_avid_mxf.h
    package_definitions.h
)

target_include_directories(writeavidmxf PRIVATE
    ${PROJECT_BINARY_DIR}
)
target_compile_definitions(writeavidmxf PRIVATE
    HAVE_CONFIG_H=1
)

target_link_libraries(writeavidmxf PUBLIC
    MXF
)

set_target_properties(writeavidmxf PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(writeavidmxf "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

# Only install if build is enabled, and not only for testing avidmxfinfo & reader
if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_WRITEAVIDMXF)
    install(TARGETS writeavidmxf
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
    )
    install(FILES ${writeavidmxf_headers} DESTINATION include/mxf/examples/writeavidmxf)
endif()


add_executable(writeavidmxf-bin
    main.c
)
target_include_directories(writeavidmxf-bin PUBLIC
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(writeavidmxf-bin
    writeavidmxf
    MXF
)
set_target_properties(writeavidmxf-bin PROPERTIES
    OUTPUT_NAME writeavidmxf
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(writeavidmxf-bin "${CMAKE_CURRENT_LIST_DIR}" "libMXF")


# Only install if build is enabled, and not only for testing avidmxfinfo & reader
if(LIBMXF_BUILD_EXAMPLES OR LIBMXF_BUILD_WRITEAVIDMXF)
    install(TARGETS writeavidmxf-bin DESTINATION bin)
endif()


if(LIBMXF_BUILD_TESTING AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
    add_executable(writeavidmxf_create_test_essence
        create_test_essence.c
    )

    include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
    set_source_filename(writeavidmxf_create_test_essence "${CMAKE_CURRENT_LIST_DIR}" "libMXF")

    set(tests_25
        IMX30 IMX40 IMX50
        DNxHD1080p1235 DNxHD1080p1237 DNxHD1080p1238
        DNxHD1080i1241 DNxHD1080i1242 DNxHD1080i1243
        DNxHD720p1250 DNxHD720p1251 DNxHD720p1252
        DNxHD1080p1253
        unc unc1080i unc720p
    )

    foreach(test ${tests_25})
        add_test(NAME libMXF_writeavidmxf_${test}
            COMMAND ${CMAKE_COMMAND}
                -D CREATE_COMMAND=$<TARGET_FILE:writeavidmxf_create_test_essence>
                -D TEST_COMMAND=$<TARGET_FILE:writeavidmxf-bin>
                -D OUTPUT_PREFIX=test_${test}
                -D IS_FILM=FALSE
                -D TEST=${test}
                -D LIBMXF_TEST_WITH_VALGRIND=${LIBMXF_TEST_WITH_VALGRIND}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/test.cmake
        )
        set_tests_properties(libMXF_writeavidmxf_${test} PROPERTIES
            FIXTURES_SETUP writeavidmxf_samples
        )
    endforeach()

    set(tests_film23.976
        DNxHD1080p1253 DNxHD1080p1235 DNxHD1080p1237 DNxHD1080p1238
    )

    foreach(test ${tests_film23.976})
        add_test(NAME libMXF_writeavidmxf_film23.976_${test}
            COMMAND ${CMAKE_COMMAND}
                -D CREATE_COMMAND=$<TARGET_FILE:writeavidmxf_create_test_essence>
                -D TEST_COMMAND=$<TARGET_FILE:writeavidmxf-bin>
                -D OUTPUT_PREFIX=test_${test}_film23.976
                -D IS_FILM=TRUE
                -D TEST=${test}
                -D LIBMXF_TEST_WITH_VALGRIND=${LIBMXF_TEST_WITH_VALGRIND}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/test.cmake
        )
        set_tests_properties(libMXF_writeavidmxf_film23.976_${test} PROPERTIES
            FIXTURES_SETUP writeavidmxf_samples
        )
    endforeach()
endif()
