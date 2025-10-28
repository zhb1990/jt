module;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#endif

export module jt.detail.cpu_pause;

export namespace jt::detail {

inline void cpu_pause() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(_MSC_VER)
    _mm_pause();
#else
    __builtin_ia32_pause();
#endif
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#if (defined(__ARM_ARCH) && __ARM_ARCH >= 7) || defined(__aarch64__)
    asm volatile("yield" ::: "memory");
#elif defined(_M_ARM64)
    __yield();
#else
    asm volatile("nop" ::: "memory");
#endif
#else
    // empty
#endif
}

}  // namespace jt
