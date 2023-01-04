# Test creating an MXF file with audio tracks labelled.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})
endif()


foreach(num RANGE 7)
    execute_process(COMMAND ${CREATE_TEST_ESSENCE}
        -t 42
        -d 3
        -s ${num}
        "audio_mcalabels_${num}"
        OUTPUT_QUIET
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to create test audio: ${ret}")
    endif()
endforeach()

execute_process(COMMAND ${CREATE_TEST_ESSENCE}
    -t 7
    -d 3
    video_mcalabels
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create test video: ${ret}")
endif()


# '\\\;' escape is needed to avoid 2 times list interpretation of ';'
set(tests
    stereo stereo as11
    mono mono as11
    stereop51 "0,1\\\;2-7" as11
    mess "0,1\\\;2-7" as11
    mononochan mono as11
    imf "0,1\\\;2-7" any
    mixed "0,1\\\;2-7" any
    properties "0,1\\\;2-7" any
)

list(LENGTH tests len_tests)
math(EXPR max_index "(${len_tests} / 3) - 1")

foreach(index RANGE ${max_index})
    math(EXPR test_name_index "${index} * 3")
    list(GET tests ${test_name_index} test_name)

    math(EXPR track_map_index "${index} * 3 + 1")
    list(GET tests ${track_map_index} track_map)

    math(EXPR scheme_index "${index} * 3 + 2")
    list(GET tests ${scheme_index} scheme)

    if(TEST_MODE STREQUAL "check")
        set(output_file_raw2bmx mcalabels_${test_name}_raw2bmx.mxf)
        set(output_file_bmxtranswrap mcalabels_${test_name}_bmxtranswrap.mxf)
        set(output_info_file_raw2bmx mcalabels_${test_name}_raw2bmx.xml)
        set(output_info_file_bmxtranswrap mcalabels_${test_name}_bmxtranswrap.xml)
    elseif(TEST_MODE STREQUAL "samples")
        set(output_file_raw2bmx ${BMX_TEST_SAMPLES_DIR}/mcalabels_${test_name}_raw2bmx.mxf)
        set(output_file_bmxtranswrap ${BMX_TEST_SAMPLES_DIR}/mcalabels_${test_name}_bmxtranswrap.mxf)
        set(output_info_file_raw2bmx ${BMX_TEST_SAMPLES_DIR}/mcalabels_${test_name}_raw2bmx.xml.bin)
        set(output_info_file_bmxtranswrap ${BMX_TEST_SAMPLES_DIR}/mcalabels_${test_name}_bmxtranswrap.xml.bin)
    else()
        set(output_file_raw2bmx mcalabels_${test_name}_raw2bmx.mxf)
        set(output_file_bmxtranswrap mcalabels_${test_name}_bmxtranswrap.mxf)
        set(output_info_file_raw2bmx mcalabels_${test_name}_raw2bmx.xml)
        set(output_info_file_bmxtranswrap ${TEST_SOURCE_DIR}/mcalabels_${test_name}.xml.bin)
    endif()

    set(create_command_1 ${RAW2BMX}
        --regtest
        -t wave
        -o pass1_mcalabels.wav
        -q 24 --locked true --pcm audio_mcalabels_0
        -q 24 --locked true --pcm audio_mcalabels_1
        -q 24 --locked true --pcm audio_mcalabels_2
        -q 24 --locked true --pcm audio_mcalabels_3
        -q 24 --locked true --pcm audio_mcalabels_4
        -q 24 --locked true --pcm audio_mcalabels_5
    )

    set(create_command_2 ${RAW2BMX}
        --regtest
        -t op1a
        -f 25
        -o ${output_file_raw2bmx}
        --track-map "${track_map}"
        --track-mca-labels ${scheme} "${TEST_SOURCE_DIR}/${test_name}.txt"
        --audio-layout as11-mode-0
        --avci100_1080i video_mcalabels
        --locked true --wave pass1_mcalabels.wav
        -q 24 --locked true --pcm audio_mcalabels_6
        -q 24 --locked true --pcm audio_mcalabels_7
    )

    set(read_command ${MXF2RAW}
        --regtest
        --info
        --info-format xml
        --info-file ${output_info_file_raw2bmx}
        --mca-detail
        ${output_file_raw2bmx}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        ""
        ""
        ""
        "${create_command_1}"
        "${create_command_2}"
        ""
        "${read_command}"
        "${output_file_raw2bmx}"
        "mcalabels_${test_name}.md5"
        "${output_info_file_raw2bmx};mcalabels_${test_name}.xml.bin"
        ""
    )


    set(create_command_1 ${RAW2BMX}
        --regtest
        -t op1a
        -f 25
        -o pass1_mcalabels.mxf
        --avci100_1080i video_mcalabels
        -q 24 --locked true --pcm audio_mcalabels_0
        -q 24 --locked true --pcm audio_mcalabels_1
        -q 24 --locked true --pcm audio_mcalabels_2
        -q 24 --locked true --pcm audio_mcalabels_3
        -q 24 --locked true --pcm audio_mcalabels_4
        -q 24 --locked true --pcm audio_mcalabels_5
        -q 24 --locked true --pcm audio_mcalabels_6
        -q 24 --locked true --pcm audio_mcalabels_7
    )

    set(create_command_2 ${BMXTRANSWRAP}
        --regtest
        -t op1a
        -o ${output_file_bmxtranswrap}
        --track-map "${track_map}"
        --track-mca-labels ${scheme} "${TEST_SOURCE_DIR}/${test_name}.txt"
        --audio-layout as11-mode-0
        pass1_mcalabels.mxf
    )

    set(read_command ${MXF2RAW}
        --regtest
        --info
        --info-format xml
        --info-file ${output_info_file_bmxtranswrap}
        --mca-detail
        ${output_file_bmxtranswrap}
    )

    run_test_a(
        "${TEST_MODE}"
        "${BMX_TEST_WITH_VALGRIND}"
        ""
        ""
        ""
        "${create_command_1}"
        "${create_command_2}"
        ""
        "${read_command}"
        "${output_file_bmxtranswrap}"
        "mcalabels_${test_name}.md5"
        "${output_info_file_bmxtranswrap};mcalabels_${test_name}.xml.bin"
        ""
    )
endforeach()
