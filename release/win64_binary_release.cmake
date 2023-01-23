# Create a Windows 64-bit binary release of bmx.

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
set(bmx_package_dir "${package_dir}/bmx-win64-binary-${bmx_version}")
file(MAKE_DIRECTORY ${bmx_package_dir})

# Configure, build, test and install
run_command("${build_dir}" cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=../install ../../)
run_command("${build_dir}" cmake --build . --config Release)
run_command("${build_dir}" ctest -C Release)
run_command("${build_dir}" cmake --build . --config Release --target install)

# Create the package
file(MAKE_DIRECTORY ${bmx_package_dir}/bin)
file(GLOB bin_files ${install_dir}/bin/*)
file(COPY ${bin_files} DESTINATION ${bmx_package_dir}/bin)

file(MAKE_DIRECTORY ${bmx_package_dir}/licenses)
file(GLOB license_files ${bmx_dir}/release/binary_licenses/LICENSE_*.txt)
file(COPY ${license_files} DESTINATION ${bmx_package_dir}/licenses)
if(EXISTS ${bmx_dir}/deps/cmake-git-version-tracking)
    copy_and_rename_file(${bmx_dir}/deps/cmake-git-version-tracking/LICENSE ${bmx_package_dir}/licenses/LICENSE_cmake_git_version_tracking.txt)
else()
    copy_and_rename_file(${build_dir}/_deps/bmx_git_version-src/LICENSE ${bmx_package_dir}/licenses/LICENSE_cmake_git_version_tracking.txt)
endif()
if(EXISTS ${bmx_dir}/deps/libexpat)
    copy_and_rename_file(${bmx_dir}/deps/libexpat/COPYING ${bmx_package_dir}/licenses/LICENSE_expat.txt)
else()
    copy_and_rename_file(${build_dir}/_deps/ft_libexpat-src/COPYING ${bmx_package_dir}/licenses/LICENSE_expat.txt)
endif()
if(EXISTS ${bmx_dir}/deps/uriparser)
    copy_and_rename_file(${bmx_dir}/deps/uriparser/COPYING ${bmx_package_dir}/licenses/LICENSE_uriparser.txt)
else()
    copy_and_rename_file(${build_dir}/_deps/liburiparser-src/COPYING ${bmx_package_dir}/licenses/LICENSE_uriparser.txt)
endif()

file(COPY ${bmx_dir}/COPYING DESTINATION ${bmx_package_dir})
file(COPY ${bmx_dir}/README.md DESTINATION ${bmx_package_dir})
file(COPY ${bmx_dir}/CHANGELOG.md DESTINATION ${bmx_package_dir})

file(MAKE_DIRECTORY ${bmx_package_dir}/docs)
file(GLOB docs_files ${bmx_dir}/docs/*)
file(COPY ${docs_files} DESTINATION ${bmx_package_dir}/docs)

run_command("${package_dir}" ${CMAKE_COMMAND} -E tar "cfv" bmx-win64-binary-${bmx_version}.zip --format=zip bmx-win64-binary-${bmx_version})
message("Created bmx-win64-binary-${bmx_version}.zip")
