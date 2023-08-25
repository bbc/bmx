if(uuid_link_lib)
    return()
endif()


if(UNIX AND NOT APPLE)
    find_library(uuid_lib
        NAMES uuid
    )
    find_path(uuid_include_dir
        NAMES uuid/uuid.h
    )

    if(NOT uuid_lib OR NOT uuid_include_dir)
        message(FATAL_ERROR "uuid dependency not found")
    endif()

    add_library(libuuid UNKNOWN IMPORTED)
    set_target_properties(libuuid PROPERTIES
        IMPORTED_LOCATION ${uuid_lib}
        INTERFACE_INCLUDE_DIRECTORIES ${uuid_include_dir}
    )
    set(uuid_link_lib libuuid)
else()
    # MSVC: "ole" will already be linked in
    # APPLE: doesn't require uuid library
    set(uuid_link_lib)
endif()
