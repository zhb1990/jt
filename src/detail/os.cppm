/**
 * 操作系统工具函数接口声明
 * 声明跨平台的进程/线程ID获取和系统错误处理函数
 * 实际实现位于 src/detail/impl/os.cpp
 */

module;

#include "config.h"

export module jt:detail.os;

import std;

export namespace jt::detail {

/**
 * 获取当前进程ID
 * 实际实现见 src/detail/impl/os.cpp 的 pid() 函数
 */
[[nodiscard]] JT_API int pid();

/**
 * 获取当前线程ID（64位无符号整数）
 * 实际实现见 src/detail/impl/os.cpp 的 tid() 函数
 */
[[nodiscard]] JT_API std::uint64_t tid();

/**
 * 获取适当的系统错误类别
 * Windows平台返回自定义system_error_category
 * 其他平台返回std::system_category()
 * 实际实现见 src/detail/impl/os.cpp 的 system_category() 函数
 */
[[nodiscard]] JT_API const std::error_category& system_category() noexcept;

/**
 * 获取当前可执行文件的文件系统路径
 * @param ec 错误代码输出参数（成功时清除）
 * @return 成功时的可执行文件路径，失败时为空路径
 * 实际实现见 src/detail/impl/os.cpp 的 program_location() 函数
 */
[[nodiscard]] JT_API std::filesystem::path program_location(
    std::error_code& ec);

}  // namespace jt::detail