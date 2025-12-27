export module jt.detail.cache_line;

export namespace jt::detail {
#if defined(__powerpc64__) || defined(_ARCH_PPC64) || defined(__PPC64__)
inline constexpr auto cache_line_bytes = 128;
#else
inline constexpr auto cache_line_bytes = 64;
#endif
}  // namespace jt::detail
