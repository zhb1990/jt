export module jt.cache_line;

export namespace jt {
#if defined(__powerpc64__) || defined(_ARCH_PPC64) || defined(__PPC64__)
inline constexpr auto cache_line_bytes = 128;
#else
inline constexpr auto cache_line_bytes = 64;
#endif
}  // namespace jt
