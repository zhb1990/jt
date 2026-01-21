cmake_minimum_required(VERSION 4.1.2)

if(${CMAKE_SIZEOF_VOID_P} MATCHES "4")
    message(FATAL_ERROR "only support x64 toolchain")
endif()

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 检测是否为 Apple 平台，并设置 SDKROOT
if(APPLE)
    find_program(CMAKE_XCRUN xcrun REQUIRED)
    execute_process(
        COMMAND ${CMAKE_XCRUN} --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT}")

    find_program(CMAKE_BREW brew REQUIRED)
    execute_process(
        COMMAND ${CMAKE_BREW} --prefix llvm
        OUTPUT_VARIABLE CMAKE_LLVM_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CMAKE_CXX_COMPILER_ID_ARG1 "-resource-dir=${CMAKE_LLVM_PREFIX}/lib/c++")
endif()

if(WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>"
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

project(jt)

find_package(lz4 CONFIG REQUIRED)
if(NOT lz4_FOUND)
    message(WARNING "lz4 not found.")
endif()

find_package(asio CONFIG REQUIRED)
if(NOT asio_FOUND)
    message(WARNING "asio not found.")
endif()

find_package(simdjson CONFIG REQUIRED)
if(NOT simdjson_FOUND)
    message(WARNING "simdjson not found.")
endif()

set(JT_MODULES
    "src/jt.cppm"

    "src/types/writable_buffer.cppm"

    "src/detail/cache_line.cppm"
    "src/detail/cpu_pause.cppm"
    "src/detail/memory.cppm"
    "src/detail/buffer.cppm"
    "src/detail/intrusive_queue.cppm"
    "src/detail/atomic_intrusive_queue.cppm"
    "src/detail/intrusive_mpsc_queue.cppm"
    "src/detail/metric_value.cppm"
    "src/detail/os.cppm"
    "src/detail/string.cppm"
    "src/detail/vector.cppm"
    "src/detail/deque.cppm"
    "src/detail/unordered_map.cppm"

    "src/log/level.cppm"
    "src/log/fwd.cppm"
    "src/log/logger.cppm"
    "src/log/message.cppm"
    "src/log/service.cppm"
    "src/log/formatter.cppm"
    "src/log/sink.cppm"
    "src/log/default_formatter.cppm"
    "src/log/sink_console.cppm"
    "src/log/sink_file.cppm"
    "src/log/functions.cppm"
)

set(JT_HEADERS "src/detail/config.h")

set(JT_SOURCES
    "src/detail/impl/buffer.cpp"
    "src/detail/impl/memory.cpp"
    "src/detail/impl/os.cpp"

    "src/log/impl/logger.cpp"
    "src/log/impl/service.cpp"
    "src/log/impl/sink.cpp"
    "src/log/impl/sink_console.cpp"
    "src/log/impl/sink_file.cpp"
)

add_library(libjt SHARED)
target_link_libraries(libjt PRIVATE simdjson::simdjson lz4::lz4)
set_target_properties(libjt PROPERTIES PREFIX "")
if(APPLE)
    target_link_directories(libjt PUBLIC ${CMAKE_LLVM_PREFIX}/lib/c++)
endif()
target_sources(libjt
    PRIVATE ${JT_HEADERS} ${JT_SOURCES}
    PUBLIC FILE_SET CXX_MODULES FILES ${JT_MODULES}
)
if(WIN32)
    target_compile_definitions(libjt PRIVATE JT_DLL_EXPORT)
endif()

add_executable(main "src/main.cpp")
add_dependencies(main libjt)
target_link_libraries(main PRIVATE libjt)
