add_executable(jt_log_benchmark log.cpp)
target_link_libraries(jt_log_benchmark PRIVATE jt::jt)

jt_configure_executable(jt_log_benchmark)
