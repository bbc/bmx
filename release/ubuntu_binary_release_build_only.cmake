# Build bmx on Ubuntu using the source release.

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(NOT DEFINED BMX_BRANCH)
    set(BMX_BRANCH main)
endif()
if(NOT DEFINED USE_GIT_CLONE)
    set(USE_GIT_CLONE OFF)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)


function(copy_and_rename_file source dest)
    get_filename_component(dest_dir ${dest} DIRECTORY)
    get_filename_component(source_name ${source} NAME)
    file(COPY ${source} DESTINATION ${dest_dir})
    file(RENAME ${dest_dir}/${source_name} ${dest})
endfunction()


if(USE_GIT_CLONE)
    # Clone bmx
    set(bmx_dir "${CMAKE_CURRENT_BINARY_DIR}/bmx")
    if(EXISTS ${bmx_dir})
        message(FATAL_ERROR "Can't continue with clean release as 'bmx' directory already exists")
    endif()
    run_command("${CMAKE_CURRENT_BINARY_DIR}" git clone https://github.com/bbc/bmx.git)
    run_command("${bmx_dir}" git checkout ${BMX_BRANCH})
else()
    get_filename_component(bmx_dir "${CMAKE_CURRENT_LIST_DIR}/.." REALPATH)
    if(EXISTS ${bmx_dir}/out)
        message(FATAL_ERROR "Can't continue with clean release as 'out' sub-directory already exists")
    endif()
endif()

# Create build, install and package directories
set(build_dir "${bmx_dir}/out/build")
file(MAKE_DIRECTORY ${build_dir})
set(install_dir "${bmx_dir}/out/install")
file(MAKE_DIRECTORY ${install_dir})

extract_version("${bmx_dir}/CMakeLists.txt" bmx_version)
set(package_dir "${bmx_dir}/out/package")
set(bmx_package_dir "${package_dir}/bmx-ubuntu-binary-${bmx_version}")
file(MAKE_DIRECTORY ${bmx_package_dir})

# Configure, build, test and install
run_command("${build_dir}" cmake -DCMAKE_INSTALL_PREFIX=../install -DBMX_BUILD_URIPARSER_SOURCE=ON -DBMX_BUILD_EXPAT_SOURCE=ON -DBMX_BUILD_WITH_LIBCURL=ON -DCMAKE_BUILD_TYPE=Release ../../)
run_command("${build_dir}" cmake --build .)
run_command("${build_dir}" ctest --output-on-failure)
run_command("${build_dir}" cmake --build . --target install)
