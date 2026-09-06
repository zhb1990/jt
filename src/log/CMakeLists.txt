find_package(lz4 CONFIG REQUIRED GLOBAL)
find_package(RapidJSON CONFIG REQUIRED GLOBAL)
target_link_libraries(libjt PRIVATE lz4::lz4 RapidJSON)
jt_modules(PUBLIC
    log.cppm level.cppm record.cppm formatter.cppm functions.cppm
    core/core.cppm core/fwd.cppm core/logger.cppm core/service.cppm
    sinks/sink.cppm sinks/console/console.cppm sinks/file/file.cppm)
jt_modules(PRIVATE
    core/detail/message.cppm core/detail/service_impl.cppm
    core/detail/writer.cppm core/detail/archive.cppm detail/default_formatter.cppm)
target_sources(libjt PRIVATE
    formatter.cpp core/logger.cpp core/service.cpp core/detail/service_impl.cpp
    core/detail/writer.cpp core/detail/archive.cpp
    sinks/sink.cpp sinks/console/console.cpp sinks/file/file.cpp)
