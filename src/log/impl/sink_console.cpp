module;

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>

#include <cstdio>
#endif

// module jt:log.sink.console;
module jt;

import std;

namespace jt::log {

#ifndef _WIN32

// Determine if the terminal supports colors
// Based on: https://github.com/agauniyal/rang/
bool is_color_terminal() noexcept {
  static const bool result = []() {
    const char* env_colorterm_p = std::getenv("COLORTERM");
    if (env_colorterm_p != nullptr) {
      return true;
    }

    constexpr std::array terms{"ansi",  "color",   "console",   "cygwin",
                               "gnome", "konsole", "kterm",     "linux",
                               "msys",  "putty",   "rxvt",      "screen",
                               "vt100", "xterm",   "alacritty", "vt102"};
    const char* env_term_p = std::getenv("TERM");
    if (env_term_p == nullptr) {
      return false;
    }

    return std::any_of(terms.begin(), terms.end(), [&](const char* term) {
      return std::strstr(env_term_p, term) != nullptr;
    });
  }();

  return result;
}

// Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
bool in_terminal(std::FILE* file) { return ::isatty(::fileno(file)) != 0; }

#endif

class sink_console_impl {
 public:
#ifdef _WIN32
  explicit sink_console_impl(const DWORD handle) {
    handle_ = GetStdHandle(handle);
    DWORD mode;
    enable_color_ = GetConsoleMode(handle_, &mode) != 0;
  }
#else
  explicit sink_console_impl(std::FILE* handle) : handle_(handle) {
    enable_color_ = is_color_terminal() && in_terminal(handle_);
  }
#endif

  void write(const sink::time_point& point, const detail::buffer_1k& buf,
             std::size_t& color_start, std::size_t& color_stop) {
    std::lock_guard lock(mutex_);
    write_range(buf, 0, buf.readable());
  }

  void flush_unlock() {
#ifndef _WIN32
    std::lock_guard lock(mutex_);
    std::fflush(handle_);
#endif
  }

 private:
  void write_range(const detail::buffer_1k& buf, const std::size_t start,
                   const std::size_t end) {
#ifdef _WIN32
    auto* ptr = reinterpret_cast<const char*>(buf.begin_read()) + start;
    const int str_len = static_cast<int>(end - start);
    const auto len = MultiByteToWideChar(CP_UTF8, 0, ptr, str_len, nullptr, 0);
    if (len <= 0) return;

    std::wstring temp(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, ptr, str_len, temp.data(),
                        static_cast<int>(temp.size()));
    DWORD written;
    WriteConsoleW(handle_, temp.c_str(), static_cast<DWORD>(temp.size()),
                  &written, nullptr);
#else
    fwrite(buf.begin_read() + start, sizeof(char), end - start, handle_);
#endif
  }

  static std::mutex mutex_;
#ifdef _WIN32
  HANDLE handle_{INVALID_HANDLE_VALUE};
#else
  std::FILE* handle_{nullptr};
#endif
  bool enable_color_{false};
};

std::mutex sink_console_impl::mutex_;

#ifdef _WIN32
sink_console_impl console_stdout(STD_OUTPUT_HANDLE);
sink_console_impl console_stderr(STD_ERROR_HANDLE);
#else
sink_console_impl console_stdout(stdout);
sink_console_impl console_stderr(stderr);
#endif

sink_stdout::sink_stdout() : impl_(console_stdout) {}

sink_stdout::~sink_stdout() noexcept = default;

void sink_stdout::write(const time_point& point, const detail::buffer_1k& buf,
                        std::size_t& color_start, std::size_t& color_stop) {
  return impl_.write(point, buf, color_start, color_stop);
}

void sink_stdout::flush_unlock() { return impl_.flush_unlock(); }

sink_stderr::sink_stderr() : impl_(console_stderr) {}

sink_stderr::~sink_stderr() noexcept = default;

void sink_stderr::write(const time_point& point, const detail::buffer_1k& buf,
                        std::size_t& color_start, std::size_t& color_stop) {
  return impl_.write(point, buf, color_start, color_stop);
}

void sink_stderr::flush_unlock() { return impl_.flush_unlock(); }

}  // namespace jt::log