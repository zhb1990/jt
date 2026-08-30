/**
 * 特定于操作系统的工具函数实现
 * 提供对进程/线程ID和系统错误处理的跨平台访问
 */

/**
 * 开始模块序言
 */
module;

/**
 * 平台特定的包含
 */
#if defined(_WIN32)
#include <Windows.h>
#include <process.h>
#elif defined(macintosh) || defined(Macintosh) || \
    (defined(__APPLE__) && defined(__MACH__))  // mac ios
#include <libproc.h>
#include <pthread.h>
#include <unistd.h>

#include <cerrno>
#else
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#endif

/**
 * 声明此为jt模块的一部分（不是独立的子模块）
 * // module jt:detail.os;
 */
module jt;

import std;

namespace jt::detail {

/**
 * 返回当前进程ID
 * 使用thread_local在首次调用后缓存值
 */
int pid() {
#if defined(_WIN32)
  thread_local const int v = _getpid();
#else
  thread_local const int v = getpid();
#endif
  return v;
}

/**
 * 以64位无符号整数形式返回当前线程ID
 * 使用thread_local在首次调用后缓存值
 * 平台特定的实现：
 *   Windows: GetCurrentThreadId()
 *   macOS: pthread_threadid_np()
 *   Linux: syscall(SYS_gettid)
 */
std::uint64_t tid() {
#if defined(_WIN32)
  thread_local const auto v = static_cast<std::uint64_t>(GetCurrentThreadId());
#elif defined(macintosh) || defined(Macintosh) || \
    (defined(__APPLE__) && defined(__MACH__))
  thread_local const std::uint64_t v = []() {
    uint64_t t;
    pthread_threadid_np(nullptr, &t);
    return t;
  }();
#else
  thread_local const std::uint64_t v = syscall(SYS_gettid);
#endif
  return v;
}

/**
 * Windows特定的错误类别，用于将系统错误代码映射到消息
 */
#if defined(_WIN32)
class system_error_category : public std::error_category {  // 分类一个通用错误
 public:
  /**
   * 获取错误类别名称
   * @return 错误类别名称
   */
  const char* name() const noexcept override {
    return "windows_system_error_category";
  }

  /**
   * 将Windows错误代码格式化为UTF-8字符串消息
   * @param code 错误代码
   * @return 错误消息
   */
  std::string message(const int code) const override {
    if (code == 0) return "no error";

    LPWSTR buffer = nullptr;
    const DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    if (size == 0 || !buffer) return "unknown error";

    // 将宽字符缓冲区转换为UTF-8
    const int len =
        WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast<int>(size), nullptr,
                            0, nullptr, nullptr);
    if (len <= 0) return "unknown error";

    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast<int>(size),
                        result.data(), len, nullptr, nullptr);
    return result;
  }
};

#endif

/**
 * 根据当前平台返回适当的系统错误类别
 * Windows: 自定义system_error_category
 * 其他平台: std::system_category()
 */
const std::error_category& system_category() noexcept {
#if defined(_WIN32)
  static system_error_category category;
  return category;
#else
  return std::system_category();
#endif
}

/**
 * 获取当前可执行文件的文件系统路径
 * @param ec 错误代码输出参数（成功时清除）
 * @return 成功时的可执行文件路径，失败时为空路径
 */
std::filesystem::path program_location(std::error_code& ec) {
  ec.clear();
#if defined(_WIN32)

  /**
   * 获取可执行文件路径所需的缓冲区大小
   */
  std::ignore = GetLastError();
  const DWORD len = GetModuleFileNameW(nullptr, nullptr, 0);
  auto err = GetLastError();
  if (err != ERROR_SUCCESS) {
    ec.assign(static_cast<int>(err), system_category());
    return {};
  }

  /**
   * 获取可执行文件路径
   */
  std::wstring p(len, L'\0');
  GetModuleFileNameW(nullptr, p.data(), len);
  err = GetLastError();
  if (err != ERROR_SUCCESS) {
    ec.assign(static_cast<int>(err), system_category());
    return {};
  }

  /**
   * 返回规范化路径
   */
  return std::filesystem::canonical(p.data(), ec);
#elif defined(macintosh) || defined(Macintosh) || \
    (defined(__APPLE__) && defined(__MACH__))
  /**
   * macOS: 使用proc_pidpath获取可执行文件路径
   */
  std::vector<char> buffer(PROC_PIDPATHINFO_MAXSIZE);
  int ret = proc_pidpath(getpid(), buffer.data(),
                         static_cast<std::uint32_t>(buffer.size()));
  if (ret <= 0) {
    ec = std::error_code(errno, std::generic_category());
    return {};
  }

  return std::filesystem::path(buffer.data());
#else
  /**
   * Linux: 读取/proc/self/exe符号链接
   */
  return std::filesystem::read_symlink("/proc/self/exe", ec);
#endif
}

}  // namespace jt::detail