# Create a source release of bmx, including dependencies.
#
# Unix-like build:
#    Add the '-DBMX_BUILD_EXPAT_SOURCE=ON' and '-DBMX_BUILD_URIPARSER_SOURCE=ON' cmake
#    configure option to make use the provided code. The default assumption is that these
#    libraries are installed on the system.
#

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(NOT DEFINED BMX_BRANCH)
    set(BMX_BRANCH main)
endif()
if(NOT DEFINED USE_GIT_CLONE)
    set(USE_GIT_CLONE OFF)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)


# Extract the git tag from a cmake file
# The unique line is assumed to be "^ +GIT_TAG +(<value>)$", where <value> is the tag
function(extract_ext_git_tag cmake_path output_var)
    file(STRINGS ${cmake_path} data LIMIT_COUNT 100)
    foreach(line ${data})
        string(REGEX MATCH "^ +GIT_TAG +.*$" result "${line}")
        if(result)
            string(REGEX REPLACE "^ +GIT_TAG +\"?([^\"]*)\"?$" "\\1" result "${result}")
            set(tag ${result})
            break()
        endif()
    endforeach()

    if(NOT tag)
        message(FATAL_ERROR "Failed to extract git tag from ${cmake_path}")
    endif()

    set(${output_var} ${tag} PARENT_SCOPE)
endfunction()

# Write the fallback_git_version.h with a define set to the "git describe" output
function(set_fallback_git_version working_dir version)
    get_command_output("${working_dir}" describe_str git describe --match "v${version}")

    set(output "\
// Set this define when building code from a source package without
// the .git directory
#define PACKAGE_GIT_VERSION_STRING \"${describe_str}\"
")
    file(WRITE ${working_dir}/fallback_git_version.h "${output}")
endfunction()

# Clone the external dependency and checkout with the given tag
function(download_external base name url tag)
    run_command("${base}" git clone ${url} ${name})
    run_command("${base}/${name}" git checkout -q ${tag})
endfunction()

function(download_git_version base)
    extract_ext_git_tag("${base}/cmake/git_version.cmake" git_version_tag)
    download_external("${base}/deps" cmake-git-version-tracking https://github.com/andrew-hardin/cmake-git-version-tracking.git ${git_version_tag})
    run_command("${base}/deps/cmake-git-version-tracking" git apply --ignore-whitespace ../../cmake/git_version_904dbda.patch)
endfunction()


if(USE_GIT_CLONE)
    # Clone bmx and update the submodules
    set(bmx_dir "${CMAKE_CURRENT_BINARY_DIR}/bmx")
    if(EXISTS ${bmx_dir})
        message(FATAL_ERROR "Can't continue with clean release as 'bmx' directory already exists")
    endif()
    run_command("${CMAKE_CURRENT_BINARY_DIR}" git clone https://github.com/bbc/bmx.git)
    run_command("${bmx_dir}" git checkout ${BMX_BRANCH})
    run_command("${bmx_dir}" git submodule update --init)
else()
    get_filename_component(bmx_dir "${CMAKE_CURRENT_LIST_DIR}/.." REALPATH)
endif()

# Get the project versions from CMakeLists.txt
extract_version("${bmx_dir}/CMakeLists.txt" bmx_version)
extract_version("${bmx_dir}/deps/libMXF/CMakeLists.txt" libmxf_version)
extract_version("${bmx_dir}/deps/libMXFpp/CMakeLists.txt" libmxfpp_version)

# Write git describe output to the fallback_git_version.h files in bmx, libMXF and libMXFpp
set_fallback_git_version("${bmx_dir}" ${bmx_version})
set_fallback_git_version("${bmx_dir}/deps/libMXF" ${libmxf_version})
set_fallback_git_version("${bmx_dir}/deps/libMXFpp" ${libmxfpp_version})

# Download the external dependencies into deps/
extract_ext_git_tag("${bmx_dir}/cmake/ext_expat.cmake" libexpat_tag)
download_external("${bmx_dir}/deps" libexpat https://github.com/libexpat/libexpat ${libexpat_tag})

extract_ext_git_tag("${bmx_dir}/cmake/ext_uriparser.cmake" uriparser_tag)
download_external("${bmx_dir}/deps" uriparser https://github.com/uriparser/uriparser ${uriparser_tag})

download_git_version("${bmx_dir}/")

file(MAKE_DIRECTORY ${bmx_dir}/deps/libMXF/deps)
download_git_version("${bmx_dir}/deps/libMXF")

file(MAKE_DIRECTORY ${bmx_dir}/deps/libMXFpp/deps)
download_git_version("${bmx_dir}/deps/libMXFpp")

# Remove the .git directories
foreach(name libMXF libMXFpp libexpat uriparser cmake-git-version-tracking)
    file(REMOVE_RECURSE ${bmx_dir}/deps/${name}/.git)
endforeach()
file(REMOVE_RECURSE ${bmx_dir}/.git)

# Add the version to the directory name and create packages
# Creating a copy because the GitHub Windows actions runner complained
# that the directory was in use by another process when trying to rename it.
file(MAKE_DIRECTORY ${bmx_dir}/../source_release)
file(COPY ${bmx_dir} DESTINATION ${bmx_dir}/../source_release)
file(RENAME ${bmx_dir}/../source_release/bmx ${bmx_dir}/../source_release/bmx-${bmx_version})

# Create packages
run_command(${bmx_dir}/../source_release ${CMAKE_COMMAND} -E tar "cf" bmx-${bmx_version}.zip --format=zip bmx-${bmx_version})
message("Created source_release/bmx-${bmx_version}.zip")
run_command(${bmx_dir}/../source_release ${CMAKE_COMMAND} -E tar "cz" bmx-${bmx_version}.tar.gz bmx-${bmx_version})
message("Created source_release/bmx-${bmx_version}.tar.gz")
