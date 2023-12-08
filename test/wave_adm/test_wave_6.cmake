# Create a Wave file from test generated content,
# an axml chunk file and a chna text file.

set(test_name "wave_6")
include("${TEST_SOURCE_DIR}/test_common.cmake")


foreach(chan RANGE 1 4)
    set(create_command ${CREATE_TEST_ESSENCE}
        -t 42
        -d 1
        -s ${chan}
        audio_${chan}_${test_name}
    )

    execute_process(COMMAND ${create_command}
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to create test audio: ${ret}")
    endif()
endforeach()

set(create_command ${RAW2BMX}
    --regtest
    -t wave
    -o ${output_file}
    --wave-chunk-data ${TEST_SOURCE_DIR}/axml_1.xml.bin axml
    --chna-audio-ids ${TEST_SOURCE_DIR}/chna_1.txt
    -s 48000 -q 24 --pcm audio_1_${test_name}
    -s 48000 -q 24 --pcm audio_2_${test_name}
    -s 48000 -q 24 --pcm audio_3_${test_name}
    -s 48000 -q 24 --pcm audio_4_${test_name}
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
    "test_${test_name}.md5"
    ""
    ""
)
