cmake_minimum_required(VERSION 4.3.0)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>"
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

project(jt)

if(${CMAKE_SIZEOF_VOID_P} MATCHES "4")
    message(FATAL_ERROR "only support x64 toolchain")
endif()

find_package(lz4 CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)

set(JT_PUBLIC_MODULES
    "src/jt.cppm"

    "src/types/writable_buffer.cppm"

    "src/detail/memory.cppm"
    "src/detail/buffer.cppm"
    "src/detail/vector.cppm"

    "src/log/fwd.cppm"
    "src/log/level.cppm"
    "src/log/record.cppm"
    "src/log/formatter.cppm"
    "src/log/sink.cppm"
    "src/log/logger.cppm"
    "src/log/service.cppm"
    "src/log/sink_console.cppm"
    "src/log/sink_file.cppm"
    "src/log/functions.cppm"
)

set(JT_PRIVATE_MODULES
    "src/detail/cache_line.cppm"
    "src/detail/cpu_pause.cppm"
    "src/detail/metric_value.cppm"
    "src/detail/os.cppm"
    "src/detail/string.cppm"
    "src/detail/deque.cppm"
    "src/detail/unordered_map.cppm"
    "src/detail/intrusive_queue.cppm"
    "src/detail/atomic_intrusive_queue.cppm"
    "src/detail/intrusive_mpsc_queue.cppm"

    "src/log/message.cppm"
    "src/log/default_formatter.cppm"
    "src/log/impl/service_impl.cppm"
)

set(JT_HEADERS "src/detail/config.h")

set(JT_SOURCES
    "src/detail/impl/buffer.cpp"
    "src/detail/impl/memory.cpp"
    "src/detail/impl/os.cpp"

    "src/log/impl/logger.cpp"
    "src/log/impl/service.cpp"
    "src/log/impl/formatter.cpp"
    "src/log/impl/sink.cpp"
    "src/log/impl/sink_console.cpp"
    "src/log/impl/sink_file.cpp"
    "src/log/impl/service_impl.cpp"
)

add_library(libjt SHARED)
target_link_libraries(libjt PRIVATE
    $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>
    RapidJSON
    lz4::lz4
)
set_target_properties(libjt PROPERTIES PREFIX "")

target_sources(libjt
    PUBLIC
        FILE_SET jt_public_modules
        TYPE CXX_MODULES
        FILES ${JT_PUBLIC_MODULES}
    PRIVATE
        FILE_SET jt_private_modules
        TYPE CXX_MODULES
        FILES ${JT_PRIVATE_MODULES}
    PRIVATE
        ${JT_HEADERS}
        ${JT_SOURCES}
)
if(WIN32 OR CYGWIN)
    # BMI bakes in this define; a consumer-side JT_DLL_IMPORT would not re-expand JT_API.
    target_compile_definitions(libjt PRIVATE JT_DLL_EXPORT)
else()
    target_compile_definitions(libjt PRIVATE JT_LIB_VISIBILITY)
    set_target_properties(libjt PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

add_executable(main "src/main.cpp")
add_dependencies(main libjt)
target_link_libraries(main PRIVATE libjt)
