if(libcurl_link_lib)
    return()
endif()


include(FindCURL)

if(NOT CURL_FOUND)
    message(FATAL_ERROR "curl dependency not found")
endif()

set(libcurl_link_lib CURL::libcurl)
set(HAVE_LIBCURL 1)
