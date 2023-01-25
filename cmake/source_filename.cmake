# Adds the __FILENAME__=filename compile definition.
# Based on https://stackoverflow.com/a/27990434, but changed to define __FILENAME__ to be a relative project path
function(set_source_filename target sources_base_dir project_dirname)
    get_target_property(source_files "${target}" SOURCES)
    foreach(source_file ${source_files})
        file(RELATIVE_PATH rel_path ${PROJECT_SOURCE_DIR} "${sources_base_dir}/${source_file}")
        set_property(
            SOURCE "${source_file}" APPEND
            PROPERTY COMPILE_DEFINITIONS "__FILENAME__=\"${project_dirname}/${rel_path}\"")
    endforeach()
endfunction()
