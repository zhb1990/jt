cmake_minimum_required(VERSION 4.1.2)

if(${CMAKE_SIZEOF_VOID_P} MATCHES "4")
    message(FATAL_ERROR "only support x64 toolchain")
endif()

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(jt)

file(GLOB JT_MODULES
    "src/detail/cache_line.cppm"
    "src/detail/cpu_pause.cppm"
    "src/detail/allocator.cppm"
    "src/detail/buffer.cppm"
    "src/jt.cppm"
)

file(GLOB JT_HEAEDERS
    "src/detail/config.h"
)

file(GLOB JT_SOURCES
    "src/detail/allocator.cpp"
)

add_library(libjt SHARED)
set_target_properties(libjt PROPERTIES PREFIX "")
target_sources(libjt
    PRIVATE ${JT_HEAEDERS} ${JT_SOURCES}
    PUBLIC FILE_SET CXX_MODULES FILES ${JT_MODULES}
)

add_executable(main "src/main.cpp")
add_dependencies(main libjt)
target_link_libraries(main PRIVATE libjt)
