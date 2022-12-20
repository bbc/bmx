# Test creating an MXF OP1a file with index table segments following the essence.
# Test with different partioning structures and index table segment repetition.

include("${TEST_SOURCE_DIR}/test_common.cmake")

if(TEST_MODE STREQUAL "check")
    set(output_file_1 test_index_follows_1.mxf)
    set(output_file_2 test_index_follows_2.mxf)
    set(output_file_3 test_index_follows_3.mxf)
    set(output_file_4 test_index_follows_4.mxf)
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file_1 ${BMX_TEST_SAMPLES_DIR}/test_index_follows_1.mxf)
    set(output_file_2 ${BMX_TEST_SAMPLES_DIR}/test_index_follows_2.mxf)
    set(output_file_3 ${BMX_TEST_SAMPLES_DIR}/test_index_follows_3.mxf)
    set(output_file_4 ${BMX_TEST_SAMPLES_DIR}/test_index_follows_4.mxf)
else()
    set(output_file_1 test_index_follows_1.mxf)
    set(output_file_2 test_index_follows_2.mxf)
    set(output_file_3 test_index_follows_3.mxf)
    set(output_file_4 test_index_follows_4.mxf)
endif()

set(create_test_audio ${CREATE_TEST_ESSENCE}
    -t 1
    -d 3
    audio_index_follows
)

set(create_test_video ${CREATE_TEST_ESSENCE}
    -t 7
    -d 3
    video_index_follows
)


set(tests
    "x"
    "--min-part"
    "--body-part"
    "--repeat-index"
)

list(LENGTH tests num_tests)
math(EXPR max_index "${num_tests}")

foreach(index RANGE 1 ${max_index})
    math(EXPR test_index "${index} - 1")
    list(GET tests ${test_index} test)

    if(${test} STREQUAL "x")
        set(add_opt "")
    else()
        set(add_opt ${test})
    endif()

    set(create_command ${RAW2BMX}
        --regtest
        -t op1a
        -f 25
        -o ${output_file_${index}}
        --index-follows
        ${add_opt}
        --avci100_1080i video_index_follows
        -q 16 --pcm audio_index_follows
    )

    set(read_command ${MXF2RAW}
        --read-ess
        ${output_file_${index}}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        "${create_test_video}"
        "${create_test_audio}"
        ""
        "${create_command}"
        ""
        ""
        "${read_command}"
        "${output_file_${index}}"
        "index_follows_${index}.md5"
        ""
        ""
    )
endforeach()
