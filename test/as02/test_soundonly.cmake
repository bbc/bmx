# Test creating an AS02 MXF file containing sound only.
# Create file using raw2bmx and then transwrap.

include("${TEST_SOURCE_DIR}/test_common.cmake")


if(TEST_MODE STREQUAL "check")
    file(MAKE_DIRECTORY test_sound_only_from_raw)
    file(MAKE_DIRECTORY test_sound_only_transwrap)

    set(output_dir_name_1 as02test1)
    set(output_file_prefix_1 test_sound_only_from_raw/${output_dir_name_1})
    set(output_dir_name_2 as02test2)
    set(output_file_prefix_2 test_sound_only_transwrap/${output_dir_name_2})
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR}/test_sound_only_from_raw)
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR}/test_sound_only_transwrap)

    set(output_dir_name_1 as02test1)
    set(output_file_prefix_1 ${BMX_TEST_SAMPLES_DIR}/test_sound_only_from_raw/${output_dir_name_1})
    set(output_dir_name_2 as02test2)
    set(output_file_prefix_2 ${BMX_TEST_SAMPLES_DIR}/test_sound_only_transwrap/${output_dir_name_2})
else()
    file(MAKE_DIRECTORY test_sound_only_from_raw)
    file(MAKE_DIRECTORY test_sound_only_transwrap)

    set(output_dir_name_1 as02test1)
    set(output_file_prefix_1 test_sound_only_from_raw/${output_dir_name_1})
    set(output_dir_name_2 as02test2)
    set(output_file_prefix_2 test_sound_only_transwrap/${output_dir_name_2})
endif()

if(TEST_MODE STREQUAL "check" AND BMX_TEST_WITH_VALGRIND)
    set(prefix_command valgrind --leak-check=full -q --error-exitcode=1)
else()
    set(prefix_command)
endif()

# Create the test audio
execute_process(COMMAND ${CREATE_TEST_ESSENCE}
    -t 57
    -d 5123
    audio_1
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create test audio essence: ${ret}")
endif()

execute_process(COMMAND ${CREATE_TEST_ESSENCE}
    -t 57
    -d 6123
    audio_2
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create test audio essence: ${ret}")
endif()


# Create the MXF file
execute_process(COMMAND ${prefix_command} ${RAW2BMX}
    --regtest
    -t as02
    -o ${output_file_prefix_1}
    -q 16 --pcm audio_1
    -q 16 --pcm audio_2
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create AS02 MXF file: ${ret}")
endif()

# Validate the checksums
if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
    process_checksum("sound_only_from_raw.md5s" ${output_dir_name_1} ${output_file_prefix_1} FALSE)
endif()


# Transwrap the MXF file
execute_process(COMMAND ${prefix_command} ${BMXTRANSWRAP}
    --regtest
    -t as02
    -o ${output_file_prefix_2}
    "${output_file_prefix_1}/${output_dir_name_1}.mxf"
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create AS02 MXF file: ${ret}")
endif()

# Validate the checksums
if(TEST_MODE STREQUAL "check" OR TEST_MODE STREQUAL "data")
    process_checksum("sound_only_transwrap.md5s" ${output_dir_name_2} ${output_file_prefix_2} FALSE)
endif()
