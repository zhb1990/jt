add_executable(jt_tests behavior.cpp)
target_link_libraries(jt_tests PRIVATE jt::jt)
add_test(NAME jt.behavior COMMAND jt_tests)
set_tests_properties(jt.behavior PROPERTIES TIMEOUT 30)

add_executable(jt_file_tests files.cpp)
target_link_libraries(jt_file_tests PRIVATE jt::jt lz4::lz4)
add_test(NAME jt.files COMMAND jt_file_tests)
set_tests_properties(jt.files PROPERTIES TIMEOUT 30)

jt_configure_executable(jt_tests)
jt_configure_executable(jt_file_tests)

# Compile each public named module in its own consumer translation unit.
get_target_property(public_modules libjt CXX_MODULE_SET_jt_public_modules)
foreach(source IN LISTS public_modules)
    file(READ "${source}" contents)
    string(REGEX MATCH "export module ([a-zA-Z0-9_.]+);" declaration "${contents}")
    if(NOT declaration)
        continue() # Partitions are reached through their primary interface.
    endif()
    set(module "${CMAKE_MATCH_1}")
    string(REPLACE "." "_" target "consume_${module}")
    set(consumer "${CMAKE_CURRENT_BINARY_DIR}/${target}.cpp")
    file(CONFIGURE OUTPUT "${consumer}" CONTENT
        "import std;\nimport @module@;\nint main() { return 0; }\n" @ONLY)
    add_executable(${target} "${consumer}")
    target_link_libraries(${target} PRIVATE jt::jt)
    jt_configure_executable(${target})
    add_test(NAME "modules.${module}" COMMAND ${target})
endforeach()

add_executable(jt_base_consumer base_consumer.cpp)
target_link_libraries(jt_base_consumer PRIVATE jt::jt)
jt_configure_executable(jt_base_consumer)
add_test(NAME modules.base_api COMMAND jt_base_consumer)

# Negative consumers must fail for the named module, not an unrelated build error.
foreach(module IN ITEMS jt.detail.os jt.detail.intrusive_mpsc_queue jt.detail)
    string(REPLACE "." "_" target "reject_${module}")
    set(consumer "${CMAKE_CURRENT_BINARY_DIR}/${target}.cpp")
    file(CONFIGURE OUTPUT "${consumer}" CONTENT
        "import @module@;\nint main() {}\n" @ONLY)
    add_executable(${target} EXCLUDE_FROM_ALL "${consumer}")
    target_link_libraries(${target} PRIVATE jt::jt)
    add_test(NAME "modules.reject.${module}" COMMAND "${CMAKE_COMMAND}"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}" "-DTARGET=${target}" "-DMODULE=${module}"
        "-DCONFIG=$<CONFIG>" -P "${CMAKE_CURRENT_SOURCE_DIR}/reject_import.cmake")
    set_tests_properties("modules.reject.${module}" PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
endforeach()
