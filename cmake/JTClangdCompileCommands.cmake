# Rewrite a compilation database so clangd can scan GCC's import-std units.
# CMake emits two commands for bits/std.cc: an object compile without -std=,
# then a BMI compile with -std=. clangd uses the first match, so scanning
# std.cc fails unless the -std= command is preferred.
#
# Script mode:
#   cmake -D JT_CLANGD_COMPILE_COMMANDS_IN=<path>
#         -D JT_CLANGD_COMPILE_COMMANDS_OUT=<path>
#         -P cmake/JTClangdCompileCommands.cmake

function(jt_strip_gcc_clangd_flags command_var)
    set(jt_cdb_command "${${command_var}}")
    string(REGEX REPLACE " -fmodules-ts" "" jt_cdb_command "${jt_cdb_command}")
    string(REGEX REPLACE " -fmodule-only" "" jt_cdb_command "${jt_cdb_command}")
    string(REGEX REPLACE " -fmodule-mapper=[^ ]+" "" jt_cdb_command
        "${jt_cdb_command}")
    string(REGEX REPLACE " -fdeps-format=[^ ]+" "" jt_cdb_command
        "${jt_cdb_command}")
    set("${command_var}" "${jt_cdb_command}" PARENT_SCOPE)
endfunction()

function(jt_write_clangd_compile_commands input output)
    if(NOT EXISTS "${input}")
        message(FATAL_ERROR "Compilation database not found: ${input}")
    endif()

    file(READ "${input}" jt_cdb_raw)
    string(JSON jt_cdb_length ERROR_VARIABLE jt_cdb_error LENGTH "${jt_cdb_raw}")
    if(jt_cdb_error)
        message(FATAL_ERROR "Failed to parse ${input}: ${jt_cdb_error}")
    endif()

    set(jt_cdb_keys)
    if(jt_cdb_length GREATER 0)
        math(EXPR jt_cdb_last "${jt_cdb_length} - 1")
        foreach(jt_cdb_index RANGE ${jt_cdb_last})
            string(JSON jt_cdb_entry GET "${jt_cdb_raw}" ${jt_cdb_index})
            string(JSON jt_cdb_file GET "${jt_cdb_entry}" "file")
            string(JSON jt_cdb_command GET "${jt_cdb_entry}" "command")
            string(MD5 jt_cdb_key "${jt_cdb_file}")
            if(NOT DEFINED jt_cdb_seen_${jt_cdb_key})
                list(APPEND jt_cdb_keys "${jt_cdb_key}")
                set(jt_cdb_seen_${jt_cdb_key} TRUE)
                set(jt_cdb_has_std_${jt_cdb_key} FALSE)
                if(jt_cdb_command MATCHES " -std=")
                    set(jt_cdb_has_std_${jt_cdb_key} TRUE)
                endif()
                set(jt_cdb_entry_${jt_cdb_key} "${jt_cdb_entry}")
            elseif(NOT jt_cdb_has_std_${jt_cdb_key} AND jt_cdb_command MATCHES " -std=")
                set(jt_cdb_has_std_${jt_cdb_key} TRUE)
                set(jt_cdb_entry_${jt_cdb_key} "${jt_cdb_entry}")
            endif()
        endforeach()
    endif()

    set(jt_cdb_out "[]")
    set(jt_cdb_out_index 0)
    foreach(jt_cdb_key IN LISTS jt_cdb_keys)
        string(JSON jt_cdb_command GET "${jt_cdb_entry_${jt_cdb_key}}" "command")
        jt_strip_gcc_clangd_flags(jt_cdb_command)
        # string(JSON SET) requires a JSON value, not a raw command line.
        string(JSON jt_cdb_command_json STRING_ENCODE "${jt_cdb_command}")
        string(JSON jt_cdb_entry SET "${jt_cdb_entry_${jt_cdb_key}}" "command"
            "${jt_cdb_command_json}")
        string(JSON jt_cdb_out SET "${jt_cdb_out}" ${jt_cdb_out_index}
            "${jt_cdb_entry}")
        math(EXPR jt_cdb_out_index "${jt_cdb_out_index} + 1")
    endforeach()

    get_filename_component(jt_cdb_out_dir "${output}" DIRECTORY)
    file(MAKE_DIRECTORY "${jt_cdb_out_dir}")
    file(WRITE "${output}" "${jt_cdb_out}\n")
endfunction()

function(jt_configure_clangd_compile_commands)
    if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
        return()
    endif()

    set(jt_clangd_compile_commands "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(jt_clangd_compile_commands_stamp
        "${CMAKE_BINARY_DIR}/CMakeFiles/jt_clangd_compile_commands.stamp")
    add_custom_command(
        OUTPUT "${jt_clangd_compile_commands_stamp}"
        COMMAND "${CMAKE_COMMAND}"
            -D "JT_CLANGD_COMPILE_COMMANDS_IN=${jt_clangd_compile_commands}"
            -D "JT_CLANGD_COMPILE_COMMANDS_OUT=${jt_clangd_compile_commands}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${jt_clangd_compile_commands_stamp}"
        DEPENDS "${jt_clangd_compile_commands}"
            "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Rewriting compile_commands.json for clangd"
        VERBATIM)
    add_custom_target(jt_clangd_compile_commands ALL
        DEPENDS "${jt_clangd_compile_commands_stamp}")

    if(EXISTS "${jt_clangd_compile_commands}")
        file(READ "${jt_clangd_compile_commands}" jt_clangd_existing)
        string(JSON jt_clangd_existing_length ERROR_VARIABLE
            jt_clangd_existing_error LENGTH "${jt_clangd_existing}")
        if(jt_clangd_existing_error)
            message(WARNING
                "Skipping clangd rewrite; ${jt_clangd_compile_commands} is not valid JSON")
        else()
            jt_write_clangd_compile_commands(
                "${jt_clangd_compile_commands}"
                "${jt_clangd_compile_commands}")
        endif()
    endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
    if(NOT DEFINED JT_CLANGD_COMPILE_COMMANDS_IN
            OR NOT DEFINED JT_CLANGD_COMPILE_COMMANDS_OUT)
        message(FATAL_ERROR
            "Set JT_CLANGD_COMPILE_COMMANDS_IN and JT_CLANGD_COMPILE_COMMANDS_OUT")
    endif()
    jt_write_clangd_compile_commands(
        "${JT_CLANGD_COMPILE_COMMANDS_IN}"
        "${JT_CLANGD_COMPILE_COMMANDS_OUT}")
endif()
