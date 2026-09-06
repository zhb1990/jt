# All domains contribute to one target; visibility is explicit at registration.
function(jt_modules visibility)
    set(files)
    foreach(file IN LISTS ARGN)
        get_filename_component(absolute "${file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        list(APPEND files "${absolute}")
    endforeach()
    if(visibility STREQUAL "PUBLIC")
        set(file_set jt_public_modules)
        if(NOT WIN32 AND NOT CYGWIN)
            # GCC consumers require exported named-module initializers (_ZGIW*).
            set_property(SOURCE ${files} TARGET_DIRECTORY libjt APPEND PROPERTY
                COMPILE_OPTIONS -fvisibility=default)
        endif()
    elseif(visibility STREQUAL "PRIVATE")
        set(file_set jt_private_modules)
    else()
        message(FATAL_ERROR "Invalid module visibility: ${visibility}")
    endif()
    target_sources(libjt ${visibility}
        FILE_SET ${file_set} TYPE CXX_MODULES
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
        FILES ${files})
endfunction()
