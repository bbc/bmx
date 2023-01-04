# Test creating an MXF file with audio tracks mapped.

include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})
endif()


foreach(num RANGE 7)
    execute_process(COMMAND ${CREATE_TEST_ESSENCE}
        -t 42
        -d 3
        -s ${num}
        "audio_mca_${num}"
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
    video_mca
    OUTPUT_QUIET
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to create test video: ${ret}")
endif()


# '\\\;' escape is needed to avoid 2 times list interpretation of ';'
set(tests
    mono mono
    stereo stereo
    singlemca singlemca
    monorem mx
    stereoprem "0-1\\\;x"
    stereops2p4 "0-1\\\;s2,2-5"
    stereopstereopmonorem "0-1\\\;0-1\\\;mx"
    reorder_mono "7\\\;6\\\;5\\\;4\\\;3\\\;2\\\;1\\\;0"
    reorder_stereo "7,6\\\;5,4\\\;3,2\\\;1,0"
    mono_silence "m1-2,s2"
    stereo_silence "s2\\\;1-2"
)

list(LENGTH tests len_tests)
math(EXPR max_index "(${len_tests} / 2) - 1")

foreach(index RANGE ${max_index})
    math(EXPR test_name_index "${index} * 2")
    list(GET tests ${test_name_index} test_name)

    math(EXPR track_map_index "${index} * 2 + 1")
    list(GET tests ${track_map_index} track_map)

    if(TEST_MODE STREQUAL "check")
        set(output_file_raw2bmx mca_${test_name}_raw2bmx.mxf)
        set(output_file_bmxtranswrap mca_${test_name}_bmxtranswrap.mxf)
        set(output_info_file_raw2bmx mca_${test_name}_raw2bmx.xml)
        set(output_info_file_bmxtranswrap mca_${test_name}_bmxtranswrap.xml)
    elseif(TEST_MODE STREQUAL "samples")
        set(output_file_raw2bmx ${BMX_TEST_SAMPLES_DIR}/mca_${test_name}_raw2bmx.mxf)
        set(output_file_bmxtranswrap ${BMX_TEST_SAMPLES_DIR}/mca_${test_name}_bmxtranswrap.mxf)
        set(output_info_file_raw2bmx ${BMX_TEST_SAMPLES_DIR}/mca_${test_name}_raw2bmx.xml.bin)
        set(output_info_file_bmxtranswrap ${BMX_TEST_SAMPLES_DIR}/mca_${test_name}_bmxtranswrap.xml.bin)
    else()
        set(output_file_raw2bmx mca_${test_name}_raw2bmx.mxf)
        set(output_file_bmxtranswrap mca_${test_name}_bmxtranswrap.mxf)
        set(output_info_file_raw2bmx mca_${test_name}_raw2bmx.xml)
        set(output_info_file_bmxtranswrap ${TEST_SOURCE_DIR}/mca_${test_name}.xml.bin)
    endif()

    set(create_command_1 ${RAW2BMX}
        --regtest
        -t wave
        -o pass1_mca.wav
        -q 24 --locked true --pcm audio_mca_0
        -q 24 --locked true --pcm audio_mca_1
        -q 24 --locked true --pcm audio_mca_2
        -q 24 --locked true --pcm audio_mca_3
        -q 24 --locked true --pcm audio_mca_4
        -q 24 --locked true --pcm audio_mca_5
    )

    set(create_command_2 ${RAW2BMX}
        --regtest
        -t op1a
        -f 25
        -o ${output_file_raw2bmx}
        --track-map "${track_map}"
        --avci100_1080i video_mca
        --locked true --wave pass1_mca.wav
        -q 24 --locked true --pcm audio_mca_6
        -q 24 --locked true --pcm audio_mca_7
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
        "mca_${test_name}.md5"
        "${output_info_file_raw2bmx};mca_${test_name}.xml.bin"
        ""
    )


    set(create_command_1 ${RAW2BMX}
        --regtest
        -t op1a
        -f 25
        -o pass1_mca.mxf
        --avci100_1080i video_mca
        -q 24 --locked true --pcm audio_mca_0
        -q 24 --locked true --pcm audio_mca_1
        -q 24 --locked true --pcm audio_mca_2
        -q 24 --locked true --pcm audio_mca_3
        -q 24 --locked true --pcm audio_mca_4
        -q 24 --locked true --pcm audio_mca_5
        -q 24 --locked true --pcm audio_mca_6
        -q 24 --locked true --pcm audio_mca_7
    )

    set(create_command_2 ${BMXTRANSWRAP}
        --regtest
        -t op1a
        -o ${output_file_bmxtranswrap}
        --track-map "${track_map}"
        pass1_mca.mxf
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
        "mca_${test_name}.md5"
        "${output_info_file_bmxtranswrap};mca_${test_name}.xml.bin"
        ""
    )
endforeach()
