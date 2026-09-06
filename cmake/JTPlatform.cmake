function(jt_configure_library)
    if(WIN32 OR CYGWIN)
        # BMIs bake in this definition. Consumers must not redefine JT_API.
        target_compile_definitions(libjt PRIVATE JT_DLL_EXPORT)
    else()
        target_compile_definitions(libjt PRIVATE JT_LIB_VISIBILITY)
        set_target_properties(libjt PROPERTIES
            CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
    endif()
    if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(libjt PRIVATE "LINKER:--allow-multiple-definition")
        target_link_libraries(libjt PUBLIC stdc++exp)
    endif()
    target_compile_options(libjt PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endfunction()

function(jt_configure_executable target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t "$<TARGET_FILE_DIR:${target}>"
                $<TARGET_RUNTIME_DLLS:${target}>
            COMMAND_EXPAND_LISTS VERBATIM)
    endif()
    if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_filename_component(mingw_bindir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${mingw_bindir}/libstdc++-6.dll"
                "${mingw_bindir}/libgcc_s_seh-1.dll"
                "${mingw_bindir}/libwinpthread-1.dll"
                "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endif()
endfunction()
