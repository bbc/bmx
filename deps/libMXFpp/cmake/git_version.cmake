if(TARGET libmxfpp_git_version)
    return()
endif()


# Ensure the cmake targets and code symbols are libMXFpp specific
set(GIT_VERSION_PROJECT_PREFIX "libmxfpp_")

# Set the git release tag pattern to describe relative to
set(GIT_DESCRIBE_TAG_PATTERN "v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(GIT_WORKING_DIR ${PROJECT_SOURCE_DIR})

# Set to ignore git_version errors if there is no .git directory
set(GIT_FAIL_IF_NONZERO_EXIT FALSE)

include(FetchContent)

set(git_version_deps_source "${PROJECT_SOURCE_DIR}/deps/cmake-git-version-tracking")

if(EXISTS ${git_version_deps_source})
    FetchContent_Declare(libmxfpp_git_version
        SOURCE_DIR ${git_version_deps_source}
    )
    message("-- Build using git version source: ${git_version_deps_source}")
else()
    FetchContent_Declare(libmxfpp_git_version
        GIT_REPOSITORY "https://github.com/andrew-hardin/cmake-git-version-tracking.git"
        GIT_TAG "904dbda1336ba4b9a1415a68d5f203f576b696bb"
        PATCH_COMMAND git clean -fdx
        COMMAND git reset --hard
        COMMAND git apply --ignore-whitespace "${CMAKE_CURRENT_LIST_DIR}/git_version_904dbda.patch"
    )
endif()

FetchContent_GetProperties(libmxfpp_git_version)
if(NOT libmxfpp_git_version_POPULATED)
    FetchContent_Populate(libmxfpp_git_version)
    add_subdirectory(${libmxfpp_git_version_SOURCE_DIR} ${libmxfpp_git_version_BINARY_DIR})
endif()
