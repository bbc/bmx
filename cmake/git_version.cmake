if(TARGET cmake_git_version_tracking)
    return()
endif()


# Set to ignore git_version errors if there is no .git directory
set(GIT_FAIL_IF_NONZERO_EXIT FALSE)

include(FetchContent)

set(git_version_deps_source "${PROJECT_SOURCE_DIR}/deps/cmake-git-version-tracking")

if(EXISTS ${git_version_deps_source})
    FetchContent_Declare(cmake_git_version_tracking
        SOURCE_DIR ${git_version_deps_source}
    )
    message("-- Build using git version source: ${git_version_deps_source}")
else()
    FetchContent_Declare(cmake_git_version_tracking
        GIT_REPOSITORY "https://github.com/andrew-hardin/cmake-git-version-tracking.git"
        GIT_TAG "904dbda1336ba4b9a1415a68d5f203f576b696bb"
    )
endif()

FetchContent_GetProperties(cmake_git_version_tracking)
if(NOT cmake_git_version_tracking_POPULATED)
    FetchContent_Populate(cmake_git_version_tracking)
    add_subdirectory(${cmake_git_version_tracking_SOURCE_DIR} ${cmake_git_version_tracking_BINARY_DIR})
endif()
