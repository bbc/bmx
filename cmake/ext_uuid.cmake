if(uuid_link_lib)
    return()
endif()


if(UNIX AND NOT APPLE)
    include(FindPkgConfig)

    pkg_search_module(PC_uuid uuid QUIET IMPORTED_TARGET)
    if(PC_uuid_FOUND)
        set(uuid_link_lib PkgConfig::PC_uuid)
    else()
        find_library(LIB_uuid uuid)
        if(NOT LIB_uuid)
            message(FATAL_ERROR "uuid dependency not found")
        endif()

        add_library(libuuid UNKNOWN IMPORTED)
        set_target_properties(libuuid PROPERTIES
            IMPORTED_LOCATION ${LIB_uuid}
        )
        set(uuid_link_lib libuuid)
    endif()
else()
    # MSVC: "ole" will already be linked in
    # APPLE: doesn't require uuid library
    set(uuid_link_lib)
endif()
