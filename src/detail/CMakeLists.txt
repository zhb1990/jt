jt_modules(PRIVATE
    platform/cache_line.cppm platform/cpu_pause.cppm platform/os.cppm
    metrics/metric_value.cppm concurrency/intrusive_queue.cppm
    concurrency/atomic_intrusive_queue.cppm concurrency/intrusive_mpsc_queue.cppm)
target_sources(libjt PRIVATE platform/os.cpp platform/config.h platform/win32.h)
