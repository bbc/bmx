# Run a command
function(run_command working_dir)
    list(SUBLIST ARGV 1 -1 cmd)

    execute_process(COMMAND ${cmd}
        WORKING_DIRECTORY ${working_dir}
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to execute ${cmd}: ${ret}")
    endif()
endfunction()

# Run a command and set the given variable to the command output
function(get_command_output working_dir output_var)
    list(SUBLIST ARGV 2 -1 cmd)

    execute_process(COMMAND ${cmd}
        WORKING_DIRECTORY ${working_dir}
        OUTPUT_VARIABLE out
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE ret
    )
    if(NOT ret EQUAL 0)
        message(FATAL_ERROR "Failed to execute ${cmd}: ${ret}")
    endif()

    set(${output_var} "${out}" PARENT_SCOPE)
endfunction()

# Extract the project version from a CMakeLists.txt
# The unique line is assumed to be "^ +VERSION +(<value>)$", where <value> is the version
function(extract_version lists_path output_var)
    file(STRINGS ${lists_path} data LIMIT_COUNT 100)
    foreach(line ${data})
        string(REGEX MATCH "^ +VERSION +.*$" result "${line}")
        if(result)
            string(REGEX REPLACE "^ +VERSION +(.*)$" "\\1" result "${result}")
            set(version ${result})
            break()
        endif()
    endforeach()

    if(NOT version)
        message(FATAL_ERROR "Failed to extract version string from ${lists_path}")
    endif()

    set(${output_var} ${version} PARENT_SCOPE)
endfunction()
