if(EXISTS essence.dat)
else()
    # Create sample essence. Size is the largest input, unc1080i
    execute_process(COMMAND ${CREATE_COMMAND} 4147200 essence.dat)
endif()

if(${IS_FILM} STREQUAL "TRUE")
    set(film_opt --film23.976)
endif()

if(LIBMXF_TEST_WITH_VALGRIND)
    set(execute_command valgrind --leak-check=full -q --error-exitcode=1 ${TEST_COMMAND})
else()
    set(execute_command ${TEST_COMMAND})
endif()

execute_process(COMMAND ${execute_command}
    --prefix ${OUTPUT_PREFIX}
    ${film_opt}
    --${TEST} essence.dat
    --pcm essence.dat
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "writeavidmxf failed to write file with prefix '${OUTPUT_PREFIX}'")
endif()
