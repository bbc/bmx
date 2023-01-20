if(TARGET bmx_cmake_git_version_tracking)
    return()
endif()


# Set the git release tag pattern to describe relative to
set(GIT_DESCRIBE_TAG_PATTERN "v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(GIT_WORKING_DIR ${PROJECT_SOURCE_DIR})

# Set to ignore git_version errors if there is no .git directory
set(GIT_FAIL_IF_NONZERO_EXIT FALSE)

include(FetchContent)

set(git_version_deps_source "${PROJECT_SOURCE_DIR}/deps/cmake-git-version-tracking")

if(EXISTS ${git_version_deps_source})
    FetchContent_Declare(bmx_cmake_git_version_tracking
        SOURCE_DIR ${git_version_deps_source}
    )
    message("-- Build using git version source: ${git_version_deps_source}")
else()
    FetchContent_Declare(bmx_cmake_git_version_tracking
        GIT_REPOSITORY "https://github.com/andrew-hardin/cmake-git-version-tracking.git"
        GIT_TAG "904dbda1336ba4b9a1415a68d5f203f576b696bb"
        PATCH_COMMAND git reset --hard
        COMMAND git apply --ignore-whitespace "${CMAKE_CURRENT_LIST_DIR}/git_version_904dbda.patch"
    )
endif()

FetchContent_GetProperties(bmx_cmake_git_version_tracking)
if(NOT bmx_cmake_git_version_tracking_POPULATED)
    FetchContent_Populate(bmx_cmake_git_version_tracking)
    add_subdirectory(${bmx_cmake_git_version_tracking_SOURCE_DIR} ${bmx_cmake_git_version_tracking_BINARY_DIR})
endif()
