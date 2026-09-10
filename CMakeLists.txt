cmake_minimum_required(VERSION 4.3.0)

# Experimental gate for the supported CMake version; allow a toolchain override.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "CMake experimental import std gate")
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(jt LANGUAGES CXX)

include(cmake/JTToolchain.cmake)
include(cmake/JTModules.cmake)
include(cmake/JTPlatform.cmake)

add_library(libjt SHARED)
add_library(jt::jt ALIAS libjt)
target_compile_features(libjt PUBLIC cxx_std_23)
set_target_properties(libjt PROPERTIES PREFIX "")
jt_configure_library()
jt_modules(PUBLIC src/jt.cppm)
add_subdirectory(src/base)
add_subdirectory(src/detail)
add_subdirectory(src/log)

option(JT_BUILD_EXAMPLES "Build examples" ON)
option(JT_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(JT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
if(JT_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

include(cmake/JTClangdCompileCommands.cmake)
jt_configure_clangd_compile_commands()
