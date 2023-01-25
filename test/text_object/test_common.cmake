include("${TEST_SOURCE_DIR}/../testing.cmake")


if(TEST_MODE STREQUAL "check")
    set(output_file test_${test_name}.mxf)
    set(output_info_file info_${test_name}.xml)
    set(output_text_file_prefix text_${test_name})
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.mxf)
    set(output_info_file ${BMX_TEST_SAMPLES_DIR}/info_${test_name}.xml.bin)
    set(output_text_file_prefix text_${test_name})
else()
    set(output_file test_${test_name}.mxf)
    set(output_info_file ${TEST_SOURCE_DIR}/info_${test_name}.xml.bin)
    set(output_text_file_prefix text_${test_name})
endif()


function(set_create_command type video_opt)
    set(create_command ${RAW2BMX}
        --regtest
        -t ${type}
        -f 25
        -y 10:00:00:00
        -o ${output_file}
        --embed-xml ${TEST_SOURCE_DIR}/utf8.xml.bin
        --embed-xml ${TEST_SOURCE_DIR}/utf16be.xml.bin
        --embed-xml ${TEST_SOURCE_DIR}/utf16le.xml.bin
        --xml-scheme-id "0772e8bd-f9a1-4b80-a517-85fd71c85675"
        --xml-lang "de"
        --embed-xml ${TEST_SOURCE_DIR}/utf8.xml.bin
        --embed-xml ${TEST_SOURCE_DIR}/other.xml.bin
        --embed-xml ${TEST_SOURCE_DIR}/other.xml.bin
        --embed-xml ${TEST_SOURCE_DIR}/utf8_noprolog.xml.bin
        --${video_opt} video_${test_name}
        -q 24 --pcm audio_${test_name}
        -q 24 --pcm audio_${test_name}
        PARENT_SCOPE
    )
endfunction()

set(read_command ${MXF2RAW}
    --regtest
    --log-level 2
    --info
    --info-format xml
    --info-file ${output_info_file}
    --text-out ${output_text_file_prefix}
    ${output_file}
)
