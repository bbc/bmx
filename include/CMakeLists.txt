add_subdirectory(bmx)

foreach(include_file ${bmx_headers})
    get_filename_component(dir ${include_file} DIRECTORY)
    install(FILES ${include_file} DESTINATION "include/${dir}")
endforeach()
