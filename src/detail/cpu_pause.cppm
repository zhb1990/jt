/**
 * CPU暂停/让步函数实现
 * 提供跨平台的CPU暂停指令，用于自旋等待中降低功耗和总线占用
 */

module;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || \
    defined(_M_IX86)
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#endif

export module jt:detail.cpu_pause;

export namespace jt::detail {

/**
 * CPU暂停函数
 * 在自旋等待循环中调用此函数可以:
 *   1. 降低功耗消耗
 *   2. 减少对内存总线的占用
 *   3. 提供给其他硬件线程执行机会（在SMT系统中）
 * 平台特定实现:
 *   x86/x64: PAUSE指令 (_mm_pause 或 __builtin_ia32_pause)
 *   ARM: YIELD指令或__yield()内建函数
 *   其他: 空操作
 */
inline void cpu_pause() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || \
    defined(_M_IX86)
#if defined(_MSC_VER)
  // MSVC内建函数生成PAUSE指令
  _mm_pause();
#else
  // GCC/Clang内建函数生成PAUSE指令
  __builtin_ia32_pause();
#endif
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#if (defined(__ARM_ARCH) && __ARM_ARCH >= 7) || defined(__aarch64__)
  // ARMv7及以上: YIELD指令让出执行单元
  asm volatile("yield" ::: "memory");
#elif defined(_M_ARM64)
  // MSVC on ARM64: __yield()内建函数
  __yield();
#else
  // 较旧ARM架构: NOP指令
  asm volatile("nop" ::: "memory");
#endif
#else
  // 其他平台: 空操作
  // empty
#endif
}

}  // namespace jt::detail