include("${TEST_SOURCE_DIR}/../testing.cmake")

# If the test name starts with 'mxf' then the output file type is MXF, else Wave
string(FIND ${test_name} "mxf" find_type_result)
if(find_type_result EQUAL 0)
    set(suffix "mxf")
else()
    set(suffix "wav")
endif()

if(TEST_MODE STREQUAL "check")
    set(output_file test_${test_name}.${suffix})
elseif(TEST_MODE STREQUAL "samples")
    file(MAKE_DIRECTORY ${BMX_TEST_SAMPLES_DIR})

    set(output_file ${BMX_TEST_SAMPLES_DIR}/test_${test_name}.${suffix})
else()
    set(output_file test_${test_name}.${suffix})
endif()


# for --avci100_1080i
set(create_test_video_1 ${CREATE_TEST_ESSENCE}
    -t 7
    -d 1
    video_1_${test_name}
)
