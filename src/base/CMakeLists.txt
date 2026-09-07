find_package(mimalloc CONFIG REQUIRED GLOBAL)
target_link_libraries(libjt PRIVATE
    $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>)
jt_modules(PUBLIC base.cppm memory.cppm buffer.cppm containers.cppm concepts.cppm)
target_sources(libjt PRIVATE memory.cpp memory_backend.cpp buffer.cpp)
