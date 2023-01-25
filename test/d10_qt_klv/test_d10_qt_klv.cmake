# Test creating D10 MXF from input that has the D10 essence wrapped in KLV
# Perform the following checks:
# 1) wrapping from the raw essence extracted from sample.mxf containing the D-10 KL prefix
# 2) wrapping from the raw essence extracted from 1) produces the same result as 1)
# 3) transwrapping from the sample.mxf containing the D-10 KL prefix
# 4) transwrapping from the MXF from 3) produces the same result as 3)

include("${TEST_SOURCE_DIR}/../testing.cmake")


function(run_rewrap_test input_file output_file)
    execute_process(COMMAND ${MXF2RAW}
        -p input
        --deint
        ${input_file}
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to extract D10 KLV essence: ${ret}")
    endif()

    set(create_command ${RAW2BMX}
        --regtest-real
        -t d10
        -o ${output_file}
        --clip test
        -y 23:58:30:00
        --d10 input_v0.raw
        -q 24 --pcm input_a0_0.raw
        -q 24 --pcm input_a0_1.raw
        -q 24 --pcm input_a0_2.raw
        -q 24 --pcm input_a0_3.raw
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
        ""
        "${output_file}"
        "test.md5"
        ""
        ""
    )
endfunction()

function(run_transwrap_test input_file output_file)
    set(create_command ${BMXTRANSWRAP}
        --regtest
        -t d10
        -o ${output_file}
        --clip test
        --assume-d10-50
        ${input_file}
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
        ""
        "${output_file}"
        "test.md5"
        ""
        ""
    )
endfunction()


if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})
endif()


if(TEST_MODE STREQUAL "check")
    set(output_file_rewrap test_rewrap.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_rewrap ${BMX_TEST_SAMPLES_DIR}/test_d10_qt_klv_rewrap.mxf)
else()
    set(output_file_rewrap test_rewrap.mxf)
endif()

run_rewrap_test("${TEST_SOURCE_DIR}/sample.mxf" ${output_file_rewrap})


if(TEST_MODE STREQUAL "check")
    set(output_file_rewrap_again test_rewrap_again.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_rewrap_again ${BMX_TEST_SAMPLES_DIR}/test_d10_qt_klv_rewrap_again.mxf)
else()
    set(output_file_rewrap_again test_rewrap_again.mxf)
endif()

run_rewrap_test(${output_file_rewrap} ${output_file_rewrap_again})


if(TEST_MODE STREQUAL "check")
    set(output_file_transwrap test_transwrap.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_transwrap ${BMX_TEST_SAMPLES_DIR}/test_d10_qt_klv_transwrap.mxf)
else()
    set(output_file_transwrap test_transwrap.mxf)
endif()

run_transwrap_test("${TEST_SOURCE_DIR}/sample.mxf" ${output_file_transwrap})


if(TEST_MODE STREQUAL "check")
    set(output_file_transwrap_again test_transwrap_again.mxf)
elseif(TEST_MODE STREQUAL "samples")
    set(output_file_transwrap_again ${BMX_TEST_SAMPLES_DIR}/test_d10_qt_klv_transwrap_again.mxf)
else()
    set(output_file_transwrap_again test_transwrap_again.mxf)
endif()

run_transwrap_test(${output_file_transwrap} ${output_file_transwrap_again})
