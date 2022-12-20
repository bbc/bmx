# Test creating IMF Track file with audio.
# First test create an IMF audio Track File.
# Second test transwraps the IMF audio Track File.
# Third test create an IMF audio Track File with audio labels.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})
endif()


function(set_raw2bmx_create_command output_file labels_opt)
    set(create_command ${RAW2BMX}
        --regtest
        -t imf
        -o ${output_file}
        --clip test
        ${labels_opt}
        --ref-image-edit-rate 30000/1001
        --ref-audio-align-level -20
        -q 24 --pcm audio0
        -q 24 --pcm audio1
        -q 24 --pcm audio2
        -q 24 --pcm audio3
        -q 24 --pcm audio4
        -q 24 --pcm audio5
        PARENT_SCOPE
    )
endfunction()


foreach(num RANGE 6)
    execute_process(COMMAND ${CREATE_TEST_ESSENCE}
        -t 42
        -d 3
        -s ${num}
        "audio${num}"
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to create test audio: ${ret}")
    endif()
endforeach()


if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_audio_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_audio_1.mxf)
else()
    set(output_file_1 test_audio_1.mxf)
endif()

set_raw2bmx_create_command(${output_file_1} "")

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    ""
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file_1}"
    "test_audio_1.md5"
    ""
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file test_audio_2.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_audio_2.mxf)
else()
    set(output_file test_audio_2.mxf)
endif()

set(create_command ${BMXTRANSWRAP}
    --regtest
    -t imf
    -o ${output_file}
    --clip test
    ${output_file_1}
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    ""
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_audio_2.md5"
    ""
    ""
)


if(TEST_MODE STREQUAL "check")
    set(output_file test_audio_with_labels_1.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_audio_with_labels_1.mxf)
else()
    set(output_file test_audio_with_labels_1.mxf)
endif()

set_raw2bmx_create_command(
    ${output_file}
    "--track-mca-labels;any;${TEST_SOURCE_DIR}/audio_labels.txt"
)

run_test_a(
    "${TEST_MODE}"
    "${BMX_TEST_WITH_VALGRIND}"
    ""
    ""
    ""
    "${create_command}"
    ""
    ""
    "${read_command}"
    "${output_file}"
    "test_audio_with_labels_1.md5"
    ""
    ""
)

