// 缓存行大小定义
// 定义缓存行大小以用于避免伪共享的对齐

export module jt:detail.cache_line;

export namespace jt::detail {
// 根据架构定义缓存行大小
// PowerPC架构使用128字节缓存行，其他架构使用64字节
#if defined(__powerpc64__) || defined(_ARCH_PPC64) || defined(__PPC64__)
inline constexpr auto cache_line_bytes = 128;
#else
inline constexpr auto cache_line_bytes = 64;
#endif
}  // namespace jt::detail