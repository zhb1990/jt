if(WIN32)
    # Ninja runs POST_BUILD in the link edge. Serialize Windows link/deploy
    # edges so our copies cannot race with each other or vcpkg's applocal step
    # in shared output directories. Compilation remains parallel.
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS jt_windows_deploy=1)
endif()

function(jt_configure_library)
    if(WIN32)
        set_property(TARGET libjt PROPERTY JOB_POOL_LINK jt_windows_deploy)
    endif()
    if(WIN32 OR CYGWIN)
        # BMIs bake in this definition. Consumers must not redefine JT_API.
        target_compile_definitions(libjt PRIVATE JT_DLL_EXPORT)
    else()
        target_compile_definitions(libjt PRIVATE JT_LIB_VISIBILITY)
        set_target_properties(libjt PROPERTIES
            CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
    endif()
    if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_libraries(libjt PUBLIC stdc++exp)
    endif()
    target_compile_options(libjt PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endfunction()

function(jt_configure_executable target)
    if(WIN32)
        set_property(TARGET ${target} PROPERTY JOB_POOL_LINK jt_windows_deploy)
        # Also copy libjt's private DLL dependencies into consumer directories.
        set(runtime_dlls
            "$<REMOVE_DUPLICATES:$<TARGET_RUNTIME_DLLS:${target}>;$<TARGET_RUNTIME_DLLS:libjt>>")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dlls}" "$<TARGET_FILE_DIR:${target}>"
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
