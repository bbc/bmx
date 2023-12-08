include("${CMAKE_CURRENT_SOURCE_DIR}/../testing.cmake")

setup_test_dir("wave_adm")


# Function to add dependencies on a test that provides the input file
function(set_test_input_dep test_name dep_test_name)
    # Test target
    set_tests_properties(bmx_wave_adm_${dep_test_name} PROPERTIES
        FIXTURES_SETUP bmx_wave_adm_${dep_test_name}_input
    )
    set_tests_properties(bmx_wave_adm_${test_name} PROPERTIES
        FIXTURES_REQUIRED bmx_wave_adm_${dep_test_name}_input
    )

    # Sample and data custom targets
    add_dependencies(bmx_wave_adm_${test_name}_samples bmx_wave_adm_${dep_test_name}_samples)
    add_dependencies(bmx_wave_adm_${test_name}_data bmx_wave_adm_${dep_test_name}_data)
endfunction()


# MXF output tests
foreach(test_num RANGE 1 10)
    set(args
        "${common_args}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test_mxf_${test_num}.cmake
    )
    setup_test("wave_adm" "bmx_wave_adm_mxf_${test_num}" "${args}")
endforeach()

# Wave output tests
foreach(test_num RANGE 1 12)
    set(args
        "${common_args}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test_wave_${test_num}.cmake
    )
    setup_test("wave_adm" "bmx_wave_adm_wave_${test_num}" "${args}")
endforeach()

# Add dependencies for tests relying on the output of another test
set_test_input_dep(mxf_4 wave_3)
set_test_input_dep(mxf_7 wave_7)
set_test_input_dep(mxf_8 wave_3)
set_test_input_dep(mxf_9 wave_3)
set_test_input_dep(wave_1 mxf_1)
set_test_input_dep(wave_2 mxf_1)
set_test_input_dep(wave_3 mxf_3)
set_test_input_dep(wave_4 mxf_3)
set_test_input_dep(wave_8 mxf_7)
set_test_input_dep(wave_9 mxf_8)
set_test_input_dep(wave_10 mxf_9)
set_test_input_dep(wave_11 mxf_1)
set_test_input_dep(wave_12 mxf_1)
