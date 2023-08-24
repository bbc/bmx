if(LIBMXFPP_TEST_WITH_VALGRIND)
    set(command_prefix valgrind --leak-check=full -q --error-exitcode=1)
else()
    set(command_prefix)
endif()

add_executable(simple
    simple.cpp
)

target_link_libraries(simple
    MXFpp
    ${MXF_link_lib}
)

include("${PROJECT_SOURCE_DIR}/cmake/source_filename.cmake")
set_source_filename(simple "${CMAKE_CURRENT_LIST_DIR}" "libMXF++")

add_test(NAME libMXFpp_simple_test
    COMMAND ${command_prefix} $<TARGET_FILE:simple>
)
