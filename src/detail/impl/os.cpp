module;

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

// module jt:detail.os;
module jt;

import std;

namespace jt::detail {

int pid() {
#if defined(_WIN32)
  thread_local const int v = _getpid();
#else
  thread_local const int v = getpid();
#endif
  return v;
}

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

#if defined(_WIN32)
class system_error_category
    : public std::error_category {  // categorize a generic error
 public:
  const char* name() const noexcept override {
    return "windows_system_error_category";
  }

  std::string message(const int code) const override {
    if (code == 0) return "no error";

    LPWSTR buffer = nullptr;
    const DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    if (size == 0 || !buffer) return "unknown error";

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

const std::error_category& system_category() noexcept {
#if defined(_WIN32)
  static system_error_category category;
  return category;
#else
  return std::system_category();
#endif
}

std::filesystem::path program_location(std::error_code& ec) {
  ec.clear();
#if defined(_WIN32)

  std::ignore = GetLastError();
  const DWORD len = GetModuleFileNameW(nullptr, nullptr, 0);
  auto err = GetLastError();
  if (err != ERROR_SUCCESS) {
    ec.assign(static_cast<int>(err), system_category());
    return {};
  }

  std::wstring p(len, L'\0');
  GetModuleFileNameW(nullptr, p.data(), len);
  err = GetLastError();
  if (err != ERROR_SUCCESS) {
    ec.assign(static_cast<int>(err), system_category());
    return {};
  }

  return std::filesystem::canonical(p.data(), ec);
#elif defined(macintosh) || defined(Macintosh) || \
    (defined(__APPLE__) && defined(__MACH__))
  std::vector<char> buffer(PROC_PIDPATHINFO_MAXSIZE);
  int ret = proc_pidpath(getpid(), buffer.data(),
                         static_cast<std::uint32_t>(buffer.size()));
  if (ret <= 0) {
    ec = std::error_code(errno, std::generic_category());
    return {};
  }

  return std::filesystem::path(buffer.data());
#else
  return std::filesystem::read_symlink("/proc/self/exe", ec);
#endif
}

}  // namespace jt::detail